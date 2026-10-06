#!/usr/bin/env node
/**
 * Pocket Chordsmith reference-audio harness. No instrument or synthesizer code.
 * Imports the unmodified, pinned Pocket Audio Core source tree.
 */
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { spawnSync } from 'node:child_process';

const here = path.dirname(fileURLToPath(import.meta.url));
export const CORE_COMMIT = '0c6975cadcd4aba0047ab94170a2adabc722517f';
export const DEFAULT_CORE_ROOT = process.env.POCKET_CHORDSMITH_ROOT || null;
const moduleCache = new Map();

export async function loadCore(coreRoot = DEFAULT_CORE_ROOT) {
  if (!coreRoot) throw new Error('Set POCKET_CHORDSMITH_ROOT or pass --core-root to a separately authorized pinned engine checkout.');
  coreRoot = path.resolve(coreRoot);
  if (!moduleCache.has(coreRoot)) {
    const base = path.join(coreRoot, 'packages/pocket-audio-core/src');
    const [normalizer, parser, timeline, renderer, wav, metrics, worker] = await Promise.all([
      'schema/normalise-project.js', 'schema/parse-share-code.js', 'events/timeline-events.js',
      'engine/offline-renderer.js', 'export/wav.js', 'export/audio-metrics.js',
      'export/chordsmith-wav-worker.js'
    ].map(file => import(pathToFileURL(path.join(base, file)).href)));
    moduleCache.set(coreRoot, { ...normalizer, ...parser, ...timeline, ...renderer, ...wav, ...metrics, ...worker });
  }
  return moduleCache.get(coreRoot);
}

export function sha256(bytes) {
  return crypto.createHash('sha256').update(bytes).digest('hex');
}

/** Keep exact musical duration; circular addition carries true event tails. */
export function foldTailToLoop(rendered, musicalDuration) {
  const frames = Math.max(1, Math.round(musicalDuration * rendered.sampleRate));
  const channels = rendered.channels.map(source => {
    const target = new Float32Array(frames);
    for (let i = 0; i < source.length; i++) target[i % frames] += source[i];
    return target;
  });
  return { ...rendered, channels, duration: frames / rendered.sampleRate };
}

export function applyGain(rendered, gainDb = 0) {
  if (!Number.isFinite(gainDb)) throw new Error('gainDb must be finite.');
  if (gainDb === 0) return rendered;
  const scale = 10 ** (gainDb / 20);
  return { ...rendered, channels: rendered.channels.map(channel => Float32Array.from(channel, x => x * scale)) };
}

