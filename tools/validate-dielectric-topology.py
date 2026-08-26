"""Reject open or non-manifold thick dielectric primitives in a runtime GLB."""

from __future__ import annotations

import collections
import json
import math
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
    if transmission > 0.0 and thickness <= 0.0:
        thin_wall = True
    return transmission, thickness, thin_wall


def multiply_matrices(left: list[float], right: list[float]) -> list[float]:
    return [sum(left[k * 4 + row] * right[column * 4 + k] for k in range(4))
            for column in range(4) for row in range(4)]


def local_matrix(node: dict) -> list[float]:
    if "matrix" in node:
        matrix = [float(value) for value in node["matrix"]]
        if len(matrix) != 16:
            raise ValidationError("node matrix must contain 16 finite values")
        return matrix
    tx, ty, tz = (float(value) for value in node.get("translation", [0, 0, 0]))
    x, y, z, w = (float(value) for value in node.get("rotation", [0, 0, 0, 1]))
    sx, sy, sz = (float(value) for value in node.get("scale", [1, 1, 1]))
    return [
        (1 - 2 * (y * y + z * z)) * sx,
        (2 * (x * y + z * w)) * sx,
        (2 * (x * z - y * w)) * sx, 0,
        (2 * (x * y - z * w)) * sy,
        (1 - 2 * (x * x + z * z)) * sy,
        (2 * (y * z + x * w)) * sy, 0,
        (2 * (x * z + y * w)) * sz,
        (2 * (y * z - x * w)) * sz,
        (1 - 2 * (x * x + y * y)) * sz, 0,
        tx, ty, tz, 1,
    ]


def determinant(matrix: list[float]) -> float:
    return (matrix[0] * (matrix[5] * matrix[10] - matrix[9] * matrix[6])
            - matrix[4] * (matrix[1] * matrix[10] - matrix[9] * matrix[2])
            + matrix[8] * (matrix[1] * matrix[6] - matrix[5] * matrix[2]))


def transform_point(matrix: list[float], point: tuple) -> tuple[float, float, float]:
    x, y, z = (float(value) for value in point)
    return (matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
            matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
            matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14])


def node_world_matrices(document: dict) -> list[list[float]]:
    nodes = document.get("nodes", [])
    parents: dict[int, int] = {}
    for parent_index, node in enumerate(nodes):
        for child_index in node.get("children", []):
            parents[int(child_index)] = parent_index
    cache: dict[int, list[float]] = {}

    def resolve(index: int, active: set[int]) -> list[float]:
        if index in cache:
            return cache[index]
        if index in active:
            raise ValidationError("node hierarchy contains a cycle")
        active.add(index)
        local = local_matrix(nodes[index])
        world = (multiply_matrices(resolve(parents[index], active), local)
                 if index in parents else local)
        active.remove(index)
        if any(not math.isfinite(value) for value in world):
            raise ValidationError(f"node {index} contains a non-finite transform")
        if determinant(world) < 0.0:
            name = nodes[index].get("name", "<unnamed>")
            raise ValidationError(
                f"node '{name}' has a negative-determinant transform; bake the reflection and reverse triangle winding/normals before runtime import")
        cache[index] = world
        return world

    return [resolve(index, set()) for index in range(len(nodes))]


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

    nodes = document.get("nodes", [])
    world_matrices = node_world_matrices(document)
    checked_primitives = 0
    for node_index, node in enumerate(nodes):
        if "mesh" not in node:
            continue
        mesh_index = int(node["mesh"])
        mesh = document.get("meshes", [])[mesh_index]
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
            positions = [transform_point(world_matrices[node_index], value)
                         for value in read_accessor(document, binary, position_accessor)]
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
            edges: collections.defaultdict[
                tuple[tuple[int, int, int], tuple[int, int, int]], list[int]
            ] = collections.defaultdict(lambda: [0, 0])
            signed_six_times_volume = 0.0
            volume_origin = None
            for triangle_offset in range(0, len(indices), 3):
                try:
                    triangle_positions = [positions[index]
                                          for index in indices[triangle_offset : triangle_offset + 3]]
                    triangle = [tuple(round(float(value) * 100000.0) for value in position)
                                for position in triangle_positions]
                except IndexError as error:
                    raise ValidationError(
                        f"mesh {mesh_index} primitive {primitive_index}: triangle index exceeds POSITION count") from error
                for corner in range(3):
                    a, b = triangle[corner], triangle[(corner + 1) % 3]
                    if a == b:
                        raise ValidationError(
                            f"mesh {mesh_index} primitive {primitive_index}: thick dielectric has a degenerate edge")
                    ascending = a < b
                    key = (a, b) if ascending else (b, a)
                    edges[key][0 if ascending else 1] += 1
                a, b, c = triangle_positions
                if volume_origin is None:
                    volume_origin = a
                a = tuple(a[axis] - volume_origin[axis] for axis in range(3))
                b = tuple(b[axis] - volume_origin[axis] for axis in range(3))
                c = tuple(c[axis] - volume_origin[axis] for axis in range(3))
                signed_six_times_volume += (
                    a[0] * (b[1] * c[2] - b[2] * c[1])
                    + a[1] * (b[2] * c[0] - b[0] * c[2])
                    + a[2] * (b[0] * c[1] - b[1] * c[0]))
            boundary_count = sum(sum(references) == 1 for references in edges.values())
            non_manifold_count = sum(sum(references) > 2 for references in edges.values())
            inconsistent_count = sum(references != [1, 1] for references in edges.values()
                                     if sum(references) == 2)
            material_name = materials[material_index].get("name", f"material {material_index}")
            if boundary_count:
                raise ValidationError(
                    f"{path}: open thick dielectric volume '{material_name}' has {boundary_count} boundary edge(s); close/weld the volume or set thinWall=true in the audited manifest override")
            if non_manifold_count:
                raise ValidationError(
                    f"{path}: non-manifold thick dielectric volume '{material_name}' has {non_manifold_count} edge(s) referenced more than twice; repair the topology or set thinWall=true only for an intentional pane")
            if inconsistent_count:
                raise ValidationError(
                    f"{path}: thick dielectric volume '{material_name}' has inconsistent winding; each shared edge must be used once in each direction")
            if not math.isfinite(signed_six_times_volume) or signed_six_times_volume <= 1.0e-18:
                raise ValidationError(
                    f"{path}: thick dielectric volume '{material_name}' is inward-wound after baked node transforms; reverse every triangle winding so normals face outward")
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
