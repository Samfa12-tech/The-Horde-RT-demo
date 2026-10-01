// Independent PCM/score audit and preview-only constant-gain A/B assembly.
// Does not render synthesis, edit the admitted bank, or claim human listening.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const repo = path.resolve(__dirname, '..');
const root = 'C:/Dev/tmp/horde-music-instrumentation-20261001';
const ownerSupplement = process.argv[2] === 'owner-combo';
const transitionSupplement = process.argv[3] === 'transition-supplement';
const output = path.join(root, (ownerSupplement ? 'verified-owner-combo' : 'verified-previews') +
  (transitionSupplement ? '-transition-supplement' : ''));
if (fs.existsSync(output)) throw new Error('Refusing to overwrite/repeat completed audition verification.');
const variants = ownerSupplement ? ['reference','owner-combo'] : ['reference','whistle','reed'], cues = ['A','E'];
const music = path.join(repo,'assets/audio/music/what-the-dark-keeps');
const sha = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const original = JSON.parse(fs.readFileSync(path.join(music,'source/What_the_Dark_Keeps_Pocket_Chordsmith.json')));
const allowed = new Set(['chordInstrument','chordVolume','fxDelay','fxChorus','fxMix','melodyInstrumentsA','melodyInstrumentsE']);
const originalManifest = JSON.parse(fs.readFileSync(path.join(music,'asset.manifest.json')));
function protectedOriginalKeys(project) {
  return Object.fromEntries(Object.keys(original).filter(key=>!allowed.has(key)).map(key=>[key,project[key]]));
}
function wave(file, expectedFrames) {
  const b = fs.readFileSync(file);
  assert.equal(b.toString('ascii',0,4),'RIFF'); assert.equal(b.toString('ascii',8,16),'WAVEfmt ');
  assert.equal(b.readUInt32LE(4),b.length-8); assert.equal(b.readUInt32LE(16),16);
  assert.equal(b.readUInt16LE(20),1); assert.equal(b.readUInt16LE(22),2);
  assert.equal(b.readUInt32LE(24),48000); assert.equal(b.readUInt32LE(28),192000);
  assert.equal(b.readUInt16LE(32),4); assert.equal(b.readUInt16LE(34),16);
  assert.equal(b.toString('ascii',36,40),'data'); assert.equal(b.readUInt32LE(40),b.length-44);
  const frames=(b.length-44)/4;
  if (expectedFrames!==undefined) assert.equal(frames,expectedFrames);
  const samples = new Float64Array(frames*2);
  let peak=0,squares=0,saturated=0;
  for(let i=0;i<samples.length;i++) {
    const s=b.readInt16LE(44+i*2); samples[i]=s/32767;
    peak=Math.max(peak,Math.abs(samples[i])); squares+=samples[i]**2;
    saturated+=s===32767||s===-32768;
  }
  assert.equal(saturated,0);
  return {samples,frames,bytes:b.length,sha256:sha(b),peak,rms:Math.sqrt(squares/samples.length),
    seam:Math.max(Math.abs(samples[0]-samples.at(-2)),Math.abs(samples[1]-samples.at(-1)))};
}
function encode(samples) {
  const b=Buffer.alloc(44+samples.length*2);
  b.write('RIFF',0); b.writeUInt32LE(b.length-8,4); b.write('WAVEfmt ',8);
  b.writeUInt32LE(16,16); b.writeUInt16LE(1,20); b.writeUInt16LE(2,22);
  b.writeUInt32LE(48000,24); b.writeUInt32LE(192000,28); b.writeUInt16LE(4,32);
  b.writeUInt16LE(16,34); b.write('data',36); b.writeUInt32LE(b.length-44,40);
  for(let i=0;i<samples.length;i++) {
    assert(Number.isFinite(samples[i])&&Math.abs(samples[i])<32766/32767,'No clipping/NaN concealment');
    b.writeInt16LE(Math.round(samples[i]*32767),44+i*2);
  }
  return b;
}
function writeNew(file,bytes) {fs.writeFileSync(file,bytes,{flag:'wx'});}
function measure(file) {
  // Analysis only: never use loudnorm's dynamic output as an audition derivative.
  const proc=spawnSync('ffmpeg',['-hide_banner','-nostats','-i',file,
    '-af','loudnorm=I=-23:TP=-1:LRA=11:print_format=json','-f','null','-'],
    {encoding:'utf8',maxBuffer:2*1024*1024,windowsHide:true});
  assert.equal(proc.status,0,proc.stderr);
  const match=proc.stderr.match(/\{\s*"input_i"[\s\S]*?\}/);
  assert(match,'FFmpeg loudness report missing');
  const value=JSON.parse(match[0]);
  return {integratedLufs:Number(value.input_i),truePeakDbtp:Number(value.input_tp),
    lraLu:Number(value.input_lra),analysisOnly:true};
}
function metric(w) {return {frames:w.frames,bytes:w.bytes,sha256:w.sha256,peak:w.peak,
  rms:w.rms,bodyCutSeam:w.seam};}
