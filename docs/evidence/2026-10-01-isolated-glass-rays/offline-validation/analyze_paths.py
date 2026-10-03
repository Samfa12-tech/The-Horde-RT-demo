"""Offline validation of the isolated SM-S948B dielectric path capture.

This rechecks captured float object-space rays against the pinned runtime GLB
with Python double-precision Moller-Trumbore intersections. It corroborates
the captured hardware candidates; it does not emulate GPU traversal arithmetic.
Run from any directory with Python 3:
  python analyze_paths.py [path-analysis.json] [path-counter-comparison.json]
"""
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

GLB_PATH = Path(
    r"C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass"
    r"\assets\models\props\runtime\reward-lantern-body\reward-lantern-body-lod0.runtime.glb"
)
EXPECTED_GLB_SHA256 = "34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4"
DEFAULT_ROOT = Path(r"C:\Dev\tmp\horde-glass-isolated-20261001")
NEAREST_TIE_TOLERANCE = 1.0e-6
TMIN_COMPARISON_TOLERANCE = 1.0e-9
DETERMINANT_MINIMUM = 1.0e-15
BARYCENTRIC_TOLERANCE = 1.0e-9


def load_glb(path):
    blob = path.read_bytes()
    if hashlib.sha256(blob).hexdigest() != EXPECTED_GLB_SHA256:
        raise ValueError("Pinned GLB SHA-256 mismatch")
    magic, version, declared_length = struct.unpack_from("<III", blob)
    if (magic, version, declared_length) != (0x46546C67, 2, len(blob)):
        raise ValueError("Invalid GLB header/version/length")
    chunks = {}
    offset = 12
    while offset < len(blob):
        size, kind = struct.unpack_from("<II", blob, offset)
        start = offset + 8
        if start + size > len(blob):
            raise ValueError("GLB chunk exceeds file length")
        chunks[kind] = blob[start:start + size]
        offset = start + size
    document = json.loads(chunks[0x4E4F534A])
    binary = chunks[0x004E4942]
    node = next(n for n in document["nodes"] if n.get("name") == "LanternGlass")
    if any(k in node for k in ("rotation", "scale", "matrix")):
        raise ValueError("Unexpected LanternGlass node transform")
    primitive = document["meshes"][node["mesh"]]["primitives"][0]

    def accessor(index):
        a = document["accessors"][index]
        view = document["bufferViews"][a["bufferView"]]
        fmt, width = {5126: ("f", 4), 5123: ("H", 2), 5125: ("I", 4)}[
            a["componentType"]
        ]
        components = {"VEC3": 3, "SCALAR": 1}[a["type"]]
        start = view.get("byteOffset", 0) + a.get("byteOffset", 0)
        stride = view.get("byteStride", components * width)
        return [
            struct.unpack_from("<" + fmt * components, binary,
                               start + i * stride)
            for i in range(a["count"])
        ]

    # Match StaticMeshAsset's float addition of the GLB node translation.
    positions = accessor(primitive["attributes"]["POSITION"])
    translation = node.get("translation", [0.0, 0.0, 0.0])
    positions = [
        tuple(struct.unpack("<f", struct.pack("<f", p + t))[0]
              for p, t in zip(vertex, translation))
        for vertex in positions
    ]
    indices = [item[0] for item in accessor(primitive["indices"])]
    triangles = [
        tuple(positions[j] for j in indices[i:i + 3])
        for i in range(0, len(indices), 3)
    ]
    if len(triangles) != 72:
        raise ValueError(f"Expected 72 LanternGlass triangles, got {len(triangles)}")
    return blob, node, triangles


def component(primitive_id):
    # Existing analysis convention: six 12-triangle closed pane groups.
    for group in range(6):
        if (group * 6 <= primitive_id < group * 6 + 6 or
                36 + group * 6 <= primitive_id < 42 + group * 6):
            return group
    raise ValueError(f"Primitive outside known pane groups: {primitive_id}")


def sub(a, b):
    return tuple(a[i] - b[i] for i in range(3))


def dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def ray_triangle(origin, direction, triangle):
    a, b, c = triangle
    edge1, edge2 = sub(b, a), sub(c, a)
    pvec = cross(direction, edge2)
    determinant = dot(edge1, pvec)
    if abs(determinant) < DETERMINANT_MINIMUM:
        return None
    inverse = 1.0 / determinant
    tvec = sub(origin, a)
    u = dot(tvec, pvec) * inverse
    qvec = cross(tvec, edge1)
    v = dot(direction, qvec) * inverse
    t = dot(edge2, qvec) * inverse
    if (t <= 0.0 or u < -BARYCENTRIC_TOLERANCE or
            v < -BARYCENTRIC_TOLERANCE or
            u + v > 1.0 + BARYCENTRIC_TOLERANCE):
        return None
    return t, u, v


