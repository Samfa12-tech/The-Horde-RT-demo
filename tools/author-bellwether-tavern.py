"""Author the B01 interior, B05 mill yard and B09 archive from the preserved master.

blender --background --python-exit-code 1 --python tools/author-bellwether-tavern.py -- ROOT [OUTPUT_WORLD [INPUT_TOWN_GLB]]
The editable and runtime output directories must both be new.
"""
import hashlib
import json
import math
from pathlib import Path
import runpy
import shutil
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree


args = sys.argv[sys.argv.index("--") + 1:]
if len(args) not in (1, 2, 3):
    raise RuntimeError("Expected ROOT and optional OUTPUT_WORLD and INPUT_TOWN_GLB.")
ROOT = Path(args[0]).resolve()
WORLD = ROOT / "assets/models/world"
MASTER = WORLD / "source/bellwether-shell-v4/bellwether-shell-v4.blend"
TERRAIN = WORLD / "runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb"
DESTINATION = Path(args[1]).resolve() if len(args) >= 2 else WORLD
OUTPUT = DESTINATION / "source/bellwether-tavern-v1"
RUNTIME = DESTINATION / "runtime/bellwether-tavern-v1"
INPUT_TOWN = (Path(args[2]).resolve() if len(args) == 3 else
              WORLD / "runtime/bellwether-tavern-v1/bellwether-tavern-v1.glb")
FORK_LAMP_SOURCE_TOWN_SHA256 = "1133a71dbe7ac47b3c306bc687489b10d9036f7b4d6727bfd6115557d1b667b2"
# Filled with the first authorized layered output hash so a later exact rerun
# recognizes an already-installed lamp and cannot append its triangles twice.
FORK_LAMP_LAYERED_TOWN_SHA256 = "4a970dc48d0d7028310ddc0cf4dd26c00c1325a77136ce52470f9287e784a4d6"
if OUTPUT.exists() or RUNTIME.exists():
    raise RuntimeError("Preserve the previous source and runtime outputs.")

parent = json.loads(MASTER.with_name("authoring-receipt.json").read_text())
tavern = next(building for building in parent["buildings"] if building["id"] == "B01")
mill = next(building for building in parent["buildings"] if building["id"] == "B05")
bpy.ops.wm.open_mainfile(filepath=str(MASTER))
bpy.context.preferences.filepaths.save_version = 0
stone = bpy.data.materials["HordeTownStone"]
solids = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and not obj.hide_get()]
walkable = []
glazing = []
archive_cover = []
archive_register = []

# The v4 model has a closed foundation and a solid wall block. Retain B01's
# roof, chimney and upper shutter. B05 keeps its shell, with the entrance
# detail moving to the mapped west working edge.
for name in ("B01_stone_foundation", "B01_closed_walls", "B01_door_post_-1",
             "B01_door_post_1", "B01_door_lintel", "B01_front_step",
             "B05_door_post_-1", "B05_door_post_1", "B05_door_lintel", "B05_front_step"):
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

# B01's editable hinge is posed open for the arrival. Its leaf is exported
# in hinge-local space for shared rendering and contact.
door_hinge = (door_west + .088, floor, south - wall_depth - .085)
door_open_degrees = 94
door_width = 2.02
door_leaf = []
door_frame = []
hinge = bpy.data.objects.new("B01_SouthDoorHinge", None)
bpy.context.collection.objects.link(hinge)
hinge.empty_display_type = "ARROWS"
hinge.empty_display_size = .28
hinge.location = (door_hinge[0], -door_hinge[2], door_hinge[1])
hinge.rotation_euler.z = math.radians(door_open_degrees)


def leaf_piece(name, x0, x1, z0, z1, y0, y1, material=oak):
    piece = box("B01_door_" + name, x0, x1, z0, z1, y0, y1, material, door_leaf)
    piece.parent = hinge


for index in range(8):
    start, end = index * door_width / 8, (index + 1) * door_width / 8
    leaf_piece(f"plank_{index + 1}", start + .0018, end - .0018, -.045, .045,
               .038 + .002 * (index % 3), 2.536 - .002 * ((index + 1) % 3))
for suffix, height in (("lower", .49), ("upper", 1.98)):
    leaf_piece(suffix + "_rail", .08, door_width - .065, -.087, -.043, height, height + .17)
    leaf_piece(suffix + "_strap", -.022, .71, .042, .062, height + .058, height + .12, ember)
    leaf_piece(suffix + "_hinge_knuckle", -.032, .032, -.031, .034, height + .029, height + .145, ember)
    # Fixed pintles stay with the jamb when editing the hinge.
    box("B01_door_fixed_" + suffix + "_pintle", door_west - .014, door_hinge[0] + .016,
        door_hinge[2] - .045, south - wall_depth + .036, floor + height + .047,
        floor + height + .105, ember, door_frame)
for face, side in (("inside", -1), ("outside", 1)):
    plate_z = sorted((side * .046, side * .069))
    pull_z = sorted((side * .061, side * .117))
    leaf_piece(face + "_latch_plate", 1.74, 1.87, *plate_z, .99, 1.25, ember)
    leaf_piece(face + "_pull_low", 1.799, 1.839, *pull_z, 1.035, 1.069, ember)
    leaf_piece(face + "_pull_high", 1.799, 1.839, *pull_z, 1.174, 1.208, ember)
    leg_z = sorted((side * .102, side * .128))
    leaf_piece(face + "_pull_grip", 1.799, 1.839, *leg_z, 1.048, 1.193, ember)

door_frame_width = .068
for label, x0 in (("west", door_west), ("east", door_east - door_frame_width)):
    box("B01_door_" + label + "_jamb", x0, x0 + door_frame_width, south - .426, south - .129,
        floor + .002, header_y, oak, door_frame)
box("B01_door_head_frame", door_west + door_frame_width, door_east - door_frame_width,
    south - .426, south - .129, header_y - .068, header_y, oak, door_frame)

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


