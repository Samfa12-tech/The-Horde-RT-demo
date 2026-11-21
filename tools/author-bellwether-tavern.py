"""Author the first playable B01 interior from the preserved Bellwether master.

blender --background --python-exit-code 1 --python tools/author-bellwether-tavern.py -- ROOT
The editable and runtime output directories must both be new.
"""
import hashlib
import json
from pathlib import Path
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree


ROOT = Path(sys.argv[sys.argv.index("--") + 1]).resolve()
WORLD = ROOT / "assets/models/world"
MASTER = WORLD / "source/bellwether-shell-v4/bellwether-shell-v4.blend"
TERRAIN = WORLD / "runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb"
OUTPUT = WORLD / "source/bellwether-tavern-v1"
RUNTIME = WORLD / "runtime/bellwether-tavern-v1"
if OUTPUT.exists() or RUNTIME.exists():
    raise RuntimeError("Preserve the previous source and runtime outputs.")

parent = json.loads(MASTER.with_name("authoring-receipt.json").read_text())
tavern = next(building for building in parent["buildings"] if building["id"] == "B01")
bpy.ops.wm.open_mainfile(filepath=str(MASTER))
bpy.context.preferences.filepaths.save_version = 0
stone = bpy.data.materials["HordeTownStone"]
solids = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and not obj.hide_get()]
walkable = []

# The v4 model has a closed foundation and a solid wall block. Retain B01's
# roof, chimney and upper shutter, and all of the other original buildings.
for name in ("B01_stone_foundation", "B01_closed_walls", "B01_door_post_-1",
             "B01_door_post_1", "B01_door_lintel", "B01_front_step"):
    obj = bpy.data.objects[name]
    solids.remove(obj)
    bpy.data.objects.remove(obj, do_unlink=True)

# Survey the selected runtime terrain, including any changes after v4.
for obj in list(bpy.context.scene.objects):
    if obj.type == "MESH" and obj.hide_get():
        bpy.data.objects.remove(obj, do_unlink=True)
existing = set(bpy.context.scene.objects)
bpy.ops.import_scene.gltf(filepath=str(TERRAIN))
points, faces = [], []
for obj in set(bpy.context.scene.objects) - existing:
    if obj.type != "MESH":
        continue
    offset = len(points)
    points.extend(obj.matrix_world @ vertex.co for vertex in obj.data.vertices)
    faces.extend([offset + index for index in face.vertices] for face in obj.data.polygons)
    obj.name = "B01_current_terrain_reference"
    obj.hide_set(True)
    obj.hide_render = True
terrain = BVHTree.FromPolygons(points, faces)


def ground(x, z):
    hit, _, _, _ = terrain.ray_cast(Vector((x, -z, 100)), Vector((0, 0, -1)), 300)
    if hit is None:
        raise RuntimeError(f"Missing B01 ground at {x}, {z}")
    return hit.z


def mesh(name, vertices, faces, group):
    data = bpy.data.meshes.new(name)
    data.from_pydata([(x, -z, y) for x, y, z in vertices], [], faces)
    data.materials.append(stone)
    data.update()
    uv = data.uv_layers.new(name="metre_projection")
    for face in data.polygons:
        axis = max(range(3), key=lambda candidate: abs(face.normal[candidate]))
        u, v = ((1, 2), (0, 2), (0, 1))[axis]
        for loop in face.loop_indices:
            vertex = data.vertices[data.loops[loop].vertex_index].co
            uv.data[loop].uv = (vertex[u] * .5, vertex[v] * .5)
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    group.append(obj)


def box(name, west, east, north, south, bottom, top):
    lower = [(west, bottom, north), (east, bottom, north),
             (east, bottom, south), (west, bottom, south)]
    upper = [(x, top, z) for x, _, z in lower]
    mesh(name, lower + upper, [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1),
                               (6, 7, 3, 2), (7, 4, 0, 3), (7, 6, 5, 4)], solids)


west = tavern["engineX"] - tavern["width"] / 2
east = tavern["engineX"] + tavern["width"] / 2
north = tavern["engineZ"] - tavern["depth"] / 2
south = tavern["engineZ"] + tavern["depth"] / 2
floor = tavern["actualFloorY"]
base = tavern["foundationBottomY"]
wall_top = floor + tavern["wallHeight"] - .02
# The mapped B01 door is at East 4, North 7 m. This is its registered position.
door_x = 82.3
door_west, door_east = door_x - 1.1, door_x + 1.1
wall_depth = .42
header_y = floor + 2.65
for name, x in (("west", west), ("east", east - wall_depth)):
    box(f"B01_{name}_wall", x, x + wall_depth, north, south, base, wall_top)
box("B01_north_wall", west + wall_depth, east - wall_depth, north, north + wall_depth, base, wall_top)
for name, lo, hi in (("west", west + wall_depth, door_west),
                     ("east", door_east, east - wall_depth)):
    box(f"B01_south_{name}_wall", lo, hi, south - wall_depth, south, base, wall_top)
box("B01_south_door_header", door_west, door_east, south - wall_depth, south, header_y, wall_top)

highest_inside = max(ground(west + .5 + column * .5, north + .5 + row * .5)
                     for column in range(43) for row in range(31))
