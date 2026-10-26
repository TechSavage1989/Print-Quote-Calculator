'use strict';
const fs=require('node:fs');
const {execFileSync}=require('node:child_process');
const path=require('node:path');
const allowed=new Set(['.github','.gitignore','AGENTS.md','DEVELOPMENT_HANDOFF.md','README.md','assets','build.cmd','build.sh','docs','package-lock.json','package.json','scripts','sdk','src','tests','vendor','web']);
function check(root){
 let entries;
 if(fs.existsSync(path.join(root,'.git'))){
  entries=execFileSync('git',['ls-files','--cached','--others','--exclude-standard','-z'],{cwd:root,encoding:'utf8'}).split('\0').filter(Boolean).map(f=>f.split('/')[0]);
 }else{
  // Source archives have no Git metadata. Ignore only documented local build outputs.
  const generated=/^(?:.*\.(?:o|exe|log)|embedded\.html|3D_Print_Quote_Calculator_Offline\.html|release|node_modules|__pycache__|UV_.*Check\.pdf|sdk-check.*)$/;
  entries=fs.readdirSync(root).filter(n=>!generated.test(n));
 }
 const unique=[...new Set(entries)];
 const unexpected=unique.filter(n=>!allowed.has(n));
 if(unexpected.length)throw Error('Unexpected repository-root entries: '+unexpected.join(', ')+'. Use the documented folders; do not expand the allowlist for release notes or generated files.');
 return unique.length;
}
module.exports={check};
if(require.main===module)console.log('Repository layout OK ('+check(path.resolve(__dirname,'..'))+' root entries).');
