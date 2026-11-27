"""Admit sparse per-invocation clock observations, never GPU-time percentages."""
import argparse
import csv
import io
import json
import re
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


def admit_raw(raw, row, width, height, *, max_hit_class=26, by_primary_class=False):
    count = grid_axis(width)*grid_axis(height)
    if len(raw) != count*80:
        raise AnalysisError("raw clock witness has missing/extra samples")
    ticket = unsigned(row["clock_ticket"], "clock ticket")
    totals = [0]*8
    classes = {}
    class_ticks = {}
    for i, values in enumerate(struct.iter_unpack("<20I", raw)):
        if values[:3] != (ticket, (i % grid_axis(width))*32+16, (i // grid_axis(width))*32+16):
            raise AnalysisError("raw clock ticket/pixel mismatch")
        if values[3] > max_hit_class:
            raise AnalysisError("invalid primary instance class")
        classes[values[3]] = classes.get(values[3], 0)+1
        elapsed = [values[4+2*c] | (values[5+2*c] << 32) for c in range(8)]
        if not 0 < sum(elapsed) <= (1 << 64)-1:
            raise AnalysisError("invalid raw invocation clock total")
        totals = [a+b for a, b in zip(totals, elapsed)]
        if by_primary_class:
            group = class_ticks.setdefault(values[3], [0]*len(CATEGORIES))
            for j, ticks in enumerate(elapsed):
                group[j] += ticks
    if totals != [unsigned(row["clock_"+c], c) for c in CATEGORIES]:
        raise AnalysisError("raw clock totals do not match owning CSV row")
    result = {"samples": count, "primaryInstancePlusOneCounts": classes}
    if by_primary_class:
        result["sampledClockTicksByPrimaryClass"] = {
            k: dict(zip(CATEGORIES, values)) for k, values in class_ticks.items()
        }
    return result



def analyze_hearth(directory, shader_sha256, verify_room_fire, resident_ids):
    """Join each first-visit sample to the ordinary Bellwether route recorder."""
    def require(ok, message):
        if not ok:
            raise AnalysisError(message)

    require(re.fullmatch(r"[0-9a-f]{64}", shader_sha256), "Expected the observer shader SHA-256")
    abi = Path(__file__).resolve().parents[1] / "src/vulkan/raytracing/RtSceneAbi.def"
    max_class = json.loads(abi.read_bytes())["capacities"]["instanceMetadata"]
    directory = Path(directory)
    with (directory / "bellwether-west-roundtrip.csv").open(newline="") as stream:
        by_serial = {}
        for row in csv.DictReader(stream):
            serial = unsigned(row["submission_serial"], "submission serial")
            require(serial not in by_serial, "Repeated submission in route CSV")
            by_serial[serial] = row

    witnesses = []
    clocks = []
    run_identity = None
    previous_serial = previous_sample = 0
    json_fields = {
        "scene_epoch": "scene_epoch", "measurement_generation": "generation",
        "record_serial": "record_serial", "simulation_tick": "tick",
        "internal_width": "internal_width", "internal_height": "internal_height",
        "output_width": "output_width", "output_height": "output_height",
    }
    for path in sorted(directory.glob("bellwether-westroundtrip-hearth-clock-*.json")):
        meta = json.loads(path.read_bytes())
        def number(key):
            value = meta[key]
            require(type(value) is int and 0 < value < 2**64, f"Invalid witness {key}")
            return value

        require(meta["format"] == "horde-rt-dispatch-clock-v1" and meta["route"] == "west-roundtrip" and
                meta["room_id"] == "bellwether_tavern_staffed_hearth" and meta["phase"] == "room" and
                number("measurement_checkpoint_id") == 349 and number("checkpoint_id") == 4 and
                number("return_index") == 1, "Expected first staffed-hearth visit")
        serial = number("submission_serial")
        require(serial in by_serial and serial > previous_serial, "Missing or stale owning submission")
        row = by_serial[serial]
        require(all(number(k) == unsigned(row[col], col) for k, col in json_fields.items()),
                "Clock frame differs from the route frame")
        require(number("presented_serial") == number("completion_serial") == serial and
                all(unsigned(row[k], k) == serial for k in
                    ("present_serial", "completion_serial", "gpu_serial", "gpu_stage_serial")),
                "Clock does not join completed GPU work and successful presentation")
        require(meta["pipeline_sha256"] == row["shader_sha256"] == shader_sha256 and
                meta["execution_mode"] == row["backend"] and
                row["backend"] in ("RayTracingPipeline", "RayQueryCompute"), "Observer pipeline mismatch")
        require(row["phase"] == "room" and row["returns"] == "0" and
                all(row[k] == "1" for k in ("kit_mask", "resident_mask", "record_keeper_mask", "tavern_door_mask")) and
                row["glass_enabled"] == row["window_mask"] in ("0", "1") and
                int(row["resident_slot0_id"]) == resident_ids["TavernKeeper"] and
                int(row["resident_slot1_id"]) == resident_ids["RecordKeeper"], "Staffed room roster mismatch")
        verify_room_fire(row)
        index = number("sample_index")
        require(previous_sample < index <= 3, "Repeated or invalid dwell window")
        dwell = number("room_dwell_ticks")
        first_tick = (300, 360, 420)[index - 1]
        require(first_tick <= dwell <= first_tick + 30, "Clock outside its settled dwell window")
        run_ns = unsigned(row["present_return_ns"], "present return") - unsigned(row["elapsed_ns"], "elapsed")
        stem = f"bellwether-westroundtrip-hearth-clock-{run_ns}-return-1-sample-{index}-{serial}"
        require(run_ns > 0 and path.stem == stem and meta["raw_file"] == stem + ".bin",
                "Clock raw/metadata files do not belong to this run and sample")
        identity = (run_ns, row["scene_epoch"], row["generation"], row["backend"], row["station"],
                    number("simulation_tick") - dwell, row["glass_enabled"], row["fire_ids"],
                    row["internal_width"], row["internal_height"], row["output_width"], row["output_height"])
        if run_identity is None:
            run_identity = identity
        require(identity == run_identity, "Mixed runs, room views or graphics settings")
        require(number("record_size_bytes") == 80 and tuple(meta["category_order"]) == CATEGORIES and
                len(meta["category_ticks"]) == len(CATEGORIES), "Clock record ABI mismatch")
        totals = meta["category_ticks"]
        require(all(type(v) is int and 0 <= v < 2**64 for v in totals), "Invalid sampled ticks")
        clock = dict(clock_status="valid", clock_ticket=str(number("clock_ticket")),
                     clock_samples=str(number("record_count")))
        clock.update({"clock_" + k: str(v) for k, v in zip(CATEGORIES, totals)})
        clocks.append(clock)
        width, height = number("internal_width"), number("internal_height")
        admit_clock_rows(clocks, width, height)
        raw = admit_raw(path.with_suffix(".bin").read_bytes(), clock, width, height, max_hit_class=max_class, by_primary_class=True)
        gpu_ns = unsigned(row["gpu_ns"], "GPU time")
        dispatch_ns = unsigned(row["gpu_dispatch_ns"], "dispatch time")
        require(0 < dispatch_ns <= gpu_ns, "Invalid GPU dispatch interval")
        witnesses.append({"sample": index, "submission": serial, "simulationTick": number("simulation_tick"),
                          "dwellTicks": dwell, "clockTicket": number("clock_ticket"), **raw,
                          "sampledClockTicks": dict(zip(CATEGORIES, totals)),
                          "sampledClockShare": {k: v/sum(totals) for k, v in zip(CATEGORIES, totals)},
                          "instrumentedGpuMs": gpu_ns/1e6, "instrumentedDispatchMs": dispatch_ns/1e6})
        previous_sample, previous_serial = index, serial

    require(witnesses, "No verified staffed-hearth clock witnesses")
    return {"shaderSha256": shader_sha256, "backend": run_identity[3],
            "completeSampleSet": len(witnesses) == 3, "witnesses": witnesses,
            "units": "Device-clock ticks aggregated over a sparse pixel grid. Shares describe sampled invocation time only."}

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
