const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto'), zlib = require('node:zlib');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..'), repo = path.resolve(root, '../../..');
const work = 'C:/Dev/tmp/horde-music-drone-20261003';
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
function put(file, b) { fs.mkdirSync(path.dirname(file), {recursive:true}); fs.writeFileSync(file,b,{flag:'wx'}); }
assert(!fs.existsSync(path.join(root, 'SHA256SUMS.json')), 'Inspect completed evidence instead of overwriting');
for (const [from,to] of [
  ['reference-source.json','reference-source.json'], ['reference-manifest.json','reference-manifest.json'],
  ['render/wrapper-config.json','render/wrapper-config.json'], ['verification.json','verification.json'],
  ['admission.json','admission.json'], ['native-Release-drone.json','validation/native-first-failure.log'],
  ['native-Release-drone-corrected.json','validation/native-Release-drone.json'],
  ['android-debug-build.log','validation/android-debug-build.log']
]) put(path.join(root,to),fs.readFileSync(path.join(work,from)));
put(path.join(root,'render/all-cues.json.gz'),zlib.gzipSync(fs.readFileSync(path.join(work,'render/all-cues.json'))));
// Recover the exact producer used before adding negative contract checks;
// accept this reconstruction only when the original producer hash agrees.
const current = fs.readFileSync(path.join(__dirname,'edit-render.cjs'),'utf8');
const contractStart = current.indexOf("} else if (action === 'contract') {");
const renderStart = current.indexOf("} else if (action === 'render') {");
assert(contractStart>0 && renderStart>contractStart);
let used = current.slice(0,contractStart)+current.slice(renderStart);
const negativeStart = used.indexOf('  for (const mutate of [s => { s.bpm++; }');
const negativeEnd = used.indexOf('  const receipt = ',negativeStart);
assert(negativeStart>0 && negativeEnd>negativeStart);
used = used.slice(0,negativeStart)+used.slice(negativeEnd);
const config = JSON.parse(fs.readFileSync(path.join(work,'render/wrapper-config.json')));
assert.equal(hash(Buffer.from(used)),config.wrapperSha256);
put(path.join(__dirname,'render-wrapper-used.cjs'),used);
const files = [];
function walk(dir) { for(const e of fs.readdirSync(dir,{withFileTypes:true})) {
  const f=path.join(dir,e.name); if(e.isDirectory())walk(f);else if(e.name!=='SHA256SUMS.json')files.push(f);
} }
walk(root);
const records=files.sort().map(f=>{const b=fs.readFileSync(f);return {path:path.relative(root,f).replaceAll('\\','/'),bytes:b.length,sha256:hash(b)};});
const music=path.join(repo,'assets/audio/music/what-the-dark-keeps');
const manifest=JSON.parse(fs.readFileSync(path.join(music,'asset.manifest.json')));
const inputs=[{path:'assets/audio/music/what-the-dark-keeps/asset.manifest.json'},
  ...manifest.sources.map(e=>({path:'assets/audio/music/what-the-dark-keeps/'+e.path})),
  ...manifest.cues.flatMap(c=>[c.body,c.tail]).map(e=>({path:'assets/audio/music/what-the-dark-keeps/'+e.path})),
  {path:'tools/music-instrumentation-core-check/check.cpp'}
].map(e=>{const b=fs.readFileSync(path.join(repo,e.path));return {...e,bytes:b.length,sha256:hash(b)};});
put(path.join(root,'SHA256SUMS.json'),JSON.stringify({schema:1,records,inputs,
  apk:{path:path.join(work,'HordeLanternRT-music-drone-debug.apk'),sha256:hash(fs.readFileSync(path.join(work,'HordeLanternRT-music-drone-debug.apk')))},
  coreExecutable:{sha256:hash(fs.readFileSync(path.join(work,'core-build/Release/horde_music_instrumentation_core_check.exe')))},
  scope:'Only A/D Melody3 held-phrase deletion. New native sample/format/package checks; no shader, renderer, gameplay, OS-audio or perceptual equivalence inferred.'},null,2)+'\n');
console.log(JSON.stringify({records:records.length,inputs:inputs.length,exactRenderProducerRecovered:true}));
