import { pathToFileURL as externalModuleURL } from 'node:url';
const externalRoot = process.env.POCKET_CHORDSMITH_ROOT;
if (!externalRoot) throw new Error('Set POCKET_CHORDSMITH_ROOT to your separately authorized checkout at 0c6975cadcd4aba0047ab94170a2adabc722517f');
import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';
const {canonicalizePcsProject,validatePcsProject,encodePcsProject,parsePcsProject} = await import(externalModuleURL(externalRoot + '/packages/pcs-format/src/index.js').href);
const {normalisePocketChordsmithProject} = await import(externalModuleURL(externalRoot + '/packages/pocket-audio-core/src/schema/normalise-project.js').href);
const {buildPocketAudioTimeline} = await import(externalModuleURL(externalRoot + '/packages/pocket-audio-core/src/events/timeline-events.js').href);
const root=path.resolve(import.meta.dirname,'..');fs.mkdirSync(root+'/editor-compatible',{recursive:true});
const audit=JSON.parse(fs.readFileSync(root+'/evidence/editor-compatibility.json'));let items=[];
for(const item of audit.scores.filter(x=>!x.compactGridRegisterCompatible)){
 const canonical=JSON.parse(fs.readFileSync(root+'/'+item.score));const copy=structuredClone(canonical);
 copy.title+=' — optional editor octave projection';copy.projectVersion=16;delete copy.sections;
 const shifts=[];
 for(const s of 'ABCDEFGH')for(let t=0;t<copy['melodyOctaves'+s].length;t++){
  const before=copy['melodyOctaves'+s][t],after=Math.max(-1,Math.min(1,before));
  if(before!==after){copy['melodyOctaves'+s][t]=after;const events=copy['melodyTracks'+s][t].flatMap((n,step)=>n===null||step>=copy.sectionBars[s]*16||copy['melodyHold'+s][t][step]?[]:[{step,beforeMidi:72+12*before+n,afterMidi:72+12*after+n}]);if(events.length)shifts.push({section:s,track:t+1,semitones:12*(after-before),events});}
 }
 copy.compositionProvenance={...copy.compositionProvenance,edition:'OPTIONAL current-editor octave projection; canonical original remains unchanged',canonicalSource:item.score,changed:'Listed melody tracks deliberately moved up one octave. Bass, note timing, rhythm, harmony and instruments unchanged.'};
 const out=canonicalizePcsProject(copy);assert(out.ok);out.project.soundProfile=canonical.soundProfile;
 for(const sec of Object.values(out.project.sections))for(const tr of Object.values(sec.tracks||{}))tr.compatibility={...(tr.compatibility||{}),compactMirror:true,liveMirror:false};
 assert(validatePcsProject(out.project).ok);
 const before=buildPocketAudioTimeline(normalisePocketChordsmithProject(canonical)),after=buildPocketAudioTimeline(normalisePocketChordsmithProject(out.project));assert.equal(before.events.length,after.events.length);
 for(let i=0;i<before.events.length;i++){
  const a=before.events[i],b=after.events[i];assert.equal(a.time,b.time);assert.equal(a.duration,b.duration);assert.equal(a.type,b.type);assert.equal(a.instrument,b.instrument);
  if(a.stem!=='melody')assert.deepEqual(a,b,'A non-melody event changed');else assert([0,12].includes(b.midi-a.midi));
 }
 const name=path.basename(item.score,'.json')+'.editor-compatible.json';const code=encodePcsProject(out.project);assert.deepEqual(parsePcsProject(code).project,out.project);
 fs.writeFileSync(root+'/editor-compatible/'+name,JSON.stringify(out.project,null,2)+'\n');fs.writeFileSync(root+'/editor-compatible/'+name.replace('.json','.pcs1.txt'),code+'\n');
 items.push({canonicalSource:item.score,editorEdition:name,formatValid:true,pcsRoundtrip:true,eventCountUnchanged:true,allNonMelodyEventsUnchanged:true,shiftedTracks:shifts,browserImportExport:'pending_actual_app_verification'});
}
fs.writeFileSync(root+'/editor-compatible/EXACT-OCTAVE-SHIFTS.json',JSON.stringify({purpose:'Optional consciously changed range projection; not the canonical intended performance and not a bass-rendering fix.',items},null,2)+'\n');
fs.writeFileSync(root+'/editor-compatible/READ-ME-FIRST.txt','OPTIONAL CURRENT-EDITOR EDITION\n\nThese11 supplemental banks deliberately move only out-of-range melody tracks up one octave so the current grid can edit them. EXACT-OCTAVE-SHIFTS.json lists every changed note. The underlying low bass notes are unchanged; this is NOT the bass-rendering fix.\n\nThe15 canonical family scores and2 event banks remain unchanged in their original folders. Six other canonical banks are already within the current editor register range.\n\nActual app import/export verification is a separate gate; see the final browser evidence before using these as editor-ready.\n');
console.log(`Created${items.length} explicit editor octave projections; canonical scores and bass events unchanged.`);
