"""Build bounded wet-contact cues from the admitted waterfall and owner MP3.

The owner-supplied MP3 stays outside the repository. Pass its path explicitly;
only the short, mixed-in-game PCM cuts and exact processing receipt are written.
"""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]
AUDIO = ROOT / "assets/audio/pixabay"
WATER_SOURCE = AUDIO / "waterfall_loop.wav"
WATER_SOURCE_HASH = "de7711f0e6ef9cf0bdd3d04ba7a1b713ce09ef18169bd994182b09ab2017ee62"
STEP_SOURCE_HASH = "70f548d544985d6fa7e14463684d3792c51d87da5a987b631d51f3e589f401a9"
STEP_SOURCE_URL = "https://pixabay.com/sound-effects/footsteps-water-01-73731/"
STEP_SOURCE_TITLE = "Footsteps Water 01"
STEP_SOURCE_CONTRIBUTOR = "aglinder (Freesound)"
STEP_SOURCE_LICENSE = "Pixabay Content License"
STEP_SOURCE_LICENSE_URL = "https://pixabay.com/service/terms/"
STEP_SOURCE_USAGE_TERMS = ("Commercial and non-commercial use plus modification/adaptation are permitted; "
                           "standalone distribution is prohibited; attribution is appreciated but not required.")
SAMPLE_RATE = 48000
PLAYER_DRY_GAIN = 0.29
WET_MEAN_EVENT_INTENSITY = 0.525
WET_TARGET_TO_DRY_MIX_RMS = 0.5
# Starts are measured leading edges of separated impact envelopes in the
# supplied recording; each 0.38 s window captures one footfall and its tail.
WET_VARIANTS = (
    ("water_wet_step_1", 32.00),
    ("water_wet_step_2", 32.80),
    ("water_wet_step_3", 33.90),
    ("water_wet_step_4", 34.85),
)


def rms(values):
    return math.sqrt(sum(float(value) * value for value in values) / max(1, len(values))) / 32768.0


def peak(values):
    return max((abs(value) for value in values), default=0) / 32768.0


def source_pcm(path, ffmpeg):
    completed = subprocess.run(
        [ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error", "-i", str(path),
         "-f", "s16le", "-ac", "1", "-ar", str(SAMPLE_RATE), "pipe:1"],
        check=True, capture_output=True)
    samples = array.array("h")
    samples.frombytes(completed.stdout)
    return samples


def read_wave(path):
    with wave.open(str(path), "rb") as source:
        assert source.getparams()[:3] == (1, 2, SAMPLE_RATE)
        samples = array.array("h")
        samples.frombytes(source.readframes(source.getnframes()))
        return samples


