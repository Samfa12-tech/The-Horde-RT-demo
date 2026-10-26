"""Separate same-rig source study; no native binding or runtime asset export.

Run Blender in background from the repository root. All outputs must be absent.
The original GLBs are only read. Blender's display bone tails are not joint
lengths: use the actual shoulder/elbow/wrist node origins for this imported rig.
"""

from pathlib import Path
import argparse
import hashlib
import json
import math
import struct
import sys

import bpy
from mathutils import Euler, Matrix, Quaternion, Vector

ROOT = Path.cwd()
SOURCE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output-directory", type=Path, required=True)
parser.add_argument("--editable-output", type=Path, default=SOURCE/"flask-use-study-r1.blend")
args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
OUT = args.output_directory.resolve()
EDITABLE = args.editable_output.resolve()
if EDITABLE.exists() or OUT.exists():
    raise RuntimeError("Preserve prior source/evidence; outputs must be new")
OUT.mkdir(parents=True)
spec = json.loads((SOURCE/"intent.json").read_text())
fixed_phases = [("SwordStow",0.,.30),("FlaskLift",.30,.55),("Drink",.55,.70),
                ("FlaskReturn",.70,.89),("SwordRestore",.89,1.25)]
if (spec["status"] != "source-only-unadmitted" or spec["durationSeconds"] != 1.25 or
        spec["sampleRateHz"] != 60 or
        [(p["name"],p["start"],p["end"]) for p in spec["phases"]] != fixed_phases or
        (spec["cancelSampleTime"],spec["cancelFlaskReturnSeconds"],spec["cancelSwordRestoreSeconds"]) != (.60,.20,.36) or
        (spec["swordReleaseTime"],spec["swordRegripTime"]) != (.20,1.00)):
    raise RuntimeError("Fixed source-study timing changed; review authorer and policy together, do not silently ignore intent edits")
paths = {
    "body": "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb",
    "viewmodel": "assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb",
    "sword": "assets/models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb",
    "flask": spec["flaskSource"],
    "equipment": "src/gameplay/items/HeldItemState.h",
    "kinematics": "src/gameplay/items/HeldItemKinematics.cpp",
    "guard": "src/gameplay/animation/PlayerAnimationState.cpp",
    "combat": "src/gameplay/SwordCombat.h",
}
hashes = {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths.values()}
bpy.ops.wm.read_factory_settings(use_empty=True)


def import_objects(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(ROOT/path), import_shading="NORMALS")
    return list(set(bpy.data.objects)-before)


def unit_matrix(matrix):
    return Matrix.LocRotScale(matrix.translation, matrix.to_quaternion(), Vector((1,1,1)))


body_objects = import_objects(paths["body"])
rig = next(o for o in body_objects if o.type == "ARMATURE")
body = next(o for o in body_objects if o.type == "MESH" and
            any(m.type == "ARMATURE" for m in o.modifiers))
for obj in body_objects:
    if obj.type == "MESH" and obj != body:
        obj.hide_render = True
original_actions = list(bpy.data.actions)
for action in original_actions:
    action.use_fake_user = True
rig.animation_data.action = next(a for a in original_actions if a.name == "Idle")
bpy.context.scene.frame_set(1)
bpy.context.view_layer.update()
rest_basis = {p.name:p.matrix_basis.copy() for p in rig.pose.bones}
bind = {b.name:[list(row) for row in b.matrix_local] for b in rig.data.bones}
initial_world = {p.name:rig.matrix_world@p.matrix for p in rig.pose.bones}
view_objects = import_objects(paths["viewmodel"])
view_rig = next(o for o in view_objects if o.type == "ARMATURE")
view = next(o for o in view_objects if o.type == "MESH" and
            any(m.type == "ARMATURE" for m in o.modifiers))
for obj in view_objects:
    if obj.type == "MESH" and obj != view:
        obj.hide_render = True
if set(view_rig.data.bones.keys()) != set(rig.data.bones.keys()):
    raise RuntimeError("Body/viewmodel bone roster differs")
for bone in view_rig.data.bones:
    if max(abs(bone.matrix_local[r][c]-rig.data.bones[bone.name].matrix_local[r][c])
           for r in range(4) for c in range(4)) > 1e-5:
        raise RuntimeError("Body/viewmodel bind mismatch")
for mod in view.modifiers:
    if mod.type == "ARMATURE":
        mod.object = rig
view.matrix_world = body.matrix_world.copy()
view.hide_render = True


def prop(path, label):
    objects = import_objects(path)
    marker = next(o for o in objects if o.name.startswith("Grip"))
    marker_world = unit_matrix(marker.matrix_world)
    parent = bpy.data.objects.new(label, None)
    bpy.context.collection.objects.link(parent)
    for obj in objects:
        if obj.parent is None:
            world = obj.matrix_world.copy()
            obj.parent = parent
            obj.matrix_world = world
    return parent, marker_world, objects


