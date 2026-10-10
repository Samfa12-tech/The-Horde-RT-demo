"""Prepare the keeper-death scream as compact game-integrated PCM assets.

The owner-supplied MP3 remains outside the repository. Pass its location with
--source; only the mono and dual-mono game-ready derivatives and provenance
manifest are written.
"""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import wave


ROOT = Path(__file__).resolve().parents[1]
AUDIO = ROOT / "assets/audio/pixabay"
SOURCE_SHA256 = "d16f67856849c0bb87d9dd6a9c09b15fd729436bdf59ec6c269108b0ebc58d35"
SOURCE_URL = "https://pixabay.com/sound-effects/film-special-effects-monster-demon-voice-death-defeat-scream-582531/"
SOURCE_TITLE = "Monster Demon Voice Death Defeat Scream"
SOURCE_CONTRIBUTOR = "PhatPhrogStudio"
SOURCE_PUBLISHED = "2026-08-17"
LICENSE = "Pixabay Content License"
LICENSE_URL = "https://pixabay.com/service/terms/"
SAMPLE_RATE = 48000
FADE_IN_FRAMES = 480       # 10 ms: remove the MP3 decoder's tiny onset edge.
FADE_OUT_FRAMES = 1440     # 30 ms: soften the already quiet file tail.


def measure(samples):
    peak = max((abs(int(sample)) for sample in samples), default=0) / 32768.0
    rms = math.sqrt(sum(float(sample) * sample for sample in samples) /
                    max(1, len(samples))) / 32768.0
    return peak, rms


def decode_source(path, ffmpeg):
    result = subprocess.run(
        [ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error", "-i", str(path),
         "-map", "0:a:0", "-vn", "-ac", "1", "-ar", str(SAMPLE_RATE),
         "-c:a", "pcm_s16le", "-f", "s16le", "pipe:1"],
        check=True, capture_output=True)
    samples = array.array("h")
    samples.frombytes(result.stdout)
    if sys.byteorder != "little":
        samples.byteswap()
    return samples


def apply_endpoint_fades(samples):
    result = array.array("h")
    frames = len(samples)
    for index, sample in enumerate(samples):
        gain = 1.0
        if index < FADE_IN_FRAMES:
            gain = min(gain, index / FADE_IN_FRAMES)
        remaining = frames - 1 - index
        if remaining < FADE_OUT_FRAMES:
            gain = min(gain, remaining / FADE_OUT_FRAMES)
        result.append(round(sample * max(0.0, gain)))
    return result


def write_wave(path, samples, channels):
    if channels == 1:
        output = samples
    else:
        output = array.array("h", (sample for value in samples for sample in (value, value)))
    if sys.byteorder != "little":
        output.byteswap()
    with wave.open(str(path), "wb") as destination:
        destination.setparams((channels, 2, SAMPLE_RATE, len(samples), "NONE", "not compressed"))
        destination.writeframes(output.tobytes())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path,
                        help="owner-supplied Pixabay MP3; kept outside this repository")
    parser.add_argument("--ffmpeg", default="ffmpeg")
    args = parser.parse_args()
    ffmpeg = shutil.which(args.ffmpeg)
    if ffmpeg is None:
        raise SystemExit(f"FFmpeg executable not found: {args.ffmpeg}")

    source_bytes = args.source.read_bytes()
    source_hash = hashlib.sha256(source_bytes).hexdigest()
    if source_hash != SOURCE_SHA256:
        raise ValueError(f"Source MP3 hash mismatch: expected {SOURCE_SHA256}, got {source_hash}")
    decoded = decode_source(args.source, ffmpeg)
    if not decoded:
        raise ValueError("FFmpeg produced no samples from the source MP3")
    output = apply_endpoint_fades(decoded)
    peak, output_rms = measure(output)
    if peak >= 1.0:
        raise ValueError(f"PCM output would clip: peak={peak:.6f}")

    cues = []
    for filename, channels, platform in (
            ("keeper_death.wav", 1, "Windows"),
            ("keeper_death_core.wav", 2, "Android")):
        path = AUDIO / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        write_wave(path, output, channels)
        cues.append(dict(
            path=path.relative_to(ROOT).as_posix(), platform=platform,
            channels=channels, sampleRate=SAMPLE_RATE, bitsPerSample=16,
            frames=len(output), durationSeconds=round(len(output) / SAMPLE_RATE, 8),
            fadeInSeconds=FADE_IN_FRAMES / SAMPLE_RATE,
            fadeOutSeconds=FADE_OUT_FRAMES / SAMPLE_RATE,
            processingGain=1.0, outputPeak=round(peak, 8), outputRms=round(output_rms, 8),
            bytes=path.stat().st_size,
            sha256=hashlib.sha256(path.read_bytes()).hexdigest()))

    manifest = dict(
        schema=1,
        source=dict(
            filename=args.source.name, sha256=source_hash, includedInRepository=False,
            title=SOURCE_TITLE, contributor=SOURCE_CONTRIBUTOR,
            published=SOURCE_PUBLISHED, url=SOURCE_URL,
            license=LICENSE, licenseTermsUrl=LICENSE_URL,
            usageTerms=("Game-integrated and adapted use is permitted under the Pixabay Content License; "
                        "standalone redistribution of the source content is prohibited.")),
        decodedSource=dict(sampleRate=SAMPLE_RATE, channels=1, frames=len(decoded),
                           durationSeconds=round(len(decoded) / SAMPLE_RATE, 8)),
        processing=dict(
            tool="FFmpeg decode to mono 48 kHz PCM16, followed by deterministic linear endpoint fades",
            fadeInSeconds=FADE_IN_FRAMES / SAMPLE_RATE,
            fadeOutSeconds=FADE_OUT_FRAMES / SAMPLE_RATE,
            gain=1.0,
            reason="Retains the complete short scream; 10 ms onset and 30 ms quiet-tail fades prevent edge clicks.",
            androidLayout="Dual mono: each output frame is duplicated to left and right channels."),
        ffmpegVersion=subprocess.run([ffmpeg, "-version"], check=True,
                                     capture_output=True, text=True).stdout.splitlines()[0],
        cues=cues, ownerListeningApproval=False)
    manifest_path = AUDIO / "keeper-death.manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8",
                             newline="\n")


if __name__ == "__main__":
    main()
