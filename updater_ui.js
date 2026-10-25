(function(){
 'use strict';
 if(!window.desktopAPI)return;
 const style=document.createElement('style');style.textContent=`
 .update-banner{margin:10px 22px 0;padding:10px 14px;border:1px solid var(--line);border-radius:9px;background:var(--card);color:var(--text);font-size:12px;display:flex;gap:10px;align-items:center;flex-wrap:wrap}.update-banner[hidden]{display:none}.update-summary{flex:1;min-width:220px}.update-title{font-weight:700}.update-copy{margin-top:3px}.update-actions{display:flex;gap:7px;flex-wrap:wrap}.update-actions button{padding:7px 10px;font-size:12px}.update-details{width:100%}.update-details ul{padding-left:18px;margin:7px 0;line-height:1.5}.update-details summary{cursor:pointer}.update-card h2{margin-bottom:10px}.update-card .update-actions{margin-top:10px}.update-card p{font-size:12px;margin:5px 0;line-height:1.5}.update-warning{font-weight:600}@media print{.update-banner{display:none!important}}
 `;document.head.append(style);
 const banner=document.createElement('aside');banner.className='update-banner';banner.hidden=true;banner.setAttribute('aria-label','Application update');banner.innerHTML='<div class="update-summary"><div class="update-title"></div><div class="update-copy" role="status"></div></div><div class="update-actions"></div><details class="update-details"><summary>What’s new</summary><ul></ul><button class="quiet" type="button">Read more</button></details>';
 document.querySelector('header').after(banner);
 const card=document.createElement('section');card.className='card update-card';card.innerHTML='<h2>About Print Quote Calculator</h2><p>Resin, FDM and UV project pricing.</p><p class="update-version"></p><p class="update-message" role="status"></p><div class="update-actions"></div><p>Checks run in the background. Downloads and restarts require your action. Release hashes verify integrity against the repository feed; this build is not publisher signed.</p>';
 document.querySelector('#settings-dialog .settings-body').prepend(card);
 let state={},dismissedVersion='';
 function button(label,action){const b=document.createElement('button');b.type='button';b.className='quiet';b.textContent=label;b.addEventListener('click',()=>{if(!window.desktopAPI.update(action))card.querySelector('.update-message').textContent='Finish the current operation before installing.';});return b;}
 function actions(box){box.replaceChildren();const phase=state.phase,busy=['checking','downloading','installing'].includes(phase);const check=button('Check for updates','check');check.disabled=busy;box.append(check);
  if(state.canDownload&&!['downloaded','installing'].includes(phase)){const b=button('Download','download');b.disabled=busy;box.append(b);}
  if(state.canInstall&&!busy){box.append(button('Install and restart','install'));}
 }
 function render(s){state=s;const versions='Installed '+s.installed+(s.latest&&s.latest!==s.installed?' · Latest '+s.latest:'');card.querySelector('.update-version').textContent=versions;card.querySelector('.update-message').textContent=s.message;actions(card.querySelector('.update-actions'));
  for(const el of document.querySelectorAll('main,header,#settings-dialog .settings-body,#ink-dialog,#stock-dialog,#converters-dialog'))el.inert=s.phase==='installing';
  const show=['available','downloading','downloaded','installing','error'].includes(s.phase);banner.hidden=!show||(s.phase==='available'&&dismissedVersion===s.latest);banner.querySelector('.update-title').textContent=versions;banner.querySelector('.update-copy').textContent=s.message;actions(banner.querySelector('.update-actions'));
  if(s.phase==='downloaded'){const warning=document.createElement('span');warning.className='update-warning';warning.textContent='Save your work before restarting.';banner.querySelector('.update-actions').prepend(warning);}
  if(s.phase==='available'||s.phase==='error'){const later=document.createElement('button');later.type='button';later.className='quiet';later.textContent='Dismiss';later.onclick=()=>{dismissedVersion=s.latest;banner.hidden=true;};banner.querySelector('.update-actions').append(later);}
  const details=banner.querySelector('details'),notes=Array.isArray(s.notes)?s.notes:[];details.hidden=!notes.length;const list=details.querySelector('ul');list.replaceChildren();for(const note of notes.slice(0,3)){const li=document.createElement('li');li.textContent=note;list.append(li);}const more=details.querySelector('button');more.hidden=notes.length<=3;more.onclick=()=>window.desktopAPI.update('notes');
 }
 window.desktopAPI.updates(render);
})();
