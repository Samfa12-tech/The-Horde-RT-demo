"""Build the mill footway and dry crossing into the existing town GLB.

Blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-mill-crossing.py -- ROOT PREVIOUS_TOWN_GLB NEW_OUTPUT
"""
import json
from pathlib import Path
import runpy
import sys

import bpy
import numpy as np


HERE = Path(__file__).resolve()
WORKS_RECIPE = HERE.with_name("author-bellwether-mill-works.py")
work = runpy.run_path(str(WORKS_RECIPE))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PREVIOUS_TOWN = "605612b5d65c01d7c20b37a979ee159c0b0aefe4852e13b9a486f1e55dba0c66"
X_WEST = 134.58
X_EAST = 136.02
BRIDGE_NORTH = 92.60
BRIDGE_SOUTH = 95.40
BRIDGE_END = X_EAST + 7.0
TURNOUT_END = BRIDGE_END + 1.45
TURNOUT_HEIGHT = -18.50

# Z, west edge and east edge heights. The first two metres blend the
# south causeway's cross slope into a level section across the footway.
FOOTWAY = (
    (122.28, -21.754, -22.615), (121.55, -21.72, -22.575),
    (120.45, -21.66, -22.31), (119.00, -21.48, -21.83),
    (116.00, -21.16, -21.19), (114.00, -20.77, -20.77),
    (112.10, -20.03, -20.03), (111.15, -19.57, -19.57),
    (110.12, -19.30, -19.30), (108.65, -19.05, -19.05),
    (106.70, -18.68, -18.68), (104.10, -18.00, -18.00),
    (103.00, -17.98, -17.98), (101.00, -17.47, -17.47),
    (98.80, -16.46, -16.46), (96.20, -15.08, -15.08),
    (BRIDGE_SOUTH, -14.65, -14.65), (BRIDGE_NORTH, -14.65, -14.65),
)

WALK_STATIONS = ((135.30, 122.55), (135.30, 114.00), (135.30, 94.00),
                 (138.80, 94.00), (143.72, 94.00))


def mesh(name, points, faces, finish):
    data = bpy.data.meshes.new(name)
    data.from_pydata([(x, -z, y) for x, y, z in points], [], faces)
    data.materials.append(finish)
    data.update()
    uv = data.uv_layers.new(name="Metre coordinates")
    for face in data.polygons:
        axis = max(range(3), key=lambda value: abs(face.normal[value]))
        u, v = ((1, 2), (0, 2), (0, 1))[axis]
        for loop in face.loop_indices:
            point = data.vertices[data.loops[loop].vertex_index].co
            uv.data[loop].uv = (.5 * point[u], .5 * point[v])
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    return obj


def rail(name, edge, timber, omit_posts=()):
    from mathutils import Vector
    parts = []
    for index, (a, b) in enumerate(zip(edge, edge[1:])):
        direction = Vector((b[0] - a[0], b[2] - a[2])).normalized()
        side = Vector((-direction[1], direction[0])) * .065
        beam = [(x + offset * side[0], y + height, z + offset * side[1])
                for height in (.78, .92) for (x, y, z), offset in
                ((a, -1), (a, 1), (b, 1), (b, -1))]
        parts.append(mesh(f"{name} rail {index + 1}", beam,
                          [(3, 2, 1, 0), (4, 5, 6, 7), (0, 1, 5, 4),
                           (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)], timber))
        if index == 0:
            posts = (a, b)
        else:
            posts = (b,)
        for x, y, z in posts:
            if (x, z) in omit_posts:
                continue
            parts.append(work["block"](f"{name} post", x-.088, x+.088, z-.088, z+.088,
                                        y-.06, y+.99, timber, .016))
    return parts


