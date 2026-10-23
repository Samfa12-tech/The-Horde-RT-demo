#!/usr/bin/env python3
"""Strictly join one isolated Android motion ledger to actual image presentations.

This is separate from the schema-2 fixed-route analyzer. It accepts only one
continuous motion resource scope and never infers displayed FPS from GPU time.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import sys
from typing import Any

from analyze_android_present_timing import (
    AnalysisError, MAX_REPORT_BYTES, MAX_TIMING_ROWS, MAX_PENDING_ROWS,
    _object, _reject_constant, _mapping, _array, _integer, _text,
    _percentile_nearest_rank,
)

U64_MAX = (1 << 64) - 1
SCENARIOS = frozenset(("keeper-torch-transition", "torch-low-opening", "shaft-up", "keeper-first-entry",
                       "keeper-retry-reward", "waterfall-equipment", "torch-drench", "chapter-round-trip", "chapter-streaming-cycle", "chapter-resident-comparison", "chapter-streamed-comparison", "chapter-performance-route"))
CHAPTER_SCENARIOS = frozenset(("keeper-torch-transition", "chapter-round-trip", "chapter-streaming-cycle", "chapter-resident-comparison", "chapter-streamed-comparison", "chapter-performance-route"))
OUTDOOR_HOLD_SCENARIO = "chapter-outdoor-scale-hold"
POLICY = ("shared GameSimulation.AdvanceFrame with raw monotonic input-owner delta and normal "
          "fixed-step catch-up; generated motion axes/edges enter through the existing "
          "InputMailbox owner; no per-render fixed-delta override")
ADVERSE_COUNTERS = ("rejectedPresents", "invalidRegistrations", "invalidMetadata",
    "pendingCapacityExhausted", "queryErrors", "zeroPresentTimestamps", "zeroPresentIds",
    "unknownPresentIds", "duplicateTimings", "rowCapacityExhausted", "missingOnRebind",
    "missingOnUnbind", "abandonedPreparedPresents")
COUNTERS = ("preparedPresents", "acceptedPresents", "queryCalls", "incompleteQueries") + ADVERSE_COUNTERS
LIMITS = {"wallSeconds": 120, "presentationDrainMilliseconds": 2000, "stateRows": 16384, "eventRows": 1024, "rtRows": 16384,
          "captures": 64, "presentationRows": MAX_TIMING_ROWS, "presentationPending": MAX_PENDING_ROWS}


def _u64(value: Any, label: str, minimum: int = 0) -> int:
    return _integer(value, label, minimum, U64_MAX)


def _number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise AnalysisError(f"{label} must be a finite number")
    return float(value)


def _scope(row: dict[str, Any], label: str) -> tuple[int, int, int]:
    return tuple(_u64(row.get(field), f"{label}.{field}", 1) for field in
                 ("surfaceGeneration", "sceneEpoch", "measurementGeneration"))


def _identity(row: dict[str, Any], label: str, ledger: bool = False) -> tuple[int, ...]:
    return _scope(row, label) + tuple(_u64(row.get(field), f"{label}.{field}", minimum)
        for field, minimum in (("record" if ledger else "recordSerial", 1),
                               ("submission" if ledger else "submissionSerial", 1),
                               ("tick" if ledger else "simulationTick", 0)))


def analyze_bytes(raw: bytes, *, outdoor_hold: bool = False) -> dict[str, Any]:
    if not raw or len(raw) > MAX_REPORT_BYTES:
        raise AnalysisError("motion report is empty or exceeds the 64 MiB limit")
    try:
        report = _mapping(json.loads(raw, object_pairs_hook=_object,
                                    parse_constant=_reject_constant), "report")
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AnalysisError(f"invalid JSON report: {exc}") from exc
    if _integer(report.get("schema"), "schema") != 1 or report.get("result") != "complete":
        raise AnalysisError("motion report requires schema 1 and result complete")
    run_id = _text(report.get("runId"), "runId")
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", run_id):
        raise AnalysisError("unsafe motion runId")
    scenario = _text(report.get("scenario"), "scenario")
    allowed_scenarios = (OUTDOOR_HOLD_SCENARIO,) if outdoor_hold else SCENARIOS
    if scenario not in allowed_scenarios or report.get("workload") != f"motion-{scenario}-v1":
        raise AnalysisError("unknown or mismatched motion scenario/workload")
    chapter = outdoor_hold or scenario in CHAPTER_SCENARIOS
    build_id = _text(report.get("buildId"), "buildId")
    if report.get("simulationPolicy") != POLICY:
        raise AnalysisError("motion simulationPolicy differs from the admitted shared-clock path")
    limits = _mapping(report.get("limits"), "limits")
    for field, expected in LIMITS.items():
        if _integer(limits.get(field), f"limits.{field}") != expected:
            raise AnalysisError(f"motion limit differs in {field}")
    settings = _mapping(report.get("settings"), "settings")
    scale = _integer(settings.get("scalePercent"), "settings.scalePercent")
    allowed_scales = (20, 33, 40) if outdoor_hold else (33, 40, 50)
    if scale not in allowed_scales:
        raise AnalysisError("outdoor diagnostic scale must be an explicit 20, 33 or 40 comparison" if outdoor_hold else
                            "motion scale must be an explicit 33, 40 or 50 comparison")
    width = _integer(settings.get("width"), "settings.width", 1, 16384)
    height = _integer(settings.get("height"), "settings.height", 1, 16384)
    internal_width = _integer(settings.get("internalWidth"), "settings.internalWidth", 1, width)
    internal_height = _integer(settings.get("internalHeight"), "settings.internalHeight", 1, height)
    backend = _text(settings.get("backend"), "settings.backend")
    if backend not in ("RayTracingPipeline", "RayQueryCompute"):
        raise AnalysisError("motion settings require a real admitted RT backend")
    for field, maximum in (("water", 2), ("fire", 2), ("shadow", 2), ("dust", 2)):
        _integer(settings.get(field), f"settings.{field}", 0, maximum)
    _integer(settings.get("cap"), "settings.cap", 15, 60)
    for field in ("glass", "mist"):
        if not isinstance(settings.get(field), bool):
            raise AnalysisError(f"settings.{field} must be a boolean")
    manifest = _mapping(report.get("motionManifest"), "motionManifest")
    evidence = _mapping(report.get("motionEvidence"), "motionEvidence")
    hold = None
    if outdoor_hold:
        hold = _mapping(report.get("outdoorHold"), "outdoorHold")
        if hold.get("enabled") is not True or hold.get("normalFixedStep") is not True or \
           _integer(hold.get("warmupWallSeconds"), "hold.warmupWallSeconds") != 10 or \
           _integer(hold.get("sampleWallSeconds"), "hold.sampleWallSeconds") != 30:
            raise AnalysisError("outdoor hold requires the declared normal fixed-step 10s warmup/30s sample")
        _u64(hold.get("startWallNanoseconds"), "hold.startWallNanoseconds", 1)
    for label, obj in (("manifest", manifest), ("evidence", evidence)):
        if _integer(obj.get("schema"), f"{label}.schema") != 1 or obj.get("runId") != run_id or obj.get("scenario") != scenario:
            raise AnalysisError(f"{label} identity differs from the motion report")
    if any(manifest.get(field) is not True for field in ("finished", "complete", "armed")):
        raise AnalysisError("motion manifest is not finished, complete and armed")
    if evidence.get("scenarioComplete") is not True or evidence.get("failure") != "":
        raise AnalysisError("motion ledger is incomplete or failed")
    isolation = _mapping(manifest.get("isolation"), "manifest.isolation")
    for field in ("secondSimulation", "fixedDeltaOverride", "phaseForced", "preferencesWritten", "ownerAcceptance"):
        if isolation.get(field) is not False:
            raise AnalysisError(f"motion isolation lacks false {field}")
    if _integer(manifest.get("scale"), "manifest.scale") != scale:
        raise AnalysisError("manifest scale differs from actual settings")
    for field in ("water", "fire", "cap", "glass", "shadow", "mist", "dust"):
        if type(manifest.get(field)) is not type(settings[field]) or manifest[field] != settings[field]:
            raise AnalysisError(f"manifest {field} differs from actual settings")
    _integer(manifest.get("captures"), "manifest.captures", 0 if chapter else 1, 64)
    if chapter and manifest["captures"] != 0:
        raise AnalysisError("chapter pacing interval must not contain image readbacks")
    if _integer(evidence.get("pendingSubmissionCount"), "ledger.pendingSubmissionCount") != 0 or evidence.get("currentScopePresented") is not True:
        raise AnalysisError("motion ledger has pending work or no current-scope presentation")
    if _array(evidence.get("retiredSubmissions"), "retiredSubmissions", 16384):
        raise AnalysisError("retired motion submissions cannot support continuous displayed-image intervals")
    scopes = _array(evidence.get("resourceScopes"), "resourceScopes", 64)
    if len(scopes) != 1:
        raise AnalysisError("interval summary requires one continuous motion resource scope")
    scope = _scope(_mapping(scopes[0], "scope"), "scope")
    if _scope(manifest, "manifest") != scope or _integer(scopes[0].get("nextStateRow"), "scope.nextStateRow") != 0:
        raise AnalysisError("manifest/resource scope mismatch")
    states = _array(evidence.get("states"), "states", 16384)
    if not states:
        raise AnalysisError("motion ledger has no state rows")
    previous_wall = previous_tick = previous_publication = previous_overrun = 0
    for index, raw_state in enumerate(states):
        state = _mapping(raw_state, f"state {index}")
        if _integer(state.get("row"), "state.row") != index:
            raise AnalysisError("motion state row indices are discontinuous")
        wall = _u64(state.get("wallNs"), "state.wallNs", 1)
        tick = _u64(state.get("tick"), "state.tick")
        publication = _u64(state.get("publication"), "state.publication")
        overrun = _u64(state.get("overruns"), "state.overruns")
        _u64(state.get("ticksThisFrame"), "state.ticksThisFrame")
        if wall < previous_wall or tick < previous_tick or publication < previous_publication or overrun < previous_overrun:
            raise AnalysisError("motion wall/tick/publication/overrun state regressed")
        if _number(state.get("simulationSeconds"), "state.simulationSeconds") < 0:
            raise AnalysisError("negative motion simulation time")
        axes = _array(state.get("axes"), "state.axes", 2)
        if len(axes) != 2 or any(abs(_number(axis, "axis")) > 1.00001 for axis in axes):
            raise AnalysisError("motion state axes are outside admitted bounds")
        previous_wall, previous_tick, previous_publication, previous_overrun = wall, tick, publication, overrun
    if states[-1]["wallNs"] - states[0]["wallNs"] > 120_000_000_000:
        raise AnalysisError("motion state span exceeds the wall-time limit")
    if outdoor_hold:
        start = hold["startWallNanoseconds"]
        if states[0]["wallNs"] < start or states[-1]["wallNs"] < start + 40_000_000_000 or \
           states[-1]["wallNs"] - start > 120_000_000_000 or states[-1]["tick"] <= states[0]["tick"]:
            raise AnalysisError("outdoor states must cover the complete bounded live 40s hold")
    events = _array(evidence.get("events"), "events", 1024)
    last_event = 0
    for index, raw_event in enumerate(events):
        event = _mapping(raw_event, f"event {index}")
        seq = _u64(event.get("sequence"), "event.sequence", 1)
        _integer(event.get("observedStateRow"), "event.observedStateRow", 0, len(states) - 1)
        if seq <= last_event:
            raise AnalysisError("motion semantic event order is not strictly increasing")
        last_event = seq
    frames = _array(evidence.get("completedRtFrames"), "completedRtFrames", 16384)
    if len(frames) < 2:
        raise AnalysisError("motion interval summary requires at least two completed RT frames")
    selected: dict[tuple[int, ...], dict[str, Any]] = {}
    for index, raw_frame in enumerate(frames):
        frame = _mapping(raw_frame, f"frame {index}")
        key = _identity(frame, f"frame {index}", True)
        if key[:3] != scope:
            raise AnalysisError("completed motion frame has a mixed resource scope")
        if key in selected:
            raise AnalysisError("duplicate completed motion frame identity")
        state_index = _integer(frame.get("stateRow"), "frame.stateRow", 0, len(states) - 1)
        if key[-1] != states[state_index]["tick"]:
            raise AnalysisError("completed motion frame tick differs from its bound state")
        _u64(frame.get("recordAttempt"), "frame.recordAttempt", 1)
        _u64(frame.get("completion"), "frame.completion", 1)
        if "actualUploadedDustQuality" in frame and frame["actualUploadedDustQuality"] != settings["dust"]:
            raise AnalysisError("actual uploaded dust differs from settings")
        if "actualUploadedMistEnabled" in frame and frame["actualUploadedMistEnabled"] is not settings["mist"]:
            raise AnalysisError("actual uploaded mist differs from settings")
        selected[key] = frame
    if outdoor_hold:
        sample_walls = [states[frame["stateRow"]]["wallNs"] for frame in frames
                        if start + 10_000_000_000 <= states[frame["stateRow"]]["wallNs"] <= start + 40_000_000_000]
        if len(sample_walls) < 20 or sample_walls[-1] - sample_walls[0] < 25_000_000_000:
            raise AnalysisError("outdoor completed frames must cover at least 25s of the declared 30s sample")
    images = _array(report.get("motionImageCaptures"), "motionImageCaptures", 64)
    if len(images) != manifest["captures"]:
        raise AnalysisError("motion image count differs from the completed manifest")
    for index, raw_image in enumerate(images):
        image = _mapping(raw_image, f"motion image {index}")
        if image.get("file") != f"android-motion-{run_id}-{index}.rgba":
            raise AnalysisError("motion image filename is not its exact owned run/index")
        rt_index = _integer(image.get("rtRow"), "image.rtRow", 0, len(frames) - 1)
        if _integer(image.get("stateRow"), "image.stateRow", 0, len(states) - 1) != frames[rt_index]["stateRow"]:
            raise AnalysisError("motion image lacks its exact RT/state binding")
        if _integer(image.get("width"), "image.width", 1) != internal_width or \
           _integer(image.get("height"), "image.height", 1) != internal_height or \
           _integer(image.get("bytes"), "image.bytes", 1) != internal_width * internal_height * 4:
            raise AnalysisError("motion image dimensions/bytes differ from the traced resource extent")
    first_key = min(selected, key=lambda key: key[4])
    if _identity(_mapping(report.get("startingFrameIdentity"), "startingFrameIdentity"), "startingFrameIdentity") != first_key:
        raise AnalysisError("starting frame identity differs from the first completed motion frame")
    eligibility = _mapping(report.get("timingEligibility"), "timingEligibility")
    if eligibility.get("eligible") is not True or any(_integer(eligibility.get(field), field) != len(frames) for field in
                                                     ("matchedCompletedFrames", "expectedCompletedFrames")):
        raise AnalysisError("native timing eligibility/counts are incomplete")
    wrapper = _mapping(report.get("imagePresentationTiming"), "imagePresentationTiming")
    capture = _mapping(wrapper.get("capture"), "capture")
    status = _mapping(capture.get("status"), "capture.status")
    if wrapper.get("extensionEnabled") is not True or status.get("enabled") is not True or status.get("bound") is not True:
        raise AnalysisError("actual image presentation collector is unavailable or unbound")
    if _integer(capture.get("schemaVersion"), "capture.schemaVersion") != 1:
        raise AnalysisError("unsupported presentation collector schema")
    if _integer(status.get("pendingCount"), "capture.status.pendingCount") or _array(capture.get("unresolved"), "unresolved", 256):
        raise AnalysisError("presentation collector has unresolved IDs")
    counters = _mapping(capture.get("counters"), "capture.counters")
    baseline = _mapping(wrapper.get("counterBaseline"), "counterBaseline")
    delta = _mapping(wrapper.get("counterDelta"), "counterDelta")
    for field in COUNTERS:
        total, before, change = (_u64(obj.get(field), f"{label}.{field}") for obj, label in
                                 ((counters, "counters"), (baseline, "baseline"), (delta, "delta")))
        if before > total or total - before != change:
            raise AnalysisError(f"presentation counter delta is inconsistent in {field}")
        if field in ADVERSE_COUNTERS and change:
            raise AnalysisError(f"adverse presentation run counter {field}")
    timings = _array(capture.get("rows"), "capture.rows", MAX_TIMING_ROWS)
    row_baseline = _integer(wrapper.get("rowIndexBeforeRun"), "rowIndexBeforeRun", 0, len(timings))
    if counters["acceptedPresents"] != len(timings) or row_baseline > baseline["acceptedPresents"]:
        raise AnalysisError("presentation row/counter baseline mismatch")
    if counters["preparedPresents"] != counters["acceptedPresents"]:
        raise AnalysisError("presentation collector has an unregistered prepared image")
    matched: dict[tuple[int, ...], dict[str, Any]] = {}
    seen_ids: set[int] = set()
    for index, raw_timing in enumerate(timings):
        timing = _mapping(raw_timing, f"timing {index}")
        key = _identity(timing, f"timing {index}")
        present_id = _integer(timing.get("presentID"), "presentID", 1, (1 << 32) - 1)
        if present_id in seen_ids:
            raise AnalysisError("duplicate presentation ID")
        seen_ids.add(present_id)
        if key not in selected:
            if key[4] >= first_key[4]:
                raise AnalysisError("unmatched presentation interleaves or follows the motion span")
            continue  # Startup owns earlier submissions; it is not a motion interval.
        if index < row_baseline or key in matched:
            raise AnalysisError("duplicate or pre-run motion timing identity")
        if _integer(timing.get("scalePercent"), "timing.scalePercent") != scale or timing.get("backend") != backend or \
           _integer(timing.get("width"), "timing.width", 1) != width or _integer(timing.get("height"), "timing.height", 1) != height:
            raise AnalysisError("motion timing metadata differs from settings")
        if key[0] != _u64(status.get("surfaceGeneration"), "status.surfaceGeneration", 1) or \
           _u64(timing.get("swapchainSerial"), "timing.swapchainSerial", 1) != _u64(status.get("swapchainSerial"), "status.swapchainSerial", 1):
            raise AnalysisError("motion timing collector has mixed surface/swapchain ownership")
        _u64(timing.get("actualPresentTime"), "actualPresentTime", 1)
        _u64(timing.get("earliestPresentTime"), "earliestPresentTime")
        _u64(timing.get("presentMargin"), "presentMargin")
        matched[key] = timing
    if matched.keys() != selected.keys():
        raise AnalysisError("completed motion frames and presentation identities do not join exactly")
    ordered = sorted(matched.items(), key=lambda item: item[0][4])
    for (previous_key, previous), (key, current) in zip(ordered, ordered[1:]):
        if key[3] != previous_key[3] + 1 or key[4] != previous_key[4] + 1 or current["presentID"] != previous["presentID"] + 1:
            raise AnalysisError("motion record/submission/presentation IDs contain a gap")
        if current["actualPresentTime"] <= previous["actualPresentTime"]:
            raise AnalysisError("actual presentation timestamps are not strictly increasing")
    times = [timing["actualPresentTime"] for _, timing in ordered]
    intervals = [b - a for a, b in zip(times, times[1:])]
    sorted_intervals = sorted(intervals)
    median = (sorted_intervals[(len(intervals) - 1) // 2] + sorted_intervals[len(intervals) // 2]) // 2
    duration = times[-1] - times[0]
    return {"eligible": True, "reportSha256": hashlib.sha256(raw).hexdigest(),
        "runId": run_id, "workload": report["workload"], "buildId": build_id, "settings": settings,
        "scope": {"surfaceGeneration": scope[0], "sceneEpoch": scope[1], "measurementGeneration": scope[2],
            "matchedCompletedFrames": len(frames), "intervalCount": len(intervals),
            "durationNanoseconds": duration, "setupPresentationRowsExcluded": len(timings) - len(frames)},
        "intervalsNanoseconds": {"median": median,
            "p90NearestRank": _percentile_nearest_rank(sorted_intervals, .90),
            "p95NearestRank": _percentile_nearest_rank(sorted_intervals, .95), "maximum": max(intervals),
            "over33333000Nanoseconds": sum(value > 33_333_000 for value in intervals),
            "over250000000Nanoseconds": sum(value > 250_000_000 for value in intervals)},
        "observedPresentedImageRatePerSecond": round(len(intervals) * 1_000_000_000 / duration, 6),
        "simulation": {"stateRows": len(states), "eventRows": len(events),
            "ticksThisFrameMaximum": max(state["ticksThisFrame"] for state in states),
            "catchUpOverrunCountFirst": states[0]["overruns"], "catchUpOverrunCountLast": states[-1]["overruns"],
            "catchUpOverrunDelta": states[-1]["overruns"] - states[0]["overruns"]},
        "collector": {"runCounterDelta": delta,
            "queryCpuWallNanoseconds": _u64(wrapper.get("queryCpuWallNanoseconds"), "queryCpuWallNanoseconds"),
            "presentMarginAtLeast2Pow63CountIgnored": sum(timing["presentMargin"] >= (1 << 63) for _, timing in ordered)},
        "limits": [("Bounded diagnostic outdoor hold with zero image readbacks; 20% is not image-quality admission." if outdoor_hold else
                    "Bounded ordinary chapter traversal with zero mid-route image readbacks; recorder overhead remains." if chapter else "Short harness-generated moving scenario with milestone RT readbacks; total instrumentation affects pacing."),
                   "One continuous resource scope; retry/rotation/lifecycle discontinuities require separate evidence.",
                   "Actual inter-present intervals; not sustained FPS, scanout, photons, GPU reciprocal time, touch latency or owner acceptance.",
                   "Trace extents are native resource observations; this analyzer does not certify appearance, thermals or power."]}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--outdoor-hold", action="store_true", help="explicit diagnostic outdoor 20/33/40 cohort; preserves legacy route policy")
    args = parser.parse_args(argv)
    try:
        with args.report.open("rb") as handle:
            result = analyze_bytes(handle.read(MAX_REPORT_BYTES + 1), outdoor_hold=args.outdoor_hold)
    except (OSError, AnalysisError) as exc:
        print(json.dumps({"eligible": False, "error": str(exc)}, separators=(",", ":")), file=sys.stderr)
        return 2
    print(json.dumps(result, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
