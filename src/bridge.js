(function () {
 'use strict';
 if (!window.chrome || !window.chrome.webview || location.origin !== 'https://printquote.local') return;
 const channel = window.chrome.webview;
 const pending = new Map();
 let sequence = 0;
 const operations = new Set();
 const updateListeners = new Set();
 let updateState = null;
 function request(payload) {
  const id = String(++sequence);
  return new Promise((resolve, reject) => {
   pending.set(id, {resolve, reject});
   try { channel.postMessage({...payload, id}); }
   catch (error) { pending.delete(id); reject(error); }
  });
 }
 channel.addEventListener('message', event => {
  const message = event.data;
  if(message?.type==='health-probe') {
   requestAnimationFrame(()=>channel.postMessage({type:'health',healthy:!!window.currentQuote&&!!document.querySelector('#quote')?.textContent?.trim()&&document.querySelector('#project-fields')?.childElementCount>0}));
   return;
  }
  if (message?.type === 'update-state') {
   updateState = message.state;
   for (const listener of updateListeners) listener(updateState);
   return;
  }
  const entry = message && pending.get(message.id);
  if (!entry) return;
  pending.delete(message.id);
  if (message.error) entry.reject(new Error(message.error));
  else entry.resolve(message);
 });
 function base64(bytes) {
  let binary = '';
  for (let i = 0; i < bytes.length; i += 8192) binary += String.fromCharCode(...bytes.subarray(i, i + 8192));
  return btoa(binary);
 }
 window.desktopAPI = Object.freeze({
  savePDF(bytes, name, open) { return request({type:'save-file', kind:'pdf', data:base64(new Uint8Array(bytes)), name, open:!!open}); },
  saveSettings(settings) { return request({type:'save-file', kind:'json', data:base64(new TextEncoder().encode(JSON.stringify(settings, null, 2))), name:'Print_Quote_Settings.json', open:false}); },
  setTheme(theme) { channel.postMessage({type:'theme', dark:theme === 'dark'}); },
  operation(name, busy) { if(busy)operations.add(name);else operations.delete(name);channel.postMessage({type:'update-busy',busy:operations.size>0}); },
  updates(listener) { updateListeners.add(listener); if(updateState)listener(updateState); channel.postMessage({type:'update-state'}); },
  update(action) { if(action==='install'&&operations.size)return false;if(!['check','download','install','notes'].includes(action))return false;channel.postMessage({type:'update-'+action});return true; }
 });
 try {
  if (!localStorage.getItem('print-quote-v1') && window.__PrintQuoteInitial?.states) {
   localStorage.setItem('print-quote-v1', JSON.stringify(window.__PrintQuoteInitial));
  }
 } catch (_) { /* The app reports unavailable storage in its existing status area. */ }
 delete window.__PrintQuoteInitial;
 window.addEventListener('DOMContentLoaded', () => {
  document.querySelector('.brand').addEventListener('click', event => event.preventDefault());
  channel.postMessage({type:'ready'});
 });
})();
