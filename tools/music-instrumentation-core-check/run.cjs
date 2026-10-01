// Bind offline checker results to exact inputs before and after execution.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),cp=require('node:child_process');
const assert=require('node:assert/strict');
const repo=path.resolve(__dirname,'../..'),root='C:/Dev/tmp/horde-music-instrumentation-20261001/core-verification';
const evidence=path.join(repo,'docs/evidence/2026-10-01-music-instrumentation');
const configuration=process.argv[2],variant=process.argv[3];
assert(['Debug','Release'].includes(configuration));assert(['reference','whistle','reed','owner-combo','bank'].includes(variant));
const resultPath=path.join(root,`${configuration}-${variant}.json`),receiptPath=path.join(root,`${configuration}-${variant}-bound.json`);
assert(!fs.existsSync(resultPath)&&!fs.existsSync(receiptPath),'Completed result exists; do not repeat.');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function snapshot(){
  const archiveBytes=fs.readFileSync(path.join(evidence,'SHA256SUMS.json'));
  const archive=JSON.parse(archiveBytes);
  for(const row of archive.records){const bytes=fs.readFileSync(path.resolve(evidence,row.path));assert.equal(sha(bytes),row.sha256);assert.equal(bytes.length,row.bytes);}
  const music=path.join(repo,'assets/audio/music/what-the-dark-keeps');
  const manifestBytes=fs.readFileSync(path.join(music,'asset.manifest.json'));
  const manifest=JSON.parse(manifestBytes);
  const inputs=[{path:'assets/audio/music/what-the-dark-keeps/asset.manifest.json',
    bytes:manifestBytes.length,sha256:sha(manifestBytes)},
    {path:'docs/evidence/2026-10-01-music-instrumentation/SHA256SUMS.json',
      bytes:archiveBytes.length,sha256:sha(archiveBytes)}];
  for(const row of [...manifest.sources,...manifest.cues.flatMap(cue=>[cue.body,cue.tail])]){
    const bytes=fs.readFileSync(path.join(music,row.path));assert.equal(sha(bytes),row.sha256);assert.equal(bytes.length,row.bytes);
    inputs.push({path:path.relative(repo,path.join(music,row.path)).replaceAll('\\','/'),sha256:sha(bytes),bytes:bytes.length});
  }
  for(const row of archive.records.filter(x=>x.path.startsWith(`audio/${variant}/`)))inputs.push({path:path.relative(repo,path.join(evidence,row.path)).replaceAll('\\','/'),sha256:row.sha256,bytes:row.bytes});
  const sources=['tools/music-instrumentation-core-check/check.cpp','tools/music-instrumentation-core-check/CMakeLists.txt',
    'tools/music-instrumentation-core-check/run.cjs','src/audio/MusicPcmStream.cpp','src/audio/MusicPcmStream.h',
    'src/audio/MusicPcmAssets.h','src/audio/MusicPcmWave.cpp','src/audio/MusicPcmWave.h',
    'third_party/pocket-audio-core/manifest.json','third_party/pocket-audio-core/native/src/PcmLoopStream.cpp',
    'third_party/pocket-audio-core/native/src/PcmWave.cpp'];
  for(const file of sources){const bytes=fs.readFileSync(path.join(repo,file));inputs.push({path:file,sha256:sha(bytes),bytes:bytes.length});}
  return inputs;
}
const before=snapshot(),executable=path.join(root,`build/${configuration}/horde_music_instrumentation_core_check.exe`);
const executableBytes=fs.readFileSync(executable),started=new Date().toISOString();
const run=cp.spawnSync(executable,[repo,evidence,variant],{encoding:'utf8',windowsHide:true,timeout:300000,maxBuffer:1024*1024});
fs.writeFileSync(resultPath,run.stdout||run.stderr||String(run.error),{flag:'wx'});
assert.equal(run.status,0,run.stderr||String(run.error));const result=JSON.parse(run.stdout);
assert(result.status.startsWith('PASS-native-offline'));assert.deepEqual(snapshot(),before);
const receipt={schema:1,configuration,variant,started,finished:new Date().toISOString(),exitCode:run.status,
  executable:{bytes:executableBytes.length,sha256:sha(executableBytes)},inputs:before,inputHashesStable:true,
  resultSha256:sha(fs.readFileSync(resultPath)),scope:'Production Core/Horde adapter offline sample clock; no OS audio, DAW, renderer, device or audible acceptance.'};
fs.writeFileSync(receiptPath,JSON.stringify(receipt,null,2)+'\n',{flag:'wx'});
console.log(JSON.stringify({status:result.status,configuration,variant,loops:result.loops.length,
  transitions:(result.transitions||result.handoffs||[]).length,inputHashesStable:true}));
