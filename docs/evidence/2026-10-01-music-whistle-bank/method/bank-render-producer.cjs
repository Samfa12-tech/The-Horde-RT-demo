// Owner-selected whistle palette. Reuse accepted A/E; render the six new cues
// through the pinned actual app only. Never call Pocket DAW or Core synthesis.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),zlib=require('node:zlib');
const assert=require('node:assert/strict'),Module=require('node:module');
const repo=path.resolve(__dirname,'..'),root='C:/Dev/tmp/horde-music-instrumentation-20261001';
const work=path.join(root,'whistle-bank'),music=path.join(repo,'assets/audio/music/what-the-dark-keeps');
const auditions=path.join(repo,'docs/evidence/2026-10-01-music-instrumentation');
const method=path.join(repo,'docs/evidence/2026-10-01-music-render/method/render-sections.cjs');
const renderer=path.join(root,'chordsmith-render-source');
const dependency='C:/Users/sam_s/Documents/Pocket Chordsmith/apps/chordsmith-web';
const archive='C:/Users/sam_s/Downloads/What_the_Dark_Keeps_Horde_RT_Music_Pack.zip';
const jsonName='What_the_Dark_Keeps_Pocket_Chordsmith.json',pcsName='What_the_Dark_Keeps_PCS1.txt';
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const readJson=file=>JSON.parse(fs.readFileSync(file));
const writeNew=(file,b)=>{fs.mkdirSync(path.dirname(file),{recursive:true});fs.writeFileSync(file,b,{flag:'wx'});};
const decode=s=>JSON.parse(Buffer.from(s.trim().slice(5),'base64url').toString('utf8'));
const originalManifestHash='1126f9f537efb607b11bd492e1c79d6e8b94814567ce06b654e03b0d915c9ff3';
const immutableSourceHash='bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6';
const mutable=new Set(['chordInstrument','chordVolume','fxChorus','fxDelay','fxMix',
  ...[...'ABCDEFGH'].map(cue=>`melodyInstruments${cue}`)]);
