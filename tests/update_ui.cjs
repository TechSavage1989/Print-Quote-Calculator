const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {JSDOM,VirtualConsole}=require('jsdom');
const errors=[],sent=[];let receive;
const vc=new VirtualConsole();vc.on('jsdomError',e=>errors.push(e.message));
const dom=new JSDOM(fs.readFileSync(path.join(__dirname,'../embedded.html'),'utf8'),{url:'https://printquote.local/',runScripts:'dangerously',pretendToBeVisual:true,virtualConsole:vc,beforeParse(w){
 w.TextEncoder=TextEncoder;w.TextDecoder=TextDecoder;w.matchMedia=()=>({matches:false});w.URL.createObjectURL=()=> 'blob:test';w.URL.revokeObjectURL=()=>{};w.HTMLDialogElement.prototype.showModal=function(){this.open=true;};w.HTMLDialogElement.prototype.close=function(){this.open=false;};
 w.chrome={webview:{addEventListener(t,cb){receive=cb;},postMessage(m){sent.push(m);}}};
}});
const w=dom.window,d=w.document;const state=(overrides={})=>receive({data:{type:'update-state',state:{installed:'1.6.0',latest:'1.6.0',phase:'current',message:'You are up to date.',notes:[],canDownload:false,canInstall:false,...overrides}}});
function button(text,where='.update-banner'){return [...d.querySelectorAll(where+' button')].find(b=>b.textContent===text);}
state();assert(d.querySelector('.update-banner').hidden);assert.equal(d.querySelector('.update-version').textContent,'Installed 1.6.0');assert(!button('Download','.update-card'));
state({latest:'1.6.1',phase:'available',canDownload:true,notes:['First','Second','Third','Fourth']});assert(!d.querySelector('.update-banner').hidden);assert.equal(d.querySelectorAll('.update-banner li').length,3);assert(!button('Read more').hidden);assert.equal(sent.filter(m=>m.type==='update-download').length,0);button('Download').click();assert.equal(sent.at(-1).type,'update-download');
state({latest:'1.6.1',phase:'downloading',canDownload:true,message:'Downloading update: 50%'});assert(button('Download').disabled);assert(!button('Install and restart'));
state({latest:'1.6.1',phase:'downloaded',canInstall:true,notes:['<img src=x onerror=alert(1)>']});assert(button('Read more').hidden);assert(!d.querySelector('.update-banner img'));assert.match(d.querySelector('.update-banner li').textContent,/<img/);
w.desktopAPI.operation('export',true);let count=sent.filter(m=>m.type==='update-install').length;button('Install and restart').click();assert.equal(sent.filter(m=>m.type==='update-install').length,count);w.desktopAPI.operation('export',false);button('Install and restart').click();assert.equal(sent.at(-1).type,'update-install');
state({phase:'installing'});assert(d.querySelector('main').inert);state({phase:'error',message:'Offline. Check failed.'});assert(!d.querySelector('main').inert);assert.match(d.querySelector('.update-copy').textContent,/failed/);assert(!d.querySelector('.update-copy').textContent.includes('up to date'));
assert.deepEqual(errors,[]);dom.window.close();console.log('PASS: actual desktop update UI, explicit actions, same-version display, progress, shortened notes, HTML escaping, busy guard and failed-check state.');
