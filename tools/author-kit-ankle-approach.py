"""Author Kit's road injury and protective walk on the preserved Warden rig.

blender --background --disable-autoexec --python-exit-code 1 \
    --python tools/author-kit-ankle-approach.py -- OUTPUT
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
SOURCE_HASH = "47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e"
FPS = 60
STRIDE = .76
CATCH_END = 97
WALK_END = 53
FORWARD = Vector((0, -1, 0))
SIDES = ("Left", "Right")
# Healthy left leg carries for longer. Offsets are fractions of one full stride.
GAIT = {"Left": (.48, .73, .260), "Right": (0, .39, .145)}


def ease(t):
    t = min(1, max(0, t))
    return t * t * (3 - 2 * t)


def rise_and_fall(t, start, peak, end):
    return ease((t - start) / (peak - start)) * (1 - ease((t - peak) / (end - peak)))


def rotate(matrix, degrees, axis):
    result = Matrix.Rotation(math.radians(degrees), 4, axis) @ matrix
    result.translation = matrix.translation
    return result


if hashlib.sha256(SOURCE.read_bytes()).hexdigest() != SOURCE_HASH:
    raise RuntimeError("The approved Kit source has changed.")
OUTPUT.mkdir(parents=True, exist_ok=False)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
scene = bpy.context.scene
scene.name = "Bellwether road - Kit ankle and protective gait"
scene.render.fps = FPS
arm = next(obj for obj in scene.objects if obj.type == "ARMATURE")
skin = scene.objects["briarhold-warden"]
skin.color = (.30, .38, .45, 1)
for obj in scene.objects:
    if obj.name == "Icosphere":
        obj.hide_viewport = obj.hide_render = True

for track in arm.animation_data.nla_tracks:
    track.mute = True
idle = bpy.data.actions["Warden|idle"]
arm.animation_data.action = idle
arm.animation_data.action_slot = idle.slots[0]
scene.frame_set(0)
bpy.context.view_layer.update()
neutral = {bone.name: bone.matrix_basis.copy() for bone in arm.pose.bones}


def pose(name):
    return arm.matrix_world @ arm.pose.bones[name].matrix


def set_pose(name, matrix):
    arm.pose.bones[name].matrix = arm.matrix_world.inverted() @ matrix
    bpy.context.view_layer.update()


def skin_points():
    mesh = skin.evaluated_get(bpy.context.evaluated_depsgraph_get())
    return [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]


def solve_limb(names, target, pole, end_rotation=None):
    upper, lower, end = (pose(name) for name in names)
    origin, joint, tip = (matrix.translation.copy() for matrix in (upper, lower, end))
    first, second = joint - origin, tip - joint
    delta = target - origin
    length = min(delta.length, first.length + second.length - .00005)
    axis = delta.normalized()
    bend = (pole - axis * pole.dot(axis)).normalized()
    along = (first.length_squared - second.length_squared + length * length) / (2 * length)
    middle = origin + axis * along + bend * math.sqrt(max(0, first.length_squared - along * along))
    reachable = origin + axis * length
    for name, matrix, point, rotation in (
        (names[0], upper, origin, first.rotation_difference(middle - origin)),
        (names[1], lower, middle, second.rotation_difference(reachable - middle)),
    ):
        tilted = rotation.to_matrix().to_4x4() @ matrix
        tilted.translation = point
        set_pose(name, tilted)
    if end_rotation is not None:
        foot = end_rotation.copy()
        foot.translation = reachable
        set_pose(names[2], foot)
    return (reachable - target).length


stand = skin_points()
feet = {side: pose(side + "Foot") for side in SIDES}
sole = {}
sole_offsets = {}
for side, foot in feet.items():
    group = skin.vertex_groups[side + "Foot"].index
    toe = skin.vertex_groups[side + "ToeBase"].index
    ids = [v.index for v in skin.data.vertices if v.groups and
           sum(g.weight for g in v.groups if g.group in (group, toe)) > .95]
    bottom = min(stand[i].z for i in ids)
    sole[side] = [i for i in ids if stand[i].z <= bottom + .015]
    sole_offsets[side] = [stand[i] - foot.translation for i in sole[side]]
    foot.translation.z -= bottom

# Keep a common model floor even though the source idle's boots differ slightly.
FLOOR = min(p.z for p in stand)
for foot in feet.values():
    foot.translation.z += FLOOR


def plant(side, forward, lift, roll=0, pitch=0):
    foot = rotate(rotate(feet[side], pitch, "X"), roll, "Y")
    foot.translation += FORWARD * forward
    if roll or pitch:
        # Preserve the outside edge of the contacting boot during the roll.
        offsets = sole_offsets[side]
        original = feet[side].to_quaternion()
        change = foot.to_quaternion() @ original.inverted()
        foot.translation.z += min(p.z for p in offsets) - min((change @ p).z for p in offsets)
    foot.translation.z += lift + .003
    return solve_limb(tuple(side + j for j in ("UpLeg", "Leg", "Foot")),
                      foot.translation, FORWARD, foot)


def upper_body(forward, lateral, dip, lean, tilt, glance, left_arm, right_arm):
    for bone in arm.pose.bones:
        bone.matrix_basis = neutral[bone.name]
    bpy.context.view_layer.update()
    hips = pose("Hips")
    hips.translation += Vector((lateral, -forward, -dip))
    set_pose("Hips", hips)
    set_pose("Spine", rotate(rotate(pose("Spine"), lean, "X"), tilt, "Y"))
    set_pose("Head", rotate(pose("Head"), glance, "X"))
    for side, offset in (("Left", left_arm), ("Right", right_arm)):
        if not any(offset):
            continue
        hand = pose(side + "Hand").translation
        outside = 1 if hand.x > 0 else -1
        spread, reach, height = offset
        target = hand + Vector((outside * spread, -reach, height))
        bend = pose(side + "ForeArm").translation - pose(side + "Arm").translation
        solve_limb(tuple(side + j for j in ("Arm", "ForeArm", "Hand")),
                   target, bend + Vector((outside * spread, spread, 0)))


left_sign = math.copysign(1, feet["Left"].translation.x - feet["Right"].translation.x)


def assess(t):
    slip = rise_and_fall(t, .08, .23, .48)
    catch = rise_and_fall(t, .12, .33, .67)
    test = rise_and_fall(t, .63, 1.00, 1.26)
    offload = ease((t - .23) / .30)
    upper_body(.041 * catch, left_sign * (.081 * offload - .024 * slip),
               .025 + .062 * catch + .022 * test, 8 * catch + 5 * test + 3 * offload,
               -left_sign * (9 * catch + 3 * offload), 13 * test + 8 * catch,
               (.11 * catch, .072 * catch, .065 * catch), (.17 * catch, -.032 * catch, .11 * catch))
    left = plant("Left", -.105 * ease((t - .16) / .26),
                 .048 * rise_and_fall(t, .16, .28, .42))
    right = plant("Right", .145 * ease(t / .12) + .034 * test,
                  .026 * (rise_and_fall(t, 0, .07, .16) + rise_and_fall(t, .47, .64, .85)) +
                  .018 * rise_and_fall(t, .99, 1.19, 1.4), -left_sign * 15 * slip, -7 * test)
    return max(left, right)


def stride_foot(side, phase, support, centre):
    phase %= 1
    swing = (phase - support) / (1 - support)
    lift = math.sin(math.pi * swing) ** 2 if swing > 0 else 0
    forward = centre - STRIDE * phase + STRIDE * ease(swing)
    height = .061 if side == "Left" else .077
    return plant(side, forward, height * lift, pitch=-8 * lift)


def protect(t):
    phase = t / ((WALK_END - 1) / FPS)
    loaded = (1 - math.cos(2 * math.pi * phase)) * .5
    sway = math.sin(2 * math.pi * phase)
    push = rise_and_fall(phase, 0, .25, .49)
    upper_body(.013 * sway - .025 * push, left_sign * (.045 + .035 * loaded),
               .047 + .020 * loaded + .027 * push,
               5 + 3 * loaded, -left_sign * (3 + 4 * loaded), 3 * loaded,
               (.015, -.062 * sway, .01), (.028, .085 * sway, .022 + .023 * loaded))
    return max(stride_foot(side, phase + offset, support, centre)
               for side, (offset, support, centre) in GAIT.items())


arm.animation_data.action = None
for bone in arm.pose.bones:
    bone.rotation_mode = "QUATERNION"


def record(name, last, perform):
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data.action = action
    samples = []
    previous = {}
    for frame in range(1, last + 1):
        scene.frame_set(frame)
        error = perform((frame - 1) / FPS)
        for bone in arm.pose.bones:
            q = bone.rotation_quaternion.copy()
            if bone.name in previous and previous[bone.name].dot(q) < 0:
                q.negate()
                bone.rotation_quaternion = q
            previous[bone.name] = q
            for path in ("location", "rotation_quaternion", "scale"):
                bone.keyframe_insert(path, frame=frame)
        points = skin_points()
        samples.append({"frame": frame, "targetErrorMetres": error,
                        "feet": {side: {"soleMetres": min(points[i].z for i in ids) - FLOOR,
                                        "forwardMetres": -pose(side + "Foot").translation.y}
                                 for side, ids in sole.items()}})
    return action, samples


catch, assessment = record("Kit|ankle_assess", CATCH_END, assess)
walk, protective = record("Kit|protective_walk", WALK_END, protect)

for row in assessment + protective:
    if row["targetErrorMetres"] > .002 or min(foot["soleMetres"] for foot in row["feet"].values()) < -.002:
        raise RuntimeError(f"A boot left its intended contact bounds: {row}")
for side, (offset, support, _) in GAIT.items():
    planted_forward = None
    for i, row in enumerate(protective):
        phase = i / (WALK_END - 1)
        if (phase + offset) % 1 > support:
            planted_forward = None
            continue
        foot = row["feet"][side]
        world_forward = foot["forwardMetres"] + STRIDE * phase
        if foot["soleMetres"] > .006 or (planted_forward is not None and abs(world_forward - planted_forward) > .002):
            raise RuntimeError(f"{side} boot slipped or lifted during support: {row}")
        planted_forward = world_forward

scene.frame_start, scene.frame_end = 1, CATCH_END
arm.animation_data.action = catch
arm.animation_data.action_slot = catch.slots[0]
scene.frame_set(21)

bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, FLOOR))
bpy.context.object.name = "STUDY ONLY - local ground plane"
bpy.context.object.color = (.28, .26, .21, 1)
camera_data = bpy.data.cameras.new("Approach study")
camera = bpy.data.objects.new("Approach study", camera_data)
scene.collection.objects.link(camera)
target = Vector((0, 0, 1.03))
camera.location = target + Vector((3.45, -3.7, .56))
camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
camera_data.type = "ORTHO"
camera_data.ortho_scale = 2.6
scene.camera = camera
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "OBJECT"
scene.display.shading.show_cavity = True
scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = 630, 630, 100
scene.render.image_settings.file_format = "PNG"
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "kit-ankle-approach.blend"), compress=True)
for action, poses in ((catch, (1, 15, 21, 61, 97)), (walk, (1, 15, 27, 41))):
    arm.animation_data.action = action
    arm.animation_data.action_slot = action.slots[0]
    for frame in poses:
        scene.frame_set(frame)
        scene.render.filepath = str(OUTPUT / f"{action.name.split('|')[1]}-{frame:02d}.png")
        bpy.ops.render.render(write_still=True)

contact = {"sourceSha256": SOURCE_HASH, "fps": FPS, "injuredLeg": "Right",
           "protectiveStrideMetres": STRIDE, "localFloorMetres": FLOOR,
           "assessment": assessment, "protectiveGait": protective}
(OUTPUT / "contact.json").write_text(json.dumps(contact, indent=2) + "\n", encoding="utf-8")
