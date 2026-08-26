import bpy
import json
import math
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python render-player-rig-review.py -- input.glb output-directory")
source, output_directory = sys.argv[sys.argv.index("--") + 1:]
source = os.path.abspath(source)
output_directory = os.path.abspath(output_directory)
os.makedirs(output_directory, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and
          any(modifier.type == "ARMATURE" for modifier in obj.modifiers)]
armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
if len(meshes) != 1 or len(armatures) != 1:
    raise RuntimeError(
        "reviewed player rig must contain exactly one mesh and one armature; "
        + repr([(obj.name, obj.type) for obj in bpy.context.scene.objects])
    )
mesh = meshes[0]
armature = armatures[0]
actions = list(bpy.data.actions)
action = next((candidate for candidate in actions if candidate.name == "Walking"), None)
if action is None and len(actions) == 1:
    action = actions[0]
if action is None:
    raise RuntimeError("reviewed player rig has no unambiguous Walking action")
if armature.animation_data is None:
    armature.animation_data_create()
armature.animation_data.action = action

weighted_vertices = 0
maximum_influences = 0
zero_weight_vertices = 0
for vertex in mesh.data.vertices:
    influences = [group for group in vertex.groups if group.weight > 0.0001]
    maximum_influences = max(maximum_influences, len(influences))
    if influences:
        weighted_vertices += 1
    else:
        zero_weight_vertices += 1

world_points = [mesh.matrix_world @ Vector(corner) for corner in mesh.bound_box]
minimum = Vector(tuple(min(point[axis] for point in world_points) for axis in range(3)))
maximum = Vector(tuple(max(point[axis] for point in world_points) for axis in range(3)))
centre = (minimum + maximum) * 0.5
dimensions = maximum - minimum
radius = max(dimensions) * 0.72

world = bpy.data.worlds.new("RigReviewWorld")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.055, 0.06, 0.07, 1.0)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.55
bpy.context.scene.world = world


def point_at(obj, target):
    obj.rotation_euler = (target - obj.location).to_track_quat("-Z", "Y").to_euler()


for location, energy, size in [
    ((centre.x + radius, centre.y - radius, centre.z + radius), 120.0, radius * 1.2),
    ((centre.x - radius, centre.y + radius * 0.4, centre.z + radius * 0.4), 80.0, radius),
]:
    light_data = bpy.data.lights.new("RigReviewArea", "AREA")
    light_data.energy = energy
    light_data.size = max(size, 0.1)
    light = bpy.data.objects.new("RigReviewArea", light_data)
    light.location = location
    point_at(light, centre)
    bpy.context.collection.objects.link(light)

camera_data = bpy.data.cameras.new("RigReviewCamera")
camera = bpy.data.objects.new("RigReviewCamera", camera_data)
bpy.context.collection.objects.link(camera)
bpy.context.scene.camera = camera
camera.data.type = "ORTHO"
camera.data.ortho_scale = max(dimensions) * 1.24

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 640
scene.render.resolution_y = 640
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.view_settings.look = "AgX - Medium High Contrast"
frame_start, frame_end = (int(round(value)) for value in action.frame_range)
sample_frames = sorted(set((frame_start, (frame_start + frame_end) // 2, frame_end)))
for frame in sample_frames:
    scene.frame_set(frame)
    camera.location = centre + Vector((radius * 2.1, -radius * 2.1, radius * 0.25))
    point_at(camera, centre)
    scene.render.filepath = os.path.join(output_directory, f"walk-frame-{frame:03}.png")
    bpy.ops.render.render(write_still=True)

report = {
    "source": os.path.basename(source),
    "triangles": sum(max(0, len(polygon.vertices) - 2) for polygon in mesh.data.polygons),
    "vertices": len(mesh.data.vertices),
    "weightedVertices": weighted_vertices,
    "zeroWeightVertices": zero_weight_vertices,
    "maximumInfluences": maximum_influences,
    "bones": [bone.name for bone in armature.data.bones],
    "action": action.name,
    "frameRange": [frame_start, frame_end],
    "sampleFrames": sample_frames,
    "boundsBlender": {
        "minimum": list(minimum),
        "maximum": list(maximum),
        "dimensions": list(dimensions),
    },
}
with open(os.path.join(output_directory, "rig-review.json"), "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
