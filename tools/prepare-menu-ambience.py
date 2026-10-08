"""Prepare bounded menu audio candidates from the owner's licensed local sources.

Original source files remain local and are never copied into the repository.
The outputs are game candidates, not owner-approved final mix assets.
"""
from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import shutil
import subprocess
import sys
import wave
from pathlib import Path

SAMPLE_RATE = 48_000
SAMPLE_BYTES = 2
SOURCE_HASHES = {
    "flame": "f1e9e068a21db68221447ba22c26f09d9e928e5de596334272e733f0b60022b4",
    "room": "102b22009583699ae35f9a84f14f4a37cef0d667d34ecbf692d19bdbe190692c",
    "chain": "345785fa9f5b3179a6432ff45ca0164eee1dd11ae19481e0ff1522baecd02c89",
}
EXPECTED_FILES = {
    "menu_flame.wav": 4 * SAMPLE_RATE,
    "menu_room.wav": 4 * SAMPLE_RATE,
    "menu_chain.wav": SAMPLE_RATE,
}
PIXABAY_LICENSE = "https://pixabay.com/service/license-summary/"
FILMCOW_LICENSE = "https://filmcow.itch.io/filmcow-sfx"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def dbfs(value: float) -> float:
    return 20.0 * math.log10(max(value, 1e-12))


def decode_pcm16(source: Path, filters: str | None = None) -> array.array:
    if sys.byteorder != "little":
        raise RuntimeError("This deterministic PCM16 pipeline currently requires a little-endian host.")
    args = ["ffmpeg", "-v", "error", "-i", str(source)]
    if filters:
        args += ["-af", filters]
    args += ["-ac", "1", "-ar", str(SAMPLE_RATE), "-f", "s16le", "pipe:1"]
    raw = subprocess.check_output(args)
    if len(raw) % SAMPLE_BYTES:
        raise RuntimeError(f"FFmpeg returned incomplete PCM16 samples for {source.name}")
    samples = array.array("h")
    samples.frombytes(raw)
    return samples


def crop(samples: array.array, start_s: float, duration_s: float) -> array.array:
    begin = round(start_s * SAMPLE_RATE)
    end = min(len(samples), begin + round(duration_s * SAMPLE_RATE))
    if end - begin != round(duration_s * SAMPLE_RATE):
        raise RuntimeError("The selected source crop is shorter than requested.")
    return array.array("h", samples[begin:end])


def stats(samples: array.array) -> dict[str, float | int]:
    if not samples:
        raise RuntimeError("Cannot measure empty audio.")
    peak = max(abs(value) for value in samples) / 32768.0
    rms = math.sqrt(sum(value * value for value in samples) / len(samples)) / 32768.0
    return {
        "frames": len(samples),
        "durationSeconds": len(samples) / SAMPLE_RATE,
        "rmsDbfs": round(dbfs(rms), 4),
        "peakDbfs": round(dbfs(peak), 4),
    }


def apply_gain(samples: array.array, gain_db: float, ceiling_dbfs: float | None = None) -> tuple[array.array, float]:
    peak = max(abs(value) for value in samples) / 32768.0
    gain = 10.0 ** (gain_db / 20.0)
    if ceiling_dbfs is not None and peak > 0:
        max_gain = (10.0 ** (ceiling_dbfs / 20.0)) / peak
        gain = min(gain, max_gain)
    applied_db = dbfs(gain)
    return array.array("h", (max(-32768, min(32767, round(value * gain))) for value in samples)), applied_db


def cyclic_crossfade(samples: array.array, overlap_frames: int) -> array.array:
    if overlap_frames <= 0 or len(samples) <= overlap_frames * 2:
        raise RuntimeError("Invalid cyclic crossfade length.")
    # Rotate past the head we blend into the tail. The final blended sample
    # approaches head[overlap-1], so the next wrap continues at head[overlap].
    # Keeping head[0] at the start would jump back half a second at every wrap.
    output = array.array("h", samples[overlap_frames:-overlap_frames])
    denominator = max(1, overlap_frames - 1)
    for index in range(overlap_frames):
        amount = index / denominator
        tail_weight = math.cos(amount * math.pi / 2.0)
        head_weight = math.sin(amount * math.pi / 2.0)
        mixed = samples[len(samples) - overlap_frames + index] * tail_weight + samples[index] * head_weight
        output.append(max(-32768, min(32767, round(mixed))))
    return output


