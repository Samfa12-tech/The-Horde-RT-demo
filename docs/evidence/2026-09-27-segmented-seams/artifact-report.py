"""Read-only comparison of segmented sleeve-seam GLBs.

Usage: python reports/player-segmented-seam-artifact-report.py BASE_WORLD CANDIDATE_WORLD BASE_VIEW CANDIDATE_VIEW
"""
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import struct
import sys


FORMATS = {5121: ("B", 1), 5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4)}
WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}
ATTRS = ("NORMAL", "TANGENT", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0")


def load_glb(path):
    raw = Path(path).read_bytes()
    assert struct.unpack_from("<III", raw) == (0x46546C67, 2, len(raw)), f"invalid GLB: {path}"
    offset = 12
    document = binary = None
    while offset < len(raw):
        length, kind = struct.unpack_from("<II", raw, offset)
        chunk = raw[offset + 8:offset + 8 + length]
        offset += 8 + length
        if kind == 0x4E4F534A:
            document = json.loads(chunk.decode("utf-8").rstrip("\0 \t\r\n"))
        elif kind == 0x004E4942:
            binary = chunk
    assert document is not None and binary is not None

    def read_accessor(index):
        accessor = document["accessors"][index]
        assert "sparse" not in accessor, "sparse accessors need explicit handling"
        view = document["bufferViews"][accessor["bufferView"]]
        fmt, size = FORMATS[accessor["componentType"]]
        width = WIDTHS[accessor["type"]]
        stride = view.get("byteStride", size * width)
        start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
        return [
            struct.unpack_from("<" + fmt * width, binary, start + i * stride)
            for i in range(accessor["count"])
        ]

    primitives = {}
    for primitive in document["meshes"][0]["primitives"]:
        material = document["materials"][primitive["material"]]["name"]
        attributes = {name: read_accessor(index) for name, index in primitive["attributes"].items()}
        indices = [value[0] for value in read_accessor(primitive["indices"])]
        assert len(indices) % 3 == 0 and primitive.get("mode", 4) == 4
        triangles = []
        for offset in range(0, len(indices), 3):
            tri = [{name: rows[indices[offset + corner]] for name, rows in attributes.items()}
                   for corner in range(3)]
            rotation = min(range(3), key=lambda r: tuple(tri[(r + c) % 3]["POSITION"] for c in range(3)))
            triangles.append(tri[rotation:] + tri[:rotation])
        primitives[material] = {"attributes": attributes, "triangles": triangles}
    return raw, document, primitives


def triangle_counters(triangles):
    positions = Counter()
    full = Counter()
    common_key_rows = defaultdict(list)
    for tri in triangles:
        position_key = tuple(corner["POSITION"] for corner in tri)
        uv_key = tuple((corner["POSITION"], corner["TEXCOORD_0"]) for corner in tri)
        positions[position_key] += 1
        full[tuple(tuple(sorted(corner.items())) for corner in tri)] += 1
        common_key_rows[uv_key].append(tri)
    return positions, full, common_key_rows


def attr_stats(old_rows, new_rows, shared_uv_keys):
    result = {}
    for name in ATTRS:
        exact_triangles = 0
        changed_triangles = 0
        changed_corners = 0
        max_component = 0.0
        max_vector = 0.0
        for key in shared_uv_keys:
            left, right = old_rows[key], new_rows[key]
            for old_tri, new_tri in zip(left, right):
                changed = False
                for old_corner, new_corner in zip(old_tri, new_tri):
                    a, b = old_corner[name], new_corner[name]
                    if a == b:
                        continue
                    changed = True
                    changed_corners += 1
                    delta = [abs(float(x) - float(y)) for x, y in zip(a, b)]
                    max_component = max(max_component, *delta)
                    max_vector = max(max_vector, math.sqrt(sum(x * x for x in delta)))
                changed_triangles += int(changed)
                exact_triangles += int(not changed)
        result[name] = {
            "exactTriangles": exact_triangles,
            "changedTriangles": changed_triangles,
            "changedCorners": changed_corners,
            "maxComponentDelta": max_component,
            "maxVectorDelta": max_vector,
        }
    return result


def main(args):
    assert len(args) == 4, __doc__
    old_path, new_path, old_view_path, new_view_path = map(Path, args)
    old_raw, old_doc, old = load_glb(old_path)
    new_raw, new_doc, new = load_glb(new_path)
    _, _, old_view = load_glb(old_view_path)
    _, _, new_view = load_glb(new_view_path)

    report = {
        "hashes": {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in (old_path, new_path, old_view_path, new_view_path)},
        "viewmodelByteIdentical": old_view_path.read_bytes() == new_view_path.read_bytes(),
        "worldStructureIdentical": {
            key: old_doc.get(key) == new_doc.get(key)
            for key in ("nodes", "skins", "animations", "materials", "textures", "images")
        },
        "primitives": {},
    }
    assert set(old) == set(new)
    view_sleeve = new_view["ViewmodelSleeves"]["attributes"]
    view_weights = defaultdict(set)
    for position, joints, weights in zip(view_sleeve["POSITION"], view_sleeve["JOINTS_0"], view_sleeve["WEIGHTS_0"]):
        view_weights[tuple(position)].add((tuple(joints), tuple(weights)))

    for material in sorted(old):
        old_tri = old[material]["triangles"]
        new_tri = new[material]["triangles"]
        op, ofull, orows = triangle_counters(old_tri)
        np, nfull, nrows = triangle_counters(new_tri)
        shared_positions = op & np
        shared_uv_keys = orows.keys() & nrows.keys()
        per_attr = attr_stats(orows, nrows, shared_uv_keys)
        item = {
            "vertices": [len(old[material]["attributes"]["POSITION"]),
                         len(new[material]["attributes"]["POSITION"])],
            "triangles": [len(old_tri), len(new_tri)],
            "exactFullAttributeTriangles": sum((ofull & nfull).values()),
            "samePositionTriangles": sum(shared_positions.values()),
            "positionTrianglesRemovedAdded": [sum((op - np).values()), sum((np - op).values())],
            "unchangedPositionTriangleAttributes": per_attr,
        }
        if material == "BodyRemainderPrimaryVisible":
            old_positions = {tuple(row) for row in old[material]["attributes"]["POSITION"]}
            new_positions = {tuple(row) for row in new[material]["attributes"]["POSITION"]}
            added = new_positions - old_positions
            old_edges = set()
            new_edges = set()
            for triangles, edges in ((old_tri, old_edges), (new_tri, new_edges)):
                for tri in triangles:
                    for i in range(3):
                        edges.add(tuple(sorted((tri[i]["POSITION"], tri[(i + 1) % 3]["POSITION"]))))
            edge_midpoints = {
                tuple(round((a[i] + b[i]) * 0.5, 6) for i in range(3)) for a, b in old_edges
            }
            edge_splits = 0
            max_edge_distance = 0.0
            for point in added:
                matched_edge = None
                best_distance = float("inf")
                for a, b in old_edges:
                    direction = tuple(b[i] - a[i] for i in range(3))
                    denominator = sum(x * x for x in direction)
                    t = max(0.0, min(1.0, sum((point[i] - a[i]) * direction[i] for i in range(3)) / denominator))
                    closest = tuple(a[i] + t * direction[i] for i in range(3))
                    distance = math.sqrt(sum((point[i] - closest[i]) ** 2 for i in range(3)))
                    if distance < best_distance:
                        best_distance, matched_edge = distance, (a, b, t)
                max_edge_distance = max(max_edge_distance, best_distance)
                if best_distance < 1e-6 and 1e-5 < matched_edge[2] < 1.0 - 1e-5 and \
                        tuple(sorted((matched_edge[0], point))) in new_edges and \
                        tuple(sorted((point, matched_edge[1]))) in new_edges:
                    edge_splits += 1
            item["insertedWorldPositions"] = {
                "count": len(added),
                "matchingViewSleevePositions": sum(point in view_weights for point in added),
                "matchingViewSleeveSkinWeights": sum(
                    any((tuple(new[material]["attributes"]["JOINTS_0"][i]),
                         tuple(new[material]["attributes"]["WEIGHTS_0"][i])) in view_weights[tuple(point)]
                        for i, point in enumerate(new[material]["attributes"]["POSITION"])
                        if tuple(point) == position)
                    for position in added
                ),
                "oldEdgeMidpointAt6dp": sum(tuple(round(x, 6) for x in point) in edge_midpoints for point in added),
                "interiorOldEdgeSplitSegments": edge_splits,
                "maxDistanceFromOldEdgeMetres": max_edge_distance,
            }
        report["primitives"][material] = item

    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main(sys.argv[1:])
