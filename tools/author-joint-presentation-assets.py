"""Author/verify the bounded ring v1 and UV-only flask v1 candidates offline.

Uses only Python's standard library and the existing strict source-flask checks.
Only the four named new asset folders can be written; different existing bytes
are refused. --verify reads/regenerates in memory and never writes. Neither mode
admits a runtime roster, changes capacities, invokes Blender, or launches a game.
"""

import argparse
from collections import Counter
import copy
import hashlib
import json
import math
from pathlib import Path
import runpy
import struct

ROOT = Path(__file__).resolve().parents[1]
PROPS = ROOT / "assets/models/props"
RING_SOURCE = PROPS / "source/attack-warning-ring-v1"
RING_RUNTIME = PROPS / "runtime/attack-warning-ring-v1"
FLASK_SOURCE = PROPS / "source/healing-flask-v1"
FLASK_RUNTIME = PROPS / "runtime/healing-flask-v1"
FLASK_INPUT = PROPS / "source/healing-flask-neck-fit-r2/healing-flask-neck-fit-r2.glb"
FLASK_INPUT_SHA = "4a4337e81d98065c050ab896b8b0b61563feeedac55806665d2727ad14555cfa"
RIGHTS = ("Original project-authored geometry supplied for Horde; no third-party mesh, "
          "image, texture, provider or paid generation. Applicable user/service rights "
          "remain unchanged; no blanket MIT, CC0, Meshy or other licence is assigned.")
PENDING = ("Candidate only. Native importer/capacity integration, real RT presentation, "
           "lifecycle and owner visual acceptance remain pending; normal enablement stays OFF.")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def json_bytes(value):
    return (json.dumps(value, indent=2, allow_nan=False) + "\n").encode("utf-8")


def relative(path):
    return path.relative_to(ROOT).as_posix()


def unpack_glb(data):
    require(len(data) >= 28, "Truncated GLB")
    require(struct.unpack_from("<III", data) == (0x46546C67, 2, len(data)), "Bad GLB header")
    size, kind = struct.unpack_from("<II", data, 12)
    require(kind == 0x4E4F534A and size % 4 == 0 and 28 + size <= len(data), "Bad JSON chunk")
    doc = json.loads(data[20:20 + size])
    binary_size, kind = struct.unpack_from("<II", data, 20 + size)
    binary = data[28 + size:]
    require(kind == 0x004E4942 and binary_size == len(binary) and len(binary) % 4 == 0,
            "Bad BIN chunk")
    require(doc["asset"]["version"] == "2.0" and len(doc["buffers"]) == 1,
            "Expected one embedded glTF 2.0 buffer")
    logical_size = doc["buffers"][0]["byteLength"]
    require("uri" not in doc["buffers"][0] and 0 <= len(binary) - logical_size <= 3,
            "Unexpected external buffer or padding")
    for view in doc["bufferViews"]:
        require(view["buffer"] == 0 and view.get("byteOffset", 0) % 4 == 0,
                "Invalid buffer-view alignment")
        require(0 <= view.get("byteOffset", 0) < logical_size and
                view["byteLength"] > 0 and
                view.get("byteOffset", 0) + view["byteLength"] <= logical_size,
                "Buffer view out of range")
    return doc, binary


def pack_glb(doc, binary):
    payload = json.dumps(doc, separators=(",", ":"), allow_nan=False).encode("utf-8")
    payload += b" " * (-len(payload) % 4)
    binary = bytes(binary) + b"\0" * (-len(binary) % 4)
    return (struct.pack("<III", 0x46546C67, 2, 28 + len(payload) + len(binary)) +
            struct.pack("<II", len(payload), 0x4E4F534A) + payload +
            struct.pack("<II", len(binary), 0x004E4942) + binary)


def attribute(doc, binary, index):
    accessor = doc["accessors"][index]
    widths = {"SCALAR": 1, "VEC2": 2, "VEC3": 3}
    codes = {5123: "H", 5126: "f"}
    require(accessor["type"] in widths and accessor["componentType"] in codes,
            "Unsupported candidate accessor")
    require(not accessor.get("normalized", False) and "sparse" not in accessor,
            "Unexpected accessor encoding")
    width, code = widths[accessor["type"]], codes[accessor["componentType"]]
    view = doc["bufferViews"][accessor["bufferView"]]
    step = struct.calcsize("<" + code * width)
    require("byteStride" not in view, "Unexpected interleaving")
    offset = accessor.get("byteOffset", 0)
    require(offset + step * accessor["count"] <= view["byteLength"], "Accessor out of range")
    start = view.get("byteOffset", 0) + offset
    return [struct.unpack_from("<" + code * width, binary, start + i * step)
            for i in range(accessor["count"])]


