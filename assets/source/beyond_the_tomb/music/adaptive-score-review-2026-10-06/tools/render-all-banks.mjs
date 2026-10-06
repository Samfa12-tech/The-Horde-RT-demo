#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn, spawnSync } from 'node:child_process';
import { renderSelection, writeRender, applyGain, basicMetrics, sha256 } from './render-core-reference.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const manifest = JSON.parse(fs.readFileSync(path.join(root, 'manifests/cue-manifest.json'), 'utf8'));
const targetPeak = 10 ** (-3 / 20);
const pilots = { 3: ['B', 'D', 'E', 'G'], 4: ['B', 'C', 'H'] };

function flacFromWav(wav) {
  const flac = wav.replace(/\.wav$/, '.flac');
  const args = ['-hide_banner', '-loglevel', 'error', '-y', '-i', wav, '-c:a', 'flac', '-compression_level', '8', flac];
  const job = spawnSync('ffmpeg', args, { encoding: 'utf8' });
  if (job.status !== 0) throw new Error(`FLAC encoding failed: ${job.error || job.stderr}`);
  const check = spawnSync('ffmpeg', ['-hide_banner', '-loglevel', 'error', '-i', flac,
    '-c:a', 'pcm_s16le', '-f', 'hash', '-hash', 'sha256', '-'], { encoding: 'utf8' });
  if (check.status !== 0) throw new Error(`FLAC decode verification failed: ${check.error || check.stderr}`);
  const sourcePcmSha256 = sha256(fs.readFileSync(wav).subarray(44));
  const decodedPcmSha256 = check.stdout.trim().split('=')[1]?.toLowerCase();
  if (sourcePcmSha256 !== decodedPcmSha256) throw new Error('FLAC decoded PCM is not byte-identical to WAV.');
  return { filename: path.basename(flac), bytes: fs.statSync(flac).size,
    sha256: sha256(fs.readFileSync(flac)), pcmSha256: sourcePcmSha256, pcmRoundtripVerified: true };
}

function finishGain(result, gainDb) {
  result.rendered = applyGain(result.rendered, gainDb);
  result.report.options.gainDb = gainDb;
  result.report.metrics = basicMetrics(result.rendered);
  result.report.gainPolicy = 'One common scalar across every section and full review sequence in this bank, based on section-bank maximum peak; no per-section normalization or limiting.';
  if (result.report.metrics.nonFiniteSamples || result.report.metrics.peak >= 1) {
    throw new Error('Refusing to encode clipping or non-finite audio.');
  }
}

