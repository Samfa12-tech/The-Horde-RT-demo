// One finite preservation operation. Never rerender audio or repeat native checks.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),zlib=require('node:zlib');
const assert=require('node:assert/strict');
const repo='C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.6.1-engineering-pass';
const scratch=path.dirname(__filename),bank=path.join(scratch,'whistle-bank'),core=path.join(scratch,'core-verification');
const destination=path.join(repo,'docs/evidence/2026-10-01-music-whistle-bank');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=file=>fs.readFileSync(file),json=file=>JSON.parse(read(file));
const records=[];
function save(name,bytes){
  const target=path.join(destination,name);fs.mkdirSync(path.dirname(target),{recursive:true});
  fs.writeFileSync(target,bytes,{flag:'wx'});records.push({path:name,bytes:bytes.length,sha256:sha(bytes)});
}
assert(!fs.existsSync(path.join(destination,'SHA256SUMS.json')),'Archive already complete; do not restart.');
const verification=json(path.join(bank,'verification.json')),config=json(path.join(bank,'render/wrapper-config.json'));
const current=read(path.join(repo,'tools/music-instrumentation-bank.cjs')).toString();
const oldEnd="}else throw new Error('Use prepare, render, verify once each; review evidence before admitting runtime files.');\n";
const verifier=(current.slice(0,current.indexOf("}else if(action==='stage'||action==='admit'){"))+oldEnd).replace(/\r?\n/g,'\n');
assert.equal(sha(Buffer.from(verifier)),verification.verifierSha256);
const renderer=verifier.replace("  const pcsReference=reused.renders[0].render.auditionPcsRoundTrip;\n",'')
  .replace('    // App JSON export normalizes to schema17; legacy PCS parsing stays schema16\n    // and drops the non-schema title. Compare each route to its own reference.\n','')
  .replace('normalizedProtected(pcsReference)','normalizedProtected(normalizedReference)');
assert.equal(sha(Buffer.from(renderer)),config.wrapperSha256);
save('method/bank-render-producer.cjs',Buffer.from(renderer));
save('method/bank-verify-producer.cjs',Buffer.from(verifier));
save('method/archive-whistle-bank.cjs',read(__filename));
for(const name of ['plan.json','reference-source.json','reference-manifest.json','verification.json',
  'verification-first-failure.json','staging.json','admission.json','render/wrapper-config.json'])save(name,read(path.join(bank,name)));
const raw=read(path.join(bank,'render/all-cues.json'));
save('render/all-cues.json.gz',zlib.gzipSync(raw));
save('render/all-cues-decompressed.json',Buffer.from(JSON.stringify({bytes:raw.length,sha256:sha(raw)})+'\n'));
for(const name of ['native-configure.log','native-build-Release.log','native-test-Release.log',
  'native-build-Release-silent-tail-corrected.log','native-build-Release-final.log','native-test-Release-final.log',
  'native-build-Debug-final.log','native-test-Debug-final.log','android-asset-tasks.log'])save('validation/'+name,read(path.join(bank,name)));
for(const name of ['configure.log','build-release.log','build-release-padding-corrected.log','build-debug.log',
  'build-Release-bank.log','build-Debug-bank.log','initial-CMakeLists.txt','initial-check.cpp',
  'Release-reference.json','Release-whistle.json','Release-reed.json','Release-owner-combo.json',
  'Debug-whistle.json','Debug-whistle-bound.json','Release-bank.json','Release-bank-bound.json',
  'Debug-bank.json','Debug-bank-bound.json'])save('native/'+name,read(path.join(core,name)));
save('native/initial-run-bank.cjs',read(path.join(core,'initial-run.cjs')));
const debug=json(path.join(core,'Debug-whistle-bound.json'));
for(const [file,copy] of [['check.cpp','initial-check.cpp'],['CMakeLists.txt','initial-CMakeLists.txt']]){
  const expected=debug.inputs.find(x=>x.path==='tools/music-instrumentation-core-check/'+file);
  assert.equal(sha(read(path.join(core,copy))),expected.sha256);
}
save('method/MusicPcmWaveTests.cpp',read(path.join(repo,'tests/MusicPcmWaveTests.cpp')));
const music=path.join(repo,'assets/audio/music/what-the-dark-keeps'),manifest=json(path.join(music,'asset.manifest.json'));
assert.equal(sha(read(path.join(music,'asset.manifest.json'))),verification.manifestSha256);
const refs=['assets/audio/music/what-the-dark-keeps/asset.manifest.json',
  'tools/music-instrumentation-bank.cjs','tools/music-instrumentation-core-check/CMakeLists.txt',
  'tools/music-instrumentation-core-check/check.cpp','tools/music-instrumentation-core-check/run.cjs'];
for(const row of [...manifest.sources,...manifest.cues.flatMap(c=>[c.body,c.tail])]){
  const bytes=read(path.join(music,row.path));assert.equal(bytes.length,row.bytes);assert.equal(sha(bytes),row.sha256);
  refs.push('assets/audio/music/what-the-dark-keeps/'+row.path);
}
const references=refs.map(p=>({path:path.relative(destination,path.join(repo,p)).replaceAll('\\','/'),
  bytes:read(path.join(repo,p)).length,sha256:sha(read(path.join(repo,p)))}));
const bankDebug=json(path.join(core,'Debug-bank-bound.json'));
assert.equal(bankDebug.inputs.find(x=>x.path.endsWith('asset.manifest.json')).sha256,verification.manifestSha256);
save('scope.json',Buffer.from(JSON.stringify({schema:1,ownerChoice:'whistle-lead',runtimeAdmitted:true,
  renderCount:6,reused:['A','E'],pcmBytes:20160000,waveBytes:20160704,
  nativeCorePin:'534a6e6811ce653efd5422138c5772b967263ed0',
  priorAuditionArchive:'../2026-10-01-music-instrumentation/SHA256SUMS.json',
  priorAuditionSha256:'72134d1f4172e957a8fe7935ae55fba8965527de72b9cdb4a8622e42f20293e3',
  knownEvidenceLimits:['Early Release audition results did not pre-bind source/executable hashes.',
    'Historical Debug-whistle and Release-bank bound PCM/source files but only named the runtime manifest; final Debug-bank also hashes the manifest/archive before and after.',
    'Historical verification/plan runtimeNotChanged described their pre-admission step; admission.json supersedes that state.',
    'Sequential admission is not atomic; manifest-last hash admission fails closed on partial assets. Original bank remains recoverable from Git and original render evidence.',
    'No OS audio/native consumed clock, phone audition, audible loop/transition or SFX masking acceptance in this slice.',
    'No Pocket DAW use, shared preset/Core change, new shader/gameplay work, publication or device actions.']},null,2)+'\n'));
const sums={schema:1,scope:'Owner-selected runtime instrumentation; local validation only, listening/device/release gates open.',
  rights:'Owner-supplied; authorised for Horde use only',records,references};
fs.writeFileSync(path.join(destination,'SHA256SUMS.json'),JSON.stringify(sums,null,2)+'\n',{flag:'wx'});
for(const row of [...records,...references]){const bytes=read(path.resolve(destination,row.path));assert.equal(bytes.length,row.bytes);assert.equal(sha(bytes),row.sha256);}
assert.equal(sha(zlib.gunzipSync(read(path.join(destination,'render/all-cues.json.gz')))),sha(raw));
console.log(JSON.stringify({status:'PASS-preserved-and-byte-verified',records:records.length,references:references.length,
  bytes:records.reduce((n,x)=>n+x.bytes,0),sha256:sha(read(path.join(destination,'SHA256SUMS.json')))}));
