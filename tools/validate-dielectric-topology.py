"""Reject open or non-manifold thick dielectric primitives in a runtime GLB."""

from __future__ import annotations

import collections
import json
import pathlib
import struct
import sys


GLB_MAGIC = 0x46546C67
JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942
COMPONENTS = {5121: ("B", 1), 5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4)}
TYPE_WIDTHS = {"SCALAR": 1, "VEC3": 3}


class ValidationError(RuntimeError):
    pass


def read_glb(path: pathlib.Path) -> tuple[dict, bytes]:
    payload = path.read_bytes()
    if len(payload) < 20:
        raise ValidationError(f"{path}: malformed GLB header")
    magic, version, declared_length = struct.unpack_from("<III", payload)
    if magic != GLB_MAGIC or version != 2 or declared_length != len(payload):
        raise ValidationError(f"{path}: malformed GLB header or length")
    offset = 12
    document = None
    binary = b""
    while offset + 8 <= len(payload):
        length, chunk_type = struct.unpack_from("<II", payload, offset)
        offset += 8
        if offset + length > len(payload):
            raise ValidationError(f"{path}: truncated GLB chunk")
        chunk = payload[offset : offset + length]
        offset += length
        if chunk_type == JSON_CHUNK:
            document = json.loads(chunk.rstrip(b"\x00 ").decode("utf-8"))
        elif chunk_type == BIN_CHUNK:
            binary = chunk
    if document is None:
        raise ValidationError(f"{path}: missing GLB JSON chunk")
    return document, binary


def read_accessor(document: dict, binary: bytes, accessor_index: int) -> list[tuple]:
    accessor = document["accessors"][accessor_index]
    if "sparse" in accessor:
        raise ValidationError(f"accessor {accessor_index} is sparse and cannot be topology-validated")
    view = document["bufferViews"][accessor["bufferView"]]
    if view.get("buffer", 0) != 0:
        raise ValidationError(f"accessor {accessor_index} does not use the embedded GLB buffer")
    component_type = accessor["componentType"]
    accessor_type = accessor["type"]
    if component_type not in COMPONENTS or accessor_type not in TYPE_WIDTHS:
        raise ValidationError(
            f"accessor {accessor_index} uses unsupported component/type {component_type}/{accessor_type}")
    code, component_bytes = COMPONENTS[component_type]
    width = TYPE_WIDTHS[accessor_type]
    item_bytes = width * component_bytes
    stride = view.get("byteStride", item_bytes)
    if stride < item_bytes:
        raise ValidationError(f"accessor {accessor_index} has an invalid byte stride")
    start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    values = []
    for item_index in range(accessor["count"]):
        item_offset = start + item_index * stride
        if item_offset + item_bytes > len(binary):
            raise ValidationError(f"accessor {accessor_index} exceeds the GLB binary chunk")
        values.append(struct.unpack_from("<" + code * width, binary, item_offset))
    return values


def material_properties(material: dict, overrides: dict[str, dict]) -> tuple[float, float, bool]:
    extensions = material.get("extensions", {})
    transmission = float(
        extensions.get("KHR_materials_transmission", {}).get("transmissionFactor", 0.0))
    thickness = float(extensions.get("KHR_materials_volume", {}).get("thicknessFactor", 0.0))
    thin_wall = False
    override = overrides.get(material.get("name", ""), {})
    if "transmissionFactor" in override:
        transmission = float(override["transmissionFactor"])
    if "thicknessFactor" in override:
        thickness = float(override["thicknessFactor"])
    if "thinWall" in override:
        thin_wall = bool(override["thinWall"])
    return transmission, thickness, thin_wall


def validate(path: pathlib.Path, manifest_path: pathlib.Path) -> None:
    document, binary = read_glb(path)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    overrides = {
        entry.get("material", ""): entry for entry in manifest.get("materialOverrides", [])
    }
    materials = document.get("materials", [])
    thick_materials = set()
    for material_index, material in enumerate(materials):
        transmission, thickness, thin_wall = material_properties(material, overrides)
        if transmission > 0.0 and thickness > 0.0 and not thin_wall:
            thick_materials.add(material_index)

    checked_primitives = 0
    for mesh_index, mesh in enumerate(document.get("meshes", [])):
        for primitive_index, primitive in enumerate(mesh.get("primitives", [])):
            material_index = primitive.get("material", -1)
            if material_index not in thick_materials:
                continue
            if primitive.get("mode", 4) != 4:
                raise ValidationError(
                    f"mesh {mesh_index} primitive {primitive_index}: thick dielectric geometry must use TRIANGLES")
            position_accessor = primitive.get("attributes", {}).get("POSITION")
            if position_accessor is None:
                raise ValidationError(
                    f"mesh {mesh_index} primitive {primitive_index}: thick dielectric geometry has no POSITION accessor")
            positions = read_accessor(document, binary, position_accessor)
            if document["accessors"][position_accessor]["componentType"] != 5126:
                raise ValidationError(
                    f"mesh {mesh_index} primitive {primitive_index}: thick dielectric POSITION must use finite FLOAT data")
            if "indices" in primitive:
                raw_indices = read_accessor(document, binary, primitive["indices"])
                indices = [int(value[0]) for value in raw_indices]
            else:
                indices = list(range(len(positions)))
            if len(indices) == 0 or len(indices) % 3:
                raise ValidationError(
                    f"mesh {mesh_index} primitive {primitive_index}: thick dielectric triangle index count is invalid")
            edges: collections.Counter[tuple[tuple[int, int, int], tuple[int, int, int]]] = collections.Counter()
            for triangle_offset in range(0, len(indices), 3):
                try:
                    triangle = [
                        tuple(round(float(value) * 100000.0) for value in positions[index])
                        for index in indices[triangle_offset : triangle_offset + 3]
                    ]
                except IndexError as error:
                    raise ValidationError(
                        f"mesh {mesh_index} primitive {primitive_index}: triangle index exceeds POSITION count") from error
                for corner in range(3):
                    a, b = triangle[corner], triangle[(corner + 1) % 3]
                    if a == b:
                        raise ValidationError(
                            f"mesh {mesh_index} primitive {primitive_index}: thick dielectric has a degenerate edge")
                    edges[tuple(sorted((a, b)))] += 1
            boundary_count = sum(references == 1 for references in edges.values())
            non_manifold_count = sum(references > 2 for references in edges.values())
            material_name = materials[material_index].get("name", f"material {material_index}")
            if boundary_count:
                raise ValidationError(
                    f"{path}: open thick dielectric volume '{material_name}' has {boundary_count} boundary edge(s); close/weld the volume or set thinWall=true in the audited manifest override")
            if non_manifold_count:
                raise ValidationError(
                    f"{path}: non-manifold thick dielectric volume '{material_name}' has {non_manifold_count} edge(s) referenced more than twice; repair the topology or set thinWall=true only for an intentional pane")
            checked_primitives += 1

    print(f"Dielectric topology validation passed: {checked_primitives} closed/manifold thick primitive(s) in {path}")


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: validate-dielectric-topology.py <runtime.glb> <asset.manifest.json>", file=sys.stderr)
        return 2
    try:
        validate(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]))
        return 0
    except (OSError, ValueError, KeyError, json.JSONDecodeError, ValidationError) as error:
        print(f"Dielectric topology validation failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