sword, sword_marker, sword_objects = prop(paths["sword"], "StudySword")
flask, flask_marker, flask_objects = prop(paths["flask"], "StudyFlask")
rig.animation_data_clear()
rig.animation_data_create()
rot90 = Matrix.Rotation(math.pi/2,4,"X")
rest_rotation = Euler(spec["restSwordEulerXYZ"], "XYZ").to_matrix().to_4x4()
rest_root = Matrix.Translation(Vector(spec["restGripPosition"]))@rest_rotation@sword_marker.inverted()
# Exact existing body-stow matrix converted from glTF Y-up to Blender Z-up.
co, si = math.cos(.5), math.sin(.5)
mount = Matrix(((-1,0,0,-.30),(0,-co,-si,.135),(0,-si,co,-.09),(0,0,0,1)))
c = Matrix(((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)))
def raw_hips_world():
    # Blender chooses display-bone orientations; they are not native joint
    # axes. Read the immutable glTF node frame for the existing body mount.
    data = (ROOT/paths["body"]).read_bytes()
    length = struct.unpack_from("<I",data,12)[0]
    document = json.loads(data[20:20+length])
    nodes = document["nodes"]
    parents = {child:i for i,node in enumerate(nodes) for child in node.get("children",[])}
    def world(i):
        node = nodes[i]
        if "matrix" in node:
            local = Matrix(tuple(tuple(node["matrix"][col*4+row] for col in range(4)) for row in range(4)))
        else:
            x,y,z,w = node.get("rotation",(0,0,0,1))
            local = Matrix.LocRotScale(Vector(node.get("translation",(0,0,0))),
                Quaternion((w,x,y,z)),Vector(node.get("scale",(1,1,1))))
        return world(parents[i])@local if i in parents else local
    return unit_matrix(world(next(i for i,n in enumerate(nodes) if n.get("name")=="Hips")))
native_hips = raw_hips_world()
stow_root = c@native_hips@mount@c.inverted()
if ((c@native_hips).translation-initial_world["Hips"].translation).length > 1e-5:
    raise RuntimeError("Raw Hips and imported Idle origin differ")
stow_grip = stow_root@sword_marker
belt_position = stow_grip.translation+Vector(spec["flaskBeltOffsetFromSwordGrip"])
belt_root = Matrix.Translation(belt_position)@flask_marker.inverted()
drink_rotation = Matrix.Rotation(spec["drinkYawRadiansZ"],4,"Z")@Matrix.Rotation(spec["drinkTiltRadiansX"],4,"X")
mouth_local = Vector((0,0,spec["flaskMouthHeightMetres"]))
mouth_in_grip = flask_marker.inverted()@mouth_local
drink_grip_position = Vector(spec["mouthTarget"])-drink_rotation.to_3x3()@mouth_in_grip
drink_root = Matrix.Translation(drink_grip_position)@drink_rotation@flask_marker.inverted()
hand_unit = unit_matrix(initial_world["RightHand"])
grip_unit = unit_matrix(initial_world["RightGrip"])
hand_from_grip = hand_unit.inverted()@grip_unit
upper_length = (initial_world["RightForeArm"].translation-initial_world["RightArm"].translation).length
lower_length = (initial_world["RightHand"].translation-initial_world["RightForeArm"].translation).length


def smooth(x):
    x = max(0., min(1., x))
    return x*x*(3-2*x)


def blend(a,b,x):
    x = smooth(x)
    return Matrix.LocRotScale(a.translation.lerp(b.translation,x),
        a.to_quaternion().slerp(b.to_quaternion(),x),Vector((1,1,1)))


def roots(t):
    sw = blend(rest_root,stow_root,t/.30) if t <= .30 else (
         blend(stow_root,rest_root,(t-.89)/.36) if t >= .89 else stow_root.copy())
    fl = blend(belt_root,drink_root,(t-.30)/.25) if .30 < t < .55 else (
         drink_root.copy() if .55 <= t <= .70 else (
         blend(drink_root,belt_root,(t-.70)/.19) if .70 < t < .89 else belt_root.copy()))
    # An authored outward carry arc keeps the wrist outside the original
    # arm's minimum folded reach; neither bone length nor the endpoint moves.
    if .30 < t < .55:
        fl.translation.x -= spec["outwardCarryArcMetres"]*math.sin(math.pi*(t-.30)/.25)**2
    elif .70 < t < .89:
        fl.translation.x -= spec["outwardCarryArcMetres"]*math.sin(math.pi*(t-.70)/.19)**2
    return sw,fl


