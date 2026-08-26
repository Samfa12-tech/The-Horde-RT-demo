"""Validate edge-connected closed components for each thick dielectric material."""

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
FLOAT32_MAX = 3.4028234663852886e38
MINIMUM_WELD_TOLERANCE_METRES = 1.0e-7
MAXIMUM_WELD_TOLERANCE_METRES = 1.0e-5
EXCLUSIVE_WELD_CELL_COORDINATE_LIMIT = float.fromhex("0x1p63")


class ValidationError(RuntimeError):
    pass


def runtime_float(value: float) -> float:
    converted = float(value)
    if not math.isfinite(converted) or abs(converted) > FLOAT32_MAX:
        return math.copysign(math.inf, converted)
    return struct.unpack("<f", struct.pack("<f", converted))[0]


def runtime_add(left: float, right: float) -> float:
    return runtime_float(runtime_float(left) + runtime_float(right))


def runtime_subtract(left: float, right: float) -> float:
    return runtime_float(runtime_float(left) - runtime_float(right))


def runtime_multiply(left: float, right: float) -> float:
    return runtime_float(runtime_float(left) * runtime_float(right))


def manifest_metres_per_unit(manifest: dict) -> float:
    try:
        value = float(manifest["metresPerUnit"])
    except (KeyError, TypeError, ValueError, OverflowError) as error:
        raise ValidationError(
            "Asset manifest metresPerUnit must be finite and greater than zero") from error
    value = runtime_float(value)
    if not math.isfinite(value) or value <= 0.0:
        raise ValidationError(
            "Asset manifest metresPerUnit must be finite and greater than zero")
    return value


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


def local_matrix(node: dict) -> list[float]:
    if "matrix" in node:
        matrix = [runtime_float(value) for value in node["matrix"]]
        if len(matrix) != 16:
            raise ValidationError("node matrix must contain 16 finite values")
        return matrix
    tx, ty, tz = (runtime_float(value) for value in node.get("translation", [0, 0, 0]))
    x, y, z, w = (runtime_float(value) for value in node.get("rotation", [0, 0, 0, 1]))
    sx, sy, sz = (runtime_float(value) for value in node.get("scale", [1, 1, 1]))
    two = runtime_float(2.0)
    one = runtime_float(1.0)

    def twice_product(left: float, right: float) -> float:
        return runtime_multiply(runtime_multiply(two, left), right)

    # Match cgltf_node_transform_local expression-for-expression. Every
    # cgltf_float product and add/subtract rounds to float32 before the next
    # operation; Python double regrouping would move seams at micrometre scale.
    return [
        runtime_multiply(runtime_subtract(
            runtime_subtract(one, twice_product(y, y)),
            twice_product(z, z)), sx),
        runtime_multiply(runtime_add(
            twice_product(x, y), twice_product(z, w)), sx),
        runtime_multiply(runtime_subtract(
            twice_product(x, z), twice_product(y, w)), sx), runtime_float(0.0),
        runtime_multiply(runtime_subtract(
            twice_product(x, y), twice_product(z, w)), sy),
        runtime_multiply(runtime_subtract(
            runtime_subtract(one, twice_product(x, x)),
            twice_product(z, z)), sy),
        runtime_multiply(runtime_add(
            twice_product(y, z), twice_product(x, w)), sy), runtime_float(0.0),
        runtime_multiply(runtime_add(
            twice_product(x, z), twice_product(y, w)), sz),
        runtime_multiply(runtime_subtract(
            twice_product(y, z), twice_product(x, w)), sz),
        runtime_multiply(runtime_subtract(
            runtime_subtract(one, twice_product(x, x)),
            twice_product(y, y)), sz), runtime_float(0.0),
        tx, ty, tz, runtime_float(1.0),
    ]


def determinant(matrix: list[float]) -> float:
    return (matrix[0] * (matrix[5] * matrix[10] - matrix[9] * matrix[6])
            - matrix[4] * (matrix[1] * matrix[10] - matrix[9] * matrix[2])
            + matrix[8] * (matrix[1] * matrix[6] - matrix[5] * matrix[2]))


def transform_point_metres(matrix: list[float], point: tuple,
                           metres_per_unit: float) -> tuple[float, float, float]:
    x, y, z = (runtime_float(value) for value in point)
    result = []
    for row in range(3):
        first = runtime_multiply(runtime_multiply(matrix[row], x), metres_per_unit)
        second = runtime_multiply(runtime_multiply(matrix[4 + row], y), metres_per_unit)
        third = runtime_multiply(runtime_multiply(matrix[8 + row], z), metres_per_unit)
        translation = runtime_multiply(matrix[12 + row], metres_per_unit)
        result.append(runtime_add(
            runtime_add(runtime_add(first, second), third), translation))
    return tuple(result)