export function basicMetrics(rendered) {
  const { channels, sampleRate } = rendered;
  const frames = channels[0]?.length || 0;
  let peak = 0, squares = 0, count = 0, clippedSamples = 0, nonFiniteSamples = 0;
  let firstAudibleFrame = null, lastAudibleFrame = null, adjacentPeak = 0;
  const dcOffsets = [];
  const seamByChannel = [];
  for (const channel of channels) {
    let sum = 0;
    for (let i = 0; i < channel.length; i++) {
      const sample = channel[i];
      if (!Number.isFinite(sample)) { nonFiniteSamples++; continue; }
      const abs = Math.abs(sample);
      peak = Math.max(peak, abs); squares += sample * sample; count++; sum += sample;
      if (abs >= 1) clippedSamples++;
      if (abs >= 0.001) {
        firstAudibleFrame = firstAudibleFrame === null ? i : Math.min(firstAudibleFrame, i);
        lastAudibleFrame = lastAudibleFrame === null ? i : Math.max(lastAudibleFrame, i);
      }
      if (i) adjacentPeak = Math.max(adjacentPeak, Math.abs(sample - channel[i - 1]));
    }
    dcOffsets.push(sum / Math.max(1, channel.length));
    const n = channel.length;
    const jump = n ? channel[0] - channel[n - 1] : 0;
    const precedingSlope = n > 1 ? channel[n - 1] - channel[n - 2] : 0;
    const followingSlope = n > 1 ? channel[1] - channel[0] : 0;
    const w = Math.max(1, Math.min(n, Math.round(sampleRate * 0.01)));
    let headEnergy = 0, tailEnergy = 0, localAdjacentPeak = 0;
    for (let j = 0; j < w; j++) {
      headEnergy += channel[j] ** 2; tailEnergy += channel[n - 1 - j] ** 2;
      if (j > 0) localAdjacentPeak = Math.max(localAdjacentPeak,
        Math.abs(channel[j] - channel[j - 1]), Math.abs(channel[n - j] - channel[n - j - 1]));
    }
    seamByChannel.push({ jump, absoluteJump: Math.abs(jump), precedingSlope, followingSlope,
      slopeChangeBefore: Math.abs(jump - precedingSlope), slopeChangeAfter: Math.abs(followingSlope - jump),
      head10msRms: Math.sqrt(headEnergy / w), tail10msRms: Math.sqrt(tailEnergy / w),
      nearby10msMaxAdjacentDelta: localAdjacentPeak,
      jumpToNearbyDeltaRatio: localAdjacentPeak > 0 ? Math.abs(jump) / localAdjacentPeak : (jump === 0 ? 0 : null) });
  }
  const rms = Math.sqrt(squares / Math.max(1, count));
  const seamPeak = Math.max(0, ...seamByChannel.map(x => x.absoluteJump));
  return {
    sampleRate, frames, channels: channels.length, durationSeconds: frames / sampleRate,
    eventCount: rendered.eventCount, peak, peakDbfs: peak > 0 ? 20 * Math.log10(peak) : null,
    rms, rmsDbfs: rms > 0 ? 20 * Math.log10(rms) : null,
    clippedSamples, nonFiniteSamples, dcOffsets, maxAdjacentSampleDelta: adjacentPeak,
    firstAudibleFrame, lastAudibleFrame,
    firstAudibleSeconds: firstAudibleFrame === null ? null : firstAudibleFrame / sampleRate,
    trailingSilenceSeconds: lastAudibleFrame === null ? frames / sampleRate : (frames - 1 - lastAudibleFrame) / sampleRate,
    loopSeam: { peakAbsoluteJump: seamPeak, jumpDbfs: seamPeak > 0 ? 20 * Math.log10(seamPeak) : null,
      channels: seamByChannel,
      warning: seamByChannel.some(x => x.absoluteJump > 0.01 &&
        (x.jumpToNearbyDeltaRatio === null || x.jumpToNearbyDeltaRatio > 4)),
      interpretation: 'Diagnostic only. A nonzero seam is not automatically a click; compare nearby waveform slope and listen to repeated playback.' }
  };
}

/**
 * selection: { sectionId:'A' }, { sectionIds:['A','B','A','C'] }, or {} for songSequence.
 * Loop folding and an explicitly requested scalar gain are the only DSP here.
 */
