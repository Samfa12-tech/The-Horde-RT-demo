"""Closed-component pine LOD2 experiment. Writes only beside this script.

Run from any directory with Python 3 and Pillow. The admitted source is pinned;
no Blender re-export, remeshing, vertex arithmetic, texture edit or asset admission.
The source authoring loop emits ten tiers of five seven-lens sprays, then sixteen
leader lenses. Export-order windows retain both endpoints and at least five lenses.
"""
from __future__ import annotations

import collections
import copy
import hashlib
import json
import math
from pathlib import Path
import struct

from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[1]
SOURCE = ROOT / "assets/models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb"
README = ROOT / "assets/models/world/source/forest-tree-pair-v1/README.txt"
AUTHORING = ROOT / "assets/models/world/source/forest-tree-pair-v1/source/build_horde_trees.py"
EXPECTED = "db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f"
NEEDLE_MESH, POSITION, INDICES = 1, 5, 9
BANDS = 24


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write_json(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2, allow_nan=False) + "\n", encoding="utf-8")


def read_glb(data):
    assert struct.unpack_from("<III", data) == (0x46546C67, 2, len(data))
    json_length, kind = struct.unpack_from("<II", data, 12)
    assert kind == 0x4E4F534A
    document = json.loads(data[20:20 + json_length])
    bin_length, kind = struct.unpack_from("<II", data, 20 + json_length)
    assert kind == 0x004E4942 and len(data) == 28 + json_length + bin_length
    assert len(document["buffers"]) == 1
    return document, data[28 + json_length:]


def view_bytes(document, blob, view_id):
    view = document["bufferViews"][view_id]
    assert view["buffer"] == 0
    begin = view.get("byteOffset", 0)
    return blob[begin:begin + view["byteLength"]]


def accessor(document, blob, accessor_id):
    item = document["accessors"][accessor_id]
    view = document["bufferViews"][item["bufferView"]]
    assert not item.get("sparse") and not view.get("byteStride")
    assert item.get("byteOffset", 0) == 0
    fmt = {5126: "f", 5123: "H"}[item["componentType"]]
    width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[item["type"]]
    raw = view_bytes(document, blob, item["bufferView"])
    size = struct.calcsize("<" + fmt * width)
    assert len(raw) == item["count"] * size
    return list(struct.iter_unpack("<" + fmt * width, raw)), raw, size


def bounds(points):
    return {"min": [min(p[k] for p in points) for k in range(3)],
            "max": [max(p[k] for p in points) for k in range(3)]}