def target_grip(t,sw,fl):
    sword_grip = sw@sword_marker@rot90
    flask_grip = fl@flask_marker@rot90
    if t <= spec["swordReleaseTime"] or t >= spec["swordRegripTime"]:
        return sword_grip
    if t < .30:
        released = roots(spec["swordReleaseTime"])[0]@sword_marker@rot90
        return blend(released,belt_root@flask_marker@rot90,(t-.20)/.10)
    if t <= .89:
        return flask_grip
    return blend(belt_root@flask_marker@rot90,sword_grip,(t-.89)/.11)


def solve_arm(desired_grip):
    desired_hand = desired_grip@hand_from_grip.inverted()
    shoulder = (rig.matrix_world@rig.pose.bones["RightArm"].matrix).translation
    wanted = desired_hand.translation-shoulder
    requested = wanted.length
    direction = wanted.normalized()
    distance = max(abs(upper_length-lower_length)+1e-5, min(upper_length+lower_length,requested))
    pole = Vector(spec["rightElbowPole"])
    bend = (pole-direction*pole.dot(direction)).normalized()
    adjacent = (upper_length**2+distance**2-lower_length**2)/(2*distance)
    height = math.sqrt(max(0.,upper_length**2-adjacent**2))
    elbow = shoulder+direction*adjacent+bend*height
    wrist = shoulder+direction*distance
    for bone,child,destination in (("RightArm","RightForeArm",elbow),
                                   ("RightForeArm","RightHand",wrist)):
        world = rig.matrix_world@rig.pose.bones[bone].matrix
        child_position = (rig.matrix_world@rig.pose.bones[child].matrix).translation
        delta = (child_position-world.translation).rotation_difference(destination-world.translation)
        rotated = delta.to_matrix().to_4x4()@world
        rotated.translation = world.translation
        rig.pose.bones[bone].matrix = rig.matrix_world.inverted()@rotated
        bpy.context.view_layer.update()
    # Preserve the imported scale rather than using display bone tails/stretch.
    final_hand = Matrix.LocRotScale(wrist,desired_hand.to_quaternion(),
                                    (rig.matrix_world@rig.pose.bones["RightHand"].matrix).to_scale())
    rig.pose.bones["RightHand"].matrix = rig.matrix_world.inverted()@final_hand
    bpy.context.view_layer.update()
    actual = unit_matrix(rig.matrix_world@rig.pose.bones["RightGrip"].matrix)
    return requested,(actual.translation-desired_grip.translation).length


scene = bpy.context.scene
scene.render.fps = spec["sampleRateHz"]
scene.frame_start, scene.frame_end = 1,76


def cancelled_roots(t):
    if t <= .60:
        return roots(t)
    sw = stow_root.copy() if t <= .80 else blend(stow_root,rest_root,(t-.80)/.36)
    fl = blend(roots(.60)[1],belt_root,(t-.60)/.20)
    if .60 < t < .80:
        fl.translation.x -= spec["outwardCarryArcMetres"]*math.sin(math.pi*(t-.60)/.20)**2
    return sw,fl


def cancelled_grip(t,sw,fl):
    if t <= .60:
        return target_grip(t,sw,fl)
    if t <= .80:
        return fl@flask_marker@rot90
    if t < .91:
        return blend(belt_root@flask_marker@rot90,sw@sword_marker@rot90,(t-.80)/.11)
    return sw@sword_marker@rot90


def author_sequence(label,count,root_fn,grip_fn):
    for obj in (rig,sword,flask):
        if obj.animation_data and obj.animation_data.action:
            obj.animation_data.action.use_fake_user = True
        obj.animation_data_clear()
        obj.animation_data_create()
        obj.animation_data.action = bpy.data.actions.new(obj.name+"_"+label)
    samples = []
    for frame in range(1,count+1):
        t = (frame-1)/60
        scene.frame_set(frame)
        for p in rig.pose.bones:
            p.matrix_basis = rest_basis[p.name].copy()
        bpy.context.view_layer.update()
        sw,fl = root_fn(t)
        target = grip_fn(t,sw,fl)
        requested,error = solve_arm(target)
        for name in ("RightArm","RightForeArm","RightHand"):
            p = rig.pose.bones[name]
            p.rotation_mode = "QUATERNION"
            for path in ("location","rotation_quaternion","scale"):
                p.keyframe_insert(data_path=path,frame=frame,group="StudyRightArm")
        for obj,transform in ((sword,sw),(flask,fl)):
            obj.rotation_mode = "QUATERNION"
            obj.matrix_world = transform
            for path in ("location","rotation_quaternion","scale"):
                obj.keyframe_insert(data_path=path,frame=frame)
        left = rig.matrix_world@rig.pose.bones["LeftGrip"].matrix
        left_error = max(abs(left[r][col]-initial_world["LeftGrip"][r][col]) for r in range(4) for col in range(4))
        samples.append({"time":t,"gripTarget":[list(row) for row in target],
                        "swordRoot":[list(row) for row in sw],"flaskRoot":[list(row) for row in fl],
                        "requestedWristReachMetres":requested,"gripErrorMetres":error,
                        "unchangedLeftMatrixError":left_error,
                        "rightBasis":{name:[list(row) for row in rig.pose.bones[name].matrix_basis]
                                      for name in ("RightArm","RightForeArm","RightHand")}})
    return samples,{obj.name:obj.animation_data.action for obj in (rig,sword,flask)}


