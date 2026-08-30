import bpy
import collections
import json
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python audit-player-arm-components.py -- player.glb output.json")
source, destination = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=os.path.abspath(source), import_shading="NORMALS")
player = next(obj for obj in bpy.context.scene.objects if obj.type == "MESH")
rig = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")


def chain_indices(side):
    names = {side + "Arm", side + "ForeArm", side + "Hand"}
    return {group.index for group in player.vertex_groups if group.name in names}


def chain_weight(vertex, indices):
    return sum(assignment.weight for assignment in vertex.groups if assignment.group in indices)


report = {}
for side in ("Left", "Right"):
    indices = chain_indices(side)
    candidates = []
    vertex_to_faces = collections.defaultdict(list)
    for polygon in player.data.polygons:
        points = [player.matrix_world @ player.data.vertices[index].co for index in polygon.vertices]
        maximum_edge = max(
            (points[(edge + 1) % len(points)] - points[edge]).length
            for edge in range(len(points)))
        if (maximum_edge <= 0.12 and
                all(chain_weight(player.data.vertices[index], indices) >= 0.85
                    for index in polygon.vertices)):
            candidate_index = len(candidates)
            candidates.append(polygon)
            for vertex_index in polygon.vertices:
                vertex_to_faces[vertex_index].append(candidate_index)
    unseen = set(range(len(candidates)))
    components = []
    while unseen:
        seed = unseen.pop()
        queue = [seed]
        component = [seed]
        while queue:
            face_index = queue.pop()
            for vertex_index in candidates[face_index].vertices:
                for adjacent in vertex_to_faces[vertex_index]:
                    if adjacent in unseen:
                        unseen.remove(adjacent)
                        queue.append(adjacent)
                        component.append(adjacent)
        vertices = {
            vertex_index
            for face_index in component
            for vertex_index in candidates[face_index].vertices
        }
        points = [player.matrix_world @ player.data.vertices[index].co for index in vertices]
        minimum = [min(point[axis] for point in points) for axis in range(3)]
        maximum = [max(point[axis] for point in points) for axis in range(3)]
        components.append({
            "triangles": sum(max(0, len(candidates[index].vertices) - 2) for index in component),
            "faces": len(component),
            "vertices": len(vertices),
            "minimum": minimum,
            "maximum": maximum,
            "extent": [maximum[axis] - minimum[axis] for axis in range(3)],
        })
    components.sort(key=lambda value: value["triangles"], reverse=True)
    arm_head = rig.matrix_world @ rig.data.bones[side + "Arm"].head_local
    forearm_head = rig.matrix_world @ rig.data.bones[side + "ForeArm"].head_local
    hand_head = rig.matrix_world @ rig.data.bones[side + "Hand"].head_local
    hand_end = hand_head + (hand_head - forearm_head).normalized() * 0.16
    bone_segments = [(arm_head, forearm_head),
                     (forearm_head, hand_head),
                     (hand_head, hand_end)]

    def segment_distance(point, segment):
        start, end = segment
        extent = end - start
        parameter = max(0.0, min(1.0,
            (point - start).dot(extent) / max(extent.length_squared, 1.0e-9)))
        return (point - (start + extent * parameter)).length

    radius_counts = {}
    for radius in (0.08, 0.10, 0.12, 0.14, 0.16, 0.18):
        count = 0
        for polygon in player.data.polygons:
            points = [player.matrix_world @ player.data.vertices[index].co
                      for index in polygon.vertices]
            maximum_edge = max(
                (points[(edge + 1) % len(points)] - points[edge]).length
                for edge in range(len(points)))
            on_side = all(point.x >= 0.08 if side == "Left" else point.x <= -0.08
                          for point in points)
            close = all(min(segment_distance(point, segment)
                            for segment in bone_segments) <= radius
                        for point in points)
            if maximum_edge <= 0.12 and on_side and close:
                count += max(0, len(polygon.vertices) - 2)
        radius_counts[str(radius)] = count
    report[side] = {
        "bones": [{"head": list(start), "tail": list(end)}
                  for start, end in bone_segments],
        "spatialRadiusTriangleCounts": radius_counts,
        "weightComponents": components,
    }

os.makedirs(os.path.dirname(os.path.abspath(destination)), exist_ok=True)
with open(destination, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
