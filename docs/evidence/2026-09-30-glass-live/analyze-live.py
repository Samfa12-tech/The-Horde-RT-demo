#!/usr/bin/env python3
"""Bounded counter/identity analysis for the 2026-09-30 live glass runs."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent
REPO = Path.cwd()  # Invoke from the repository root containing the generated ABI.
ABI = REPO / "src/vulkan/raytracing/RtSceneAbi.generated.h"
PHONE_DIR = ROOT / "glass-geometric-live-20260930-01"
MAX_REPORT_BYTES = 16 * 1024 * 1024
MAX_ROWS = 4000
MAX_COUNTERS = 128
MAX_WINDOWS_SHOWN = 8

IDENTITY_FIELDS = (
    "sceneEpoch",
    "measurementGeneration",
    "recordAttemptSerial",
    "recordSerial",
    "simulationTick",
    "frameSlot",
    "submissionSerial",
)

# Exact field names from RtSceneAbi.generated.h, not inferred counter labels.
FAIL_COUNTER_NAMES = (
    "transportOverflowCount",
    "shadowOverflowCount",
    "secondaryDielectricRejectCount",
    "unclosedVolumeCount",
    "primaryUnclosedVolumeCount",
    "shadowUnclosedVolumeCount",
    "productionPaneStackFailureCount",
    "secondaryNearSelfHitCount",
    "primaryOpenMissCount",
    "primaryOpenOpaqueCount",
    "primaryMismatchedExitCount",
    "primaryInterfaceBudgetCount",
    "primaryVolumeBudgetCount",
    "shadowOpenMissCount",
    "shadowMismatchedExitCount",
    "primaryInterfaceBudgetOpenVolumeCount",
    "primaryInterfaceBudgetClosedVolumeCount",
    "shadowMismatchEmptyCount",
    "primaryOpenOpaqueSameInstanceDifferentMaterialCount",
    "primaryOpenOpaqueAfterTirCount",
)
BOUNDED_EVENT_NAMES = ("secondaryDielectricTerminalCount",)
RECOVERY_NAMES = (
    "primaryCertifiedClosedVolumeRecoveryCount",
    "shadowCertifiedClosedVolumeRecoveryCount",
)
TIR_NAMES = ("primaryTirCount", "primaryTirTerminationCount")
WINDOW_NAMES = FAIL_COUNTER_NAMES + BOUNDED_EVENT_NAMES + RECOVERY_NAMES + TIR_NAMES
MASK_NAMES = (
    "primaryOpenOpaqueTerminalInstanceMask",
    "primaryOpenOpaqueVolumeInstanceMask",
    "primaryOpenOpaqueTerminalMaterialMask",
    "certifiedClosedVolumeRecoveryReasonMask",
)


def read_json(path: Path) -> Any:
    size = path.stat().st_size
    if size > MAX_REPORT_BYTES:
        raise ValueError(f"report exceeds bounded {MAX_REPORT_BYTES}-byte limit: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def counter_names_from_abi() -> list[str]:
    source = ABI.read_text(encoding="utf-8")
    match = re.search(
        r"struct\s+alignas\(16\)\s+RtDielectricDiagnostics\s*\{(.*?)\n\};",
        source,
        re.DOTALL,
    )
    if not match:
        raise ValueError(f"RtDielectricDiagnostics not found in {ABI}")
    names = re.findall(r"std::uint32_t\s+(\w+)\s*=", match.group(1))
    if len(names) != 41 or len(names) > MAX_COUNTERS:
        raise ValueError(f"expected ABI's 41 counters; parsed {len(names)}")
    for name in FAIL_COUNTER_NAMES + BOUNDED_EVENT_NAMES + RECOVERY_NAMES + TIR_NAMES + MASK_NAMES:
        if name not in names:
            raise ValueError(f"expected exact ABI field is absent: {name}")
    return names


def witness(row: dict[str, Any], value: int) -> dict[str, Any]:
    submitted = row.get("submittedIdentity") or {}
    completed = row.get("completionIdentity") or {}
    return {
        "index": row.get("index"),
        "lap": row.get("lap"),
        "zone": row.get("zone"),
        "simulationTick": submitted.get("simulationTick"),
        "submissionSerial": submitted.get("submissionSerial"),
        "completionSerial": completed.get("completionSerial"),
        "value": value,
    }


def counter_windows(rows: list[dict[str, Any]], counter_index: int) -> dict[str, Any]:
    nonzero: list[tuple[dict[str, Any], int]] = []
    for row in rows:
        value = row["diagnosticCounters"][counter_index]
        if value:
            nonzero.append((row, value))
    if not nonzero:
        return {"frames": 0, "sum": 0, "max": 0, "maxWitness": None, "first": None, "last": None, "windowCount": 0, "windows": []}

    windows: list[dict[str, Any]] = []
    run_rows: list[tuple[dict[str, Any], int]] = []
    for row, value in nonzero:
        if run_rows and row["index"] != run_rows[-1][0]["index"] + 1:
            windows.append(make_window(run_rows))
            run_rows = []
        run_rows.append((row, value))
    if run_rows:
        windows.append(make_window(run_rows))

    return {
        "frames": len(nonzero),
        "sum": sum(value for _, value in nonzero),
        "max": max(value for _, value in nonzero),
        "maxWitness": witness(*next(entry for entry in nonzero if entry[1] == max(value for _, value in nonzero))),
        "first": witness(*nonzero[0]),
        "last": witness(*nonzero[-1]),
        "windowCount": len(windows),
        "windows": windows[:MAX_WINDOWS_SHOWN],
        "windowsTruncated": len(windows) > MAX_WINDOWS_SHOWN,
    }


def make_window(entries: list[tuple[dict[str, Any], int]]) -> dict[str, Any]:
    values = [value for _, value in entries]
    first_row, first_value = entries[0]
    last_row, last_value = entries[-1]
    return {
        "first": witness(first_row, first_value),
        "last": witness(last_row, last_value),
        "rows": len(entries),
        "sum": sum(values),
        "max": max(values),
        "maxWitness": witness(*next(entry for entry in entries if entry[1] == max(values))),
    }


def mask_summary(rows: list[dict[str, Any]], counter_index: int) -> dict[str, Any]:
    masks = [row["diagnosticCounters"][counter_index] for row in rows]
    combined_or = 0
    bits: dict[int, list[tuple[dict[str, Any], int]]] = {}
    for row, mask in zip(rows, masks):
        combined_or |= mask
        bit = 0
        remaining = mask
        while remaining:
            if remaining & 1:
                bits.setdefault(bit, []).append((row, mask))
            remaining >>= 1
            bit += 1
    return {
        "or": combined_or,
        "nonzeroFrames": sum(mask != 0 for mask in masks),
        "bitOccurrences": [
            {
                "bit": bit,
                "frames": len(entries),
                "first": witness(entries[0][0], entries[0][1]),
                "last": witness(entries[-1][0], entries[-1][1]),
            }
            for bit, entries in sorted(bits.items())
        ],
    }


def analyze_report(path: Path, backend_label: str, counter_names: list[str]) -> dict[str, Any]:
    report = read_json(path)
    evidence = report.get("completedFrameEvidence") or {}
    rows = evidence.get("rows") or []
    errors: list[str] = []
    if len(rows) > MAX_ROWS:
        raise ValueError(f"row count exceeds bounded {MAX_ROWS}-row limit: {path}")
    if report.get("schema") != 2:
        errors.append(f"outer schema is {report.get('schema')!r}, expected 2")
    if report.get("status") != "complete" or evidence.get("status") != "complete":
        errors.append("native or completed-frame report is not marked complete")
    if len(rows) != report.get("measuredFrames") or len(rows) != 600:
        errors.append(f"row count {len(rows)} does not match a 600-row measured run")

    counter_length_errors: list[dict[str, Any]] = []
    diagnostic_status_counts: dict[str, int] = {}
    identity_mismatches: list[dict[str, Any]] = []
    nonmonotonic_submissions: list[int] = []
    nonmonotonic_completions: list[int] = []
    previous_submission = 0
    previous_completion = 0
    for expected_index, row in enumerate(rows):
        if row.get("index") != expected_index:
            errors.append(f"row position {expected_index} has index {row.get('index')!r}")
        status = str(row.get("diagnosticStatus"))
        diagnostic_status_counts[status] = diagnostic_status_counts.get(status, 0) + 1
        counters = row.get("diagnosticCounters")
        if status == "valid":
            if not isinstance(counters, list) or len(counters) != len(counter_names):
                counter_length_errors.append({"index": row.get("index"), "length": len(counters) if isinstance(counters, list) else None})
            elif any(not isinstance(v, int) or isinstance(v, bool) or v < 0 or v > 0xFFFFFFFF for v in counters):
                counter_length_errors.append({"index": row.get("index"), "length": "non-uint32-like value"})
        elif counters is not None:
            counter_length_errors.append({"index": row.get("index"), "length": "non-null counters for unavailable status"})

        submitted = row.get("submittedIdentity") or {}
        completed = row.get("completionIdentity") or {}
        if any(submitted.get(field) != completed.get(field) for field in IDENTITY_FIELDS):
            identity_mismatches.append({"index": row.get("index"), "submitted": submitted, "completion": completed})
        submission = submitted.get("submissionSerial", 0)
        completion = completed.get("completionSerial", 0)
        if not isinstance(submission, int) or submission <= previous_submission:
            nonmonotonic_submissions.append(expected_index)
        if not isinstance(completion, int) or completion <= previous_completion:
            nonmonotonic_completions.append(expected_index)
        previous_submission = submission if isinstance(submission, int) else previous_submission
        previous_completion = completion if isinstance(completion, int) else previous_completion

    if counter_length_errors:
        errors.append(f"counter shape/status errors in {len(counter_length_errors)} rows")
    if identity_mismatches:
        errors.append(f"submitted/completion identity mismatch in {len(identity_mismatches)} rows")
    if nonmonotonic_submissions:
        errors.append(f"non-monotonic submitted identity at {len(nonmonotonic_submissions)} rows")
    if nonmonotonic_completions:
        errors.append(f"non-monotonic completion identity at {len(nonmonotonic_completions)} rows")

    valid_rows = [row for row in rows if row.get("diagnosticStatus") == "valid" and isinstance(row.get("diagnosticCounters"), list) and len(row["diagnosticCounters"]) == len(counter_names)]
    stats: dict[str, Any] = {}
    for name in WINDOW_NAMES:
        index = counter_names.index(name)
        summary = counter_windows(valid_rows, index)
        if summary["frames"]:
            category = "failure" if name in FAIL_COUNTER_NAMES else "boundedEvent" if name in BOUNDED_EVENT_NAMES else "certifiedRecovery" if name in RECOVERY_NAMES else "tir"
            stats[name] = {"category": category, "index": index, **summary}
    masks = {name: mask_summary(valid_rows, counter_names.index(name)) for name in MASK_NAMES}

    result_path = PHONE_DIR / "result.json" if backend_label == "Android-RayTracingPipeline" else None
    result = read_json(result_path) if result_path and result_path.exists() else None
    if backend_label == "Android-RayTracingPipeline":
        if not isinstance(result, dict) or result.get("runId") != report.get("runId") or result.get("status") != "complete":
            errors.append("phone result marker does not confirm this report run as complete")

    return {
        "label": backend_label,
        "source": str(path),
        "resultMarker": result,
        "metadata": {
            "schema": report.get("schema"),
            "status": report.get("status"),
            "workload": report.get("workload"),
            "runId": report.get("runId"),
            "timestampUtc": report.get("timestampUtc"),
            "executionBackend": report.get("executionBackend"),
            "rtMode": report.get("rtMode"),
            "gpu": report.get("gpu"),
            "vulkanApi": report.get("vulkanApi"),
            "renderScalePercent": report.get("renderScalePercent"),
            "internalExtent": report.get("internalExtent"),
            "presentationExtent": report.get("presentationExtent"),
            "shaderArtifactIdentity": report.get("shader"),
            "lapsCompleted": report.get("lapsCompleted"),
            "lapsRequested": report.get("lapsRequested"),
            "measuredFrames": report.get("measuredFrames"),
            "presentedEveryFrame": report.get("presentedEveryFrame"),
        },
        "ledger": {
            "expected": evidence.get("counts", {}).get("expected"),
            "completed": evidence.get("counts", {}).get("completed"),
            "rejected": evidence.get("counts", {}).get("rejected"),
            "cancelled": evidence.get("counts", {}).get("cancelled"),
            "cpuAccepted": evidence.get("counts", {}).get("cpuAccepted"),
            "gpuValid": evidence.get("gpuStatusCounts", {}).get("valid"),
            "diagnosticStatusCounts": diagnostic_status_counts,
            "validCounterRows": len(valid_rows),
            "counterLengthOrAvailabilityErrors": counter_length_errors[:10],
            "rows": len(rows),
            "identitiesEqualSubmittedToCompletion": not identity_mismatches,
            "identityMismatchCount": len(identity_mismatches),
            "submittedSerialStrictlyIncreasing": not nonmonotonic_submissions,
            "completionSerialStrictlyIncreasing": not nonmonotonic_completions,
            "nonmonotonicSubmittedRows": nonmonotonic_submissions[:10],
            "nonmonotonicCompletionRows": nonmonotonic_completions[:10],
            "firstIdentity": rows[0].get("completionIdentity") if rows else None,
            "lastIdentity": rows[-1].get("completionIdentity") if rows else None,
        },
        "nonzeroCounterWindows": stats,
        "maskBitEvidence": masks,
        "validationErrors": errors,
    }


def main() -> int:
    counter_names = counter_names_from_abi()
    inputs = (
        (ROOT / "windows-high-pipeline/HordeLanternRT-benchmark-20260929-235544.json", "Windows-RayTracingPipeline"),
        (ROOT / "windows-high-compute/HordeLanternRT-benchmark-20260929-235715.json", "Windows-RayQueryCompute"),
        (PHONE_DIR / "benchmark.json", "Android-RayTracingPipeline"),
    )
    reports = [analyze_report(path, label, counter_names) for path, label in inputs]
    context_path = ROOT / "context.json"
    context = read_json(context_path) if context_path.exists() else {"missing": str(context_path)}
    context_samples = context.get("samples", [])
    temperatures = []
    for sample in context_samples:
        match = re.search(r"temperature:\s*(\d+)", str(sample.get("battery", "")))
        if match:
            temperatures.append(int(match.group(1)))
    output = {
        "analysis": "live bounded diagnostic counters; no visual/performance/parity pass inferred",
        "counterSchema": {"header": str(ABI), "count": len(counter_names), "fieldOrder": counter_names},
        "phoneWarmContext": {
            "source": str(context_path),
            "runId": context.get("runId"),
            "workload": context.get("workload"),
            "startUtc": context.get("startUtc"),
            "endUtc": context.get("endUtc"),
            "recordingEnabled": context.get("recordingEnabled"),
            "sampleCount": len(context_samples),
            "firstSample": context_samples[0] if context_samples else None,
            "lastSample": context_samples[-1] if context_samples else None,
            "batteryTemperatureRawMinMax": [min(temperatures), max(temperatures)] if temperatures else None,
            "thermalStatuses": sorted({str(s.get("thermal")) for s in context_samples}),
            "gpuThermalPowerLevels": sorted({str(s.get("gpuThermalPowerLevel")) for s in context_samples}),
        },
        "runs": reports,
        "caveats": [
            "Counter arrays are per completed measured submission; only counters declared in the generated ABI are named.",
            "Reason masks are combined by bitwise OR and per-bit frame occurrence, never arithmetic sum.",
            "Complete ledger status does not establish glass visual acceptance, backend parity, or Shipping performance.",
        ],
    }
    json.dump(output, sys.stdout, indent=2, sort_keys=False)
    sys.stdout.write("\n")
    return 1 if any(run["validationErrors"] for run in reports) else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # keep failures concise and reproducible
        print(f"analyze-live: {exc}", file=sys.stderr)
        raise SystemExit(2)
