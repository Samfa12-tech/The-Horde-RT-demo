"""Fit Bellwether's connected lanes to the admitted terrain without regrading it.

Blender --background --factory-startup --python-exit-code 1
  --python tools/author-bellwether-paths.py -- ROOT PARENT_TOWN NEW_OUTPUT
"""

import json
import math
from pathlib import Path
import runpy
import sys

import bpy
import numpy as np
from mathutils import Vector


RECIPE = Path(__file__).resolve()
sys.path.insert(0, str(RECIPE.parent))
from chapter_water_conformance import area, partition, triangulate

town_tools = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
mesh_tools = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
SOURCE = Path("assets/models/world/source/bellwether-paths-v1")
PARENT = "a2031a2c782d37fe9b2c3e63617c3f708bd10d9b506c025b928df69dbaf40a8f"
SURFACE_LIFT = .018


def ribbon(stations):
    centres = [Vector((x, z)) for x, z, _ in stations]
    tangents = [(end - start).normalized() for start, end in zip(centres, centres[1:])]
    normals = [Vector((-direction.y, direction.x)) for direction in tangents]
    joints = [normals[0], *((a + b) / (1 + a.dot(b)) for a, b in zip(normals, normals[1:])), normals[-1]]
    sections = []
    travelled = 0.

    def section(centre, offset, radius, distance):
        # A worn shoulder wanders independently on each side of the walked line.
        left = radius * (1 + .09 * math.sin(distance * .71) + .05 * math.cos(distance * 1.31))
        right = radius * (1 + .10 * math.cos(distance * .62) + .04 * math.sin(distance * 1.77))
        return tuple(centre + offset * left), tuple(centre - offset * right)

    for i, (start, end) in enumerate(zip(centres, centres[1:])):
        length = (end - start).length
        intervals = math.ceil(length / 1.65)
        for step in range(intervals):
            t = step / intervals
            half_width = (1 - t) * stations[i][2] + t * stations[i + 1][2]
            sections.append(section(start.lerp(end, t), joints[i].lerp(joints[i + 1], t),
                                    half_width, travelled + t * length))
        travelled += length
    sections.append(section(centres[-1], joints[-1], stations[-1][2], travelled))
    for (left, right), (next_left, next_right) in zip(sections, sections[1:]):
        for piece in ((left, right, next_right), (left, next_right, next_left)):
            yield piece if area(piece) > 0 else piece[::-1]


def bounds(points):
    return (min(p[0] for p in points), max(p[0] for p in points),
            min(p[1] for p in points), max(p[1] for p in points))


def fit_routes(terrain, routes):
    document, blocks = town_tools["read_glb"](terrain)
    primitives = [p for mesh in document["meshes"] for p in mesh["primitives"]]
    read = town_tools["read_accessor"]
    ribbons = [(i, piece, bounds(piece)) for i, route in enumerate(routes) for piece in ribbon(route["stations"])]
    extent = np.array([box for _, _, box in ribbons])
    result = [[] for _ in routes]
    for primitive in primitives:
        points = read(document, blocks, primitive["attributes"]["POSITION"]).astype(float)
        faces = read(document, blocks, primitive["indices"]).reshape(-1, 3)
        for face in points[faces]:
            projected = [tuple(vertex[[0, 2]]) for vertex in face]
            x0, x1, z0, z1 = bounds(projected)
            hits = np.flatnonzero((extent[:, 0] < x1) & (extent[:, 1] > x0) &
                                  (extent[:, 2] < z1) & (extent[:, 3] > z0))
            if not len(hits):
                continue
            a, b, c = face
            normal = np.cross(b - a, c - a)
            # Leave any steep banks on the verge to the original terrain.
            # The native walkable intake caps rise/run at .65.
            if normal[1] <= 0 or np.hypot(normal[0], normal[2]) > .64 * normal[1]:
                continue
            remainder = [projected]
            for index in hits:
                route, constraint, _ = ribbons[index]
                unassigned = []
                for polygon in remainder:
                    pieces, assigned = partition(polygon, constraint)
                    if assigned:
                        unassigned.extend(pieces[:-1])
                        for triangle in triangulate(pieces[-1]):
                            # Upward-facing in Horde's X/Y/Z coordinates.
                            result[route].append(tuple((x, a[1] + SURFACE_LIFT -
                                (normal[0] * (x - a[0]) + normal[2] * (z - a[2])) / normal[1], z)
                                for x, z in triangle[::-1]))
                    else:
                        unassigned.append(polygon)
                remainder = unassigned
                if not remainder:
                    break
    return result


