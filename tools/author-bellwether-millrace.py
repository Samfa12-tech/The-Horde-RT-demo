"""Carve the mapped Bellwether millrace into the pinned chapter terrain GLB.

The source mesh supplies the exact outside surface, including its material images.
This derivative partitions only intersecting top faces and their seam neighbours.
"""
import argparse
from collections import Counter
from decimal import Decimal
from itertools import product
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

import numpy as np

from chapter_water_conformance import EPS, area, cross, on_segment, partition, triangulate


LAYOUT_PATH = "assets/models/world/source/bellwether-waterwheel-v1/layout.json"
TERRAIN_NAME = "chapter-terrain-lod0.runtime.glb"
AUTHORING_RECEIPT = "authoring-receipt.json"
POINT_TOLERANCE = 1e-6


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_glb(data):
    magic, version, length, json_length, chunk = struct.unpack_from("<4sIII4s", data)
    if (magic, version, length, chunk) != (b"glTF", 2, len(data), b"JSON"):
        raise ValueError("Expected chapter terrain GLB 2.0")
    end_json = 20 + json_length
    bin_length, bin_type = struct.unpack_from("<I4s", data, end_json)
    if bin_type != b"BIN\0" or end_json + 8 + bin_length != len(data):
        raise ValueError("Expected the terrain's single binary chunk")
    return json.loads(data[20:end_json]), data[end_json + 8:]


def accessor(document, binary, index, dtype, width):
    record = document["accessors"][index]
    view = document["bufferViews"][record["bufferView"]]
    if "byteStride" in view:
        raise ValueError("Expected dense terrain attributes")
    array = np.frombuffer(binary, dtype, record["count"] * width,
                          view["byteOffset"] + record.get("byteOffset", 0))
    return array.reshape(-1, width)


def write_glb(document, binary, updates):
    buffer = bytearray()
    for index, view in enumerate(document["bufferViews"]):
        if index in updates:
            data = updates[index]
        else:
            start = view["byteOffset"]
            data = binary[start:start + view["byteLength"]]
        view["byteOffset"], view["byteLength"] = len(buffer), len(data)
        buffer.extend(data)
        buffer.extend(b"\0" * (-len(buffer) % 4))
    document["buffers"][0]["byteLength"] = len(buffer)
    metadata = json.dumps(document, separators=(",", ":"), ensure_ascii=True).encode()
    metadata += b" " * (-len(metadata) % 4)
    return (struct.pack("<4sIII4s", b"glTF", 2, 28 + len(metadata) + len(buffer), len(metadata), b"JSON")
            + metadata + struct.pack("<I4s", len(buffer), b"BIN\0") + buffer)


def channel_sections(stations):
    """Sections share the native ribbon miter and the surveyed centreline."""
    stations = np.asarray(stations, dtype=float)
    positions = stations[:, [0, 2]]
    delta = np.diff(positions, axis=0)
    normal = delta[:, [1, 0]] * [-1, 1]
    normal /= np.linalg.norm(normal, axis=1)[:, None]
    before = normal[np.maximum(0, np.arange(len(stations)) - 1)]
    after = normal[np.minimum(len(normal) - 1, np.arange(len(stations)))]
    miter = before + after
    miter /= (miter * before).sum(axis=1)[:, None]
    return list(zip(positions, stations[:, 1], miter))


def channel_patches(layout):
    sections = channel_sections(layout["headrace"] + layout["tailrace"])
    # The ends overlap the main river. Keep the admitted bed there and taper
    # the lining's height back to the established shoreline.
    def ramp(a, b, t):
        return tuple((1 - t) * start + t * finish for start, finish in zip(a, b))
    sections.insert(1, ramp(sections[0], sections[1], .18))
    sections.insert(-1, ramp(sections[-2], sections[-1], .78))

    # Keep the surveyed water grade. Open a deeper chamber below the surface
    # so the upstream end of the wheel clears the bed throughout a rotation.
    _, axle_y, axle_z = layout["wheel"]["axleCentre"]
    swept_radius = layout["wheel"]["radius"]
    clearance = .18
    pit_start, pit_end = axle_z - swept_radius - clearance, axle_z + swept_radius + clearance
    pit_bottom = axle_y - swept_radius - clearance
    for z in (pit_start-.80, pit_start):
        for index, (first, last) in enumerate(zip(sections, sections[1:])):
            z0, z1 = first[0][1], last[0][1]
            if z0 < z < z1:
                sections.insert(index+1, ramp(first, last, (z-z0)/(z1-z0)))
                break
        else:
            raise ValueError(f"Wheel pit section at Z={z} is outside the surveyed race")
    half = layout["channelWidth"] / 2
    positive_offsets = (half-.19, half-.03, half+.083, half+.67)
    offsets = tuple(-width for width in reversed(positive_offsets)) + positive_offsets
    rows = []
    for index, (centre, water_y, miter) in enumerate(sections):
        bed_y = water_y - .40
        if pit_start-EPS <= centre[1] <= pit_end+EPS:
            bed_y = min(bed_y, pit_bottom)
        bank_y = water_y + .24
        shore_y = water_y + .045
        heights = (bank_y, bank_y, shore_y, bed_y, bed_y, shore_y, bank_y, bank_y)
        rows.append([(tuple(centre + miter * offset), height,
                      float(index in (0, len(sections)-1) or col in (0, len(offsets)-1)))
                     for col, (offset, height) in enumerate(zip(offsets, heights))])
    patches = []
    for first, last in zip(rows, rows[1:]):
        for col in range(len(offsets)-1):
            # Same diagonal in every strip, with an explicit bottom and crest.
            for vertices in ((first[col], last[col+1], first[col+1]),
                             (first[col], last[col], last[col+1])):
                if area([vertex[0] for vertex in vertices]) < 0:
                    vertices = vertices[::-1]
                patches.append(vertices)
    return patches