export async function renderSelection(rawInput, {
  coreRoot = DEFAULT_CORE_ROOT, sampleRate = 44100, sectionId, sectionIds,
  loop = false, tailSeconds = 0.6, gainDb = 0, fullMetrics = false
} = {}) {
  if (!Number.isInteger(sampleRate) || sampleRate < 8000 || sampleRate > 192000) throw new Error('Invalid sample rate.');
  if (!Number.isFinite(tailSeconds) || tailSeconds < 0 || tailSeconds > 30) throw new Error('Invalid tail duration.');
  if (sectionId && sectionIds) throw new Error('Choose sectionId OR sectionIds.');
  const ids = sectionIds || (sectionId ? [sectionId] : null);
  if (ids && (!ids.length || ids.some(id => !/^[A-H]$/.test(id)))) throw new Error('Section IDs must be A through H.');
  const core = await loadCore(coreRoot);
  const raw = core.parsePocketChordsmithInput(rawInput);
  // Same normalization as the current Chordsmith WAV worker.
  const project = raw.app === 'PocketAudioProject' ? raw : core.normalisePocketChordsmithProject(raw);
  if (loop && project.lofi?.texture?.enabled &&
      (project.lofi.texture.tapeHiss > 0.005 || project.lofi.texture.vinylCrackle > 0.005)) {
    throw new Error('Loop tail folding with continuous Lofi texture needs a dedicated periodic-texture path. Use an audition render or disable the texture in an explicitly approved source variant.');
  }
  const timelineOptions = sectionIds ? { sectionIds } : sectionId ? { scope: 'section', sectionId } : { scope: 'sequence' };
  const timeline = core.buildPocketAudioTimeline(project, timelineOptions);
  const lastEventEnd = timeline.events.reduce((max, e) => Math.max(max, e.time + Math.max(0.02, e.duration || 0.08)), 0);
  const safeTail = Math.max(tailSeconds, lastEventEnd - timeline.duration + 2 / sampleRate, 0);
  if (safeTail > 120) throw new Error('An event tail exceeds the bounded reference-render budget.');
  const source = core.renderPocketAudioEventBuffer(timeline.events, {
    sampleRate, durationSeconds: timeline.duration, tailSeconds: safeTail,
    lofiTexture: project.lofi?.texture, timeline
  });
  const priorMetrics = basicMetrics(source);
  const processed = applyGain(loop ? foldTailToLoop(source, timeline.duration) : source, gainDb);
  const metrics = basicMetrics(processed);
  const rendererFile = path.join(coreRoot, 'packages/pocket-audio-core/src/engine/offline-renderer.js');
  const report = {
    classification: 'Pocket Audio Core deterministic reference audio; not accepted v68 mastered-tone parity',
    sourceRepository: 'Samfa12-tech/Pocket-Chordsmith', sourceCommit: CORE_COMMIT,
    rendererPath: 'packages/pocket-audio-core/src/engine/offline-renderer.js',
    rendererSha256: sha256(fs.readFileSync(rendererFile)),
    projectTitle: project.meta.title, sectionIds: timeline.sectionIds,
    bpm: project.meta.bpm, timeSig: project.meta.timeSig, resolution: project.meta.resolution,
    musicalDurationSeconds: timeline.duration, musicalDurationTicks: timeline.durationTicks,
    options: { sampleRate, loop, requestedTailSeconds: tailSeconds, renderedTailSeconds: safeTail, gainDb },
    processing: {
      onset: 'Preserve the timeline start and intentional leading silence; no onset trimming.',
      tail: loop ? 'All samples after the nearest sample-clock bar boundary are circularly added into the beginning; no inserted silence, fades, or crossfades.' : 'Keep the complete final event plus at least the requested audition tail.',
      loopDurationRoundingErrorSeconds: loop ? processed.duration - timeline.duration : null,
      synthesis: 'Unmodified Pocket Audio Core renderPocketAudioEventBuffer.'
    },
    capabilityReport: timeline.capabilityReport, eventLossReport: timeline.lossReport,
    limitations: [
      'Core render is not proven timbral parity with the historical accepted v68 live-voice/FX renderer.',
      'Core offline synthesis does not apply project master volume/FX, instrument filters, generic pitch slides, or all expressive intent; capability exact=true is not a listening/parity gate.',
      'TailSeconds allocates space; it does not add reverb or an instrument release absent from the Core recipe.',
      'MP3 is an audition convenience, not a sample-accurate looping runtime asset.',
      'Acoustic-sounding instrument labels denote engine synthesis approximations, not recorded instruments.'
    ],
    preProcessingMetrics: priorMetrics, metrics,
    ...(fullMetrics ? { coreMetrics: core.analyseRenderedBuffer(processed) } : {})
  };
  return { raw, project, timeline, source, rendered: processed, report, core };
}