def add_attribute(doc, binary, rows, kind, component, code, target):
    binary.extend(b"\0" * (-len(binary) % 4))
    offset = len(binary)
    width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3}[kind]
    binary.extend(struct.pack("<" + code * len(rows) * width,
                              *(v for row in rows for v in row)))
    doc["bufferViews"].append({"buffer": 0, "byteOffset": offset,
                               "byteLength": len(binary) - offset, "target": target})
    accessor = {"bufferView": len(doc["bufferViews"]) - 1, "componentType": component,
                "count": len(rows), "type": kind}
    doc["accessors"].append(accessor)
    return len(doc["accessors"]) - 1


def ring_recipe():
    return {"schema": 1, "asset": "attack-warning-ring-v1", "status": "candidate-unadmitted",
            "metresPerUnit": 1, "coordinateSystem": {"up": "+Y", "forward": "+Z"},
            "geometry": {"segments": 16, "innerRadius": 0.88, "outerRadius": 1.0,
                         "plane": "local XY", "normal": [0, 0, 1],
                         "vertices": 32, "triangles": 32,
                         "topology": "Two vertices per segment, shared seam, two CCW triangles per segment"},
            "uv0": "Planar: (0.5 + 0.5*x, 0.5 + 0.5*y); finite loader-required UV0, no texture",
            "material": {"name": "AttackWarningAmber", "pbrMetallicRoughness": {
                "baseColorFactor": [0.72, 0.09, 0.015, 1.0], "metallicFactor": 0.0,
                "roughnessFactor": 1.0}, "alphaMode": "OPAQUE", "doubleSided": True,
                "emissiveFactor": [1.0, 0.16, 0.015]},
            "textures": 0, "lights": 0, "provenance": RIGHTS,
            "authoringScript": "tools/author-joint-presentation-assets.py", "acceptance": PENDING}


def author_ring(recipe):
    positions, indices = [], []
    for segment in range(16):
        angle = math.tau * segment / 16
        for radius in (0.88, 1.0):
            positions.append((radius * math.cos(angle), radius * math.sin(angle), 0.0))
    for segment in range(16):
        inner, outer = 2 * segment, 2 * segment + 1
        next_inner, next_outer = 2 * ((segment + 1) % 16), 2 * ((segment + 1) % 16) + 1
        indices.extend((inner, outer, next_outer, inner, next_outer, next_inner))
    doc = {"asset": {"version": "2.0", "generator": "Horde original attack-warning annulus v1"},
           "scene": 0, "scenes": [{"nodes": [0]}],
           "nodes": [{"name": "AttackWarningRing", "mesh": 0}],
           "meshes": [{"name": "AttackWarningRing", "primitives": []}],
           "materials": [recipe["material"]], "accessors": [], "bufferViews": [], "buffers": []}
    binary = bytearray()
    position = add_attribute(doc, binary, positions, "VEC3", 5126, "f", 34962)
    stored = attribute(doc, binary, position)
    doc["accessors"][position]["min"] = [min(p[a] for p in stored) for a in range(3)]
    doc["accessors"][position]["max"] = [max(p[a] for p in stored) for a in range(3)]
    normal = add_attribute(doc, binary, [(0, 0, 1)] * 32, "VEC3", 5126, "f", 34962)
    uv0 = add_attribute(doc, binary, [(0.5 + 0.5*x, 0.5 + 0.5*y) for x, y, _ in positions],
                        "VEC2", 5126, "f", 34962)
    index = add_attribute(doc, binary, [(i,) for i in indices], "SCALAR", 5123, "H", 34963)
    doc["meshes"][0]["primitives"] = [{"attributes": {
        "POSITION": position, "NORMAL": normal, "TEXCOORD_0": uv0},
        "indices": index, "material": 0, "mode": 4}]
    doc["buffers"] = [{"byteLength": len(binary)}]
    return pack_glb(doc, binary)


def derive_flask(source):
    doc, original_binary = unpack_glb(source)
    primitive = doc["meshes"][0]["primitives"][0]
    require(set(primitive["attributes"]) == {"POSITION", "NORMAL"}, "Flask source layout changed")
    positions = attribute(doc, original_binary, primitive["attributes"]["POSITION"])
    low = [min(p[a] for p in positions) for a in range(3)]
    high = [max(p[a] for p in positions) for a in range(3)]
    uv = [((p[0] - low[0]) / (high[0] - low[0]),
           (p[1] - low[1]) / (high[1] - low[1])) for p in positions]
    binary = bytearray(original_binary[:doc["buffers"][0]["byteLength"]])
    primitive["attributes"]["TEXCOORD_0"] = add_attribute(doc, binary, uv, "VEC2", 5126, "f", 34962)
    doc["buffers"][0]["byteLength"] = len(binary)
    return pack_glb(doc, binary)