def bounds(points):
    x, z = zip(*points)
    return min(x), max(x), min(z), max(z)


def overlaps(a, b):
    return a[0] <= b[1]+EPS and a[1] >= b[0]-EPS and a[2] <= b[3]+EPS and a[3] >= b[2]-EPS


def within_segment(point, a, b):
    return all(min(x, y)-EPS <= p <= max(x, y)+EPS for p, x, y in zip(point, a, b)) and on_segment(point, a, b)


def barycentric(point, a, b, c):
    divisor = cross(a, b, c)
    u, v = cross(point, b, c) / divisor, cross(a, point, c) / divisor
    return np.array((u, v, 1-u-v))


def edges(face):
    return [tuple(sorted((a, b))) for a, b in zip(face, face[1:] + face[:1])]


def split_surface(positions, faces, attributes, patches):
    # Promote projected coordinates before clipping. Float32 shoelace terms
    # lose narrow seam polygons at this large world offset.
    points = [tuple(point) for point in positions[:, [0, 2]].tolist()]
    regions = [tuple(entry[0] for entry in patch) for patch in patches]
    region_bounds = [bounds(region) for region in regions]
    outline = bounds([point for region in regions for point in region])
    # Spatial limits protect the B05 east wall and the practice landing.
    if outline[0] <= 134.3 or outline[3] >= 121.0:
        raise ValueError("Millrace grading overlaps a protected town feature")
    fields = [np.array([entry[1:] for entry in patch]) for patch in patches]
    subdivisions = {}
    splits = {}
    for index, face in enumerate(faces):
        footprint = [points[i] for i in face]
        footprint_bounds = bounds(footprint)
        if not overlaps(footprint_bounds, outline) or area(footprint) >= -EPS:
            continue
        pieces = [footprint[::-1]]
        taken = []
        for i, (region, box) in enumerate(zip(regions, region_bounds)):
            if not overlaps(footprint_bounds, box):
                continue
            remainder = []
            for piece in pieces:
                if not overlaps(bounds(piece), box):
                    remainder.append(piece)
                    continue
                parts, inside = partition(piece, region)
                if inside:
                    taken.append((parts[-1], i))
                    parts = parts[:-1]
                remainder.extend(parts)
            pieces = remainder
        if not taken:
            continue
        subdivisions[index] = taken + [(piece, None) for piece in pieces]
        generated = {point for piece, _ in subdivisions[index] for point in piece}
        for edge in edges(face):
            a, b = (points[i] for i in edge)
            new = {p for p in generated if min(math.dist(p, a), math.dist(p, b)) > POINT_TOLERANCE
                   and within_segment(p, a, b)}
            if new:
                splits.setdefault(edge, set()).update(new)

    output = {key: list(values.copy()) for key, values in attributes.items()}
    new_positions = output["POSITION"]
    normals = output["NORMAL"]
    tangents = output["TANGENT"]
    result = []
    seam = {}
    inserted = []
    graded = set()
    old_boundary, new_boundary = Counter(), Counter()

    def register(point, source, source_points, patch):
        weights = barycentric(point, *source_points)
        geometry = weights @ positions[source]
        height = geometry[1]
        if patch is not None:
            grade, blend = barycentric(point, *regions[patch]) @ fields[patch]
            height += (grade - height) * (1 - np.clip(blend, 0, 1))
        touched = abs(height - geometry[1]) > POINT_TOLERANCE
        geometry[:] = (point[0], height, point[1])
        for original, xy in zip(source, source_points):
            if math.dist(point, xy) <= POINT_TOLERANCE:
                if original in graded and abs(new_positions[original][1] - height) > POINT_TOLERANCE:
                    raise ValueError("Original vertex has conflicting grades")
                if touched:
                    new_positions[original] = np.array((positions[original][0], height, positions[original][2]))
                    graded.add(original)
                return int(original)
        key = tuple(math.floor(component / POINT_TOLERANCE) for component in point)
        neighbours = (index for dx, dz in product((-1, 0, 1), repeat=2)
                      for index in seam.get((key[0]+dx, key[1]+dz), ()))
        index = next((i for i in neighbours if np.linalg.norm(geometry[[0,2]] - new_positions[i][[0,2]])
                      <= POINT_TOLERANCE), None)
        if index is None:
            index = len(new_positions)
            seam.setdefault(key, []).append(index)
            inserted.append(index)
            for semantic, values in attributes.items():
                output[semantic].append(geometry if semantic == "POSITION" else weights @ values[source])
        elif np.linalg.norm(new_positions[index] - geometry) > 3 * POINT_TOLERANCE:
            raise ValueError("Generated seam has conflicting heights")
        if touched:
            graded.add(index)
        return index

    def count_boundary(counter, face):
        for edge in edges(face):
            counter[edge] += 1

    for index, face in enumerate(faces):
        source = list(face)
        footprint = [points[i] for i in face]
        adjacent = set().union(*(splits.get(edge, ()) for edge in edges(face)))
        if index not in subdivisions and not adjacent:
            result.append(face)
            continue
        pieces = subdivisions.get(index, [(footprint[::-1], None)])
        local = adjacent | {point for piece, _ in pieces for point in piece}
        count_boundary(old_boundary, face)
        for piece, patch in pieces:
            expanded = []
            for a, b in zip(piece, piece[1:] + piece[:1]):
                interior = [point for point in local
                            if min(math.dist(point, a), math.dist(point, b)) > POINT_TOLERANCE
                            and within_segment(point, a, b)]
                interior.sort(key=lambda point: math.dist(point, a))
                expanded.extend([a, *interior])
            try:
                triangulated = triangulate(expanded)
            except ValueError as error:
                raise ValueError(f"Face {index}, patch {patch}, piece {piece}, expanded {expanded}") from error
            for a, b, c in triangulated:
                child = tuple(register(point, source, footprint, patch) for point in (a, c, b))
                if len(set(child)) != 3:
                    raise ValueError(f"Degenerate face in terrain triangle {index}")
                result.append(child)
                count_boundary(new_boundary, child)
    if ({edge for edge, count in old_boundary.items() if count == 1}
            != {edge for edge, count in new_boundary.items() if count == 1}):
        difference = ({edge for edge, count in old_boundary.items() if count == 1}
                      ^ {edge for edge, count in new_boundary.items() if count == 1})
        examples = [(new_positions[a].tolist(), new_positions[b].tolist()) for a,b in sorted(difference)[:3]]
        raise ValueError(f"The grade would leave {len(difference)} unmatched edges: {examples}")

    blended = {index: np.zeros(3) for index in graded}
    for face in result:
        if not any(index in graded for index in face):
            continue
        a, b, c = (new_positions[i].astype(float) for i in face)
        normal = np.cross(b-a, c-a)
        for index in face:
            if index in graded:
                blended[index] += normal
    for index in graded | set(inserted):
        normal = blended.get(index, normals[index])
        normal /= np.linalg.norm(normal)
        tangent = tangents[index][:3] - normal * np.dot(normal, tangents[index][:3])
        normals[index] = normal
        tangents[index] = np.append(tangent / np.linalg.norm(tangent), tangents[index][3])
    return output, np.array(result, dtype="<u2"), len(subdivisions), outline