def node_world_matrices(document: dict) -> list[list[float]]:
    nodes = document.get("nodes", [])
    parents: dict[int, int] = {}
    for parent_index, node in enumerate(nodes):
        for child_index in node.get("children", []):
            parents[int(child_index)] = parent_index
    cache: dict[int, list[float]] = {}

    def resolve(index: int) -> list[float]:
        if index in cache:
            return cache[index]
        world = list(local_matrix(nodes[index]))
        parent_index = parents.get(index)
        visited = {index}
        while parent_index is not None:
            if parent_index in visited:
                raise ValidationError("node hierarchy contains a cycle")
            visited.add(parent_index)
            parent = local_matrix(nodes[parent_index])
            # Match cgltf_node_transform_world exactly: transform each existing
            # column by the parent's 3x3 linear part, then add parent translation.
            for column in range(4):
                offset = column * 4
                left0 = world[offset]
                left1 = world[offset + 1]
                left2 = world[offset + 2]
                result0 = runtime_add(runtime_add(
                    runtime_multiply(left0, parent[0]),
                    runtime_multiply(left1, parent[4])),
                    runtime_multiply(left2, parent[8]))
                result1 = runtime_add(runtime_add(
                    runtime_multiply(left0, parent[1]),
                    runtime_multiply(left1, parent[5])),
                    runtime_multiply(left2, parent[9]))
                result2 = runtime_add(runtime_add(
                    runtime_multiply(left0, parent[2]),
                    runtime_multiply(left1, parent[6])),
                    runtime_multiply(left2, parent[10]))
                world[offset] = result0
                world[offset + 1] = result1
                world[offset + 2] = result2
            world[12] = runtime_add(world[12], parent[12])
            world[13] = runtime_add(world[13], parent[13])
            world[14] = runtime_add(world[14], parent[14])
            parent_index = parents.get(parent_index)
        if any(not math.isfinite(value) for value in world):
            raise ValidationError(f"node {index} contains a non-finite transform")
        if determinant(world) < 0.0:
            name = nodes[index].get("name", "<unnamed>")
            raise ValidationError(
                f"node '{name}' has a negative-determinant transform; bake the reflection and reverse triangle winding/normals before runtime import")
        cache[index] = world
        return world

    return [resolve(index) for index in range(len(nodes))]


def dielectric_weld_tolerance(triangles: list[dict]) -> float:
    minimum = tuple(min(triangle["positions"][corner][axis]
                        for triangle in triangles for corner in range(3))
                    for axis in range(3))
    maximum = tuple(max(triangle["positions"][corner][axis]
                        for triangle in triangles for corner in range(3))
                    for axis in range(3))
    extent = max(maximum[axis] - minimum[axis] for axis in range(3))
    # Baked positions are metres. One millionth of material extent, clamped
    # to 0.1-10 micrometres, welds transform-rounding equivalents while keeping
    # authored shells separated by more than the documented 10 um ceiling.
    return max(MINIMUM_WELD_TOLERANCE_METRES,
               min(MAXIMUM_WELD_TOLERANCE_METRES, extent * 1.0e-6))


def weld_cell_coordinate(scaled: float) -> int:
    if (not math.isfinite(scaled) or
            not (-EXCLUSIVE_WELD_CELL_COORDINATE_LIMIT < scaled <
                 EXCLUSIVE_WELD_CELL_COORDINATE_LIMIT)):
        raise ValidationError(
            "thick dielectric position exceeds the deterministic weld coordinate range")
    return math.floor(scaled)


def baked_position_in_weld_domain(position: tuple[float, float, float]) -> bool:
    return all(
        math.isfinite(coordinate) and
        -EXCLUSIVE_WELD_CELL_COORDINATE_LIMIT <
        coordinate / MINIMUM_WELD_TOLERANCE_METRES <
        EXCLUSIVE_WELD_CELL_COORDINATE_LIMIT
        for coordinate in position)


def weld_cell(position: tuple[float, float, float],
              origin: tuple[float, float, float],
              tolerance: float) -> tuple[int, int, int]:
    coordinates = []
    for axis in range(3):
        scaled = (position[axis] - origin[axis]) / tolerance
        coordinates.append(weld_cell_coordinate(scaled))
    return tuple(coordinates)


def squared_distance(left: tuple[float, float, float],
                     right: tuple[float, float, float]) -> float:
    return sum((left[axis] - right[axis]) ** 2 for axis in range(3))


