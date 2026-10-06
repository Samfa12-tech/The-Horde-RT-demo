#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';
import { loadCore, renderSelection, foldTailToLoop, applyGain, basicMetrics, sha256 } from './render-core-reference.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const out = path.join(root, 'previews/bass-voice-alternative');
const core = await loadCore();
const familyManifest = JSON.parse(fs.readFileSync(path.join(root, 'manifests/cue-manifest.json')));
const tour = JSON.parse(fs.readFileSync(path.join(root, 'previews/listening-tour-chapters.json')));
const selections = [['01-what-the-dark-keeps','A'],['02-tomb','B'],['04-forest','B'],['06-bellwether','B'],['07-abbey','B'],['03-fourth-keeper','B'],['03-fourth-keeper','D'],['11-glass-court','B'],['14-entity','E'],['15-homecoming','H']];
const basenames = ['01-what-the-dark-keeps-A','06-bellwether-B'];
const buffers = new Map();
const report = {
  title: 'Soft-upright Core voice alternative, same notes and timing',
  classification: 'Alternate Core bass voice only. Not actual app-filter rendering or production mastering.',
  engineCommit: familyManifest.engine_revision,
  change: 'On a render-only clone, set bassTone=soft_upright on bass events. Leave every other event field and all non-bass events unchanged.',
  reasonForEventLevelOverride: 'In the Standard profile, raw.bassTone=soft_upright is discarded by current Core normalization; verified comparison emitted classic in both cases.',
  gainPolicy: 'Identical existing bank gain for both members of every pair. No stem/clip normalization, EQ or limiter.',
  canonicalScoresChanged: false, previousAudioChanged: false, excerpts: []
};
if (!process.argv.includes('--allow-render-only-event-voice-override')) {
  throw new Error('Requires explicit render-only event bassTone override authorization flag.');
}
fs.mkdirSync(out, { recursive: true });
function encode(buffer, file) {
  const bytes = core.encodePcm16WavBytes(buffer);
  fs.writeFileSync(file, bytes);
  return sha256(bytes);
}
function mp3(wav) {
  const dest = wav.replace(/\.wav$/, '.mp3');
  const job = spawnSync('ffmpeg', ['-hide_banner','-loglevel','error','-y','-i',wav,'-c:a','libmp3lame','-q:a','2',dest], { encoding:'utf8' });
  if (job.status !== 0) throw new Error(job.error || job.stderr);
  return dest;
}
function withoutTone(events) {
  return events.map(event => { const x = {...event}; if (x.stem === 'bass') delete x.bassTone; return x; });
}
for (const [index, [bank, sectionId]] of selections.entries()) {
  const scorePath = path.join(root,'scores',`${bank}.json`);
  const bytes = fs.readFileSync(scorePath);
  const raw = JSON.parse(bytes);
  const family = familyManifest.families.find(x => x.score === `${bank}.json`);
  const section = family.sections[sectionId];
  const gain = JSON.parse(fs.readFileSync(path.join(root,'section-audio',bank,'bank-audio-manifest.json'))).commonGainDb;
  const loop = ['loop','bridge'].includes(section.behavior);
  const original = await renderSelection(raw,{sectionId,loop,sampleRate:44100,tailSeconds:0.6});
  const originalEvents = original.timeline.events;
  const alternateEvents = originalEvents.map(event => event.stem === 'bass' ? {...event,bassTone:'soft_upright'} : {...event});
  const eventHash = sha256(JSON.stringify(withoutTone(originalEvents)));
  const alternateEventHash = sha256(JSON.stringify(withoutTone(alternateEvents)));
  if (eventHash !== alternateEventHash) throw new Error('A non-voice event field changed.');
  const alternateRaw = core.renderPocketAudioEventBuffer(alternateEvents,{
    sampleRate:44100,durationSeconds:original.timeline.duration,
    tailSeconds:original.report.options.renderedTailSeconds,lofiTexture:null
  });
  const alternate = applyGain(loop ? foldTailToLoop(alternateRaw,original.timeline.duration) : alternateRaw,gain);
  const classic = applyGain(original.rendered,gain);
  const name = `${bank}-${sectionId}`;
  const existing = path.join(root,'section-audio',bank,`${sectionId}-${section.name}.wav`);
  const baselineHash = sha256(core.encodePcm16WavBytes(classic));
  if (baselineHash !== sha256(fs.readFileSync(existing))) throw new Error('Classic reconstruction differs from existing authoritative section PCM.');
  const metrics = basicMetrics(alternate);
  if (metrics.clippedSamples || metrics.nonFiniteSamples) throw new Error('Alternate failed clipping/nonfinite guard.');
  const outputName = `${name}.soft-upright-context.wav`;
  const outputHash = encode(alternate,path.join(out,outputName));
  const entry = {name,chapter:index+1,title:tour.chapters[index].title,sectionId,behavior:section.behavior,
    sourceScore:`scores/${bank}.json`,sourceSha256:sha256(bytes),
    fixedBankGainDb:gain,bassEventCount:alternateEvents.filter(e=>e.stem==='bass').length,
    exactAllEventEqualityExcludingBassTone:eventHash===alternateEventHash,
    originalEventsWithoutToneSha256:eventHash,alternateEventsWithoutToneSha256:alternateEventHash,
    originalContextPcmMatchesPreviouslyDeliveredWav:true,
    originalContextSha256:baselineHash,alternateContextSha256:outputHash,
    outputWav:outputName,metrics,
    originalBassNotes:originalEvents.filter(e=>e.stem==='bass').map(e=>({midi:e.midi,time:e.time,duration:e.duration,velocity:e.velocity})),
    events:alternateEvents};
  fs.writeFileSync(path.join(out,`${name}.comparison.json`),JSON.stringify(entry,null,2)+'\n');
  report.excerpts.push(entry);
  if (basenames.includes(name)) {
    const bass = {};
    for (const [kind,events] of [['classic',originalEvents],['soft-upright',alternateEvents]]) {
      const rendered = core.renderPocketAudioEventBuffer(events.filter(e=>e.stem==='bass'),{sampleRate:44100,durationSeconds:original.timeline.duration,tailSeconds:0.6});
      bass[kind]=applyGain(rendered,gain);
      const file=path.join(out,`${name}.${kind}-bass.wav`);
      encode(bass[kind],file);mp3(file);
    }
    buffers.set(name,{classic,alternate,bass});
  }
  if (sha256(fs.readFileSync(scorePath))!==entry.sourceSha256) throw new Error('Canonical source changed during test.');
}
// A short listening comparison; editorial edge fades only in the montage, not the source clips.
const chapters=[];const chunks=[];let elapsedFrames=0;
for (const name of basenames) {
  const b=buffers.get(name);const label=name.startsWith('01')?'Main theme':'Bellwether';
  for(const [type,variant,buffer,seconds] of [
    ['bass only','classic',b.bass.classic,6],['bass only','soft upright',b.bass['soft-upright'],6],
    ['full context','classic',b.classic,b.classic.duration],['full context','soft upright',b.alternate,b.alternate.duration]
  ]) {
    const frames=Math.min(buffer.channels[0].length,Math.round(seconds*44100));
    const channels=buffer.channels.map(channel=>channel.slice(0,frames));
    const fade=Math.min(Math.round(0.01*44100),Math.floor(frames/2));
    for(const channel of channels)for(let i=0;i<fade;i++) {channel[i]*=i/fade;channel[frames-1-i]*=i/fade;}
    chunks.push(channels);
    chapters.push({startSeconds:elapsedFrames/44100,endSeconds:(elapsedFrames+frames)/44100,title:`${label}: ${type}, ${variant}`,name});
    elapsedFrames+=frames;
    const gap=Math.round(.4*44100);chunks.push([new Float32Array(gap),new Float32Array(gap)]);elapsedFrames+=gap;
  }
}
const joined=[new Float32Array(elapsedFrames),new Float32Array(elapsedFrames)];let cursor=0;
for(const chunk of chunks){joined[0].set(chunk[0],cursor);joined[1].set(chunk[1],cursor);cursor+=chunk[0].length;}
const montage={channels:joined,sampleRate:44100,duration:elapsedFrames/44100};
const montageFile=path.join(out,'Bass-Voice-Comparison-Classic-vs-Soft-Upright.wav');
encode(montage,montageFile);mp3(montageFile);
report.comparisonMontage={wav:path.basename(montageFile),mp3:path.basename(montageFile.replace('.wav','.mp3')),
  durationSeconds:montage.duration,edits:'10 ms editorial edge fades and400 ms gaps; fixed bank gain unchanged.',chapters,metrics:basicMetrics(montage)};
fs.writeFileSync(path.join(out,'bass-voice-alternative-manifest.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({excerpts:report.excerpts.length,allEventParity:report.excerpts.every(x=>x.exactAllEventEqualityExcludingBassTone),
  durationSeconds:montage.duration,montageMp3:montageFile.replace('.wav','.mp3')},null,2));