def author(source_path, layout_path, manifest_path, output):
    source_data = source_path.read_bytes()
    layout_data = layout_path.read_bytes()
    layout = json.loads(layout_data)
    if digest(source_data) != layout["terrainSha256"]:
        raise ValueError("Use the exact chapter terrain pinned by the wheel survey")
    manifest = json.loads(manifest_path.read_bytes())
    document, binary = read_glb(source_data)
    old = copy.deepcopy(document)
    first, terrain = document["meshes"][0]["primitives"]
    attributes = {key: accessor(document, binary, index, "<f4",
                                 {"POSITION": 3, "NORMAL": 3, "TEXCOORD_0": 2, "TANGENT": 4}[key])
                  for key, index in terrain["attributes"].items()}
    indices = accessor(document, binary, terrain["indices"], "<u2", 1).reshape(-1, 3)
    vertices, faces, changed, outline = split_surface(
        attributes["POSITION"], [tuple(face) for face in indices], attributes, channel_patches(layout))
    triangle_count = len(faces) + document["accessors"][first["indices"]]["count"] // 3
    vertex_count = len(vertices["POSITION"]) + document["accessors"][first["attributes"]["POSITION"]]["count"]
    if vertex_count > manifest["budgets"]["maxVertices"] or triangle_count > manifest["lods"][0]["maxTriangles"]:
        raise ValueError(f"Millrace exceeds current terrain budget: {triangle_count} triangles, {vertex_count} vertices")
    data = {terrain["indices"]: faces.reshape(-1, 1)}
    data.update({terrain["attributes"][key]: np.asarray(values, dtype="<f4") for key, values in vertices.items()})
    quantized = data[terrain["attributes"]["POSITION"]]
    new_faces = faces[np.any(faces >= len(attributes["POSITION"]), axis=1)]
    a, b, c = quantized[new_faces].transpose(1, 0, 2).astype(float)
    min_up_area = np.cross(b-a, c-a)[:, 1].min()
    if min_up_area <= 0.:
        raise ValueError("A new triangle folds or collapses after GLB float32 quantization")
    updates = {}
    for index, values in data.items():
        record = document["accessors"][index]
        record["count"] = len(values)
        if "min" in record:
            record["min"], record["max"] = values.min(axis=0).tolist(), values.max(axis=0).tolist()
        updates[record["bufferView"]] = values.tobytes()
    document["nodes"][0]["name"] = "chapter-terrain-bellwether-millrace"
    imported = write_glb(document, binary, updates)
    new, payload = read_glb(imported)
    for index in range(len(old["bufferViews"])):
        if index in updates:
            continue
        original, derived = (item["bufferViews"][index] for item in (old, new))
        size = original["byteLength"]
        a, b = original["byteOffset"], derived["byteOffset"]
        if binary[a:a+size] != payload[b:b+size]:
            raise ValueError("A protected attribute or material image changed")
    receipt = dict(terrainSourceSha256=digest(source_data), layoutSha256=digest(layout_data),
                   runtimeSha256=digest(imported), runtimeBytes=len(imported),
                   triangles=triangle_count, vertices=vertex_count,
                   trianglesIntersectingRaceBeforeSubdivision=changed,
                   patchBoundsXZ=outline, minimumNewProjectedDoubleArea=float(min_up_area))
    output.mkdir(parents=True)
    (output / TERRAIN_NAME).write_bytes(imported)
    (output / AUTHORING_RECEIPT).write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8", newline="\n")
    print(json.dumps(receipt))


