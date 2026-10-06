import { pathToFileURL as externalModuleURL } from 'node:url';
const externalRoot = process.env.POCKET_CHORDSMITH_ROOT;
if (!externalRoot) throw new Error('Set POCKET_CHORDSMITH_ROOT to your separately authorized checkout at 0c6975cadcd4aba0047ab94170a2adabc722517f');
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const {canonicalizePcsProject,validatePcsProject,encodePcsProject,parsePcsProject} = await import(externalModuleURL(externalRoot + '/packages/pcs-format/src/index.js').href);
const {normalisePocketChordsmithProject} = await import(externalModuleURL(externalRoot + '/packages/pocket-audio-core/src/schema/normalise-project.js').href);
const {buildPocketAudioTimeline} = await import(externalModuleURL(externalRoot + '/packages/pocket-audio-core/src/events/timeline-events.js').href);
const {POCKET_CHORD_INSTRUMENTS,POCKET_MELODY_INSTRUMENTS} = await import(externalModuleURL(externalRoot + '/packages/pocket-audio-core/src/sounds/instruments.js').href);
const root=path.resolve(import.meta.dirname,'..');const results=[];
for(const name of fs.readdirSync(root+'/event-scores').filter(x=>x.endsWith('.raw.json')).sort()){
 const raw=JSON.parse(fs.readFileSync(root+'/event-scores/'+name));
 const can=canonicalizePcsProject(raw);assert(can.ok,JSON.stringify(can));
 const project=can.project; project.soundProfile=normalisePocketChordsmithProject(raw).soundProfile;
 // Format migration emits non-authoritative sparse mirrors with placeholder durations.
 // Explicit mirror status keeps original compact holds/mutes/pan/mix authoritative.
 for(const sec of Object.values(project.sections))for(const tr of Object.values(sec.tracks||{}))tr.compatibility={...(tr.compatibility||{}),compactMirror:true,liveMirror:false};
 const val=validatePcsProject(project);assert(val.ok,JSON.stringify(val));
 assert(project.songSequence.length<=64);assert(POCKET_CHORD_INSTRUMENTS.includes(project.chordInstrument));
 for(const s of 'ABCDEFGH'){
  assert(project.sectionBars[s]>=1&&project.sectionBars[s]<=4);
  assert(project['melodyTracks'+s].length<=6);
  assert(project['melodyOctaves'+s].every(o=>Number.isInteger(o)&&o>=-2&&o<=2),'Unsupported Core compact melody register');
  for(const lane of Object.values(project['grid'+s]))assert.equal(lane.length,64);
  for(const tr of project['melodyTracks'+s]){assert.equal(tr.length,64);assert(tr.every(x=>x===null||(Number.isInteger(x)&&x>=0&&x<=23)));}
  for(const instr of project['melodyInstruments'+s])assert(POCKET_MELODY_INSTRUMENTS.includes(instr));
 }
 const code=encodePcsProject(project);const rt=parsePcsProject(code);assert(rt.ok);assert.deepEqual(rt.project,project);
 const normal=normalisePocketChordsmithProject(project);const timeline=buildPocketAudioTimeline(normal);
 const originalTimeline=buildPocketAudioTimeline(normalisePocketChordsmithProject(raw));
 assert.deepEqual(timeline.events,originalTimeline.events,'Schema17 migration changed performed notes, timing, velocity or instrumentation');
 const bySection={};for(const s of 'ABCDEFGH'){
  const t=buildPocketAudioTimeline(normal,{scope:'section',sectionId:s});
  const expected=[];
  const anySolo=project['melodySolo'+s].some(Boolean);
  for(let tr=0;tr<project['melodyTracks'+s].length;tr++){
   if(project['melodyMute'+s][tr]||(anySolo&&!project['melodySolo'+s][tr]))continue;
   for(let step=0;step<project.sectionBars[s]*16;step++){
    const note=project['melodyTracks'+s][tr][step];if(note===null||project['melodyHold'+s][tr][step]||project['melodySlide'+s][tr][step])continue;
    let span=1;while(step+span<project.sectionBars[s]*16&&project['melodyHold'+s][tr][step+span])span++;
    expected.push({step,tr,midi:72+12*project['melodyOctaves'+s][tr]+note,instrument:project['melodyInstruments'+s][tr],duration:Math.max(.18,span*60/project.bpm/project.resolution*.92)});
   }
  }
  expected.sort((a,b)=>a.step-b.step||a.tr-b.tr);
  const performed=t.events.filter(e=>e.stem==='melody').map(e=>({step:e.step,tr:Number(e.id.match(/_(\d+)$/)[1]),midi:e.midi,instrument:e.instrument,duration:e.duration})).sort((a,b)=>a.step-b.step||a.tr-b.tr);
  assert.equal(performed.length,expected.length,'Performed melody event count differs from source intent');
  expected.forEach((e,i)=>{const a=performed[i];for(const k of ['step','tr','midi','instrument'])assert.equal(a[k],e[k],name+s+k);assert(Math.abs(a.duration-e.duration)<1e-8,name+s+'duration');});
  bySection[s]={allAuthoredMelodyPitchesVoicesAndDurationsVerified:true,duration:t.duration,eventCount:t.events.length,melodyMidi:t.events.filter(e=>e.stem==='melody'&&(e.trackIndex===0||e.id.endsWith('_0'))).map(e=>e.midi),losses:t.lossReport};
 }
 if(name.startsWith('01-')){
  const ta=buildPocketAudioTimeline(normal,{scope:'section',sectionId:'A'});
  const lead=ta.events.filter(e=>e.stem==='melody'&&(e.track===0||(e.trackIndex===0||e.id.endsWith('_0'))||e.trackId==='melody1'));
  const notes=project.melodyTracksA[0].flatMap((x,i)=>x===null?[]:[{step:i,midi:72+12*project.melodyOctavesA[0]+x}]);
  assert.deepEqual(notes.map(n=>n.midi),[62,63,69,64,62,65,64,64,62]);
  assert.deepEqual(notes.map(n=>n.step),[0,8,12,20,28,36,42,48,58]);
  assert(project.melodyTracksG[0].includes(18));assert(project.melodyTracksH[0].includes(6));
  for(const s of 'AD')assert.equal(project['melodyHold'+s][2].filter(Boolean).length,0);
 }
 const outfile=name.replace('.raw.json','.json');fs.writeFileSync(root+'/event-scores/'+outfile,JSON.stringify(project,null,2)+'\n');
 fs.writeFileSync(root+'/share-codes/'+outfile.replace('.json','.pcs1.txt'),code+'\n');
 results.push({file:outfile,valid:true,schema:project.projectVersion,shareRoundtripExact:true,sectionDimensions:true,instrumentsKnown:true,compactToRichTimelineParity:true,warnings:can.warnings||[],timelineSeconds:timeline.duration,eventCount:timeline.events.length,sectionReports:bySection});
}
fs.writeFileSync(root+'/evidence/event-score-validation.json',JSON.stringify({allPassed:results.length===2&&results.every(x=>x.valid),count:results.length,results},null,2)+'\n');console.log(JSON.stringify(results.map(({file,eventCount,timelineSeconds})=>({file,eventCount,timelineSeconds})),null,2));
