#!/usr/bin/env python3
"""Compose retained Horde music PCM prototypes for offline loop-tail analysis.

No score rendering or audio processing: source PCM16 frames are converted to
IEEE-float WAV. For A/B/D/E/F/H, each new 12s body begins on its exact frame;
the prior exact 3s tail is sample-added over that body's opening 3s. The final
body's tail remains appended. C/G are each one unchanged body followed by tail.
"""
from __future__ import annotations

import array
import gzip
import hashlib
import json
import math
import struct
import sys
import wave
from pathlib import Path

RATE = 48_000
CHANNELS = 2
BODY_12 = 576_000
BODY_C = 144_000
BODY_G = 288_000
TAIL = 144_000
CYCLES = 20
CHUNK_SAMPLES = 32_768
SOURCE_REL = Path("docs/evidence/2026-10-01-music-render")
LOOPS = ("A", "B", "D", "E", "F", "H")
ONESHOTS = ("C", "G")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def load_pcm(path: Path, expected_frames: int) -> tuple[array.array, dict]:
    with wave.open(str(path), "rb") as w:
        params = w.getparams()
        if (params.nchannels, params.sampwidth, params.framerate, params.comptype) != (2, 2, RATE, "NONE"):
            raise RuntimeError(f"unexpected source WAV format: {path} {params}")
        if params.nframes != expected_frames:
            raise RuntimeError(f"unexpected source frame count: {path} {params.nframes}")
        raw = w.readframes(params.nframes)
    samples = array.array("h")
    samples.frombytes(raw)
    if sys.byteorder != "little":
        samples.byteswap()
    peak = max(abs(x) for x in samples) / 32768.0
    return samples, {
        "path": str(path.relative_to(path.parents[3])).replace("\\", "/"),
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
        "frames": expected_frames,
        "channels": CHANNELS,
        "sampleRate": RATE,
        "pcmPeak": peak,
        "fullScaleChannelSamples": sum(1 for x in samples if abs(x) == 32768),
    }


class FloatWaveSink:
    def __init__(self, path: Path, total_frames: int):
        self.path = path
        self.total_frames = total_frames
        self.f = path.open("wb")
        data_bytes = total_frames * CHANNELS * 4
        if data_bytes > 0xFFFFFFFF - 36:
            raise RuntimeError("RIFF output exceeds 32-bit size")
        self.f.write(b"RIFF" + struct.pack("<I", 36 + data_bytes) + b"WAVE")
        self.f.write(b"fmt " + struct.pack("<IHHIIHH", 16, 3, CHANNELS, RATE,
                                            RATE * CHANNELS * 4, CHANNELS * 4, 32))
        self.f.write(b"data" + struct.pack("<I", data_bytes))
        self.written_frames = 0
        self.prev = None
        self.boundaries = []
        self.global_peak = 0.0
        self.overrange_channel_samples = 0
        self.window_peak = 0.0
        self.window_overrange = 0
        self.window_frames = 0
        self.window_start_frame = 0
        self.windows = []

    def write(self, samples: array.array, boundary: str | None = None):
        if samples.typecode != "f" or len(samples) % CHANNELS:
            raise RuntimeError("internal float block shape error")
        if boundary is not None and self.prev is not None and len(samples):
            first = (samples[0], samples[1])
            self.boundaries.append({
                "label": boundary,
                "frame": self.written_frames,
                "seconds": self.written_frames / RATE,
                "maxAbsAdjacentDelta": max(abs(first[c] - self.prev[c]) for c in range(CHANNELS)),
            })
        self.f.write(samples.tobytes())
        for i in range(0, len(samples), CHANNELS):
            left, right = samples[i], samples[i + 1]
            frame_peak = max(abs(left), abs(right))
            self.global_peak = max(self.global_peak, frame_peak)
            over = int(abs(left) > 1.0) + int(abs(right) > 1.0)
            self.overrange_channel_samples += over
            self.window_peak = max(self.window_peak, frame_peak)
            self.window_overrange += over
            self.window_frames += 1
            if self.window_frames == RATE:
                self.windows.append({
                    "startFrame": self.window_start_frame,
                    "peak": self.window_peak,
                    "overrangeChannelSamples": self.window_overrange,
                })
                self.window_start_frame += RATE
                self.window_peak = 0.0
                self.window_overrange = 0
                self.window_frames = 0
        self.written_frames += len(samples) // CHANNELS
        self.prev = (samples[-2], samples[-1]) if len(samples) else self.prev

    def close(self):
        if self.window_frames:
            self.windows.append({
                "startFrame": self.window_start_frame,
                "peak": self.window_peak,
                "overrangeChannelSamples": self.window_overrange,
            })
        self.f.close()
        if self.written_frames != self.total_frames:
            raise RuntimeError(f"output frame mismatch {self.written_frames}/{self.total_frames}")
        return {
            "output": self.path.name,
            "outputSha256": sha256(self.path),
            "outputBytes": self.path.stat().st_size,
            "format": "stereo 48 kHz IEEE-float32 WAV; unnormalised additive mix",
            "frames": self.written_frames,
            "durationSeconds": self.written_frames / RATE,
            "globalPeak": self.global_peak,
            "peakAboveFullScale": max(0.0, self.global_peak - 1.0),
            "overrangeChannelSamples": self.overrange_channel_samples,
            "rollingOneSecondWindows": len(self.windows),
            "maxOneSecondPeak": max(w["peak"] for w in self.windows),
            "maxOneSecondWindow": max(self.windows, key=lambda w: w["peak"]),
            "boundaries": self.boundaries,
        }