def native_header(layout_path):
    def literal(value):
        value = format(Decimal(value), "f")
        return value + ("f" if "." in value else ".0f")
    layout = json.loads(layout_path.read_bytes(), parse_float=Decimal)
    stations = layout["headrace"] + layout["tailrace"]
    rows = ["    {" + ",".join(map(literal, point)) + "}" for point in stations]
    return "\n".join((
        "#pragma once",
        "#include <array>",
        "",
        "namespace horde::scene {",
        f"// Generated from {LAYOUT_PATH}",
        "// Generated by tools/author-bellwether-millrace.py --root . --emit-native-header.",
        f"inline constexpr float kBellwetherMillraceChannelWidth={literal(layout['channelWidth'])};",
        "// Engine XYZ metres, ordered headrace then tailrace. The 5->6 segment is the",
        "// authored short, steep drop at the waterwheel.",
        f"inline constexpr std::array<std::array<float,3>,{len(stations)}> kBellwetherMillraceStations{{{{",
        ",\n".join(rows),
        "}};",
        "} // namespace horde::scene",
        "",
    ))


if __name__ == "__main__":
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("--root", required=True, type=Path)
    args.add_argument("--input", type=Path)
    args.add_argument("--output", type=Path)
    args.add_argument("--emit-native-header", action="store_true")
    options = args.parse_args()
    layout_path = options.root / LAYOUT_PATH
    if options.emit_native_header:
        if options.input or options.output:
            args.error("--emit-native-header uses only --root")
        sys.stdout.buffer.write(native_header(layout_path).encode())
    else:
        if not options.input or not options.output:
            args.error("--input and --output are required to author terrain")
        author(options.input, layout_path,
               options.root / "assets/models/world/runtime/chapter-terrain-v1/asset.manifest.json",
               options.output)