def build_meshes(ground):
    stone = work["material"]("HordeTownStone", (.22, .20, .16), 1.)
    timber = work["material"]("HordeTavernTimber", (.25, .115, .065), .84)
    deck = []
    solid = []
    upper = [(x, y, z) for z, west, east in FOOTWAY for x, y in ((X_WEST, west), (X_EAST, east))]
    walk_faces = [(i, i+1, i+3, i+2) for i in range(0, len(upper)-2, 2)]
    deck.append(mesh("Stone path from the river landing", upper, walk_faces, stone))
    lower = [(x, min(y-.16, ground(x, z)-.22), z) for x,y,z in upper]
    count = len(upper)
    walls = []
    for i in range(0, count-2, 2):
        walls.append((i, i+2, i+2+count, i+count))
        if FOOTWAY[i//2][0] > BRIDGE_SOUTH:
            walls.append((i+3, i+1, i+1+count, i+3+count))
    solid.append(mesh("Mill footway revetment", upper+lower, walls, stone))

    rail_stations = (1, 4, 6, 8, 10, 12, 14, 16)
    solid += rail("Race-side oak fence", [upper[i*2+1] for i in rail_stations], timber)

    bridge_rows = ((X_EAST, -14.65), (BRIDGE_END, TURNOUT_HEIGHT), (TURNOUT_END, TURNOUT_HEIGHT))
    points = [(x, y, z) for x, y in bridge_rows for z in (BRIDGE_NORTH, BRIDGE_SOUTH)]
    strip_faces = [(i, i+1, i+3, i+2) for i in (0, 2)]
    deck.append(mesh("Inclined stone crossing and service turnout", points, strip_faces, stone))
    bottom = [(x, y-.24, z) for x, y, z in points]
    skirts = [(i, i+2, i+8, i+6) for i in (0, 2)] + [(i+3, i+1, i+7, i+9) for i in (0, 2)]
    soffit = [tuple(i+6 for i in reversed(face)) for face in strip_faces]
    solid.append(mesh("Crossing oak fascia", points+bottom, skirts+[(4, 5, 11, 10)], timber))
    solid.append(mesh("Crossing masonry soffit", points+bottom, soffit, stone))

    def bridge_height(x):
        t = min(1, (x-X_EAST)/(BRIDGE_END-X_EAST))
        return (1-t)*bridge_rows[0][1]+t*TURNOUT_HEIGHT

    rail_x = (X_EAST, 137.75, 139.50, 141.25, BRIDGE_END, TURNOUT_END)
    north = [(X_WEST, -14.65, BRIDGE_NORTH)]
    north += [(x, bridge_height(x), BRIDGE_NORTH) for x in rail_x]
    solid += rail("North crossing fence", north, timber)
    south = [(x, bridge_height(x), BRIDGE_SOUTH) for x in rail_x]
    solid += rail("South crossing fence", south, timber, ((X_EAST, BRIDGE_SOUTH),))
    solid += rail("Turnout end fence", (north[-1], south[-1]), timber,
                  ((TURNOUT_END, BRIDGE_NORTH), (TURNOUT_END, BRIDGE_SOUTH)))

    # The opening between the abutments spans the entire cut race.
    for name, x0, x1 in (("West abutment", 136.05, 136.68),
                         ("East abutment", 140.55, 141.52),
                         ("Turnout pier", 143.11, 144.24)):
        for z0, z1 in ((92.76, 93.36), (94.64, 95.24)):
            bed = min(ground(x, z) for x in (x0, x1) for z in (z0, z1))-.22
            top = bridge_height(x1)-.18
            solid.append(work["block"](name, x0, x1, z0, z1, bed, top, stone, .055))
    return deck, solid


def join_parts(parts, name):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = name
    joined.modifiers.new("Export triangles", "TRIANGULATE")
    return joined


def open_causeway_wall(town, blocks, target):
    # The pinned causeway has two sloping wall panels through the junction.
    # Keep their original plane and UVs outside the footway opening.
    attributes = {name: work["read_accessor"](town, blocks, index)
                  for name, index in target["attributes"].items()}
    positions = attributes["POSITION"]
    indices = work["read_accessor"](town, blocks, target["indices"])
    faces = indices.reshape((-1, 3))
    grown = {name: [] for name in attributes}
    rebuilt = []
    affected = 0

    def cut(a, b, limit):
        ratio = (limit-positions[a, 0]) / (positions[b, 0]-positions[a, 0])
        new_index = len(positions) + len(grown["POSITION"])
        for name, data in attributes.items():
            grown[name].append((data[a]+ratio*(data[b]-data[a])).astype(data.dtype))
        return new_index

    for face in faces:
        xyz = positions[face]
        if (np.max(np.abs(xyz[:, 2]-121.60)) > .0001 or
            xyz[:, 0].min() >= X_EAST or xyz[:, 0].max() <= X_WEST):
            rebuilt.extend(face)
            continue
        affected += 1
        for limit, side in ((X_WEST, -1), (X_EAST, 1)):
            keep = side*(xyz[:, 0]-limit) >= 0
            polygon = []
            for i in range(3):
                j = (i+1)%3
                if keep[i]:
                    polygon.append(face[i])
                if keep[i] != keep[j]:
                    polygon.append(cut(face[i], face[j], limit))
            for i in range(1, len(polygon)-1):
                rebuilt.extend((polygon[0], polygon[i], polygon[i+1]))
    if affected != 4:
        raise ValueError("The original causeway wall no longer matches the surveyed four faces")
    for name, accessor in target["attributes"].items():
        work["write_accessor"](town, blocks, accessor, np.concatenate((attributes[name], grown[name])))
    work["write_accessor"](town, blocks, target["indices"],
                           np.array(rebuilt, dtype=indices.dtype).reshape((-1, 1)))
    return len(rebuilt)//3-len(faces)


def append_town(original, crossing, output):
    town, blocks = work["read_glb"](original)
    addition, added_blocks = work["read_glb"](crossing)
    support, = (n for n in town["nodes"] if n["name"] == "BellwetherSolid")
    masonry, = (p for p in town["meshes"][support["mesh"]]["primitives"]
                 if town["materials"][p["material"]]["name"] == "HordeTownStone")
    counts = {"BellwetherSolid": open_causeway_wall(town, blocks, masonry), "BellwetherWalkable": 0}
    for node in addition["nodes"]:
        role = node["name"]
        owner, = (n for n in town["nodes"] if n["name"] == role)
        for patch in addition["meshes"][node["mesh"]]["primitives"]:
            name = addition["materials"][patch["material"]]["name"]
            target, = (p for p in town["meshes"][owner["mesh"]]["primitives"]
                       if town["materials"][p["material"]]["name"] == name)
            old_vertex_count = town["accessors"][target["attributes"]["POSITION"]]["count"]
            for semantic, accessor in target["attributes"].items():
                old = work["read_accessor"](town, blocks, accessor)
                new = work["read_accessor"](addition, added_blocks, patch["attributes"][semantic])
                work["write_accessor"](town, blocks, accessor, np.concatenate((old, new)))
            indices = work["read_accessor"](town, blocks, target["indices"])
            append = work["read_accessor"](addition, added_blocks, patch["indices"])
            if old_vertex_count + int(append.max()) > np.iinfo(indices.dtype).max:
                raise ValueError("Crossing exceeds the existing town index range")
            work["write_accessor"](town, blocks, target["indices"],
                np.concatenate((indices, append.astype(indices.dtype)+old_vertex_count)))
            counts[role] += len(append)//3
    work["save_glb"](output, town, blocks)
    vertices = sum(town["accessors"][p["attributes"]["POSITION"]]["count"]
                   for mesh_data in town["meshes"] for p in mesh_data["primitives"])
    return counts, vertices


def main(root, original, output):
    if output.exists() or work["sha"](original) != PREVIOUS_TOWN or \
       work["sha"](root/work["TERRAIN"]) != work["TERRAIN_HASH"]:
        raise ValueError("Choose a new directory, the pre-crossing town GLB and the pinned wheel-bay terrain")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](root/work["TERRAIN"])
    bpy.data.orphans_purge(do_recursive=True)
    deck, solid = build_meshes(ground)
    output.mkdir(parents=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output/"mill-crossing.blend"), compress=True)
    walkable = join_parts(deck, "BellwetherWalkable")
    structure = join_parts(solid, "BellwetherSolid")
    walkable.select_set(True)
    structure.select_set(True)
    crossing = output/"mill-crossing.glb"
    bpy.ops.export_scene.gltf(filepath=str(crossing), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output/"bellwether-tavern-v1.glb"
    counts, vertices = append_town(original, crossing, town)
    manifest = json.loads((root/TOWN/"asset.manifest.json").read_bytes())
    solid_count = counts["BellwetherSolid"]
    walkable_count = counts["BellwetherWalkable"]
    if solid_count > 1_200 or walkable_count != 38 or vertices > 12_500:
        raise ValueError(f"Crossing exceeds its geometry budget: {counts}, {vertices} town vertices")
    triangles = 4556+solid_count+walkable_count
    manifest["budgets"].update(maxVertices=vertices, maxIndices=triangles*3)
    manifest["lods"][0]["maxTriangles"] = triangles
    work["save_json"](output/"asset.manifest.json", manifest)
    stations = ", ".join(f"{{{x:.2f}f,{z:.2f}f}}" for x,z in WALK_STATIONS)
    (output/"geometry.generated.h").write_text(
        "#pragma once\n#include <array>\n#include <cstddef>\n\n"
        "// Generated by tools/author-bellwether-mill-crossing.py.\n"
        "namespace horde::gameplay::bellwether {\n"
        f"inline constexpr std::size_t kMillCrossingSolidTriangles={solid_count};\n"
        f"inline constexpr std::size_t kMillCrossingGroundTriangles={walkable_count};\n"
        f"inline constexpr std::array<std::array<float,2>,{len(WALK_STATIONS)}> kMillCrossingStations"
        f"{{{{{stations}}}}};\n"
        "} // namespace horde::gameplay::bellwether\n", encoding="utf-8", newline="\n")
    artifacts = (town.name, crossing.name, "mill-crossing.blend", "asset.manifest.json", "geometry.generated.h")
    work["save_json"](output/"authoring-receipt.json", dict(
        originalTownSha256=PREVIOUS_TOWN, terrainSha256=work["TERRAIN_HASH"], townVertices=vertices,
        solidTriangles=solid_count, groundTriangles=walkable_count, stations=WALK_STATIONS,
        recipeSha256=work["sha"](HERE), millWorksRecipeSha256=work["sha"](WORKS_RECIPE),
        outputs={name:dict(bytes=(output/name).stat().st_size,sha256=work["sha"](output/name))
                 for name in artifacts}))
    print(f"Crossing adds {solid_count} solid and {walkable_count} walkable triangles to the town.")


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