def components(points, indices):
    """Weld only exactly equal float positions; UV/normal/tangent seams stay intact."""
    welded = {}
    vertex_ids = [welded.setdefault(p, len(welded)) for p in points]
    parent = list(range(len(welded)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for offset in range(0, len(indices), 3):
        ids = [vertex_ids[i] for i in indices[offset:offset + 3]]
        assert len(set(ids)) == 3
        parent[find(ids[0])] = find(ids[1])
        parent[find(ids[0])] = find(ids[2])
    groups = collections.defaultdict(list)
    for triangle in range(len(indices) // 3):
        groups[find(vertex_ids[indices[triangle * 3]])].append(triangle)
    result = []
    for triangles in sorted(groups.values(), key=min):
        vertices = sorted({indices[t * 3 + k] for t in triangles for k in range(3)})
        edges = collections.Counter()
        directed = collections.Counter()
        centre = tuple(sum(points[i][k] for i in vertices) / len(vertices) for k in range(3))
        volume = 0.0
        minimum_area_squared = math.inf
        for t in triangles:
            ids = indices[t * 3:t * 3 + 3]
            for k in range(3):
                edge = (vertex_ids[ids[k]], vertex_ids[ids[(k + 1) % 3]])
                edges[tuple(sorted(edge))] += 1
                directed[edge] += 1
            a, b, c = [tuple(points[i][k] - centre[k] for k in range(3)) for i in ids]
            cross = (b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2], b[0] * c[1] - b[1] * c[0])
            volume += sum(a[k] * cross[k] for k in range(3)) / 6
            ab, ac = [b[k] - a[k] for k in range(3)], [c[k] - a[k] for k in range(3)]
            normal = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
            minimum_area_squared = min(minimum_area_squared, sum(n * n for n in normal) / 4)
        welded_count = len({vertex_ids[i] for i in vertices})
        closed = all(n == 2 for n in edges.values())
        oriented = all(n == 1 and directed[(b, a)] == 1 for (a, b), n in directed.items())
        euler = welded_count - len(edges) + len(triangles)
        assert closed and oriented and euler == 2 and abs(volume) > 1e-12 and minimum_area_squared > 0
        result.append({"id": len(result), "triangles": triangles, "vertices": vertices,
                       "weldedVertices": welded_count, "edges": len(edges), "euler": euler,
                       "closed": closed, "consistentWinding": oriented,
                       "signedVolumeM3": volume, "bounds": bounds([points[i] for i in vertices]),
                       "centre": centre})
    return result


def horizontal_hull(rows):
    """Exact orientation tests on original float coordinates, no quantization."""
    points = sorted({(p[0], p[2], owner) for owner, p in rows})
    result = []
    for sequence in (points, points[::-1]):
        half = []
        for point in sequence:
            while len(half) > 1 and ((half[-1][0] - half[-2][0]) * (point[1] - half[-2][1]) -
                                    (half[-1][1] - half[-2][1]) * (point[0] - half[-2][0])) <= 0:
                half.pop()
            half.append(point)
        result.extend(half[:-1])
    return result


def directions():
    return [(math.cos(math.radians(e)) * math.cos(math.tau * a / 72), math.sin(math.radians(e)),
             math.cos(math.radians(e)) * math.sin(math.tau * a / 72))
            for e in range(-90, 91, 15) for a in range(72)]


def select_interior(points, lenses):
    rows = [(c["id"], points[i]) for c in lenses for i in c["vertices"]]
    y0, y1 = min(p[1] for p in points), max(p[1] for p in points)
    bands = [[(i, p) for i, p in rows if min(BANDS - 1, int((p[1] - y0) / (y1 - y0) * BANDS)) == band]
             for band in range(BANDS)]
    # Every hull owner's entire closed lens survives. This preserves the convex
    # envelope of original vertex samples in each height band for every azimuth.
    protected = set(range(350, 366))
    for band in bands:
        protected.update(p[2] for p in horizontal_hull(band))
    for direction in directions():
        protected.add(max(rows, key=lambda row: sum(a * b for a, b in zip(row[1], direction)))[0])
    protected.update(i for i in range(350) if i % 7 in (0, 6))
    omitted = set()
    windows = []
    for begin in range(0, 350, 7):
        eligible = [i for i in range(begin, begin + 7) if i not in protected]
        # Prefer components toward the original ground-origin trunk axis. No
        # branch is removed: each complete seven-lens spray keeps at least five.
        eligible.sort(key=lambda i: (math.hypot(lenses[i]["centre"][0], lenses[i]["centre"][2]), i))
        chosen = eligible[:2]
        omitted.update(chosen)
        windows.append({"firstComponent": begin, "omitted": chosen, "retained": 7 - len(chosen)})
    assert .25 <= len(omitted) / len(lenses) <= .40, "Reject: safe interior pruning misses requested modest range"
    retained_rows = [(i, p) for i, p in rows if i not in omitted]
    assert bounds([p for _, p in rows]) == bounds([p for _, p in retained_rows])
    for band in bands:
        original = {(x, z) for x, z, _ in horizontal_hull(band)}
        retained = {(x, z) for x, z, _ in horizontal_hull([(i, p) for i, p in band if i not in omitted])}
        assert original == retained
    maximum_error = max(abs(max(sum(a * b for a, b in zip(p, d)) for _, p in rows) -
                            max(sum(a * b for a, b in zip(p, d)) for _, p in retained_rows)) for d in directions())
    assert maximum_error == 0
    return omitted, protected, windows, maximum_error


def repack(document, blob, points, indices, omitted_triangles):
    output = copy.deepcopy(document)
    kept_indices = [i for t in range(len(indices) // 3) if t not in omitted_triangles for i in indices[t * 3:t * 3 + 3]]
    kept_vertices = sorted(set(kept_indices))
    remap = {old: new for new, old in enumerate(kept_vertices)}
    replacements = {}
    primitive = document["meshes"][NEEDLE_MESH]["primitives"][0]
    for semantic, aid in primitive["attributes"].items():
        _, raw, size = accessor(document, blob, aid)
        view_id = document["accessors"][aid]["bufferView"]
        replacements[view_id] = b"".join(raw[i * size:(i + 1) * size] for i in kept_vertices)
        output["accessors"][aid]["count"] = len(kept_vertices)
        if semantic == "POSITION":
            current = bounds([points[i] for i in kept_vertices])
            output["accessors"][aid].update(current)
    index_view = document["accessors"][INDICES]["bufferView"]
    replacements[index_view] = struct.pack("<" + "H" * len(kept_indices), *(remap[i] for i in kept_indices))
    output["accessors"][INDICES]["count"] = len(kept_indices)
    packed = bytearray()
    unchanged = []
    for view_id, old_view in enumerate(document["bufferViews"]):
        packed.extend(b"\0" * (-len(packed) % 4))
        old = view_bytes(document, blob, view_id)
        payload = replacements.get(view_id, old)
        output["bufferViews"][view_id]["byteOffset"] = len(packed)
        output["bufferViews"][view_id]["byteLength"] = len(payload)
        packed.extend(payload)
        if view_id not in replacements:
            unchanged.append({"bufferView": view_id, "bytes": len(old), "sha256": sha(old), "exact": payload == old})
    output["buffers"][0]["byteLength"] = len(packed)
    packed.extend(b"\0" * (-len(packed) % 4))
    text = json.dumps(output, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    text += b" " * (-len(text) % 4)
    result = (struct.pack("<III", 0x46546C67, 2, 28 + len(text) + len(packed)) +
              struct.pack("<II", len(text), 0x4E4F534A) + text +
              struct.pack("<II", len(packed), 0x004E4942) + packed)
    return result, kept_vertices, kept_indices, unchanged


def projection_metrics(points, indices, omitted_triangles):
    # CPU orthographic coverage diagnostic only. No material, lighting, game
    # renderer, physical device, camera-distance or cutover acceptance is claimed.
    results = []
    sheet = Image.new("RGB", (4 * 300, 2 * 380), "#111820")
    draw_sheet = ImageDraw.Draw(sheet)
    for azimuth in range(0, 360, 30):
        for elevation in (0, 15, 30):
            az, el = math.radians(azimuth), math.radians(elevation)
            side = (math.cos(az), 0, math.sin(az))
            up = (-math.sin(az) * math.sin(el), math.cos(el), math.cos(az) * math.sin(el))
            projected = [(sum(a * b for a, b in zip(p, side)), sum(a * b for a, b in zip(p, up))) for p in points]
            xmin, xmax = min(p[0] for p in projected), max(p[0] for p in projected)
            ymin, ymax = min(p[1] for p in projected), max(p[1] for p in projected)
            scale = min(296 / (xmax - xmin), 348 / (ymax - ymin))
            pixels = [(2 + (x - xmin) * scale, 2 + (ymax - y) * scale) for x, y in projected]
            original, candidate = Image.new("1", (300, 352)), Image.new("1", (300, 352))
            source_draw, candidate_draw = ImageDraw.Draw(original), ImageDraw.Draw(candidate)
            for t in range(len(indices) // 3):
                triangle = [pixels[i] for i in indices[t * 3:t * 3 + 3]]
                source_draw.polygon(triangle, fill=1)
                if t not in omitted_triangles:
                    candidate_draw.polygon(triangle, fill=1)
            source_area = sum(original.histogram()[1:])
            candidate_area = sum(candidate.histogram()[1:])
            results.append({"azimuthDegrees": azimuth, "elevationDegrees": elevation,
                            "sourceFoliagePixels": source_area, "candidateFoliagePixels": candidate_area,
                            "lostCoverageFraction": 1 - candidate_area / source_area})
            if elevation == 15 and azimuth in (0, 90, 180, 270):
                column = azimuth // 90
                for row, mask, label in ((0, original, "LOD1"), (1, candidate, "candidate")):
                    tile = Image.new("RGB", mask.size, "#111820")
                    tile.paste("#83a787", (0, 0, 300, 352), mask)
                    sheet.paste(tile, (column * 300, row * 380 + 24))
                    draw_sheet.text((column * 300 + 5, row * 380 + 4),
                                    f"CPU coverage {label} az={azimuth} el=15", fill="white")
    sheet.save(OUT / "cpu-projected-coverage.png", optimize=False)
    return results


def main():
    source = SOURCE.read_bytes()
    assert sha(source) == EXPECTED, "Reject: admitted source digest changed"
    document, blob = read_glb(source)
    primitive = document["meshes"][NEEDLE_MESH]["primitives"][0]
    assert primitive["attributes"] == {"POSITION": 5, "NORMAL": 6, "TEXCOORD_0": 7, "TANGENT": 8}
    assert primitive["indices"] == INDICES and primitive["material"] == 1
    assert all(m.get("alphaMode", "OPAQUE") == "OPAQUE" for m in document["materials"])
    points, _, _ = accessor(document, blob, POSITION)
    indices = [i[0] for i in accessor(document, blob, INDICES)[0]]
    assert len(points) == 3660 and len(indices) == 2928 * 3
    assert all(math.isfinite(x) for point in points for x in point)
    lenses = components(points, indices)
    assert len(lenses) == 366
    for i, lens in enumerate(lenses):
        assert lens["triangles"] == list(range(i * 8, (i + 1) * 8))
        assert lens["vertices"] == list(range(i * 10, (i + 1) * 10))
        assert lens["weldedVertices"] == 6 and lens["edges"] == 12
    omitted, protected, windows, maximum_error = select_interior(points, lenses)
    omitted_triangles = {t for i in omitted for t in lenses[i]["triangles"]}
    candidate, kept_vertices, kept_indices, unchanged = repack(document, blob, points, indices, omitted_triangles)
    candidate_doc, candidate_blob = read_glb(candidate)
    candidate_points = accessor(candidate_doc, candidate_blob, POSITION)[0]
    candidate_indices = [i[0] for i in accessor(candidate_doc, candidate_blob, INDICES)[0]]
    candidate_lenses = components(candidate_points, candidate_indices)
    assert len(candidate_lenses) == len(lenses) - len(omitted)
    assert bounds(candidate_points) == bounds(points)
    # Reparse output and compare actual payloads, rather than trusting repack's receipt.
    for entry in unchanged:
        assert view_bytes(candidate_doc, candidate_blob, entry["bufferView"]) == view_bytes(document, blob, entry["bufferView"])
    for semantic, aid in primitive["attributes"].items():
        _, old, size = accessor(document, blob, aid)
        _, current, _ = accessor(candidate_doc, candidate_blob, aid)
        assert current == b"".join(old[i * size:(i + 1) * size] for i in kept_vertices)
    remap = {old: new for new, old in enumerate(kept_vertices)}
    assert candidate_indices == [remap[i] for i in kept_indices]
    assert set(candidate_indices) == set(range(len(candidate_points)))
    for key in document.keys() - {"accessors", "bufferViews", "buffers"}:
        assert document[key] == candidate_doc[key]
    for aid in set(range(len(document["accessors"]))) - set(range(5, 10)):
        assert document["accessors"][aid] == candidate_doc["accessors"][aid]
    for old, new in zip(document["bufferViews"], candidate_doc["bufferViews"]):
        assert {k: v for k, v in old.items() if k not in ("byteOffset", "byteLength")} == {
            k: v for k, v in new.items() if k not in ("byteOffset", "byteLength")}
    (OUT / "candidate.glb").write_bytes(candidate)
    write_json("omitted-needle-triangles.json", {"source": SOURCE.relative_to(ROOT).as_posix(),
               "sourceSha256": EXPECTED, "mesh": NEEDLE_MESH, "primitive": 0,
               "triangleIdConvention": "zero-based local needle primitive source triangle order",
               "omittedComponentIds": sorted(omitted), "omittedLocalTriangleIds": sorted(omitted_triangles),
               "retainedSourceVertexIds": kept_vertices})
    metrics = projection_metrics(points, indices, omitted_triangles)
    geometry_views = [0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16]
    geometry_bytes = lambda d: sum(d["bufferViews"][i]["byteLength"] for i in geometry_views)
    all_positions = lambda d, b: [p for mesh in d["meshes"] for primitive in mesh["primitives"]
                                  for p in accessor(d, b, primitive["attributes"]["POSITION"])[0]]
    full_source_bounds = bounds(all_positions(document, blob))
    full_candidate_bounds = bounds(all_positions(candidate_doc, candidate_blob))
    assert full_source_bounds == full_candidate_bounds
    write_json("derivation-receipt.json", {
        "experiment": "one-pine closed interior lens pruning; candidate only",
        "source": SOURCE.relative_to(ROOT).as_posix(), "sourceSha256": EXPECTED,
        "provenance": {"readme": README.relative_to(ROOT).as_posix(), "readmeSha256": sha(README.read_bytes()),
                       "authoring": AUTHORING.relative_to(ROOT).as_posix(), "authoringSha256": sha(AUTHORING.read_bytes()),
                       "rights": "Owner-authored originals; existing owner-controlled rights unchanged; no new license assigned"},
        "scriptSha256": sha(Path(__file__).read_bytes()), "candidateSha256": sha(candidate),
        "sourceBytes": len(source), "candidateBytes": len(candidate), "fileBytesRemoved": len(source) - len(candidate),
        "sourceGeometryPayloadBytes": geometry_bytes(document),
        "candidateGeometryPayloadBytes": geometry_bytes(candidate_doc),
        "geometryPayloadBytesRemoved": geometry_bytes(document) - geometry_bytes(candidate_doc),
        "fullSourceBounds": full_source_bounds, "fullCandidateBounds": full_candidate_bounds,
        "sourceTotalVertices": sum(a["count"] for a in document["accessors"] if a["type"] == "VEC3" and "min" in a),
        "candidateTotalVertices": sum(a["count"] for a in candidate_doc["accessors"] if a["type"] == "VEC3" and "min" in a),
        "sourceTotalTriangles": 5000, "candidateTotalTriangles": 5000 - len(omitted_triangles),
        "foliage": {"sourceVertices": len(points), "candidateVertices": len(candidate_points),
                    "sourceTriangles": len(indices) // 3, "candidateTriangles": len(candidate_indices) // 3,
                    "sourceComponents": len(lenses), "candidateComponents": len(candidate_lenses),
                    "omittedComponents": len(omitted), "reductionFraction": len(omitted) / len(lenses),
                    "sourceBounds": bounds(points), "candidateBounds": bounds(candidate_points),
                    "allSourceAndRetainedComponentsClosed": True, "componentWeldedVertices": 6,
                    "componentEdges": 12, "componentTriangles": 8, "componentEulerCharacteristic": 2,
                    "minimumSourceAbsoluteVolumeM3": min(abs(c["signedVolumeM3"]) for c in lenses),
                    "minimumCandidateAbsoluteVolumeM3": min(abs(c["signedVolumeM3"]) for c in candidate_lenses)},
        "selection": {"heightBands": BANDS, "horizontalBandHullVertexCoordinatesExact": True,
                      "supportDirections": len(directions()), "maximumSupportExtremumErrorM": maximum_error,
                      "protectedComponentIds": sorted(protected), "sprigWindows": windows,
                      "leaderLensesRetained": 16, "maximumOmittedPerSevenLensSpray": 2,
                      "meaningOfInterior": "No ownership of height-band hull vertices or sampled 3D support extrema; endpoints/leader protected"},
        "preservation": {"keptVertexAttributeBytesExact": ["POSITION", "NORMAL", "TEXCOORD_0", "TANGENT"],
                         "keptTriangleOrderAndWindingExact": True, "allCandidateVerticesReferenced": True,
                         "allNonNeedleAccessorsExact": True, "barkRootMossGeometryExact": True,
                         "materialsImagesTexturesSamplersNodesScenesExact": True,
                         "unchangedBufferViewPayloads": unchanged,
                         "changedBufferViewsOnly": [6, 7, 8, 9, 10],
                         "bufferOffsets": "Repacked with four-byte alignment; unaffected payload bytes remain exact"},
        "cpuProjectedCoverage": metrics,
        "maximumProjectedFoliageCoverageLoss": max(m["lostCoverageFraction"] for m in metrics),
        "validationBoundary": "Static GLB, exact payload, welded closedness, CPU orthographic coverage only. No game import, RT image, LOD cutover, GPU timing, device, listening, package admission or public release acceptance.",
        "gameDevCli": "Not available on PATH; custom deterministic Python inspection/derivation used; no package validation claim"})
    print(json.dumps({"candidate": str(OUT / "candidate.glb"), "sha256": sha(candidate),
                      "omittedLenses": len(omitted), "foliageTriangles": len(candidate_indices) // 3,
                      "totalTriangles": 5000 - len(omitted_triangles), "bytesRemoved": len(source) - len(candidate),
                      "maximumCpuProjectedCoverageLoss": max(m["lostCoverageFraction"] for m in metrics)}))


if __name__ == "__main__":
    main()
