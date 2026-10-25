(function(root){
'use strict';
async function quotePDF(s,r,mode,logo=null){
 const lib=typeof PDFLib!=='undefined'?PDFLib:require('./vendor/pdf-lib.min.js');
 const doc=await lib.PDFDocument.create(),font=await doc.embedFont(lib.StandardFonts.Helvetica),bold=await doc.embedFont(lib.StandardFonts.HelveticaBold),page=doc.addPage([612,792]);
 const ink=lib.rgb(.12,.16,.23),muted=lib.rgb(.36,.40,.47),line=lib.rgb(.85,.87,.90),accent=lib.rgb(.25,.32,.64);
 const money=n=>n===null?'Unavailable':new Intl.NumberFormat('en-US',{style:'currency',currency:'USD'}).format(n),percent=n=>n===null?'N/A':(n*100).toFixed(1)+'%';
 const clean=s=>String(s).normalize('NFKD').replace(/[^\x20-\x7E]/g,'?');
 function fit(s,size,max=500){s=clean(s);while(font.widthOfTextAtSize(s,size)>max)s=s.slice(0,-4)+'...';return s;}
 function text(s,x,y,size=11,strong=false,color=ink){page.drawText(clean(s),{x,y,size,font:strong?bold:font,color});}
 function row(label,value,y,strong=false){text(label,48,y,11,strong);const str=clean(value),f=strong?bold:font;text(str,564-f.widthOfTextAtSize(str,11),y,11,strong);}
 function rule(y){page.drawLine({start:{x:48,y},end:{x:564,y},thickness:.7,color:line});}
 if(logo){const image=await (logo.startsWith('data:image/jpeg')?doc.embedJpg(logo):doc.embedPng(logo)),size=image.scaleToFit(140,43);page.drawImage(image,{x:564-size.width,y:737,width:size.width,height:size.height});}
 doc.setTitle((s.name||'3D Print')+' - Quote estimate');doc.setCreator('3D Print Quote Calculator');
 text('3D PRINT QUOTE ESTIMATE',48,744,18,true);text(mode==='uv'?fit('UV printing | '+(s.profiles?.find(p=>p.id===s.printerId)?.name||'UV printer'),11,300):mode==='resin'?'Resin printing':'FDM printing',48,722,11,false,muted);
 text('Prepared '+new Date().toLocaleDateString('en-US'),400,722,10,false,muted);rule(707);
 text(fit(s.name||'Untitled project',15),48,679,15,true);
 page.drawRectangle({x:48,y:576,width:516,height:79,color:lib.rgb(.94,.95,.98)});
 text('RECOMMENDED CUSTOMER TOTAL',63,631,10,true,accent);text(money(r.quote),63,598,28,true);text('Includes shipping. Sales tax excluded.',320,604,10,false,muted);
 text('PROJECT ESTIMATES',48,550,11,true,accent);
 const batch=s.batch===''?1:Number(s.batch),hours=(h,m)=>(Number(h||0)+Number(m||0)/60).toFixed(2)+' hr';
 row(mode==='uv'?(s.inkMode==='estimate'?'Estimated ink per run':'Ink per run'):'Slicer material per run',(mode==='uv'?Number(Number(r.inkVolume||0).toFixed(6)):Number(s.material||0))+' '+(mode==='fdm'?'g':'ml'),528);
 row('Print time per run',hours(s.printH,s.printM),509);if(mode==='uv'){row('Labor: per run / additional job',hours(s.laborH,s.laborM)+' / '+hours(s.extraH,s.extraM),490);row('Batch multiplier',batch+' runs',471);row(fit(s.substrateName||'Blanks, whole job',11,270),Number(s.substrateQty||0)+' x '+money(Number(s.substratePrice||0)),452);}else{row('Hands-on labor per run',hours(s.laborH,s.laborM),490);row('Additional labor, whole job',hours(s.extraH,s.extraM),471);row('Batch multiplier',batch+' runs',452);}
 rule(437);text('COST AND PRICING',48,415,11,true,accent);
 row('Labor',money(r.labor),392);row(mode==='uv'?'Ink':mode==='resin'?'Resin':'Filament',money(r.material),373);row('Printer + electricity',money(r.machine),354);
 row(mode==='uv'?'Blanks + extras':'Extras',money(Number(s.extras||0)+(r.substrate||0)),335);row('Shipping',money(Number(s.shipping||0)),316);row('Total project cost',money(r.cost),293,true);
 row('Sales channel',s.channel,268);row('Target profit margin',Number(s.margin||0)+'%',249);row('Selling fees',money(r.fees),230);
 row('Profit after labor',money(r.profit),211,true);row('Actual profit margin',percent(r.margin),192);row('Owner labor + profit',money(r.owner),173);row('Break-even customer total',money(r.breakEven),154);
 if(r.custom){rule(140);text('CUSTOM PRICE CHECK',48,122,10,true,accent);text('Total '+money(r.custom.price)+'   |   Profit '+money(r.custom.profit)+'   |   Margin '+percent(r.custom.margin),48,104,10);}
 rule(81);text('Internal pricing estimate. Owner labor + profit is before income tax.',48,63,9,false,muted);text(mode==='uv'&&s.inkMode==='estimate'?'USD | Ink allowance estimated from a reference print; actual usage may vary.':'USD | Based on current project inputs and saved rates.',48,49,9,false,muted);
 return doc.save();
}
if(typeof module!=='undefined')module.exports={quotePDF};else root.QuoteExport={quotePDF};
})(typeof window!=='undefined'?window:globalThis);
