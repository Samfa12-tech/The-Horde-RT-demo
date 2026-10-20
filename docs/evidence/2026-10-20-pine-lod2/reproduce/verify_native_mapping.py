"""Offline verification of one-pine omission IDs against an actual native export.

Usage: python reports/pine-lod2-20261020/verify_native_mapping.py EXPORT_DIRECTORY
The complete Debug CPU source export is read-only. Successful output goes beside
this script, or to --output-dir inside this worktree's ignored reports directory.
No native import/build, scene mutation, free transform fit or LOD acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import struct
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PIN_SOURCE = "db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f"
PIN_TERRAIN = "c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774"
PIN_DERIVATION = "a8945e481ea69160faef6ddf4d3e8141aec40ed10902e14bb37dd874599a7a65"
TREE_VERTICES, TREE_TRIANGLES, TREE_INDICES = 3660, 2928, 8784
PINE_ORDINAL, PLACEMENT_INDEX = 3, 7  # Zero based: fourth pine, eighth placement.
LIMIT = 1e-5
INPUTS = {}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    path = path.resolve()
    data = path.read_bytes()
    INPUTS[str(path)] = {"bytes": len(data), "sha256": digest(data)}
    return data


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def normalize(value):
    length = math.sqrt(dot(value, value))
    require(math.isfinite(length) and length > 1e-10, "Degenerate basis or face")
    return tuple(x / length for x in value)


def rotate(value, cosine, sine):
    return (cosine * value[0] + sine * value[2], value[1], -sine * value[0] + cosine * value[2])


def rotate_f32(value, cosine, sine):
    return (f32(f32(cosine * value[0]) + f32(sine * value[2])), value[1],
            f32(f32(-sine * value[0]) + f32(cosine * value[2])))


def identity_nodes(document, label):
    for node in document["nodes"]:
        require(not any(key in node for key in ("matrix", "translation", "rotation", "scale")),
                label + " source node transform is no longer identity")


def terrain_query(triangles, x, z):
    """QueryNode-equivalent topmost upward face; no authored ribbon or Y fit."""
    result = None
    for triangle_id, points in enumerate(triangles):
        a, b, c = points
        if not (min(p[0] for p in points) <= x <= max(p[0] for p in points) and
                min(p[2] for p in points) <= z <= max(p[2] for p in points)):
            continue
        ab, ac = tuple(b[k] - a[k] for k in range(3)), tuple(c[k] - a[k] for k in range(3))
        normal = tuple(f32(v) for v in normalize(cross(ab, ac)))
        if normal[1] <= 1e-7:
            continue
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        u = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
        v = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
        if min(u, v, 1 - u - v) < -1e-7:
            continue
        y = f32(u * a[1] + v * b[1] + (1 - u - v) * c[1])
        if result and (y < result["worldY"] or (y == result["worldY"] and triangle_id > result["triangle"])):
            continue
        result = {"worldY": y, "triangle": triangle_id, "normal": normal,
                  "walkable": math.hypot(normal[0], normal[2]) <= f32(.65) * normal[1] + 1e-7}
    # ForestDressing uses Query().found for upright tree/root placement. Steep
    # forest banks may grow trees; player walkability is a separate contract.
    require(result is not None, "Actual terrain has no root sample at " + str((x, z)))
    return result


def source_and_grounding(derive):
    source_path = ROOT / "assets/models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb"
    data = read(source_path)
    require(digest(data) == PIN_SOURCE, "Pinned admitted pine source changed")
    document, blob = derive.read_glb(data)
    identity_nodes(document, "Pine")
    manifest = json.loads(read(source_path.with_name("asset.manifest.json")))
    require(manifest["metresPerUnit"] == 1 and manifest["coordinateSystem"] == {"up": "+Y", "forward": "+Z"},
            "Pine metres/axis contract changed")
    primitive = document["meshes"][1]["primitives"][0]
    require(primitive["attributes"] == {"POSITION": 5, "NORMAL": 6, "TEXCOORD_0": 7, "TANGENT": 8} and
            primitive["indices"] == 9 and primitive["material"] == 1, "Needle source primitive identity changed")
    attributes = {key: derive.accessor(document, blob, aid) for key, aid in primitive["attributes"].items()}
    indices = [row[0] for row in derive.accessor(document, blob, 9)[0]]
    require(len(attributes["POSITION"][0]) == TREE_VERTICES and len(indices) == TREE_INDICES,
            "Needle source span changed")
    all_positions = [p for mesh in document["meshes"] for primitive in mesh["primitives"]
                     for p in derive.accessor(document, blob, primitive["attributes"]["POSITION"])[0]]
    source_minimum_y = min(p[1] for p in all_positions)
    header_path = ROOT / "src/gameplay/simulation/ForestTreePlacementContract.h"
    text = read(header_path).decode("utf-8")
    placements = [(species, [float(value.strip().removesuffix("f")) for value in row.split(",")])
                  for species, row in re.findall(r"\{ForestTreeSpecies::(Pine|Alder),\s*([^{}]+)\}", text)]
    require(len(placements) == 64 and all(len(row) == 8 for _, row in placements), "Placement contract parse failed")
    pine_placements = [i for i, (species, _) in enumerate(placements) if species == "Pine"]
    require(len(pine_placements) == 32 and pine_placements[PINE_ORDINAL] == PLACEMENT_INDEX,
            "Fourth pine is no longer contract placement7")
    authored = placements[PLACEMENT_INDEX][1]
    require(authored[0] == -22.298101214 and authored[1] == 17.699113131 and
            authored[2] == .628318531 and authored[-1] == .382095568, "Pinned placement7 changed")
    x, z, yaw, burial = (f32(authored[i]) for i in (0, 1, 2, 7))
    cosine, sine = f32(math.cos(yaw)), f32(math.sin(yaw))
    terrain_path = ROOT / "assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb"
    terrain_bytes = read(terrain_path)
    require(digest(terrain_bytes) == PIN_TERRAIN, "Pinned terrain for this experiment changed")
    terrain_doc, terrain_blob = derive.read_glb(terrain_bytes)
    identity_nodes(terrain_doc, "Terrain")
    terrain_manifest = json.loads(read(terrain_path.with_name("asset.manifest.json")))
    require(terrain_manifest["metresPerUnit"] == 1 and terrain_manifest["coordinateSystem"] == {"up": "+Y", "forward": "+Z"},
            "Terrain metres/axis contract changed")
    triangles = []
    for mesh in terrain_doc["meshes"]:
        for primitive in mesh["primitives"]:
            points = derive.accessor(terrain_doc, terrain_blob, primitive["attributes"]["POSITION"])[0]
            local_indices = [row[0] for row in derive.accessor(terrain_doc, terrain_blob, primitive["indices"])[0]]
            triangles.extend(tuple(points[i] for i in local_indices[t:t + 3]) for t in range(0, len(local_indices), 3))
    support = terrain_query(triangles, x, z)
    unburied_y = f32(support["worldY"] - source_minimum_y)
    origin_y = f32(unburied_y - burial)
    fitted_y, tip_count, steep_tip_count = unburied_y, 0, 0
    for position in all_positions:
        if position[1] > f32(source_minimum_y + f32(.02)):
            continue
        tip = rotate_f32(position, cosine, sine)
        ground = terrain_query(triangles, f32(x + tip[0]), f32(z + tip[2]))
        steep_tip_count += not ground["walkable"]
        fitted_y = min(fitted_y, f32(f32(ground["worldY"] - tip[1]) - f32(.02)))
        tip_count += 1
    require(tip_count >= 18 and unburied_y - fitted_y <= .65 and abs(fitted_y - origin_y) <= .002,
            "Actual root footprint no longer matches canonical burial receipt")
    grounding = {"placementIndex": PLACEMENT_INDEX, "pineOrdinal": PINE_ORDINAL,
                 "authoredX": authored[0], "authoredZ": authored[1], "authoredYawRadians": authored[2],
                 "float32X": x, "float32Z": z, "float32YawRadians": yaw, "float32RootBurialMetres": burial,
                 "sourceMetresPerUnit": 1, "sourceAxes": "+Y up, +Z forward; no node transform",
                 "terrainSupport": support, "sourceMinimumY": source_minimum_y,
                 "fixedOrigin": [x, origin_y, z], "recomputedRootFitY": fitted_y,
                 "rootFitVsReceiptResidualM": abs(fitted_y - origin_y), "rootTipSamples": tip_count,
                 "rootTipSamplesOnSteepGround": steep_tip_count,
                 "treeGroundingQueryPolicy": "Highest upward Query face found; walkability recorded, not required by ForestDressing",
                 "fitPolicy": "Contract x/z/yaw fixed. OriginY from Query-equivalent actual terrain and authored burial; no fitted correction used."}
    return attributes, indices, grounding, cosine, sine


def native_inputs(directory):
    metadata = json.loads(read(directory / "static-world-source.json"))
    require(metadata["schema"] == 1 and metadata["completeContributors"] is True and
            metadata["owner"]["instance"] == 21, "Native export is not the complete expected world owner")
    require(metadata["vertexLayout"] == {"strideBytes": 64, "position": 0, "normal": 16, "tangent": 32,
                                        "uv0": 48, "fieldFormat": "float32x4", "byteOrder": "little-endian"} and
            metadata["indexStrideBytes"] == 4 and metadata["regionStrideBytes"] == 1,
            "Native binary layout differs from the frozen schema")
    require(metadata["regionValues"] == {"Shared": 0, "TombDressing": 1, "ExteriorDressing": 2},
            "Native region identities changed")
    selected = metadata["selected"]
    require(selected["materialName"] == "ForestPine" and selected["indicesArePrimitiveLocal"] is True,
            "Export must select the primitive-local ForestPine family")
    rows = [p for p in metadata["primitives"] if p["materialName"] == "ForestPine"]
    require(len(rows) == 1, "ForestPine family is not unique")
    row = rows[0]
    require(row["ownerLocalRow"] == selected["ownerLocalRow"] and
            row["cachedPrimitiveRow"] == metadata["owner"]["primitiveBase"] + row["ownerLocalRow"] and
            row["indexOffset"] % 3 == 0 and row["triangleOffset"] == row["indexOffset"] // 3 and
            selected["cachedTriangleOffset"] == row["triangleOffset"], "Cached native ownership offsets disagree")
    require(row["vertexCount"] == 32 * TREE_VERTICES and row["indexCount"] == 32 * TREE_INDICES and
            row["triangleCount"] == 32 * TREE_TRIANGLES and row["regionCounts"] == [0, 0, 32 * TREE_TRIANGLES],
            "Selected family is not exactly32 complete pine needle primitives")
    expected = {"static-world-selected.vertices.bin": (64 * row["vertexCount"], row["verticesSha256"]),
                "static-world-selected.indices.bin": (4 * row["indexCount"], row["indicesSha256"]),
                "static-world-selected.regions.bin": (row["triangleCount"], row["regionsSha256"])}
    files = {entry["name"]: entry for entry in selected["files"]}
    require(set(files) == set(expected) and len(selected["files"]) == 3, "Native export file roster changed")
    payloads = {}
    for name, (count, expected_hash) in expected.items():
        data = read(directory / name)
        require(len(data) == count == files[name]["bytes"] and
                digest(data) == expected_hash == files[name]["sha256"], "Native file digest/span mismatch: " + name)
        payloads[name] = data
    require(set(payloads["static-world-selected.regions.bin"]) == {2}, "Native selected triangles are not all ExteriorDressing")
    return metadata, row, payloads


def verify_mapping(attributes, source_indices, grounding, cosine, sine, payloads):
    vertex_bytes = payloads["static-world-selected.vertices.bin"]
    native_indices = [i[0] for i in struct.iter_unpack("<I", payloads["static-world-selected.indices.bin"])]
    for ordinal in range(32):
        expected = [i + ordinal * TREE_VERTICES for i in source_indices]
        require(native_indices[ordinal * TREE_INDICES:(ordinal + 1) * TREE_INDICES] == expected,
                "Ordered source index/winding/vertex-ordinal mapping differs at pine ordinal " + str(ordinal))
    uv_bytes = attributes["TEXCOORD_0"][1]
    for ordinal in range(32):
        for i in range(TREE_VERTICES):
            offset = (ordinal * TREE_VERTICES + i) * 64
            require(vertex_bytes[offset + 48:offset + 56] == uv_bytes[i * 8:i * 8 + 8],
                    "UV0 bytes changed at pine/vertex " + str((ordinal, i)))
            require(vertex_bytes[offset + 12:offset + 16] == struct.pack("<f", 1) and
                    vertex_bytes[offset + 28:offset + 32] == bytes(4) and
                    vertex_bytes[offset + 56:offset + 64] == bytes(8), "Native fourth-channel/padding contract changed")
    native = list(struct.iter_unpack("<16f", vertex_bytes[PINE_ORDINAL * TREE_VERTICES * 64:(PINE_ORDINAL + 1) * TREE_VERTICES * 64]))
    require(all(math.isfinite(x) for vertex in native for x in vertex), "Fourth pine has nonfinite native attributes")
    positions, normals, tangents = (attributes[key][0] for key in ("POSITION", "NORMAL", "TANGENT"))
    origin = grounding["fixedOrigin"]
    maxima = {"positionComponentM": 0.0, "positionVectorM": 0.0,
              "normalComponent": 0.0, "normalVector": 0.0, "tangentComponent": 0.0, "tangentVector": 0.0}
    minimum_normal_cosine, minimum_tangent_cosine = 1.0, 1.0
    inferred_origins = []
    for i, current in enumerate(native):
        rotated_position = rotate(positions[i], cosine, sine)
        expected_position = tuple(rotated_position[k] + origin[k] for k in range(3))
        normal = normalize(normals[i])  # Identity-node native inverse transpose + normalization.
        tangent = normalize(tuple(tangents[i][k] - normal[k] * dot(tangents[i][:3], normal) for k in range(3)))
        expected_normal, expected_tangent = rotate(normal, cosine, sine), rotate(tangent, cosine, sine)
        for label, expected, actual in (("position", expected_position, current[0:3]),
                                        ("normal", expected_normal, current[4:7]), ("tangent", expected_tangent, current[8:11])):
            difference = tuple(actual[k] - expected[k] for k in range(3))
            suffix = "M" if label == "position" else ""
            maxima[label + "Component" + suffix] = max(maxima[label + "Component" + suffix], *(abs(d) for d in difference))
            maxima[label + "Vector" + suffix] = max(maxima[label + "Vector" + suffix], math.sqrt(dot(difference, difference)))
        require(current[11] == tangents[i][3] and current[11] in (-1.0, 1.0), "Tangent handedness changed")
        minimum_normal_cosine = min(minimum_normal_cosine, dot(normalize(expected_normal), normalize(current[4:7])))
        minimum_tangent_cosine = min(minimum_tangent_cosine, dot(normalize(expected_tangent), normalize(current[8:11])))
        inferred_origins.append(tuple(current[k] - rotated_position[k] for k in range(3)))
    require(all(value < LIMIT for value in maxima.values()), "Fixed contract/terrain transform residual exceeds1e-5: " + str(maxima))
    minimum_face_cosine = 1.0
    for offset in range(0, TREE_INDICES, 3):
        ids = source_indices[offset:offset + 3]
        a, b, c = (positions[i] for i in ids)
        expected = rotate(normalize(cross(tuple(b[k] - a[k] for k in range(3)),
                                          tuple(c[k] - a[k] for k in range(3)))), cosine, sine)
        a, b, c = (native[i][0:3] for i in ids)
        actual = normalize(cross(tuple(b[k] - a[k] for k in range(3)), tuple(c[k] - a[k] for k in range(3))))
        minimum_face_cosine = min(minimum_face_cosine, dot(normalize(expected), actual))
    require(minimum_face_cosine > .999, "Native face winding/parity differs from source")
    return {"all32IndexRepetitionsExact": True, "verifiedNativeIndices": len(native_indices),
            "all32Uv0BytesExact": True, "all32NativeFourthChannelsExact": True,
            "fourthPineVerticesVerified": TREE_VERTICES, "fourthPineTrianglesVerified": TREE_TRIANGLES,
            "fixedTransformMaximumResiduals": maxima, "strictResidualLimit": LIMIT,
            "minimumNormalDirectionCosine": minimum_normal_cosine, "minimumTangentDirectionCosine": minimum_tangent_cosine,
            "tangentHandednessExact": True, "minimumFaceWindingCosine": minimum_face_cosine,
            "diagnosticInferredOriginRanges": {"min": [min(p[k] for p in inferred_origins) for k in range(3)],
                                              "max": [max(p[k] for p in inferred_origins) for k in range(3)]},
            "floatArithmeticClaim": "Quantified float residuals only; reconstructed transforms/bases are not claimed bit-exact"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export_directory", type=Path)
    parser.add_argument("--output-dir", type=Path, default=HERE)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    require(output.is_dir() and output.is_relative_to((ROOT / "reports").resolve()) and output != ROOT / "reports",
            "Output must be an existing child of this worktree's ignored reports directory")
    read(Path(__file__))
    derivation_path = HERE / "derive_pine_lod2.py"
    require(digest(read(derivation_path)) == PIN_DERIVATION, "Original deterministic derivation script changed")
    reproducibility = json.loads(read(HERE / "reproducibility-receipt.json"))
    require(reproducibility["identicalRerun"] is True and reproducibility["sourceSha256"] == PIN_SOURCE,
            "Original derivation reproducibility receipt changed")
    for name, expected in reproducibility["sha256"].items():
        require(Path(name).name == name and digest(read(HERE / name)) == expected, "Original derivation artifact changed: " + name)
    spec = importlib.util.spec_from_file_location("pine_lod2_derivation", derivation_path)
    derive = importlib.util.module_from_spec(spec)
    # Avoid __pycache__ writes: this verifier owns only its explicit output files.
    previous_bytecode = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    try:
        spec.loader.exec_module(derive)
    finally:
        sys.dont_write_bytecode = previous_bytecode
    attributes, source_indices, grounding, cosine, sine = source_and_grounding(derive)
    metadata, row, payloads = native_inputs(args.export_directory.resolve())
    mapping = verify_mapping(attributes, source_indices, grounding, cosine, sine, payloads)
    omissions = json.loads(read(HERE / "omitted-needle-triangles.json"))
    require(omissions["sourceSha256"] == PIN_SOURCE and omissions["mesh"] == 1 and omissions["primitive"] == 0,
            "Omission source identity differs")
    omitted = set(omissions["omittedLocalTriangleIds"])
    require(len(omitted) == len(omissions["omittedLocalTriangleIds"]) == 768, "Unexpected or duplicate omitted triangle IDs")
    components = derive.components(attributes["POSITION"][0], source_indices)
    complete_omitted = [c for c in components if set(c["triangles"]).issubset(omitted)]
    require(len(components) == 366 and len(complete_omitted) == 96 and
            {t for c in complete_omitted for t in c["triangles"]} == omitted and
            [c["id"] for c in complete_omitted] == omissions["omittedComponentIds"] and
            all(c["closed"] and c["consistentWinding"] and c["euler"] == 2 and
                len(c["triangles"]) == 8 and c["weldedVertices"] == 6 for c in components),
            "Omissions are not exactly96 whole closed source lenses")
    local_base = PINE_ORDINAL * TREE_TRIANGLES
    require(local_base == 8784, "Fourth-pine selected-family triangle base changed")
    global_ids = [row["indexOffset"] // 3 + local_base + t for t in sorted(omitted)]
    require(len(set(global_ids)) == 768 and min(global_ids) >= row["triangleOffset"] and
            max(global_ids) < row["triangleOffset"] + row["triangleCount"], "Global IDs escaped actual selected primitive")
    for t in omitted:
        start = (local_base + t) * 3
        native = struct.unpack_from("<3I", payloads["static-world-selected.indices.bin"], start * 4)
        require(native == tuple(i + PINE_ORDINAL * TREE_VERTICES for i in source_indices[t * 3:t * 3 + 3]),
                "Omitted global triangle does not map to its original ordered source face")
    for name in ("src/platform/windows/WindowsStaticGeometrySourceInspection.inl", "src/scene/ForestDressing.cpp",
                 "src/scene/assets/StaticMeshAsset.cpp", "src/gameplay/simulation/TriangleSupportSurface.h",
                 "src/vulkan/raytracing/RtSceneAbi.generated.h"):
        read(ROOT / name)
    evidence = {"schema": 1, "verification": "passed", "inputs": INPUTS, "nativeOwner": metadata["owner"],
                "nativeCheckpoint": metadata["checkpoint"], "nativeStaticGeometryGeneration": metadata["staticGeometryGeneration"],
                "selected": row, "allSelectedRegionsExteriorDressing": True, "grounding": grounding, "mapping": mapping,
                "omissionClosedness": {"sourceComponents": 366, "omittedComponents": 96, "omittedTriangles": 768,
                                      "partialComponentsOmitted": 0},
                "globalFormula": "actual ForestPine indexOffset / 3 + 8784 + source needle local triangle ID",
                "globalMinimum": min(global_ids), "globalMaximum": max(global_ids),
                "validationBoundary": "Actual complete cached CPU source mapping and original closed-component omission proof only; not GPU visibility, LOD cutover, performance or device acceptance"}
    global_omissions = {"schema": 1, "sourceSha256": PIN_SOURCE,
                        "nativeMetadataSha256": INPUTS[str((args.export_directory.resolve() / "static-world-source.json").resolve())]["sha256"],
                        "selectedMaterial": "ForestPine", "selectedIndexOffset": row["indexOffset"],
                        "selectedCachedTriangleOffset": row["triangleOffset"],
                        "placementIndex": PLACEMENT_INDEX, "pineOrdinal": PINE_ORDINAL,
                        "selectedFamilyTriangleBase": local_base, "sourceLocalTriangleIds": sorted(omitted),
                        "globalTriangleIds": global_ids, "verifiedWholeClosedComponents": 96}
    products = {"global-omissions.json": json.dumps(global_omissions, indent=2) + "\n",
                "global-omissions.txt": " ".join(map(str, global_ids)) + "\n"}
    evidence["outputs"] = {name: {"bytes": len(text.encode("utf-8")), "sha256": digest(text.encode("utf-8"))}
                           for name, text in products.items()}
    products["native-mapping-verification.json"] = json.dumps(evidence, indent=2, allow_nan=False) + "\n"
    for name, text in products.items():
        (output / name).write_text(text, encoding="utf-8", newline="\n")
    print(json.dumps({"verification": "passed", "outputDirectory": str(output), "globalTriangleIds": len(global_ids),
                      "globalMinimum": min(global_ids), "globalMaximum": max(global_ids),
                      "maximumResiduals": mapping["fixedTransformMaximumResiduals"], "outputs": evidence["outputs"]}))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, AssertionError, KeyError, OSError, struct.error) as error:
        print("Native mapping verification FAILED; no new omission outputs published: " + str(error), file=sys.stderr)
        sys.exit(1)
