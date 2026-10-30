#!/usr/bin/env python3
"""Strict offline evidence checks for the frozen Keeper/mist diagnostic fixture.

These checks do not turn synthetic tests or a serialized workload into physical
validation, moving-combat performance, sustained FPS, scanout or input latency.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import sys

MAX_BYTES = 64 * 1024 * 1024
U64_MAX = (1 << 64) - 1
STAGES = (
    "simulationStepCpuMs", "skinCpuMs", "playerSkinCpuMs", "characterSkinCpuMs",
    "dynamicUploadCpuMs", "blasRefitRecordCpuMs", "tlasUpdateRecordCpuMs",
    "traceCopyRecordCpuMs", "frameFenceWaitCpuMs", "imageAcquireCpuMs",
    "queueSubmitCpuMs", "presentCallCpuMs", "wholeFrameCycleCpuMs", "poseCopyCpuMs",
)
LOSS_COUNTERS = (
    "rejectedPresents", "invalidRegistrations", "invalidMetadata", "pendingCapacityExhausted",
    "queryErrors", "zeroPresentTimestamps", "zeroPresentIds", "unknownPresentIds",
    "duplicateTimings", "rowCapacityExhausted", "missingOnRebind", "missingOnUnbind",
    "abandonedPreparedPresents",
)
RESOURCE_KEYS = (
    "bufferCount", "memoryAllocationCount", "bottomLevelAccelerationStructureCount",
    "topLevelAccelerationStructureCount", "tlasInstanceCount", "pipelineCount",
    "shaderBindingTableCount", "descriptorSetCount", "hostVisibleBytes", "deviceLocalBytes",
)
LIMITS = (
    "Isolated frozen live-reveal renderer workload with per-frame device idle/serialization; "
    "not moving combat, gameplay FPS, sustained performance, scanout or input latency. "
    "GPU, CPU stages and diagnostic actual image-presentation intervals are separate metrics; "
    "never add their durations or infer gameplay FPS. Camera is the frozen native manifest "
    "anchor; canonical frame JSON does not independently serialize each camera. "
    "This analyzer validates supplied evidence; synthetic tests are not physical validation."
)


class AnalysisError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise AnalysisError(message)


def integer(value, name, minimum=0, maximum=U64_MAX):
    require(type(value) is int and minimum <= value <= maximum, name + ": invalid integer")
    return value


def number(value, name, minimum=None):
    require(type(value) in (int, float) and math.isfinite(value), name + ": nonfinite/non-numeric")
    require(minimum is None or value >= minimum, name + ": negative value")
    return value


def object_(value, name):
    require(type(value) is dict, name + ": missing object")
    return value


def vector(value, length, name):
    require(type(value) is list and len(value) == length, name + ": wrong vector length")
    for x in value:
        number(x, name)
    return value


def exact(value, expected, name):
    require(type(value) is type(expected) and value == expected, name + ": unexpected value")


def load_json(raw):
    require(len(raw) <= MAX_BYTES, "JSON exceeds 64 MiB bound")

    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, "duplicate JSON key: " + key)
            result[key] = value
        return result

    def bad_constant(value):
        raise AnalysisError("nonfinite JSON constant: " + value)

    try:
        return json.loads(raw, object_pairs_hook=pairs, parse_constant=bad_constant)
    except (ValueError, UnicodeError) as error:
        raise AnalysisError(str(error)) from error


def stats(values):
    ordered = sorted(values)
    require(bool(ordered), "empty statistics")
    return {"n": len(ordered), "medianMs": statistics.median(ordered),
            "p95Ms": ordered[math.ceil(.95 * len(ordered)) - 1], "maximumMs": ordered[-1]}


def graphics_settings(report):
    graphics = object_(report.get("graphics"), "graphics")
    require(set(graphics) == {"scalePercent", "water", "fire", "frameCap", "glass", "shadow", "mist", "dust"},
            "graphics: exact complete tuple required")
    exact(graphics["scalePercent"], 40, "graphics.scalePercent")
    for key in ("water", "fire", "dust"):
        integer(graphics[key], "graphics." + key, 0, 2)
    integer(graphics["shadow"], "graphics.shadow", 1, 2)
    integer(graphics["frameCap"], "graphics.frameCap", 15, 60)
    for key in ("glass", "mist"):
        require(type(graphics[key]) is bool, "graphics." + key + ": boolean required")
    exact(report.get("scale"), graphics["scalePercent"], "scale")
    exact(report.get("mist"), graphics["mist"], "mist")
    for key, expected in (("water", 1), ("fire", 0), ("frameCap", 30), ("glass", False), ("shadow", 1), ("dust", 1)):
        exact(graphics[key], expected, "declared graphics." + key)
    return graphics


def pipeline_identity(frame, graphics):
    pipeline = object_(frame.get("pipeline"), "pipeline")
    # This experiment is declared Shipping/Mobile on the RT pipeline. A real
    # alternative RT backend or Diagnostic/High build is a different experiment.
    exact(pipeline.get("executionBackend"), "RayTracingPipeline", "declared RT backend")
    exact(pipeline.get("instrumentation"), "shipping", "declared instrumentation")
    exact(pipeline.get("dielectricQuality"), "mobile", "declared compiled quality")
    exact(pipeline.get("waterQuality"), ("off", "mobile", "high")[graphics["water"]], "pipeline water")
    for key in ("bundleKey", "activeKey"):
        require(type(pipeline.get(key)) is str and pipeline[key] and
                all(32 <= ord(c) <= 126 for c in pipeline[key]), "missing/noncanonical pipeline " + key)
    for key in ("opaqueFast", "genericDielectric"):
        artifact = object_(pipeline.get(key), key)
        require(type(artifact.get("key")) is str and artifact["key"] and
                all(32 <= ord(c) <= 126 for c in artifact["key"]), "missing/noncanonical artifact key")
        require(type(artifact.get("sha256")) is str and re.fullmatch(r"[0-9a-f]{64}", artifact["sha256"]), "invalid artifact hash")
    require(pipeline.get("activeStrategy") in ("opaque-fast", "generic-dielectric"), "invalid strategy")
    selected = pipeline["opaqueFast" if pipeline["activeStrategy"] == "opaque-fast" else "genericDielectric"]
    exact(pipeline.get("activeKey"), selected["key"], "active artifact key")
    exact(pipeline.get("activeSha256"), selected["sha256"], "active artifact hash")
    return pipeline


def light_evidence(frame):
    lights = object_(frame.get("fireLighting"), "fireLighting")
    exact(lights.get("count"), 2, "uploaded light count")
    emitters = lights.get("emitters")
    require(type(emitters) is list and len(emitters) == 4, "canonical four-slot light upload required")
    for index, emitter in enumerate(emitters):
        object_(emitter, "emitter")
        position = vector(emitter.get("positionStrength"), 4, "light.positionStrength")
        colour = vector(emitter.get("colourIntensity"), 4, "light.colourIntensity")
        exact(emitter.get("stableId"), (3, 4, 0, 0)[index], "production light ID")
        if index < 2:
            require(position[3] > 0 and colour[3] == position[3] and min(colour) >= 0 and max(colour[:3]) > 0,
                    "invalid active uploaded light")
        else:
            require(position == [0, 0, 0, 0] and colour == [0, 0, 0, 0], "nonzero unused light slot")
    return lights


def validate_frame(frame, graphics, tick):
    object_(frame, "completedFrame")
    exact(frame.get("schema"), 1, "canonical schema")
    identity = object_(frame.get("identity"), "identity")
    for key in ("sceneEpoch", "measurementGeneration", "recordAttemptSerial", "recordSerial", "submissionSerial", "completionSerial"):
        integer(identity.get(key), "identity." + key, 1)
    exact(identity.get("simulationTick"), tick, "frozen simulation tick")
    exact(identity.get("frameSlot"), 0, "sole frame slot")
    pipeline = pipeline_identity(frame, graphics)
    dispatch = object_(frame.get("dispatch"), "dispatch")
    for key in ("sceneReady", "rtDispatchRecorded", "swapchainCopyRecorded"):
        exact(dispatch.get(key), True, "dispatch." + key)
    presentation = object_(frame.get("presentation"), "owning presentation")
    exact(presentation.get("outcome"), "presented", "presentation outcome")
    exact(presentation.get("presented"), True, "presentation presented")
    exact(presentation.get("lastSuccessfulPresentSubmissionSerial"), identity["submissionSerial"], "owning presentation serial")
    exact(presentation.get("finalIdleCompletion"), True, "owning device-idle completion")
    gpu = object_(frame.get("gpu"), "gpu")
    exact(gpu.get("status"), "valid", "GPU status")
    exact(gpu.get("available"), True, "GPU available")
    exact(gpu.get("completedSubmissionSerial"), identity["submissionSerial"], "owning GPU serial")
    require(number(gpu.get("wholeRtGpuMs"), "wholeRtGpuMs", 0) > 0, "positive GPU duration required")
    integer(gpu.get("timestampValidBits"), "timestamp bits", 1, 64)
    integer(gpu.get("timestampPeriodPicoseconds"), "timestamp period", 1)
    integer(gpu.get("sampleCount"), "GPU sample count", 1)
    for key in ("errorCount", "unavailableResultCount"):
        exact(gpu.get(key), 0, "GPU " + key)
    for key in ("cpuBenchmarkEligible", "benchmarkEligible"):
        exact(frame.get(key), True, key)
    dielectric = object_(frame.get("dielectric"), "dielectric")
    for key, value in (("status", "compiled-out"), ("available", False), ("compiled", False),
                       ("completedSubmissionSerial", 0), ("readCount", 0), ("resetCount", 0), ("counters", None)):
        require(key in dielectric, "missing shipping diagnostic " + key)
        exact(dielectric.get(key), value, "shipping diagnostic " + key)
    stages = object_(frame.get("stages"), "stages")
    exact(stages.get("status"), "valid", "CPU stage status")
    for name in STAGES:
        number(stages.get(name), name, 0)
    require(stages["wholeFrameCycleCpuMs"] > 0, "positive whole CPU duration required")
    require(abs(stages["skinCpuMs"] - stages["playerSkinCpuMs"] - stages["characterSkinCpuMs"]) <= .0000015,
            "CPU skin aggregate mismatch")
    for key in ("workInvocationCounts", "byteCounts", "operationCounts"):
        values = stages.get(key)
        require(type(values) is list and len(values) == len(STAGES), "canonical stage count-array length")
        for value in values:
            integer(value, key)
        require(values[1] == values[2] + values[3], "skin count aggregate mismatch")
    exact(frame.get("actualUploadedMistEnabled"), graphics["mist"], "owning uploaded Mist")
    exact(frame.get("actualUploadedDustQuality"), graphics["dust"], "owning uploaded Dust")
    fire = object_(frame.get("fireQuality"), "fireQuality")
    fire_name, steps, reflection = (("Mobile", 4, 1), ("High", 10, 2), ("Low", 2, 1))[graphics["fire"]]
    for key, value in (("quality", fire_name), ("volumeSteps", steps), ("reflectionSamples", reflection),
                       ("reflectedVolumeSteps", min(steps, reflection * 4))):
        exact(fire.get(key), value, "uploaded fire " + key)
    shadow = object_(frame.get("shadowQuality"), "shadowQuality")
    exact(shadow.get("mode"), ("current", "higher")[graphics["shadow"] - 1], "uploaded shadow mode")
    high_samples = 4 if pipeline["dielectricQuality"] == "high" else 2
    for key, value in (("reserved", 0), ("localPrimarySamples", 1 if graphics["shadow"] == 1 else high_samples),
                       ("skyPrimarySamples", 1 if graphics["shadow"] == 1 else 2)):
        exact(shadow.get(key), value, "uploaded shadow " + key)
    resources = object_(frame.get("resources"), "resources")
    for key in RESOURCE_KEYS:
        integer(resources.get(key), "resources." + key)
    for key in ("bottomLevelAccelerationStructureCount", "topLevelAccelerationStructureCount", "tlasInstanceCount", "pipelineCount", "descriptorSetCount"):
        require(resources[key] > 0, "missing real RT resource " + key)
    if pipeline["executionBackend"] == "RayTracingPipeline":
        require(resources["shaderBindingTableCount"] > 0, "RT pipeline requires shader binding table")
    player = object_(frame.get("player"), "player")
    for key in ("skinCadenceHz", "skinUpdateCount", "maximumSocketErrorMicrometres"):
        integer(player.get(key), "player." + key)
    exact(player.get("primaryPixelCountAvailable"), False, "shipping player pixel availability")
    require("primaryPixelCount" in player, "missing canonical player pixel value")
    exact(player["primaryPixelCount"], None, "compiled-out primary player pixels")
    exact(player.get("primaryVisible"), False, "compiled-out player visibility")
    return identity, {"pipeline": pipeline, "lights": light_evidence(frame), "resources": resources,
                      "fireQuality": fire, "shadowQuality": shadow, "dust": graphics["dust"],
                      "gpuClockMetadata": {key: gpu[key] for key in ("timestampValidBits", "timestampPeriodPicoseconds")}}


def thermals_summary(document):
    require(type(document) is list and len(document) >= 2, "thermal phase list required")
    starts, ends, facts = [], [], []
    for entry in document:
        object_(entry, "thermal entry")
        thermal = object_(entry.get("thermal"), "thermal")
        exact(thermal.get("schemaVersion"), 1, "typed current-HAL schema")
        exact(thermal.get("temperatureSource"), "current HAL", "thermal source")
        exact(thermal.get("frameworkStatus"), 0, "framework thermal status")
        sensors = object_(thermal.get("sensors"), "thermal sensors")
        values = {}
        for sensor in ("AP", "BAT", "SKIN"):
            value = object_(sensors.get(sensor), sensor)
            exact(value.get("type"), {"AP": 0, "BAT": 2, "SKIN": 3}[sensor], sensor + " HAL sensor type")
            exact(value.get("status"), 0, sensor + " thermal status")
            values[sensor] = number(value.get("celsius"), sensor + " Celsius")
        facts.append(values)
        if entry.get("phase") in ("before-run", "before-route", "start"):
            starts.append(values)
        if entry.get("phase") in ("after-run", "after-route", "end"):
            ends.append(values)
    require(len(starts) == 1 and len(ends) == 1, "unique starting and ending thermal phases required")
    return {"startCelsius": starts[0], "endCelsius": ends[0], "observations": len(facts),
            "frameworkStatus": 0, "source": "current HAL"}


def analyze(report, thermals=None):
    object_(report, "fixture report")
    for key, value in (("schema", 1), ("profile", "keeper-mist"), ("result", "complete"),
                       ("error", None), ("ownerSimulationBytesUnchanged", True),
                       ("retirementProven", True), ("armed", True), ("warmupFrames", 60), ("sampleFrames", 120)):
        require(key in report, "missing " + key)
        exact(report[key], value, key)
    require(type(report.get("requestId")) is str and re.fullmatch(r"[A-Za-z0-9_-]{1,64}", report["requestId"]), "invalid request ID")
    require(type(report.get("gpu")) is str and report["gpu"], "missing device GPU identity")
    integer(report.get("driverVersion"), "driverVersion")
    surface = integer(report.get("surfaceGeneration"), "surfaceGeneration", 1)
    setup = integer(report.get("setupTicks"), "setupTicks", 1, 599)
    tick = integer(report.get("simulationTick"), "simulationTick", 1)
    exact(tick, setup, "setup/simulation tick")
    for key in ("width", "height", "surfaceWidth", "surfaceHeight"):
        integer(report.get(key), key, 1, 16384)
    graphics = graphics_settings(report)
    camera = vector(report.get("camera"), 5, "camera")
    require(abs(camera[0] + 31.2) < .0001 and abs(camera[1] + .95) < .0001 and abs(camera[2] + 15.2) < .0001 and
            abs(camera[3] + math.pi / 2) < .0001 and abs(camera[4]) < .0001, "wrong fixed Keeper camera")
    keeper = object_(report.get("keeper"), "keeper")
    for key in ("x", "y", "z"):
        number(keeper.get(key), "keeper." + key)
    require(abs(keeper["x"] + 34.7) < .0001 and abs(keeper["z"] + 15.2) < .0001, "wrong live Keeper anchor")
    for key, value in (("health", 3), ("readable", True), ("roomMistDensity", 1), ("staffLight", 0)):
        if key in ("roomMistDensity", "staffLight"):
            exact(number(keeper.get(key), key), value if type(keeper.get(key)) is int else float(value), key)
        else:
            exact(keeper.get(key), value, "keeper." + key)
    image = object_(report.get("image"), "post-timing image")
    exact(image.get("file"), "environment-survey-" + report["requestId"] + "-final.rgba", "request-scoped image")
    for key in ("width", "height"):
        exact(image.get(key), report[key], "image." + key)
    frames = report.get("frames")
    require(type(frames) is list and len(frames) == 180, "exactly 60 warmup + 120 sample frames required")
    identities, samples, scene = [], [], None
    previous = None
    for index, row in enumerate(frames):
        object_(row, "frame row")
        exact(row.get("index"), index, "contiguous frame index")
        frame = row.get("completedFrame")
        identity, current_scene = validate_frame(frame, graphics, tick)
        if scene is None:
            scene = current_scene
        require(scene == current_scene, "fixed pipeline/lights/resources/settings changed within run")
        if previous:
            for key in ("recordAttemptSerial", "recordSerial", "submissionSerial", "completionSerial"):
                require(identity[key] > previous[key], "non-increasing owning " + key)
            for key in ("sceneEpoch", "measurementGeneration"):
                exact(identity[key], previous[key], "fixed " + key)
        previous = identity
        identities.append((surface,) + tuple(identity[k] for k in
                          ("sceneEpoch", "measurementGeneration", "recordSerial", "submissionSerial", "simulationTick")))
        samples.append(frame)
    timing = object_(report.get("presentation"), "actual presentation")
    exact(timing.get("extensionEnabled"), True, "actual timing extension")
    integer(timing.get("queryCpuWallNanoseconds"), "timing query CPU")
    capture = object_(timing.get("capture"), "actual capture")
    exact(capture.get("schemaVersion"), 1, "actual capture schema")
    status = object_(capture.get("status"), "actual status")
    for key, value in (("enabled", True), ("bound", True), ("surfaceGeneration", surface), ("pendingCount", 0), ("lastQueryResult", 0)):
        exact(status.get(key), value, "actual status." + key)
    swapchain = integer(status.get("swapchainSerial"), "swapchainSerial", 1)
    exact(capture.get("unresolved"), [], "unresolved presentations")
    rows = capture.get("rows")
    require(type(rows) is list and 180 <= len(rows) <= 32768, "missing/bounded actual rows")
    counters = object_(capture.get("counters"), "actual counters")
    for key in LOSS_COUNTERS:
        exact(counters.get(key), 0, "actual collector " + key)
    for key in ("preparedPresents", "acceptedPresents"):
        exact(counters.get(key), len(rows), key + "/actual row accounting")
    integer(counters.get("queryCalls"), "queryCalls", 1)
    integer(counters.get("incompleteQueries"), "incompleteQueries")
    actual, present_ids = {}, set()
    for row in rows:
        object_(row, "actual row")
        key = tuple(integer(row.get(k), "actual." + k, 1) for k in
                    ("surfaceGeneration", "sceneEpoch", "measurementGeneration", "recordSerial", "submissionSerial", "simulationTick"))
        require(key not in actual, "duplicate actual owning identity")
        exact(key[:3], identities[0][:3], "actual surface/scene/measurement scope")
        exact(row.get("swapchainSerial"), swapchain, "actual swapchain scope")
        present_id = integer(row.get("presentID"), "presentID", 1, (1 << 32) - 1)
        require(present_id not in present_ids, "duplicate present ID")
        present_ids.add(present_id)
        timestamp = integer(row.get("actualPresentTime"), "actualPresentTime", 1)
        queued = integer(row.get("queuedSteadyNs"), "queuedSteadyNs", 1)
        require(timestamp >= queued, "actual timestamp precedes queue timestamp")
        integer(row.get("earliestPresentTime"), "earliestPresentTime", 1)
        integer(row.get("presentMargin"), "presentMargin")
        for field, expected in (("scalePercent", 40), ("width", report["surfaceWidth"]),
                                ("height", report["surfaceHeight"]), ("backend", scene["pipeline"]["executionBackend"])):
            exact(row.get(field), expected, "actual " + field)
        actual[key] = timestamp
    require(all(key in actual for key in identities), "missing exact actual presentation identity join")
    timestamps = [actual[key] for key in identities]
    require(all(b > a for a, b in zip(timestamps, timestamps[1:])), "non-increasing retained actual timestamps")
    measured = samples[60:]
    summary = {"accepted": True, "requestId": report["requestId"], "joinedFrames": 180,
               "warmupFrames": 60, "sampleFrames": 120, "gpu": stats([f["gpu"]["wholeRtGpuMs"] for f in measured]),
               "cpuStages": {name: stats([f["stages"][name] for f in measured]) for name in STAGES},
               "diagnosticDisplayIntervals": stats([(timestamps[i] - timestamps[i - 1]) / 1e6 for i in range(61, 180)]),
               "diagnosticDisplayIntervalPolicy": "119 adjacent intervals wholly inside the 120-frame sample; excludes warmup boundary; not gameplay FPS",
               "statisticsPolicy": "60 warmup frames excluded; 120 GPU/CPU samples; p95 uses nearest rank ceil(0.95*n)",
               "workload": {"gpu": report["gpu"], "driverVersion": report["driverVersion"], "setupTicks": setup,
                            "simulationTick": tick, "camera": camera, "keeper": keeper, "graphics": graphics,
                            "extent": [report[k] for k in ("width", "height", "surfaceWidth", "surfaceHeight")], "scene": scene},
               "thermals": None if thermals is None else thermals_summary(thermals), "limits": LIMITS}
    return summary


def compare(first, second, first_thermals=None, second_thermals=None):
    a, b = analyze(first, first_thermals), analyze(second, second_thermals)
    require(a["requestId"] != b["requestId"], "pair must use distinct request IDs")
    require(a["workload"]["graphics"]["mist"] != b["workload"]["graphics"]["mist"], "pair must be Mist On/Off")
    def fixed(summary):
        return {**summary["workload"], "graphics": {k: v for k, v in summary["workload"]["graphics"].items() if k != "mist"}}
    require(fixed(a) == fixed(b), "pair fixed workload/settings differ beyond Mist")
    require((first_thermals is None) == (second_thermals is None), "pair requires both thermal receipts or neither")
    thermal = {"verified": False, "detail": "No thermal receipts supplied; temperature matching unverified."}
    if first_thermals is not None:
        deltas = {endpoint: {sensor: abs(a["thermals"][endpoint][sensor] - b["thermals"][endpoint][sensor])
                            for sensor in ("AP", "BAT", "SKIN")} for endpoint in ("startCelsius", "endCelsius")}
        require(all(delta <= 1.5 + 1e-9 for endpoint in deltas.values() for delta in endpoint.values()),
                "pair current-HAL start/end difference exceeds 1.5 C")
        thermal = {"verified": True, "maximumAllowedDeltaCelsius": 1.5, "absolutePairDeltas": deltas, "frameworkStatus": 0}
    on, off = (a, b) if a["workload"]["graphics"]["mist"] else (b, a)
    return {"accepted": True, "mistOn": on, "mistOff": off, "thermalMatch": thermal,
            "gpuOnMinusOffMs": {name: on["gpu"][name] - off["gpu"][name] for name in ("medianMs", "p95Ms")},
            "limits": LIMITS}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--compare", type=Path, help="other Mist state; fixed workload must match")
    parser.add_argument("--thermals", type=Path, help="current-HAL phase/thermal JSON list")
    parser.add_argument("--compare-thermals", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        require(args.compare is not None or args.compare_thermals is None, "--compare-thermals requires --compare")
        raw = args.report.read_bytes()
        report = load_json(raw)
        thermal = load_json(args.thermals.read_bytes()) if args.thermals else None
        if args.compare:
            other_raw = args.compare.read_bytes()
            other_thermal = load_json(args.compare_thermals.read_bytes()) if args.compare_thermals else None
            result = compare(report, load_json(other_raw), thermal, other_thermal)
            result["inputSha256"] = [hashlib.sha256(raw).hexdigest(), hashlib.sha256(other_raw).hexdigest()]
        else:
            result = analyze(report, thermal)
            result["inputSha256"] = hashlib.sha256(raw).hexdigest()
        text = json.dumps(result, indent=2, allow_nan=False) + "\n"
        if args.output:
            args.output.write_text(text, encoding="utf-8")
        else:
            print(text, end="")
        return 0
    except (AnalysisError, OSError, ValueError, TypeError, KeyError, OverflowError) as error:
        print("Keeper fixture rejected: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
