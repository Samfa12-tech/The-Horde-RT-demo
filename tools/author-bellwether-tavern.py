"""Author the B01 interior and B09 wayside mechanism from the preserved Bellwether master.

blender --background --python-exit-code 1 --python tools/author-bellwether-tavern.py -- ROOT [OUTPUT_WORLD]
The editable and runtime output directories must both be new.
"""
import hashlib
import json
import math
from pathlib import Path
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree


args = sys.argv[sys.argv.index("--") + 1:]
if len(args) not in (1, 2):
    raise RuntimeError("Expected ROOT and an optional OUTPUT_WORLD.")
ROOT = Path(args[0]).resolve()
WORLD = ROOT / "assets/models/world"
MASTER = WORLD / "source/bellwether-shell-v4/bellwether-shell-v4.blend"
TERRAIN = WORLD / "runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb"
DESTINATION = Path(args[1]).resolve() if len(args) == 2 else WORLD
OUTPUT = DESTINATION / "source/bellwether-tavern-v1"
RUNTIME = DESTINATION / "runtime/bellwether-tavern-v1"
if OUTPUT.exists() or RUNTIME.exists():
    raise RuntimeError("Preserve the previous source and runtime outputs.")

parent = json.loads(MASTER.with_name("authoring-receipt.json").read_text())
tavern = next(building for building in parent["buildings"] if building["id"] == "B01")
bpy.ops.wm.open_mainfile(filepath=str(MASTER))
bpy.context.preferences.filepaths.save_version = 0
stone = bpy.data.materials["HordeTownStone"]
solids = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and not obj.hide_get()]
walkable = []
glazing = []

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
        raise RuntimeError(f"Missing Bellwether ground at {x}, {z}")
    return hit.z


def mesh(name, vertices, faces, group, material=stone):
    data = bpy.data.meshes.new(name)
    data.from_pydata([(x, -z, y) for x, y, z in vertices], [], faces)
    data.materials.append(material)
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
    return obj


def box(name, west, east, north, south, bottom, top, material=stone, group=solids):
    lower = [(west, bottom, north), (east, bottom, north),
             (east, bottom, south), (west, bottom, south)]
    upper = [(x, top, z) for x, _, z in lower]
    return mesh(name, lower + upper, [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1),
                                      (6, 7, 3, 2), (7, 4, 0, 3), (7, 6, 5, 4)], group, material)


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
window_west, window_east = 86.36, 88.18
window_bottom, window_top = floor + 1.65, floor + 2.65
for name, x in (("west", west), ("east", east - wall_depth)):
    box(f"B01_{name}_wall", x, x + wall_depth, north, south, base, wall_top)
box("B01_north_wall", west + wall_depth, east - wall_depth, north, north + wall_depth, base, wall_top)
for name, lo, hi, bottom, top_y in (
        ("west", west + wall_depth, door_west, base, wall_top),
        ("door_pier", door_east, window_west, base, wall_top),
        ("east", window_east, east - wall_depth, base, wall_top),
        ("sill", window_west, window_east, base, window_bottom),
        ("window_header", window_west, window_east, window_top, wall_top)):
    box(f"B01_south_{name}_wall", lo, hi, south - wall_depth, south, bottom, top_y)
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


def opaque_finish(name, color, roughness):
    material = bpy.data.materials.new(name)
    material.diffuse_color = (*color, 1)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = material.diffuse_color
    principled.inputs["Metallic"].default_value = 0
    principled.inputs["Roughness"].default_value = roughness
    return material


