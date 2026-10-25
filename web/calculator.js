(function(root){
 'use strict';
 const channels={Direct:{transaction:0,processing:0,payment:0,listing:0},'Etsy US':{transaction:6.5,processing:3,payment:.25,listing:.2},Custom:{transaction:0,processing:0,payment:0,listing:0}};
 function defaults(type){const base={type,name:'',laborH:0,laborM:0,extraH:0,extraM:0,material:0,printH:0,printM:0,batch:1,margin:0,extras:0,shipping:0,channel:'Etsy US',customPrice:'',laborRate:20,packCost:type==='resin'?30:20,packSize:1000,materialBuffer:1.1,printerRate:.5,printerBuffer:1.3,electricity:.14,watts:150,ads:0,tax:0,channels:JSON.parse(JSON.stringify(channels))};if(type==='uv')Object.assign(base,{packCost:29.99,packSize:100,materialBuffer:1,printerRate:0,printerBuffer:1,watts:0,electricity:0,inkMode:'estimate',referencePreset:'custom',referenceMl:0,referenceScale:1,inkPreset:'single',bundleCredit:0,substratePrice:0,substrateQty:0,stockPackCost:0,stockPackQty:1,substrateName:'',printerId:'e1',profiles:[{id:'e1',name:'EufyMake E1',packCost:29.99,packSize:100,materialBuffer:1,printerRate:0,printerBuffer:1,watts:0,electricity:0,inkPreset:'single',bundleCredit:0}],...Object.fromEntries(['C','M','Y','K','W','WF','G'].flatMap(c=>[['ink'+c,0],['rate'+c,'']]))});return base;}
 function calculate(s){
  const keys=['laborH','laborM','extraH','extraM','material','printH','printM','batch','margin','extras','shipping','laborRate','packCost','packSize','materialBuffer','printerRate','printerBuffer','electricity','watts','ads','tax',...(s.type==='uv'?['bundleCredit','substratePrice','substrateQty']:[])];
  for(const k of keys)if(s[k]!==''&&(!Number.isFinite(Number(s[k]))||Number(s[k])<0))return {error:'Enter a valid nonnegative number for '+k+'.'};
  const n=k=>Number(s[k]||0), fallback=k=>s[k]===''?1:n(k);
  const b=fallback('batch');if(b<1||!Number.isInteger(b))return {error:'Batch multiplier must be a whole number of 1 or more.'};
  if(n('material')>0&&n('packSize')<=0)return {error:'Enter a material package size greater than zero.'};
  let uv=s.type==='uv',ink=null;if(uv){ink=inkCost(s);if(ink.error)return ink;if(n('substrateQty')%1!==0)return {error:'Blank quantity must be a whole number.'};}
  const ch=s.channels[s.channel];if(!ch)return {error:'Choose a sales channel.'};
  if(Object.values(ch).some(v=>!Number.isFinite(Number(v))||Number(v)<0))return {error:'Selling fees must be valid nonnegative numbers.'};
  if(n('margin')>=100)return {error:'Target margin must be less than 100%.'};
  const labor=((n('laborH')+n('laborM')/60)*b+n('extraH')+n('extraM')/60)*n('laborRate');
  const material=uv?ink.cost*b*fallback('materialBuffer'):n('material')===0?0:n('material')*b*n('packCost')/n('packSize')*fallback('materialBuffer');
  const substrate=uv?n('substrateQty')*n('substratePrice'):0;
  const machine=(n('printH')+n('printM')/60)*b*(n('printerRate')*fallback('printerBuffer')+n('watts')/1000*n('electricity'));
  const extras=n('extras')+n('shipping'),cost=labor+material+machine+extras+substrate;
  if(!Number.isFinite(cost))return {error:'The entered values are too large.'};
  const rate=(Number(ch.transaction)+Number(ch.processing))/100,fixed=Number(ch.payment)+Number(ch.listing)+n('tax')*Number(ch.processing)/100,ad=s.channel==='Etsy US'?n('ads')/100:0;
  const active=(uv&&(ink.volume>0||substrate>0))||['laborH','laborM','extraH','extraM','material','printH','printM','extras','shipping'].filter(k=>!uv||k!=='material').some(k=>n(k)>0);
  const round=p=>Math.ceil(Number(p.toFixed(10))*100-1e-9)/100;
  const solve=m=>{const d=1-rate-m;if(d<=0)return null;const capped=(cost+fixed+100)/d;const uncapped=d>ad?(cost+fixed)/(d-ad):Infinity;return round(ad===0?uncapped:Math.min(uncapped,capped));};
  const fees=p=>p*rate+fixed+Math.min(p*ad,100);
  const quote=active?solve(n('margin')/100):0,breakEven=active?solve(0):0;
  const selling=quote===null?null:active?fees(quote):0,profit=quote===null?null:quote-selling-cost;
  let custom=null;
  if(s.customPrice!==''){let p=Number(s.customPrice);if(!Number.isFinite(p)||p<0)return {error:'Custom price must be zero or a positive number.'};const f=fees(p),pr=p-f-cost;custom={price:p,fees:f,profit:pr,margin:p>0?pr/p:null,owner:labor+pr};}
  return {labor,material,machine,extras,substrate,inkVolume:uv?ink.volume:null,cost,quote,breakEven,fees:selling,profit,margin:quote>0?profit/quote:0,owner:profit===null?null:labor+profit,active,custom};
 }
 const inkChannels=['C','M','Y','K','W','WF','G'];
 const presets={single:{name:'Single cartridge - one-time',cost:29.99,ml:100},sub4:{name:'Single - subscription every 4 weeks',cost:19.99,ml:100},sub8:{name:'Single - subscription every 8 weeks',cost:21.89,ml:100},sub12:{name:'Single - subscription every 12 weeks',cost:23.99,ml:100},bundle:{name:'Ink + cleaning kit - one-time',cost:209.99,ml:600},bundle4:{name:'Ink + cleaning kit - every 4 weeks',cost:139.98,ml:600},bundle8:{name:'Ink + cleaning kit - every 8 weeks',cost:153.29,ml:600},bundle12:{name:'Ink + cleaning kit - every 12 weeks',cost:167.99,ml:600}};
 function inkCost(s){
  const cost=Number(s.packCost||0),size=Number(s.packSize||0),credit=Number(s.bundleCredit||0);
  if(!Number.isFinite(credit)||credit<0||credit>cost)return {error:'The non-ink bundle allocation must be between zero and the purchase price.'};
  if(size<=0)return {error:'Enter an ink capacity greater than zero.'};
  const rate=(cost-credit)/size;let volume=0,total=0;
  if(s.inkMode==='estimate'){
   const reference=Number(s.referenceMl||0),scale=Number(s.referenceScale??1);if(!Number.isFinite(reference)||reference<0||!Number.isFinite(scale)||scale<=0)return {error:'Enter a nonnegative reference ink volume and a scale greater than zero.'};volume=reference*scale;total=volume*rate;
  }else if(s.inkMode==='channels'){
   for(const c of inkChannels){const qty=Number(s['ink'+c]||0),override=s['rate'+c],r=override===''||override===undefined?rate:Number(override);if(!Number.isFinite(qty)||qty<0||!Number.isFinite(r)||r<0)return {error:'Enter valid nonnegative ink quantities and rates.'};volume+=qty;total+=qty*r;}
  }else{volume=Number(s.material||0);total=volume*rate;}
  if(!Number.isFinite(total)||!Number.isFinite(volume))return {error:'Ink amounts are too large.'};return {cost:total,volume,rate};
 }
 const profileKeys=['packCost','packSize','materialBuffer','printerRate','printerBuffer','watts','electricity','inkPreset','bundleCredit',...inkChannels.map(c=>'rate'+c)];
 const references={light:{name:'Light color example (8m 4s)',ml:.23},balanced:{name:'Balanced color example (5m 16s)',ml:1.25},white:{name:'White-heavy example (10m 48s)',ml:4.73},smallWhite:{name:'Small white-backed example (3m 55s)',ml:.48},whiteGloss:{name:'White + gloss example (10m 49s)',ml:4.64},gloss:{name:'Gloss / lenticular example (2h 16m)',ml:33.53}};
 const api={defaults,calculate,inkCost,inkChannels,presets,profileKeys,references};if(typeof module!=='undefined')module.exports=api;else root.PrintCalc=api;
})(typeof window!=='undefined'?window:globalThis);
