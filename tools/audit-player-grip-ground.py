import bpy
import json
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python audit-player-grip-ground.py -- input.glb output.json"
    )
source, destination = sys.argv[sys.argv.index("--") + 1:]
source = os.path.abspath(source)
destination = os.path.abspath(destination)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and
          any(modifier.type == "ARMATURE" for modifier in obj.modifiers)]
armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
if len(meshes) != 1 or len(armatures) != 1:
    raise RuntimeError("player audit requires exactly one skinned mesh and armature")
mesh = meshes[0]
rig = armatures[0]
if rig.animation_data is None:
    rig.animation_data_create()
idle = next((action for action in bpy.data.actions if action.name == "Idle"), None)
if idle is not None:
    rig.animation_data.action = idle
bpy.context.scene.frame_set(1)
bpy.context.view_layer.update()

depsgraph = bpy.context.evaluated_depsgraph_get()
evaluated_mesh_object = mesh.evaluated_get(depsgraph)
evaluated_mesh = evaluated_mesh_object.to_mesh()


def bounds(points):
    if not points:
        return None
    minimum = [min(point[axis] for point in points) for axis in range(3)]
    maximum = [max(point[axis] for point in points) for axis in range(3)]
    return {
        "minimum": minimum,
        "maximum": maximum,
        "dimensions": [maximum[axis] - minimum[axis] for axis in range(3)],
    }


def group_weight(vertex, names):
    indices = {mesh.vertex_groups[name].index for name in names
               if mesh.vertex_groups.get(name) is not None}
    return sum(group.weight for group in vertex.groups if group.group in indices)


def weighted_points(names, threshold=0.25):
    return [mesh.matrix_world @ evaluated_mesh.vertices[vertex.index].co
            for vertex in mesh.data.vertices
            if group_weight(vertex, names) >= threshold]


report = {
    "source": os.path.basename(source),
    "bones": [bone.name for bone in rig.data.bones],
    "meshObjectMatrix": [list(row) for row in mesh.matrix_world],
    "armatureObjectMatrix": [list(row) for row in rig.matrix_world],
    "groups": {},
    "armBones": {},
}

for name in ("LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "LeftGrip",
             "RightShoulder", "RightArm", "RightForeArm", "RightHand", "RightGrip"):
    bone = rig.data.bones.get(name)
    if bone is None:
        continue
    report["armBones"][name] = {
        "headArmature": list(bone.head_local),
        "tailArmature": list(bone.tail_local),
        "length": bone.length,
        "parent": bone.parent.name if bone.parent is not None else None,
        "matrixArmature": [list(row) for row in bone.matrix_local],
    }

for name in ("LeftArm", "RightArm", "LeftHand", "RightHand", "LeftFoot",
             "LeftToeBase", "RightFoot", "RightToeBase"):
    points = weighted_points([name])
    entry = {
        "countAtWeight025": len(points),
        "worldBounds": bounds(points),
    }
    pose_bone = rig.pose.bones.get(name)
    if pose_bone is not None:
        world_from_bone = rig.matrix_world @ pose_bone.matrix
        bone_from_world = world_from_bone.inverted_safe()
        local_points = [bone_from_world @ point for point in points]
        entry["boneWorldOrigin"] = list(world_from_bone.translation)
        entry["boneLocalBounds"] = bounds(local_points)
    report["groups"][name] = entry

left_sole = weighted_points(["LeftFoot", "LeftToeBase"], 0.10)
right_sole = weighted_points(["RightFoot", "RightToeBase"], 0.10)
all_points = [mesh.matrix_world @ vertex.co for vertex in evaluated_mesh.vertices]
report["ground"] = {
    "leftFootAndToeBounds": bounds(left_sole),
    "rightFootAndToeBounds": bounds(right_sole),
    "wholeMeshBounds": bounds(all_points),
}

report["clipGroundSamples"] = []
for action_name in ("Idle", "Walking"):
    action = next((candidate for candidate in bpy.data.actions
                   if candidate.name == action_name), None)
    if action is None:
        continue
    rig.animation_data.action = action
    first = int(round(action.frame_range[0]))
    last = int(round(action.frame_range[1]))
    for frame in sorted(set((first, (first + last) // 2, last))):
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        sampled_object = mesh.evaluated_get(depsgraph)
        sampled_mesh = sampled_object.to_mesh()
        left_indices = {mesh.vertex_groups[name].index
                        for name in ("LeftFoot", "LeftToeBase")
                        if mesh.vertex_groups.get(name) is not None}
        right_indices = {mesh.vertex_groups[name].index
                         for name in ("RightFoot", "RightToeBase")
                         if mesh.vertex_groups.get(name) is not None}
        left_points = []
        right_points = []
        for vertex in mesh.data.vertices:
            left_weight = sum(group.weight for group in vertex.groups
                              if group.group in left_indices)
            right_weight = sum(group.weight for group in vertex.groups
                               if group.group in right_indices)
            point = mesh.matrix_world @ sampled_mesh.vertices[vertex.index].co
            if left_weight >= 0.10:
                left_points.append(point)
            if right_weight >= 0.10:
                right_points.append(point)
        report["clipGroundSamples"].append({
            "clip": action_name,
            "frame": frame,
            "leftMinimumUp": bounds(left_points)["minimum"][2],
            "rightMinimumUp": bounds(right_points)["minimum"][2],
        })
        sampled_object.to_mesh_clear()

os.makedirs(os.path.dirname(destination), exist_ok=True)
with open(destination, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
evaluated_mesh_object.to_mesh_clear()
