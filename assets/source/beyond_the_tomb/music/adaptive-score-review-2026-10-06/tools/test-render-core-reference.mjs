import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadCore, renderSelection, foldTailToLoop, basicMetrics, writeRender, sha256 } from './render-core-reference.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const outDir = path.resolve(here, '../validation/renderer-smoke');
const zeros = () => Array(64).fill(0);
const melody = Array(64).fill(null);
melody[0] = 2; melody[4] = 3; melody[8] = 9; melody[12] = 4;
const raw = {
  projectVersion: 17, title: 'Renderer infrastructure smoke fixture', key: 'D', scale: 'minor',
  bpm: 80, timeSig: 4, resolution: 4, swing: 0, humanizeOn: false,
  melodyPitchMode: 'chromatic', chordInstrument: 'harp', chordType: 'sus2',
  chordsOn: true, bassOn: false, guitarEnabled: false, chordPlayMode: 'block',
  chordRhythmMode: 'sustain', sectionBars: { A: 1, B: 1, C: 1, D: 1, E: 1, F: 1, G: 1, H: 1 },
  songSequence: ['A'], progressionA: [0, 0, 0, 0],
  gridA: { kick: zeros(), snare: zeros(), hat: zeros(), bass: zeros() },
  melodyTracksA: [melody], melodyInstrumentsA: ['soft'], melodyOctavesA: [-1], melodyPanA: [-0.2]
};

const core = await loadCore();
const result = await renderSelection(raw, { sectionId: 'A', sampleRate: 44100, fullMetrics: true });
assert.equal(result.timeline.events.length, 5);
assert.equal(result.timeline.duration, 3);
assert.deepEqual(result.timeline.events.filter(e => e.type === 'melody').map(e => e.midi), [62, 63, 69, 64]);
assert.equal(result.report.metrics.clippedSamples, 0);
assert.equal(result.report.metrics.nonFiniteSamples, 0);
assert.ok(result.report.metrics.peak > 0.01);

const directBytes = core.renderPocketAudioWavBytes(core.normalisePocketChordsmithProject(raw), { scope: 'section', sectionId: 'A', sampleRate: 44100, tailSeconds: 0.6 });
const worker = await core.renderChordsmithWavWorkerJob({ project: raw, options: { scope: 'section', sectionId: 'A', sampleRate: 44100, tailSeconds: 0.6 } });
const harnessBytes = core.encodePcm16WavBytes(result.rendered);
assert.equal(sha256(directBytes), sha256(new Uint8Array(worker.bytes)));
assert.equal(sha256(directBytes), sha256(harnessBytes));
const repeat = await renderSelection(raw, { sectionId: 'A', sampleRate: 44100 });
assert.equal(sha256(harnessBytes), sha256(core.encodePcm16WavBytes(repeat.rendered)));

const loop = await renderSelection(raw, { sectionId: 'A', loop: true, sampleRate: 44100 });
assert.equal(loop.rendered.channels[0].length, 132300);
assert.equal(loop.rendered.duration, 3);
assert.equal(loop.report.metrics.loopSeam.warning, false);
const folded = foldTailToLoop({ channels: [Float32Array.of(1, 2, 3, 4, 5, 6)], sampleRate: 2 }, 2);
assert.deepEqual([...folded.channels[0]], [6, 8, 3, 4]);
assert.equal(folded.duration, 2);
assert.equal(basicMetrics({ channels: [Float32Array.of(0, NaN, Infinity, -1, 1)], sampleRate: 1 }).nonFiniteSamples, 2);

const fxRaw = { ...raw, masterVolume: 0, fxReverb: true, fxDelay: true, fxChorus: true, fxMix: 1 };
const fxResult = await renderSelection(fxRaw, { sectionId: 'A', sampleRate: 44100 });
assert.equal(sha256(core.encodePcm16WavBytes(fxResult.rendered)), sha256(harnessBytes), 'Documented Core master-volume/FX omission changed; revisit limitation.');

fs.mkdirSync(outDir, { recursive: true });
const files = writeRender(result, { outDir, name: 'core-reference-smoke', mp3: true });
const loopFiles = writeRender(loop, { outDir, name: 'core-reference-loop-smoke' });
const checks = {
  passed: true,
  tests: [
    'Exact same PCM16 WAV bytes as current Chordsmith WAV worker at 44.1 kHz',
    'Exact same bytes as direct Core renderPocketAudioWavBytes',
    'Deterministic repeated render', 'Chromatic MIDI 62,63,69,64 preserved',
    'One-bar 80 BPM 4/4 timeline is exactly 3 seconds', 'Exact 132300-frame loop at 44.1 kHz',
    'Circular tail addition arithmetic', 'No clipping or non-finite real-render samples',
    'Non-finite sample metric detection', 'MP3 encoded through installed ffmpeg',
    'Core master-volume and FX omission reproduced and documented'
  ],
  workerPcm16Sha256: sha256(directBytes), files, loopFiles,
  note: 'Infrastructure smoke, not an artistic listening approval or accepted v68 live/FX parity proof.'
};
fs.writeFileSync(path.join(outDir, 'test-results.json'), JSON.stringify(checks, null, 2) + '\n');
console.log(JSON.stringify(checks, null, 2));
