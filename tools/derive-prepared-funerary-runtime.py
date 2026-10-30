#!/usr/bin/env python3
"""Add deterministic, texture-free TEXCOORD_0 data to prepared T02 GLBs.

The Horde native static-GLB loader currently requires TEXCOORD_0 even when a
material has no texture maps. This narrow derivative keeps source geometry,
indices, normals, materials and scene hierarchy intact and appends a planar UV
attribute derived from each primitive's existing positions. It does not merge,
subdivide, regenerate, remesh, texture, or admit the source geometry.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


GLB_MAGIC = b"glTF"
JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942
SOURCES = {
    "skull-jaw": ("meshes/skull_jaw.glb", "tomb-skull-jaw-lod0.runtime.glb"),
    "femur": ("meshes/femur.glb", "tomb-femur-lod0.runtime.glb"),
    "humerus": ("meshes/humerus.glb", "tomb-humerus-lod0.runtime.glb"),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_glb(data: bytes) -> tuple[dict, bytes]:
    if len(data) < 20:
        raise ValueError("GLB is shorter than its header and first chunk")
    magic, version, total_length = struct.unpack_from("<4sII", data, 0)
    if magic != GLB_MAGIC or version != 2 or total_length != len(data):
        raise ValueError("Expected a valid GLB 2.0 container")
    offset = 12
    document = None
    binary = None
    while offset < len(data):
        chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
        offset += 8
        chunk = data[offset : offset + chunk_length]
        if len(chunk) != chunk_length:
            raise ValueError("Truncated GLB chunk")
        offset += chunk_length
        if chunk_type == JSON_CHUNK:
            if document is not None:
                raise ValueError("Multiple JSON chunks are unsupported")
            document = json.loads(chunk.decode("utf-8").rstrip(" \t\r\n\0"))
        elif chunk_type == BIN_CHUNK:
            if binary is not None:
                raise ValueError("Multiple BIN chunks are unsupported")
            binary = chunk
        else:
            raise ValueError(f"Unsupported GLB chunk type 0x{chunk_type:08x}")
    if document is None or binary is None:
        raise ValueError("Expected one JSON and one BIN chunk")
    buffers = document.get("buffers", [])
    if len(buffers) != 1 or "uri" in buffers[0]:
        raise ValueError("Expected one embedded, URI-free GLB buffer")
    return document, binary


def primitive_positions(document: dict, binary: bytes, accessor_index: int) -> list[tuple[float, float, float]]:
    accessors = document.get("accessors", [])
    views = document.get("bufferViews", [])
    accessor = accessors[accessor_index]
    if accessor.get("componentType") != 5126 or accessor.get("type") != "VEC3":
        raise ValueError("POSITION accessor must be FLOAT VEC3")
    if accessor.get("sparse") or "bufferView" not in accessor:
        raise ValueError("Sparse or implicit POSITION accessors are unsupported")
    view = views[accessor["bufferView"]]
    if view.get("buffer", 0) != 0:
        raise ValueError("POSITION references a non-primary buffer")
    stride = view.get("byteStride", 12)
    if stride < 12:
        raise ValueError("POSITION bufferView stride is too small")
    first = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    result = []
    for index in range(accessor["count"]):
        position = struct.unpack_from("<3f", binary, first + index * stride)
        if not all(math.isfinite(component) for component in position):
            raise ValueError("POSITION contains a non-finite coordinate")
        result.append(position)
    return result


def add_uvs(document: dict, binary: bytes) -> tuple[dict, bytes, int, int]:
    buffer_views = document.setdefault("bufferViews", [])
    accessors = document.setdefault("accessors", [])
    primitive_count = 0
    uv_count = 0
    for mesh in document.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            attributes = primitive.get("attributes", {})
            if "TEXCOORD_0" in attributes:
                raise ValueError("Source already has TEXCOORD_0; refusing to rewrite it")
            if "POSITION" not in attributes:
                raise ValueError("Primitive is missing POSITION")
            positions = primitive_positions(document, binary, attributes["POSITION"])
            if not positions:
                raise ValueError("Primitive has an empty POSITION accessor")
            minima = [min(position[axis] for position in positions) for axis in range(3)]
            maxima = [max(position[axis] for position in positions) for axis in range(3)]
            axes = sorted(range(3), key=lambda axis: maxima[axis] - minima[axis], reverse=True)[:2]
            spans = [maxima[axis] - minima[axis] for axis in axes]
            uv_bytes = bytearray()
            u_values = []
            v_values = []
            for position in positions:
                uv = tuple(
                    0.5 if span <= 1.0e-12 else (position[axis] - minima[axis]) / span
                    for axis, span in zip(axes, spans)
                )
                u_values.append(uv[0])
                v_values.append(uv[1])
                uv_bytes.extend(struct.pack("<2f", *uv))
            while len(binary) % 4:
                binary += b"\0"
            byte_offset = len(binary)
            binary += bytes(uv_bytes)
            buffer_views.append({
                "buffer": 0,
                "byteOffset": byte_offset,
                "byteLength": len(uv_bytes),
                "target": 34962,
            })
            accessors.append({
                "bufferView": len(buffer_views) - 1,
                "componentType": 5126,
                "count": len(positions),
                "max": [max(u_values), max(v_values)],
                "min": [min(u_values), min(v_values)],
                "type": "VEC2",
            })
            attributes["TEXCOORD_0"] = len(accessors) - 1
            primitive_count += 1
            uv_count += len(positions)
    if primitive_count == 0:
        raise ValueError("GLB contains no mesh primitives")
    document["buffers"][0]["byteLength"] = len(binary)
    return document, binary, primitive_count, uv_count


def primitive_signature(document: dict) -> list[dict]:
    """Capture source geometry references while ignoring newly derived UVs."""
    result = []
    accessors = document.get("accessors", [])
    for mesh in document.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            attributes = {
                name: accessor for name, accessor in primitive.get("attributes", {}).items()
                if name != "TEXCOORD_0"
            }
            result.append({
                "mode": primitive.get("mode", 4),
                "indices": primitive.get("indices"),
                "attributes": attributes,
                "material": primitive.get("material"),
            })
    return result


def encode_glb(document: dict, binary: bytes) -> bytes:
    json_bytes = json.dumps(document, separators=(",", ":"), sort_keys=True, ensure_ascii=False).encode("utf-8")
    json_bytes += b" " * ((-len(json_bytes)) % 4)
    binary += b"\0" * ((-len(binary)) % 4)
    total = 12 + 8 + len(json_bytes) + 8 + len(binary)
    return (
        struct.pack("<4sII", GLB_MAGIC, 2, total)
        + struct.pack("<II", len(json_bytes), JSON_CHUNK)
        + json_bytes
        + struct.pack("<II", len(binary), BIN_CHUNK)
        + binary
    )


def derive(source_root: Path, output_root: Path) -> dict:
    if output_root.exists() and any(output_root.iterdir()):
        raise ValueError(f"Refusing to overwrite non-empty output directory: {output_root}")
    output_root.mkdir(parents=True, exist_ok=True)
    records = []
    for asset_name, (source_relative, runtime_name) in SOURCES.items():
        source_path = source_root / source_relative
        source_data = source_path.read_bytes()
        source_document, source_binary = parse_glb(source_data)
        document = json.loads(json.dumps(source_document))
        source_signature = primitive_signature(source_document)
        source_buffer_views = len(source_document.get("bufferViews", []))
        source_accessors = len(source_document.get("accessors", []))
        source_positions = []
        for mesh in source_document.get("meshes", []):
            for primitive in mesh.get("primitives", []):
                source_positions.extend(primitive_positions(
                    source_document, source_binary, primitive["attributes"]["POSITION"]
                ))
        minima = [min(position[axis] for position in source_positions) for axis in range(3)]
        maxima = [max(position[axis] for position in source_positions) for axis in range(3)]
        document, binary, primitive_count, uv_count = add_uvs(document, source_binary)
        runtime_data = encode_glb(document, binary)
        runtime_path = output_root / asset_name / runtime_name
        runtime_path.parent.mkdir(parents=True, exist_ok=True)
        runtime_path.write_bytes(runtime_data)
        # Reparse the result and verify that the only new attributes are UV data.
        reparsed, runtime_binary = parse_glb(runtime_data)
        if any("TEXCOORD_0" not in p.get("attributes", {})
               for m in reparsed.get("meshes", []) for p in m.get("primitives", [])):
            raise ValueError(f"Derived GLB failed its UV reparse check: {runtime_path}")
        if primitive_signature(reparsed) != source_signature:
            raise ValueError(f"Derived GLB changed a source geometry reference: {runtime_path}")
        if len(reparsed.get("bufferViews", [])) != source_buffer_views + primitive_count:
            raise ValueError(f"Derived GLB added an unexpected buffer view: {runtime_path}")
        if len(reparsed.get("accessors", [])) != source_accessors + primitive_count:
            raise ValueError(f"Derived GLB added an unexpected accessor: {runtime_path}")
        if not runtime_binary.startswith(source_binary):
            raise ValueError(f"Derived GLB did not preserve its complete source binary payload: {runtime_path}")
        records.append({
            "asset": asset_name,
            "source": source_relative.replace("\\", "/"),
            "sourceBytes": len(source_data),
            "sourceSha256": sha256(source_data),
            "runtime": runtime_path.relative_to(output_root).as_posix(),
            "runtimeBytes": len(runtime_data),
            "runtimeSha256": sha256(runtime_data),
            "triangles": 0,
            "boundsGlbMeters": {
                "min": [round(value, 9) for value in minima],
                "max": [round(value, 9) for value in maxima],
                "dimensions": [round(maxima[axis] - minima[axis], 9) for axis in range(3)],
                "basis": "glTF +Y up, +Z forward; metres per T02 source package",
            },
            "geometrySignaturePreserved": True,
            "primitiveCount": primitive_count,
            "uvVertexCount": uv_count,
            "derivation": "append deterministic dominant-axis planar TEXCOORD_0 from source positions; preserve source geometry, indices, normals, materials and scene nodes",
        })
        # Triangle totals come from indexed accessor counts, without decoding geometry.
        source_document, _ = parse_glb(source_data)
        triangles = 0
        for mesh in source_document.get("meshes", []):
            for primitive in mesh.get("primitives", []):
                if "indices" in primitive:
                    index_accessor = source_document["accessors"][primitive["indices"]]
                    triangles += index_accessor["count"] // 3
                else:
                    position_accessor = source_document["accessors"][primitive["attributes"]["POSITION"]]
                    triangles += position_accessor["count"] // 3
        records[-1]["triangles"] = triangles
    receipt = {
        "status": "candidate-only; not admitted",
        "reason": "Horde static GLB loader requires TEXCOORD_0 on texture-free primitives",
        "geometryChanged": False,
        "texturesAdded": False,
        "sourceRoot": "assets/source/beyond_the_tomb/funerary/t02-skeletal-source-v1",
        "assets": records,
    }
    (output_root / "derivation-receipt.json").write_text(
        json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n"
    )
    (output_root / "README.md").write_text(
        "# Prepared T02 native-import candidates\n\n"
        "These candidate-only GLBs derive from the recovered T02 source package. "
        "They append deterministic UV coordinates because the Horde static GLB loader "
        "currently requires `TEXCOORD_0`, including for texture-free materials. "
        "Geometry, indices, normals, materials, and node transforms are preserved; "
        "no texture, remesh, merge, subdivision, or master rebuild is performed. "
        "See `derivation-receipt.json` for exact source/runtime hashes. Root-owned "
        "licensing, runtime manifest admission, placement, and collision remain pending.\n",
        encoding="utf-8",
    )
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path("assets/source/beyond_the_tomb/funerary/t02-skeletal-source-v1"))
    parser.add_argument("--output-root", type=Path, default=Path("assets/models/world/runtime/prepared-funerary-v01/t02-native-import-candidates"))
    args = parser.parse_args()
    receipt = derive(args.source_root.resolve(), args.output_root.resolve())
    print(json.dumps(receipt, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