candle_x, candle_y, candle_z = 83.15, floor, 106.66
candle_stand_height = 1.00
holder_radius = .115
if not (table_z + .3065 < candle_z - holder_radius and candle_z + holder_radius < chair_z - .296):
    raise RuntimeError("The candlestick foot must clear the table and chair.")
candle_piece("B01_candlestick_stand", [(0, .104), (.021, holder_radius), (.065, .026),
             (candle_stand_height, .024), (candle_stand_height + .018, .053),
             (candle_stand_height + .042, .062)], ember)
candle_piece("B01_candlestick_wax",
             [(candle_stand_height + height, radius) for height, radius in
              [(.03, .045), (.232, .042), (.251, .034)]], oak)
box("B01_candlestick_wick", candle_x - .005, candle_x + .005, candle_z - .005,
    candle_z + .005, candle_y + candle_stand_height + .248,
    candle_y + candle_stand_height + .27, ember)
candle_flame = (candle_x, candle_y + candle_stand_height + .269, candle_z)
candle_light = (candle_x, candle_y + candle_stand_height + .326, candle_z)


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


# Timber waymarkers with original chisel-stroke inlays.
letter_strokes = {
    "C": [((1, 1), (.26, 1), (0, .8), (0, .2), (.26, 0), (1, 0))],
    "H": [((0, 0), (0, 1)), ((1, 0), (1, 1)), ((0, .5), (1, .5))],
    "U": [((0, 1), (0, .22), (.25, 0), (.75, 0), (1, .22), (1, 1))],
    "R": [((0, 0), (0, 1), (.79, 1), (1, .87), (1, .61), (.79, .5), (0, .5)),
          ((.52, .5), (1, 0))],
    "M": [((0, 0), (0, 1), (.5, .48), (1, 1), (1, 0))],
    "I": [((.5, 0), (.5, 1)), ((.13, 0), (.87, 0)), ((.13, 1), (.87, 1))],
    "L": [((0, 1), (0, 0), (1, 0))],
}


