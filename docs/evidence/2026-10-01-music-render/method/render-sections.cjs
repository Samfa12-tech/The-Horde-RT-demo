const http = require("node:http");
const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");
const { chromium } = require("@playwright/test");

const repo = "C:/Users/sam_s/Documents/Pocket Chordsmith";
const output = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render-corrected";
const htmlPath = "apps/chordsmith-web/pocket_chordsmith_v68_core_bridge.html";
const allowedRoot = path.resolve(repo);
const htmlFile = path.join(repo, htmlPath);
const scoreBase64 = process.env.HORDE_MUSIC_SCORE_BASE64;
if (!scoreBase64) throw new Error("Set HORDE_MUSIC_SCORE_BASE64 from the authorized score entry.");
const zipFile = process.env.HORDE_MUSIC_SCORE_ZIP_PATH;
if (!zipFile) throw new Error("Set HORDE_MUSIC_SCORE_ZIP_PATH to the authorized owner archive.");
const cueSpecs = [
  { id: "A", frames: 576000 }, { id: "B", frames: 576000 }, { id: "C", frames: 144000 }, { id: "D", frames: 576000 },
  { id: "E", frames: 576000 }, { id: "F", frames: 576000 }, { id: "G", frames: 288000 }, { id: "H", frames: 576000 }
];
const rate = 48000, leaderFrames = 1920, tailFrames = 144000;
const scoreBytes = Buffer.from(scoreBase64, "base64");
const score = JSON.parse(scoreBytes.toString("utf8"));
const expectedHashes = {
  html: "b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf",
  ownerArchive: "e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa",
  score: "bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6"
};
const sha256 = bytes => crypto.createHash("sha256").update(bytes).digest("hex");
function provenance() {
  return {
    html: sha256(fs.readFileSync(htmlFile)),
    ownerArchive: sha256(fs.readFileSync(zipFile)),
    score: sha256(scoreBytes),
    adapter: sha256(fs.readFileSync(__filename))
  };
}
function assertProvenance(stage, hashes) {
  for (const key of ["html", "ownerArchive", "score"]) {
    if (hashes[key] !== expectedHashes[key]) throw new Error(`${stage} ${key} SHA-256 mismatch: ${hashes[key]}`);
  }
}
const provenanceBefore = provenance();
assertProvenance("pre-render", provenanceBefore);
const mime = { ".html": "text/html", ".js": "text/javascript", ".css": "text/css", ".json": "application/json", ".svg": "image/svg+xml", ".png": "image/png", ".woff2": "font/woff2" };
const server = http.createServer((request, response) => {
  const url = new URL(request.url, "http://127.0.0.1");
  const file = path.resolve(repo, `.${decodeURIComponent(url.pathname)}`);
  if (!file.startsWith(`${allowedRoot}${path.sep}`)) { response.writeHead(403); response.end("denied"); return; }
  fs.readFile(file, (error, bytes) => {
    if (error) { response.writeHead(404); response.end("not found"); return; }
    response.writeHead(200, { "content-type": mime[path.extname(file)] || "application/octet-stream" });
    response.end(bytes);
  });
});

function wavBase64(channels, sampleRate, start, length) {
  const byteLength = 44 + length * channels.length * 2;
  const bytes = Buffer.alloc(byteLength);
  bytes.write("RIFF", 0); bytes.writeUInt32LE(byteLength - 8, 4); bytes.write("WAVEfmt ", 8);
  bytes.writeUInt32LE(16, 16); bytes.writeUInt16LE(1, 20); bytes.writeUInt16LE(channels.length, 22);
  bytes.writeUInt32LE(sampleRate, 24); bytes.writeUInt32LE(sampleRate * channels.length * 2, 28);
  bytes.writeUInt16LE(channels.length * 2, 32); bytes.writeUInt16LE(16, 34); bytes.write("data", 36); bytes.writeUInt32LE(byteLength - 44, 40);
  let offset = 44;
  for (let frame = start; frame < start + length; frame++) for (const channel of channels) {
    bytes.writeInt16LE(Math.round(Math.max(-1, Math.min(1, channel[frame])) * 32767), offset); offset += 2;
  }
  return bytes.toString("base64");
}