def edge_fade(samples: array.array, in_frames: int, out_frames: int) -> array.array:
    result = array.array("h", samples)
    for index in range(min(in_frames, len(result))):
        result[index] = round(result[index] * index / max(1, in_frames - 1))
    for index in range(min(out_frames, len(result))):
        sample_index = len(result) - 1 - index
        result[sample_index] = round(result[sample_index] * index / max(1, out_frames - 1))
    return result


def write_wav(path: Path, samples: array.array) -> dict[str, object]:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(SAMPLE_BYTES)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(samples.tobytes())
    payload = path.read_bytes()
    return {
        "runtimeFile": path.name,
        "runtimeSha256": sha256(payload),
        "runtimeBytes": len(payload),
        "sampleRate": SAMPLE_RATE,
        "channels": 1,
        "bitsPerSample": 16,
        **stats(samples),
    }


def verify(output_dir: Path) -> None:
    # A continuously advancing source ramp must continue by one source sample
    # across the baked loop seam. This catches repeating the blended head.
    probe = array.array("h", range(3000))
    loop = cyclic_crossfade(probe, 500)
    if len(loop) != 2500 or loop[0] - loop[-1] != 1:
        raise RuntimeError("Cyclic crossfade wrap continuity regression.")
    manifest_path = output_dir / "asset.manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError("Menu ambience manifest is missing.")
    allowed = set(EXPECTED_FILES) | {manifest_path.name}
    found = {path.name for path in output_dir.iterdir()}
    if found != allowed:
        raise RuntimeError(f"Unexpected menu audio file roster: {sorted(found)}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    assets = manifest.get("assets")
    if not isinstance(assets, list) or {asset.get("runtimeFile") for asset in assets} != set(EXPECTED_FILES):
        raise RuntimeError("Manifest does not contain the closed three-file runtime roster.")
    expected_ids = {"softflamehisscandidate": "flame", "filteredroomaircandidate": "room", "quietchainswingcandidate": "chain"}
    if manifest.get("sourceHashes") != SOURCE_HASHES:
        raise RuntimeError("Manifest source-hash roster differs from the pinned inputs.")
    for asset in assets:
        if asset.get("id") not in expected_ids:
            raise RuntimeError(f"Unexpected manifest asset id: {asset.get('id')}")
        if asset.get("sourceSha256") != SOURCE_HASHES[expected_ids[asset["id"]]]:
            raise RuntimeError(f"Source hash does not match pinned input for {asset['id']}.")
        path = output_dir / asset["runtimeFile"]
        if sha256(path.read_bytes()) != asset.get("runtimeSha256"):
            raise RuntimeError(f"Runtime hash mismatch: {path.name}")
        with wave.open(str(path), "rb") as audio:
            if (audio.getframerate(), audio.getnchannels(), audio.getsampwidth(), audio.getnframes()) != (
                SAMPLE_RATE, 1, SAMPLE_BYTES, EXPECTED_FILES[path.name]
            ):
                raise RuntimeError(f"Runtime WAV format/duration mismatch: {path.name}")
        measured = stats(array.array("h", path.read_bytes()[44:]))
        for key in ("frames", "durationSeconds", "rmsDbfs", "peakDbfs"):
            if abs(float(measured[key]) - float(asset[key])) > (0.0002 if key != "durationSeconds" else 1e-8):
                raise RuntimeError(f"Manifest measurement mismatch for {path.name}: {key}")
        if path.stat().st_size > 384_044:
            raise RuntimeError(f"Runtime candidate exceeds 4-second mono PCM16 size bound: {path.name}")
    print("Verified three menu WAV candidates, exact manifest roster, format, frames, measurements and runtime hashes.")


def source_record(source: Path, expected_hash: str, source_name: str) -> tuple[bytes, int]:
    if not source.is_file():
        raise FileNotFoundError(f"Missing local source: {source}")
    payload = source.read_bytes()
    actual = sha256(payload)
    if actual != expected_hash:
        raise RuntimeError(f"Source hash mismatch for {source_name}: {actual}")
    return payload, len(payload)


def quiet_chain_window(samples: array.array) -> tuple[array.array, int, dict[str, float | int]]:
    window = SAMPLE_RATE
    hop = SAMPLE_RATE // 10
    candidates = []
    for begin in range(0, len(samples) - window + 1, hop):
        segment = samples[begin:begin + window]
        measured = stats(segment)
        # Exclude silence; among the remaining one-second excerpts, choose the
        # lowest-RMS movement. The low peak guard also avoids a hidden clank.
        if measured["peakDbfs"] >= -40.0:
            candidates.append((float(measured["rmsDbfs"]), float(measured["peakDbfs"]), begin, segment, measured))
    if not candidates:
        raise RuntimeError("No audible bounded chain interval met the selection guard.")
    _, _, begin, segment, measured = min(candidates, key=lambda item: (item[0], item[1], item[2]))
    return array.array("h", segment), begin, measured


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    filmcow_default = Path(r"C:\Users\sam_s\Documents\Possum Cafe\PossumCafeAndroid\Archive\FilmCow Recorded SFX")
    chain_default = Path(r"C:\Users\sam_s\Downloads\hammy01-chain-287197.mp3")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true", help="verify generated candidates without reading private source files")
    parser.add_argument("--source-root", type=Path, default=filmcow_default)
    parser.add_argument("--chain-source", type=Path, default=chain_default)
    parser.add_argument("--output-dir", type=Path, default=repo / "assets" / "audio" / "menu")
    args = parser.parse_args()
    output_dir = args.output_dir.resolve()
    if args.verify:
        verify(output_dir)
        return
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise RuntimeError("FFmpeg is required to regenerate menu ambience candidates.")
    version = subprocess.check_output([ffmpeg, "-version"], text=True).splitlines()[0]
    if "version 9." not in version:
        raise RuntimeError(f"Expected pinned FFmpeg 9 for deterministic regeneration; found: {version}")

    flame_path = args.source_root / "gas leak.wav"
    room_path = args.source_root / "ambience - air conditioner.wav"
    _, flame_bytes = source_record(flame_path, SOURCE_HASHES["flame"], "FilmCow gas leak")
    _, room_bytes = source_record(room_path, SOURCE_HASHES["room"], "FilmCow air conditioner")
    _, chain_bytes = source_record(args.chain_source, SOURCE_HASHES["chain"], "Pixabay Hammy01 Chain")

    # Filter the full decoded source before trimming, avoiding filter startup
    # transients at the selected middle crop. A 0.5 s equal-power cyclic seam
    # reduces the 4.5 s crops to exactly 4.0 s runtime loops.
    flame_src = decode_pcm16(flame_path, "highpass=f=180,lowpass=f=4200")
    flame_crop = crop(flame_src, 4.5, 4.5)
    flame_loop = cyclic_crossfade(flame_crop, SAMPLE_RATE // 2)
    flame_pre = stats(flame_loop)
    flame_gain_db = max(-60.0, -24.0 - float(flame_pre["rmsDbfs"]))
    flame_loop, flame_gain_applied = apply_gain(flame_loop, flame_gain_db, ceiling_dbfs=-6.0)
    flame_runtime = write_wav(output_dir / "menu_flame.wav", flame_loop)

    # The air-conditioner source has prominent 100 Hz and weaker 200 Hz tones
    # in this crop. Notches reduce those lines before a restrained low-band bed.
    room_filters = (
        "equalizer=f=100:t=q:w=5:g=-18,equalizer=f=200:t=q:w=5:g=-10,"
        "highpass=f=140,lowpass=f=700"
    )
    room_src = decode_pcm16(room_path, room_filters)
    room_crop = crop(room_src, 4.5, 4.5)
    room_loop = cyclic_crossfade(room_crop, SAMPLE_RATE // 2)
    room_pre = stats(room_loop)
    room_gain_db = max(-60.0, -30.0 - float(room_pre["rmsDbfs"]))
    room_loop, room_gain_applied = apply_gain(room_loop, room_gain_db, ceiling_dbfs=-9.0)
    room_runtime = write_wav(output_dir / "menu_room.wav", room_loop)

    chain_src = decode_pcm16(args.chain_source)
    chain_excerpt, chain_begin, chain_source_stats = quiet_chain_window(chain_src)
    chain_faded = edge_fade(chain_excerpt, SAMPLE_RATE // 50, SAMPLE_RATE * 12 // 100)
    chain_pre = stats(chain_faded)
    chain_gain_db = -12.0 - float(chain_pre["peakDbfs"])
    chain_runtime_samples, chain_gain_applied = apply_gain(chain_faded, chain_gain_db, ceiling_dbfs=-12.0)
    chain_runtime = write_wav(output_dir / "menu_chain.wav", chain_runtime_samples)

    assets = [
        {
            "id": "softflamehisscandidate",
            **flame_runtime,
            "sourceFile": "FilmCow Recorded SFX/gas leak.wav",
            "sourceSha256": SOURCE_HASHES["flame"],
            "sourceBytes": flame_bytes,
            "creator": "FilmCow",
            "sourceUrl": FILMCOW_LICENSE,
            "license": "FilmCow custom royalty-free license; not CC0",
            "licenseTerms": "Personal and commercial project use permitted; national-government, law-enforcement, and SPLC/CAHN-designated hate-group uses prohibited; do not claim authorship or resell.",
            "processing": {
                "decode": "FFmpeg 9, mono 48 kHz PCM16",
                "cropSourceSeconds": [4.5, 9.0],
                "filters": ["highpass 180 Hz", "lowpass 4200 Hz"],
                "cyclicCrossfadeFrames": SAMPLE_RATE // 2,
                "loopFrames": 4 * SAMPLE_RATE,
                "targetRmsDbfs": -24.0,
                "peakCeilingDbfs": -6.0,
                "appliedGainDb": round(flame_gain_applied, 4),
                "preGain": flame_pre,
            },
            "limitations": "Filtered gas-leak hiss proxy, not a recording of fire; has no ignition or crackle. Owner exact-candidate listening and in-game mix acceptance pending.",
        },
        {
            "id": "filteredroomaircandidate",
            **room_runtime,
            "sourceFile": "FilmCow Recorded SFX/ambience - air conditioner.wav",
            "sourceSha256": SOURCE_HASHES["room"],
            "sourceBytes": room_bytes,
            "creator": "FilmCow",
            "sourceUrl": FILMCOW_LICENSE,
            "license": "FilmCow custom royalty-free license; not CC0",
            "licenseTerms": "Personal and commercial project use permitted; national-government, law-enforcement, and SPLC/CAHN-designated hate-group uses prohibited; do not claim authorship or resell.",
            "processing": {
                "decode": "FFmpeg 9, mono 48 kHz PCM16",
                "cropSourceSeconds": [4.5, 9.0],
                "filters": ["notch 100 Hz, Q5, -18 dB", "notch 200 Hz, Q5, -10 dB", "highpass 140 Hz", "lowpass 700 Hz"],
                "cyclicCrossfadeFrames": SAMPLE_RATE // 2,
                "loopFrames": 4 * SAMPLE_RATE,
                "targetRmsDbfs": -30.0,
                "peakCeilingDbfs": -9.0,
                "appliedGainDb": round(room_gain_applied, 4),
                "preGain": room_pre,
            },
            "limitations": "Strongly filtered mechanical air-conditioner source with measured 100/200 Hz tonal components; this remains a room-air candidate, not verified natural room tone. Owner exact-candidate listening and in-game mix acceptance pending.",
        },
        {
            "id": "quietchainswingcandidate",
            **chain_runtime,
            "sourceFile": args.chain_source.name,
            "sourceSha256": SOURCE_HASHES["chain"],
            "sourceBytes": chain_bytes,
            "creator": "Hammy01",
            "sourceUrl": "https://pixabay.com/sound-effects/film-special-effects-chain-287197/",
            "license": "Pixabay Content License",
            "licenseTerms": "Modified game use permitted; standalone distribution of the source audio is prohibited.",
            "voluntaryCredit": "Chain by Hammy01 via Pixabay",
            "processing": {
                "decode": "FFmpeg 9, mono 48 kHz PCM16",
                "selection": "Lowest-RMS one-second window on a 100 ms grid, requiring source peak >= -40 dBFS to exclude silence; lowest peak breaks ties.",
                "sourceCropFrames": [chain_begin, chain_begin + SAMPLE_RATE],
                "sourceCropSeconds": [round(chain_begin / SAMPLE_RATE, 6), round((chain_begin + SAMPLE_RATE) / SAMPLE_RATE, 6)],
                "sourceWindowStats": chain_source_stats,
                "edgeFadeFrames": {"in": SAMPLE_RATE // 50, "out": SAMPLE_RATE * 12 // 100},
                "targetPeakDbfs": -12.0,
                "appliedGainDb": round(chain_gain_applied, 4),
            },
            "limitations": "Bounded quiet movement excerpt selected by waveform level, not by listening. Timed one-shot candidate; owner exact-candidate listening and in-game mix acceptance pending.",
        },
    ]
    manifest = {
        "schema": 1,
        "target": "Horde Lantern RT menu audio candidate",
        "sourceHashes": SOURCE_HASHES,
        "sourcePolicy": "Private original sources stay local and outside Git/packages. Only the three bounded runtime derivatives and provenance manifest are written. No standalone audio-library distribution.",
        "generator": "tools/prepare-menu-ambience.py",
        "ffmpegVersion": version,
        "assets": assets,
        "ownerListeningAcceptance": "pending",
    }
    manifest_path = output_dir / "asset.manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    verify(output_dir)
    print(json.dumps(assets, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        raise SystemExit(f"prepare-menu-ambience: {error}") from error