export function writeRender(result, { outDir, name = 'reference', mp3 = false, bitDepth = 24 } = {}) {
  if (!outDir) throw new Error('outDir is required.');
  if (!/^[A-Za-z0-9_.-]+$/.test(name)) throw new Error('Use a simple output name.');
  if (![16, 24].includes(bitDepth)) throw new Error('bitDepth must be 16 or 24.');
  fs.mkdirSync(outDir, { recursive: true });
  const base = path.join(outDir, name);
  const bytes = result.core.encodePcmWavBytes({ ...result.rendered, bitDepth });
  fs.writeFileSync(`${base}.wav`, bytes);
  result.report.audio = { filename: `${name}.wav`, bitDepth, bytes: bytes.length, sha256: sha256(bytes) };
  if (mp3) {
    const encode = spawnSync('ffmpeg', ['-hide_banner', '-loglevel', 'error', '-y', '-i', `${base}.wav`,
      '-codec:a', 'libmp3lame', '-q:a', '2', '-metadata', 'comment=Pocket Audio Core reference audition; not a runtime loop', `${base}.mp3`], { encoding: 'utf8' });
    if (encode.status !== 0) throw new Error(`MP3 encode failed: ${encode.error || encode.stderr}`);
    result.report.mp3 = { filename: `${name}.mp3`, use: 'Audition only; encoder delay/padding is not loop-safe.', sha256: sha256(fs.readFileSync(`${base}.mp3`)) };
  }
  fs.writeFileSync(`${base}.events.json`, JSON.stringify(result.timeline, null, 2) + '\n');
  fs.writeFileSync(`${base}.metrics.json`, JSON.stringify(result.report, null, 2) + '\n');
  return { wav: `${base}.wav`, mp3: mp3 ? `${base}.mp3` : null, report: `${base}.metrics.json`, events: `${base}.events.json` };
}

async function main() {
  const args = process.argv.slice(2);
  const input = args.shift();
  if (!input || input === '--help') {
    console.log('Usage: node render-core-reference.mjs PROJECT.json --out DIR [--section A | --section-ids A,B,A,C | --sequence] [--name NAME] [--loop] [--mp3] [--sample-rate 44100] [--tail 0.6] [--gain-db 0] [--full-metrics] [--core-root PATH]');
    return;
  }
  const opts = {}, output = {};
  while (args.length) {
    const arg = args.shift();
    const value = () => { if (!args.length) throw new Error(`Missing value for ${arg}`); return args.shift(); };
    if (arg === '--out') output.outDir = path.resolve(value());
    else if (arg === '--name') output.name = value();
    else if (arg === '--section') opts.sectionId = value();
    else if (arg === '--section-ids') opts.sectionIds = value().split(',');
    else if (arg === '--sequence') { /* default */ }
    else if (arg === '--loop') opts.loop = true;
    else if (arg === '--mp3') output.mp3 = true;
    else if (arg === '--sample-rate') opts.sampleRate = Number(value());
    else if (arg === '--tail') opts.tailSeconds = Number(value());
    else if (arg === '--gain-db') opts.gainDb = Number(value());
    else if (arg === '--full-metrics') opts.fullMetrics = true;
    else if (arg === '--core-root') opts.coreRoot = path.resolve(value());
    else throw new Error(`Unknown argument ${arg}`);
  }
  output.name ||= `${path.basename(input, path.extname(input))}_${opts.sectionId || (opts.sectionIds ? opts.sectionIds.join('') : 'sequence')}${opts.loop ? '_loop' : '_audition'}`;
  const sourceBytes = fs.readFileSync(input);
  const result = await renderSelection(sourceBytes.toString('utf8'), opts);
  result.report.compositionSource = { filename: path.basename(input), sha256: sha256(sourceBytes) };
  const files = writeRender(result, output);
  console.log(JSON.stringify({ ...files, metrics: result.report.metrics }, null, 2));
  if (result.report.metrics.nonFiniteSamples || result.report.metrics.clippedSamples) process.exitCode = 2;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });
}