async function renderBank(family) {
  const bank = `${family.isEventBank ? 'events-' : ''}${path.basename(family.score, '.json')}`;
  const sourceFile = path.join(root, family.isEventBank ? 'event-scores' : 'scores', family.score);
  const sourceBytes = fs.readFileSync(sourceFile);
  const sourceHash = sha256(sourceBytes);
  const raw = JSON.parse(sourceBytes);
  const sections = [];
  for (const [id, section] of Object.entries(family.sections)) {
    const loop = ['loop', 'bridge'].includes(section.behavior);
    if (!loop && section.behavior !== 'one_shot') throw new Error(`Unrecognized behavior ${section.behavior}`);
    const result = await renderSelection(raw, { sectionId: id, sampleRate: 44100, loop, tailSeconds: 0.6 });
    result.report.compositionSource = { filename: family.score, sha256: sourceHash };
    result.report.adaptiveSection = { id, ...section };
    sections.push({ id, section, result });
  }
  const sectionPeak = Math.max(...sections.map(x => x.result.report.metrics.peak));
  const full = await renderSelection(raw, { sectionIds: family.review_sequence, sampleRate: 44100, tailSeconds: 0.6 });
  full.report.compositionSource = { filename: family.score, sha256: sourceHash };
  full.report.reviewSequencePurpose = 'Linear listening tour only; in-game adaptive transitions are not implemented by this recording.';
  let gain = sectionPeak > 0 ? targetPeak / sectionPeak : 1;
  const guardAdjusted = full.report.metrics.peak * gain >= 0.999;
  if (guardAdjusted) gain = Math.min(gain, targetPeak / full.report.metrics.peak);
  const gainDb = 20 * Math.log10(gain);
  const bankReport = {
    bank, title: family.title, sourceScore: family.score, sourceSha256: sourceHash,
    referenceAudio: true, sourceEngineCommit: manifest.engine_revision,
    commonGainDb: gainDb, commonGainLinear: gain, targetSectionBankMaxPeakDbfs: -3,
    preGainSectionBankMaxPeak: sectionPeak, fullSequenceClipGuardAdjustedGain: guardAdjusted,
    sampleRate: 44100, bitDepth: 16, sections: [], fullReview: null,
    limitations: ['Core reference synthesis; no v68 mastered tone parity or production approval.',
      'Per-bank scalar preserves state-relative levels but is not a cross-bank loudness master.',
      'Loop/bridge files are exact bar length with event-tail wrap. One-shots carry at least0.6s tail allocation.',
      'MP3 is for audition; lossless WAV/FLAC provides authoritative sample frames.']
  };
  const outDir = path.join(root, 'section-audio', bank);
  for (const { id, section, result } of sections) {
    finishGain(result, gainDb);
    const name = `${id}-${section.name}`;
    const files = writeRender(result, { outDir, name, bitDepth: 16, mp3: pilots[family.number]?.includes(id) || false });
    const flac = flacFromWav(files.wav);
    result.report.flac = flac;
    fs.writeFileSync(files.report, JSON.stringify(result.report, null, 2) + '\n');
    bankReport.sections.push({ id, name: section.name, behavior: section.behavior,
      wav: path.basename(files.wav), flac, mp3: files.mp3 ? path.basename(files.mp3) : null,
      metrics: result.report.metrics, report: path.basename(files.report), events: path.basename(files.events) });
    // Release the original pre-tail-wrap buffer after encoding this section.
    result.source = null; result.rendered = null;
  }
  finishGain(full, gainDb);
  const fullFiles = writeRender(full, { outDir: path.join(root, 'previews/full'), name: bank, mp3: true, bitDepth: 16 });
  bankReport.fullReview = { ...fullFiles, metrics: full.report.metrics, sectionIds: family.review_sequence };
  bankReport.achievedSectionBankMaxPeakDbfs = Math.max(...bankReport.sections.map(x => x.metrics.peakDbfs ?? -Infinity));
  bankReport.loopSeamWarnings = bankReport.sections.filter(x => x.behavior !== 'one_shot' && x.metrics.loopSeam.warning).map(x => x.id);
  if (sha256(fs.readFileSync(sourceFile)) !== sourceHash) throw new Error('Canonical score changed during render; outputs need regeneration.');
  fs.writeFileSync(path.join(outDir, 'bank-audio-manifest.json'), JSON.stringify(bankReport, null, 2) + '\n');
  console.log(JSON.stringify({ bank, status: 'done', sectionCount: sections.length,
    commonGainDb: gainDb, sectionBankPeakDbfs: bankReport.achievedSectionBankMaxPeakDbfs,
    fullPeakDbfs: full.report.metrics.peakDbfs, loopSeamWarnings: bankReport.loopSeamWarnings,
    pilotMp3: bankReport.sections.filter(x => x.mp3).map(x => path.join(outDir, x.mp3)) }));
}

const requestedEvent = process.argv.indexOf('--event-bank');
const requested = process.argv.indexOf('--bank');
if (requestedEvent >= 0) {
  const score = process.argv[requestedEvent + 1];
  const raw = JSON.parse(fs.readFileSync(path.join(root, 'event-scores', score), 'utf8'));
  const items = Object.values(manifest).find(value => Array.isArray(value) && value.some(x => x?.bank === score)) || [];
  const eventItems = items.filter(x => x.bank === score);
  if (eventItems.length !== 8) throw new Error(`Expected eight event entries for ${score}`);
  await renderBank({ isEventBank: true, number: score, score, title: raw.title,
    sections: Object.fromEntries(eventItems.map(x => [x.section, { bars: x.bars, name: x.id, behavior: 'one_shot' }])),
    review_sequence: eventItems.map(x => x.section) });
} else if (requested >= 0) {
  const number = Number(process.argv[requested + 1]);
  const family = manifest.families.find(x => x.number === number);
  if (!family) throw new Error(`Unknown bank ${number}`);
  await renderBank(family);
} else {
  const eventMode = process.argv.includes('--events-only');
  const requestedNumbers = eventMode ? fs.readdirSync(path.join(root, 'event-scores')).filter(x => x.endsWith('.json') && !x.endsWith('.raw.json')) :
    process.argv.includes('--pilots-only') ? [4, 3] : [4, 3, ...manifest.families.map(x => x.number).filter(x => ![3, 4].includes(x))];
  let cursor = 0;
  async function runner() {
    while (cursor < requestedNumbers.length) {
      const number = requestedNumbers[cursor++];
      await new Promise((resolve, reject) => {
        const child = spawn(process.execPath, [fileURLToPath(import.meta.url), eventMode ? '--event-bank' : '--bank', String(number)], { stdio: 'inherit' });
        child.on('error', reject);
        child.on('exit', code => code === 0 ? resolve() : reject(new Error(`Bank ${number} exited ${code}`)));
      });
    }
  }
  await Promise.all([runner(), runner()]);
}