def waymarker(post, board, label, x, z, *, north_facing=False, glyph_width=.135, spacing=.182):
    y = ground(x, z)
    bottom = min(ground(x + dx, z + dz) for dx in (-.10, .10) for dz in (-.10, .10)) - .17
    box(post, x - .10, x + .10, z - .10, z + .10, bottom, y + 1.60, oak)
    board_axis = -x if north_facing else z
    board_y = y + 1.37

    def point(depth, along, height):
        if north_facing:
            return (-along, height, z + depth)
        return (x + depth, height, along)

    outline = [(-.77, 0), (-.51, -.20), (.73, -.20), (.73, .20), (-.51, .20)]
    count = len(outline)
    mesh(board + "_board",
         [point(depth, board_axis + along, board_y + height)
          for depth in (-.13, .03) for along, height in outline],
         [tuple(range(count)), tuple(reversed(range(count, count * 2)))] +
         [((i + 1) % count, i, i + count, (i + 1) % count + count) for i in range(count)],
         solids, ember)
    letters, faces = [], []
    first_glyph = board_axis + (.0715 - ((len(label) - 1) * spacing + glyph_width) / 2)
    for index, letter in enumerate(label):
        left = first_glyph + index * spacing
        for path in letter_strokes[letter]:
            for start, end in zip(path, path[1:]):
                a = Vector((left + start[0] * glyph_width, board_y + (start[1] - .5) * .225))
                b = Vector((left + end[0] * glyph_width, board_y + (end[1] - .5) * .225))
                tangent = (b - a).normalized() * .017
                normal = Vector((-tangent.y, tangent.x))
                corners = (a - tangent - normal, b + tangent - normal,
                           b + tangent + normal, a - tangent + normal)
                first = len(letters)
                front = -.132 - (first // 8) * .00003
                letters.extend(point(depth, along, height)
                               for depth in (front, -.128) for along, height in corners)
                faces.extend((tuple(range(first, first + 4)), tuple(range(first + 7, first + 3, -1))))
                for corner in range(4):
                    a_index, b_index = first + corner, first + (corner + 1) % 4
                    faces.append((b_index, a_index, a_index + 4, b_index + 4))
    mesh(board + "_letters", letters, faces, solids, oak)
    return [x, y, z]


church_waymarker = waymarker("B02_waymarker_post", "B02_church_arrow", "CHURCH", 34.65, 82.10)
# The arrows point east from the square and north at the fork. Keep the
# posts outside the player lane and its branch toward the salvager.
mill_waymarkers = [
    waymarker("B05_square_waymarker_post", "B05_square_arrow", "MILL", 85.55, 131.85,
              north_facing=True, glyph_width=.18, spacing=.23),
    waymarker("B05_fork_waymarker_post", "B05_fork_arrow", "MILL", 104.6, 123.8,
              glyph_width=.18, spacing=.23),
]


# The preserved B05 shell stays shut. The west door, landing and the repair
# bench establish the working edge facing Bellwether, above the river cut.
mill_west = mill["engineX"] - mill["width"] / 2
mill_floor, mill_z = mill["actualFloorY"], mill["engineZ"]
for side in (-1, 1):
    jamb_z = mill_z + side * 1.19
    box(f"B05_west_jamb_{side}", mill_west - .17, mill_west + .02, jamb_z - .17, jamb_z + .17,
        mill_floor - .03, mill_floor + 2.60)
box("B05_west_lintel", mill_west - .18, mill_west + .02, mill_z - 1.36, mill_z + 1.36,
    mill_floor + 2.32, mill_floor + 2.62)
for plank in range(7):
    start = mill_z - 1.05 + plank * .30
    box(f"B05_west_door_plank_{plank}", mill_west - .12, mill_west + .01, start, start + .287,
        mill_floor, mill_floor + 2.32, oak)
for height in (.42, 1.88):
    box(f"B05_door_strap_{height}", mill_west - .147, mill_west - .115, mill_z - 1.07, mill_z + .97,
        mill_floor + height, mill_floor + height + .095, ember)
box("B05_door_latch", mill_west - .195, mill_west - .142, mill_z + .78, mill_z + .98,
    mill_floor + 1.05, mill_floor + 1.18, ember)

landing_west = mill_west - 2.06
landing_north, landing_south = mill_z - 1.28, mill_z + 1.28
landing_top = mill_floor - .015
landing_bottom = min(ground(x, z) for x in (landing_west, mill_west)
                     for z in (landing_north, landing_south)) - .12
# The western edge of the top meets the grade while still under the terrain.
if min(ground(landing_west, landing_north + i * .08) for i in range(33)) <= landing_top:
    raise RuntimeError("B05 landing must have a buried approach from the village.")
landing_outline = [(landing_west, landing_top, landing_north),
                   (mill_west, landing_top, landing_north), (mill_west, landing_top, landing_south),
                   (landing_west, landing_top, landing_south)]
mesh("B05_west_landing_top", landing_outline, [(3, 2, 1, 0)], walkable)
mesh("B05_west_landing_retaining_stone",
     [(x, landing_bottom, z) for x, _, z in landing_outline] + landing_outline,
     [(0, 1, 2, 3), (4, 5, 1, 0), (6, 7, 3, 2), (7, 4, 0, 3)], solids)

bench_west, bench_east = mill_west - 1.57, mill_west - .54
bench_north, bench_south = mill_z + 1.70, mill_z + 3.38
bench_top = ground(bench_west, (bench_north + bench_south) / 2) + .98
for z in (bench_north + .15, bench_south - .15):
    for x in (bench_west + .14, bench_east - .14):
        bottom = min(ground(x + dx, z + dz) for dx in (-.08, .08) for dz in (-.08, .08)) - .08
        box(f"B05_workbench_leg_{x}_{z}", x - .08, x + .08, z - .08, z + .08,
            bottom, bench_top - .07, oak)
    box(f"B05_workbench_trestle_{z}", bench_west + .05, bench_east - .05, z - .08, z + .08,
        bench_top - .27, bench_top - .075, oak)
for board in range(4):
    x = bench_west + board * .26
    box(f"B05_workbench_top_{board}", x, x + .25, bench_north, bench_south,
        bench_top - .085, bench_top, oak)
bench_z = (bench_north + bench_south) / 2
for side in (-1, 1):
    z = bench_z + side * .29
    box(f"B05_bearing_cradle_{side}", bench_east - .45, bench_east - .11, z - .09, z + .09,
        bench_top, bench_top + .13, ember)
mill_work_mark = (mill_west - 2.20, ground(mill_west - 2.20, bench_z), bench_z)
mill_door = (mill_west, mill_floor, mill_z)
mill_comparison = (bench_west + .32, bench_top + .16, bench_south - .28)
# The old bearing remains as a reference at the bench. The spare stock is
# kept in an open bin, with individual contents below the front board.
ring_x, ring_y, ring_z = mill_comparison
segments = 12
ring = [(ring_x + radius * math.cos(i * math.tau / segments), ring_y + height,
         ring_z + radius * math.sin(i * math.tau / segments))
        for height, radius in ((-.16, .19), (-.16, .105), (-.07, .19), (-.07, .105))
        for i in range(segments)]
ring_faces = []
for i in range(segments):
    j = (i + 1) % segments
    outer, inner = segments * 2, segments * 3
    ring_faces.extend(((i, j, segments + j, segments + i), (outer + i, inner + i, inner + j, outer + j),
                       (i, outer + i, outer + j, j), (segments + i, segments + j, inner + j, inner + i)))
mesh("B05_old_bearing", ring, ring_faces, solids, ember)

bin_x, bin_z = 112.90, 128.30
bin_west, bin_east = bin_x - .39, bin_x + .39
bin_north, bin_south = bin_z - .46, bin_z + .46
bin_bottom = min(ground(x, z) for x in (bin_west, bin_east) for z in (bin_north, bin_south)) - .06
bin_top = ground(bin_x, bin_z) + .70
box("B05_spare_parts_bin_base", bin_west, bin_east, bin_north, bin_south, bin_bottom, bin_top - .32, oak)
for x in (bin_west, bin_east - .055):
    box(f"B05_spare_parts_bin_end_{x}", x, x + .055, bin_north, bin_south, bin_top - .33, bin_top, oak)
for z in (bin_north, bin_south - .055):
    box(f"B05_spare_parts_bin_side_{z}", bin_west + .055, bin_east - .055, z, z + .055,
        bin_top - .33, bin_top, oak)
for y in (bin_top - .23, bin_top - .055):
    box(f"B05_spare_parts_bin_strap_{y}", bin_west - .015, bin_west + .002, bin_north, bin_south,
        y - .011, y + .013, ember)
mill_bearing_bin = (bin_x, bin_top + .09, bin_z)


# A small shared-town wick lamp sits just off the fork's shore-path edge. The
# open face points back toward the Salvager; its footprint is admitted against
# both the NPC approach spoke and the full river landing route using the same
# .28 m player capsule radius as Chapter movement.
fork_lamp_x, fork_lamp_z = 113.30, 126.95
fork_lamp_y = ground(fork_lamp_x, fork_lamp_z)
fork_lamp_target = (114.20, 124.95)
fork_lamp_heading = Vector((fork_lamp_target[0] - fork_lamp_x,
                            fork_lamp_target[1] - fork_lamp_z)).normalized()
fork_lamp_forward = (fork_lamp_heading.x, fork_lamp_heading.y)
fork_lamp_side = (fork_lamp_heading.y, -fork_lamp_heading.x)
fork_lamp_width, fork_lamp_depth = .66, .54
fork_lamp_motor_radius = .28


def route_clearance(segments):
    dx, dz = fork_lamp_forward
    sx, sz = fork_lamp_side
    best = None
    for (x0, z0), (x1, z1) in zip(segments, segments[1:]):
        vx, vz = x1 - x0, z1 - z0
        length_squared = vx * vx + vz * vz
        t = max(0.0, min(1.0, ((fork_lamp_x - x0) * vx + (fork_lamp_z - z0) * vz) /
                                length_squared))
        px, pz = x0 + t * vx, z0 + t * vz
        distance = math.hypot(fork_lamp_x - px, fork_lamp_z - pz)
        length = math.hypot(vx, vz)
        nx, nz = -vz / length, vx / length
        half_extent = (abs(nx * sx + nz * sz) * fork_lamp_width / 2 +
                       abs(nx * dx + nz * dz) * fork_lamp_depth / 2)
        margin = distance - half_extent - fork_lamp_motor_radius
        if best is None or margin < best[0]:
            best = (margin, distance)
    return best


fork_lamp_npc_spoke = route_clearance(((103.30, 125.20), (114.30, 126.20)))
fork_lamp_slope = math.hypot(ground(fork_lamp_x + .05, fork_lamp_z) -
                             ground(fork_lamp_x - .05, fork_lamp_z),
                             ground(fork_lamp_x, fork_lamp_z + .05) -
                             ground(fork_lamp_x, fork_lamp_z - .05)) / .1
if (fork_lamp_npc_spoke[0] < .15 or
        fork_lamp_slope > .65 or math.hypot(fork_lamp_target[0] - fork_lamp_x,
                                            fork_lamp_target[1] - fork_lamp_z) > 2.2):
    raise RuntimeError("B05 fork lamp fails support, resident-reach or capsule-route clearance admission.")


def fork_lamp_point(u, forward, height):
    return (fork_lamp_x + fork_lamp_side[0] * u + fork_lamp_forward[0] * forward,
            fork_lamp_y + height,
            fork_lamp_z + fork_lamp_side[1] * u + fork_lamp_forward[1] * forward)


def fork_lamp_box(name, u0, u1, f0, f1, y0, y1):
    local_corners = [(u0, f0), (u1, f0), (u1, f1), (u0, f1)]
    lower = [fork_lamp_point(u, f, y0) for u, f in local_corners]
    upper = [fork_lamp_point(u, f, y1) for u, f in local_corners]
    return mesh(name, lower + upper,
                [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
                 (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)],
                fork_lamp_solids, ember)


def fork_lamp_post(name, profile, sides=8, center=(0.0, 0.0)):
    post_x, _, post_z = fork_lamp_point(center[0], center[1], 0.0)
    vertices = [(post_x + radius * math.cos(math.tau * i / sides),
                 fork_lamp_y + height,
                 post_z + radius * math.sin(math.tau * i / sides))
                for height, radius in profile for i in range(sides)]
    faces = [tuple(reversed(range(sides))),
             tuple(range((len(profile) - 1) * sides, len(profile) * sides))]
    for ring in range(len(profile) - 1):
        for i in range(sides):
            a, b = ring * sides + i, ring * sides + (i + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    obj = mesh(name, vertices, faces, fork_lamp_solids, ember)
    editable = bmesh.new()
    editable.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(editable, faces=list(editable.faces))
    editable.to_mesh(obj.data)
    editable.free()
    return obj


fork_lamp_solids = []
# Ground plate is partly buried; the tapered post is a real collidable solid.
fork_lamp_box("B05_fork_lamp_foot", -.12, .12, -.12, .12, -.045, .055)
fork_lamp_post("B05_fork_lamp_post", ((.025, .095), (.14, .072), (1.11, .045)))
fork_lamp_post("B05_fork_lamp_post_collar", ((1.015, .067), (1.11, .048)))
# Wick pan and a rear rain shield; both sides stay open toward the actor-facing mouth.
fork_lamp_box("B05_fork_lamp_pan", -.245, .245, -.205, .205, 1.10, 1.15)
fork_lamp_box("B05_fork_lamp_back_shield", -.245, .245, -.205, -.171, 1.15, 1.85)
fork_lamp_box("B05_fork_lamp_left_cheek", -.258, -.225, -.205, .155, 1.15, 1.51)
fork_lamp_box("B05_fork_lamp_right_cheek", .225, .258, -.205, .155, 1.15, 1.51)
# Narrow open-face rails frame the wick without glass, an emitter mesh, or a front plate.
for side in (-1, 1):
    fork_lamp_box(f"B05_fork_lamp_front_rail_{side}", side * .225 - .018,
                  side * .225 + .018, .138, .174, 1.15, 1.51)
fork_lamp_box("B05_fork_lamp_front_sill", -.23, .23, .145, .177, 1.15, 1.19)
# One sloping, thick weather hood covers the wick and projects beyond the open front.
hood_local = [(-.30, -.225, 1.88), (.30, -.225, 1.88),
              (.32, .245, 1.75), (-.32, .245, 1.75)]
hood_top = [fork_lamp_point(u, f, y) for u, f, y in hood_local]
hood_bottom = [fork_lamp_point(u, f, y - .035) for u, f, y in hood_local]
hood_mesh = mesh("B05_fork_lamp_weather_hood", hood_bottom + hood_top,
     [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
      (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)], fork_lamp_solids, ember)
hood_editable = bmesh.new()
hood_editable.from_mesh(hood_mesh.data)
bmesh.ops.recalc_face_normals(hood_editable, faces=list(hood_editable.faces))
hood_editable.to_mesh(hood_mesh.data)
hood_editable.free()
fork_lamp_box("B05_fork_lamp_drip_edge", -.33, .33, .227, .267, 1.72, 1.76)
# The wick cup is a dark holder only; the generated Flame and Light are fixed anchors.
fork_lamp_post("B05_fork_lamp_wick_cup", ((1.18, .044), (1.265, .020)),
               sides=6, center=(0, .12))
fork_lamp_post("B05_fork_lamp_wick", ((1.245, .010), (1.37, .010)), sides=6,
               center=(0, .12))
fork_lamp_flame = fork_lamp_point(0, .12, 1.39)
fork_lamp_light = fork_lamp_point(0, .15, 1.52)
fork_lamp_post_foot = (fork_lamp_x, fork_lamp_y, fork_lamp_z)
fork_lamp_triangles = sum(len(face.vertices) - 2 for obj in fork_lamp_solids
                          for face in obj.data.polygons)
if fork_lamp_triangles >= 240:
    raise RuntimeError(f"Fork lamp has {fork_lamp_triangles} solid triangles; budget is below 240.")
fork_lamp_hood_under_at_flame = 1.88 + (1.75 - 1.88) * ((.12 + .225) / (.245 + .225)) - .035
fork_lamp_flame_top = 1.39 + .22 + .06
if fork_lamp_hood_under_at_flame - fork_lamp_flame_top < .05:
    raise RuntimeError("Fork lamp hood does not clear the preset flame height/radius.")
solids.extend(fork_lamp_solids)


def bvh_from_objects(objects):
    points, faces = [], []
    for obj in objects:
        if obj.type != "MESH":
            continue
        offset = len(points)
        obj.data.calc_loop_triangles()
        points.extend(obj.matrix_world @ vertex.co for vertex in obj.data.vertices)
        faces.extend([offset + index for index in tri.vertices] for tri in obj.data.loop_triangles)
    return BVHTree.FromPolygons(points, faces, all_triangles=True)


def direct_ray_clear(tree, source, target):
    origin = Vector((source[0], -source[2], source[1]))
    endpoint = Vector((target[0], -target[2], target[1]))
    delta = endpoint - origin
    return delta.length > .01 and tree.ray_cast(origin, delta.normalized(), delta.length - .005)[0] is None


fork_lamp_source = list(fork_lamp_light)
fork_lamp_face_samples = [
    (fork_lamp_target[0] - .10, -10.18, fork_lamp_target[1]),
    (fork_lamp_target[0], -10.18, fork_lamp_target[1]),
    (fork_lamp_target[0] + .10, -10.18, fork_lamp_target[1]),
    (fork_lamp_target[0], -10.02, fork_lamp_target[1]),
    (fork_lamp_target[0], -10.30, fork_lamp_target[1]),
]
fork_lamp_hood_tree = bvh_from_objects(fork_lamp_solids)
fork_lamp_town_tree = bvh_from_objects([obj for obj in solids if obj not in fork_lamp_solids])
fork_lamp_shadow_witness = [
    {"target": target,
     "hoodClear": direct_ray_clear(fork_lamp_hood_tree, fork_lamp_source, target),
     "townAndTerrainClear": direct_ray_clear(fork_lamp_town_tree, fork_lamp_source, target) and
                            direct_ray_clear(terrain, fork_lamp_source, target)}
    for target in fork_lamp_face_samples
]
if not all(witness["hoodClear"] and witness["townAndTerrainClear"]
           for witness in fork_lamp_shadow_witness):
    raise RuntimeError(f"Fork lamp source has a direct shadow blocker: {fork_lamp_shadow_witness}")


# The mapped church path crosses a steep diagonal terrain band around the
# southwest corner of B02. A stone
# approach bridges the two faces of the cut, with buried toes at both ends.
church_sections = [
    (62.30, 65.20, None), (62.70, 67.20, -3.08), (63.00, 68.70, -2.84),
    (63.40, 70.70, -2.77), (63.90, 73.20, -3.40), (64.30, 75.20, -3.90),
    (66.50, 75.20, -3.48), (69.60, 75.20, -2.58), (71.50, 75.20, -2.54),
    (73.50, 75.20, None),
]
church_half_width = 1.1


def stone_approach(name, sections, half_width):
    centres = [Vector((x, z)) for x, z, _ in sections]
    normals = []
    for a, b in zip(centres, centres[1:]):
        dx, dz = (b - a).normalized()
        normals.append(Vector((-dz, dx)))
    top, base = [], []
    for index, (centre, (_, _, height)) in enumerate(zip(centres, sections)):
        if index == 0:
            normal = normals[0]
        elif index == len(sections) - 1:
            normal = normals[-1]
        else:
            incoming, outgoing = normals[index - 1:index + 1]
            normal = (incoming + outgoing) / (1 + incoming.dot(outgoing))
        offset = normal * half_width
        edges = (centre - offset, centre + offset)
        heights = [ground(*point) for point in edges]
        if height is None:
            height = min(heights + [ground(*centre)]) - .035
        for (x, z), terrain_y in zip(edges, heights):
            top.append((x, height, z))
            base.append((x, min(height, terrain_y) - .20, z))

    deck, walls = [], []
    offset = len(top)
    for index in range(len(sections) - 1):
        left, right = 2 * index, 2 * index + 1
        next_left, next_right = left + 2, right + 2
        deck.append((left, right, next_right, next_left))
        walls.extend(((next_left, left, left + offset, next_left + offset),
                      (right, next_right, next_right + offset, right + offset)))
    walls.extend(((0, 1, 1 + offset, offset),
                  (offset - 1, offset - 2, 2 * offset - 2, 2 * offset - 1)))
    mesh(name + "_deck", top, deck, walkable)
    mesh(name + "_retaining", top + base, [tuple(reversed(face)) for face in walls], solids)


stone_approach("B02_church_approach", church_sections, church_half_width)

# One continuous stone causeway follows the south edge of the mill and
# ends under the original river surface. The first section starts below
# the grade, with retaining sides reaching the bed on the exposed slope.
river_half_width = .95
river_sections = [
    (115.20, 127.80, None), (116.45, 124.57, -10.73), (118.90, 122.75, -12.24),
    (121.02, 122.55, -13.365), (124.40, 122.55, -15.51),
    (129.40, 122.55, -18.64), (134.70, 122.55, -21.81),
    (137.50, 122.55, -23.48), (138.55, 122.55, -23.48),
]
stone_approach("B05_river_landing", river_sections, river_half_width)
river_stations = [(113.68, 127.86), (114.94, 128.44)]
river_stations += [(x, z) for x, z, _ in river_sections[:-1]] + [(138.11, 122.55)]
fork_lamp_shore_path = route_clearance(river_stations)
if fork_lamp_shore_path[0] < .15:
    raise RuntimeError("B05 fork lamp intrudes into the river landing's motor capsule corridor.")
river_dry_mark = (136.55, 122.55)
river_view = (135.80, 122.55)
for z in (121.68, 123.42):
    post_bottom = min(ground(138.46 + dx, z + dz) for dx in (-.08, .08) for dz in (-.08, .08)) - .10
    box(f"B05_river_landing_post_{z}", 138.38, 138.54, z - .08, z + .08,
        post_bottom, -22.43, oak)
box("B05_river_landing_rail", 138.40, 138.52, 121.60, 123.50, -22.71, -22.53, oak)


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


def shrine_face(name, outline, back, front, material=stone, group=solids):
    size = len(outline)
    mesh("B09_" + name,
         [(shrine_x + x, opening_bottom + y, archive_front + z)
          for z in (back, front) for x, y in outline],
         [tuple(reversed(range(size))), tuple(range(size, size * 2))] +
         [(i, (i + 1) % size, (i + 1) % size + size, i + size) for i in range(size)], group, material)


cover_stone = opaque_finish("HordeWaysideCoverStone", (.12, .084, .060), .91)
shrine_face("lower_stone", [(-.61, .03), (.50, .03), (.62, .14), (.60, .49), (.47, .60),
                           (-.50, .60), (-.61, .48)], -.19, -.045, cover_stone, group=archive_cover)
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
# Move the editable object's origin to its real hinge without changing world placement.
cover = archive_cover[0]
pivot = Vector((shrine_hinge[0], -shrine_hinge[2], shrine_hinge[1]))
for vertex in cover.data.vertices:
    vertex.co -= pivot
cover.location = pivot
shrine_view = [shrine_x, ground(shrine_x, archive_front + 2.2), archive_front + 2.2]

# A closed ledger resting on the sill behind the hinged stone.
register_leather = opaque_finish("HordeRegisterLeather", (.19, .065, .029), .78)
register_pages = opaque_finish("HordeRegisterPaper", (.61, .47, .29), .84)
register_z = archive_front - .34
register_x = shrine_x + .15
for name, width, depth, low, high, material in (
    ("lower_binding", .49, .30, .001, .026, register_leather),
    ("page_block", .45, .27, .026, .103, register_pages),
    ("upper_binding", .49, .30, .103, .128, register_leather),
):
    box("B09_register_" + name, register_x - width / 2, register_x + width / 2,
        register_z - depth / 2, register_z + depth / 2, opening_bottom + low,
        opening_bottom + high, material, group=archive_register)
box("B09_register_spine", register_x - .263, register_x - .224, register_z - .151,
    register_z + .151, opening_bottom + .022, opening_bottom + .107, register_leather,
    group=archive_register)
register_mark = [register_x, opening_bottom + .12, register_z]
solids.extend(door_frame)


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
for name, mark in (("Hinge rotation origin", shrine_hinge), ("Player inspection mark", shrine_view),
                   ("Road register", register_mark)):
    marker = bpy.data.objects.new(name, None)
    marker.empty_display_type = "PLAIN_AXES"
    marker.empty_display_size = .25
    marker.location = (mark[0], -mark[2], mark[1])
    archive_blocking.objects.link(marker)
fork_lamp_blocking = bpy.data.collections.new("B05 - River fork lamp anchors")
bpy.context.scene.collection.children.link(fork_lamp_blocking)
for name, mark in (("River fork lamp post foot", fork_lamp_post_foot),
                   ("River fork lamp Flame", fork_lamp_flame),
                   ("River fork lamp Light", fork_lamp_light)):
    marker = bpy.data.objects.new(name, None)
    marker.empty_display_type = "PLAIN_AXES"
    marker.empty_display_size = .12
    marker.location = (mark[0], -mark[2], mark[1])
    fork_lamp_blocking.objects.link(marker)
bpy.data.orphans_purge(do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "bellwether-tavern-v1.blend"), compress=True)
shell_mesh = joined_copy(solids, "BellwetherSolid")
floor_mesh = joined_copy(walkable, "BellwetherWalkable")
shell_mesh.select_set(True)
path = RUNTIME / "bellwether-tavern-v1.glb"
bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)
base_runtime_sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
# The live runtime owner already contains the separately authored millworks
# and crossing. Append only the new lamp mesh to its matching Hearth-char
# primitive; do not replace its old accessors or regenerate downstream work.
lamp_source_path = RUNTIME / ".fork-lamp-support.glb"
lamp_source_mesh = joined_copy(fork_lamp_solids, "ForkLampSupport")
bpy.ops.object.select_all(action="DESELECT")
lamp_source_mesh.select_set(True)
bpy.context.view_layer.objects.active = lamp_source_mesh
bpy.ops.export_scene.gltf(filepath=str(lamp_source_path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)
live_town = INPUT_TOWN
if not live_town.is_file():
    raise RuntimeError(f"Missing exact pre-lamp runtime town input: {live_town}")
live_town_sha256 = hashlib.sha256(live_town.read_bytes()).hexdigest()
layered_town_path = RUNTIME / ".fork-lamp-layered.glb"
mill_works_tools = runpy.run_path(str(ROOT / "tools/author-bellwether-mill-works.py"))
installed_town_hashes = {FORK_LAMP_LAYERED_TOWN_SHA256}
for layer in ("bellwether-relic-stall-v1", "bellwether-road-cache-v1", "bellwether-well-v1",
              "bellwether-garden-v1", "bellwether-privy-v1", "bellwether-privy-rag-v1",
              "bellwether-graveyard-v1", "bellwether-smith-v1", "bellwether-grate-v1",
              "bellwether-tally-v1", "bellwether-stock-v1", "bellwether-mill-lamp-v1", "bellwether-paths-v1"):
    receipt_path = WORLD / "source" / layer / "authoring-receipt.json"
    if receipt_path.is_file():
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        if receipt["originalTownSha256"] in installed_town_hashes:
            installed_town_hashes.add(receipt["runtimeTownSha256"])
if live_town_sha256 == FORK_LAMP_SOURCE_TOWN_SHA256:
    layered_source_town_sha256 = live_town_sha256
    live_doc, _ = mill_works_tools["read_glb"](live_town)
    input_runtime_triangles = sum(live_doc["accessors"][p["indices"]]["count"] // 3
                                  for mesh_record in live_doc["meshes"]
                                  for p in mesh_record["primitives"])
    layered_extra_triangles, layered_vertex_count, _ = mill_works_tools["join_town"](
        live_town, lamp_source_path, layered_town_path)
    if layered_extra_triangles != fork_lamp_triangles:
        raise RuntimeError("Live town append did not retain the exact lamp triangle payload.")
    layered_runtime_triangles = input_runtime_triangles + layered_extra_triangles
    path.unlink()
    shutil.move(str(layered_town_path), str(path))
elif live_town_sha256 in installed_town_hashes:
    layered_source_town_sha256 = FORK_LAMP_SOURCE_TOWN_SHA256
    layered_doc, _ = mill_works_tools["read_glb"](live_town)
    layered_runtime_triangles = sum(layered_doc["accessors"][p["indices"]]["count"] // 3
                                    for mesh_record in layered_doc["meshes"]
                                    for p in mesh_record["primitives"])
    layered_vertex_count = sum(layered_doc["accessors"][p["attributes"]["POSITION"]]["count"]
                               for mesh_record in layered_doc["meshes"]
                               for p in mesh_record["primitives"])
    shutil.copyfile(live_town, path)
else:
    raise RuntimeError(f"Fork lamp needs an exact admitted town owner; got {live_town_sha256}.")
lamp_source_path.unlink()
pane_mesh = joined_copy(glazing, "BellwetherGlazing")
pane_path = RUNTIME / "bellwether-window.glb"
bpy.ops.export_scene.gltf(filepath=str(pane_path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)
cover_mesh = joined_copy(archive_cover, "WaysideArchiveCover")
cover_mesh.location = (0, 0, 0)
cover_path = RUNTIME / "wayside-archive-cover.glb"
bpy.ops.export_scene.gltf(filepath=str(cover_path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)

register_mesh = joined_copy(archive_register, "WaysideRoadRegister")
register_path = RUNTIME / "wayside-road-register.glb"
bpy.ops.export_scene.gltf(filepath=str(register_path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)
door_mesh = joined_copy(door_leaf, "BellwetherSouthDoor")
door_mesh.parent = None
door_path = RUNTIME / "bellwether-south-door.glb"
bpy.ops.export_scene.gltf(filepath=str(door_path), export_format="GLB", use_selection=True,
                          export_apply=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_animations=False)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


solid_triangles = sum(len(face.vertices) - 2 for face in shell_mesh.data.polygons)
walkable_triangles = sum(len(face.vertices) - 2 for face in floor_mesh.data.polygons)
current_manifest = WORLD / "runtime/bellwether-tavern-v1/asset.manifest.json"
manifest = json.loads((current_manifest if current_manifest.is_file() else
                       WORLD / "runtime/bellwether-shell-v4/asset.manifest.json").read_text())
manifest["asset"] = "bellwether-tavern-v1"
manifest["budgets"].update(maxVertices=max(manifest["budgets"].get("maxVertices", 9000),
                                           layered_vertex_count),
                           maxIndices=max(manifest["budgets"].get("maxIndices", 0),
                                          layered_runtime_triangles * 3),
                           maxPrimitives=5, maxMaterials=4)
manifest["lods"][0]["maxTriangles"] = max(manifest["lods"][0].get("maxTriangles", 0),
                                           layered_runtime_triangles)
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
cover_triangles = len(cover_mesh.data.polygons)
cover_manifest = dict(manifest, asset="wayside-archive-cover", materialOverrides=[])
cover_manifest["budgets"] = dict(manifest["budgets"], maxVertices=72, maxIndices=cover_triangles * 3,
                                 maxPrimitives=1, maxMaterials=1)
cover_manifest["lods"] = [{"name": "lod0", "maxTriangles": cover_triangles}]
(RUNTIME / "wayside-archive-cover.manifest.json").write_text(json.dumps(cover_manifest, indent=2) + "\n",
                                                             encoding="utf-8", newline="\n")
register_triangles = len(register_mesh.data.polygons)
register_manifest = dict(manifest, asset="wayside-road-register", materialOverrides=[])
register_manifest["budgets"] = dict(manifest["budgets"], maxVertices=112, maxIndices=register_triangles * 3,
                                    maxPrimitives=2, maxMaterials=2)
register_manifest["lods"] = [{"name": "lod0", "maxTriangles": register_triangles}]
(RUNTIME / "wayside-road-register.manifest.json").write_text(json.dumps(register_manifest, indent=2) + "\n",
                                                             encoding="utf-8", newline="\n")
door_triangles = len(door_mesh.data.polygons)
door_manifest = dict(manifest, asset="bellwether-south-door", materialOverrides=[])
door_manifest["budgets"] = dict(manifest["budgets"], maxVertices=1100, maxIndices=door_triangles * 3,
                                maxPrimitives=2, maxMaterials=2)
door_manifest["lods"] = [{"name": "lod0", "maxTriangles": door_triangles}]
(RUNTIME / "south-door.manifest.json").write_text(json.dumps(door_manifest, indent=2) + "\n",
                                                   encoding="utf-8", newline="\n")
receipt = dict(registerSha256=digest(register_path), registerTriangles=register_triangles, registerMark=register_mark,
               coverSha256=digest(cover_path), coverTriangles=cover_triangles, sourceMasterSha256=digest(MASTER), terrainSha256=digest(TERRAIN),
               layoutSha256=parent["planSha256"], runtimeSha256=digest(path),
               baseRuntimeSha256=base_runtime_sha256,
               layeredRuntime=dict(inputTownSha256=layered_source_town_sha256,
                                   appendedLampTriangles=fork_lamp_triangles,
                                   runtimeTriangles=layered_runtime_triangles,
                                   runtimeVertices=layered_vertex_count),
               solidTriangles=solid_triangles, walkableTriangles=walkable_triangles,
               windowSha256=digest(pane_path), windowTriangles=pane_triangles,
               windowBounds=[pane_min, pane_max],
               southDoor=[door_x, floor, south], clearDoorWidth=2.2 - 2 * door_frame_width,
               clearDoorHeight=2.65 - .068, southDoorHinge=door_hinge,
               southDoorOpenDegrees=door_open_degrees, southDoorLeafWidth=door_width,
               southDoorSha256=digest(door_path), southDoorTriangles=door_triangles,
               rampToe=[door_x, toe_y, toe_z], rampWidth=3, highestGroundUnderFloor=highest_inside,
               hearthMouth=[hearth_front, floor, (hearth_north + hearth_south) / 2],
               chairSeat=seat_mark, chairSitRoot=sit_root, chairApproach=approach_mark, chairTurn=turn_mark,
               chairSeatBoundsXZ=[chair_board_x - chair_depth / 2, chair_board_x + chair_depth / 2,
                                  chair_z - .296, chair_z + .296],
               tableTop=[table_x, floor + table_top, table_z], conversation=talk_mark,
               tavernCandleFlame=candle_flame, tavernCandleLight=candle_light,
               riverForkLamp=dict(postFoot=fork_lamp_post_foot, flame=fork_lamp_flame,
                                  light=fork_lamp_light, supportSlope=fork_lamp_slope,
                                  width=fork_lamp_width, depth=fork_lamp_depth,
                                  solidTriangles=fork_lamp_triangles,
                                  npcSpokeCenterDistance=fork_lamp_npc_spoke[1],
                                  npcSpokeMotorMargin=fork_lamp_npc_spoke[0],
                                  shoreCenterDistance=fork_lamp_shore_path[1],
                                  shoreMotorMargin=fork_lamp_shore_path[0],
                                  headroomAbovePresetFlame=fork_lamp_hood_under_at_flame - fork_lamp_flame_top,
                                  shadowWitness=fork_lamp_shadow_witness),
               churchWaymarker=church_waymarker,
               churchApproach=dict(width=2*church_half_width, sections=church_sections),
               millWaymarkers=mill_waymarkers, millWestDoor=mill_door, millWorkMark=mill_work_mark,
               millComparison=mill_comparison, millBearingBin=mill_bearing_bin,
               riverLanding=dict(width=2*river_half_width, sections=river_sections, stations=river_stations,
                                 dryMark=river_dry_mark, view=river_view),
               millLanding=dict(west=landing_west, east=mill_west, north=landing_north,
                                south=landing_south, height=landing_top),
               waysideArchive=[shrine_mechanism, shrine_hinge, shrine_view],
               keeperRoute=[keeper_door, keeper_aisle, keeper_hearth],
               keeperDoorYaw=keeper_door_yaw, keeperHearthYaw=keeper_hearth_yaw)
(OUTPUT / "authoring-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")

stage = ["// Generated by tools/author-bellwether-tavern.py. Engine coordinates in metres.",
         "#pragma once", "#include <array>", "namespace horde::gameplay::bellwether {", ""]
for name, mark in (("ChairSeat", seat_mark), ("ChairSitRoot", sit_root),
                   ("ChairApproach", approach_mark), ("ChairTurn", turn_mark),
                   ("TableTop", receipt["tableTop"]), ("Conversation", talk_mark),
                   ("RoomMin", (west + wall_depth, floor, north + wall_depth)),
                   ("RoomMax", (east - wall_depth, floor, south - wall_depth)),
                   ("SouthDoorHinge", door_hinge),
                   ("TavernCandleFlame", candle_flame), ("TavernCandleLight", candle_light),
                   ("RiverForkLampPostFoot", fork_lamp_post_foot),
                   ("RiverForkLampFlame", fork_lamp_flame), ("RiverForkLampLight", fork_lamp_light),
                   ("ChurchWaymarker", church_waymarker),
                   ("MillWestDoor", mill_door), ("MillWorkMark", mill_work_mark),
                   ("MillComparison", mill_comparison), ("MillBearingBin", mill_bearing_bin),
                   ("WaysideMechanism", shrine_mechanism), ("WaysideHinge", shrine_hinge),
                   ("WaysideInspection", shrine_view), ("WaysideRoadRegister", register_mark),
                   ("KeeperDoor", keeper_door), ("KeeperAisle", keeper_aisle), ("KeeperHearth", keeper_hearth)):
    position = ", ".join(f"{coordinate:.6f}f" for coordinate in mark)
    stage.append(f"inline constexpr std::array<float, 3> k{name}{{{{{position}}}}};")
stage.append(f"inline constexpr std::array<std::array<float, 2>, {len(river_stations)}> kRiverLandingStations{{{{")
stage.extend(f"    {{{x:.6f}f, {z:.6f}f}}," for x, z in river_stations)
stage.append("}};")
for name, (x, z) in (("RiverLandingDryMark", river_dry_mark), ("RiverLandingView", river_view)):
    stage.append(f"inline constexpr std::array<float, 2> k{name}{{{x:.6f}f, {z:.6f}f}};")
stage += ["", f"inline constexpr unsigned kSolidTriangles={solid_triangles};",
          f"inline constexpr unsigned kGroundTriangles={walkable_triangles};",
          f"inline constexpr unsigned kSouthDoorTriangles={door_triangles};",
          f"inline constexpr float kSouthDoorOpenRadians={math.radians(door_open_degrees):.9f}f;",
          f"inline constexpr float kSouthDoorSwingRadius={door_width + .01:.3f}f;",
          "inline constexpr float kChairFacingRadians=1.570796327f;",
          f"inline constexpr float kKeeperDoorFacingRadians={keeper_door_yaw:.9f}f;",
          f"inline constexpr float kKeeperHearthFacingRadians={keeper_hearth_yaw:.9f}f;", "}", ""]
(OUTPUT / "staging.generated.h").write_text("\n".join(stage), encoding="utf-8", newline="\n")
print(json.dumps(receipt))