def convert_block(samples: array.array, start: int, end: int) -> array.array:
    return array.array("f", (samples[i] / 32768.0 for i in range(start, end)))


def mixed_block(left: array.array, left_start: int,
                right: array.array, right_start: int, count: int) -> array.array:
    return array.array("f", ((left[left_start + i] + right[right_start + i]) / 32768.0
                              for i in range(count)))


def write_source(sink: FloatWaveSink, samples: array.array, boundary: str | None = None):
    for start in range(0, len(samples), CHUNK_SAMPLES):
        end = min(len(samples), start + CHUNK_SAMPLES)
        sink.write(convert_block(samples, start, end), boundary if start == 0 else None)


def write_mix(sink: FloatWaveSink, a: array.array, b: array.array,
              start_frame: int, count_frames: int, boundary: str | None = None):
    start = start_frame * CHANNELS
    total = count_frames * CHANNELS
    for offset in range(0, total, CHUNK_SAMPLES):
        count = min(CHUNK_SAMPLES, total - offset)
        # Keep stereo channel pairs aligned at every block boundary.
        count -= count % CHANNELS
        sink.write(mixed_block(a, start + offset, b, start + offset, count),
                   boundary if offset == 0 else None)


def source_delta(a: array.array, b: array.array, frame_a: int, frame_b: int) -> float:
    ia, ib = frame_a * CHANNELS, frame_b * CHANNELS
    return max(abs(a[ia + c] - b[ib + c]) for c in range(CHANNELS)) / 32768.0


