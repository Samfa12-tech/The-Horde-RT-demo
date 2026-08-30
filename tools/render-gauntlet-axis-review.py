import bpy
import json
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python render-gauntlet-axis-review.py -- input.glb output-directory")
source, output_directory = sys.argv[sys.argv.index("--") + 1:]
source = os.path.abspath(source)
output_directory = os.path.abspath(output_directory)
os.makedirs(output_directory, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if len(meshes) != 1:
    raise RuntimeError("gauntlet axis review requires exactly one mesh")
gauntlet = meshes[0]
gauntlet.color = (0.12, 0.055, 0.025, 1.0)
points = [gauntlet.matrix_world @ vertex.co for vertex in gauntlet.data.vertices]
minimum = Vector(tuple(min(point[axis] for point in points) for axis in range(3)))
maximum = Vector(tuple(max(point[axis] for point in points) for axis in range(3)))
dimensions = maximum - minimum
centre = (minimum + maximum) * 0.5
fist_centre = Vector((centre.x, centre.y,
                      minimum.z + dimensions.z * 0.70))

handles = {}
for label, axis, colour in (
        ("x", Vector((1.0, 0.0, 0.0)), (0.8, 0.08, 0.04, 1.0)),
        ("y", Vector((0.0, 1.0, 0.0)), (0.04, 0.65, 0.10, 1.0)),
        ("z", Vector((0.0, 0.0, 1.0)), (0.95, 0.52, 0.05, 1.0))):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=48, radius=max(dimensions.x, dimensions.y) * 0.135,
        depth=max(dimensions) * 1.5, location=fist_centre)
    handle = bpy.context.active_object
    handle.name = label.upper() + "GripAxisAudit"
    handle.rotation_mode = "QUATERNION"
    handle.rotation_quaternion = axis.to_track_quat("Z", "Y")
    handle.color = colour
    handles[label] = handle

camera_data = bpy.data.cameras.new("GauntletAxisCamera")
camera = bpy.data.objects.new("GauntletAxisCamera", camera_data)
bpy.context.collection.objects.link(camera)
bpy.context.scene.camera = camera
camera.data.type = "ORTHO"
camera.data.ortho_scale = max(dimensions) * 1.22


def point_at(target):
    camera.rotation_euler = (target - camera.location).to_track_quat(
        "-Z", "Y").to_euler()


scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "OBJECT"
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.render.resolution_x = 640
scene.render.resolution_y = 640
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
distance = max(dimensions) * 2.3
views = {
    "palm": centre + Vector((0.0, -distance, 0.0)),
    "side": centre + Vector((distance, 0.0, 0.0)),
    "three-quarter": centre + Vector((distance * 0.7, -distance * 0.7,
                                        distance * 0.12)),
}
for axis_label, active_handle in handles.items():
    for label, handle in handles.items():
        handle.hide_render = label != axis_label
    for view_label, location in views.items():
        camera.location = location
        point_at(fist_centre)
        scene.render.filepath = os.path.join(
            output_directory, axis_label + "-" + view_label + ".png")
        bpy.ops.render.render(write_still=True)

with open(os.path.join(output_directory, "axis-review.json"), "w",
          encoding="utf-8") as handle:
    json.dump({
        "source": os.path.basename(source),
        "bounds": {"minimum": list(minimum), "maximum": list(maximum)},
        "fistCentre": list(fist_centre),
        "handleRadius": max(dimensions.x, dimensions.y) * 0.135,
    }, handle, indent=2)