def manifest(name, vertices, triangles, sockets):
    return {"schema": 1, "asset": name, "metresPerUnit": 1.0,
            "coordinateSystem": {"up": "+Y", "forward": "+Z"},
            "budgets": {"maxVertices": vertices, "maxIndices": triangles * 3,
                        "maxPrimitives": 1, "maxMaterials": 1, "maxTextureLayersPerKind": 1},
            "lods": [{"name": "lod0", "maxTriangles": triangles}],
            "requiredSockets": sockets,
            "runtimeTextureProfile": {"android": "astc", "windows": "rgba8", "mipmapped": True},
            "materialOverrides": [], "distribution": "candidate-unadmitted", "licenceStatus": RIGHTS}


def inspect(data, expected_vertices, expected_triangles):
    doc, binary = unpack_glb(data)
    require(len(doc["meshes"]) == len(doc["meshes"][0]["primitives"]) == len(doc["materials"]) == 1,
            "Expected exactly one mesh/primitive/material")
    require(not any(k in doc for k in ("textures", "images", "samplers", "skins", "animations",
                                        "extensions", "extensionsUsed", "extensionsRequired")),
            "Unexpected texture/light/animation/extension payload")
    primitive = doc["meshes"][0]["primitives"][0]
    require(primitive["mode"] == 4 and primitive["material"] == 0, "Expected ordinary PBR triangles")
    require(set(primitive["attributes"]) == {"POSITION", "NORMAL", "TEXCOORD_0"}, "Bad attribute roster")
    positions, normals, uv0 = [attribute(doc, binary, primitive["attributes"][name])
                              for name in ("POSITION", "NORMAL", "TEXCOORD_0")]
    indices = [row[0] for row in attribute(doc, binary, primitive["indices"])]
    require(len(positions) == len(normals) == len(uv0) == expected_vertices and
            len(indices) == expected_triangles * 3, "Candidate geometry counts differ")
    require(all(math.isfinite(v) for row in positions + normals + uv0 for v in row), "Nonfinite attributes")
    require(all(abs(sum(v*v for v in n) - 1) < 1e-6 for n in normals), "Nonunit normal")
    require(all(0 <= v <= 1 for row in uv0 for v in row), "UV0 outside planar unit range")
    material = doc["materials"][0]
    require(material["alphaMode"] == "OPAQUE" and "extensions" not in material,
            "Unexpected alpha or extended material")
    material_values = material["emissiveFactor"] + list(material["pbrMetallicRoughness"]["baseColorFactor"])
    material_values += [material["pbrMetallicRoughness"][name] for name in ("metallicFactor", "roughnessFactor")]
    require(all(math.isfinite(v) and 0 <= v <= 1 for v in material_values), "Invalid constant PBR factor")
    low = [min(p[a] for p in positions) for a in range(3)]
    high = [max(p[a] for p in positions) for a in range(3)]
    accessor = doc["accessors"][primitive["attributes"]["POSITION"]]
    require(accessor["min"] == low and accessor["max"] == high, "Accessor bounds differ")
    min_area, edges = math.inf, Counter()
    for start in range(0, len(indices), 3):
        triangle = indices[start:start + 3]
        require(len(set(triangle)) == 3 and min(triangle) >= 0 and max(triangle) < len(positions), "Bad index")
        a, b, c = [positions[i] for i in triangle]
        ab, ac = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
        face = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
        area = math.sqrt(sum(v*v for v in face)) / 2
        require(area > 1e-10, "Degenerate triangle")
        require(sum(face[axis] * sum(normals[i][axis] for i in triangle) for axis in range(3)) > 0,
                "Winding disagrees with authored normals")
        min_area = min(min_area, area)
        for u, v in ((a, b), (b, c), (c, a)):
            edges[(u, v)] += 1
    require(all(count == 1 for count in edges.values()), "Duplicate directed edge")
    boundary = sum((v, u) not in edges for u, v in edges)
    return {"sha256": sha(data), "bytes": len(data), "vertices": len(positions),
            "indices": len(indices), "triangles": len(indices)//3, "primitives": 1, "materials": 1,
            "texturePayloads": 0, "lights": 0, "minimumMetres": low, "maximumMetres": high,
            "dimensionsMetres": [hi-lo for lo, hi in zip(low, high)],
            "finiteAttributes": True, "unitNormals": True, "finiteUv0": True,
            "boundsMatchStoredFloat32": True, "windingMatchesNormals": True,
            "minimumTriangleAreaM2": min_area, "boundaryEdgesAfterExactPositionWeld": boundary}