samples,complete_actions = author_sequence("FlaskUseStudy_ONLY",76,roots,target_grip)
cancel_samples,cancel_actions = author_sequence("FlaskCancelStudy_ONLY",71,cancelled_roots,cancelled_grip)
for obj in (rig,sword,flask):
    obj.animation_data.action.use_fake_user = True
    obj.animation_data.action = complete_actions[obj.name]

camera_data = bpy.data.cameras.new("StudyCamera")
camera = bpy.data.objects.new("StudyCamera",camera_data)
bpy.context.collection.objects.link(camera)
scene.camera = camera
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "MATERIAL"
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.render.resolution_x,scene.render.resolution_y = 960,540
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"


def camera_at(position,target,orthographic=None):
    camera.location = position
    camera.rotation_euler = (Vector(target)-camera.location).to_track_quat("-Z","Y").to_euler()
    camera.data.type = "ORTHO" if orthographic else "PERSP"
    if orthographic:
        camera.data.ortho_scale = orthographic
    else:
        camera.data.lens = 24
        camera.data.clip_start = .01


scene["evidence_boundary"] = "Source authoring only; no native IK parity, gameplay timing authority or RT acceptance"
scene["candidate_status"] = "Unadmitted; review motion, contact, native integration and interruptions"
camera_at((2,-3,1.8),(0,0,1.15),2.6)
scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(EDITABLE))
for frame in (1,19,34,43,54,76):
    scene.frame_set(frame)
    scene.render.filepath = str(OUT/f"body-{frame:03}.png")
    bpy.ops.render.render(write_still=True)
body.hide_render,view.hide_render = True,False
camera_at((0,-.025,1.68),(0,-1,1.60))
for frame in (1,19,34,43,54,76):
    scene.frame_set(frame)
    scene.render.filepath = str(OUT/f"first-person-{frame:03}.png")
    bpy.ops.render.render(write_still=True)
scene.frame_set(34)
contact = (rig.matrix_world@rig.pose.bones["RightGrip"].matrix).translation
camera_at(contact+Vector((.25,-.35,.05)),contact,.27)
scene.render.resolution_x,scene.render.resolution_y = 640,640
scene.render.filepath = str(OUT/"grip-contact.png")
bpy.ops.render.render(write_still=True)
if bind != {b.name:[list(row) for row in b.matrix_local] for b in rig.data.bones}:
    raise RuntimeError("Bind changed")
if not all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==sha for p,sha in hashes.items()):
    raise RuntimeError("Source bytes changed")
result = {"scope":"Unadmitted source authoring; no native/RT/phone/behavior acceptance",
          "sourceHashes":hashes,"sampleCount":len(samples),"durationSeconds":1.25,
          "upperArmMetres":upper_length,"foreArmMetres":lower_length,
          "maximumRequestedWristReachMetres":max(s["requestedWristReachMetres"] for s in samples),
          "maximumGripErrorMetres":max(s["gripErrorMetres"] for s in samples),
          "maximumLeftMatrixError":max(s["unchangedLeftMatrixError"] for s in samples),
          "bindUnchanged":True,"sourceBytesUnchanged":True,"runtimeBindingChanged":False,
          "stowMountSource":"Unchanged raw glTF Hips frame plus SwordBodyStowFromHips; Blender display axes not substituted",
          "blendSha256":hashlib.sha256(EDITABLE.read_bytes()).hexdigest(),"samples":samples}
result["cancelStudy"] = {"cancelAtSeconds":.60,"flaskReturnSeconds":.20,"swordRestoreSeconds":.36,
    "restoredAtSeconds":1.16,"sampleCount":71,
    "maximumGripErrorMetres":max(s["gripErrorMetres"] for s in cancel_samples),
    "maximumLeftMatrixError":max(s["unchangedLeftMatrixError"] for s in cancel_samples),
    "samples":cancel_samples,"gameplaySideEffects":False}
(OUT/"source-motion.json").write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps({k:v for k,v in result.items() if k not in ('samples','cancelStudy')},indent=2))
