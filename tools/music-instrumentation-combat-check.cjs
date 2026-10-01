// Bounded offline near-field mix probe. Not live spatialization/listening proof.
const fs=require('node:fs'), path=require('node:path'), assert=require('node:assert/strict');
const crypto=require('node:crypto');
const repo=path.resolve(__dirname,'..'), root='C:/Dev/tmp/horde-music-instrumentation-20261001';
const destination=path.join(root,'combat-check');
if(fs.existsSync(destination))throw new Error('Refusing to repeat completed combat probe.');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function read(file){
  const b=fs.readFileSync(file); assert.equal(b.toString('ascii',0,4),'RIFF');
  assert.equal(b.toString('ascii',8,16),'WAVEfmt ');assert.equal(b.readUInt16LE(20),1);
  assert.equal(b.readUInt32LE(24),48000);assert.equal(b.readUInt16LE(34),16);
  assert.equal(b.toString('ascii',36,40),'data');assert.equal(b.readUInt32LE(40),b.length-44);
  const channels=b.readUInt16LE(22);assert(channels===1||channels===2);
  const data=new Float64Array((b.length-44)/2);
  for(let i=0;i<data.length;i++)data[i]=b.readInt16LE(44+i*2)/32767;
  return{b,data,channels,frames:data.length/channels,sha256:sha(b)};
}
const specs=[{cue:'A',file:'skeleton_attack.wav',gain:0.85},
  {cue:'E',file:'lich_charge.wav',gain:0.42},{cue:'E',file:'sword_swing_1.wav',gain:1}];
const rows=[], listening=[];fs.mkdirSync(destination);
for(const variant of ['reference','whistle','reed','owner-combo']) {
  for(const spec of specs){
    const bodyFile=variant==='reference'?path.join(repo,`assets/audio/music/what-the-dark-keeps/runtime/${spec.cue}-body.wav`):
      path.join(root,variant,`render/${spec.cue}-loop.wav`);
    const body=read(bodyFile), sfx=read(path.join(repo,'assets/audio/filmcow',spec.file));
    const musicGain=0.70, startFrame=3*48000;
    let musicSquares=0,sfxSquares=0,peak=0;
    assert.equal(body.channels,2); assert.equal(sfx.channels,1);
    for(let frame=0;frame<sfx.frames;frame++)for(let ch=0;ch<2;ch++){
      const index=(startFrame+frame)*2+ch;if(index>=body.data.length)break;
      const m=body.data[index]*musicGain,s=sfx.data[frame]*spec.gain;
      musicSquares+=m*m;sfxSquares+=s*s;peak=Math.max(peak,Math.abs(m+s));
    }
    const count=Math.min(sfx.frames,body.frames-startFrame)*2;
    rows.push({variant,cue:spec.cue,sfx:spec.file,musicGain,sfxGain:spec.gain,
      attenuation:1,stereoModel:'mono duplicated to both channels; conservative near-field probe, not runtime pan',
      sfxToMusicRmsDb:10*Math.log10(sfxSquares/musicSquares),mixedPeak:peak,
      musicRms:Math.sqrt(musicSquares/count),sfxRms:Math.sqrt(sfxSquares/count),
      musicSha256:body.sha256,sfxSha256:sfx.sha256});
  }
  if(variant==='owner-combo'){
    const body=read(path.join(root,variant,'render/E-loop.wav'));
    const samples=Float64Array.from(body.data,x=>x*0.70);
    const events=[{file:'lich_charge.wav',time:3,gain:0.42},{file:'sword_swing_1.wav',time:7,gain:1}];
    for(const event of events){const w=read(path.join(repo,'assets/audio/filmcow',event.file));
      for(let frame=0;frame<w.frames;frame++)for(let ch=0;ch<2;ch++){
        const index=(event.time*48000+frame)*2+ch;if(index<samples.length)samples[index]+=w.data[frame]*event.gain;
      }
    }
    const peak=samples.reduce((m,x)=>Math.max(m,Math.abs(x)),0);
    const row={variant,musicGain:0.70,events,peak,clipping:peak>=1,
      acceptance:'owner listening required; illustrative near-field mix, not installed-game acceptance'};
    if(peak<1){
      const bytes=Buffer.alloc(44+samples.length*2);body.b.copy(bytes,0,0,44);
      for(let i=0;i<samples.length;i++)bytes.writeInt16LE(Math.round(samples[i]*32767),44+i*2);
      row.path='owner-combo-combat-context.wav';row.sha256=sha(bytes);
      fs.writeFileSync(path.join(destination,row.path),bytes,{flag:'wx'});
    }
    listening.push(row);
  }
}
const report={schema:1,rows,listening,scope:'Offline default70% music with unchanged authored combat assets/gains. No SFX/pan/gain/runtime edits.',
  limits:['RMS ratios do not establish perceptual masking.','No phone speakers, obstruction/distance, other simultaneous sounds or live event timing tested.',
    'Preview loudness gains are deliberately NOT used in this game-context check.'],
  verdict:'No subjective intelligibility pass; owner must listen to warning/impact cues in the game.',verifierSha256:sha(fs.readFileSync(__filename))};
fs.writeFileSync(path.join(destination,'combat-verification.json'),`${JSON.stringify(report,null,2)}\n`,{flag:'wx'});
console.log(JSON.stringify({rows,listening,verdict:report.verdict},null,2));
