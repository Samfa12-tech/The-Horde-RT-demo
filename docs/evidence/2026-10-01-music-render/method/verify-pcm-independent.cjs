const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");

const root = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render-corrected";
const output = path.join(root, "pcm-verification.json");
if (fs.existsSync(output)) throw new Error("Refusing to overwrite prior PCM verification receipt.");
const receipt = JSON.parse(fs.readFileSync(path.join(root, "all-cues.json"), "utf8"));
const rate = 48000, channels = 2, bits = 16, blockAlign = channels * bits / 8;
const framesByCue = { A:576000, B:576000, C:144000, D:576000, E:576000, F:576000, G:288000, H:576000 };
const sha256 = bytes => crypto.createHash("sha256").update(bytes).digest("hex");
const close = (a, b, tolerance) => Math.abs(a - b) <= tolerance;
const qTolerance = 2 / 32767;

function fail(message) { throw new Error(message); }
function auditWav(cue, kind, row, frames) {
  const name = `${cue}-${kind}.wav`, file = path.join(root, name), bytes = fs.readFileSync(file);
  if (bytes.length !== 44 + frames * blockAlign) fail(`${name}: unexpected total byte length`);
  if (bytes.toString("ascii", 0, 4) !== "RIFF" || bytes.toString("ascii", 8, 12) !== "WAVE" || bytes.toString("ascii", 12, 16) !== "fmt ") fail(`${name}: bad RIFF/WAVE header`);
  if (bytes.readUInt32LE(4) !== bytes.length - 8 || bytes.readUInt32LE(16) !== 16) fail(`${name}: RIFF/fmt chunk length mismatch`);
  if (bytes.readUInt16LE(20) !== 1 || bytes.readUInt16LE(22) !== channels || bytes.readUInt32LE(24) !== rate || bytes.readUInt32LE(28) !== rate * blockAlign || bytes.readUInt16LE(32) !== blockAlign || bytes.readUInt16LE(34) !== bits) fail(`${name}: PCM format mismatch`);
  if (bytes.toString("ascii", 36, 40) !== "data" || bytes.readUInt32LE(40) !== frames * blockAlign) fail(`${name}: data chunk length mismatch`);

  let peak = 0, squares = 0, saturated = 0;
  const perChannel = [new Float64Array(frames), new Float64Array(frames)];
  for (let frame = 0; frame < frames; frame++) {
    for (let channel = 0; channel < channels; channel++) {
      const pcm = bytes.readInt16LE(44 + (frame * channels + channel) * 2);
      const sample = pcm / 32767;
      perChannel[channel][frame] = sample;
      peak = Math.max(peak, Math.abs(sample));
      squares += sample * sample;
      if (pcm === 32767 || pcm === -32768) saturated++;
    }
  }
  const rms = Math.sqrt(squares / (frames * channels));
  const seam = kind === "loop" ? Math.max(...perChannel.map(samples => Math.abs(samples[frames - 1] - samples[0]))) : null;
  const expectedPeak = kind === "loop" ? row.render.audio.metrics.bodyPeak : row.render.audio.metrics.tailPeak;
  const expectedRms = kind === "loop" ? row.render.audio.metrics.bodyRms : row.render.audio.metrics.tailRms;
  if (!close(peak, expectedPeak, qTolerance)) fail(`${name}: decoded PCM peak outside quantization tolerance`);
  if (!close(rms, expectedRms, qTolerance)) fail(`${name}: decoded PCM RMS outside quantization tolerance`);
  if (kind === "loop" && !close(seam, row.render.audio.metrics.seamMaxAbsDifference, qTolerance)) fail(`${name}: decoded PCM seam outside quantization tolerance`);
  const expectedOutput = row.outputs.find(output => output.path === name);
  if (!expectedOutput || sha256(bytes) !== expectedOutput.sha256) fail(`${name}: SHA-256 does not match render receipt`);
  return { name, frames, bytes: bytes.length, sha256: sha256(bytes), pcmPeak: peak, sourceFloatPeak: expectedPeak, peakAbsDelta: Math.abs(peak - expectedPeak), pcmRms: rms, sourceFloatRms: expectedRms, rmsAbsDelta: Math.abs(rms - expectedRms), pcmSaturatedSamples: saturated, pcmSeamMaxAbsDifference: seam, sourceFloatSeamMaxAbsDifference: kind === "loop" ? row.render.audio.metrics.seamMaxAbsDifference : null };
}

if (receipt.renders.length !== 8 || receipt.renders.map(row => row.cue).join("") !== "ABCDEFGH") fail("Cue set/order differs from the eight-cue render receipt.");
const cues = [];
const lfoPeriods = [];
for (const row of receipt.renders) {
  const frames = framesByCue[row.cue];
  if (!frames || row.render.audio.sampleRate !== rate || row.render.audio.channels !== channels || row.render.audio.bodyFrames !== frames || row.render.audio.tailFrames !== 144000) fail(`${row.cue}: receipt frame/audio metadata mismatch`);
  if (row.render.errors.length || row.pageErrors.length) fail(`${row.cue}: renderer/page errors present`);
  if (row.render.audio.metrics.nonFiniteSamples !== 0 || row.render.audio.metrics.clippedSamplesAtPcm16Ceiling !== 0) fail(`${row.cue}: recorded float render has nonfinite/clipped samples`);
  const chorusAmount = Number(row.render.imported.fx.chorus);
  const chorusFrequencyHz = 0.25 + chorusAmount * 1.9;
  lfoPeriods.push({ cue: row.cue, bodySeconds: row.render.imported.expectedDurationSeconds, chorusAmount, chorusFrequencyHz, chorusCyclesInBody: chorusFrequencyHz * row.render.imported.expectedDurationSeconds });
  const loop = auditWav(row.cue, "loop", row, frames);
  const tail = auditWav(row.cue, "tail", row, 144000);
  cues.push({ cue: row.cue, loop, tail });
}
const report = {
  status: "PASS",
  scope: "Independent PCM16 payload/header/hash audit against float metrics in the frozen render receipt; no rendering or audio transformation.",
  earlierAttempt: { status: "did_not_complete", cause: "Verifier initially compared the four-byte WAVE tag to the longer WAVEfmt literal; the WAVs were not changed. The check was corrected to validate WAVE, fmt, and data tags independently before this passing run." },
  quantizationTolerance: qTolerance,
  sourceFloatMetricsLimitation: "Nonfinite/clipping counts are from the recorded pre-PCM OfflineAudioContext buffer; cropped PCM WAV cannot independently recover NaN values that the encoder may coerce to zero or leader samples outside the crops.",
  cues,
  chorusLoopFinding: { source: "Pocket Chordsmith current HTML updateFx at line 4267: chorus LFO frequency = 0.25 + chorusAmount * 1.9 Hz.", perCue: lfoPeriods, implication: "Each body ends at a nonintegral chorus-LFO phase; exact bar-duration cropping alone does not establish seamless looping." }
};
fs.writeFileSync(output, `${JSON.stringify(report, null, 2)}\n`);
console.log(JSON.stringify(report));