const protectedValues=score=>Object.fromEntries(Object.entries(score).filter(([key])=>!mutable.has(key)));
const sectionKey=/^(progression|grid|gridTuplets|melodyTracks|melodyInstruments|melodyOctaves|melodyMute|melodySolo|melodyPan|melodyHold|melodySlide|melodyTuplets|bassHold|bassSlide|bassNotes|bassAccent|guitarPattern)([A-H])$/;
const activeProjection=(score,cue)=>Object.fromEntries(Object.entries(score).filter(([key])=>{
  const match=key.match(sectionKey);return !match||match[2]===cue;
}));
const source=path.join(work,'source'),output=path.join(work,'render');
const action=process.argv[2];
if(action==='prepare'){
  assert(!fs.existsSync(work),'Completed/prepared bank exists; inspect its record rather than restarting.');
  const before=fs.readFileSync(path.join(music,'source',jsonName));
  assert.equal(sha(before),immutableSourceHash);
  assert.equal(sha(fs.readFileSync(path.join(music,'asset.manifest.json'))),originalManifestHash);
  const original=JSON.parse(before),selected=readJson(path.join(auditions,'sources/whistle',jsonName));
  assert.deepEqual(protectedValues(selected),protectedValues(original));
  const chosen=structuredClone(selected);
  for(const cue of 'ABCDEFGH') chosen[`melodyInstruments${cue}`]=['cowboy_whistle','soft_pluck','mellow_sax'];
  // Sparse authored bell accents remain only in the two short transition cues:
  // three C attacks and four G answers. No note or attack is added/deleted.
  chosen.melodyInstrumentsC=['bell','soft_pluck','mellow_sax'];
  chosen.melodyInstrumentsG=['cowboy_whistle','bell','mellow_sax'];
  assert.deepEqual(protectedValues(chosen),protectedValues(original));
  for(const cue of ['A','E'])assert.deepEqual(activeProjection(chosen,cue),activeProjection(selected,cue));
  // Keep the canonical file's readable, compact-array layout: thirteen field
  // replacements, rather than unrelated reformatting of every musical grid.
  let text=before.toString('utf8');
  for(const key of mutable){
    const match=new RegExp(`("${key}"\\s*:\\s*)(\\[[^\\n]*?\\]|"[^"\\n]*"|[0-9.]+)`,'g');
    assert.equal([...text.matchAll(match)].length,1,`source key is not unique: ${key}`);
    text=text.replace(match,(_,prefix)=>prefix+JSON.stringify(chosen[key]));
  }
  assert.deepEqual(JSON.parse(text),chosen);
  const pcs=`PCS1:${Buffer.from(JSON.stringify(chosen)).toString('base64url')}\n`;
  assert.deepEqual(decode(pcs),chosen);
  writeNew(path.join(source,jsonName),text);writeNew(path.join(source,pcsName),pcs);
  writeNew(path.join(work,'reference-source.json'),before);
  writeNew(path.join(work,'reference-manifest.json'),fs.readFileSync(path.join(music,'asset.manifest.json')));
  writeNew(path.join(work,'plan.json'),JSON.stringify({schema:1,ownerChoice:'whistle-lead, explicitly selected October 1',
    sourceSha256:sha(Buffer.from(text)),pcs1Sha256:sha(Buffer.from(pcs)),
    originalSourceSha256:immutableSourceHash,originalManifestSha256:originalManifestHash,
    reused:['A','E'],renderRemaining:['B','C','D','F','G','H'],
    changedKeys:[...mutable],activeAEProjectionMatchesAcceptedCandidate:true,
    bells:{C:{part:1,attacks:3},G:{part:2,attacks:4}},runtimeNotChanged:true},null,2)+'\n');
  console.log('Prepared owner-selected full score; reuse A/E, render B/C/D/F/G/H once.');
}else if(action==='render'){
  assert(!fs.existsSync(output),'Render already started/completed; do not rerun unchanged artifacts.');
  const bytes=fs.readFileSync(path.join(source,jsonName)),plan=readJson(path.join(work,'plan.json'));
  assert.equal(sha(bytes),plan.sourceSha256);
  assert.equal(sha(fs.readFileSync(path.join(renderer,'apps/chordsmith-web/pocket_chordsmith_v68_core_bridge.html'))),
    'b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf');
  assert.equal(sha(fs.readFileSync(archive)),'e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa');
  process.env.HORDE_MUSIC_SCORE_BASE64=bytes.toString('base64');process.env.HORDE_MUSIC_SCORE_ZIP_PATH=archive;
  let adapter=fs.readFileSync(method,'utf8');
  function once(anchor,value){assert.equal(adapter.split(anchor).length,2);adapter=adapter.replace(anchor,value);}
  once('require("@playwright/test")',`require(${JSON.stringify(Module.createRequire(path.join(dependency,'package.json')).resolve('@playwright/test'))})`);
  once('const repo = "C:/Users/sam_s/Documents/Pocket Chordsmith";',`const repo = ${JSON.stringify(renderer)};`);
  once('const output = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render-corrected";',`const output = ${JSON.stringify(output)};`);
  once(`score: "${immutableSourceHash}"`,`score: "${sha(bytes)}"`);
  const begin=adapter.indexOf('const cueSpecs = ['),end=adapter.indexOf('];',begin)+2;
  assert(begin>=0&&end>begin);
  adapter=adapter.slice(0,begin)+'const cueSpecs = '+JSON.stringify(plan.renderRemaining.map(id=>({id,frames:id==='C'?144000:id==='G'?288000:576000})))+';'+adapter.slice(end);
  once('state.currentSection = cueId;','const auditionRoundTrip = exportProject({targetSchema:17});\nconst auditionPcsRoundTrip = parseShareCode(\'PCS1:\' + utf8ToBase64Url(JSON.stringify(project)));\nstate.currentSection = cueId;');
  once('cue:cueId, errors,','cue:cueId, errors, auditionRoundTrip, auditionPcsRoundTrip,');
  once('no source changes, Core renderer, built-in WAV exporter, normalization, score edits, or generation.',
    'no app source changes, Core renderer, built-in WAV exporter, normalization or generation. Owner-selected instrumentation-only full-bank derivative; unchanged musical events.');
  writeNew(path.join(output,'wrapper-config.json'),JSON.stringify({sourceSha256:sha(bytes),wrapperSha256:sha(fs.readFileSync(__filename)),
    retainedAdapterSha256:sha(fs.readFileSync(method)),effectiveAdapterSha256:sha(Buffer.from(adapter)),renderer,output},null,2)+'\n');
  const run=new Module(__filename,module);run.filename=__filename;run.paths=module.paths;run._compile(adapter,__filename);
}else if(action==='verify'){
  assert(!fs.existsSync(path.join(work,'verification.json')),'Verification completed; no repeat without a specific validity problem.');
  const plan=readJson(path.join(work,'plan.json')),score=readJson(path.join(source,jsonName));
  const original=readJson(path.join(work,'reference-source.json'));
  assert.deepEqual(protectedValues(score),protectedValues(original));
  assert.deepEqual(decode(fs.readFileSync(path.join(source,pcsName),'utf8')),score);
  const old=JSON.parse(zlib.gunzipSync(fs.readFileSync(path.join(repo,'docs/evidence/2026-10-01-music-render/verification/all-cues.json.gz'))));
  const fresh=readJson(path.join(output,'all-cues.json'));
  const reused=JSON.parse(zlib.gunzipSync(fs.readFileSync(path.join(auditions,'receipts/whistle.json.gz'))));
  assert.deepEqual(fresh.renders.map(row=>row.cue),plan.renderRemaining);
  const normalizedReference=reused.renders[0].render.auditionRoundTrip;
  const normalizedProtected=p=>Object.fromEntries(Object.keys(original).filter(k=>!mutable.has(k)).map(k=>[k,p[k]]));
  const manifest=readJson(path.join(work,'reference-manifest.json')),checks=[];
  for(const entry of manifest.sources){const b=fs.readFileSync(path.join(source,path.basename(entry.path)));entry.bytes=b.length;entry.sha256=sha(b);}
  for(const cue of manifest.cues){
    const reuse=plan.reused.includes(cue.cue);
    const row=(reuse?reused:fresh).renders.find(x=>x.cue===cue.cue),r=row.render;
    const control=old.renders.find(x=>x.cue===cue.cue).render;
    assert.deepEqual(row.pageErrors,[]);assert.deepEqual(r.errors,[]);
    assert(r.sourceMask.melodyStartsMatch&&r.sourceMask.holdsMatch);
    assert(r.calls.leadCallsMatch&&r.calls.chordCallsMatch&&r.calls.bassCallsMatch);
    assert(Object.values(r.calls.drumCountsMatch).every(Boolean));
    assert.deepEqual(r.eventTrace,control.eventTrace);assert.deepEqual(r.schedule.steps,control.schedule.steps);
    for(const key of ['actualChordCalls','actualBassPhraseCalls','actualDrumCounts'])assert.deepEqual(r.calls[key],control.calls[key]);
    const events=calls=>calls.map(({instrument,...event})=>event);
    assert.deepEqual(events(r.calls.actualLeadPhraseCalls),events(control.calls.actualLeadPhraseCalls));
    assert.deepEqual(normalizedProtected(r.auditionRoundTrip),normalizedProtected(normalizedReference));
    assert.deepEqual(normalizedProtected(r.auditionPcsRoundTrip),normalizedProtected(normalizedReference));
    assert.deepEqual(r.auditionRoundTrip[`melodyInstruments${cue.cue}`],score[`melodyInstruments${cue.cue}`]);
    assert.equal(r.audio.metrics.nonFiniteSamples,0);assert.equal(r.audio.metrics.clippedSamplesAtPcm16Ceiling,0);
    const parts=[];
    for(const [kind,part] of [['loop',cue.body],['tail',cue.tail]]){
      const origin=reuse?path.join(auditions,`audio/whistle/${cue.cue}-${kind}.wav`):path.join(output,`${cue.cue}-${kind}.wav`);
      const b=fs.readFileSync(origin),declared=row.outputs.find(x=>x.path===`${cue.cue}-${kind}.wav`);
      assert.equal(sha(b),declared.sha256);assert.equal(b.length,part.frames*4+44);
      assert.equal(b.toString('ascii',0,4),'RIFF');assert.equal(b.readUInt32LE(4),b.length-8);
      assert.equal(b.toString('ascii',8,16),'WAVEfmt ');assert.equal(b.readUInt32LE(16),16);
      assert.equal(b.readUInt16LE(20),1);assert.equal(b.readUInt16LE(22),2);
      assert.equal(b.readUInt32LE(24),48000);assert.equal(b.readUInt32LE(28),192000);
      assert.equal(b.readUInt16LE(32),4);assert.equal(b.readUInt16LE(34),16);
      assert.equal(b.toString('ascii',36,40),'data');assert.equal(b.readUInt32LE(40),part.frames*4);
      let peak=0,squares=0;for(let i=44;i<b.length;i+=2){const s=b.readInt16LE(i);assert(s!==32767&&s!==-32768);peak=Math.max(peak,Math.abs(s/32767));squares+=(s/32767)**2;}
      assert(Math.abs(peak-r.audio.metrics[kind==='loop'?'bodyPeak':'tailPeak'])<=2/32767);
      assert(Math.abs(Math.sqrt(squares/((b.length-44)/2))-r.audio.metrics[kind==='loop'?'bodyRms':'tailRms'])<=2/32767);
      part.bytes=b.length;part.sha256=sha(b);writeNew(path.join(work,'runtime',path.basename(part.path)),b);
      parts.push({kind,frames:part.frames,sha256:part.sha256,peak,reused:reuse});
    }
    checks.push({cue:cue.cue,scheduleEquivalent:true,jsonPcs1Consistent:true,parts,reused:reuse});
  }
  assert.equal(checks.flatMap(x=>x.parts).reduce((n,x)=>n+x.frames*4,0),20160000);
  manifest.render.evidenceManifest='docs/evidence/2026-10-01-music-whistle-bank/SHA256SUMS.json';
  manifest.render.processing='Owner-selected whistle-lead instrumentation; retained v68 actual app voices/live FX. Accepted A/E PCM reused with identical active-cue source projection; B/C/D/F/G/H rendered once. No notes/timing changes, normalization or PCM playback change.';
  writeNew(path.join(work,'asset.manifest.json'),JSON.stringify(manifest,null,2)+'\n');
  writeNew(path.join(work,'verification.json'),JSON.stringify({schema:1,status:'PASS-source-schedule-and-PCM; native/listening pending',
    sourceSha256:plan.sourceSha256,manifestSha256:sha(fs.readFileSync(path.join(work,'asset.manifest.json'))),checks,
    normalMusicVolumeUnchanged:70,pcmBytes:20160000,waveBytes:20160704,
    verifierSha256:sha(fs.readFileSync(__filename)),runtimeNotChanged:true},null,2)+'\n');
  console.log(JSON.stringify({status:'PASS',cues:checks.length,pcmBytes:20160000,manifestSha256:sha(fs.readFileSync(path.join(work,'asset.manifest.json')))}));
}else throw new Error('Use prepare, render, verify once each; review evidence before admitting runtime files.');
