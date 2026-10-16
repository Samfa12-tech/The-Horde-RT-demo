"""Admit sparse per-invocation clock observations, never GPU-time percentages."""
import argparse
import csv
import io
import json
from pathlib import Path
import struct

from analyze_android_rt_stage_profile import AnalysisError, VIEWS, analyze as admit_stages, unsigned

CATEGORIES = ("other", "trace", "material", "shadow", "surface", "fire", "mist", "dust")
CAPACITY = 8192


def grid_axis(extent):
    return (extent - 17)//32 + 1 if extent > 16 else 0


def admit_clock_rows(rows, width, height):
    count = grid_axis(width)*grid_axis(height)
    if not 0 < count <= CAPACITY:
        raise AnalysisError("invalid sparse grid extent/capacity")
    last_ticket = 0
    for row in rows:
        if row.get("clock_status") != "valid":
            raise AnalysisError("clock unavailable/mismatched; never substitute zero")
        ticket = unsigned(row.get("clock_ticket"), "clock ticket")
        if ticket <= last_ticket or unsigned(row.get("clock_samples"), "sample count") != count:
            raise AnalysisError("stale ticket or missing sparse samples")
        last_ticket = ticket
        values = [unsigned(row.get("clock_"+c), c) for c in CATEGORIES]
        if not 0 < sum(values) <= (1 << 64)-1:
            raise AnalysisError("unavailable/overflowed clock total")
    if not rows:
        raise AnalysisError("no clock rows")
    return count


def admit_raw(raw, row, width, height):
    count = grid_axis(width)*grid_axis(height)
    if len(raw) != count*80:
        raise AnalysisError("raw clock witness has missing/extra samples")
    ticket = unsigned(row["clock_ticket"], "clock ticket")
    totals = [0]*8
    classes = {}
    for i, values in enumerate(struct.iter_unpack("<20I", raw)):
        if values[:3] != (ticket, (i % grid_axis(width))*32+16, (i // grid_axis(width))*32+16):
            raise AnalysisError("raw clock ticket/pixel mismatch")
        if values[3] > 26:
            raise AnalysisError("invalid primary instance class")
        classes[values[3]] = classes.get(values[3], 0)+1
        elapsed = [values[4+2*c] | (values[5+2*c] << 32) for c in range(8)]
        if not 0 < sum(elapsed) <= (1 << 64)-1:
            raise AnalysisError("invalid raw invocation clock total")
        totals = [a+b for a, b in zip(totals, elapsed)]
    if totals != [unsigned(row["clock_"+c], c) for c in CATEGORIES]:
        raise AnalysisError("raw clock totals do not match owning CSV row")
    return {"samples": count, "primaryInstancePlusOneCounts": classes}


def analyze(report, resources, directory):
    stages = admit_stages(report, resources)
    data = json.loads(report)
    settings = data["settings"]
    width, height = settings["internalWidth"], settings["internalHeight"]
    rows = list(csv.DictReader(io.StringIO(resources)))
    count = admit_clock_rows(rows, width, height)
    views = {}
    for phase in VIEWS:
        selected = [row for i, row in enumerate(rows) if i and row["phase"] == phase
                    and rows[i-1]["phase"] == phase and 60 <= int(row["view_ticks"]) <= 300
                    and 60 <= int(rows[i-1]["view_ticks"]) <= 300]
        totals = {c: sum(unsigned(row["clock_"+c], c) for row in selected) for c in CATEGORIES}
        total = sum(totals.values())
        if not selected or not total:
            raise AnalysisError("empty settled-view clock sample set")
        raw_files = list(directory.glob(f'android-motion-{data["runId"]}-clock-{phase}-*.bin'))
        if len(raw_files) != 1:
            raise AnalysisError("one raw witness required per held view")
        serial = int(raw_files[0].stem.rsplit("-", 1)[1])
        owner = [r for r in selected if int(r["serial"]) == serial]
        if len(owner) != 1:
            raise AnalysisError("raw clock witness not bound to one settled owner")
        views[phase] = {"frames": len(selected), "samplesPerFrame": count,
            "meanSampleTicks": {c: v/(len(selected)*count) for c, v in totals.items()},
            "sampledClockSharePercent": {c: 100*v/total for c, v in totals.items()},
            "rawWitnessSerial": serial, "rawWitness": admit_raw(raw_files[0].read_bytes(), owner[0], width, height)}
    return {"runId": data["runId"], "clockJoinedFrames": len(rows), "views": views,
            "stages": stages,
            "limits": "Implementation-defined device-clock ticks from fixed sparse invocation intervals. "
                      "Nested categories are exclusive for the sampled invocation; sums/fractions are not "
                      "cycles, GPU utilization, whole-dispatch milliseconds or causal baseline cost. "
                      "Includes scheduling/divergence and observer perturbation; requires separate image/overhead admission."}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("resources", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = analyze(args.report.read_bytes(), args.resources.read_text(), args.resources.parent)
        args.output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    except (AnalysisError, ValueError, KeyError, OSError) as error:
        parser.exit(1, f"Dispatch clock profile rejected: {error}\n")
