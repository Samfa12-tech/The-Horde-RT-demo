"""Block Kit's protected sit on the admitted rig and the exact B01 chair.

blender --background --python-exit-code 1 --python tools/author-kit-hearth-sit.py -- OUTPUT
This writes a new editable study with sampled contact evidence. It does not
export a game clip or alter the preserved Kit or tavern masters.
"""
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Vector


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path(sys.argv[sys.argv.index("--") + 1]).resolve()
SOURCE = ROOT / "assets/models/characters/source/kit-warden-v1/briarhold-warden-1k.glb"
TAVERN = ROOT / "assets/models/world/source/bellwether-tavern-v1"
MASTER = TAVERN / "bellwether-tavern-v1.blend"
FPS, LAST = 60, 83
SIDES = ("Left", "Right")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


if digest(SOURCE) != "47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e":
    raise RuntimeError("The approved Kit source has changed.")
staging = json.loads((TAVERN / "authoring-receipt.json").read_text())
seat_x, seat_y, seat_z = staging["chairSeat"]
back_x, front_x, north_z, south_z = staging["chairSeatBoundsXZ"]
floor = staging["chairApproach"][1]
seat = Vector((seat_x, -seat_z, seat_y))
OUTPUT.mkdir(parents=True, exist_ok=False)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
scene = bpy.context.scene
scene.name = "Bellwether B01 - Kit protected sit"
scene.render.fps = FPS
scene.frame_start, scene.frame_end = 1, LAST
armature = next(obj for obj in scene.objects if obj.type == "ARMATURE")
skin = bpy.data.objects["briarhold-warden"]
rig_objects = list(scene.objects)
with bpy.data.libraries.load(str(MASTER)) as (available, selected):
    selected.objects = [name for name in available.objects
                        if name.startswith(("B01_chair_", "B01_table_", "B01_hearth_"))
                        or name == "B01_room_floor"]
for obj in selected.objects:
    scene.collection.objects.link(obj)
    obj.color = (.34, .22, .13, 1) if obj.name.startswith(("B01_chair_", "B01_table_")) else (.32, .30, .24, 1)

root = bpy.data.objects.new("STUDY ONLY - seated placement", None)
scene.collection.objects.link(root)
for obj in rig_objects:
    if obj.parent is None:
        obj.parent = root
    if obj.name == "Icosphere":
        obj.hide_viewport = obj.hide_render = True
root.location = (seat.x + .46, seat.y, floor)
# +Z authored glTF forward becomes -Y in Blender. The chair faces east.
root.rotation_euler.z = math.pi / 2
skin.color = (.28, .37, .43, 1)

for track in armature.animation_data.nla_tracks:
    track.mute = True
idle = bpy.data.actions["Warden|idle"]
armature.animation_data.action = idle
armature.animation_data.action_slot = idle.slots[0]
scene.frame_set(0)
bpy.context.view_layer.update()
neutral = {bone.name: bone.matrix_basis.copy() for bone in armature.pose.bones}


def world_pose(name):
    return armature.matrix_world @ armature.pose.bones[name].matrix


def set_pose(name, matrix):
    armature.pose.bones[name].matrix = armature.matrix_world.inverted() @ matrix
    bpy.context.view_layer.update()


def skin_points():
    evaluated = skin.evaluated_get(bpy.context.evaluated_depsgraph_get())
    return [evaluated.matrix_world @ point.co for point in evaluated.data.vertices]


def solve_limb(names, target, pole, end_rotation=None):
    upper, lower, end = (world_pose(name) for name in names)
    origin, elbow, tip = (matrix.translation.copy() for matrix in (upper, lower, end))
    first, second = elbow - origin, tip - elbow
    delta = target - origin
    length = min(delta.length, first.length + second.length - .00005)
    if length <= abs(first.length - second.length):
        raise RuntimeError(f"Degenerate target for {names[0]}")
    axis = delta.normalized()
    bend = (pole - axis * pole.dot(axis)).normalized()
    along = (first.length_squared - second.length_squared + length * length) / (2 * length)
    middle = origin + axis * along + bend * math.sqrt(max(0, first.length_squared - along * along))
    reachable = origin + axis * length
    for name, base, position, rotation in (
        (names[0], upper, origin, first.rotation_difference(middle - origin)),
        (names[1], lower, middle, second.rotation_difference(reachable - middle)),
    ):
        matrix = rotation.to_matrix().to_4x4() @ base
        matrix.translation = position
        set_pose(name, matrix)
    if end_rotation is not None:
        matrix = end_rotation.copy()
        matrix.translation = reachable
        set_pose(names[2], matrix)
    return (reachable - target).length


stand = skin_points()
boots = {}
feet = {}
for side in SIDES:
    names = tuple(side + joint for joint in ("UpLeg", "Leg", "Foot", "ToeBase"))
    groups = {skin.vertex_groups[name].index for name in names}
    ids = [vertex.index for vertex in skin.data.vertices if vertex.groups and
           all(item.group in groups for item in vertex.groups if item.weight > .00001)
           and stand[vertex.index].z < floor + .25]
    bottom = min(stand[index].z for index in ids)
    boots[side] = [index for index in ids if stand[index].z <= bottom + .005]
    feet[side] = world_pose(side + "Foot")
    feet[side].translation.z += floor - bottom + .002
    # Stagger the final stance established by the preceding turn. Both boots
    # stay planted throughout the lowering motion.
    if side == "Left":
        feet[side].translation.x += .065

