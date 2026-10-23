"""Author the original F03 masonry GLBs from the checked-in metre-scale recipe.

Python standard library only. No import, provider, texture, Blender or network
operation. Source recipe stays outside the runtime directory. Generation refuses
to overwrite any file; --verify compares existing output without writing it.
"""

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/source/beyond_the_tomb/forest-road-clue-v1/recipe.json"
RUNTIME = ROOT / "assets/models/world/runtime/forest-road-clue-v1"
FACES = ((0, 4, 5, 1), (2, 6, 7, 3), (3, 7, 4, 0),
         (1, 5, 6, 2), (0, 1, 2, 3), (4, 7, 6, 5))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def scale(a, multiplier):
    return tuple(x * multiplier for x in a)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def unit(a):
    length = math.sqrt(dot(a, a))
    require(math.isfinite(length) and length > 1.0e-12, "Degenerate vector")
    return scale(a, 1.0 / length)


def corners(width, depth, height, top_width=None, top_depth=None):
    top_width = width if top_width is None else top_width
    top_depth = depth if top_depth is None else top_depth
    return ((-width / 2, 0.0, -depth / 2), (width / 2, 0.0, -depth / 2),
            (width / 2, 0.0, depth / 2), (-width / 2, 0.0, depth / 2),
            (-top_width / 2, height, -top_depth / 2),
            (top_width / 2, height, -top_depth / 2),
            (top_width / 2, height, top_depth / 2),
            (-top_width / 2, height, top_depth / 2))


def components(spec):
    if spec["kind"] == "box":
        return [corners(spec["width"], spec["depth"], spec["height"])]
    require(spec["kind"] == "waystone", "Unknown source shape")
    result = [corners(spec["bottomWidth"], spec["bottomDepth"], spec["height"],
                      spec["topWidth"], spec["topDepth"])]
    slope = (spec["bottomDepth"] - spec["topDepth"]) / (2 * spec["height"])
    along_face = unit((0.0, 1.0, slope))
    into_face = unit((0.0, -slope, 1.0))
    outward, inset = spec["markOutwardDepth"], spec["markInsetDepth"]
    for mark in spec["marks"]:
        y = mark["centreY"]
        centre = (0.0, y, -spec["bottomDepth"] / 2 + slope * y)
        mark_points = []
        for x, local_y, z in corners(mark["width"], outward + inset, mark["height"]):
            point = add(centre, (x, 0.0, 0.0))
            point = add(point, scale(along_face, local_y - mark["height"] / 2))
            point = add(point, scale(into_face, z + (inset - outward) / 2))
            mark_points.append(point)
        result.append(tuple(mark_points))
    return result


