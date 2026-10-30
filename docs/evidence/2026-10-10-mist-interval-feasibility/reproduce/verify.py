#!/usr/bin/env python3
"""Bounded CPU discriminator for the diagnostic mist interval GLSL algorithm.

This mirrors the insertion-sort/union and cell-overlap arithmetic in the
prototype shader. It deliberately does not launch Vulkan or claim GPU output.
"""
from __future__ import annotations

import json
import hashlib
import math
import random
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
FIXTURE = Path(__file__).with_name("fixtures.json")
OUT = ROOT / "reports" / "mist-interval-feasibility" / "correctness.json"
PROTOTYPE = ROOT / "docs" / "evidence" / "2026-10-10-mist-interval-prototype"
PROTOTYPE_REPORT = ROOT / "reports" / "mist-interval-prototype"


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", float(value)))[0]


def add3(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub3(a, b):
    return tuple(x - y for x, y in zip(a, b))


def mul3(a, scale):
    return tuple(x * scale for x in a)


def dot3(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross3(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm3(a):
    scale = math.sqrt(dot3(a, a))
    return tuple(x / scale for x in a)


def cone_planes_double(triangle, light):
    a, b, c = triangle
    normal = cross3(sub3(b, a), sub3(c, a))
    if dot3(normal, sub3(light, a)) < 0.0:
        normal = mul3(normal, -1.0)
    rows = [(mul3(normal, -1.0), dot3(normal, a))]
    for x, y, other in ((a, b, c), (b, c, a), (c, a, b)):
        edge = cross3(sub3(x, light), sub3(y, light))
        if dot3(edge, sub3(other, light)) < 0.0:
            edge = mul3(edge, -1.0)
        rows.append((edge, -dot3(edge, light)))
    return [(n[0], n[1], n[2], w) for n, w in rows]


def clip_planes_double(origin, direction, first, last, planes):
    lo, hi = first, last
    for p in planes:
        intercept = dot3(p[:3], origin) + p[3]
        slope = dot3(p[:3], direction)
        if abs(slope) < 1e-12:
            if intercept < 0.0:
                return None
        elif slope > 0.0:
            lo = max(lo, -intercept / slope)
        else:
            hi = min(hi, -intercept / slope)
        if hi <= lo:
            return None
    return [lo, hi]


def clip_cone_float32(origin, direction, first, last, planes):
    """Mirror intervals.glsl's four-plane float arithmetic and parallel test."""
    lo, hi = f32(first), f32(last)
    for p in planes:
        products = [f32(f32(p[i]) * f32(origin[i])) for i in range(3)]
        intercept = f32(f32(f32(products[0] + products[1]) + products[2]) + f32(p[3]))
        products = [f32(f32(p[i]) * f32(direction[i])) for i in range(3)]
        slope = f32(f32(products[0] + products[1]) + products[2])
        if abs(slope) < 1e-8:
            if intercept < 0.0:
                return None
        elif slope > 0.0:
            lo = f32(max(lo, f32(f32(-intercept) / slope)))
        else:
            hi = f32(min(hi, f32(f32(-intercept) / slope)))
        if hi <= lo:
            return None
    return [lo, hi]


def box_interval(origin, direction, box_min, box_max, scene_depth):
    first, last = -math.inf, math.inf
    for axis in range(3):
        if abs(direction[axis]) < 1e-12:
            if not box_min[axis] <= origin[axis] <= box_max[axis]:
                return None
            continue
        a = (box_min[axis] - origin[axis]) / direction[axis]
        b = (box_max[axis] - origin[axis]) / direction[axis]
        first = max(first, min(a, b))
        last = min(last, max(a, b))
    first = max(first, 0.0)
    last = min(last, scene_depth)
    return None if last <= first + 0.01 else [first, last]


def ray_hits_bounds(origin, direction, first, last, bounds):
    lo, hi = first, last
    lower, upper = bounds
    for axis in range(3):
        if abs(direction[axis]) < 1e-12:
            if not lower[axis] <= origin[axis] <= upper[axis]:
                return False
            continue
        a = (lower[axis] - origin[axis]) / direction[axis]
        b = (upper[axis] - origin[axis]) / direction[axis]
        lo, hi = max(lo, min(a, b)), min(hi, max(a, b))
        if hi < lo:
            return False
    return hi >= lo


def shader_cell_bounds(origin, direction, first, last, index, sample_count):
    """Mirror samplePosition construction and dot-projected cell center."""
    step = f32(f32(f32(last) - f32(first)) / f32(sample_count))
    sample_t = f32(f32(first) + f32(step * f32(index + 0.5)))
    return shader_cell_bounds_from_sample_t(origin, direction, sample_t, step)


def shader_cell_bounds_from_sample_t(origin, direction, sample_t, step):
    sample_position = tuple(f32(f32(origin[a]) + f32(f32(direction[a]) * sample_t)) for a in range(3))
    delta = tuple(f32(sample_position[a] - f32(origin[a])) for a in range(3))
    products = [f32(delta[a] * f32(direction[a])) for a in range(3)]
    centre = f32(f32(products[0] + products[1]) + products[2])
    half = f32(step * f32(0.5))
    return sample_t, f32(centre - half), f32(centre + half)


def coverage_schedule(intervals, origin, direction, first, last, sample_count):
    result = []
    for index in range(sample_count):
        sample_t, lo, hi = shader_cell_bounds(origin, direction, first, last, index, sample_count)
        result.append({"sample_t": sample_t, "bounds": [lo, hi], "coverage": glsl_cell_coverage(intervals, lo, hi)})
    return result


def scattering_unrolled(first, last, intervals, densities, origin, direction):
    """Mirror the original fixed 2/6/8 invocation schedules, in source order."""
    n = len(densities)
    offsets = {2: (0.5, 1.5), 6: (0.5, 1.5, 2.5, 3.5, 4.5, 5.5),
               8: (0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5)}[n]
    step = f32(f32(f32(last) - f32(first)) / f32(n))
    scattered = 0.0
    transmittance = 1.0
    centers, coverages = [], []
    for index, offset in enumerate(offsets):
        center_t = f32(f32(first) + f32(step * f32(offset)))
        center, lo, hi = shader_cell_bounds_from_sample_t(origin, direction, center_t, step)
        coverage = glsl_cell_coverage(intervals, lo, hi)
        rho = f32(densities[index])
        # Sky interval coverage affects sky illumination only; the fixed extra
        # term stands for unchanged staff/fire visibility at that sample.
        sample_rgb = f32(f32(f32(1.0 - coverage) + f32(0.17)) * f32(rho * f32(0.82)))
        scattered = f32(scattered + f32(f32(transmittance * sample_rgb) * step))
        transmittance = f32(transmittance * f32(math.exp(-f32(rho * step))))
        centers.append(center)
        coverages.append(coverage)
    return {"centers": centers, "coverages": coverages, "scattered": scattered, "transmittance": transmittance}


def scattering_common_loop(first, last, intervals, densities, origin, direction):
    """Mirror the proposed common ordered-cell loop with identical operation order."""
    n = len(densities)
    step = f32(f32(f32(last) - f32(first)) / f32(n))
    scattered = f32(0.0)
    transmittance = f32(1.0)
    centers, coverages = [], []
    for index in range(n):
        center, lo, hi = shader_cell_bounds(origin, direction, first, last, index, n)
        coverage = glsl_cell_coverage(intervals, lo, hi)
        rho = f32(densities[index])
        sample_rgb = f32(f32(f32(1.0 - coverage) + f32(0.17)) * f32(rho * f32(0.82)))
        scattered = f32(scattered + f32(f32(transmittance * sample_rgb) * step))
        transmittance = f32(transmittance * f32(math.exp(-f32(rho * step))))
        centers.append(center)
        coverages.append(coverage)
    return {"centers": centers, "coverages": coverages, "scattered": scattered, "transmittance": transmittance}


def camera_ray(camera, x, y, width, height):
    origin = tuple(f32(v) for v in camera[:3])
    yaw, pitch = camera[3:]
    forward = norm3((math.sin(yaw), -0.05 + pitch, -math.cos(yaw)))
    right = norm3(cross3(forward, (0.0, 1.0, 0.0)))
    up = norm3(cross3(right, forward))
    h = ((y + 0.5) / height * 2.0 - 1.0) * -0.74
    w = ((x + 0.5) / width * 2.0 - 1.0) * width / height
    direction = norm3(add3(add3(mul3(forward, 1.22), mul3(right, w)), mul3(up, h)))
    return origin, tuple(f32(v) for v in direction)


def actual_frozen_discriminators():
    state_path = PROTOTYPE / "captures" / "current-baseline-5" / "frozen-state.json"
    cone_state_path = ROOT / "docs" / "evidence" / "2026-10-10-frozen-mist-attribution" / "extension" / "owner-view" / "current-midpoint-all" / "frozen-state.json"
    depth_dir = PROTOTYPE_REPORT / "captures" / "current-depth-3"
    depth_path = depth_dir / "192-rescue-journey-start.png"
    depth_state_path = depth_dir / "frozen-state.json"
    assert state_path.read_bytes() == depth_state_path.read_bytes(), "depth capture frozen state differs"
    state = json.loads(state_path.read_text(encoding="utf-8"))
    cone_state = json.loads(cone_state_path.read_text(encoding="utf-8"))
    assert state["triangles"] == cone_state["triangles"], "cone input and depth camera frozen ropes differ"
    depth_run = json.loads((depth_dir / "run.json").read_text(encoding="utf-8"))
    depth_frame = json.loads((depth_dir / "completed-frame.json").read_text(encoding="utf-8"))
    assert depth_run["shader"]["name"] == "depth"
    assert depth_frame["presentation"]["presented"] is True
    assert depth_frame["presentation"]["finalIdleCompletion"] is True
    assert depth_frame["pipeline"]["executionBackend"] == "RayTracingPipeline"
    assert depth_frame["pipeline"]["activeSha256"] == depth_run["shader"]["spirv_sha256"]
    cones_doc = json.loads((PROTOTYPE / "cones.json").read_text(encoding="utf-8"))
    cones_path = PROTOTYPE / "cones.json"
    depth_image = Image.open(depth_path).convert("RGBA")
    width, height = depth_image.size
    depth_raw = depth_image.tobytes("raw", "BGRA")
    lights = cones_doc["lights"]
    by_source = {source: [] for source in range(len(lights))}
    for cone in cones_doc["cones"]:
        by_source[cone["source"]].append(cone)
    assert cones_doc["casters"] == len(state["triangles"]) // 3
    assert all(len(by_source[source]) == 128 for source in range(len(lights)))
    triangles = state["triangles"]
    box_min, box_max = (-36.65, -0.95, -18.15), (-30.65, 0.20, -12.25)
    ray_roster = [[838, 574], [915, 513], [850, 556], [465, 286], [478, 305], [530, 315]]
    samples_by_quality = {"lean": 2, "current": 6, "higher": 8}
    records, failures = [], []
    max_endpoint_error = max_cell_error = max_cell_error_bound = 0.0

    for x, y in ray_roster:
        offset = (y * width + x) * 4
        scene_depth = struct.unpack_from("<f", depth_raw, offset)[0]
        origin, direction = camera_ray(state["camera"], x, y, width, height)
        ray_range = box_interval(origin, direction, box_min, box_max, scene_depth)
        assert ray_range is not None, (x, y, scene_depth, "ray misses active mist segment")
        first, last = ray_range
        aabb_candidates = [c for c in cones_doc["cones"] if ray_hits_bounds(origin, direction, first, last, c["bounds"])]
        for source, light in enumerate(lights):
            shader_raw, oracle_raw = [], []
            primitive_records = []
            for cone in by_source[source]:
                primitive = cone["primitive"]
                triangle = triangles[primitive * 3:primitive * 3 + 3]
                shader_interval = clip_cone_float32(origin, direction, first, last, cone["planes"])
                exact_planes = cone_planes_double(triangle, light)
                oracle_interval = clip_planes_double(origin, direction, first, last, exact_planes)
                if (shader_interval is not None or oracle_interval is not None) and not ray_hits_bounds(origin, direction, first, last, cone["bounds"]):
                    failures.append(f"frozen ray {(x,y)} source {source} primitive {primitive}: conservative bound missed exact interval")
                if shader_interval is not None:
                    shader_raw.append(shader_interval)
                if oracle_interval is not None:
                    oracle_raw.append(oracle_interval)
                if (shader_interval is None) != (oracle_interval is None):
                    primitive_records.append({"primitive": primitive, "shader": shader_interval, "oracle": oracle_interval})
                elif shader_interval is not None:
                    endpoint_error = max(abs(a - b) for a, b in zip(shader_interval, oracle_interval))
                    max_endpoint_error = max(max_endpoint_error, endpoint_error)
                    if endpoint_error > 0.00005:
                        primitive_records.append({"primitive": primitive, "shader": shader_interval, "oracle": oracle_interval, "endpoint_error": endpoint_error})

            shader_union = glsl_union(shader_raw)
            oracle_union = canonical_union(oracle_raw)
            union_error = None
            if len(shader_union) != len(oracle_union):
                union_error = {"shader_union_count": len(shader_union), "oracle_union_count": len(oracle_union)}
                failures.append(f"frozen ray {(x,y)} source {source}: union interval count differs")
            else:
                union_error = max((abs(a - b) for got, expected in zip(shader_union, oracle_union) for a, b in zip(got, expected)), default=0.0)
                max_endpoint_error = max(max_endpoint_error, union_error)
                if union_error > 0.00005:
                    failures.append(f"frozen ray {(x,y)} source {source}: union endpoint error {union_error}")
            if primitive_records:
                failures.append(f"frozen ray {(x,y)} source {source}: {len(primitive_records)} per-triangle interval mismatches")

            coverage_by_quality = {}
            for quality, sample_count in samples_by_quality.items():
                step = (last - first) / sample_count
                float_cells = []
                oracle_cells = []
                cell_bounds = []
                for i in range(sample_count):
                    _, lo, hi = shader_cell_bounds(origin, direction, first, last, i, sample_count)
                    actual = glsl_cell_coverage(shader_union, lo, hi)
                    # Use the exact float32 projected cell bounds for both
                    # algorithms, isolating cone interval clipping error.
                    expected = oracle_coverage(oracle_union, lo, hi)
                    float_cells.append(actual)
                    oracle_cells.append(expected)
                    cell_bounds.append([lo, hi])
                cell_error = max((abs(a - b) for a, b in zip(float_cells, oracle_cells)), default=0.0)
                interval_error_bound = max((min(1.0, 2.0 * (union_error if isinstance(union_error, float) else 0.0) / (hi - lo) + 2e-6) for lo, hi in cell_bounds), default=2e-6)
                max_cell_error = max(max_cell_error, cell_error)
                max_cell_error_bound = max(max_cell_error_bound, interval_error_bound)
                # Retain the original strict 2e-5 cell discriminator. A derived
                # roundoff explanation is analysis, not permission to replace
                # a failed acceptance threshold with a larger allowance.
                if cell_error > 0.00002:
                    failures.append(f"frozen ray {(x,y)} source {source} {quality}: cell error {cell_error} exceeds fixed 2e-5 discriminator")
                coverage_by_quality[quality] = {"shader_mirror": float_cells, "independent_double_oracle": oracle_cells, "max_abs_error": cell_error, "fixed_cell_error_limit": 0.00002, "endpoint_error_derived_bound_analysis_only": interval_error_bound}

            records.append({
                "pixel": [x, y], "source_id": source, "scene_depth": scene_depth,
                "mist_segment": ray_range, "candidate_cones_for_source": len(by_source[source]),
                "cpu_ray_aabb_candidates_all_sources": len(aabb_candidates),
                "cpu_ray_aabb_candidates_selected_source": sum(c["source"] == source for c in aabb_candidates),
                "float32_raw_count": len(shader_raw), "double_oracle_raw_count": len(oracle_raw),
                "float32_union": shader_union, "double_oracle_union": oracle_union,
                "coverage_by_quality": coverage_by_quality,
                "per_primitive_interval_mismatches": primitive_records,
            })

    # Equal-length displacement based on a real frozen-ray union. Shift by one
    # 8-cell interval so endpoint/cell checks cannot collapse to the old length.
    witness = next(r for r in records if r["source_id"] == 0 and r["float32_union"])
    aperture_one_witness = next(r for r in records if r["source_id"] == 1 and r["float32_union"])
    first, last = witness["mist_segment"]
    shift = (last - first) / 8.0
    original = witness["float32_union"]
    displaced = [[a + shift, b + shift] for a, b in original]
    assert displaced[-1][1] < last, "displacement would leave frozen mist segment"
    origin, direction = camera_ray(state["camera"], *witness["pixel"], width, height)
    original_cells = [v["coverage"] for v in coverage_schedule(original, origin, direction, first, last, 8)]
    displaced_cells = [v["coverage"] for v in coverage_schedule(displaced, origin, direction, first, last, 8)]
    displacement = {
        "pixel": witness["pixel"], "source_id": witness["source_id"], "shift": shift,
        "original_union_length": sum(b - a for a, b in original),
        "displaced_union_length": sum(b - a for a, b in displaced),
        "original_endpoints": original, "displaced_endpoints": displaced,
        "original_cells": original_cells, "displaced_cells": displaced_cells,
        "endpoint_vectors_differ": original != displaced,
        "cell_vectors_differ": original_cells != displaced_cells,
    }
    assert abs(displacement["original_union_length"] - displacement["displaced_union_length"]) < 1e-10
    assert displacement["endpoint_vectors_differ"] and displacement["cell_vectors_differ"]
    return {
        "classification": "CPU float32 cone clipping vs independent double triangle-derived clipping on frozen inputs; not GPU endpoint readback",
        "source_artifacts": {
            "frozen_state": str(state_path.relative_to(ROOT)),
            "cone_geometry_state": str(cone_state_path.relative_to(ROOT)),
            "depth_image": str(depth_path.relative_to(ROOT)),
            "depth_completed_frame": str((depth_dir / "completed-frame.json").relative_to(ROOT)),
            "cone_planes": "docs/evidence/2026-10-10-mist-interval-prototype/cones.json",
            "frozen_state_byte_identical_to_depth_capture": True,
            "cone_input_triangles_equal_depth_capture_triangles": True,
            "camera_resolution": [width, height],
            "sha256": {
                "frozen_state": sha256_file(state_path),
                "depth_image": sha256_file(depth_path),
                "cones_json": sha256_file(cones_path),
                "depth_completed_frame": sha256_file(depth_dir / "completed-frame.json"),
            },
        },
        "depth_frame_identity": {
            "presented": True, "final_idle_completion": True, "backend": "RayTracingPipeline",
            "source_head": depth_run["source_head"], "executable_sha256": depth_run["exe_sha256"],
            "active_spirv_sha256": depth_frame["pipeline"]["activeSha256"],
        },
        "lights": lights,
        "ray_count": len(ray_roster), "source_ray_records": len(records),
        "nonempty_source_witnesses": {"source_0": witness["pixel"], "source_1": aperture_one_witness["pixel"]},
        "quality_sample_counts": samples_by_quality,
        "maximum_raw_or_union_endpoint_error_metres": max_endpoint_error,
        "maximum_cell_coverage_error": max_cell_error,
        "maximum_cell_coverage_error_bound": max_cell_error_bound,
        "displaced_equal_length_actual_ray_control": displacement,
        "passed": not failures, "failures": failures, "records": records,
    }


def ordered_scattering_equivalence():
    """Compare fixed calls and a loop with aperture-selected interval masks."""
    rng = random.Random(0x4D157)
    rows, failures = [], []
    pattern_counts = {"disjoint": 0, "overlapping": 0, "thin": 0}
    for case_index in range(48):
        first = f32(0.25 + rng.random() * 3.0)
        last = f32(first + 2.0 + rng.random() * 8.0)
        raw_by_source = {0: [], 1: []}
        pattern = ("disjoint", "overlapping", "thin")[case_index % 3]
        pattern_counts[pattern] += 1
        for source in (0, 1):
            span = last - first
            if pattern == "disjoint":
                for fraction in (0.16, 0.57):
                    start = first + span * (fraction + rng.random() * 0.025)
                    width = span * (0.035 + rng.random() * 0.02)
                    raw_by_source[source].append([start, min(last, start + width)])
            elif pattern == "overlapping":
                start = first + span * (0.25 + rng.random() * 0.05)
                raw_by_source[source].extend([[start, start + span * 0.25], [start + span * 0.1, start + span * 0.35]])
            else:
                start = first + span * (0.25 + rng.random() * 0.4)
                width = span * (0.0001 + rng.random() * 0.0003)
                raw_by_source[source].append([start, min(last, start + width)])
        for source_id in (0, 1):
            intervals = glsl_union(raw_by_source[source_id])
            for n in (2, 6, 8):
                densities = [f32(0.06 + 0.11 * (0.5 + 0.5 * math.sin((case_index + 1) * (i + 0.7) * 0.31))) for i in range(n)]
                ray_origin = (f32(-34.4), f32(0.7), f32(-13.5))
                ray_direction = (f32(0.8), f32(0.48), f32(0.36))
                original = scattering_unrolled(first, last, intervals, densities, ray_origin, ray_direction)
                common = scattering_common_loop(first, last, intervals, densities, ray_origin, ray_direction)
                passed = original == common
                if not passed:
                    failures.append({"case": case_index, "source_id": source_id, "sample_count": n, "original": original, "common": common})
                rows.append({"case": case_index, "source_id": source_id, "pattern": pattern, "sample_count": n,
                    "same_centers": original["centers"] == common["centers"],
                    "same_cell_coverage": original["coverages"] == common["coverages"],
                    "same_scattered": original["scattered"] == common["scattered"],
                    "same_transmittance": original["transmittance"] == common["transmittance"],
                    "pass": passed})
    return {
        "classification": "deterministic CPU float32 arithmetic equivalence for aperture-selected sky coverage plus a fixed extra-light term; not a full physical source/light model or GPU compilation",
        "seed": "0x4D157", "random_cases": 48, "source_ids": [0, 1],
        "sample_counts": [2, 6, 8], "pattern_counts": pattern_counts,
        "comparisons": len(rows), "passed": not failures, "failures": failures, "rows": rows,
    }


def glsl_union(raw: list[list[float]]) -> list[list[float]]:
    """Mirror GLSL insertion sort then in-place overlapping interval union."""
    values = [[f32(a), f32(b)] for a, b in raw]
    for i in range(1, len(values)):
        v = values[i][:]
        j = i - 1
        while j >= 0 and values[j][0] > v[0]:
            values[j + 1] = values[j]
            j -= 1
        values[j + 1] = v
    merged: list[list[float]] = []
    for a, b in values:
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return merged


def glsl_cell_coverage(intervals: list[list[float]], lo: float, hi: float) -> float:
    length = f32(f32(hi) - f32(lo))
    if not (length > 0.0 and math.isfinite(length)):
        raise ValueError("invalid cell length must reject")
    covered = f32(0.0)
    for a, b in intervals:
        overlap = f32(max(0.0, f32(min(f32(hi), b) - max(f32(lo), a))))
        covered = f32(covered + overlap)
    return f32(min(1.0, max(0.0, f32(covered / length))))


def canonical_union(raw: list[list[float]]) -> list[list[float]]:
    """Independent double-precision oracle for exact expected endpoints."""
    values = sorted((float(a), float(b)) for a, b in raw)
    merged: list[list[float]] = []
    for a, b in values:
        if not (math.isfinite(a) and math.isfinite(b) and b > a):
            raise ValueError(f"invalid interval [{a}, {b}]")
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return merged


def oracle_coverage(intervals: list[list[float]], lo: float, hi: float) -> float:
    return sum(max(0.0, min(hi, b) - max(lo, a)) for a, b in intervals) / (hi - lo)


def expand(case: dict) -> dict:
    """Expand compact deterministic boundary fixtures into callback records."""
    case = dict(case)
    spec = case.pop("generated", None)
    if spec:
        n = spec["selected_raw"]
        other = spec.get("other_aperture_callbacks", 0)
        case["candidates"] = [
            {"aperture": 0, "interval": [2.0 * i, 2.0 * i + 1.0]}
            for i in range(n)
        ] + [
            {"aperture": 1, "interval": [2.0 * i, 2.0 * i + 0.5]}
            for i in range(other)
        ]
        case["expected_raw"] = [c["interval"] for c in case["candidates"] if c["aperture"] == case["selected_aperture"]]
    return case


def run() -> dict:
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    results = []
    failures = []

    for source_case in fixture["cases"]:
        case = expand(source_case)
        selected = case["selected_aperture"]
        candidates = case["candidates"]
        callback_count = len(candidates)
        if callback_count > 512:
            actual = []
            status = "rejected-callback-capacity"
        else:
            raw = [c["interval"] for c in candidates if c["aperture"] == selected]
            if len(raw) > 64:
                actual = []
                status = "rejected-raw-capacity"
            else:
                actual = glsl_union(raw)
                status = "valid"
        expected_status = case.get("expected_status", "valid")
        if status != expected_status:
            failures.append(f"{case['name']}: status {status} != {expected_status}")

        expected_raw = case.get("expected_raw")
        expected_union = canonical_union(expected_raw) if expected_raw is not None else None
        if status == "valid":
            if expected_union is not None:
                if len(actual) != len(expected_union) or any(
                    abs(x - y) > 2e-6
                    for got, want in zip(actual, expected_union)
                    for x, y in zip(got, want)
                ):
                    failures.append(f"{case['name']}: endpoint mismatch {actual} != {expected_union}")

            cell_rows = []
            for lo, hi in case.get("cells", []):
                got = glsl_cell_coverage(actual, lo, hi)
                want = oracle_coverage(expected_union, lo, hi) if expected_union is not None else 0.0
                passed = abs(got - want) <= 2e-6
                if not passed:
                    failures.append(f"{case['name']}: cell [{lo},{hi}] {got} != {want}")
                cell_rows.append({"cell": [lo, hi], "glsl_mirror": got, "oracle": want, "pass": passed})
        else:
            cell_rows = []

        results.append({
            "name": case["name"],
            "selected_aperture": selected,
            "callback_count": callback_count,
            "selected_raw_count": sum(c["aperture"] == selected for c in candidates),
            "status": status,
            "union_endpoints": actual,
            "expected_endpoints": expected_union,
            "cells": cell_rows,
        })

    # Explicitly establish that the old aggregate metric aliases displaced sets.
    a = canonical_union([[1.0, 2.0], [4.0, 5.0]])
    b = canonical_union([[2.0, 3.0], [5.0, 6.0]])
    length = lambda xs: sum(y - x for x, y in xs)
    aggregate_alias = length(a) == length(b) and a != b
    assert aggregate_alias, "displaced-interval counterexample no longer demonstrates the gap"
    discr_a = [glsl_cell_coverage(a, x, x + 1.0) for x in range(0, 7)]
    discr_b = [glsl_cell_coverage(b, x, x + 1.0) for x in range(0, 7)]
    assert discr_a != discr_b, "per-cell discriminator failed to distinguish equal-length displacement"
    invalid_lengths = []
    for lo, hi in [(1.0, 1.0), (2.0, 1.0), (0.0, math.nan), (0.0, math.inf)]:
        try:
            glsl_cell_coverage([], lo, hi)
        except ValueError:
            invalid_lengths.append({"cell": [str(lo), str(hi)], "rejected": True})
        else:
            invalid_lengths.append({"cell": [str(lo), str(hi)], "rejected": False})
            failures.append(f"invalid cell length [{lo},{hi}] was not rejected")

    frozen = actual_frozen_discriminators()
    if not frozen["passed"]:
        failures.extend(frozen["failures"])
    scattering = ordered_scattering_equivalence()
    if not scattering["passed"]:
        failures.extend(scattering["failures"])

    result = {
        "schema": 1,
        "classification": "bounded CPU reference, frozen-input geometry, and GLSL-arithmetic mirror; no GPU run",
        "source_glsl": "docs/evidence/2026-10-10-mist-interval-prototype/reproduce/intervals.glsl",
        "limits": {"raw_intervals": 64, "candidate_callbacks": 512},
        "case_count": len(results),
        "passed": not failures,
        "failures": failures,
        "displaced_equal_length_control": {
            "same_union_length": True,
            "left_endpoints": a,
            "right_endpoints": b,
            "left_cells": discr_a,
            "right_cells": discr_b,
            "distinguished_by_endpoints_and_cells": True,
        },
        "invalid_cell_length_controls": invalid_lengths,
        "frozen_real_input_discriminators": frozen,
        "ordered_scattering_common_loop_equivalence": scattering,
        "cases": results,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    output_path = OUT
    attempt = 2
    while output_path.exists():
        output_path = OUT.with_name(f"correctness-{attempt}.json")
        attempt += 1
    output_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    lines = [
        "# Mist interval feasibility correctness run",
        "",
        "CPU reference and float32 GLSL-arithmetic mirror only; no GPU was launched.",
        "",
        f"Result: **{'PASS' if result['passed'] else 'FAIL'}** ({len(results)} bounded fixtures; {frozen['source_ray_records']} frozen ray/source pairs; {scattering['comparisons']} scattering equivalence comparisons).",
        "",
        "| Fixture | Callbacks | Selected raw | Outcome |",
        "| --- | ---: | ---: | --- |",
    ]
    lines.extend(
        f"| {row['name']} | {row['callback_count']} | {row['selected_raw_count']} | {row['status']} |"
        for row in results
    )
    lines.extend([
        "",
        "The equal-length displaced control has distinct endpoints and per-cell vectors. Zero, negative, NaN, and infinite cell lengths reject.",
        f"Frozen CPU inputs: {frozen['ray_count']} predeclared pixels, both apertures (nonempty witnesses {frozen['nonempty_source_witnesses']}), and all 2/6/8 schedules; max endpoint error {frozen['maximum_raw_or_union_endpoint_error_metres']:.9g} m; max cell error {frozen['maximum_cell_coverage_error']:.9g}, endpoint-derived bound {frozen['maximum_cell_coverage_error_bound']:.9g}.",
        f"Depth identity: presented={frozen['depth_frame_identity']['presented']}, backend={frozen['depth_frame_identity']['backend']}, source={frozen['depth_frame_identity']['source_head']}, active SPIR-V SHA-256={frozen['depth_frame_identity']['active_spirv_sha256']}.",
        f"Common ordered loop equivalence: {scattering['comparisons']} deterministic randomized comparisons across aperture-selected interval sets and schedules 2/6/8; this is not a physical source model.",
        "GPU interval endpoints and GPU per-cell coverage remain unmeasured; the older union-length readback cannot distinguish displaced intervals.",
        "",
    ])
    output_path.with_suffix('.md').write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps({"passed": result["passed"], "cases": len(results), "output": str(output_path), "failures": result['failures']}, indent=2))
    return result


if __name__ == "__main__":
    result = run()
    sys.exit(0 if result["passed"] else 1)
