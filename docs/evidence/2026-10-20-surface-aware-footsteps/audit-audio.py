"""Read-only PCM/provenance audit. Does not play audio or write runtime assets."""
import hashlib
import io
import json
import math
from pathlib import Path
import struct
import subprocess
import wave

ROOT = next(path for path in Path(__file__).resolve().parents
            if (path / "CMakeLists.txt").is_file() and (path / "ASSET_LICENSES.md").is_file())
BANK = Path(r"C:\Users\sam_s\Documents\Possum Cafe\PossumCafeAndroid\Archive\FilmCow Recorded SFX")
PAIRS = [(9, "player_step_1.wav"), (14, "player_step_2.wav"),
         (17, "skeleton_step_1.wav"), (18, "skeleton_step_2.wav")]

def inspect(path):
    raw = path.read_bytes()
    with wave.open(io.BytesIO(raw), "rb") as wav:
        channels, width, rate, frames, compression, _ = wav.getparams()
        pcm = wav.readframes(frames)
    assert channels == 1 and rate == 48000 and compression == "NONE"
    samples = [int.from_bytes(pcm[n:n+width], "little", signed=True)
               for n in range(0, len(pcm), width)]
    full_scale = 2 ** (width * 8 - 1)
    return raw, samples, {
        "path": str(path), "sha256": hashlib.sha256(raw).hexdigest(),
        "bytes": len(raw), "channels": channels, "sampleRate": rate,
        "bitsPerSample": width*8, "frames": frames, "seconds": frames/rate,
        "peak": max(abs(s) for s in samples)/full_scale,
        "rms": math.sqrt(sum(s*s for s in samples)/len(samples))/full_scale,
        "digitalFullScaleSamples": sum(s in (-full_scale, full_scale-1) for s in samples)
    }

rows = []
for number, name in PAIRS:
    source_path = BANK / f"footstep dirt {number}.wav"
    runtime_path = ROOT / "assets/audio/filmcow" / name
    source, samples, source_info = inspect(source_path)
    runtime, _, runtime_info = inspect(runtime_path)
    assert source_info["bitsPerSample"] == 24 and runtime_info["bitsPerSample"] == 16
    assert source_info["frames"] == runtime_info["frames"]
    scale = .78 * 8388607 / max(abs(s) for s in samples)
    converted = b"".join(struct.pack("<h", max(-32768, min(32767, round(s*scale/256))))
                         for s in samples)
    reconstructed = io.BytesIO()
    with wave.open(reconstructed, "wb") as wav:
        wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(48000)
        wav.writeframes(converted)
    assert reconstructed.getvalue() == runtime
    relative = runtime_path.relative_to(ROOT).as_posix()
    assert subprocess.check_output(["git", "show", f"HEAD:{relative}"], cwd=ROOT) == runtime
    assert not source_info["digitalFullScaleSamples"] and not runtime_info["digitalFullScaleSamples"]
    assert runtime_info["seconds"] < .55 and runtime_info["peak"] < .781
    rows.append({"source": source_info, "runtime": runtime_info,
                 "byteExactImporterRecipe": True, "unchangedFromHead": True,
                 "playerCueGain": .29, "peakAtPlayerCueGain": runtime_info["peak"]*.29,
                 "role": "accepted generic Stone/Wood fallback" if number < 17 else "Earth dirt reuse"})

result = {"sourceBase": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
          "sourceBank": str(BANK), "clips": rows,
          "licensePdfSha256": hashlib.sha256((ROOT/"docs/licenses/filmcow-sfx-license.pdf").read_bytes()).hexdigest(),
          "licenseRecord": "ASSET_LICENSES.md:115", "newAssets": 0, "audioPlayback": False,
          "limitations": ["File metadata/PCM data are not listening acceptance.",
                          "Stone retains the accepted generic pair; no new verified stone recording.",
                          "Wood uses the generic fallback; a suitable admitted wood-floor cue is missing.",
                          "Earth full-file RMS is about 6 dB lower; no automatic gain compensation.",
                          "Tail overlap, cue character, speech/combat balance and mono-phone feel remain unassessed."]}
output = ROOT / "reports/dry-footstep-audio-audit.json"
output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
print(f"PASS: {len(rows)} admitted WAVs unchanged, exact conversion reproduced, no digital full-scale samples; no playback")
