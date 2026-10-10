"""Derive bounded contact cues from the already admitted waterfall recording.

No new recording, synthesis, or normalisation. This preserves the original loop.
Run from the repository root; resulting cues still need owner listening review.
"""
import array
import hashlib
import json
from pathlib import Path
import wave

ROOT = Path(__file__).resolve().parents[1]
AUDIO = ROOT / "assets/audio/pixabay"
SOURCE = AUDIO / "waterfall_loop.wav"
SOURCE_HASH = "de7711f0e6ef9cf0bdd3d04ba7a1b713ce09ef18169bd994182b09ab2017ee62"


def main():
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == SOURCE_HASH
    with wave.open(str(SOURCE), "rb") as source:
        assert source.getparams()[:3] == (1, 2, 48000)
        pcm = array.array("h", source.readframes(source.getnframes()))
    cues = []
    for name, start, duration, gain in (
        ("water_wet_step_1", 6.25, .22, .65),
        ("water_wet_step_2", 8.75, .24, .65),
        ("water_stream_contact", 13.65, .35, .70),
    ):
        frames = int(duration * 48000)
        offset = int(start * 48000)
        output = array.array("h")
        for i, value in enumerate(pcm[offset:offset + frames]):
            envelope = min(1., i / 480., (frames - 1 - i) / 1440.)
            sample = round(value * gain * max(0., envelope))
            output.append(sample)
        for platform, channels, suffix in (("Windows", 1, ""), ("Android", 2, "_core")):
            samples = output
            if channels == 2:
                samples = array.array("h", (value for sample in output for value in (sample, sample)))
            destination = AUDIO / (name + suffix + ".wav")
            with wave.open(str(destination), "wb") as target:
                target.setparams((channels, 2, 48000, frames, "NONE", "not compressed"))
                target.writeframes(samples.tobytes())
            cues.append(dict(path=destination.relative_to(ROOT).as_posix(), platform=platform,
                             channels=channels, sourceStartSeconds=start, durationSeconds=duration,
                             processingGain=gain, frames=frames,
                             bytes=destination.stat().st_size,
                             sha256=hashlib.sha256(destination.read_bytes()).hexdigest()))
    manifest = dict(schema=1, source="assets/audio/pixabay/waterfall_loop.wav",
                    sourceSha256=SOURCE_HASH, sampleRate=48000,
                    bitsPerSample=16, cues=cues,
                    processing="Fixed source cuts; 10ms fade-in/30ms fade-out, gain reduction, dual-mono. No normalisation.",
                    license="Existing DRAGON-STUDIO/Pixabay adaptation licence; retain ASSET_LICENSES.md. Not MIT.",
                    ownerListeningApproval=False)
    (AUDIO / "water-contact.manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", newline="\n")


if __name__ == "__main__":
    main()