def check_flask_preservation(source, runtime):
    before, old_bin = unpack_glb(source)
    after, new_bin = unpack_glb(runtime)
    preserved = []
    for index, view in enumerate(before["bufferViews"]):
        start, length = view.get("byteOffset", 0), view["byteLength"]
        old = old_bin[start:start + length]
        require(after["bufferViews"][index] == view and old == new_bin[start:start + length],
                "Existing flask buffer view changed")
        preserved.append({"bufferView": index, "bytes": length, "sha256": sha(old)})
    restored = copy.deepcopy(after)
    del restored["meshes"][0]["primitives"][0]["attributes"]["TEXCOORD_0"]
    restored["accessors"].pop()
    restored["bufferViews"].pop()
    restored["buffers"] = before["buffers"]
    require(restored == before, "Flask derivative changed existing JSON semantics")
    return preserved


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    source = FLASK_INPUT.read_bytes()
    require(len(source) == 9292 and sha(source) == FLASK_INPUT_SHA, "Owned r2 flask source identity changed")
    old_recipe = json.loads((FLASK_INPUT.parent / "recipe.json").read_text(encoding="utf-8"))
    source_tools = runpy.run_path(str(ROOT / "tools/author-healing-flask-study.py"))
    require(source_tools["encode"](old_recipe) == source, "Exact source-flask regeneration differs")
    source_measurement = source_tools["inspect"](source, old_recipe)
    ring_spec = ring_recipe()
    ring, flask = author_ring(ring_spec), derive_flask(source)
    require(ring == author_ring(ring_spec) and flask == derive_flask(source), "Nondeterministic GLB derivation")
    ring_check, flask_check = inspect(ring, 32, 32), inspect(flask, 242, 384)
    require(ring_check["boundaryEdgesAfterExactPositionWeld"] == 32, "Expected two open annulus boundaries")
    ring_doc, ring_bin = unpack_glb(ring)
    ring_pos = attribute(ring_doc, ring_bin, 0)
    require(all(p[2] == 0 for p in ring_pos), "Annulus left the XY plane")
    require(all(abs(math.hypot(p[0], p[1]) - (0.88 if i % 2 == 0 else 1.0)) < 1e-7
                for i, p in enumerate(ring_pos)), "Annulus radii differ")
    require(all(n == (0, 0, 1) for n in attribute(ring_doc, ring_bin, 1)), "Ring normal differs")
    require(ring_doc["materials"] == [ring_spec["material"]], "Ring material changed")
    require(flask_check["boundaryEdgesAfterExactPositionWeld"] == 0, "Flask lost closed topology")
    source_tools["inspect"](flask, old_recipe)
    preserved = check_flask_preservation(source, flask)
    flask_spec = {"schema": 1, "asset": "healing-flask-v1", "status": "candidate-unadmitted",
                  "source": relative(FLASK_INPUT), "sourceSha256": FLASK_INPUT_SHA, "sourceBytes": 9292,
                  "metresPerUnit": 1, "coordinateSystem": {"up": "+Y", "forward": "+Z"},
                  "derivation": "Append one FLOAT VEC2 TEXCOORD_0 accessor; no other JSON semantics or existing buffer-view bytes change",
                  "loaderReason": "StaticMeshAsset requires TEXCOORD_0 for every primitive, including untextured PBR",
                  "uv0": "Planar X/Y over stored float32 bounds: ((x-minX)/(maxX-minX), (y-minY)/(maxY-minY)); no texture",
                  "grip": old_recipe["grip"], "textures": 0, "lights": 0, "provenance": RIGHTS,
                  "authoringScript": "tools/author-joint-presentation-assets.py", "acceptance": PENDING}
    common = {"schema": 1, "authoringScript": "tools/author-joint-presentation-assets.py",
              "authoringScriptSha256": sha(Path(__file__).read_bytes()),
              "validationClass": "Focused offline stdlib GLB/accessor/geometry checks; native import and hardware RT not run",
              "acceptance": PENDING, "provenance": RIGHTS}
    ring_receipt = {**common, "sourceRecipeSha256": sha(json_bytes(ring_spec)),
                    "sourceGlbSha256": sha(ring), "runtime": ring_check,
                    "derivation": "Source and runtime GLBs are byte-identical; original code-authored planar annulus",
                    "material": ring_spec["material"], "closedSurface": False,
                    "annulus": ring_spec["geometry"], "newTextureLayers": 0, "newLights": 0}
    flask_receipt = {**common, "sourceRecipeSha256": sha(json_bytes(flask_spec)),
                     "source": relative(FLASK_INPUT), "sourceMeasurement": source_measurement,
                     "runtime": flask_check, "preservedSourceBufferViews": preserved,
                     "preservedMaterial": old_recipe["material"], "preservedGrip": old_recipe["grip"],
                     "closedSurface": True, "addedUv0Bytes": 242 * 2 * 4,
                     "derivation": flask_spec["derivation"], "newTextureLayers": 0, "newLights": 0}
    files = {
        RING_SOURCE / "recipe.json": json_bytes(ring_spec),
        RING_SOURCE / "attack-warning-ring-v1.glb": ring,
        RING_SOURCE / "README.md": ("# Attack warning annulus v1 candidate\n\n"
            "Original 16-segment annulus in local XY, normal +Z, inner radius 0.88 m and outer radius 1 m. "
            "It has 32 vertices/32 triangles, two open boundary loops and one opaque double-sided constant amber "
            "PBR/emissive material. It contains no texture or light. The source GLB equals the runtime GLB byte for byte. "
            "The instance transform supplies placement/radius; this asset introduces no shader branch.\n\n" +
            RIGHTS + "\n\n" + PENDING + "\n\nReproduce/verify from the worktree root with "
            "`python tools/author-joint-presentation-assets.py --verify`. The default invocation creates "
            "only missing files and refuses different existing bytes. The recipe is the editable source.\n").encode(),
        RING_RUNTIME / "attack-warning-ring-v1.runtime.glb": ring,
        RING_RUNTIME / "asset.manifest.json": json_bytes(manifest("attack-warning-ring-v1", 32, 32, [])),
        RING_RUNTIME / "processing-receipt.json": json_bytes(ring_receipt),
        FLASK_SOURCE / "recipe.json": json_bytes(flask_spec),
        FLASK_SOURCE / "README.md": ("# Healing flask v1 runtime derivation recipe\n\n"
            "The owned source remains `../healing-flask-neck-fit-r2/healing-flask-neck-fit-r2.glb`, "
            "9,292 bytes, SHA256 `" + FLASK_INPUT_SHA + "`. That source and recipe are unchanged. "
            "The loader requires UV0, so the candidate appends a finite planar X/Y UV accessor to 242 existing vertices. "
            "All existing buffer views, positions, normals, 384 triangles, material, node names and exact Grip are preserved; "
            "there are no new textures/lights or shader branches. No Blender or provider round trip is used. "
            "The source r2 recipe remains the geometry authority.\n\n" + RIGHTS + "\n\n" + PENDING +
            "\n\nRun `python tools/author-joint-presentation-assets.py --verify` for in-memory exact regeneration, "
            "source identity, preservation, geometry and manifest-byte checks.\n").encode(),
        FLASK_RUNTIME / "healing-flask-v1.runtime.glb": flask,
        FLASK_RUNTIME / "asset.manifest.json": json_bytes(manifest("healing-flask-v1", 242, 384, ["Grip"])),
        FLASK_RUNTIME / "processing-receipt.json": json_bytes(flask_receipt),
    }
    # Preflight all collisions before making any directory/file writes.
    roots = tuple(path.resolve() for path in (RING_SOURCE, RING_RUNTIME, FLASK_SOURCE, FLASK_RUNTIME))
    for path, data in files.items():
        require(any(path.resolve().is_relative_to(root) for root in roots), "Write outside owned asset folders")
        require(not path.exists() or path.read_bytes() == data, "Refusing different existing bytes: " + str(path))
        if args.verify:
            require(path.is_file(), "Missing generated output: " + str(path))
    if not args.verify:
        for path, data in files.items():
            if not path.exists():
                path.parent.mkdir(parents=True, exist_ok=True)
                with path.open("xb") as stream:
                    stream.write(data)
    require(FLASK_INPUT.read_bytes() == source, "Source flask changed during operation")
    print(json.dumps({"operation": "read-only-verify" if args.verify else "new-only-author",
                      "ring": ring_check, "flask": flask_check, "sourceFlaskSha256": FLASK_INPUT_SHA,
                      "outputs": [{"path": relative(path), "sha256": sha(data), "bytes": len(data)}
                                  for path, data in files.items()], "acceptance": PENDING}, indent=2))


if __name__ == "__main__":
    main()