def build_dielectric_welds(triangles: list[dict],
                           origin: tuple[float, float, float],
                           tolerance: float) -> list[tuple[float, float, float]]:
    occurrences = sorted(
        (triangle["positions"][corner], triangle_index, corner)
        for triangle_index, triangle in enumerate(triangles)
        for corner in range(3))
    representatives: list[tuple[float, float, float]] = []
    buckets: collections.defaultdict[tuple[int, int, int], list[int]] = (
        collections.defaultdict(list))
    tolerance_squared = tolerance * tolerance
    for position, triangle_index, corner in occurrences:
        cell = weld_cell(position, origin, tolerance)
        best = None
        best_distance_squared = math.inf
        for x in range(-1, 2):
            for y in range(-1, 2):
                for z in range(-1, 2):
                    neighbor = (cell[0] + x, cell[1] + y, cell[2] + z)
                    if any(value < -(2**63) or value > (2**63 - 1)
                           for value in neighbor):
                        continue
                    for candidate in buckets.get(neighbor, []):
                        distance_squared = squared_distance(
                            position, representatives[candidate])
                        if distance_squared > tolerance_squared:
                            continue
                        if (distance_squared < best_distance_squared or
                                (distance_squared == best_distance_squared and
                                 (best is None or candidate < best))):
                            best = candidate
                            best_distance_squared = distance_squared
        if best is None:
            best = len(representatives)
            representatives.append(position)
            buckets[cell].append(best)
        triangles[triangle_index].setdefault("welded", [None, None, None])[corner] = best
    return representatives


def component_sources(triangles: list[dict], component: list[int]) -> str:
    sources = sorted({triangles[index]["source"] for index in component})
    return ", ".join(
        f"node '{node_name}' mesh {mesh_index} primitive {primitive_index}"
        for _, node_name, mesh_index, primitive_index in sources)


def validate_material_components(path: pathlib.Path, material_name: str,
                                 triangles: list[dict]) -> int:
    if not triangles:
        return 0
    tolerance = dielectric_weld_tolerance(triangles)
    # Keep the bucket grid fixed in metre space in both validators. Signed
    # buckets plus adjacent-cell search and the explicit Euclidean predicate
    # preserve geometric welds while exposing both safe coordinate boundaries.
    representatives = build_dielectric_welds(
        triangles, (0.0, 0.0, 0.0), tolerance)
    all_edges: collections.defaultdict[tuple, list[tuple[int, bool]]] = (
        collections.defaultdict(list))
    for triangle_index, triangle in enumerate(triangles):
        for corner in range(3):
            a = triangle["welded"][corner]
            b = triangle["welded"][(corner + 1) % 3]
            if a == b:
                _, node_name, mesh_index, primitive_index = triangle["source"]
                raise ValidationError(
                    f"{path}: thick dielectric material '{material_name}' "
                    f"(node '{node_name}' mesh {mesh_index} primitive {primitive_index}) "
                    f"has a degenerate edge after the documented {tolerance:.9g} metre weld tolerance")
            ascending = a < b
            key = (a, b) if ascending else (b, a)
            all_edges[key].append((triangle_index, ascending))

    adjacency = [set() for _ in triangles]
    for references in all_edges.values():
        for left in range(len(references)):
            for right in range(left + 1, len(references)):
                a = references[left][0]
                b = references[right][0]
                if a == b:
                    continue
                adjacency[a].add(b)
                adjacency[b].add(a)

    components: list[tuple[list[int], list[int]]] = []
    visited = [False] * len(triangles)
    for first_triangle in range(len(triangles)):
        if visited[first_triangle]:
            continue
        visited[first_triangle] = True
        pending = [first_triangle]
        component = []
        while pending:
            triangle_index = pending.pop()
            component.append(triangle_index)
            for neighbor in sorted(adjacency[triangle_index], reverse=True):
                if visited[neighbor]:
                    continue
                visited[neighbor] = True
                pending.append(neighbor)
        vertex_key = sorted({vertex for triangle_index in component
                             for vertex in triangles[triangle_index]["welded"]})
        component.sort(key=lambda triangle_index: (
            tuple(triangles[triangle_index]["welded"]),
            tuple(triangles[triangle_index]["positions"]),
            triangles[triangle_index]["source"]))
        components.append((vertex_key, component))
    components.sort(key=lambda item: item[0])

    for component_number, (vertex_key, component) in enumerate(components, start=1):
        edges: collections.defaultdict[tuple, list[int]] = (
            collections.defaultdict(lambda: [0, 0]))
        for triangle_index in component:
            triangle = triangles[triangle_index]
            for corner in range(3):
                a = triangle["welded"][corner]
                b = triangle["welded"][(corner + 1) % 3]
                ascending = a < b
                key = (a, b) if ascending else (b, a)
                edges[key][0 if ascending else 1] += 1
        prefix = (f"{path}: thick dielectric material '{material_name}' "
                  f"component {component_number} "
                  f"({component_sources(triangles, component)})")
        boundary_count = sum(sum(references) == 1 for references in edges.values())
        non_manifold_count = sum(sum(references) > 2 for references in edges.values())
        inconsistent_count = sum(references != [1, 1] for references in edges.values()
                                 if sum(references) == 2)
        if boundary_count:
            raise ValidationError(
                f"{prefix} is an open thick dielectric volume with {boundary_count} boundary edge(s); "
                "close/weld the component or set thinWall=true in the audited manifest override")
        if non_manifold_count:
            raise ValidationError(
                f"{prefix} is non-manifold with {non_manifold_count} edge(s) referenced more than twice; "
                "repair the component topology or set thinWall=true only for an intentional pane")
        if inconsistent_count:
            raise ValidationError(
                f"{prefix} has inconsistent winding; each shared edge must be used once in each direction")

        origin = representatives[vertex_key[0]]
        signed_six_times_volume = 0.0
        for triangle_index in component:
            relative = [tuple(position[axis] - origin[axis] for axis in range(3))
                        for position in triangles[triangle_index]["positions"]]
            a, b, c = relative
            signed_six_times_volume += (
                a[0] * (b[1] * c[2] - b[2] * c[1])
                + a[1] * (b[2] * c[0] - b[0] * c[2])
                + a[2] * (b[0] * c[1] - b[1] * c[0]))
        if not math.isfinite(signed_six_times_volume) or signed_six_times_volume <= 1.0e-18:
            raise ValidationError(
                f"{prefix} is inward-wound after baked node transforms; "
                "reverse every triangle winding so normals face outward")
    return len(components)


