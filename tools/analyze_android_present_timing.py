#!/usr/bin/env python3
"""Validate and summarize matched Vulkan displayed-image timing evidence."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any

MAX_REPORT_BYTES = 64 * 1024 * 1024
MAX_TIMING_ROWS = 32_768
MAX_OWNING_ROWS = 4_000
MAX_PENDING_ROWS = 256
U64_HALF = 1 << 63
INTERVAL_33333_MS_NS = 33_333_000
INTERVAL_250_MS_NS = 250_000_000


class AnalysisError(ValueError):
    """The report cannot support an eligible displayed-image interval summary."""


def _object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise AnalysisError(f"duplicate JSON object key: {key}")
        result[key] = value
    return result


def _reject_constant(value: str) -> None:
    raise AnalysisError(f"nonstandard JSON numeric constant: {value}")


def _mapping(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise AnalysisError(f"{label} must be an object")
    return value


def _array(value: Any, label: str, limit: int) -> list[Any]:
    if not isinstance(value, list):
        raise AnalysisError(f"{label} must be an array")
    if len(value) > limit:
        raise AnalysisError(f"{label} exceeds the {limit}-row limit")
    return value


def _integer(value: Any, label: str, minimum: int = 0, maximum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        raise AnalysisError(f"{label} must be an integer >= {minimum}")
    if maximum is not None and value > maximum:
        raise AnalysisError(f"{label} exceeds its maximum {maximum}")
    return value


def _text(value: Any, label: str, *, allow_empty: bool = False) -> str:
    if not isinstance(value, str) or (not allow_empty and not value):
        raise AnalysisError(f"{label} must be a nonempty string")
    return value


def _same_frame_identity(row: dict[str, Any], label: str) -> tuple[int, int, int, int, int]:
    identity = _mapping(row.get("submittedIdentity"), f"{label}.submittedIdentity")
    scene = _integer(identity.get("sceneEpoch"), f"{label}.sceneEpoch", 1)
    generation = _integer(identity.get("measurementGeneration"), f"{label}.measurementGeneration", 1)
    record = _integer(identity.get("recordSerial"), f"{label}.recordSerial", 1)
    submission = _integer(identity.get("submissionSerial"), f"{label}.submissionSerial", 1)
    tick = _integer(identity.get("simulationTick"), f"{label}.simulationTick")
    completion = _mapping(row.get("completionIdentity"), f"{label}.completionIdentity")
    for field, expected in (("sceneEpoch", scene), ("measurementGeneration", generation),
                            ("recordSerial", record), ("submissionSerial", submission),
                            ("simulationTick", tick)):
        if _integer(completion.get(field), f"{label}.completionIdentity.{field}") != expected:
            raise AnalysisError(f"{label} submitted/completed identity mismatch in {field}")
    return scene, generation, record, submission, tick


def _percentile_nearest_rank(sorted_values: list[int], percentile: float) -> int:
    # Nearest-rank quantiles preserve integer nanoseconds (rank = ceil(p * N)).
    return sorted_values[max(0, math.ceil(percentile * len(sorted_values)) - 1)]


def analyze_bytes(raw: bytes) -> dict[str, Any]:
    """Analyze one bounded native benchmark JSON document."""
    if not raw:
        raise AnalysisError("report is empty")
    if len(raw) > MAX_REPORT_BYTES:
        raise AnalysisError(f"report exceeds the {MAX_REPORT_BYTES}-byte limit")
    report_sha256 = hashlib.sha256(raw).hexdigest()
    try:
        report = json.loads(raw, object_pairs_hook=_object, parse_constant=_reject_constant)
    except AnalysisError:
        raise
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AnalysisError(f"invalid JSON report: {exc}") from exc
    report = _mapping(report, "report")
    if _integer(report.get("schema"), "report.schema", 1) != 2:
        raise AnalysisError("unsupported benchmark report schema")
    if report.get("result") != "complete":
        raise AnalysisError("benchmark report result is not complete")
    if report.get("status") != "complete":
        raise AnalysisError("benchmark report status is not complete")
    if "invalidRun" in report and report.get("invalidRun") is not False:
        raise AnalysisError("benchmark report marks the run invalid")
    if report.get("workloadComplete") is not True:
        raise AnalysisError("benchmark workload is not complete")

    evidence = _mapping(report.get("completedFrameEvidence"), "completedFrameEvidence")
    if _integer(evidence.get("schema"), "completedFrameEvidence.schema", 1) != 1:
        raise AnalysisError("unsupported completedFrameEvidence schema")
    if evidence.get("status") != "complete":
        raise AnalysisError("completedFrameEvidence status is not complete")
    if evidence.get("invalidRun") is not False:
        raise AnalysisError("completedFrameEvidence is invalid or lacks an explicit valid-run marker")
    generation = _integer(evidence.get("measurementGeneration"), "measurementGeneration", 1)
    scene_epoch = _integer(evidence.get("sceneEpoch"), "sceneEpoch", 1)
    counts = _mapping(evidence.get("counts"), "completedFrameEvidence.counts")
    expected = _integer(counts.get("expected"), "counts.expected", 1)
    completed = _integer(counts.get("completed"), "counts.completed")
    for field in ("rejected", "cancelled", "outstanding", "cpuRejected"):
        if _integer(counts.get(field), f"counts.{field}") != 0:
            raise AnalysisError(f"completedFrameEvidence has nonzero {field}")
    cpu_accepted = _integer(counts.get("cpuAccepted"), "counts.cpuAccepted")
    if cpu_accepted != expected:
        raise AnalysisError("completedFrameEvidence cpuAccepted count differs from expected")
    if completed != expected:
        raise AnalysisError("completedFrameEvidence completed count differs from expected")
    if _integer(report.get("measuredFrames"), "measuredFrames") != expected:
        raise AnalysisError("report measuredFrames differs from completedFrameEvidence expected")
    if expected > MAX_OWNING_ROWS:
        raise AnalysisError(f"completed-frame evidence exceeds native capacity {MAX_OWNING_ROWS}")
    frame_rows = _array(evidence.get("rows"), "completedFrameEvidence.rows", MAX_OWNING_ROWS)
    selected_frames: dict[tuple[int, int, int, int], tuple[dict[str, Any], int]] = {}
    for index, raw_row in enumerate(frame_rows):
        row = _mapping(raw_row, f"completedFrameEvidence.rows[{index}]")
        identity = _same_frame_identity(row, f"completedFrameEvidence.rows[{index}]")
        row_scene, row_generation, record, submission, tick = identity
        if row_scene != scene_epoch or row_generation != generation:
            continue  # Earlier warm-up/measurement generations are outside this measured set.
        if row.get("disposition") != "completed":
            raise AnalysisError(f"selected frame {index} is not completed")
        if row.get("presentationOutcome") != "presented":
            raise AnalysisError(f"selected frame {index} was not presented")
        if row.get("cpuAccepted") is not True:
            raise AnalysisError(f"selected frame {index} was rejected or lacks CPU evidence acceptance")
        key = (row_scene, row_generation, record, submission)
        if key in selected_frames:
            raise AnalysisError("duplicate completed-frame identity in selected generation")
        selected_frames[key] = (row, tick)
    if len(selected_frames) != expected:
        raise AnalysisError(f"selected completed-frame row count {len(selected_frames)} differs from expected {expected}")

    image_timing = _mapping(report.get("imagePresentationTiming"), "imagePresentationTiming")
    if image_timing.get("extensionEnabled") is not True:
        raise AnalysisError("VK_GOOGLE_display_timing capture is not enabled")
    capture = _mapping(image_timing.get("capture"), "imagePresentationTiming.capture")
    if _integer(capture.get("schemaVersion"), "capture.schemaVersion", 1) != 1:
        raise AnalysisError("unsupported presentation timing capture schema")
    capture_status = _mapping(capture.get("status"), "imagePresentationTiming.capture.status")
    if capture_status.get("enabled") is not True:
        raise AnalysisError("native presentation timing collector is not enabled")
    counters = _mapping(capture.get("counters"), "imagePresentationTiming.capture.counters")
    # VK_INCOMPLETE only says a bounded poll had more records; later polls may
    # recover them. Loss/invalidity counters and a final unresolved tail fail.
    for field in ("queryErrors", "zeroPresentTimestamps", "zeroPresentIds", "unknownPresentIds",
                  "duplicateTimings", "rowCapacityExhausted", "pendingCapacityExhausted",
                  "missingOnRebind", "missingOnUnbind", "invalidRegistrations", "invalidMetadata",
                  "abandonedPreparedPresents", "rejectedPresents"):
        if _integer(counters.get(field), f"capture.counters.{field}") != 0:
            raise AnalysisError(f"presentation timing collector has nonzero {field}")
    if _integer(capture_status.get("pendingCount"), "capture.status.pendingCount") != 0:
        raise AnalysisError("presentation timing collector has unresolved presents")
    if _integer(image_timing.get("queryCpuWallNanoseconds"), "queryCpuWallNanoseconds") < 0:
        raise AnalysisError("invalid query CPU wall duration")
    unresolved_rows = _array(capture.get("unresolved"), "imagePresentationTiming.capture.unresolved", MAX_PENDING_ROWS)
    if unresolved_rows:
        raise AnalysisError("presentation timing collector exported unresolved presents")
    timing_rows = _array(capture.get("rows"), "imagePresentationTiming.capture.rows", MAX_TIMING_ROWS)
    accepted_presents = _integer(counters.get("acceptedPresents"), "capture.counters.acceptedPresents")
    if accepted_presents != len(timing_rows):
        raise AnalysisError("accepted presentation count differs from exported timing row count")
    selected_timing: dict[tuple[int, int, int, int], dict[str, Any]] = {}
    seen_ids: set[int] = set()
    for index, raw_row in enumerate(timing_rows):
        row = _mapping(raw_row, f"imagePresentationTiming.capture.rows[{index}]")
        present_id = _integer(row.get("presentID"), f"timing row {index}.presentID", 1, (1 << 32) - 1)
        if present_id in seen_ids:
            raise AnalysisError("duplicate presentID in timing rows")
        seen_ids.add(present_id)
        row_scene = _integer(row.get("sceneEpoch"), f"timing row {index}.sceneEpoch", 1)
        row_generation = _integer(row.get("measurementGeneration"), f"timing row {index}.measurementGeneration", 1)
        if row_scene != scene_epoch or row_generation != generation:
            continue
        record = _integer(row.get("recordSerial"), f"timing row {index}.recordSerial", 1)
        submission = _integer(row.get("submissionSerial"), f"timing row {index}.submissionSerial", 1)
        key = (row_scene, row_generation, record, submission)
        if key in selected_timing:
            raise AnalysisError("duplicate timing join identity in selected generation")
        selected_timing[key] = row
    if len(selected_timing) != expected:
        raise AnalysisError(f"selected timing row count {len(selected_timing)} differs from expected {expected}")
    if selected_timing.keys() != selected_frames.keys():
        raise AnalysisError("completed-frame and timing identities do not join exactly")

    report_scale = _integer(report.get("renderScalePercent"), "renderScalePercent", 1, 100)
    presentation_extent = _mapping(report.get("presentationExtent"), "presentationExtent")
    expected_width = _integer(presentation_extent.get("width"), "presentationExtent.width", 1)
    expected_height = _integer(presentation_extent.get("height"), "presentationExtent.height", 1)
    expected_backend = _text(report.get("executionBackend"), "executionBackend")
    surfaces: set[tuple[int, int]] = set()
    ordered: list[tuple[int, int, int, int, int, dict[str, Any]]] = []
    for key, (frame, frame_tick) in selected_frames.items():
        timing = selected_timing[key]
        row_label = f"timing row for submission {key[3]}"
        if _integer(timing.get("simulationTick"), f"{row_label}.simulationTick") != frame_tick:
            raise AnalysisError(f"{row_label} simulationTick differs from completed frame")
        if _integer(timing.get("scalePercent"), f"{row_label}.scalePercent", 1) != report_scale:
            raise AnalysisError(f"{row_label} scalePercent differs from report settings")
        if _integer(timing.get("width"), f"{row_label}.width", 1) != expected_width or \
           _integer(timing.get("height"), f"{row_label}.height", 1) != expected_height:
            raise AnalysisError(f"{row_label} extent differs from report presentation extent")
        if _text(timing.get("backend"), f"{row_label}.backend") != expected_backend:
            raise AnalysisError(f"{row_label} backend differs from report settings")
        surface = (_integer(timing.get("surfaceGeneration"), f"{row_label}.surfaceGeneration", 1),
                   _integer(timing.get("swapchainSerial"), f"{row_label}.swapchainSerial", 1))
        surfaces.add(surface)
        present_id = _integer(timing.get("presentID"), f"{row_label}.presentID", 1)
        actual_time = _integer(timing.get("actualPresentTime"), f"{row_label}.actualPresentTime", 1, (1 << 64) - 1)
        _integer(timing.get("earliestPresentTime"), f"{row_label}.earliestPresentTime", 0, (1 << 64) - 1)
        margin = _integer(timing.get("presentMargin"), f"{row_label}.presentMargin", 0, (1 << 64) - 1)
        ordered.append((key[3], key[2], present_id, actual_time, margin, timing))
    if len(surfaces) != 1:
        raise AnalysisError("selected timing rows span mixed surface generations or swapchains")
    only_surface = next(iter(surfaces))
    if only_surface != (_integer(capture_status.get("surfaceGeneration"), "capture.status.surfaceGeneration", 1),
                        _integer(capture_status.get("swapchainSerial"), "capture.status.swapchainSerial", 1)):
        raise AnalysisError("selected timing rows do not match collector surface/swapchain status")
    ordered.sort(key=lambda item: item[0])
    for index in range(1, len(ordered)):
        previous, current = ordered[index - 1], ordered[index]
        if current[0] != previous[0] + 1 or current[1] != previous[1] + 1 or current[2] != previous[2] + 1:
            raise AnalysisError("selected submission, record, or present IDs contain a gap")
        if current[3] <= previous[3]:
            raise AnalysisError("actualPresentTime does not strictly increase with submission order")

    intervals = [ordered[index][3] - ordered[index - 1][3] for index in range(1, len(ordered))]
    sorted_intervals = sorted(intervals)
    first_time, last_time = ordered[0][3], ordered[-1][3]
    duration = last_time - first_time
    if duration <= 0:
        raise AnalysisError("measured actualPresentTime span is not positive")
    margin_anomalies = sum(1 for item in ordered if item[4] >= U64_HALF)
    median = (sorted_intervals[(len(sorted_intervals) - 1) // 2] +
              sorted_intervals[len(sorted_intervals) // 2]) // 2
    observed_rate = (len(ordered) - 1) * 1_000_000_000 / duration
    query_cpu_wall = _integer(image_timing.get("queryCpuWallNanoseconds"), "queryCpuWallNanoseconds")
    settings_fields = ("schema", "result", "status", "measuredFrames", "runId", "timestampUtc",
                       "build", "shader", "gpu", "vulkanApi", "rtMode",
                       "executionBackend", "presentMode", "workload", "simulationPolicy",
                       "renderScalePercent", "internalExtent", "presentationExtent",
                       "lapsCompleted", "lapsRequested", "routeTraversalComplete", "workloadComplete")
    settings = {field: report[field] for field in settings_fields if field in report}
    return {
        "eligible": True,
        "reportSha256": report_sha256,
        "settings": settings,
        "scope": {
            "measurementGeneration": generation,
            "sceneEpoch": scene_epoch,
            "surfaceGeneration": next(iter(surfaces))[0],
            "swapchainSerial": next(iter(surfaces))[1],
            "matchedCompletedFrames": len(ordered),
            "matchedTimingRows": len(ordered),
            "acceptedTimingRows": accepted_presents,
            "intervalCount": len(intervals),
            "durationNanoseconds": duration,
            "firstActualPresentTimeNanoseconds": first_time,
            "lastActualPresentTimeNanoseconds": last_time,
        },
        "intervalsNanoseconds": {
            "median": median,
            "p90NearestRank": _percentile_nearest_rank(sorted_intervals, 0.90),
            "p95NearestRank": _percentile_nearest_rank(sorted_intervals, 0.95),
            "maximum": max(intervals),
            "over33333000Nanoseconds": sum(1 for value in intervals if value > INTERVAL_33333_MS_NS),
            "over250000000Nanoseconds": sum(1 for value in intervals if value > INTERVAL_250_MS_NS),
        },
        "observedPresentedImageRatePerSecond": round(observed_rate, 6),
        "collector": {
            "queryCalls": _integer(counters.get("queryCalls"), "capture.counters.queryCalls"),
            "acceptedPresents": accepted_presents,
            "incompleteQueries": _integer(counters.get("incompleteQueries"), "capture.counters.incompleteQueries"),
            "queryCpuWallNanoseconds": query_cpu_wall,
            "presentMarginAtLeast2Pow63CountIgnored": margin_anomalies,
        },
        "limits": [
            "Observed inter-present intervals and rate describe only this matched short measured span.",
            "Not sustained performance, scanout completion, photons, or CPU/GPU reciprocal timing.",
            "presentMargin is preserved only as an anomaly count and is excluded from all metrics.",
        ],
    }


def analyze_file(path: Path) -> dict[str, Any]:
    try:
        with path.open("rb") as handle:
            raw = handle.read(MAX_REPORT_BYTES + 1)
    except OSError as exc:
        raise AnalysisError(f"cannot read report: {exc}") from exc
    return analyze_bytes(raw)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, help="native benchmark JSON report")
    args = parser.parse_args(argv)
    try:
        result = analyze_file(args.report)
    except AnalysisError as exc:
        print(json.dumps({"eligible": False, "error": str(exc)}, separators=(",", ":")), file=sys.stderr)
        return 2
    print(json.dumps(result, separators=(",", ":"), ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