# Native admission maps these textureless finishes onto two existing opaque
# material rows. Edge highlights and plank seams distinguish the timber.
oak = opaque_finish("HordeTavernTimber", (.25, .115, .065), .84)
ember = opaque_finish("HordeHearthChar", (.025, .019, .013), .95)
# The sill and posts hold a closed, 8 mm thick glass volume. Put the
# glazing itself in a separate draw group for the shared dielectric route.
glass = opaque_finish("HordeTavernWindowGlass", (.96, .985, .94), .09)
glass.node_tree.nodes.get("Principled BSDF").inputs["Transmission Weight"].default_value = .94
window_plane = south - .22
frame_north, frame_south = window_plane - .065, window_plane + .065
rail = .08
for name, lo, hi in (("left", window_west, window_west + rail),
                     ("right", window_east - rail, window_east),
                     ("mullion", (window_west + window_east - rail) / 2,
                      (window_west + window_east + rail) / 2)):
    box("B01_window_" + name, lo, hi, frame_north, frame_south, window_bottom, window_top, oak)
for name, lo, hi in (("lower", window_bottom, window_bottom + rail),
                     ("upper", window_top - rail, window_top)):
    box("B01_window_" + name + "_rail", window_west + rail, window_east - rail,
        frame_north, frame_south, lo, hi, oak)
pane_min = (window_west + rail, window_bottom + rail, window_plane - .004)
pane_max = (window_east - rail, window_top - rail, window_plane + .004)
box("B01_window_pane", pane_min[0], pane_max[0], pane_min[2], pane_max[2],
    pane_min[1], pane_max[1], glass, glazing)
chair_x, chair_z, chair_top = 82.0, 107.30, .49
table_x, table_z, table_top = 82.52, 106.13, .75
chair_depth, chair_board_x = .41, chair_x - .10
seat_mark = (chair_x, floor + chair_top, chair_z)
sit_root = (chair_x + .46, floor, chair_z)
approach_mark = (82.98, floor, chair_z)
turn_mark = (84.30, floor, chair_z)
talk_mark = (84.52, floor, 107.68)
keeper_door = (83.78, floor, south - 1.0)
keeper_aisle = (85.65, floor, 116.15)
keeper_hearth = (85.65, floor, 109.10)
keeper_door_yaw = math.atan2(door_x - keeper_door[0], keeper_door[2] - south)
listener_centre = tuple((kit + player) * .5 for kit, player in zip(sit_root, talk_mark))
keeper_hearth_yaw = math.atan2(listener_centre[0] - keeper_hearth[0], keeper_hearth[2] - listener_centre[2])


def joined_timber(name, x, z, w, d, low, high, bevel=.007):
    obj = box(name, x - w / 2, x + w / 2, z - d / 2, z + d / 2, floor + low, floor + high, oak)
    bpy.context.view_layer.objects.active = obj
    modifier = obj.modifiers.new("Hand-rounded edges", "BEVEL")
    modifier.width = bevel
    modifier.segments = 1
    bpy.ops.object.modifier_apply(modifier=modifier.name)


# The chair faces east. It can remain a fixed environmental prop after Kit
# settles. The seat has no gap under the contact point, and all four feet
# touch the same exported room floor used by the gameplay motor.
for side in (-1, 1):
    z = chair_z + side * .24
    joined_timber(f"B01_chair_front_leg_{side}", chair_x + .057, z, .08, .08, 0, .45)
    joined_timber(f"B01_chair_back_post_{side}", chair_x - .24, z, .088, .09, 0, 1.09)
    joined_timber(f"B01_chair_side_stretcher_{side}", chair_x - .092, z, .29, .047, .19, .245)
for plank in (-1, 0, 1):
    joined_timber(f"B01_chair_seat_{plank}", chair_board_x, chair_z + plank * .198,
                  chair_depth, .196, .43, chair_top, .004)
joined_timber("B01_chair_back_lower", chair_x - .245, chair_z, .062, .49, .68, .795)
joined_timber("B01_chair_back_upper", chair_x - .245, chair_z, .062, .542, .89, 1.045)
joined_timber("B01_chair_front_apron", chair_x + .049, chair_z, .056, .49, .35, .438)

for side in (-1, 1):
    x = table_x + side * .375
    for corner in (-1, 1):
        z = table_z + corner * .210
        joined_timber(f"B01_table_leg_{side}_{corner}", x, z, .09, .09, 0, .70)
    joined_timber(f"B01_table_end_apron_{side}", x, table_z, .06, .44, .59, .704)
    joined_timber(f"B01_table_long_apron_{side}", table_x, table_z + side * .210, .76, .06, .59, .704)