def geometry(spec, repeats):
    positions, normals, uv0, tangents, indices = [], [], [], [], []
    for solid in components(spec):
        centre = tuple(sum(point[axis] for point in solid) / 8 for axis in range(3))
        for face in FACES:
            points = [solid[i] for i in face]
            normal = unit(cross(subtract(points[1], points[0]),
                                subtract(points[2], points[0])))
            require(dot(normal, subtract(points[0], centre)) > 0, "Inward face")
            # Keep V upright on vertical faces; top/bottom use a horizontal chart.
            tangent = ((1.0, 0.0, 0.0) if abs(normal[1]) > 0.99
                       else unit(cross((0.0, 1.0, 0.0), normal)))
            bitangent = cross(normal, tangent)
            chart = [(dot(p, tangent) * repeats, dot(p, bitangent) * repeats)
                     for p in points]
            minimum = tuple(min(uv[axis] for uv in chart) for axis in range(2))
            start = len(positions)
            positions.extend(points)
            normals.extend([normal] * 4)
            uv0.extend([subtract(uv, minimum) for uv in chart])
            tangents.extend([(*tangent, 1.0)] * 4)
            indices.extend(start + i for i in (0, 1, 2, 0, 2, 3))
    require(len(positions) == spec["expectedVertices"], "Unexpected source vertex count")
    require(len(indices) // 3 == spec["expectedTriangles"], "Unexpected source triangle count")
    return positions, normals, uv0, tangents, indices


def material(name):
    return {"name": name, "pbrMetallicRoughness": {
        "baseColorFactor": [1.0, 1.0, 1.0, 1.0], "metallicFactor": 0.0,
        "roughnessFactor": 1.0}, "emissiveFactor": [0.0, 0.0, 0.0],
        "alphaMode": "OPAQUE", "doubleSided": False}


def glb(spec, recipe):
    positions, normals, uv0, tangents, indices = geometry(spec, recipe["uvRepeatsPerMetre"])
    binary, views, accessors = bytearray(), [], []
    attributes = {}
    arrays = (("POSITION", positions, "VEC3", 3), ("NORMAL", normals, "VEC3", 3),
              ("TEXCOORD_0", uv0, "VEC2", 2), ("TANGENT", tangents, "VEC4", 4))
    for name, values, kind, width in arrays:
        offset = len(binary)
        binary.extend(struct.pack("<" + "f" * (len(values) * width),
                                  *(v for row in values for v in row)))
        views.append({"buffer": 0, "byteOffset": offset,
                      "byteLength": len(binary) - offset, "target": 34962})
        accessor = {"bufferView": len(views) - 1, "componentType": 5126,
                    "count": len(values), "type": kind}
        if name == "POSITION":
            # Derive accessor bounds from the actual stored float32 positions.
            stored = struct.unpack_from("<" + "f" * (len(values) * width), binary, offset)
            accessor["min"] = [min(stored[axis::width]) for axis in range(width)]
            accessor["max"] = [max(stored[axis::width]) for axis in range(width)]
        attributes[name] = len(accessors)
        accessors.append(accessor)
    offset = len(binary)
    binary.extend(struct.pack("<" + "H" * len(indices), *indices))
    views.append({"buffer": 0, "byteOffset": offset,
                  "byteLength": len(binary) - offset, "target": 34963})
    index_accessor = len(accessors)
    accessors.append({"bufferView": len(views) - 1, "componentType": 5123,
                      "count": len(indices), "type": "SCALAR"})
    document = {
        "asset": {"version": "2.0", "generator": "Horde original forest-road-clue recipe v1"},
        "scene": 0, "scenes": [{"nodes": [0]}],
        "nodes": [{"name": spec["name"], "mesh": 0}],
        "meshes": [{"name": spec["name"], "primitives": [{"attributes": attributes,
                     "indices": index_accessor, "material": 0, "mode": 4}]}],
        "materials": [material(recipe["material"])],
        "accessors": accessors, "bufferViews": views,
        "buffers": [{"byteLength": len(binary)}]}
    payload = json.dumps(document, separators=(",", ":"), allow_nan=False).encode("utf-8")
    payload += b" " * (-len(payload) % 4)
    binary.extend(b"\0" * (-len(binary) % 4))
    return (struct.pack("<III", 0x46546c67, 2, 28 + len(payload) + len(binary))
            + struct.pack("<II", len(payload), 0x4e4f534a) + payload
            + struct.pack("<II", len(binary), 0x004e4942) + binary)


def manifest(recipe):
    return {"schema": 1, "asset": recipe["asset"], "metresPerUnit": 1.0,
            "coordinateSystem": {"up": "+Y", "forward": "+Z"},
            "budgets": {"maxVertices": 96, "maxIndices": 144, "maxMaterials": 1,
                        "maxPrimitives": 1, "maxTextureLayersPerKind": 1},
            "lods": [{"name": "lod0", "maxTriangles": 48}], "requiredSockets": [],
            "runtimeTextureProfile": {"android": "astc", "windows": "rgba8",
                                      "mipmapped": True, "resolution": 1024},
            "materialOverrides": [], "distribution": "runtime",
            "licenceStatus": "Original project-authored F03 kerb and waystone geometry; "
            "no external mesh, texture or generation provider. Source recipe and exact "
            "provenance: assets/source/beyond_the_tomb/forest-road-clue-v1/README.md. "
            "HordeRoadMasonry is mapped to the existing licensed MedievalWall02 atlas; "
            "no new texture or blanket open-source licence is assigned. See ASSET_LICENSES.md."}


def inspect(data, spec, recipe):
    magic, version, total = struct.unpack_from("<III", data)
    require((magic, version, total) == (0x46546c67, 2, len(data)), "Invalid GLB header")
    json_size, json_type = struct.unpack_from("<II", data, 12)
    require(json_size % 4 == 0 and json_type == 0x4e4f534a, "Invalid JSON chunk")
    document = json.loads(data[20:20 + json_size])
    binary_size, binary_type = struct.unpack_from("<II", data, 20 + json_size)
    binary = data[28 + json_size:]
    require(binary_type == 0x004e4942 and binary_size == len(binary)
            and binary_size % 4 == 0, "Invalid binary chunk")
    require(document["buffers"] == [{"byteLength": binary_size}], "Unexpected buffer")
    require(document["scene"] == 0 and document["scenes"] == [{"nodes": [0]}], "Unexpected scene")
    require(document["nodes"] == [{"name": spec["name"], "mesh": 0}], "Nonidentity node")
    require(document["materials"] == [material(recipe["material"])], "Material contract changed")
    require(not any(key in document for key in ("images", "textures", "samplers", "skins",
                "animations", "cameras", "extensionsUsed", "extensionsRequired")), "Extra GLB payload")
    require(len(document["meshes"]) == 1 and len(document["meshes"][0]["primitives"]) == 1,
            "Expected one mesh and primitive")
    primitive = document["meshes"][0]["primitives"][0]
    require(primitive["mode"] == 4 and primitive["material"] == 0, "Unexpected primitive")

    def read(index, width, component_type, kind, target):
        accessor = document["accessors"][index]
        view = document["bufferViews"][accessor["bufferView"]]
        require(accessor["componentType"] == component_type and accessor["type"] == kind,
                "Unexpected accessor format")
        size = 4 if component_type == 5126 else 2
        require(view["buffer"] == 0 and view["target"] == target and view["byteOffset"] % 4 == 0,
                "Invalid buffer view")
        require(view["byteLength"] == accessor["count"] * width * size
                and view["byteOffset"] + view["byteLength"] <= len(binary), "Accessor out of bounds")
        values = struct.unpack_from("<" + ("f" if size == 4 else "H") * accessor["count"] * width,
                                    binary, view["byteOffset"])
        require(all(math.isfinite(v) for v in values), "Nonfinite attribute")
        return [tuple(values[i:i + width]) for i in range(0, len(values), width)]

    attributes = primitive["attributes"]
    require(set(attributes) == {"POSITION", "NORMAL", "TEXCOORD_0", "TANGENT"}, "Attribute roster")
    p = read(attributes["POSITION"], 3, 5126, "VEC3", 34962)
    n = read(attributes["NORMAL"], 3, 5126, "VEC3", 34962)
    uv = read(attributes["TEXCOORD_0"], 2, 5126, "VEC2", 34962)
    t = read(attributes["TANGENT"], 4, 5126, "VEC4", 34962)
    indices = [row[0] for row in read(primitive["indices"], 1, 5123, "SCALAR", 34963)]
    require(len(p) == len(n) == len(uv) == len(t) == spec["expectedVertices"], "Vertex count")
    require(len(indices) == spec["expectedTriangles"] * 3, "Index count")
    require(min(indices) >= 0 and max(indices) < len(p), "Invalid index")
    for normal, tangent in zip(n, t):
        require(abs(dot(normal, normal) - 1) < 2e-6 and abs(dot(tangent[:3], tangent[:3]) - 1) < 2e-6,
                "Nonunit normal/tangent")
        require(abs(dot(normal, tangent[:3])) < 2e-6 and tangent[3] in (-1.0, 1.0), "Invalid tangent basis")
    for start in range(0, len(indices), 3):
        a, b, c = indices[start:start + 3]
        ab, ac = subtract(p[b], p[a]), subtract(p[c], p[a])
        face_normal = unit(cross(ab, ac))
        require(all(dot(face_normal, n[i]) > 0.99999 for i in (a, b, c)), "Normal/winding mismatch")
        u1, v1 = subtract(uv[b], uv[a])
        u2, v2 = subtract(uv[c], uv[a])
        determinant = u1 * v2 - v1 * u2
        require(abs(determinant) > 1e-12, "Degenerate UV triangle")
        dpdu = scale(subtract(scale(ab, v2), scale(ac, v1)), 1 / determinant)
        dpdv = scale(subtract(scale(ac, u1), scale(ab, u2)), 1 / determinant)
        repeats = recipe["uvRepeatsPerMetre"]
        require(abs(math.sqrt(dot(dpdu, dpdu)) * repeats - 1) < 1e-5
                and abs(math.sqrt(dot(dpdv, dpdv)) * repeats - 1) < 1e-5, "UV metre scale")
        for i in (a, b, c):
            require(dot(unit(dpdu), t[i][:3]) > 0.99999
                    and dot(unit(dpdv), scale(cross(n[i], t[i][:3]), t[i][3])) > 0.99999,
                    "UV tangent handedness mismatch")
    volumes = []
    for component in range(len(p) // 24):
        welded, directed, volume = set(), Counter(), 0.0
        for start in range(component * 36, (component + 1) * 36, 3):
            triangle = indices[start:start + 3]
            require(all(component * 24 <= i < (component + 1) * 24 for i in triangle), "Component range")
            points = [p[i] for i in triangle]
            welded.update(points)
            for edge in range(3):
                directed[(points[edge], points[(edge + 1) % 3])] += 1
            volume += dot(points[0], cross(points[1], points[2])) / 6
        require(len(welded) == 8 and len(directed) == 36
                and all(count == 1 and directed[(b, a)] == 1 for (a, b), count in directed.items()),
                "Component is not closed and consistently wound after position welding")
        require(volume > 0, "Component has inward signed volume")
        volumes.append(volume)
    lower = [min(point[axis] for point in p) for axis in range(3)]
    upper = [max(point[axis] for point in p) for axis in range(3)]
    accessor = document["accessors"][attributes["POSITION"]]
    require(accessor["min"] == lower and accessor["max"] == upper, "Incorrect accessor bounds")
    return {"file": spec["file"], "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
            "vertices": len(p), "indices": len(indices), "triangles": len(indices) // 3,
            "meshes": 1, "primitives": 1, "materials": 1, "closedComponents": len(volumes),
            "minimumMetres": lower, "maximumMetres": upper, "componentSignedVolumesM3": volumes}


def prepare():
    recipe = json.loads(SOURCE.read_text(encoding="utf-8"))
    require(recipe["schema"] == 1 and recipe["metresPerUnit"] == 1.0
            and recipe["up"] == "+Y" and recipe["waystoneFront"] == "-Z"
            and recipe["material"] == "HordeRoadMasonry", "Source convention changed")
    require(math.isfinite(recipe["uvRepeatsPerMetre"]) and recipe["uvRepeatsPerMetre"] > 0,
            "Invalid UV scale")
    files, receipts = {}, []
    for spec in recipe["meshes"]:
        require(Path(spec["file"]).name == spec["file"] and spec["file"].endswith(".glb"), "Unsafe filename")
        data = glb(spec, recipe)
        require(data == glb(spec, recipe), "Nondeterministic mesh generation")
        files[spec["file"]] = data
        receipts.append(inspect(data, spec, recipe))
    files["asset.manifest.json"] = (json.dumps(manifest(recipe), indent=2, allow_nan=False) + "\n").encode("utf-8")
    return files, receipts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", type=Path, default=RUNTIME)
    parser.add_argument("--verify", action="store_true", help="Read-only exact regeneration and geometry check")
    args = parser.parse_args()
    output = args.output_directory.resolve()
    files, receipts = prepare()
    if args.verify:
        require(output.is_dir(), "Runtime directory does not exist")
        require({path.name for path in output.iterdir()} == set(files), "Unexpected runtime file roster")
        for name, expected in files.items():
            require((output / name).read_bytes() == expected, "Regeneration differs: " + name)
    else:
        # Check all destinations before creating any file; preserve existing data.
        require(not output.exists() or (output.is_dir() and not any(output.iterdir())),
                "Output must be new or empty; use --verify for existing runtime bytes")
        output.mkdir(parents=True, exist_ok=True)
        for name, data in files.items():
            with (output / name).open("xb") as stream:
                stream.write(data)
    manifest_bytes = files["asset.manifest.json"]
    print(json.dumps({"schema": 1, "operation": "verify" if args.verify else "generate",
                      "sourceRecipeSha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                      "manifestBytes": len(manifest_bytes),
                      "manifestSha256": hashlib.sha256(manifest_bytes).hexdigest(),
                      "files": receipts, "deterministicRegeneration": True,
                      "validationScope": "Static GLB bytes/geometry/UV/tangents only; no native import, RT presentation or visual acceptance claim."},
                     indent=2, allow_nan=False))


if __name__ == "__main__":
    main()
