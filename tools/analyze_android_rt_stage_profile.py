#!/usr/bin/env python3
"""Join optional GPU stage observations to an admitted Android motion report.

Intervals describe execution dependencies, not additive AS/trace/copy counters.
The normal strict motion/display analyzer remains the authority for route timing.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
import math
from pathlib import Path
import statistics

from analyze_android_motion_present_timing import AnalysisError, analyze_bytes, MAX_REPORT_BYTES

STAGES = ("as", "dispatch", "transfer")
VIEWS = ("landing-sweep", "lower-sweep", "passage-sweep", "repeat-ascent")


def summary(values: list[int]) -> dict:
    ordered = sorted(values)
    return {"n": len(ordered), "medianMs": statistics.median(ordered) / 1e6,
            "p95Ms": ordered[math.ceil(.95 * len(ordered)) - 1] / 1e6,
            "maximumMs": ordered[-1] / 1e6}


def unsigned(value: str | None, label: str) -> int:
    if not isinstance(value, str) or not value.isascii() or not value.isdecimal():
        raise AnalysisError(f"{label}: expected unsigned integer")
    number = int(value)
    if number > (1 << 64) - 1:
        raise AnalysisError(f"{label}: exceeds uint64")
    return number


def join_stages(rows: list[dict], frames: list[dict], *, held_views=VIEWS) -> dict:
    """Require every completed frame, including cutovers and the final frame."""
    if not rows or len(rows) != len(frames):
        raise AnalysisError("stage rows must match every completed frame")
    observations = {stage: [] for stage in STAGES}
    last_serial = -1
    for index, (row, frame) in enumerate(zip(rows, frames)):
        serial = unsigned(row.get("serial"), f"row {index} serial")
        if serial != frame["submission"] or serial <= last_serial:
            raise AnalysisError(f"row {index}: duplicate, reordered or wrong owner serial")
        if unsigned(row.get("tick"), f"row {index} tick") != frame["tick"]:
            raise AnalysisError(f"row {index}: wrong simulation tick")
        last_serial = serial
        for stage in STAGES:
            if row.get(stage + "_gpu_status") != "valid":
                raise AnalysisError(f"row {index}: {stage} unavailable or mismatched")
            if unsigned(row.get(stage + "_gpu_serial"), stage + " serial") != serial:
                raise AnalysisError(f"row {index}: {stage} borrowed another submission")
            observations[stage].append(unsigned(row.get(stage + "_gpu_ns"), stage + " ns"))
    views = {}
    for phase in held_views:
        indices = [i for i, row in enumerate(rows) if i and row["phase"] == phase
                   and rows[i-1]["phase"] == phase
                   and 60 <= unsigned(row.get("view_ticks"), "view ticks") <= 300
                   and 60 <= unsigned(rows[i-1].get("view_ticks"), "previous view ticks") <= 300]
        if len(indices) < 20:
            raise AnalysisError(f"{phase}: fewer than 20 settled held-view samples")
        for axis in ("x", "y", "z", "yaw", "pitch"):
            try:
                values = [float(rows[i][axis]) for i in indices]
            except (KeyError, ValueError, TypeError) as error:
                raise AnalysisError(f"{phase}: invalid {axis}") from error
            if not all(math.isfinite(v) for v in values) or max(values)-min(values) >= .0001:
                raise AnalysisError(f"{phase}: camera is not frozen")
        views[phase] = {stage: summary([observations[stage][i] for i in indices]) for stage in STAGES}
    return {"joinedFrames": len(rows), "allFrames": {s: summary(v) for s, v in observations.items()},
            "views": views,
            "limits": "GPU stage execution intervals; not additive counters or playable FPS. "
                      "AS brackets UpdateDynamicInstances; dispatch includes all shader work; "
                      "transfer brackets the post-barrier transfer/copy path."}


def analyze(report: bytes, resource_csv: str) -> dict:
    strict = analyze_bytes(report)
    if len(resource_csv.encode("utf-8")) > MAX_REPORT_BYTES:
        raise AnalysisError("stage CSV exceeds report size limit")
    frames = json.loads(report)["motionEvidence"]["completedRtFrames"]
    result = join_stages(list(csv.DictReader(io.StringIO(resource_csv))), frames)
    result["strictTiming"] = strict
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("resources", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        result = analyze(args.report.read_bytes(), args.resources.read_text(encoding="utf-8"))
    except (AnalysisError, OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Stage profile rejected: {error}\n")
    rendered = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
