'use strict';
const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict');
const {check}=require('./check-layout.cjs');
const root=fs.mkdtempSync(path.join(os.tmpdir(),'pqc-layout-'));
try{
 for(const n of ['README.md','AGENTS.md','DEVELOPMENT_HANDOFF.md'])fs.writeFileSync(path.join(root,n),'fixture');
 fs.mkdirSync(path.join(root,'src'));
 assert.equal(check(root),4);
 fs.writeFileSync(path.join(root,'PrintQuoteCalculator.exe'),'local generated fixture');
 assert.equal(check(root),4);
 fs.writeFileSync(path.join(root,'RELEASE_NOTES_99.md'),'must be archived');
 assert.throws(()=>check(root),/Unexpected repository-root entries/);
 console.log('PASS: source archive fallback, generated output, and root-clutter negative control.');
}finally{fs.rmSync(root,{recursive:true,force:true});}
