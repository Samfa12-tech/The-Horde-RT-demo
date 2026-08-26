import bpy
import numpy as np
import os
import sys


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python extract-held-item-pbr.py -- input.glb output_directory")
source, output_directory = sys.argv[sys.argv.index("--") + 1:]
source = os.path.abspath(source)
output_directory = os.path.abspath(output_directory)
os.makedirs(output_directory, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")
materials = [material for material in bpy.data.materials if material.use_nodes]
if not materials:
    raise RuntimeError("textured held item contains no node material")
material = materials[0]
principled = next((node for node in material.node_tree.nodes if node.type == "BSDF_PRINCIPLED"), None)
if principled is None:
    raise RuntimeError("textured held item contains no Principled BSDF")

def upstream_images(socket, visited=None):
    visited = set() if visited is None else visited
    found = []
    for link in socket.links:
        node = link.from_node
        if node in visited:
            continue
        visited.add(node)
        if node.type == "TEX_IMAGE" and node.image is not None:
            found.append(node.image)
        for input_socket in node.inputs:
            found.extend(upstream_images(input_socket, visited))
    return found

def first_image(socket_name):
    images = upstream_images(principled.inputs[socket_name])
    return images[0] if images else None

def pixels(image):
    width, height = image.size
    values = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(values)
    return values.reshape((-1, 4)), width, height

def save_rgba(name, values, width, height, colour_space="sRGB"):
    image = bpy.data.images.new(name, width=width, height=height, alpha=True, float_buffer=False)
    image.colorspace_settings.name = colour_space
    image.pixels.foreach_set(values.ravel())
    image.filepath_raw = os.path.join(output_directory, f"{name}.png")
    image.file_format = "PNG"
    image.save()

base = first_image("Base Color")
normal = first_image("Normal")
metallic = first_image("Metallic")
roughness = first_image("Roughness")
if base is None or normal is None or metallic is None or roughness is None:
    raise RuntimeError("Meshy PBR source is missing base-color, normal, metallic, or roughness image wiring")

base_values, width, height = pixels(base)
save_rgba("base-color", base_values, width, height)
normal_values, normal_width, normal_height = pixels(normal)
save_rgba("normal", normal_values, normal_width, normal_height, "Non-Color")

metal_values, metal_width, metal_height = pixels(metallic)
rough_values, rough_width, rough_height = pixels(roughness)
if (metal_width, metal_height) != (rough_width, rough_height):
    raise RuntimeError("Meshy metallic and roughness source dimensions differ")

def scalar_map(name, source_values, channel, width, height):
    values = np.ones((width * height, 4), dtype=np.float32)
    scalar = source_values[:, channel]
    values[:, 0] = scalar
    values[:, 1] = scalar
    values[:, 2] = scalar
    save_rgba(name, values, width, height, "Non-Color")

# glTF metallic-roughness uses B for metallic and G for roughness. Meshy may
# wire the same combined image to both Principled inputs; these channels remain
# correct even when Blender inserts Separate Color nodes during import.
scalar_map("metallic", metal_values, 2, metal_width, metal_height)
scalar_map("roughness", rough_values, 1, rough_width, rough_height)
black = np.array([[0.0, 0.0, 0.0, 1.0]], dtype=np.float32)
save_rgba("emissive", black, 1, 1)