const renders={}, audit=[], referenceRepeat=[];
for(const variant of variants) {
  const dir=path.join(root,variant);
  const project=JSON.parse(fs.readFileSync(path.join(dir,'What_the_Dark_Keeps_Pocket_Chordsmith.json')));
  const pcs=fs.readFileSync(path.join(dir,'What_the_Dark_Keeps_PCS1.txt'),'utf8').trim();
  assert(pcs.startsWith('PCS1:'));
  assert.deepEqual(JSON.parse(Buffer.from(pcs.slice(5),'base64url').toString('utf8')),project);
  assert.deepEqual(protectedOriginalKeys(project),protectedOriginalKeys(original));
  const receipt=JSON.parse(fs.readFileSync(path.join(dir,'render/all-cues.json')));
  assert.deepEqual(receipt.renders.map(row=>row.cue),cues);
  assert(receipt.source.provenanceStable&&receipt.runtime.tailsSeparate);
  renders[variant]={};
  for(const row of receipt.renders) {
    const r=row.render,cue=row.cue;
    assert.deepEqual(row.pageErrors,[]); assert.deepEqual(r.errors,[]);
    assert(r.sourceMask.melodyStartsMatch&&r.sourceMask.holdsMatch);
    assert(r.calls.leadCallsMatch&&r.calls.chordCallsMatch&&r.calls.bassCallsMatch);
    assert(Object.values(r.calls.drumCountsMatch).every(Boolean));
    assert(r.sourceProof.coreBypassed&&r.sourceProof.leadVoiceUsesAudioContext&&r.sourceProof.chordVoiceUsesAudioContext);
    assert.equal(r.audio.sampleRate,48000); assert.equal(r.audio.channels,2);
    assert.equal(r.audio.bodyFrames,576000); assert.equal(r.audio.tailFrames,144000);
    assert.equal(r.audio.metrics.nonFiniteSamples,0); assert.equal(r.audio.metrics.clippedSamplesAtPcm16Ceiling,0);
    // Compare actual normalized source and scheduler to the independently rendered reference.
    if(variant!=='reference') {
      const baseline=renders.reference[cue].trace;
      assert.deepEqual(protectedOriginalKeys(r.auditionRoundTrip),protectedOriginalKeys(baseline.auditionRoundTrip));
      assert.deepEqual(protectedOriginalKeys(r.auditionPcsRoundTrip),protectedOriginalKeys(baseline.auditionPcsRoundTrip));
      assert.deepEqual(r.eventTrace,baseline.eventTrace);
      assert.deepEqual(r.schedule.steps,baseline.schedule.steps);
      assert.deepEqual(r.calls.actualChordCalls,baseline.calls.actualChordCalls);
      assert.deepEqual(r.calls.actualBassPhraseCalls,baseline.calls.actualBassPhraseCalls);
      assert.deepEqual(r.calls.actualDrumCounts,baseline.calls.actualDrumCounts);
      const musicalLead=calls=>calls.map(({instrument,...musical})=>musical);
      assert.deepEqual(musicalLead(r.calls.actualLeadPhraseCalls),musicalLead(baseline.calls.actualLeadPhraseCalls));
    }
    const body=wave(path.join(dir,`render/${cue}-loop.wav`),576000);
    const tail=wave(path.join(dir,`render/${cue}-tail.wav`),144000);
    for(const [kind,w] of [['loop',body],['tail',tail]]) {
      assert.equal(w.sha256,row.outputs.find(o=>o.path===`${cue}-${kind}.wav`).sha256);
      const floatPeak=kind==='loop'?r.audio.metrics.bodyPeak:r.audio.metrics.tailPeak;
      const floatRms=kind==='loop'?r.audio.metrics.bodyRms:r.audio.metrics.tailRms;
      assert(Math.abs(w.peak-floatPeak)<=2/32767); assert(Math.abs(w.rms-floatRms)<=2/32767);
    }
    let auditionSource=path.join(dir,`render/${cue}-loop.wav`);
    if(variant==='reference') {
      const old=wave(path.join(music,`runtime/${cue}-body.wav`),576000);
      const oldTail=wave(path.join(music,`runtime/${cue}-tail.wav`),144000);
      let maxPcmLsb=0,differingSamples=0;
      for(let i=0;i<body.samples.length;i++) {
        const delta=Math.round(Math.abs(body.samples[i]-old.samples[i])*32767);
        maxPcmLsb=Math.max(maxPcmLsb,delta); differingSamples+=delta>0;
      }
      assert(maxPcmLsb<=1,'Pinned-route repeat differs beyond documented one-LSB bound');
      assert.equal(tail.sha256,oldTail.sha256);
      referenceRepeat.push({cue,maxPcmLsb,differingSamples,tailByteIdentical:true});
      auditionSource=path.join(music,`runtime/${cue}-body.wav`);
    }
    renders[variant][cue]={body,tail,trace:r,source:auditionSource,
      previewSource:variant==='reference'?wave(auditionSource,576000):body};
    audit.push({variant,cue,body:metric(body),tail:metric(tail),scheduleEquivalent:true,
      jsonPcs1Equivalent:true,floatFiniteAndUnclipped:true,sourceInstrument:r.auditionRoundTrip[`melodyInstruments${cue}`],
      loudness:measure(auditionSource)});
  }
}
// Validate twenty exact periods using the unchanged previous-tail-over-next-body contract.
const loopChecks=[];
for(const variant of variants) for(const cue of cues) {
  const {body,tail}=renders[variant][cue];
  let peak=0,maxBoundary=0,maxTailOff=0;
  const sample=(frame,ch,cycle)=>body.samples[frame*2+ch]+(cycle>0&&frame<tail.frames?tail.samples[frame*2+ch]:0);
  for(let cycle=0;cycle<20;cycle++) {
    for(let frame=0;frame<body.frames;frame++) for(let ch=0;ch<2;ch++) peak=Math.max(peak,Math.abs(sample(frame,ch,cycle)));
    if(cycle>0) for(let ch=0;ch<2;ch++) maxBoundary=Math.max(maxBoundary,
      Math.abs(sample(0,ch,cycle)-sample(body.frames-1,ch,cycle-1)));
    if(cycle>0) for(let ch=0;ch<2;ch++) maxTailOff=Math.max(maxTailOff,
      Math.abs(sample(tail.frames,ch,cycle)-sample(tail.frames-1,ch,cycle)));
  }
  assert(peak<1,'Loop-tail sum clips');
  loopChecks.push({variant,cue,periodFrames:576000,periodCount:20,totalBodyFrames:11520000,
    peak,loopBoundaryDelta:maxBoundary,tailOffDelta:maxTailOff,
    claim:'offline bounded PCM arithmetic, not native playback or audible-seam acceptance'});
}
const transitions=[];
for(const variant of variants) for(const direction of [['A','E'],['E','A']]) {
  const out=renders[variant][direction[0]], incoming=renders[variant][direction[1]];
  for(const repeatCycle of [0,1]) for(const exitFrame of [48000,288000,575999]) {
    let peak=0,maxDelta=0,prior=[0,0];
    const outgoing=(index,ch)=>{
      const frame=index%576000,cycle=repeatCycle+Math.floor(index/576000);
      return out.body.samples[frame*2+ch]+(cycle>0&&frame<144000?out.tail.samples[frame*2+ch]:0);
    };
    for(let frame=0;frame<12000;frame++) for(let ch=0;ch<2;ch++) {
      const weight=frame/12000;
      const s=outgoing(exitFrame+frame,ch)*(1-weight)+incoming.body.samples[frame*2+ch]*weight;
      peak=Math.max(peak,Math.abs(s)); if(frame)maxDelta=Math.max(maxDelta,Math.abs(s-prior[ch])); prior[ch]=s;
    }
    assert(peak<1);
    transitions.push({variant,from:direction[0],to:direction[1],repeatCycle,exitFrame,crossfadeFrames:12000,peak,maxAdjacentDelta:maxDelta,
      claim:'linear250ms arithmetic with outgoing loop wrap and previous-tail overlap; not actual Core clocks or audible acceptance'});
  }
}
fs.mkdirSync(output);
if (transitionSupplement) {
  const existing=path.join(root,ownerSupplement?'verified-owner-combo':'verified-previews','verification.json');
  const previous=JSON.parse(fs.readFileSync(existing));
  const report={schema:1,status:'PASS-objective-transition-supplement; owner-listening-pending',
    reason:'Review found earlier illustrative transition arithmetic omitted previous-tail overlap and loop wrap. Earlier receipt retained; no render or preview regenerated.',
    supersedesOnly:'transitions',previousReceiptSha256:sha(fs.readFileSync(existing)),
    verifierSha256:sha(fs.readFileSync(__filename)),transitions,loopChecks,
    retainedPreviews:previous.previews,unchangedRuntime:previous.unchangedRuntime};
  writeNew(path.join(output,'transition-verification.json'),`${JSON.stringify(report,null,2)}\n`);
  console.log(JSON.stringify({status:report.status,transitionCount:transitions.length,
    peak:Math.max(...transitions.map(row=>row.peak)),previewRegenerated:false},null,2));
  process.exit(0);
}
const previewPieces=Object.fromEntries(variants.map(id=>[id,{}]));
const matching=[];
for(const cue of cues) {
  const target=audit.find(row=>row.variant==='reference'&&row.cue===cue).loudness.integratedLufs;
  for(const variant of variants) {
    const source=audit.find(row=>row.variant===variant&&row.cue===cue);
    const gainDb=target-source.loudness.integratedLufs,gain=10**(gainDb/20);
    const samples=Float64Array.from(renders[variant][cue].previewSource.samples,x=>x*gain);
    const name=`${variant}-${cue}-matched.wav`,file=path.join(output,name);
    writeNew(file,encode(samples));
    const actual=measure(file); assert(Math.abs(actual.integratedLufs-target)<=0.15);
    assert(actual.truePeakDbtp<=-1,'Matched preview true peak lacks headroom');
    previewPieces[variant][cue]=samples;
    matching.push({variant,cue,path:name,gainDb,targetLufs:target,...actual,
      sha256:sha(fs.readFileSync(file)),processing:'constant gain only; preview not runtime asset'});
  }
}
const previews=[];
for(const [index,variant] of variants.filter(id=>id!=='reference').entries()) {
  const order=[['reference','A'],[variant,'A'],['reference','E'],[variant,'E']];
  const gapFrames=24000,totalFrames=4*576000+3*gapFrames;
  const samples=new Float64Array(totalFrames*2); let offset=0;
  const timeline=[];
  for(const [id,cue] of order) {
    timeline.push({startSeconds:offset/48000,variant:id,cue,durationSeconds:12});
    samples.set(previewPieces[id][cue],offset*2); offset+=576000+gapFrames;
  }
  const name=`0${index+1}-${variant}-comparison.wav`,file=path.join(output,name);
  writeNew(file,encode(samples)); const actual=wave(file,totalFrames);
  previews.push({variant,path:name,seconds:totalFrames/48000,sha256:actual.sha256,timeline,
    note:'body-only short audition with0.5s silence; full bodies/tails remain separate and unchanged'});
}
// Asset pin/footprint remains exactly the current sixteen-file production bank.
for(const entry of originalManifest.sources) assert.equal(sha(fs.readFileSync(path.join(music,entry.path))),entry.sha256);
for(const cue of originalManifest.cues) for(const part of [cue.body,cue.tail]) {
  const file=fs.readFileSync(path.join(music,part.path)); assert.equal(file.length,part.bytes); assert.equal(sha(file),part.sha256);
}
const report={schema:1,status:'PASS-objective-checks; owner-listening-pending',
  scope:'instrumentation audition only; no runtime bank/PCM playback/memory change',
  referenceRepeat,audit,loopChecks,transitions,matching,previews,
  unchangedRuntime:{manifestSha256:sha(fs.readFileSync(path.join(music,'asset.manifest.json'))),
    waveBytes:originalManifest.totalWaveBytes,pcmBytes:originalManifest.totalPcmBytes},
  limitations:['Preset names are not evidence of realistic acoustic timbre.','Audible loop/transition acceptance remains open.',
    'Combat masking requires actual mixed-game listening; no spatialized SFX guarantee from these checks.',
    'No phone install or native playback changes in this pass.'],
  inputs:{verifierSha256:sha(fs.readFileSync(__filename)),sourceVariantsSha256:sha(fs.readFileSync(path.join(root,
    ownerSupplement ? 'source-variants-owner-combo.json' : 'source-variants.json')))}};
writeNew(path.join(output,'verification.json'),`${JSON.stringify(report,null,2)}\n`);
console.log(JSON.stringify({status:report.status,referenceRepeat,matching,previews,unchangedRuntime:report.unchangedRuntime},null,2));