def build_compositions(repo: Path, out: Path):
    source = repo / SOURCE_REL
    audio = source / "audio"
    source_manifest = json.loads((source / "SHA256SUMS.json").read_text(encoding="utf-8"))
    expected_hashes = {row["path"]: row["sha256"] for row in source_manifest["files"]}
    all_inputs = {}
    results = []

    for cue in LOOPS + ONESHOTS:
        body_frames = BODY_C if cue == "C" else BODY_G if cue == "G" else BODY_12
        body, body_meta = load_pcm(audio / f"{cue}-loop.wav", body_frames)
        tail, tail_meta = load_pcm(audio / f"{cue}-tail.wav", TAIL)
        for meta in (body_meta, tail_meta):
            rel = f"audio/{Path(meta['path']).name}"
            if expected_hashes.get(rel) != meta["sha256"]:
                raise RuntimeError(f"source manifest hash mismatch: {rel}")
            all_inputs[rel] = meta

        if cue in LOOPS:
            overlap_body_peak = max(abs(x) for x in body[:TAIL * CHANNELS]) / 32768.0
            overlap_mix_peak = 0.0
            overlap_max_sample_peak_increase = 0.0
            overlap_max_sample_peak_increase_db = 0.0
            for i in range(TAIL * CHANNELS):
                body_sample = body[i] / 32768.0
                mixed_sample = (body[i] + tail[i]) / 32768.0
                overlap_mix_peak = max(overlap_mix_peak, abs(mixed_sample))
                increase = abs(mixed_sample) - abs(body_sample)
                overlap_max_sample_peak_increase = max(overlap_max_sample_peak_increase, increase)
                if abs(body_sample) > 0 and increase > 0:
                    overlap_max_sample_peak_increase_db = max(
                        overlap_max_sample_peak_increase_db,
                        20.0 * math.log10(abs(mixed_sample) / abs(body_sample)))
            output_frames = CYCLES * BODY_12 + TAIL
            sink = FloatWaveSink(out / f"{cue}-20cycle-tail-overlap-f32.wav", output_frames)
            write_source(sink, body)
            timeline = [{"kind": "body-start", "cycle": 0, "frame": 0, "seconds": 0.0}]
            for cycle in range(1, CYCLES):
                # The 3s prior tail is unchanged and sample-added over the exact
                # first 3s of the next 12s body. This preserves phase/audio rate;
                # it is not equal-power fading or a normalized mix.
                write_mix(sink, tail, body, 0, TAIL,
                          f"cycle-{cycle}-start: prior-tail + new-body")
                write_source_range(sink, body, TAIL, BODY_12,
                                   f"cycle-{cycle}-tail-off: body-only resumes")
                timeline.append({
                    "kind": "body-start-and-tail-overlap",
                    "cycle": cycle,
                    "frame": cycle * BODY_12,
                    "seconds": cycle * 12.0,
                    "overlapFrames": TAIL,
                    "overlapSeconds": TAIL / RATE,
                })
            final_tail_frame = CYCLES * BODY_12
            write_source(sink, tail,
                         f"final-tail-start-no-next-body")
            timeline.append({
                "kind": "final-tail-start",
                "afterCycle": CYCLES - 1,
                "frame": final_tail_frame,
                "seconds": final_tail_frame / RATE,
                "frames": TAIL,
                "secondsLong": TAIL / RATE,
            })
            output_metrics = sink.close()
            naive_body_peak = body_meta["pcmPeak"]
            overlap_peak_increase_db = (
                20.0 * math.log10(output_metrics["globalPeak"] / naive_body_peak)
                if output_metrics["globalPeak"] > 0.0 and naive_body_peak > 0.0 else 0.0
            )
            results.append({
                "cue": cue,
                "mode": "20 exact body starts; prior 3s tail sample-added over next body's first 3s; final tail appended",
                "bodyStarts": [{"frame": i * BODY_12, "seconds": i * 12.0} for i in range(CYCLES)],
                "overlapStartFrames": [(i + 1) * BODY_12 for i in range(CYCLES - 1)],
                "overlapFramesEach": TAIL,
                "tailFrameCount": TAIL,
                "timeline": timeline,
                "sourceBoundaryMaxAbsDelta": {
                    "body-end-to-body-start-for-naive-repeat": source_delta(body, body, BODY_12 - 1, 0),
                    "body-end-to-tail-start": source_delta(body, tail, BODY_12 - 1, 0),
                    "tail-end-to-silence": max(abs(tail[-2]), abs(tail[-1])) / 32768.0,
                },
                "sourcePeaks": {"body": body_meta["pcmPeak"], "tail": tail_meta["pcmPeak"]},
                "overlapPeakIncreaseDbVsBody": overlap_peak_increase_db,
                "overlapWindowPeakComparison": {
                    "bodyPeakSameFirst3s": overlap_body_peak,
                    "mixedPeakSame3s": overlap_mix_peak,
                    "mixedPeakIncrease": max(0.0, overlap_mix_peak - overlap_body_peak),
                    "maxPositivePerChannelSamplePeakIncrease": overlap_max_sample_peak_increase,
                    "maxPositivePerChannelSamplePeakIncreaseDb": overlap_max_sample_peak_increase_db,
                    "tailPeak": tail_meta["pcmPeak"],
                },
                **output_metrics,
            })
        else:
            output_frames = body_frames + TAIL
            sink = FloatWaveSink(out / f"{cue}-single-body-plus-tail-f32.wav", output_frames)
            write_source(sink, body)
            write_source(sink, tail, "body-tail-transition")
            output_metrics = sink.close()
            results.append({
                "cue": cue,
                "mode": "single unchanged body followed by exact 3s tail; no repeat/overlap",
                "timeline": [
                    {"kind": "body-start", "frame": 0, "seconds": 0.0, "frames": body_frames},
                    {"kind": "tail-start", "frame": body_frames, "seconds": body_frames / RATE,
                     "frames": TAIL, "secondsLong": TAIL / RATE},
                ],
                "sourceBoundaryMaxAbsDelta": {
                    "body-end-to-tail-start": source_delta(body, tail, body_frames - 1, 0),
                    "tail-end-to-silence": max(abs(tail[-2]), abs(tail[-1])) / 32768.0,
                },
                "sourcePeaks": {"body": body_meta["pcmPeak"], "tail": tail_meta["pcmPeak"]},
                **output_metrics,
            })

    receipt_path = source / "verification" / "all-cues.json.gz"
    receipt = json.loads(gzip.open(receipt_path, "rt", encoding="utf-8").read())
    g_render = next(row["render"] for row in receipt["renders"] if row["cue"] == "G")
    g_fsharp = g_render["schedule"]["firstFsharp4"]
    if not any(row.get("midi") == 66 and abs(row.get("timeFromMusicalStartSeconds", -1) - 4.5) < 1e-9
               for row in g_fsharp):
        raise RuntimeError("source receipt no longer confirms G first F#4 at 4.5s")

    report = {
        "scope": "External offline prototype composition only; not native playback, listening, runtime admission, or licensing approval.",
        "method": "Unchanged PCM16 input samples converted to float32 and summed linearly for 3s prior-tail/new-body overlap. No fades, gain changes, normalization, resampling, source edits, or score/FX-rate changes.",
        "format": {"input": "stereo 48 kHz PCM16 WAV", "output": "stereo 48 kHz IEEE-float32 WAV", "rate": RATE},
        "source": {
            "root": str(SOURCE_REL).replace("\\", "/"),
            "sha256Manifest": sha256(source / "SHA256SUMS.json"),
            "renderReceiptGzSha256": sha256(receipt_path),
            "ownerArchiveSha256": receipt["source"]["archiveSha256"] if "archiveSha256" in receipt.get("source", {}) else "see source/evidence-scope.json",
            "inputWavs": all_inputs,
        },
        "loopPolicy": {
            "cues": list(LOOPS),
            "cycleFrames": BODY_12,
            "cycleSeconds": BODY_12 / RATE,
            "bodyStarts": CYCLES,
            "bodyStartPeriodDriftFrames": 0,
            "tailFrames": TAIL,
            "tailOverlapFrames": TAIL,
            "steadyOverlapWindows": CYCLES - 1,
            "finalTailAppended": True,
            "framesPerOutput": CYCLES * BODY_12 + TAIL,
            "secondsPerOutput": (CYCLES * BODY_12 + TAIL) / RATE,
        },
        "oneShots": {
            "cues": list(ONESHOTS),
            "bodyTailPolicy": "one full body then one full separate tail; no repetition or overlap",
            "GFirstFsharp4": {"midi": 66, "timeFromMusicalStartSeconds": 4.5,
                               "confirmedUnchangedInSourceReceipt": True},
        },
        "cues": results,
    }
    report_path = out / "loop-tail-composition-metrics.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "report": str(report_path),
        "outputs": [{"path": row["output"], "sha256": row["outputSha256"],
                     "durationSeconds": row["durationSeconds"], "globalPeak": row["globalPeak"],
                     "overrangeChannelSamples": row["overrangeChannelSamples"],
                     "maxOneSecondPeak": row["maxOneSecondPeak"]} for row in results],
        "GFirstFsharp4": report["oneShots"]["GFirstFsharp4"],
    }, indent=2))


def write_source_range(sink: FloatWaveSink, samples: array.array, start_frame: int,
                       end_frame: int, boundary: str | None = None):
    start_sample = start_frame * CHANNELS
    end_sample = end_frame * CHANNELS
    first = True
    for start in range(start_sample, end_sample, CHUNK_SAMPLES):
        end = min(end_sample, start + CHUNK_SAMPLES)
        sink.write(convert_block(samples, start, end), boundary if first else None)
        first = False


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: compose.py REPO_ROOT OUTPUT_DIRECTORY")
    repo_root = Path(sys.argv[1]).resolve()
    output_dir = Path(sys.argv[2]).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    build_compositions(repo_root, output_dir)