def normal(triangle):
    a, b, c = triangle
    n = cross(sub(b, a), sub(c, a))
    length = math.sqrt(dot(n, n))
    return tuple(value / length for value in n)


def qvec(record, prefix):
    return tuple(record[prefix + suffix] for suffix in "XYZ")


def percentile_median(values):
    ordered = sorted(values)
    return ordered[len(ordered) // 2]


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_ROOT
    comparison_path = Path(sys.argv[2]) if len(sys.argv) > 2 else root / "path-counter-comparison.json"
    capture = json.loads((root / "path-analysis.json").read_text(encoding="utf-8"))
    marker = json.loads((root / "marker-analysis.json").read_text(encoding="utf-8"))
    counters = json.loads(comparison_path.read_text(encoding="utf-8"))
    blob, node, triangles = load_glb(GLB_PATH)
    paths = capture["paths"]
    if len(paths) != 81 or [len(p["records"]) for p in paths] != [5] * 81:
        raise ValueError("Expected 81 selected paths with five captured slots each")
    classes = {p["classification"] for p in paths}
    if classes != {"interfaceBudget", "certifiedBudgetReason2"}:
        raise ValueError(f"Unexpected path classes: {classes}")

    nearest_mismatches = []
    vertex_mismatches = []
    raw_t_residuals = []
    barycentric_residuals = []
    candidate_rows = []
    state_patterns = {}
    component_transitions = {}
    path_counts = {name: 0 for name in classes}

    for path in paths:
        path_counts[path["classification"]] += 1
        records = path["records"]
        signs = "".join("E" if r["directionNormalDot"] < 0.0 else "X"
                        for r in records)
        opens = [int(r["open"]) for r in records]
        tirs = [int(r["tir"]) for r in records]
        pattern = (signs, tuple(opens), tuple(tirs))
        state_patterns[str(path["classification"]), str(pattern)] = (
            state_patterns.get((str(path["classification"]), str(pattern)), 0) + 1
        )

        if path["classification"] == "certifiedBudgetReason2":
            transition = (component(int(records[0]["primitive"])),
                          component(int(records[3]["primitive"])))
            component_transitions[str(transition)] = component_transitions.get(str(transition), 0) + 1

        for slot, record in enumerate(records):
            primitive = int(record["primitive"])
            captured_triangle = tuple(
                tuple(record[f"v{vertex}{axis}"] for axis in "XYZ")
                for vertex in range(3)
            )
            error = max(abs(captured_triangle[v][axis] - triangles[primitive][v][axis])
                        for v in range(3) for axis in range(3))
            if error != 0.0:
                vertex_mismatches.append({"row": path["recordRow"], "slot": slot,
                                          "primitive": primitive, "maxError": error})

            origin, direction = qvec(record, "object"), qvec(record, "objectD")
            intersections = []
            for candidate_id, triangle in enumerate(triangles):
                result = ray_triangle(origin, direction, triangle)
                if result is not None and result[0] >= record["queryMinimum"] - TMIN_COMPARISON_TOLERANCE:
                    intersections.append((result[0], candidate_id, result[1], result[2]))
            intersections.sort()
            primitive_hit = next((hit for hit in intersections if hit[1] == primitive), None)
            if not intersections or primitive_hit is None:
                nearest_mismatches.append({"row": path["recordRow"], "slot": slot,
                                           "primitive": primitive,
                                           "nearest": intersections[:3]})
                continue
            if abs(primitive_hit[0] - intersections[0][0]) > NEAREST_TIE_TOLERANCE:
                nearest_mismatches.append({"row": path["recordRow"], "slot": slot,
                                           "primitive": primitive,
                                           "nearest": intersections[:3]})
            elif sum(abs(hit[0] - intersections[0][0]) < NEAREST_TIE_TOLERANCE
                     for hit in intersections) > 1:
                nearest_mismatches.append({"row": path["recordRow"], "slot": slot,
                                           "reason": "near-tied nearest triangles",
                                           "nearest": intersections[:3]})
            raw_t_residuals.append(abs(primitive_hit[0] - record["rawDistance"]))
            barycentric_residuals.extend((abs(primitive_hit[2] - record["baryU"]),
                                          abs(primitive_hit[3] - record["baryV"])))

        if path["classification"] == "certifiedBudgetReason2":
            previous, candidate = records[3], records[4]
            prev_tri = triangles[int(previous["primitive"])]
            cand_tri = triangles[int(candidate["primitive"])]
            n0, n1 = normal(prev_tri), normal(cand_tri)
            margin = min(candidate["baryU"], candidate["baryV"],
                         1.0 - candidate["baryU"] - candidate["baryV"])
            plane_separation = abs(dot(n0, sub(cand_tri[0], prev_tri[0])))
            candidate_rows.append({
                "pixel": path["selectedPixel"],
                "primitive": int(candidate["primitive"]),
                "componentA": component(int(records[0]["primitive"])),
                "componentB": component(int(records[3]["primitive"])),
                "rawT": candidate["rawDistance"],
                "queryMinimum": candidate["queryMinimum"],
                "enteringDot": candidate["directionNormalDot"],
                "volumeOpen": bool(candidate["open"]),
                "priorTirCount": int(candidate["tir"]),
                "barycentricInteriorMargin": margin,
                "slot3Slot4FaceNormalDot": dot(n0, n1),
                "slot3PlaneToSlot4VertexDistanceLocal": plane_separation,
                "nearestDoubleT": primitive_hit[0],
            })

    if nearest_mismatches or vertex_mismatches:
        raise ValueError("Captured geometry/intersection verification failed")
    if len(candidate_rows) != 80 or any(
            row["queryMinimum"] != 0.0 or row["enteringDot"] <= 0.0 or
            not row["volumeOpen"] or row["priorTirCount"] != 0
            for row in candidate_rows):
        raise ValueError("Fifth reason-2 candidate state contract changed")

    expected_patterns = {
        "certifiedBudgetReason2": ("EXXEX", (0, 1, 1, 0, 1), (0, 0, 1, 0, 0), 80),
        "interfaceBudget": ("EXXXE", (0, 1, 1, 1, 0), (0, 0, 1, 2, 0), 1),
    }
    for classification, (signs, opens, tirs, expected_count) in expected_patterns.items():
        actual = state_patterns.get((classification, str((signs, opens, tirs))), 0)
        if actual != expected_count:
            raise ValueError(f"Unexpected {classification} state pattern count: {actual}")
    if component_transitions != {"(3, 4)": 30, "(4, 1)": 18,
                                 "(5, 0)": 21, "(3, 2)": 11}:
        raise ValueError(f"Unexpected pane-component transitions: {component_transitions}")
    if path_counts != {"interfaceBudget": 1, "certifiedBudgetReason2": 80}:
        raise ValueError(f"Unexpected path class counts: {path_counts}")

    parallel = [r for r in candidate_rows if r["slot3Slot4FaceNormalDot"] < -0.999999]
    nonparallel = [r for r in candidate_rows if r not in parallel]
    counter_summary = {
        "differentCounterFields": len(counters["differences"]),
        "controlStatus": counters["controlStatus"],
        "probeStatus": counters["probeStatus"],
        "controlPresented": counters["controlPresented"]["presented"],
        "probePresented": counters["probePresented"]["presented"],
        "controlIdentity": counters["controlIdentity"],
        "probeIdentity": counters["probeIdentity"],
        "identityNote": "serial and simulationTick differ; equal arrays are not same-frame identity proof",
    }
    if len(counters["differences"]) != 0:
        raise ValueError("Owning counter arrays differ")
    if (counters["count"] != 41 or counters["controlStatus"] != "valid" or
            counters["probeStatus"] != "valid" or
            not counters["controlPresented"]["presented"] or
            not counters["probePresented"]["presented"]):
        raise ValueError("Owning counter evidence is not valid/presented")
    if (capture["nonrecordPixelComparison"]["changedPixels"] != 0 or
            marker["changedPixels"] != 81 or
            marker["unmarkedDifferences"]["pixels"] != 0):
        raise ValueError("Native marker/nonrecord pixel scope changed")

    summary = {
        "schema": 1,
        "classification": "investigation-only; corroboration, not GPU traversal emulation",
        "capture": {
            "analysisJsonSha256": hashlib.sha256((root / "path-analysis.json").read_bytes()).hexdigest(),
            "nativeRawSha256": capture["nativeSha256"],
            "extent": capture["extent"],
            "paths": len(paths),
            "capturedHits": sum(len(p["records"]) for p in paths),
            "reason2Rows": path_counts["certifiedBudgetReason2"],
            "closedInterfaceBudgetRows": path_counts["interfaceBudget"],
            "moduleScope": capture["module"],
        },
        "glb": {
            "absolutePath": str(GLB_PATH),
            "sha256": hashlib.sha256(blob).hexdigest(),
            "node": node["name"],
            "triangleCount": len(triangles),
            "capturedTriangleVertexMismatches": len(vertex_mismatches),
        },
        "method": {
            "intersection": "double-precision Moller-Trumbore over captured float object-ray origin/direction",
            "searchScope": "all 72 pinned LanternGlass triangles per captured query",
            "tMinComparisonTolerance": TMIN_COMPARISON_TOLERANCE,
            "determinantMinimum": DETERMINANT_MINIMUM,
            "barycentricTolerance": BARYCENTRIC_TOLERANCE,
            "nearestTMatchAndTieTolerance": NEAREST_TIE_TOLERANCE,
            "hardwareTraversalEmulated": False,
            "limitation": "Uses actual captured native query O/D and selected hits, but double-precision CPU intersections do not reproduce device BVH traversal/rounding or unrecorded rays.",
        },
        "validation": {
            "capturedTriangleMismatches": len(vertex_mismatches),
            "committedCandidatesNotNearestOrTied": len(nearest_mismatches),
            "maxRawTResidual": max(raw_t_residuals),
            "reason2Slot4MaxRawTResidual": max(
                abs(row["nearestDoubleT"] - row["rawT"]) for row in candidate_rows
            ),
            "maxAbsoluteBarycentricResidual": max(barycentric_residuals),
            "reason2ComponentTransitionCounts": component_transitions,
            "allPathStatePatterns": {
                f"{key[0]} | {key[1]}": count
                for key, count in state_patterns.items()
            },
            "reason2StatePatterns": {
                f"{key[0]} | {key[1]}": count
                for key, count in state_patterns.items()
                if key[0] == "certifiedBudgetReason2"
            },
            "reason2Slot4RawTRange": [
                min(row["rawT"] for row in candidate_rows),
                max(row["rawT"] for row in candidate_rows),
            ],
            "reason2Slot3Slot4FaceRelation": {
                "opposingParallelCount": len(parallel),
                "otherDistinctFaceCount": len(nonparallel),
                "parallelPlaneGapLocalMinMax": [
                    min(row["slot3PlaneToSlot4VertexDistanceLocal"] for row in parallel),
                    max(row["slot3PlaneToSlot4VertexDistanceLocal"] for row in parallel),
                ],
                "otherFaces": [
                    {"pixel": row["pixel"], "slot3Primitive": int(next(
                        p["records"][3]["primitive"] for p in paths
                        if p["selectedPixel"] == row["pixel"])),
                     "slot4Primitive": row["primitive"],
                     "normalDot": row["slot3Slot4FaceNormalDot"],
                     "candidateBarycentricInteriorMargin": row["barycentricInteriorMargin"]}
                    for row in nonparallel
                ],
            },
            "minimumReason2Slot4BarycentricInteriorMargin": min(
                row["barycentricInteriorMargin"] for row in candidate_rows),
            "closedBudgetWitness": {
                "pixel": paths[0]["selectedPixel"],
                "primitiveSequence": [int(r["primitive"]) for r in paths[0]["records"]],
                "componentSequence": [component(int(r["primitive"]))
                                      for r in paths[0]["records"]],
                "directionNormalSigns": "".join(
                    "E" if r["directionNormalDot"] < 0.0 else "X"
                    for r in paths[0]["records"]),
                "openBeforeEachHit": [int(r["open"]) for r in paths[0]["records"]],
                "priorTirCountBeforeEachHit": [int(r["tir"]) for r in paths[0]["records"]],
                "fifthCandidateRawT": paths[0]["records"][4]["rawDistance"],
                "fifthCandidateQueryMinimum": paths[0]["records"][4]["queryMinimum"],
                "fifthCandidateBarycentricInteriorMargin": min(
                    paths[0]["records"][4]["baryU"],
                    paths[0]["records"][4]["baryV"],
                    1.0 - paths[0]["records"][4]["baryU"] -
                    paths[0]["records"][4]["baryV"]),
                "fifthCandidateDirectionNormalDot": paths[0]["records"][4]["directionNormalDot"],
            },
        },
        "owningEvidence": counter_summary,
        "imageScope": {
            "pathVsMarkerOutsideReservedRowsChangedPixels": capture["nonrecordPixelComparison"]["changedPixels"],
            "markerVsControlChangedPixels": marker["changedPixels"],
            "markerPixels": marker["markerCounts"],
            "productionAcceptance": marker["productionAcceptance"],
        },
        "perReason2Candidate": candidate_rows,
        "conclusion": "All 80 reason-2 fifth candidates are corroborated as distinct real exits from the second pane along the recorded native object rays; they are genuine fifth-interface budget truncations, not duplicate candidates. This does not establish visible contribution magnitude, all-ray behavior, or compute/RTX behavior.",
    }
    print(json.dumps(summary, indent=2, allow_nan=False))


if __name__ == "__main__":
    main()