(async () => {
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  const browser = await chromium.launch({ headless: true });
  const renders = [];
  try {
    for (const cue of cueSpecs) {
      const page = await browser.newPage();
      const pageErrors = [];
      page.on("pageerror", error => pageErrors.push(error.message));
      const totalFrames = leaderFrames + cue.frames + tailFrames;
      await page.addInitScript(() => {
        let randomState = 0x6d2b79f5;
        Math.random = () => {
          randomState = (randomState + 0x6d2b79f5) | 0;
          let value = Math.imul(randomState ^ (randomState >>> 15), 1 | randomState);
          value ^= value + Math.imul(value ^ (value >>> 7), 61 | value);
          return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
        };
        window.__resetMusicRng = () => { randomState = 0x6d2b79f5; };
        let virtualNow = 0;
        const frameCount = Number(new URL(window.location.href).searchParams.get("renderFrames"));
        class LiveOfflineContext extends OfflineAudioContext {
          constructor() {
            super(2, frameCount, 48000);
            Object.defineProperty(this, "currentTime", { configurable: true, get: () => virtualNow });
            window.__setLiveOfflineTime = time => { virtualNow = time; };
          }
          async resume() { return undefined; }
        }
        window.AudioContext = LiveOfflineContext;
        window.webkitAudioContext = LiveOfflineContext;
        window.__liveSourceStarts = 0;
        const originalStart = AudioScheduledSourceNode.prototype.start;
        AudioScheduledSourceNode.prototype.start = function (...args) { window.__liveSourceStarts++; return originalStart.apply(this, args); };
      });
      const url = `http://127.0.0.1:${server.address().port}/${htmlPath}?cue=${cue.id}&renderFrames=${totalFrames}`;
      await page.goto(url, { waitUntil: "networkidle", timeout: 60000 });
      const rendered = await page.evaluate(async ({ project, cueId, expectedBodyFrames, leaderFrames }) => {
        const errors = [];
        try { importProject(project); } catch (error) { errors.push(`importProject: ${error.message}`); }
        state.currentSection = cueId;
        syncSection();
        const fxIds = ["fxDelay", "fxChorus", "fxFlanger", "fxReverb", "fxMix"];
        for (const id of fxIds) if (els[id]) els[id].value = String(state[id]);
        window.__resetMusicRng();
        await ensureAudio();
        for (const id of fxIds) if (els[id]) els[id].value = String(state[id]);
        updateFx();

        const plan = buildPlaybackPlan("section");
        const section = getSectionData(cueId, true);
        const stepCountExpected = section.bars * stepsPerBar();
        const expectedDuration = section.bars * state.timeSig * beatDur();
        const bodyFrames = Math.round(expectedDuration * audioCtx.sampleRate);
        const bodyStart = leaderFrames;
        const bodyEnd = bodyStart + bodyFrames;
        const tailStart = bodyEnd;
        if (bodyFrames !== expectedBodyFrames || bodyEnd + 144000 !== audioCtx.length) errors.push(`window mismatch:${bodyFrames}/${expectedBodyFrames}/${audioCtx.length}`);

        const steps = [];
        const actualLeadPhraseCalls = [], actualChordCalls = [], actualBassPhraseCalls = [];
        const actualDrums = { kick: [], snare: [], hat: [], expanded: [] };
        const origLeadPhrase = window.playLeadPhraseInstrument, origChord = window.playChord, origBassPhrase = window.playBassPhrase;
        const origKick = window.playKick, origSnare = window.playSnare, origHat = window.playHat, origExpanded = window.playExpandedDrumLane;
        window.playLeadPhraseInstrument = function (...args) { actualLeadPhraseCalls.push({ midi: args[0], time: args[1], duration: args[2], instrument: args[3] }); return origLeadPhrase.apply(this, args); };
        window.playChord = function (...args) { actualChordCalls.push({ chord: args[0], time: args[1], duration: args[2] }); return origChord.apply(this, args); };
        window.playBassPhrase = function (...args) { actualBassPhraseCalls.push({ rootMidi: args[0], time: args[1], duration: args[2], slideMidi: args[5], slideOffset: args[6] }); return origBassPhrase.apply(this, args); };
        window.playKick = function (...args) { actualDrums.kick.push({ time: args[0] }); return origKick.apply(this, args); };
        window.playSnare = function (...args) { actualDrums.snare.push({ time: args[0] }); return origSnare.apply(this, args); };
        window.playHat = function (...args) { actualDrums.hat.push({ time: args[0] }); return origHat.apply(this, args); };
        window.playExpandedDrumLane = function (...args) { actualDrums.expanded.push({ lane: args[0], time: args[1] }); return origExpanded.apply(this, args); };

        const expectedLeadPhraseCalls = [], expectedChordCalls = [], expectedBassCalls = [];
        const expectedDrums = { kick: [], snare: [], hat: [], expanded: [] };
        let time = 0.04, uiTimerCalls = 0;
        const originalSetTimeout = window.setTimeout;
        window.setTimeout = () => { uiTimerCalls++; return 0; };
        for (let index = 0; index < plan.length; index++) {
          const item = plan[index], step = item.step, onset = time;
          const duration = stepDurationForIndex(step % Math.max(1, item.stepCount));
          steps.push({ index, step, time: onset, duration });
          const bar = Math.floor(step / stepsPerBar());
          if (state.chordsOn && step % stepsPerBar() === 0) for (const [chordTime, chordDuration] of chordRhythmStarts(onset)) expectedChordCalls.push({ chord: section.progression[bar] || state.availableChords[0], time: chordTime, duration: chordDuration });
          if (state.bassOn && !gridTripletSecond(section, "bass", step) && bassStepHasTrigger(section, step) && !(section.bassHold || [])[step] && !(section.bassSlide || [])[step]) {
            const phrase = bassPhraseInfo(section, step);
            const midi = bassStepMidiAt(section, step);
            if (midi !== null) expectedBassCalls.push({ rootMidi: midi, time: onset + humanizeOffset(step, 4) + funkPocketOffset(step), duration: phrase.dur, slideMidi: phrase.slideMidi, slideOffset: phrase.slideOffset });
          }
          for (const trackId of ["kick", "snare", "hat"]) {
            const level = normalizeBeatCell(section.grid[trackId][step]);
            if (level > 0 && !gridTripletSecond(section, trackId, step)) expectedDrums[trackId].push(onset + humanizeOffset(step, trackId === "kick" ? 1 : trackId === "snare" ? 2 : 3) + funkPocketOffset(step));
          }
          EXPANDED_DRUM_LANES.forEach((lane, laneIndex) => { const level = normalizeBeatCell(section.drumLanes?.[lane]?.[step]); if (level > 0) expectedDrums.expanded.push({ lane, time: onset + humanizeOffset(step, 30 + laneIndex) + funkPocketOffset(step) }); });
          melodyTracksForCurrentMode(section).forEach((track, trackIndex) => {
            const hold = (section.melodyHold || [])[trackIndex] || [], slide = (section.melodySlide || [])[trackIndex] || [];
            if (hold[step] || slide[step] || melodyTripletSecond(section, trackIndex, step) || track[step] == null || !melodyTrackIsAudible(trackIndex, section.name) || melodyTripletStart(section, trackIndex, step)) return;
            const phrase = melodyPhraseInfo(section, trackIndex, step);
            expectedLeadPhraseCalls.push({ trackIndex, step, degree: track[step], midi: melodyIndexToMidi(track[step], section.melodyOctaves[trackIndex] ?? 0), time: onset + humanizeOffset(step, 10 + trackIndex), duration: phrase.dur, instrument: section.melodyInstruments[trackIndex] || "pulse" });
          });
          schedulePlanStep(item, onset);
          time += duration;
          window.__setLiveOfflineTime(time);
        }
        window.setTimeout = originalSetTimeout;

        const buffer = await audioCtx.startRendering();
        const channels = Array.from({ length: buffer.numberOfChannels }, (_, channel) => buffer.getChannelData(channel));
        const metrics = { peak: 0, bodyPeak: 0, tailPeak: 0, bodySquares: 0, tailSquares: 0, finiteSamples: 0, nonFiniteSamples: 0, clippedSamplesAtPcm16Ceiling: 0 };
        for (const channel of channels) for (let frame = 0; frame < buffer.length; frame++) {
          const sample = channel[frame];
          if (!Number.isFinite(sample)) { metrics.nonFiniteSamples++; continue; }
          metrics.finiteSamples++; const magnitude = Math.abs(sample); metrics.peak = Math.max(metrics.peak, magnitude);
          if (frame >= bodyStart && frame < bodyEnd) { metrics.bodyPeak = Math.max(metrics.bodyPeak, magnitude); metrics.bodySquares += sample * sample; }
          if (frame >= tailStart) { metrics.tailPeak = Math.max(metrics.tailPeak, magnitude); metrics.tailSquares += sample * sample; }
          if (magnitude >= 0.999969) metrics.clippedSamplesAtPcm16Ceiling++;
        }
        metrics.totalSamples = buffer.length * buffer.numberOfChannels;
        metrics.bodyRms = Math.sqrt(metrics.bodySquares / (bodyFrames * buffer.numberOfChannels));
        metrics.tailRms = Math.sqrt(metrics.tailSquares / (Math.max(1, buffer.length - tailStart) * buffer.numberOfChannels));
        delete metrics.bodySquares; delete metrics.tailSquares;
        const seam = Math.max(...channels.map(channel => Math.abs(channel[bodyEnd - 1] - channel[bodyStart])));

        const rawMelody = project[`melodyTracks${cueId}`] || [], rawHold = project[`melodyHold${cueId}`] || [];
        const rawStarts = rawMelody.map((track, trackIndex) => track.map((degree, step) => degree == null || rawHold[trackIndex]?.[step] ? null : ({ step, degree })).filter(Boolean));
        const importedStarts = (state[sectionPropKey("melodyTracks", cueId)] || []).map((track, trackIndex) => track.map((degree, step) => degree == null || section.melodyHold?.[trackIndex]?.[step] ? null : ({ step, degree })).filter(Boolean));
        const rawHolds = rawHold.map(track => track.flatMap((held, step) => held ? [step] : []));
        const importedHolds = (section.melodyHold || []).map(track => track.flatMap((held, step) => held ? [step] : []));
        const repeatedPitchStarts = rawStarts.map((track, trackIndex) => { const pairs=[]; for(let i=1;i<track.length;i++) if(track[i-1].degree===track[i].degree) pairs.push({trackIndex,previousStep:track[i-1].step,step:track[i].step,degree:track[i].degree}); return pairs; }).flat();
        const repeatRuns = (rows, trackKey) => rows.map((track, trackIndex) => { const runs=[]; let start=-1; for(let i=0;i<=track.length;i++){if(track[i]&&start<0)start=i;if((!track[i]||i===track.length)&&start>=0){runs.push({trackIndex,start,end:i-1,count:i-start});start=-1;}}return runs;}).flat();
        const melodyStartsWithMidi = (state[sectionPropKey("melodyTracks",cueId)] || []).map((track,trackIndex)=>track.map((degree,step)=>degree==null||section.melodyHold?.[trackIndex]?.[step]?null:{step,degree,midi:melodyIndexToMidi(degree,section.melodyOctaves?.[trackIndex]??0)}).filter(Boolean));
        const actualLead = actualLeadPhraseCalls;
        const leadCallsMatch = expectedLeadPhraseCalls.length === actualLead.length && expectedLeadPhraseCalls.every((expected,index)=>{const actual=actualLead[index];return expected.midi===actual.midi&&Math.abs(expected.time-actual.time)<1e-9&&Math.abs(expected.duration-actual.duration)<1e-9&&expected.instrument===actual.instrument;});
        const chordCallsMatch = expectedChordCalls.length === actualChordCalls.length && expectedChordCalls.every((expected,index)=>actualChordCalls[index].chord===expected.chord&&Math.abs(actualChordCalls[index].time-expected.time)<1e-9);
        const bassCallsMatch = expectedBassCalls.length === actualBassPhraseCalls.length && expectedBassCalls.every((expected,index)=>{const actual=actualBassPhraseCalls[index];return expected.rootMidi===actual.rootMidi&&Math.abs(expected.time-actual.time)<1e-9&&Math.abs(expected.duration-actual.duration)<1e-9&&expected.slideMidi===actual.slideMidi;});
        const drumCountsMatch = Object.fromEntries(Object.keys(expectedDrums).map(key=>[key,expectedDrums[key].length===actualDrums[key].length]));
        const firstFsharp = cueId === "G" ? actualLead.filter(call=>call.midi===66).map(call=>({midi:call.midi,timeFromMusicalStartSeconds:call.time-0.04})) : [];
        const sourceProof = { schedulerCalls: { chord:schedulePlanStep.toString().includes("playChord("), bassPhrase:schedulePlanStep.toString().includes("playBassPhrase("), leadPhrase:schedulePlanStep.toString().includes("playLeadPhraseInstrument(") }, chordVoiceUsesAudioContext:playChordTone.toString().includes("audioCtx.createOscillator("), leadVoiceUsesAudioContext:playTone.toString().includes("audioCtx.createOscillator()"), coreBypassed:true };

        function makeWav(start,length) {
          const byteLength=44+length*channels.length*2, bytes=new Uint8Array(byteLength), view=new DataView(bytes.buffer);
          const ascii=(offset,text)=>{for(let i=0;i<text.length;i++)bytes[offset+i]=text.charCodeAt(i);};
          ascii(0,"RIFF");view.setUint32(4,byteLength-8,true);ascii(8,"WAVEfmt ");view.setUint32(16,16,true);view.setUint16(20,1,true);view.setUint16(22,channels.length,true);view.setUint32(24,buffer.sampleRate,true);view.setUint32(28,buffer.sampleRate*channels.length*2,true);view.setUint16(32,channels.length*2,true);view.setUint16(34,16,true);ascii(36,"data");view.setUint32(40,byteLength-44,true);let offset=44;
          for(let frame=start;frame<start+length;frame++)for(const channel of channels){view.setInt16(offset,Math.round(Math.max(-1,Math.min(1,channel[frame]))*32767),true);offset+=2;}
          let binary="";for(let i=0;i<bytes.length;i+=0x8000)binary+=String.fromCharCode(...bytes.subarray(i,Math.min(i+0x8000,bytes.length)));return btoa(binary);
        }
        return {
          cue:cueId, errors,
          imported:{bpm:state.bpm,timeSig:state.timeSig,bars:section.bars,resolution:state.resolution,stepsPerBar:stepsPerBar(),stepCount:plan.length,expectedDurationSeconds:expectedDuration,bodyFrames,fx:{delay:state.fxDelay,chorus:state.fxChorus,flanger:state.fxFlanger,reverb:state.fxReverb,mix:state.fxMix},gains:{master:els.masterVol?.value,chord:els.chordVol?.value,beat:els.beatVol?.value,lead:els.leadVol?.value}},
          sourceMask:{progressionDegrees:project[`progression${cueId}`],bassTriggerSteps:project[`grid${cueId}`]?.bass?.flatMap((value,step)=>value>0?[step]:[])||[],drumTriggerCounts:Object.fromEntries(["kick","snare","hat"].map(track=>[track,(project[`grid${cueId}`]?.[track]||[]).filter(value=>value>0).length])),rawMelodyStarts:rawStarts,rawHoldIndices:rawHolds,importedMelodyStarts:importedStarts,importedHoldIndices:importedHolds,melodyStartsMatch:JSON.stringify(rawStarts)===JSON.stringify(importedStarts),holdsMatch:JSON.stringify(rawHolds)===JSON.stringify(importedHolds),repeatedPitchStarts},
          calls:{expectedLeadPhraseCalls,actualLeadPhraseCalls,leadCallsMatch,expectedChordCount:expectedChordCalls.length,actualChordCalls:actualChordCalls.map(call=>({time:call.time,duration:call.duration,root:call.chord?.root,quality:call.chord?.quality})),chordCallsMatch,expectedBassCalls,actualBassPhraseCalls,bassCallsMatch,expectedDrumCounts:Object.fromEntries(Object.keys(expectedDrums).map(key=>[key,expectedDrums[key].length])),actualDrumCounts:Object.fromEntries(Object.keys(actualDrums).map(key=>[key,actualDrums[key].length])),drumCountsMatch},
          eventTrace:{melodyStartsWithMidi,holdRuns:repeatRuns(section.melodyHold||[]),repeatedPitchStarts},sourceProof,
          schedule:{steps,uiTimerCalls,sourceStarts:window.__liveSourceStarts,plannedDurationSeconds:time-0.04,firstFsharp4: firstFsharp},
          audio:{sampleRate:buffer.sampleRate,channels:buffer.numberOfChannels,framesPerChannel:buffer.length,leaderFrames,bodyStart,bodyFrames,bodyEnd,tailStart,tailFrames:buffer.length-tailStart,exactBody:bodyEnd-tailStart===0||bodyEnd===tailStart,fxNodes:{reverb:!!reverbConvolver,delay:!!delayNode,chorus:!!chorusDelay,flanger:!!flangerDelay,limiter:!!masterLimiter},metrics:{...metrics,seamMaxAbsDifference:seam}},
          loopWav:makeWav(bodyStart,bodyFrames),tailWav:makeWav(tailStart,buffer.length-tailStart)
        };
      }, { project: score, cueId: cue.id, expectedBodyFrames: cue.frames, leaderFrames });
      const loopName = `${cue.id}-loop.wav`, tailName = `${cue.id}-tail.wav`;
      fs.writeFileSync(path.join(output, loopName), Buffer.from(rendered.loopWav, "base64"));
      fs.writeFileSync(path.join(output, tailName), Buffer.from(rendered.tailWav, "base64"));
      delete rendered.loopWav; delete rendered.tailWav;
      renders.push({ cue: cue.id, pageErrors, render: rendered, outputs: [loopName, tailName].map(name => ({ path: name, bytes: fs.statSync(path.join(output,name)).size, sha256: crypto.createHash("sha256").update(fs.readFileSync(path.join(output,name))).digest("hex") })) });
      await page.close();
      console.log(JSON.stringify({ cue:cue.id, errors:rendered.errors, duration:rendered.imported.expectedDurationSeconds, steps:rendered.schedule.steps.length, sourceStarts:rendered.schedule.sourceStarts, body:rendered.audio.metrics, scoreParity:[rendered.sourceMask.melodyStartsMatch,rendered.sourceMask.holdsMatch], calls:[rendered.calls.leadCallsMatch,rendered.calls.chordCallsMatch,rendered.calls.bassCallsMatch,rendered.calls.drumCountsMatch], firstFsharp4:rendered.schedule.firstFsharp4, outputs:renders.at(-1).outputs }, null, 2));
    }
    const provenanceAfter = provenance();
    assertProvenance("post-render", provenanceAfter);
    if (JSON.stringify(provenanceBefore) !== JSON.stringify(provenanceAfter)) throw new Error("Source/archive/score/adapter provenance changed during rendering.");
    const receipt = {
      adapter: "Investigation-only local adapter; each cue uses the current Pocket Chordsmith HTML app voices and complete live FX graph via a fresh OfflineAudioContext. Manually invokes the existing section playback plan; no source changes, Core renderer, built-in WAV exporter, normalization, score edits, or generation.",
      source: { html:htmlPath,scoreEntry:"What_the_Dark_Keeps_Pocket_Chordsmith.json",expectedHashes,provenanceBefore,provenanceAfter,provenanceStable:true,priorReceiptHtmlHash:"b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302457fa9b6e2cf",priorReceiptHtmlHashStatus:"mismatch: missing 602 segment; original receipt preserved unchanged" },
      runtime:{browserVersion:browser.version(),sampleRate:rate,channels:2,deterministicMathRandomResetPerCue:true,virtualCurrentTimeForLiveVoicePruning:true,leaderFrames,tailsSeparate:true,manualScheduler:true,unsupportedAppOfflineExportAPI:true},
      requestedFrames:Object.fromEntries(cueSpecs.map(cue=>[cue.id,cue.frames])),renders,
      acceptance:{listening:"required",ownerAcceptance:"required",exactBitRepeatability:"not-proven; prior one-cue repeats differed by 1 PCM16 LSB in a small number of loop samples despite identical trace and exact tail",audioQuality:"investigation-only; no fade or release admission",sourceToolDistribution:"unresolved; source pack owner-authorized for Horde use only"}
    };
    fs.writeFileSync(path.join(output,"all-cues.json"),`${JSON.stringify(receipt,null,2)}\n`);
    console.log(`ALL_CUES_RECEIPT ${path.join(output,"all-cues.json")}`);
  } finally { await browser.close(); await new Promise(resolve=>server.close(resolve)); }
})().catch(error=>{console.error(error.stack||error);process.exitCode=1;server.close();});