def write_platform_cues(name, output, duration, start, gain, raw_rms, receipt, source):
    frames = len(output)
    for platform, channels, suffix in (("Windows", 1, ""), ("Android", 2, "_core")):
        samples = output
        if channels == 2:
            samples = array.array("h", (value for sample in output for value in (sample, sample)))
        destination = AUDIO / (name + suffix + ".wav")
        with wave.open(str(destination), "wb") as target:
            target.setparams((channels, 2, SAMPLE_RATE, frames, "NONE", "not compressed"))
            target.writeframes(samples.tobytes())
        receipt.append(dict(
            path=destination.relative_to(ROOT).as_posix(), platform=platform,
            channels=channels, sourceStartSeconds=start, durationSeconds=duration,
            processingGain=round(gain, 8), preGainRms=round(raw_rms, 8),
            outputRms=round(rms(output), 8), outputPeak=round(peak(output), 8),
            frames=frames, bytes=destination.stat().st_size,
            sha256=hashlib.sha256(destination.read_bytes()).hexdigest(), source=source))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--footsteps-source", required=True, type=Path,
                        help="owner-supplied Pixabay MP3; kept outside this repository")
    parser.add_argument("--ffmpeg", default="ffmpeg")
    args = parser.parse_args()
    ffmpeg = shutil.which(args.ffmpeg)
    if ffmpeg is None:
        raise SystemExit(f"FFmpeg executable not found: {args.ffmpeg}")

    assert hashlib.sha256(WATER_SOURCE.read_bytes()).hexdigest() == WATER_SOURCE_HASH
    step_bytes = args.footsteps_source.read_bytes()
    step_hash = hashlib.sha256(step_bytes).hexdigest()
    assert step_hash == STEP_SOURCE_HASH, "The supplied owner MP3 does not match the admitted source receipt."
    step_samples = source_pcm(args.footsteps_source, ffmpeg)
    with wave.open(str(WATER_SOURCE), "rb") as source:
        assert source.getparams()[:3] == (1, 2, SAMPLE_RATE)
        water_samples = array.array("h", source.readframes(source.getnframes()))

    dry_references = [
        ROOT / "assets/audio/filmcow/player_step_1.wav",
        ROOT / "assets/audio/filmcow/player_step_2.wav",
    ]
    dry_rms = [rms(read_wave(path)) for path in dry_references]
    dry_mean_rms = sum(dry_rms) / len(dry_rms)
    target_wet_rms = (dry_mean_rms * PLAYER_DRY_GAIN * WET_TARGET_TO_DRY_MIX_RMS /
                      WET_MEAN_EVENT_INTENSITY)

    cues = []
    step_source = dict(filename=args.footsteps_source.name, sha256=step_hash,
                       url=STEP_SOURCE_URL, title=STEP_SOURCE_TITLE,
                       contributor=STEP_SOURCE_CONTRIBUTOR, license=STEP_SOURCE_LICENSE,
                       licenseTermsUrl=STEP_SOURCE_LICENSE_URL, usageTerms=STEP_SOURCE_USAGE_TERMS,
                       originalSourceUrl="https://freesound.org/people/aglinder/sounds/265582/",
                       originalSourceLicense="CC0 1.0 (as stated on the Freesound source page)")
    for name, start in WET_VARIANTS:
        duration = .38
        frames = int(duration * SAMPLE_RATE)
        offset = int(start * SAMPLE_RATE)
        section = step_samples[offset:offset + frames]
        if len(section) != frames:
            raise ValueError(f"Short source window for {name} at {start}s")
        faded = []
        for index, value in enumerate(section):
            envelope = min(1.0, index / 480.0, (frames - 1 - index) / 1920.0)
            faded.append(value * max(0.0, envelope))
        pre_gain_rms = rms(faded)
        gain = target_wet_rms / pre_gain_rms
        output = array.array("h")
        for value in faded:
            result = round(value * gain)
            if abs(result) >= 32767:
                raise ValueError(f"Clipping risk in {name}: {result}")
            output.append(result)
        if peak(output) > 0.45 or abs(rms(output) - target_wet_rms) > target_wet_rms * 0.01:
            raise ValueError(f"Unsafe or out-of-target cue level in {name}")
        write_platform_cues(name, output, duration, start, gain, pre_gain_rms,
                            cues, step_source)

    # Keep the existing waterfall-entry cue and its original admitted source.
    name, start, duration, gain = "water_stream_contact", 13.65, .35, .70
    frames, offset = int(duration * SAMPLE_RATE), int(start * SAMPLE_RATE)
    output = array.array("h")
    for index, value in enumerate(water_samples[offset:offset + frames]):
        envelope = min(1.0, index / 480.0, (frames - 1 - index) / 1440.0)
        output.append(round(value * gain * max(0.0, envelope)))
    write_platform_cues(name, output, duration, start, gain, rms(output), cues,
                        dict(filename=WATER_SOURCE.name, sha256=WATER_SOURCE_HASH,
                             license="Existing DRAGON-STUDIO/Pixabay Content License"))

    manifest = dict(
        schema=2,
        sources=[
            dict(filename=WATER_SOURCE.name, sha256=WATER_SOURCE_HASH,
                 title="Water Dripping 364450", contributor="DRAGON-STUDIO",
                 url="https://pixabay.com/sound-effects/nature-water-dripping-364450/",
                 license="Pixabay Content License"),
            dict(**step_source, includedInRepository=False),
        ],
        sampleRate=SAMPLE_RATE, bitsPerSample=16, cues=cues,
        levelMatch=dict(
            dryReferencePaths=[path.relative_to(ROOT).as_posix() for path in dry_references],
            dryReferenceRms=[round(value, 8) for value in dry_rms],
            dryPlayerMixGain=PLAYER_DRY_GAIN, wetMeanEventIntensity=WET_MEAN_EVENT_INTENSITY,
            targetWetToDryFinalRms=WET_TARGET_TO_DRY_MIX_RMS,
            targetWetCueRmsBeforeEventGain=round(target_wet_rms, 8),
            peakLimit=0.45),
        processing=("Wet variants: mono FFmpeg decode to 48 kHz PCM16; four fixed 0.38 s source cuts; "
                    "10 ms fade-in, 40 ms fade-out; measured RMS match to half the dry cue's final "
                    "RMS at mean wet event intensity; peak ceiling 0.45; dual-mono Android copies. "
                    "No limiter or clipping; owner MP3 remains outside repository. Stream cue retains "
                    "the previous fixed waterfall-source cut and gain."),
        ffmpegVersion=subprocess.run([ffmpeg, "-version"], check=True,
                                     capture_output=True, text=True).stdout.splitlines()[0],
        license=STEP_SOURCE_LICENSE,
        ownerListeningApproval=False)
    manifest_path = AUDIO / "water-contact.manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", newline="\n")


if __name__ == "__main__":
    main()