pelvis_groups = {skin.vertex_groups[name].index for name in ("Hips", "LeftUpLeg", "RightUpLeg")}
pelvis_ids = [v.index for v in skin.data.vertices
              if sum(g.weight for g in v.groups if g.group in pelvis_groups) > .8
              and floor + .69 < stand[v.index].z < floor + 1.10]
armature.animation_data.action = None
for bone in armature.pose.bones:
    bone.rotation_mode = "QUATERNION"


def ease(value):
    t = min(1, max(0, value))
    return t * t * (3 - 2 * t)


def pose(progress, drop):
    for bone in armature.pose.bones:
        bone.matrix_basis = neutral[bone.name]
    bpy.context.view_layer.update()
    lower = ease((progress - .10) / .75)
    bow = math.sin(math.pi * ease(progress / .88)) ** 2
    hips = world_pose("Hips")
    hips.translation += Vector((-.34 * lower + .042 * bow, -.022 * lower, -drop * lower))
    set_pose("Hips", hips)
    spine = world_pose("Spine")
    lean = Matrix.Rotation(math.radians(6 * lower + 14 * bow), 4, "Y") @ spine
    lean.translation = spine.translation
    set_pose("Spine", lean)
    stretch = 0
    for side in SIDES:
        foot = feet[side].copy()
        stretch = max(stretch, solve_limb(tuple(side + j for j in ("UpLeg", "Leg", "Foot")),
                                         foot.translation, Vector((1, 0, 0)), foot))
    for side in SIDES:
        if progress <= .03:
            continue
        wrist = world_pose(side + "Hand").translation
        thigh = world_pose(side + "UpLeg").translation
        knee = world_pose(side + "Leg").translation
        rest = thigh.lerp(knee, .76)
        rest.y += -.036 if side == "Left" else .036
        rest.z += .108
        solve_limb(tuple(side + j for j in ("Arm", "ForeArm", "Hand")),
                   wrist.lerp(rest, ease((progress - .03) / .63)),
                   Vector((-.42, -1 if side == "Left" else 1, -.6)))
    return stretch


def seated_height(points):
    return min((p.z for index in pelvis_ids for p in (points[index],)
                if back_x < p.x < front_x and -south_z < p.y < -north_z), default=math.inf)


drop = .27
for _ in range(6):
    pose(1, drop)
    drop += seated_height(skin_points()) - seat.z - .004
if not .23 < drop < .34:
    raise RuntimeError(f"Unexpected sit depth: {drop:.3f} m")

take = bpy.data.actions.new("Kit - B01 protected sit - authoring study")
take.use_fake_user = True
armature.animation_data.action = take
measurements = []
rest_points = None
for frame in range(1, LAST + 1):
    scene.frame_set(frame)
    progress = (frame - 1) / (LAST - 1)
    stretch = pose(progress, drop)
    for bone in armature.pose.bones:
        bone.keyframe_insert("location", frame=frame)
        bone.keyframe_insert("rotation_quaternion", frame=frame)
        bone.keyframe_insert("scale", frame=frame)
    points = skin_points()
    if frame == 1:
        rest_points = points
    sole = {}
    for side, ids in boots.items():
        lowest = min(points[i].z for i in ids) - floor
        movement = max(math.hypot(points[i].x - rest_points[i].x, points[i].y - rest_points[i].y) for i in ids)
        sole[side] = {"minimumHeightMetres": lowest, "maxTravelMetres": movement}
    height = seated_height(points)
    clearance = height - seat.z if math.isfinite(height) else None
    measurements.append(dict(frame=frame, maxLegTargetErrorMetres=stretch, sole=sole,
                             sampledSeatClearanceMetres=clearance))

clearance = seated_height(skin_points()) - seat.z
record = dict(kitSourceSha256=digest(SOURCE), tavernMasterSha256=digest(MASTER),
              furnitureRuntimeSha256=staging["runtimeSha256"],
              chairSeat=staging["chairSeat"], fps=FPS, finalFrame=LAST, seatedClearanceMetres=clearance,
              hipDropMetres=drop, samples=measurements)
(OUTPUT / "contact.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
if abs(clearance - .004) > .008:
    raise RuntimeError(f"Seated hips do not meet the authored chair: {clearance:.3f} m")
if min(row["sampledSeatClearanceMetres"] for row in measurements
       if row["sampledSeatClearanceMetres"] is not None) < -.006:
    raise RuntimeError("The selected hip/thigh mesh penetrates the chair during lowering.")
if any(row["maxLegTargetErrorMetres"] > .005 or
       any(witness["minimumHeightMetres"] < -.005 or witness["maxTravelMetres"] > .015
           for witness in row["sole"].values()) for row in measurements):
    raise RuntimeError("A planted boot exceeded the authoring contact bounds.")

camera_data = bpy.data.cameras.new("Seat review")
camera = bpy.data.objects.new("Seat review", camera_data)
scene.collection.objects.link(camera)
target = seat + Vector((.03, -.01, .51))
camera.location = target + Vector((3.06, 2.18, 1.00))
camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
camera_data.type = "ORTHO"
camera_data.ortho_scale = 2.72
scene.camera = camera
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "OBJECT"
scene.display.shading.show_cavity = True
scene.render.resolution_x, scene.render.resolution_y = 840, 720
scene.render.resolution_percentage = 100
scene.frame_set(LAST)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "kit-hearth-sit.blend"), compress=True)
print(f"B01 seat contact: {clearance:.5f} m, hip drop {drop:.4f} m")
