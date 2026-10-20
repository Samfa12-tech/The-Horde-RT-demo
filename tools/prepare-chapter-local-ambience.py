"""Admit two finite FilmCow cuts. Private originals are read-only and never copied."""
import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    ("tomb_drip", "ambience - dripping water 1.wav",
     "3ef75b5d3294dabf9d61077502303b6467523c4c6bbb31254c3c8031accf91ec", 5083200, 5131200),
    ("rope_contact", "rope tighten 1.wav",
     "738d827dfe689b01fb9a6d18689a9b83127a5ef3612f0473e8f545fc659176bf", 0, 47600),
)

def build(source_root):
    rows = []
    for name, filename, expected, start, end in SOURCES:
        source = (source_root / filename).read_bytes()
        assert hashlib.sha256(source).hexdigest() == expected, filename
        with wave.open(io.BytesIO(source), "rb") as wav:
            assert (wav.getnchannels(), wav.getframerate(), wav.getsampwidth()) == (1, 48000, 3)
            total = wav.getnframes()
            assert 0 <= start < end <= total
            wav.setpos(start)
            raw = wav.readframes(end-start)
        samples = [int.from_bytes(raw[i:i+3], "little", signed=True) / 8388608
                   for i in range(0, len(raw), 3)]
        # Exact frame-domain edge fades; preserve duration and original level.
        faded = [round(v * min(1, i/480, (len(samples)-1-i)/3840) * 32768)
                 for i, v in enumerate(samples)]
        assert max(abs(v) for v in faded) < 32767
        pcm = struct.pack("<" + "h"*len(faded), *faded)
        destination = io.BytesIO()
        with wave.open(destination, "wb") as wav:
            wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(48000)
            wav.writeframes(pcm)
        runtime = destination.getvalue()
        rms = math.sqrt(sum((v/32768)**2 for v in faded)/len(faded))
        rows.append((name, runtime, {
            "id": name, "runtimeFile": "runtime/"+name+".wav",
            "sourceFile": filename, "sourceSha256": expected, "sourceBytes": len(source),
            "sourceFrames": total, "sourceCropFrames": [start, end],
            "runtimeSha256": hashlib.sha256(runtime).hexdigest(), "runtimeBytes": len(runtime),
            "sampleRate": 48000, "channels": 1, "bitsPerSample": 16, "frames": len(faded),
            "durationSeconds": len(faded)/48000,
            "peakDbfs": 20*math.log10(max(abs(v) for v in faded)/32768),
            "rmsDbfs": 20*math.log10(rms), "fullScaleSamples": 0,
            "edgeFadeFrames": {"in": 480, "out": 3840},
            "processing": "Mono48k PCM24 to PCM16; nearest-even rounding, 10ms/80ms linear edge fades. No normalization, filter, pitch, time-stretch or synthesis.",
        }))
    return rows

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(
        r"C:\Users\sam_s\Documents\Possum Cafe\PossumCafeAndroid\Archive\FilmCow Recorded SFX"))
    parser.add_argument("--output-dir", type=Path, default=ROOT/"assets/audio/chapter-local-ambience")
    parser.add_argument("--verify", action="store_true", help="Reproduce in memory and compare; write nothing")
    args = parser.parse_args()
    rows = build(args.source_root)
    licence = ROOT/"docs/licenses/filmcow-sfx-license.pdf"
    licence_sha = hashlib.sha256(licence.read_bytes()).hexdigest()
    assert licence_sha == "bda56a077aa8ba8e053ae916dd4c19038baa12cd33d10209658646ac9351b73f"
    manifest = {
        "schema": 1, "target": "Horde 1.7 local finite incidental cues",
        "generator": "tools/prepare-chapter-local-ambience.py",
        "sourceLibrary": "FilmCow Recorded SFX; local Possum Cafe archive, private originals excluded",
        "licensor": "Jason Steele / FilmCow", "sourceUrl": "https://filmcow.itch.io/filmcow-sfx",
        "license": "FilmCow custom royalty-free project-use licence; not CC0",
        "licenseEvidence": "docs/licenses/filmcow-sfx-license.pdf", "licenseSha256": licence_sha,
        "sourcePolicy": "Only these bounded game-integrated derivatives and manifest ship; originals remain private. Preserve FilmCow project-use restrictions documented in ASSET_LICENSES.md.",
        "listeningAcceptance": False,
        "selectionLimit": "Purpose-labelled sources and PCM activity only, not audition. Drip is an isolated 1s activity window, not verified one droplet or stone creak. Rope is the complete labelled tightening cue. No structural rumble or continuous bed.",
        "assets": [row[2] for row in rows],
    }
    encoded = (json.dumps(manifest, indent=2)+"\n").encode()
    if args.verify:
        assert (args.output_dir/"asset.manifest.json").read_bytes() == encoded
        for _, runtime, row in rows:
            assert (args.output_dir/row["runtimeFile"]).read_bytes() == runtime
        print("PASS: both admitted finite cues and manifest reproduce byte-for-byte; sources read-only")
    else:
        (args.output_dir/"runtime").mkdir(parents=True, exist_ok=True)
        for _, runtime, row in rows:
            (args.output_dir/row["runtimeFile"]).write_bytes(runtime)
        (args.output_dir/"asset.manifest.json").write_bytes(encoded)
        print("Prepared", len(rows), "finite cues;", sum(len(row[1]) for row in rows), "runtime WAV bytes; listening unverified")

if __name__ == "__main__":
    main()
