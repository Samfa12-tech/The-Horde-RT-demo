#!/usr/bin/env python3
"""Read typed current-HAL temperatures; never substitute cached thermalservice rows.

This is offline evidence/admission only. It does not control the phone, establish
GPU clocks or power, or make matched starting temperatures a sustained-FPS pass.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import re

MAX_DUMP_BYTES = 256 * 1024
REQUIRED_SENSORS = {"AP": 0, "BAT": 2, "SKIN": 3}
TEMP = re.compile(r"^Temperature\{mValue=([^,]+), mType=(-?\d+), mName=([^,]+), mStatus=(\d+)\}$")

class ThermalEvidenceError(ValueError):
    pass

def parse_current_thermal_state(text: str) -> dict:
    if len(text.encode("utf-8")) > MAX_DUMP_BYTES:
        raise ThermalEvidenceError("thermal dump exceeds the evidence bound")
    lines = text.splitlines()
    statuses = [re.fullmatch(r"Thermal Status: (\d+)", line.strip()) for line in lines]
    statuses = [int(match.group(1)) for match in statuses if match]
    if len(statuses) != 1 or not 0 <= statuses[0] <= 6:
        raise ThermalEvidenceError("one valid framework Thermal Status is required")
    headers = [i for i, line in enumerate(lines) if line.strip() == "Current temperatures from HAL:"]
    if len(headers) != 1:
        raise ThermalEvidenceError("one current-HAL section is required; cached rows are not a substitute")
    sensors = {}
    for line in lines[headers[0] + 1:]:
        stripped = line.strip()
        if stripped and not line[:1].isspace():
            break
        match = TEMP.fullmatch(stripped)
        if not match:
            continue
        value, kind, name, status = match.groups()
        if name not in REQUIRED_SENSORS:
            continue
        if name in sensors:
            raise ThermalEvidenceError("duplicate current-HAL sensor: " + name)
        try:
            temperature = float(value)
        except ValueError as error:
            raise ThermalEvidenceError("invalid temperature for " + name) from error
        if not math.isfinite(temperature) or int(kind) != REQUIRED_SENSORS[name] or not 0 <= int(status) <= 6:
            raise ThermalEvidenceError("invalid typed current-HAL sensor: " + name)
        sensors[name] = {"type": int(kind), "celsius": temperature, "status": int(status)}
    if set(sensors) != set(REQUIRED_SENSORS):
        raise ThermalEvidenceError("missing required current-HAL sensor(s): " + ", ".join(sorted(set(REQUIRED_SENSORS) - set(sensors))))
    return {"schemaVersion": 1, "temperatureSource": "current HAL", "frameworkStatus": statuses[0],
            "sensors": sensors, "limits": "Starting-state evidence only; no GPU clock, power, sustained or causal claim."}

def admit_start(current: dict, reference: dict | None = None, max_delta_c: float = 1.5,
                required_status: int = 0, maxima: dict | None = None) -> dict:
    if not math.isfinite(max_delta_c) or max_delta_c < 0 or not 0 <= required_status <= 6:
        raise ThermalEvidenceError("invalid admission limits")
    reasons = []
    if current["frameworkStatus"] != required_status:
        reasons.append("framework thermal status differs from the required starting status")
    for name, ceiling in (maxima or {}).items():
        if name not in REQUIRED_SENSORS or not math.isfinite(ceiling):
            raise ThermalEvidenceError("invalid sensor ceiling")
        if current["sensors"][name]["celsius"] > ceiling:
            reasons.append(name + " exceeds its declared starting ceiling")
    if reference is not None:
        if not isinstance(reference, dict) or reference.get("schemaVersion") != 1 or reference.get("temperatureSource") != "current HAL":
            raise ThermalEvidenceError("reference must contain typed current-HAL evidence")
        if reference.get("frameworkStatus") != current["frameworkStatus"]:
            reasons.append("framework starting status differs from reference")
        if not isinstance(reference.get("sensors"), dict) or set(reference["sensors"]) != set(REQUIRED_SENSORS):
            raise ThermalEvidenceError("reference sensor roster differs")
        for name, expected_type in REQUIRED_SENSORS.items():
            before = reference["sensors"][name]
            if not isinstance(before, dict) or type(before.get("type")) is not int or before["type"] != expected_type or type(before.get("celsius")) not in (int, float) or not math.isfinite(before["celsius"]):
                raise ThermalEvidenceError("invalid reference sensor: " + name)
            after = current["sensors"][name]
            if type(before.get("status")) is not int or not 0 <= before["status"] <= 6:
                raise ThermalEvidenceError("invalid reference status: " + name)
            if before["status"] != after["status"]:
                reasons.append(name + " starting status differs from reference")
            if abs(before["celsius"] - after["celsius"]) > max_delta_c:
                reasons.append(name + " exceeds the declared temperature delta from reference")
    return {"admitted": not reasons, "reasons": reasons, "requiredFrameworkStatus": required_status,
            "maximumDeltaCelsius": max_delta_c, "sensorCeilingsCelsius": maxima or {},
            "referenceCompared": reference is not None}

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dump", type=Path)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--max-delta-c", type=float, default=1.5)
    parser.add_argument("--required-status", type=int, default=0)
    parser.add_argument("--max-current", action="append", default=[], metavar="SENSOR=CELSIUS")
    args = parser.parse_args()
    try:
        if args.dump.stat().st_size > MAX_DUMP_BYTES:
            raise ThermalEvidenceError("thermal dump exceeds the evidence bound")
        current = parse_current_thermal_state(args.dump.read_text(encoding="utf-8-sig"))
        reference = None
        if args.reference:
            if args.reference.stat().st_size > 64 * 1024:
                raise ThermalEvidenceError("reference exceeds the evidence bound")
            reference = json.loads(args.reference.read_text(encoding="utf-8-sig"))
            if not isinstance(reference, dict):
                raise ThermalEvidenceError("reference must be a JSON object")
            reference = reference.get("state", reference)
        maxima = {}
        for item in args.max_current:
            name, separator, value = item.partition("=")
            if not separator or name in maxima:
                raise ThermalEvidenceError("unique SENSOR=CELSIUS ceilings required")
            maxima[name] = float(value)
        admission = admit_start(current, reference, args.max_delta_c, args.required_status, maxima)
        print(json.dumps({"state": current, "admission": admission}, indent=2, sort_keys=True))
        return 0 if admission["admitted"] else 2
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"admission": {"admitted": False}, "error": str(error)}))
        return 2

if __name__ == "__main__":
    raise SystemExit(main())