def make_part(name, triangles, material):
    vertices = {}
    faces = []
    for triangle in triangles:
        if abs(area([(x, z) for x, _, z in triangle])) < .00003:
            continue
        points = [tuple(round(value, 6) for value in point) for point in triangle]
        faces.append(tuple(vertices.setdefault(point, len(vertices)) for point in points))
    if not faces:
        raise ValueError(f"No terrain-fit path for {name}")
    return mesh_tools["mesh"](name, list(vertices), faces, material)


def write_planar_tangents(patch):
    doc, blocks = town_tools["read_glb"](patch)
    primitive, = doc["meshes"][0]["primitives"]
    attributes = primitive["attributes"]
    normals = town_tools["read_accessor"](doc, blocks, attributes["NORMAL"])
    # The shared walkable UV projects U along world +X. Derive the tangent
    # directly because clipped slivers can give the exporter a zero vector.
    tangents = np.zeros((len(normals), 4), dtype="<f4")
    tangents[:, 0] = 1.
    tangents[:, 1] = -normals[:, 0] / normals[:, 1]
    tangents[:, :3] /= np.linalg.norm(tangents[:, :3], axis=1, keepdims=True)
    tangents[:, 3] = 1.
    town_tools["write_accessor"](doc, blocks, attributes["TANGENT"], tangents)
    town_tools["save_glb"](patch, doc, blocks)


def main(root, previous, output):
    terrain = root / town_tools["TERRAIN"]
    digest = town_tools["sha"]
    if output.exists() or digest(previous) != PARENT or digest(terrain) != town_tools["TERRAIN_HASH"]:
        raise ValueError("Use a new output directory and the exact town and terrain parents.")
    layout = root / SOURCE / "layout.json"
    routes = json.loads(layout.read_bytes())["routes"]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    material = town_tools["material"]("HordeTownStone", (.30, .27, .23), .9)
    parts = [make_part(route["name"], faces, material)
             for route, faces in zip(routes, fit_routes(terrain, routes), strict=True)]
    counts = {part.name: len(part.data.polygons) for part in parts}
    output.mkdir(parents=True)
    master = output / "bellwether-paths.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    mesh_tools["join_parts"](parts, "BellwetherFootpaths")
    patch = output / "paths.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    write_planar_tangents(patch)
    town = output / "bellwether-tavern-v1.glb"
    triangles, vertices, primitives = town_tools["join_town"](previous, patch, town, node_name="BellwetherWalkable")
    if triangles != sum(counts.values()) or not 0 < triangles <= 4200 or primitives != 5 or vertices > 65000:
        raise ValueError(f"Paths exceed the town budget: {triangles} new triangles, {vertices} vertices")
    doc, _ = town_tools["read_glb"](town)
    indices = sum(doc["accessors"][p["indices"]]["count"] for mesh in doc["meshes"] for p in mesh["primitives"])
    manifest = json.loads((previous.parent / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    town_tools["save_json"](output / "asset.manifest.json", manifest)
    header = output / "geometry.generated.h"
    header.write_text(
        "#pragma once\n#include <cstddef>\n\n// Generated by tools/author-bellwether-paths.py.\n"
        "namespace horde::gameplay::bellwether {\n"
        f"inline constexpr std::size_t kBellwetherPathGroundTriangles={triangles};\n"
        "} // namespace horde::gameplay::bellwether\n", encoding="utf-8", newline="\n")
    town_tools["save_json"](output / "authoring-receipt.json", dict(
        feature="Mapped Bellwether footpaths and lane junctions", originalTownSha256=PARENT,
        terrainSha256=digest(terrain), runtimeTownSha256=digest(town),
        routeTriangles=counts, addedGroundTriangles=triangles, surfaceLiftMetres=SURFACE_LIFT,
        townVertices=vertices, townTriangles=indices // 3, layoutSha256=digest(layout),
        recipeSha256=digest(RECIPE), blender=bpy.app.version_string,
        files={item.name: digest(item) for item in (master, patch, town, header, output / "asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(argument).resolve() for argument in sys.argv[sys.argv.index("--") + 1:]))