if highest_inside >= floor:
    raise RuntimeError(f"Current terrain crosses the preserved B01 floor: {highest_inside}, {floor}")

# Use a single foundation under the room and ramp, with separately exported
# walking surfaces. There is no internal cross-face at the threshold to catch
# the leading edge of the capsule while its centre is still on the ramp.
# The toe is buried across the full width so there is no vertical lip.
ramp_west, ramp_east = door_x - 1.5, door_x + 1.5
toe_z = south + 11.5
toe_y = min(ground(ramp_west + index * .125, toe_z) for index in range(25)) - .045
side_base = min(ground(x, south + step * .25)
                for x in (ramp_west, ramp_east) for step in range(47)) - .15
if (floor - toe_y) / (toe_z - south) > .35:
    raise RuntimeError("B01 approach exceeds the selected grade.")

top = [(ramp_west, floor, south), (ramp_east, floor, south),
       (ramp_east, toe_y, toe_z), (ramp_west, toe_y, toe_z)]
foundation_top = [(west + wall_depth, floor, north + wall_depth),
                  (east - wall_depth, floor, north + wall_depth),
                  (east - wall_depth, floor, south),
                  top[1], top[2], top[3], top[0],
                  (west + wall_depth, floor, south)]
foundation_bottom = [(x, min(base, side_base), z) for x, _, z in foundation_top]
outline = len(foundation_top)
side_faces = [(i + outline, (i + 1) % outline + outline, (i + 1) % outline, i)
              for i in range(outline)]
mesh("B01_continuous_foundation", foundation_bottom + foundation_top,
     [tuple(range(outline))] + side_faces, solids)
mesh("B01_ramp_top", top, [(3, 2, 1, 0)], walkable)
mesh("B01_room_floor", [(west + wall_depth, floor, north + wall_depth),
                       (east - wall_depth, floor, north + wall_depth),
                       (east - wall_depth, floor, south),
                       (west + wall_depth, floor, south)], [(3, 2, 1, 0)], walkable)

# Three sides and a lintel are real masonry. Leave the mouth and the
# player/actor approach clear for the hearth and seated conversation work.
hearth_north, hearth_south = north + 5.3, north + 8.1
hearth_back, hearth_front = west + wall_depth, west + 1.55
for name, lo, hi in (("north", hearth_north, hearth_north + .4),
                     ("south", hearth_south - .4, hearth_south)):
    box("B01_hearth_" + name, hearth_back, hearth_front, lo, hi, floor, floor + 1.55)
box("B01_hearth_lintel", hearth_back, hearth_front + .10,
    hearth_north, hearth_south, floor + 1.35, floor + 1.75)


def joined_copy(group, name):
    bpy.ops.object.select_all(action="DESELECT")
    copies = []
    for obj in group:
        copy = obj.copy()
        copy.data = obj.data.copy()
        bpy.context.collection.objects.link(copy)
        copy.select_set(True)
        copies.append(copy)
    bpy.context.view_layer.objects.active = copies[0]
    bpy.ops.object.join()
    combined = bpy.context.object
    combined.name = name
    # The continuous foundation has a concave underside. Triangulate only
    # the export copy so glTF receives explicit tangents and the Blend stays editable.
    editable = bmesh.new()
    editable.from_mesh(combined.data)
    bmesh.ops.triangulate(editable, faces=list(editable.faces))
    editable.to_mesh(combined.data)
    editable.free()
    return combined


OUTPUT.mkdir(parents=True)
RUNTIME.mkdir(parents=True)
bpy.data.orphans_purge(do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "bellwether-tavern-v1.blend"), compress=True)
shell_mesh = joined_copy(solids, "BellwetherSolid")
floor_mesh = joined_copy(walkable, "BellwetherWalkable")
shell_mesh.select_set(True)
path = RUNTIME / "bellwether-tavern-v1.glb"
bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


solid_triangles = sum(len(face.vertices) - 2 for face in shell_mesh.data.polygons)
walkable_triangles = sum(len(face.vertices) - 2 for face in floor_mesh.data.polygons)
manifest = json.loads((WORLD / "runtime/bellwether-shell-v4/asset.manifest.json").read_text())
manifest["asset"] = "bellwether-tavern-v1"
manifest["budgets"].update(maxIndices=(solid_triangles + walkable_triangles) * 3, maxPrimitives=3)
manifest["lods"][0]["maxTriangles"] = solid_triangles + walkable_triangles
manifest["licenceStatus"] = "Original Horde-owned derivative of Bellwether v4. Shared texture terms are recorded in ASSET_LICENSES.md."
(RUNTIME / "asset.manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
receipt = dict(sourceMasterSha256=digest(MASTER), terrainSha256=digest(TERRAIN),
               layoutSha256=parent["planSha256"], runtimeSha256=digest(path),
               solidTriangles=solid_triangles, walkableTriangles=walkable_triangles,
               southDoor=[door_x, floor, south], clearDoorWidth=2.2, clearDoorHeight=2.65,
               rampToe=[door_x, toe_y, toe_z], rampWidth=3, highestGroundUnderFloor=highest_inside,
               hearthMouth=[hearth_front, floor, (hearth_north + hearth_south) / 2])
(OUTPUT / "authoring-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
print(json.dumps(receipt))
