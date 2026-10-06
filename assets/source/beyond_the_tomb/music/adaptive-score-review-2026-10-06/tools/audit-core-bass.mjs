#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';
import { loadCore, applyGain, basicMetrics, sha256 } from './render-core-reference.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const output = path.join(root, 'validation/bass-audit');
fs.mkdirSync(output, { recursive: true });
const core = await loadCore();
const selections = [['01-what-the-dark-keeps', 'A'], ['02-tomb', 'B'], ['03-fourth-keeper', 'B'], ['06-bellwether', 'B']];
const manifest = { purpose: 'Bass-only exact-event diagnostic; no canonical score or previous render changes.',
  engineCommit: '0c6975cadcd4aba0047ab94170a2adabc722517f',
  gainPolicy: 'Same previously published common gain per bank. No per-stem or per-excerpt normalization.',
  comparisons: [] };
function encode(buffer, dest, bitDepth = 16) {
  const bytes = core.encodePcmWavBytes({ ...buffer, bitDepth });
  fs.writeFileSync(dest, bytes);
  return sha256(bytes);
}
function ffmpeg(args) {
  const result = spawnSync('ffmpeg', ['-hide_banner', '-loglevel', 'error', '-y', ...args], { encoding: 'utf8' });
  if (result.status !== 0) throw new Error(result.error || result.stderr);
}
for (const [bank, sectionId] of selections) {
  const score = path.join(root, 'scores', `${bank}.json`);
  const sourceBytes = fs.readFileSync(score);
  const raw = JSON.parse(sourceBytes);
  const project = core.normalisePocketChordsmithProject(raw);
  const timeline = core.buildPocketAudioTimeline(project, { scope: 'section', sectionId });
  const events = timeline.events.filter(event => event.stem === 'bass');
  const gains = JSON.parse(fs.readFileSync(path.join(root, 'section-audio', bank, 'bank-audio-manifest.json')));
  const gainDb = gains.commonGainDb;
  const stem = core.renderPocketAudioEventBuffer(events, { sampleRate: 44100, durationSeconds: timeline.duration, tailSeconds: 0.6, lofiTexture: null });
  const scaled = applyGain(stem, gainDb);
  const name = `${bank}-${sectionId}`;
  const wav = path.join(output, `${name}.core-bass.wav`);
  const hash = encode(scaled, wav);
  ffmpeg(['-i', wav, '-c:a', 'libmp3lame', '-q:a', '2', wav.replace('.wav', '.mp3')]);
  const bassAtVolume = (beatVolume) => {
    const p = core.normalisePocketChordsmithProject({ ...raw, beatVolume });
    const e = core.buildPocketAudioTimeline(p, { scope: 'section', sectionId }).events.filter(e => e.stem === 'bass');
    return sha256(core.encodePcm16WavBytes(core.renderPocketAudioEventBuffer(e, { sampleRate: 44100, durationSeconds: timeline.duration, tailSeconds: 0.6 })));
  };
  const first = { ...events[0], time: 0 };
  const firstNoteDuration = Math.max(0.5, first.duration + 0.35);
  const firstNoteFiles = {};
  for (const rate of [44100, 176400]) {
    const result = core.renderPocketAudioEventBuffer([first], { sampleRate: rate, durationSeconds: firstNoteDuration, tailSeconds: 0 });
    const file = path.join(output, `${name}.first-note-${rate}.wav`);
    firstNoteFiles[rate] = { filename: path.basename(file), sha256: encode(applyGain(result, gainDb), file, 24) };
  }
  const oversampled = path.join(output, `${name}.first-note-176400-to-44100.wav`);
  ffmpeg(['-i', path.join(output, firstNoteFiles[176400].filename), '-af', 'aresample=44100:filter_size=128:cutoff=0.95', '-c:a', 'pcm_s24le', oversampled]);
  const comparison = {
    name, bank, sectionId, sourceScore: `scores/${bank}.json`, sourceSha256: sha256(sourceBytes),
    bpm: project.meta.bpm, sectionDurationSeconds: timeline.duration, sampleRate: 44100, tailSeconds: 0.6,
    fixedBankGainDb: gainDb, fixedBankGainLinear: 10 ** (gainDb / 20),
    sourceBeatVolume: project.mixer.stems.bass.volume, sourceMasterVolume: project.mixer.masterVolume,
    bassTone: events[0]?.bassTone, events, eventTraceSha256: sha256(JSON.stringify(events)),
    metrics: basicMetrics(scaled), coreWav: path.basename(wav), coreWavSha256: hash,
    coreMp3: path.basename(wav.replace('.wav', '.mp3')),
    firstNoteFiles, firstNoteOversampledDownsampledFile: path.basename(oversampled),
    mixSensitivity: { lowBeatVolume: 0.01, highBeatVolume: 1, lowHash: bassAtVolume(0.01), highHash: bassAtVolume(1) },
    analysisCaveat: 'Oversampled same-engine diagnostic measures sample-rate dependence including aliasing, waveform phase/grid quantization and resampling; it is not an isolated alias-distortion percentage or proposed replacement synth.'
  };
  comparison.mixSensitivity.identicalPcmDespite100xBeatVolume = comparison.mixSensitivity.lowHash === comparison.mixSensitivity.highHash;
  fs.writeFileSync(path.join(output, `${name}.events.json`), JSON.stringify(comparison, null, 2) + '\n');
  manifest.comparisons.push(comparison);
  if (sha256(fs.readFileSync(score)) !== comparison.sourceSha256) throw new Error('Source changed during audit.');
}
fs.writeFileSync(path.join(output, 'comparison-inputs.json'), JSON.stringify(manifest, null, 2) + '\n');
console.log(JSON.stringify(manifest.comparisons.map(x => ({ name: x.name, events: x.events.length,
  midi: x.events.map(e => e.midi), duration: x.sectionDurationSeconds, gainDb: x.fixedBankGainDb,
  peakDbfs: x.metrics.peakDbfs, beatVolumeIgnored: x.mixSensitivity.identicalPcmDespite100xBeatVolume })), null, 2));
