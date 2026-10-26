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
 let dom=create(),w=dom.window,$=id=>w.document.getElementById(id);
 await until(()=>$('quote').textContent!=='$0.00');
 const expected=PrintCalc.calculate(seed.states.resin);
 assert.equal(w.currentQuote.quote,expected.quote);assert(w.document.body.classList.contains('dark'));assert(themes.includes(true));
 assert.equal($('logo-preview').src,seed.logo);assert.equal($('settings-dialog').open,false);
 assert.equal($('material').value,'100');assert.equal($('download-pdf').disabled,false);
 $('download-pdf').click();await until(()=>$('export-status').textContent==='PDF saved.');
 assert.equal(files.at(-1).kind,'pdf');assert.equal(files.at(-1).open,false);
 const bytes=Buffer.from(files.at(-1).data,'base64');assert.equal(bytes.subarray(0,5).toString(),'%PDF-');
 const pdf=await PDFLib.PDFDocument.load(bytes);assert.equal(pdf.getPageCount(),1);assert.match(pdf.getTitle(),/WebView2 regression quote/);
 assert.match(Buffer.from(bytes).toString('latin1'),/\/Subtype \/Image/);
 nextResult={canceled:true};$('download-pdf').click();await until(()=>$('export-status').textContent==='Export canceled.');
 nextResult={error:'Simulated file write failure'};$('download-pdf').click();await until(()=>$('export-status').textContent.startsWith('PDF export failed.'));
 $('print').click();await until(()=>$('export-status').textContent.includes('saved and opened'));assert.equal(files.at(-1).open,true);
 assert.equal($('open-converters').getAttribute('aria-controls'),'converters-dialog');
 assert.equal($('settings-dialog').querySelector('#converter'),null);
 assert.equal($('converters-dialog').open,false);
 $('open-converters').click();assert($('converters-dialog').open);assert.equal($('settings-dialog').open,false);
 assert.equal(w.document.activeElement.id,'unit-value');
 $('close-converters').click();assert.equal($('converters-dialog').open,false);
 $('open-converters').click();assert($('converters-dialog').open);
 input(w,'unit-value',1000);assert.match($('unit-results').textContent,/1,000/);
 input(w,'conv-length',10);assert.equal($('use-converted-weight').disabled,false);const mass=w.convertedMass;assert(mass>0);
 $('use-converted-weight').click();assert.equal($('converters-dialog').open,false);assert.equal($('settings-dialog').open,false);assert.equal($('fdm').getAttribute('aria-pressed'),'true');
 assert(Math.abs(Number($('material').value)-mass)<0.000001);
 input(w,'customPrice','0');assert.equal($('custom-results').hidden,false);assert.equal($('custom-status').textContent,'Below break even');
 $('theme').click();assert(!w.document.body.classList.contains('dark'));assert.equal(themes.at(-1),false);
 $('open-settings').click();assert($('settings-dialog').open);assert.equal($('converters-dialog').open,false);$('export-settings').click();await until(()=>$('transfer-status').textContent==='Settings exported.');
 const settings=JSON.parse(Buffer.from(files.at(-1).data,'base64').toString());assert.equal(settings.mode,'fdm');assert.equal(settings.logo,seed.logo);assert.equal(settings.theme,'light');
 $('close-settings').click();assert.equal($('settings-dialog').open,false);
 const stored=w.localStorage.getItem('print-quote-v1');dom.window.close();
 dom=create(stored);w=dom.window;$=id=>w.document.getElementById(id);
 await until(()=>$('material'));assert.equal($('fdm').getAttribute('aria-pressed'),'true');assert.equal($('customPrice').value,'0');assert(!w.document.body.classList.contains('dark'));
 assert.equal(JSON.parse(w.localStorage.getItem('print-quote-v1')).logo,seed.logo);dom.window.close();
 // The browser edition must not depend on the desktop host.
 assert(!fs.readFileSync(path.join(__dirname,'../3D_Print_Quote_Calculator_Offline.html'),'utf8').includes('window.desktopAPI'));
 assert.deepEqual(errors,[]);
 console.log('PASS: actual bundled app loads; native settings/logo migration; quote math; PDF with logo; export cancellation/error; print bridge; converters; custom price zero; settings menu/export; theme; restart persistence; standalone HTML independence.');
})().catch(error=>{console.error(error);process.exitCode=1;});