def validate(path: pathlib.Path, manifest_path: pathlib.Path) -> None:
    document, binary = read_glb(path)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    metres_per_unit = manifest_metres_per_unit(manifest)
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
    triangles_by_material: collections.defaultdict[int, list[dict]] = (
        collections.defaultdict(list))
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
            positions = [transform_point_metres(
                             world_matrices[node_index], value, metres_per_unit)
                         for value in read_accessor(document, binary, position_accessor)]
            if document["accessors"][position_accessor]["componentType"] != 5126:
                raise ValidationError(
                    f"mesh {mesh_index} primitive {primitive_index}: thick dielectric POSITION must use finite FLOAT data")
            invalid_position = next(
                (position for position in positions
                 if not baked_position_in_weld_domain(position)), None)
            if invalid_position is not None:
                material_name = materials[material_index].get(
                    "name", f"material {material_index}")
                node_name = node.get("name", "<unnamed>")
                raise ValidationError(
                    f"Static GLB material '{material_name}' "
                    f"(node '{node_name}' mesh {mesh_index} primitive {primitive_index}) "
                    "contains a POSITION outside the finite deterministic dielectric "
                    "weld domain; keep every baked metre-space coordinate strictly "
                    "inside +/-2^63 minimum-tolerance cells")
            if "indices" in primitive:
                raw_indices = read_accessor(document, binary, primitive["indices"])
                indices = [int(value[0]) for value in raw_indices]
            else:
                indices = list(range(len(positions)))
            if len(indices) == 0 or len(indices) % 3:
                raise ValidationError(
                    f"mesh {mesh_index} primitive {primitive_index}: thick dielectric triangle index count is invalid")
            for triangle_offset in range(0, len(indices), 3):
                try:
                    triangle_positions = [positions[index]
                                          for index in indices[triangle_offset : triangle_offset + 3]]
                except IndexError as error:
                    raise ValidationError(
                        f"mesh {mesh_index} primitive {primitive_index}: triangle index exceeds POSITION count") from error
                if any(not math.isfinite(value) for position in triangle_positions
                       for value in position):
                    raise ValidationError(
                        f"mesh {mesh_index} primitive {primitive_index}: thick dielectric POSITION must use finite FLOAT data")
                node_name = node.get("name", "<unnamed>")
                triangles_by_material[material_index].append({
                    "positions": triangle_positions,
                    "source": (node_index, node_name, mesh_index, primitive_index),
                })

    checked_components = 0
    for material_index in sorted(thick_materials):
        material_name = materials[material_index].get("name", f"material {material_index}")
        checked_components += validate_material_components(
            path, material_name, triangles_by_material[material_index])
    print(f"Dielectric topology validation passed: {checked_components} "
          f"closed/manifold thick component(s) in {path}")


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