for plank in (-1, 0, 1):
    joined_timber(f"B01_table_top_{plank}", table_x, table_z + plank * .205, 1.02, .203, .695, table_top)


def candle_piece(name, profile, material):
    sides = 8
    vertices = [(candle_x + radius * math.cos(math.tau * i / sides), candle_y + height,
                 candle_z + radius * math.sin(math.tau * i / sides))
                for height, radius in profile for i in range(sides)]
    top = (len(profile) - 1) * sides
    faces = [tuple(reversed(range(sides))), tuple(range(top, top + sides))]
    for ring_start in range(0, top, sides):
        for i in range(sides):
            a, b = ring_start + i, ring_start + (i + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    return mesh(name, vertices, faces, solids, material)


candle_x, candle_y, candle_z = 82.93, floor + table_top, 106.39
candle_piece("B01_table_candle_holder", [(0, .086), (.018, .094), (.042, .079)], ember)
candle_piece("B01_table_candle_wax", [(.03, .045), (.232, .042), (.251, .034)], oak)
box("B01_table_candle_wick", candle_x - .005, candle_x + .005, candle_z - .005,
    candle_z + .005, candle_y + .248, candle_y + .27, ember)
candle_flame = (candle_x, candle_y + .269, candle_z)
candle_light = (candle_x, candle_y + .326, candle_z)


def billet(name, x, z, offset_y, heading, length, radius):
    direction_x, direction_z = math.cos(heading), math.sin(heading)
    ring = 7
    vertices = []
    for end in (-1, 1):
        for side in range(ring):
            angle = math.tau * side / ring
            ridge = radius * (1 if side % 2 == 0 else .9)
            cross = math.sin(angle) * ridge
            vertices.append((x + end * direction_x * length / 2 - direction_z * cross,
                             floor + offset_y + math.cos(angle) * ridge,
                             z + end * direction_z * length / 2 + direction_x * cross))
    faces = [tuple(reversed(range(ring))), tuple(range(ring, ring * 2))]
    for side in range(ring):
        next_side = (side + 1) % ring
        faces.append((side, next_side, ring + next_side, ring + side))
    # Recalculate the closed stave's normals before projection/export.
    obj = mesh(name, vertices, faces, solids, ember)
    editable = bmesh.new()
    editable.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(editable, faces=list(editable.faces))
    editable.to_mesh(obj.data)
    editable.free()


billet("B01_hearth_billet_front", 80.66, 109.74, .056, .18, .53, .057)
billet("B01_hearth_billet_back", 80.53, 110.04, .055, -.23, .49, .057)
billet("B01_hearth_billet_cross", 80.54, 109.89, .12, 1.43, .50, .047)


# West-facing waymarker in the wedge between the north path and tavern road.
# From that approach the left-pointing board follows the church branch.
sign_x, sign_z = 34.65, 82.10
sign_y = ground(sign_x, sign_z)
post_bottom = min(ground(sign_x + dx, sign_z + dz)
                  for dx in (-.10, .10) for dz in (-.10, .10)) - .17
box("B02_waymarker_post", sign_x - .10, sign_x + .10, sign_z - .10, sign_z + .10,
    post_bottom, sign_y + 1.60, oak)
sign_centre_y = sign_y + 1.37
outline = [(-.77, 0), (-.51, -.20), (.73, -.20), (.73, .20), (-.51, .20)]
edge_count = len(outline)
mesh("B02_church_arrow_board",
     [(sign_x + depth, sign_centre_y + height, sign_z + along)
      for depth in (-.13, .03) for along, height in outline],
     [tuple(range(edge_count)), tuple(reversed(range(edge_count, edge_count * 2)))] +
     [((i + 1) % edge_count, i, i + edge_count, (i + 1) % edge_count + edge_count)
      for i in range(edge_count)], solids, ember)

# Original chisel strokes with a light timber inlay on the west face.
# Each strip shares the board's real surface and RT lighting.
strokes = {
    "C": [((1, 1), (.26, 1), (0, .8), (0, .2), (.26, 0), (1, 0))],
    "H": [((0, 0), (0, 1)), ((1, 0), (1, 1)), ((0, .5), (1, .5))],
    "U": [((0, 1), (0, .22), (.25, 0), (.75, 0), (1, .22), (1, 1))],
    "R": [((0, 0), (0, 1), (.79, 1), (1, .87), (1, .61), (.79, .5), (0, .5)),
          ((.52, .5), (1, 0))],
}
letter_vertices, letter_faces = [], []
for index, letter in enumerate("CHURCH"):
    left = sign_z - .451 + index * .182
    for path in strokes[letter]:
        for start, end in zip(path, path[1:]):
            a = Vector((left + start[0] * .135, sign_centre_y + (start[1] - .5) * .225))
            b = Vector((left + end[0] * .135, sign_centre_y + (end[1] - .5) * .225))
            tangent = (b - a).normalized() * .017
            normal = Vector((-tangent.y, tangent.x))
            face = (a - tangent - normal, b + tangent - normal, b + tangent + normal, a - tangent + normal)
            first = len(letter_vertices)
            front = sign_x - .132 - (first // 8) * .00003
            letter_vertices.extend((x, y, z) for x in (front, sign_x - .128) for z, y in face)
            letter_faces.extend((tuple(range(first, first + 4)), tuple(range(first + 7, first + 3, -1))))
            for corner in range(4):
                a_index, b_index = first + corner, first + (corner + 1) % 4
                letter_faces.append((b_index, a_index, a_index + 4, b_index + 4))
mesh("B02_church_arrow_letters", letter_vertices, letter_faces, solids, oak)


# B09 keeps its mapped footprint and distant silhouette. The old archive
# compartment sits outside on the reachable south slope below the shell.
shrine = next(building for building in parent["buildings"] if building["id"] == "B09")
shrine_x = shrine["engineX"]
archive_back = shrine["engineZ"] + shrine["depth"] / 2 + .80
archive_front = archive_back + 1.40
archive_half_width = 1.05
archive_edges = (-archive_half_width, 0, archive_half_width)
archive_base = min(ground(shrine_x + dx, z) for dx in archive_edges
                   for z in (archive_back, archive_front)) - .18
archive_lintel = max(ground(shrine_x + dx, archive_back) for dx in archive_edges) + .54


def shrine_box(name, lo, hi, back, front, bottom, top, material=stone):
    box("B09_" + name, shrine_x + lo, shrine_x + hi,
        archive_front + back, archive_front + front, bottom, top, material)


# A hollow compartment with separate solid sides and sill. The lower slab is
# a separate editable mesh so a later interaction can turn it about the pin.
opening_bottom = archive_lintel - .66
shrine_box("archive_sill", -archive_half_width, archive_half_width, -1.40, 0, archive_base, opening_bottom)
shrine_box("archive_back", -.66, .66, -1.40, -1.05, opening_bottom, archive_lintel)
for name, lo, hi in (("west", -archive_half_width, -.66), ("east", .66, archive_half_width)):
    shrine_box("archive_" + name, lo, hi, -1.40, 0, opening_bottom, archive_lintel)
shrine_box("archive_lintel", -archive_half_width, archive_half_width, -1.40, .06, archive_lintel, archive_lintel + .31)


def shrine_face(name, outline, back, front, material=stone):
    size = len(outline)
    mesh("B09_" + name,
         [(shrine_x + x, opening_bottom + y, archive_front + z)
          for z in (back, front) for x, y in outline],
         [tuple(reversed(range(size))), tuple(range(size, size * 2))] +
         [(i, (i + 1) % size, (i + 1) % size + size, i + size) for i in range(size)], solids, material)


shrine_face("lower_stone", [(-.61, .03), (.50, .03), (.62, .14), (.60, .49), (.47, .60),
                           (-.50, .60), (-.61, .48)], -.19, -.045)
# Two readable knuckles straddle the cover seam. The roots partly conceal the
# bottom knuckle, leaving the upper pin and the edge of the stone discoverable.
hinge_x = -.61
for name, low, high in (("hinge_lower", .13, .23), ("hinge_upper", .43, .53)):
    shrine_box(name, -.74, -.54, -.13, .028, opening_bottom + low, opening_bottom + high, ember)
shrine_box("hinge_pin", hinge_x - .028, hinge_x + .028, -.042, .031,
           opening_bottom + .08, opening_bottom + .57, ember)
shrine_face("root_at_hinge", [(-.96, -.58), (-.84, -.55), (-.80, -.29), (-.74, -.20), (-.69, .04),
                             (-.58, .25), (-.61, .28), (-.77, .17), (-.81, .02), (-.95, -.10)], -.02, .065, oak)
shrine_face("root_at_sill", [(-1.11, -.63), (-.98, -.61), (-.89, -.38), (-.69, -.30), (-.54, -.13),
                            (-.36, -.08), (-.39, -.035), (-.61, -.066), (-.77, -.22), (-1.02, -.27)], .01, .10, oak)
shrine_mechanism = [shrine_x, opening_bottom + .32, archive_front]
shrine_hinge = [shrine_x + hinge_x, opening_bottom + .08, archive_front + .01]
shrine_view = [shrine_x, ground(shrine_x, archive_front + 2.2), archive_front + 2.2]


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
blocking = bpy.data.collections.new("B01 - Tavern keeper blocking")
bpy.context.scene.collection.children.link(blocking)
for name, mark, yaw in (("Door greeting", keeper_door, keeper_door_yaw),
                        ("Aisle turn", keeper_aisle, 0),
                        ("Hearth conversation", keeper_hearth, keeper_hearth_yaw)):
    marker = bpy.data.objects.new(name, None)
    marker.empty_display_type = "ARROWS"
    marker.empty_display_size = .45
    marker.location = (mark[0], -mark[2], mark[1])
    marker.rotation_euler.z = -yaw
    blocking.objects.link(marker)
archive_blocking = bpy.data.collections.new("B09 - Wayside archive blocking")
bpy.context.scene.collection.children.link(archive_blocking)
for name, mark in (("Hinge rotation origin", shrine_hinge), ("Player inspection mark", shrine_view)):
    marker = bpy.data.objects.new(name, None)
    marker.empty_display_type = "PLAIN_AXES"
    marker.empty_display_size = .25
    marker.location = (mark[0], -mark[2], mark[1])
    archive_blocking.objects.link(marker)
bpy.data.orphans_purge(do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "bellwether-tavern-v1.blend"), compress=True)
shell_mesh = joined_copy(solids, "BellwetherSolid")
floor_mesh = joined_copy(walkable, "BellwetherWalkable")
shell_mesh.select_set(True)
path = RUNTIME / "bellwether-tavern-v1.glb"
bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)
pane_mesh = joined_copy(glazing, "BellwetherGlazing")
pane_path = RUNTIME / "bellwether-window.glb"
bpy.ops.export_scene.gltf(filepath=str(pane_path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


solid_triangles = sum(len(face.vertices) - 2 for face in shell_mesh.data.polygons)
walkable_triangles = sum(len(face.vertices) - 2 for face in floor_mesh.data.polygons)
manifest = json.loads((WORLD / "runtime/bellwether-shell-v4/asset.manifest.json").read_text())
manifest["asset"] = "bellwether-tavern-v1"
manifest["budgets"].update(maxVertices=6500, maxIndices=(solid_triangles + walkable_triangles) * 3,
                           maxPrimitives=5, maxMaterials=4)
manifest["lods"][0]["maxTriangles"] = solid_triangles + walkable_triangles
manifest["licenceStatus"] = "Original Horde-owned derivative of Bellwether v4. Shared texture terms are recorded in ASSET_LICENSES.md."
(RUNTIME / "asset.manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
pane_triangles = len(pane_mesh.data.polygons)
pane_manifest = dict(manifest, asset="bellwether-window", materialOverrides=[{
    "material": glass.name, "transmissionFactor": .94, "ior": 1.52,
    "thicknessFactor": .008, "attenuationDistance": .42,
    "attenuationColor": [.91, .96, .84], "roughnessFactor": .09, "thinWall": False
}])
pane_manifest["budgets"] = dict(manifest["budgets"], maxVertices=24, maxIndices=pane_triangles * 3,
                                maxPrimitives=1, maxMaterials=1)
pane_manifest["lods"] = [{"name": "lod0", "maxTriangles": pane_triangles}]
(RUNTIME / "window.manifest.json").write_text(json.dumps(pane_manifest, indent=2) + "\n",
                                              encoding="utf-8", newline="\n")
receipt = dict(sourceMasterSha256=digest(MASTER), terrainSha256=digest(TERRAIN),
               layoutSha256=parent["planSha256"], runtimeSha256=digest(path),
               solidTriangles=solid_triangles, walkableTriangles=walkable_triangles,
               windowSha256=digest(pane_path), windowTriangles=pane_triangles,
               windowBounds=[pane_min, pane_max],
               southDoor=[door_x, floor, south], clearDoorWidth=2.2, clearDoorHeight=2.65,
               rampToe=[door_x, toe_y, toe_z], rampWidth=3, highestGroundUnderFloor=highest_inside,
               hearthMouth=[hearth_front, floor, (hearth_north + hearth_south) / 2],
               chairSeat=seat_mark, chairSitRoot=sit_root, chairApproach=approach_mark, chairTurn=turn_mark,
               chairSeatBoundsXZ=[chair_board_x - chair_depth / 2, chair_board_x + chair_depth / 2,
                                  chair_z - .296, chair_z + .296],
               tableTop=[table_x, floor + table_top, table_z], conversation=talk_mark,
               tableCandleFlame=candle_flame, tableCandleLight=candle_light,
               churchWaymarker=[sign_x,sign_y,sign_z],
               waysideArchive=[shrine_mechanism, shrine_hinge, shrine_view],
               keeperRoute=[keeper_door, keeper_aisle, keeper_hearth],
               keeperDoorYaw=keeper_door_yaw, keeperHearthYaw=keeper_hearth_yaw)
(OUTPUT / "authoring-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")

stage = ["// Generated by tools/author-bellwether-tavern.py. Engine coordinates in metres.",
         "#pragma once", "#include <array>", "namespace horde::gameplay::bellwether {", ""]
for name, mark in (("ChairSeat", seat_mark), ("ChairSitRoot", sit_root),
                   ("ChairApproach", approach_mark), ("ChairTurn", turn_mark),
                   ("TableTop", receipt["tableTop"]), ("Conversation", talk_mark),
                   ("TableCandleFlame", candle_flame), ("TableCandleLight", candle_light),
                   ("ChurchWaymarker", receipt["churchWaymarker"]),
                   ("WaysideMechanism", shrine_mechanism), ("WaysideHinge", shrine_hinge),
                   ("WaysideInspection", shrine_view),
                   ("KeeperDoor", keeper_door), ("KeeperAisle", keeper_aisle), ("KeeperHearth", keeper_hearth)):
    position = ", ".join(f"{coordinate:.6f}f" for coordinate in mark)
    stage.append(f"inline constexpr std::array<float, 3> k{name}{{{{{position}}}}};")
stage += ["", "inline constexpr float kChairFacingRadians=1.570796327f;",
          f"inline constexpr float kKeeperDoorFacingRadians={keeper_door_yaw:.9f}f;",
          f"inline constexpr float kKeeperHearthFacingRadians={keeper_hearth_yaw:.9f}f;", "}", ""]
(OUTPUT / "staging.generated.h").write_text("\n".join(stage), encoding="utf-8", newline="\n")
print(json.dumps(receipt))
