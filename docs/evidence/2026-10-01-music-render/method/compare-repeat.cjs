const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");
const oldRoot = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render";
const newRoot = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render-corrected";
const sha256 = bytes => crypto.createHash("sha256").update(bytes).digest("hex");
const cues = [];
for (const cue of "ABCDEFGH") {
  const files = [];
  for (const kind of ["loop", "tail"]) {
    const name = `${cue}-${kind}.wav`;
    const oldBytes = fs.readFileSync(path.join(oldRoot, name));
    const newBytes = fs.readFileSync(path.join(newRoot, name));
    if (oldBytes.length !== newBytes.length || !oldBytes.subarray(0, 44).equals(newBytes.subarray(0, 44))) throw new Error(`WAV header/length mismatch: ${name}`);
    let changedSamples = 0, maxAbsDelta = 0, overOneLsb = 0;
    const firstDifferences = [];
    for (let byte = 44, sample = 0; byte < oldBytes.length; byte += 2, sample++) {
      const a = oldBytes.readInt16LE(byte), b = newBytes.readInt16LE(byte), delta = Math.abs(a - b);
      if (delta) {
        changedSamples++;
        maxAbsDelta = Math.max(maxAbsDelta, delta);
        if (delta > 1) overOneLsb++;
        if (firstDifferences.length < 5) firstDifferences.push({ sample, old: a, fresh: b, delta });
      }
    }
    files.push({ name, sampleCount: (oldBytes.length - 44) / 2, changedSamples, maxAbsDelta, overOneLsb, firstDifferences, oldSha256: sha256(oldBytes), freshSha256: sha256(newBytes) });
  }
  cues.push({ cue, files });
}
const report = { comparison: "Preserved original investigation run versus fresh provenance-corrected run; same score/app/scheduler, no audio changes", cues };
fs.writeFileSync(path.join(newRoot, "repeat-comparison.json"), `${JSON.stringify(report, null, 2)}\n`);
console.log(JSON.stringify(report));
