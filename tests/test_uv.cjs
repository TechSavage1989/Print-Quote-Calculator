// npm install --no-save jsdom@26.1.0; node test_webview.cjs
// Exercises the actual bundled document with a simulated WebView2 message channel.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {JSDOM,VirtualConsole}=require('jsdom');
const PrintCalc=require('../web/calculator.js');
const PDFLib=require('../web/vendor/pdf-lib.min.js');
const page=fs.readFileSync(path.join(__dirname,'../embedded.html'),'utf8');
const seed={mode:'resin',theme:'dark',states:{resin:PrintCalc.defaults('resin'),fdm:PrintCalc.defaults('fdm')},logo:null};
Object.assign(seed.states.resin,{name:'WebView2 regression quote',material:100,printH:2,laborM:15,extraM:30,batch:3,margin:30,extras:4,shipping:6});
// A real PNG exercises logo persistence and the PDF image path.
seed.logo='data:image/png;base64,'+fs.readFileSync(path.join(__dirname,'../assets/Print_Quote_Icon.png')).toString('base64');
const errors=[],files=[],themes=[];let nextResult={saved:true},listener;
function create(existing,initial=seed){
 const console=new VirtualConsole();console.on('jsdomError',error=>errors.push(error.message));
 return new JSDOM(page,{url:'https://printquote.local/',runScripts:'dangerously',pretendToBeVisual:true,virtualConsole:console,
  beforeParse(window){
   window.TextEncoder=TextEncoder;window.TextDecoder=TextDecoder;
   window.matchMedia=()=>({matches:false});
   window.__PrintQuoteInitial=JSON.parse(JSON.stringify(initial));
   window.URL.createObjectURL=()=> 'blob:https://printquote.local/test';window.URL.revokeObjectURL=()=>{};
   window.HTMLDialogElement.prototype.showModal=function(){this.open=true;};
   window.HTMLDialogElement.prototype.close=function(){this.open=false;};
   window.HTMLElement.prototype.scrollIntoView=function(){};
   if(existing)window.localStorage.setItem('print-quote-v1',existing);
   window.open=()=>{throw Error('Desktop export must not open a browser popup');};
   window.chrome={webview:{addEventListener(type,fn){assert.equal(type,'message');listener=fn;},postMessage(message){
    if(message.type==='theme')themes.push(message.dark);
    if(message.type==='save-file'){
     files.push(message);const result=nextResult;nextResult={saved:true};
     window.setTimeout(()=>listener({data:{id:message.id,...result}}),0);
    }
   }}};
  }});
}
async function until(predicate){for(let i=0;i<500;i++){if(predicate())return;await new Promise(r=>setTimeout(r,10));}throw Error('Timed out waiting for app action');}
function input(w,id,value){const el=w.document.getElementById(id);el.value=value;el.dispatchEvent(new w.Event('input',{bubbles:true}));}
(async()=>{
 let dom=create(),w=dom.window,$=id=>w.document.getElementById(id);await until(()=>$('material'));
 $('uv').click();assert.equal($('uv').getAttribute('aria-pressed'),'true');assert.equal($('type-badge').textContent,'EufyMake E1');
 assert.equal($('material').readOnly,true);$('uv-open-ink').click();assert.equal($('uv-ink-mode').value,'estimate');
 input(w,'referencePreset','white');assert.equal($('referenceMl').value,'4.73');assert(Math.abs(w.currentQuote.inkVolume-4.73)<1e-8);
 input(w,'referenceScale','2');assert(Math.abs(w.currentQuote.inkVolume-9.46)<1e-8);assert.equal($('material').value,'9.46');assert.match($('status').textContent,/estimated/);
 input(w,'referenceScale','0');assert.match($('status').textContent,/scale/);assert($('download-pdf').disabled);input(w,'referenceScale','1');
 input(w,'referenceMl','2.5');assert.equal($('referencePreset').value,'custom');assert.equal(JSON.parse(w.localStorage.getItem('print-quote-v1')).states.uv.referencePreset,'custom');
 $('close-ink').click();$('download-pdf').click();await until(()=>$('export-status').textContent==='PDF saved.');fs.writeFileSync(path.join(__dirname,'UV_Estimate_Check.pdf'),Buffer.from(files.at(-1).data,'base64'));const estimateStored=w.localStorage.getItem('print-quote-v1');const originalListener=listener;let check=create(estimateStored);await until(()=>check.window.document.getElementById('material'));assert.equal(check.window.document.getElementById('material').value,'2.5');check.window.close();listener=originalListener;
 $('uv-open-ink').click();input(w,'uv-ink-mode','total');$('close-ink').click();assert.equal($('material').readOnly,false);
 input(w,'material','4.73');input(w,'batch','2');input(w,'substrateQty','2');input(w,'substratePrice','1.5');input(w,'laborM','15');input(w,'printM','10.8');
 let r=w.currentQuote;assert(Math.abs(r.material-4.73*.2999*2)<1e-8);assert.equal(r.substrate,3);assert.equal(r.labor,10);assert.equal(r.machine,0);
 $('open-settings').click();input(w,'uv-ink-preset','sub4');assert.equal($('packCost').value,'19.99');assert.equal($('unit-rate').textContent,'$0.1999');$('close-settings').click();
 $('uv-open-stock').click();input(w,'stockPackCost','24.99');input(w,'stockPackQty','10');input(w,'uv-stock-name','Birch panel / sample supplier');$('uv-stock-apply').click();assert.equal($('stock-dialog').open,false);assert.equal($('substratePrice').value,'2.499');assert(Math.abs(w.currentQuote.substrate-4.998)<1e-8);
 $('uv-open-ink').click();input(w,'uv-ink-mode','channels');for(const c of ['C','M','Y','K','G'])input(w,'ink'+c,'.2');input(w,'inkW','3.72');input(w,'rateW','.15');assert.equal($('material').readOnly,true);assert(Math.abs(w.currentQuote.inkVolume-4.72)<1e-8);assert(Math.abs(w.currentQuote.material-(3.72*.15+1*.1999)*2)<1e-8);$('close-ink').click();
 $('open-settings').click();input(w,'uv-ink-preset','bundle4');assert.equal($('packCost').value,'139.98');assert.equal($('packSize').value,'600');input(w,'bundleCredit','20');let expected=(139.98-20)/600*4.72*2;assert(Math.abs(w.currentQuote.material-expected)<1e-8);
 input(w,'bundleCredit','200');assert($('download-pdf').disabled);assert.match($('status').textContent,/allocation/);input(w,'bundleCredit','20');
 $('uv-new-profile').value='Future UV printer';$('uv-add-profile').click();const second=$('uv-profile').value;assert.notEqual(second,'e1');input(w,'packCost','45');input(w,'bundleCredit','0');input(w,'printerRate','1.2');$('uv-profile').value='e1';$('uv-profile').dispatchEvent(new w.Event('change'));assert.equal($('packCost').value,'139.98');assert.equal($('printerRate').value,'0');$('uv-profile').value=second;$('uv-profile').dispatchEvent(new w.Event('change'));assert.equal($('packCost').value,'45');assert.equal($('printerRate').value,'1.2');$('uv-profile').value='e1';$('uv-profile').dispatchEvent(new w.Event('change'));$('close-settings').click();
 input(w,'name','UV wood print');input(w,'margin','30');input(w,'customPrice','80');$('download-pdf').click();await until(()=>$('export-status').textContent==='PDF saved.');const bytes=Buffer.from(files.at(-1).data,'base64');fs.writeFileSync(path.join(__dirname,'UV_Quote_Check.pdf'),bytes);assert.equal((await PDFLib.PDFDocument.load(bytes)).getPageCount(),1);
 $('open-settings').click();$('export-settings').click();await until(()=>$('transfer-status').textContent==='Settings exported.');const exported=JSON.parse(Buffer.from(files.at(-1).data,'base64'));assert.equal(exported.mode,'uv');assert.equal(exported.states.uv.profiles.length,2);assert.equal(exported.states.uv.substrateName,'Birch panel / sample supplier');$('close-settings').click();
 const stored=w.localStorage.getItem('print-quote-v1');dom.window.close();dom=create(stored);w=dom.window;$=id=>w.document.getElementById(id);await until(()=>$('material'));assert.equal($('uv').getAttribute('aria-pressed'),'true');assert.equal($('name').value,'UV wood print');assert.equal($('material').readOnly,true);assert.equal(JSON.parse(w.localStorage.getItem('print-quote-v1')).states.uv.profiles.length,2);
 // Older two-mode exports import without losing compatibility, and new three-mode exports restore profiles.
 $('open-settings').click();const fileInput=$('settings-file');Object.defineProperty(fileInput,'files',{configurable:true,value:[{size:1000,text:async()=>JSON.stringify(seed)}]});fileInput.dispatchEvent(new w.Event('change'));await until(()=>$('transfer-status').textContent==='Settings imported.');assert.equal($('resin').getAttribute('aria-pressed'),'true');
 Object.defineProperty(fileInput,'files',{configurable:true,value:[{size:1000,text:async()=>JSON.stringify(exported)}]});fileInput.dispatchEvent(new w.Event('change'));await until(()=>$('uv').getAttribute('aria-pressed')==='true');assert.equal($('name').value,'UV wood print');
 assert.deepEqual(errors,[]);dom.window.close();console.log('PASS: UV reference estimates, scaling, validation, persistence and total/per-ink pricing, all supplied purchase categories, bundle allocation, stock pack cost, quantities, labor, profile isolation, PDF export, settings round-trip, legacy import and restart persistence.');
})().catch(error=>{console.error(error);process.exitCode=1;});
