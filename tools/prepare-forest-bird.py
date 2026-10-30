"""Reproduce one candidate from the existing private FilmCow archive. No download."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

SOURCE_SHA = "eeec1e8ebae3af6395640fb0086cb2ebf006724efbb4859e93fb9c5b4a9de8a4"
RUNTIME_SHA = "811c2a1d10a565615dd786ef4b99fda5dd0e4465aa121e36ad9158531d6902ff"

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    if digest(args.source) != SOURCE_SHA:
        raise SystemExit("Unexpected FilmCow source digest; source preserved")
    if not args.output_directory.is_dir() or any(args.output_directory.iterdir()):
        raise SystemExit("Caller must provide an existing empty output directory; nothing overwritten")
    target = args.output_directory / "forest_distant_bird.wav"
    subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-n", "-i", str(args.source),
                    "-map_metadata", "-1", "-ac", "1", "-ar", "48000", "-c:a", "pcm_s16le", str(target)], check=True)
    if digest(target) != RUNTIME_SHA:
        raise SystemExit("Candidate differs from inspected FFmpeg 9 output; preserve and review")
    receipt = dict(schema=1, asset="generic distant forest bird candidate", creator="FilmCow",
        sourceFile="FilmCow Recorded SFX/distand bird.wav", sourceSha256=SOURCE_SHA,
        sourceUrl="https://filmcow.itch.io/filmcow-sfx", license="FilmCow custom royalty-free project-use terms; not CC0",
        terms="Personal/commercial project use permitted; national-government, law-enforcement, SPLC/CAHN-designated hate-group uses prohibited. No authorship claim or standalone audio-library redistribution.",
        sourcePolicy="Private original remains outside Git and game packages; runtime derivative only",
        runtimeFile=target.name, runtimeSha256=RUNTIME_SHA, runtimeBytes=target.stat().st_size,
        channels=1, sampleRate=48000, bitsPerSample=16, frames=397831, durationSeconds=397831/48000,
        processing="FFmpeg 9 PCM24 to PCM16 only; no gain, pitch, filtering, crop, fade, looping or resampling",
        meanDbfs=-49.4, peakDbfs=-28.6,
        acceptance="Candidate pending listening/species/night suitability and native mix acceptance; no device playback claim")
    (args.output_directory / "forest-distant-bird.manifest.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8")
