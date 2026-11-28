"""Author the Bellwether oak journal contact as a finite original PCM cue."""

import argparse
import hashlib
import io
import json
import math
import random
import struct
import wave
from pathlib import Path

RATE = 48000
FRAMES = 36480
PEAK_SAMPLE = 12600
WAV = Path("runtime/wheel_creak.wav")


def encode_contact():
    rng = random.Random(570921)
    low, high = 0.0, 0.0
    low_alpha = 1 - math.exp(-math.tau * 130 / RATE)
    high_alpha = 1 - math.exp(-math.tau * 1250 / RATE)
    pulses = ((.039, .012, .37), (.103, .025, .56), (.179, .035, .88),
              (.267, .032, .65), (.361, .058, .97), (.477, .046, .41))
    scrape_phase = 0.0
    mix = []
    for i in range(FRAMES):
        t = i / RATE
        noise = rng.uniform(-1, 1)
        low += low_alpha * (noise - low)
        high += high_alpha * (noise - high)
        load = sum(weight * math.exp(-((t - centre) / width) ** 2)
                   for centre, width, weight in pulses)
        scrape_phase += math.tau * (93 + 31 * math.sin(math.pi * t / .63)
                                   + 14 * math.sin(math.tau * 5.1 * t)) / RATE
        friction = (high - low) * (1.8 + .45 * math.sin(scrape_phase * .47))
        strains = .14 * math.sin(scrape_phase) + .06 * math.sin(scrape_phase * 2.71)
        release = 0.0
        for start, weight in ((.023, .55), (.242, .31), (.468, .47)):
            age = t - start
            if age > 0:
                release += weight * min(1, age / .006) * math.exp(-age * 38) * (
                    math.sin(math.tau * 181 * age) + .28 * math.sin(math.tau * 437 * age))
        edge = min(1.0, t / .012, (FRAMES - 1 - i) / (RATE * .13))
        mix.append((load * (friction + strains) + .23 * release) * edge)

    # Keep the wood contact present under the default score at the dry footway.
    scale = PEAK_SAMPLE / max(map(abs, mix))
    pcm = [round(sample * scale) for sample in mix]
    target = io.BytesIO()
    with wave.open(target, "wb") as out:
        out.setparams((1, 2, RATE, FRAMES, "NONE", "not compressed"))
        out.writeframes(struct.pack(f"<{FRAMES}h", *pcm))
    return target.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    audio = encode_contact()
    manifest = {
        "schema": 1,
        "target": "Bellwether waterwheel oak journal contact",
        "source": "Original seeded friction, resonant release and oak load envelopes",
        "recipe": "tools/prepare-bellwether-mill-audio.py",
        "sampleRate": RATE,
        "bitsPerSample": 16,
        "channels": 1,
        "assets": [{
            "id": "wheel_creak", "runtimeFile": WAV.as_posix(), "frames": FRAMES,
            "runtimeSha256": hashlib.sha256(audio).hexdigest(), "runtimeBytes": len(audio),
            "peakSample": PEAK_SAMPLE, "loop": False,
        }],
    }
    for relative, expected in ((WAV, audio), (Path("asset.manifest.json"),
                               (json.dumps(manifest, indent=2) + "\n").encode("utf-8"))):
        path = args.output_dir / relative
        if args.verify:
            if path.read_bytes() != expected:
                raise ValueError(f"Generated cue differs from {path}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(expected)


if __name__ == "__main__":
    main()
