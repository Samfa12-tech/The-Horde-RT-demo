"""Create a local Blender and GLB standing study from the prepared Bellwether rig.

Usage: blender --background --python-exit-code 1 --python author-record-keeper-idle.py -- <source-dir> <new-output-dir>

Source directory must contain recordkeeper-rigged.glb and recordkeeper-walking.glb.
The output does not include platform texture arrays or establish redistribution rights.
"""

import bpy
import json
import math
import sys
from pathlib import Path
from mathutils import Quaternion, Vector

source = Path(sys.argv[-2])
target = Path(sys.argv[-1])
target.mkdir(parents=True, exist_ok=False)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = 30

def load(name):
    existing = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(source / name))
    objects = set(bpy.data.objects) - existing
    armature = next(obj for obj in objects if obj.type == "ARMATURE")
    mesh = next(obj for obj in objects if obj.type == "MESH" and any(mod.type == "ARMATURE" for mod in obj.modifiers))
    return armature, mesh, objects

arm_names = [side + part for side in ("Left", "Right")
             for part in ("Shoulder", "Arm", "ForeArm", "Hand")]
walk, _, walking_objects = load("recordkeeper-walking.glb")
scene.frame_set(1)
arms_at_rest = {name: walk.pose.bones[name].matrix.to_quaternion() for name in arm_names}
for obj in walking_objects:
    bpy.data.objects.remove(obj, do_unlink=True)

armature, mesh, _ = load("recordkeeper-rigged.glb")
scene.frame_set(1)
poses = {bone.name: bone.matrix_basis.copy() for bone in armature.pose.bones}
armature.animation_data_clear()
for bone in armature.pose.bones:
    bone.rotation_mode = "QUATERNION"
    bone.matrix_basis = poses[bone.name]
bpy.context.view_layer.update()

# Keep the standing leg stance. Set the arms from the relaxed passing pose,
# using the standing rig's joint positions and imported bind transforms.
for name in arm_names:
    bone = armature.pose.bones[name]
    bone.matrix = arms_at_rest[name].to_matrix().to_4x4()
    bone.location = poses[name].to_translation()
    bone.scale = poses[name].to_scale()
    bpy.context.view_layer.update()

neutral = {bone.name: bone.rotation_quaternion.copy() for bone in armature.pose.bones}
for frame, breath, glance in ((1,0,0),(31,0.65,0.8),(61,0.95,1.4),(91,0.45,0.6),(121,0,0)):
    scene.frame_set(frame)
    for bone in armature.pose.bones:
        bone.rotation_quaternion = neutral[bone.name]
        if bone.name in ("Spine", "Head"):
            angle = breath if bone.name == "Spine" else glance
            world_axis = (1,0,0) if bone.name == "Spine" else (0,0,1)
            axis = bone.matrix.to_quaternion().inverted() @ Vector(world_axis)
            bone.rotation_quaternion @= Quaternion(axis, math.radians(angle))
        bone.keyframe_insert(data_path="location", frame=frame, group=bone.name)
        bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        bone.keyframe_insert(data_path="scale", frame=frame, group=bone.name)
idle = armature.animation_data.action
idle.name = "Warden|idle"
for action in list(bpy.data.actions):
    if action != idle:
        bpy.data.actions.remove(action)
scene.frame_start, scene.frame_end = 1, 121
samples = []
for frame in (1,31,61,91,121):
    scene.frame_set(frame)
    deps = bpy.context.evaluated_depsgraph_get()
    ev = mesh.evaluated_get(deps)
    points = [ev.matrix_world @ vertex.co for vertex in ev.data.vertices]
    low = [min(point[i] for point in points) for i in range(3)]
    high = [max(point[i] for point in points) for i in range(3)]
    assert abs(low[2]) < 0.003 and 1.70 < high[2] < 1.80 and high[0] - low[0] < .85, (frame,low,high)
    samples.append({"frame":frame,"bounds_min":low,"bounds_max":high})
scene.frame_set(1)

bpy.ops.object.select_all(action="DESELECT")
mesh.select_set(True)
armature.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.wm.save_as_mainfile(filepath=str(target / "record-keeper-idle.blend"))
bpy.ops.export_scene.gltf(filepath=str(target / "resident.glb"), export_format="GLB", use_selection=True,
                          export_tangents=True, export_animation_mode="ACTIONS", export_anim_slide_to_zero=True,
                          export_image_format="AUTO")
(target / "standing-pose-report.json").write_text(json.dumps({"seconds":4,"samples":samples}, indent=2))