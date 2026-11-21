"""Build and verify the two short Bellwether ankle foley cues."""

import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import random
import struct
import wave


ROOT = Path(__file__).resolve().parents[1]
RATE = 48000
LIBRARY = Path(r"C:\Users\sam_s\Documents\Possum Cafe\PossumCafeAndroid\Archive\FilmCow Recorded SFX")
LICENSE = "docs/licenses/filmcow-sfx-license.pdf"
DIRT = ("footstep dirt 7.wav", "2ba4863c2f5b055fc191dffbe51d43da869fdc782997b95e5a0ff6a29e810108")
STONE = ("rock 19.wav", "3fa86c60a95e5cdf1f0df60f24bab967db55b29f8fae9460c1ed70d0a25db4c1")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_crop(library, source, start, end):
    name, expected = source
    data = (library / name).read_bytes()
    if digest(data) != expected:
        raise RuntimeError(f"FilmCow source hash changed: {name}")
    with wave.open(io.BytesIO(data), "rb") as source_wav:
        if (source_wav.getnchannels(), source_wav.getsampwidth(), source_wav.getframerate()) != (1, 3, RATE):
            raise RuntimeError(f"Unexpected source PCM: {name}")
        source_wav.setpos(start)
        raw = source_wav.readframes(end - start)
    if len(raw) != (end - start) * 3:
        raise RuntimeError(f"Incomplete PCM crop: {name}")
    samples = [int.from_bytes(raw[i:i + 3], "little", signed=True) / 8388608
               for i in range(0, len(raw), 3)]
    return samples, {"file": name, "sha256": expected, "cropFrames": [start, end]}


def lowpass(samples, frequency):
    alpha = 1 - math.exp(-2 * math.pi * frequency / RATE)
    value = 0.0
    result = []
    for sample in samples:
        value += alpha * (sample - value)
        result.append(value)
    return result


def band(samples, low, high):
    filtered = lowpass(samples, high)
    return [sample - bass for sample, bass in zip(filtered, lowpass(filtered, low))]


def edge(samples, fade_in, fade_out):
    return [sample * min(1, i / fade_in, (len(samples) - 1 - i) / fade_out)
            for i, sample in enumerate(samples)]


def fit_peak(samples, target):
    scale = target / max(map(abs, samples))
    return [sample * scale for sample in samples]


def place(mix, layer, offset):
    for i, sample in enumerate(layer, offset):
        mix[i] += sample


def slip(library):
    dirt, dirt_source = read_crop(library, DIRT, 1200, 17184)
    pebble, stone_source = read_crop(library, STONE, 1960, 8200)
    mix = [0.0] * 17280
    # Trim the sharp leading stomp and retain the dry scrape. The stone is a
    # small, bright contact under the boot, followed by one much softer tick.
    place(mix, fit_peak(edge(band(dirt, 125, 4100), 620, 4300), .17), 0)
    gravel = fit_peak(edge(band(pebble, 920, 5000), 120, 2450), .048)
    place(mix, gravel, 1584)
    place(mix, [sample * .24 for sample in gravel[::2]], 4816)
    return edge(mix, 144, 960), [dirt_source, stone_source]


def breath():
    # One soft, unvoiced breath. Seeded filtered air with a gentle catch near
    # the beginning. No pitch, words, actor sample or voice model.
    rng = random.Random(47519)
    noise = [rng.uniform(-1, 1) for _ in range(21120)]
    body = band(noise, 330, 1370)
    air = band(noise, 1350, 2850)
    samples = []
    for i, (low, high) in enumerate(zip(body, air)):
        progress = i / (len(noise) - 1)
        envelope = math.sin(math.pi * progress) ** 1.55 * (1 - .34 * progress)
        catch = .24 * math.exp(-((progress - .24) / .065) ** 2)
        samples.append((low * .84 + high * .31) * envelope * (1 + catch))
    return fit_peak(edge(samples, 480, 3300), .109)


def encode(name, samples):
    values = [round(sample * 32768) for sample in samples]
    peak = max(map(abs, values))
    if values[0] or values[-1] or peak >= 8192:
        raise RuntimeError(f"Invalid contact edge or headroom: {name}")
    output = io.BytesIO()
    with wave.open(output, "wb") as target:
        target.setparams((1, 2, RATE, len(values), "NONE", "not compressed"))
        target.writeframes(struct.pack(f"<{len(values)}h", *values))
    data = output.getvalue()
    return data, {"runtimeFile": f"runtime/{name}.wav", "runtimeSha256": digest(data),
                  "frames": len(values), "bytes": len(data),
                  "peakDbfs": round(20 * math.log10(peak / 32768), 3)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=LIBRARY)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    licence_hash = digest((ROOT / LICENSE).read_bytes())
    if licence_hash != "bda56a077aa8ba8e053ae916dd4c19038baa12cd33d10209658646ac9351b73f":
        raise RuntimeError("The FilmCow project licence evidence changed")
    slip_samples, sources = slip(args.source_root)
    ground, ground_info = encode("boot_and_gravel", slip_samples)
    air, air_info = encode("steady_breath", breath())
    manifest = {"schema": 1, "sampleRate": RATE, "channels": 1, "bitsPerSample": 16,
                "listeningAcceptance": False,
                "assets": [{**ground_info, "license": "FilmCow custom royalty-free project-use licence",
                            "licenseEvidence": LICENSE, "licenseSha256": licence_hash,
                            "sources": sources},
                           {**air_info, "license": "Original Horde foley, Samfa12/Codex",
                            "source": "Locally synthesized unvoiced filtered noise, seed 47519"}]}
    outputs = {ground_info["runtimeFile"]: ground, air_info["runtimeFile"]: air,
               "asset.manifest.json": (json.dumps(manifest, indent=2) + "\n").encode()}
    if args.verify:
        for filename, data in outputs.items():
            path = args.output_dir / filename
            actual = path.read_text(encoding="utf-8").encode() if path.suffix == ".json" else path.read_bytes()
            if actual != data:
                raise RuntimeError(f"Rebuilt bytes differ: {filename}")
    else:
        (args.output_dir / "runtime").mkdir(parents=True, exist_ok=False)
        for filename, data in outputs.items():
            (args.output_dir / filename).write_bytes(data)
    print("Verified byte-exact Kit ankle foley" if args.verify else "Prepared Kit ankle foley")


if __name__ == "__main__":
    main()
