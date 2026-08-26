import bpy
import json
import math
import os
import sys
from mathutils import Vector


def arguments():
    separator = sys.argv.index("--")
    values = sys.argv[separator + 1:]
    if len(values) not in {2, 3, 4}:
        raise RuntimeError("Expected INPUT.glb OUTPUT_DIRECTORY [neutral|pbr] [PBR_TEXTURE_DIRECTORY]")
    mode = values[2] if len(values) >= 3 else "neutral"
    if mode not in {"neutral", "pbr"}:
        raise RuntimeError("Review mode must be neutral or pbr")
    texture_directory = os.path.abspath(values[3]) if len(values) == 4 else None
    return os.path.abspath(values[0]), os.path.abspath(values[1]), mode, texture_directory


def point_camera(camera, target):
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()


source, output_directory, review_mode, texture_directory = arguments()
os.makedirs(output_directory, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")

meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if not meshes:
    raise RuntimeError("Review source contains no mesh objects")

world_points = []
triangles = 0
vertices = 0
parts = []
for obj in meshes:
    vertices += len(obj.data.vertices)
    triangles += sum(max(0, len(poly.vertices) - 2) for poly in obj.data.polygons)
    parts.append({
        "name": obj.name,
        "vertices": len(obj.data.vertices),
        "triangles": sum(max(0, len(poly.vertices) - 2) for poly in obj.data.polygons),
        "materials": [slot.material.name if slot.material else "" for slot in obj.material_slots],
    })
    for corner in obj.bound_box:
        world_points.append(obj.matrix_world @ Vector(corner))

minimum = Vector((min(point.x for point in world_points),
                  min(point.y for point in world_points),
                  min(point.z for point in world_points)))
maximum = Vector((max(point.x for point in world_points),
                  max(point.y for point in world_points),
                  max(point.z for point in world_points)))
centre = (minimum + maximum) * 0.5
dimensions = maximum - minimum
radius = max(dimensions) * 0.72

neutral = bpy.data.materials.new("NeutralGeometryReview")
neutral.diffuse_color = (0.34, 0.36, 0.39, 1.0)
neutral.metallic = 0.05
neutral.roughness = 0.56
if review_mode == "neutral":
    for obj in meshes:
        obj.data.materials.clear()
        obj.data.materials.append(neutral)
elif texture_directory is not None:
    material = bpy.data.materials.new("ReviewedExternalPbr")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    def texture(name, colour_space):
        node = nodes.new("ShaderNodeTexImage")
        node.image = bpy.data.images.load(os.path.join(texture_directory, f"{name}.png"))
        node.image.colorspace_settings.name = colour_space
        return node
    base = texture("base-color", "sRGB")
    metallic = texture("metallic", "Non-Color")
    roughness = texture("roughness", "Non-Color")
    normal = texture("normal", "Non-Color")
    normal_map = nodes.new("ShaderNodeNormalMap")
    links.new(base.outputs["Color"], principled.inputs["Base Color"])
    links.new(metallic.outputs["Color"], principled.inputs["Metallic"])
    links.new(roughness.outputs["Color"], principled.inputs["Roughness"])
    links.new(normal.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])
    for obj in meshes:
        obj.data.materials.clear()
        obj.data.materials.append(material)

world = bpy.data.worlds.new("NeutralWorld")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.055, 0.06, 0.07, 1.0)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.55
bpy.context.scene.world = world

for location, energy, size in [
    ((centre.x + radius, centre.y - radius, centre.z + radius), 120.0, radius * 1.2),
    ((centre.x - radius, centre.y + radius * 0.4, centre.z + radius * 0.4), 80.0, radius),
    ((centre.x, centre.y + radius, centre.z - radius * 0.2), 55.0, radius * 0.8),
]:
    light_data = bpy.data.lights.new("ReviewArea", "AREA")
    light_data.energy = energy
    light_data.shape = "DISK"
    light_data.size = max(size, 0.1)
    light = bpy.data.objects.new("ReviewArea", light_data)
    light.location = location
    point_camera(light, centre)
    bpy.context.collection.objects.link(light)

camera_data = bpy.data.cameras.new("ReviewCamera")
camera = bpy.data.objects.new("ReviewCamera", camera_data)
bpy.context.collection.objects.link(camera)
bpy.context.scene.camera = camera
camera.data.type = "ORTHO"
camera.data.ortho_scale = max(dimensions.x, dimensions.y, dimensions.z) * 1.18

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 640
scene.render.resolution_y = 640
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False
scene.view_settings.look = "AgX - Medium High Contrast"

distance = max(radius * 3.0, 1.0)
views = {
    "front": Vector((centre.x, centre.y - distance, centre.z)),
    "right": Vector((centre.x + distance, centre.y, centre.z)),
    "back": Vector((centre.x, centre.y + distance, centre.z)),
    "left": Vector((centre.x - distance, centre.y, centre.z)),
    "three-quarter": Vector((centre.x + distance * 0.72, centre.y - distance * 0.72, centre.z + distance * 0.18)),
    "top": Vector((centre.x, centre.y - distance * 0.25, centre.z + distance)),
}
for name, location in views.items():
    camera.location = location
    point_camera(camera, centre)
    scene.render.filepath = os.path.join(output_directory, f"{name}.png")
    bpy.ops.render.render(write_still=True)

stats = {
    "source": os.path.basename(source),
    "vertices": vertices,
    "triangles": triangles,
    "meshParts": parts,
    "boundsBlender": {
        "minimum": list(minimum),
        "maximum": list(maximum),
        "dimensions": list(dimensions),
    },
    "reviewViews": list(views.keys()),
    "reviewMode": review_mode,
}
with open(os.path.join(output_directory, "geometry-review.json"), "w", encoding="utf-8") as handle:
    json.dump(stats, handle, indent=2)
