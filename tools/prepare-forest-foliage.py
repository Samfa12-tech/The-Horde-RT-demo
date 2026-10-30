"""Prepare one finite licensed FilmCow foliage cue; private sources stay read-only."""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = Path(r"C:\Users\sam_s\Documents\Possum Cafe\PossumCafeAndroid\Archive\FilmCow Recorded SFX")
SOURCE_NAME = "leaves rustle 1.wav"
SOURCE_SHA256 = "1c51a4b6ec25fddab7e395e253b8998f415135f6f183ffbf2133b7c3f0750769"
LICENSE_SHA256 = "bda56a077aa8ba8e053ae916dd4c19038baa12cd33d10209658646ac9351b73f"
SOURCE_FRAMES = 43283
SAMPLE_RATE = 48000
FADE_IN, FADE_OUT = 480, 3840


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def levels(samples, full_scale):
    peak = max(abs(value) for value in samples)
    rms = math.sqrt(sum(value * value for value in samples) / len(samples))
    require(peak > 0 and rms > 0, "Source/candidate must contain nonzero PCM activity")
    return {"peakDbfs": 20 * math.log10(peak / full_scale),
            "rmsDbfs": 20 * math.log10(rms / full_scale),
            "fullScaleSamples": sum(value in (-full_scale, full_scale - 1) for value in samples)}


def decode_source(source):
    with wave.open(io.BytesIO(source), "rb") as wav:
        require((wav.getnchannels(), wav.getframerate(), wav.getsampwidth(), wav.getcomptype()) ==
                (1, SAMPLE_RATE, 3, "NONE"), "Source must remain uncompressed mono48kHz PCM24")
        frames = wav.getnframes()
        raw = wav.readframes(frames)
    require(len(raw) == frames * 3, "Source contains an incomplete PCM24 payload")
    return [int.from_bytes(raw[i:i + 3], "little", signed=True) for i in range(0, len(raw), 3)]


def nearest_even(numerator, denominator):
    """Signed rational round-to-nearest, ties-to-even; no floating-point tie drift."""
    sign = -1 if numerator < 0 else 1
    quotient, remainder = divmod(abs(numerator), denominator)
    if remainder * 2 > denominator or (remainder * 2 == denominator and quotient % 2):
        quotient += 1
    return sign * quotient


def build(source_root):
    source = (source_root / SOURCE_NAME).read_bytes()
    require(sha256(source) == SOURCE_SHA256, "Selected FilmCow source digest changed")
    samples = decode_source(source)
    require(len(samples) == SOURCE_FRAMES, "Full selected source duration changed")
    converted = []
    for i, value in enumerate(samples):
        # min(1, i/480, (N-1-i)/3840), evaluated as exact rational gains.
        gain_numerator, gain_denominator = 1, 1
        for numerator, denominator in ((i, FADE_IN), (len(samples) - 1 - i, FADE_OUT)):
            if numerator * gain_denominator < gain_numerator * denominator:
                gain_numerator, gain_denominator = numerator, denominator
        converted.append(nearest_even(value * gain_numerator, 256 * gain_denominator))
    require(converted[0] == converted[-1] == 0 and all(-32768 <= value <= 32767 for value in converted),
            "PCM16 endpoint/range check failed")
    runtime = io.BytesIO()
    with wave.open(runtime, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(struct.pack("<" + "h" * len(converted), *converted))
    payload = runtime.getvalue()
    license_file = ROOT / "docs/licenses/filmcow-sfx-license.pdf"
    require(sha256(license_file.read_bytes()) == LICENSE_SHA256, "Existing FilmCow licence PDF digest changed")
    manifest = {
        "schema": 1,
        "target": "Horde 1.7 finite localized forest foliage cue",
        "generator": "tools/prepare-forest-foliage.py",
        "sourceLibrary": "FilmCow Recorded SFX; local Possum Cafe archive, private originals excluded",
        "licensor": "Jason Steele / FilmCow",
        "sourceUrl": "https://filmcow.itch.io/filmcow-sfx",
        "license": "FilmCow custom royalty-free project-use licence; not CC0",
        "licenseEvidence": "docs/licenses/filmcow-sfx-license.pdf",
        "licenseSha256": LICENSE_SHA256,
        "rightsRecord": "docs/licenses/README.md",
        "rightsSummary": "Existing worldwide non-exclusive perpetual royalty-free personal/commercial media use; no authorship claim, sound resale, national government, law-enforcement or SPLC/CAHN hate-group project use. Credit optional. Existing terms unchanged.",
        "sourcePolicy": "Only this game-integrated finite derivative and manifest ship; original recordings remain private. No source original is copied or relicensed.",
        "listeningAcceptance": False,
        "selectionLimit": "The requested full purpose-labelled source and measured PCM activity only; no audition, audible-quality, foliage character or native mix acceptance claim.",
        "assets": [{
            "id": "leaf_rustle", "runtimeFile": "runtime/leaf_rustle.wav",
            "sourceFile": SOURCE_NAME, "sourceSha256": SOURCE_SHA256, "sourceBytes": len(source),
            "sourceFrames": len(samples), "sourceCropFrames": [0, len(samples)],
            "sourceFormat": {"sampleRate": SAMPLE_RATE, "channels": 1, "bitsPerSample": 24},
            "sourceLevels": levels(samples, 8388608),
            "runtimeSha256": sha256(payload), "runtimeBytes": len(payload),
            "sampleRate": SAMPLE_RATE, "channels": 1, "bitsPerSample": 16,
            "frames": len(converted), "durationSeconds": len(converted) / SAMPLE_RATE,
            **levels(converted, 32768),
            "edgeFadeFrames": {"in": FADE_IN, "out": FADE_OUT},
            "edgeFadeMilliseconds": {"in": 10, "out": 80},
            "edgeFadeFormula": "gain[i] = min(1, i/480, (N-1-i)/3840); first and last samples are zero",
            "quantization": "PCM16[i] = nearest-even(PCM24[i] * gain[i] / 256), using exact signed integer rational rounding",
            "loop": False,
            "processing": "Full mono48kHz PCM24 source to PCM16 with 10ms/80ms linear endpoint ramps only. No gain normalization, synthesis, filtering, pitch shift, time-stretch, resampling or looping.",
        }],
    }
    return payload, (json.dumps(manifest, indent=2, allow_nan=False) + "\n").encode("utf-8"), manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "assets/audio/forest-foliage")
    parser.add_argument("--verify", action="store_true", help="Reproduce in memory and compare exact WAV/manifest; write nothing")
    args = parser.parse_args()
    runtime, manifest_bytes, manifest = build(args.source_root)
    products = {args.output_dir / "runtime/leaf_rustle.wav": runtime,
                args.output_dir / "asset.manifest.json": manifest_bytes}
    if args.verify:
        for path, expected in products.items():
            require(path.read_bytes() == expected, "Reproduction differs: " + str(path))
        print("PASS: finite leaf cue and manifest reproduce byte-for-byte; verify wrote nothing; listening unverified")
    else:
        # Existing identical output is reusable; a different file is preserved.
        for path, expected in products.items():
            require(not path.exists() or path.read_bytes() == expected, "Refusing to replace different existing output: " + str(path))
        for path, expected in products.items():
            if not path.exists():
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(expected)
        print(json.dumps({"runtimeSha256": sha256(runtime), "runtimeBytes": len(runtime),
                          "manifestSha256": sha256(manifest_bytes), "manifestBytes": len(manifest_bytes),
                          "asset": manifest["assets"][0]}, indent=2))


if __name__ == "__main__":
    main()
