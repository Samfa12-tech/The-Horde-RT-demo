import bpy
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python render-player-grip-review.py -- input.glb output-directory"
    )
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
    raise RuntimeError("grip review requires exactly one skinned mesh and armature")
player = meshes[0]
rig = armatures[0]
idle = next((action for action in bpy.data.actions if action.name == "Idle"), None)
if rig.animation_data is None:
    rig.animation_data_create()
if idle is not None:
    rig.animation_data.action = idle
bpy.context.scene.frame_set(1)
bpy.context.view_layer.update()

player.color = (0.12, 0.055, 0.025, 1.0)
depsgraph = bpy.context.evaluated_depsgraph_get()
evaluated_object = player.evaluated_get(depsgraph)
evaluated_mesh = evaluated_object.to_mesh()


def group_weight(vertex, names):
    indices = {player.vertex_groups[name].index for name in names
               if player.vertex_groups.get(name) is not None}
    return sum(assignment.weight for assignment in vertex.groups
               if assignment.group in indices)


# Inspect exactly the two final-skinned material-0 primitives seen by primary
# rays. Hiding the complete reflection/shadow body and the opposite arm keeps
# a good full-body source mesh from concealing a bad first-person partition.
primary_objects = {}
world_vertices = [player.matrix_world @ vertex.co
                  for vertex in evaluated_mesh.vertices]
for side in ("Left", "Right"):
    side_faces = []
    for polygon in player.data.polygons:
        if polygon.material_index != 0:
            continue
        left_weight = sum(group_weight(player.data.vertices[index],
                                       ("LeftForeArm", "LeftHand"))
                          for index in polygon.vertices)
        right_weight = sum(group_weight(player.data.vertices[index],
                                        ("RightForeArm", "RightHand"))
                           for index in polygon.vertices)
        if (side == "Left" and left_weight >= right_weight) or \
                (side == "Right" and right_weight > left_weight):
            side_faces.append(tuple(polygon.vertices))
    mesh = bpy.data.meshes.new(side + "PrimaryGripReviewMesh")
    mesh.from_pydata([tuple(point) for point in world_vertices], [], side_faces)
    mesh.update()
    obj = bpy.data.objects.new(side + "PrimaryGripReview", mesh)
    obj.color = player.color
    bpy.context.collection.objects.link(obj)
    primary_objects[side] = obj
player.hide_render = True
evaluated_object.to_mesh_clear()

handle_objects = {}
handle_axes = {}
for side in ("Left", "Right"):
    grip = rig.pose.bones.get(side + "Grip")
    if grip is None:
        raise RuntimeError(f"runtime player has no {side}Grip socket")
    world_from_grip = rig.matrix_world @ grip.matrix
    origin = world_from_grip.translation
    axis = (world_from_grip.to_3x3() @ Vector((0.0, 1.0, 0.0))).normalized()
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=0.018, depth=0.14,
                                       location=origin)
    handle = bpy.context.active_object
    handle.name = side + "GripHandleAudit"
    handle.rotation_mode = "QUATERNION"
    handle.rotation_quaternion = axis.to_track_quat("Z", "Y")
    handle.color = (0.95, 0.52, 0.05, 1.0)
    handle_objects[side] = handle
    handle_axes[side] = axis

camera_data = bpy.data.cameras.new("GripReviewCamera")
camera = bpy.data.objects.new("GripReviewCamera", camera_data)
bpy.context.collection.objects.link(camera)
bpy.context.scene.camera = camera
camera.data.type = "ORTHO"
camera.data.ortho_scale = 0.29


def point_at(target):
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()


scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "OBJECT"
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.display.shading.cavity_type = "WORLD"
scene.render.resolution_x = 640
scene.render.resolution_y = 640
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False

for side in ("Left", "Right"):
    for candidate_side, candidate in primary_objects.items():
        candidate.hide_render = candidate_side != side
    for candidate_side, candidate in handle_objects.items():
        candidate.hide_render = candidate_side != side
    grip = rig.pose.bones[side + "Grip"]
    target = (rig.matrix_world @ grip.matrix).translation
    camera.location = target + Vector((0.0, -0.45, 0.02))
    point_at(target)
    scene.render.filepath = os.path.join(
        output_directory, side.lower() + "-grip-front.png")
    bpy.ops.render.render(write_still=True)
    side_sign = 1.0 if side == "Left" else -1.0
    camera.location = (target + handle_axes[side] * (0.42 * side_sign) +
                       Vector((0.0, 0.0, 0.02)))
    point_at(target)
    scene.render.filepath = os.path.join(
        output_directory, side.lower() + "-grip-side.png")
    bpy.ops.render.render(write_still=True)
