import bpy
import json
import numpy as np
import os
import sys


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python grade-held-item-pbr.py -- INPUT_DIRECTORY OUTPUT_DIRECTORY torch"
    )
source_directory, output_directory, kind = sys.argv[sys.argv.index("--") + 1:]
if kind != "torch":
    raise RuntimeError("only the reviewed torch material correction is supported")
source_directory = os.path.abspath(source_directory)
output_directory = os.path.abspath(output_directory)
os.makedirs(output_directory, exist_ok=True)


def load_pixels(name):
    image = bpy.data.images.load(os.path.join(source_directory, f"{name}.png"), check_existing=False)
    width, height = image.size
    values = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(values)
    return values.reshape((-1, 4)), width, height


def save_pixels(name, values, width, height, colour_space):
    image = bpy.data.images.new(name, width=width, height=height, alpha=True, float_buffer=False)
    image.colorspace_settings.name = colour_space
    image.pixels.foreach_set(values.ravel())
    image.filepath_raw = os.path.join(output_directory, f"{name}.png")
    image.file_format = "PNG"
    image.save()


base, width, height = load_pixels("base-color")
metallic, metallic_width, metallic_height = load_pixels("metallic")
roughness, roughness_width, roughness_height = load_pixels("roughness")
if (metallic_width, metallic_height) != (width, height) or (roughness_width, roughness_height) != (width, height):
    raise RuntimeError("torch source maps must have identical dimensions")

# Meshy's accepted candidate-1 geometry arrived with overly clean pale metal
# and light varnished wood despite the approved texture prompt. Grade those
# measured PBR regions without painting illumination: metallic texels become
# blackened forged iron and dielectric texels become dark worn hardwood/char.
metal = np.clip(metallic[:, 0], 0.0, 1.0)
luminance = np.clip(base[:, :3].mean(axis=1), 0.0, 1.0)
iron = np.stack((0.055 + luminance * 0.12,
                 0.050 + luminance * 0.105,
                 0.047 + luminance * 0.09), axis=1)
wood = np.stack((0.045 + luminance * 0.24,
                 0.022 + luminance * 0.105,
                 0.012 + luminance * 0.055), axis=1)
blend = np.clip((metal - 0.18) / 0.42, 0.0, 1.0)[:, None]
base[:, :3] = wood * (1.0 - blend) + iron * blend
base[:, 3] = 1.0

# Preserve the authored metallic mask while increasing the roughness floor for
# soot, fibre, and hand wear. This remains neutral PBR with no baked light.
roughness[:, :3] = np.maximum(roughness[:, :3], 0.48 + (1.0 - blend) * 0.16)
roughness[:, 3] = 1.0
save_pixels("base-color", base, width, height, "sRGB")
save_pixels("metallic", metallic, width, height, "Non-Color")
save_pixels("roughness", roughness, width, height, "Non-Color")

normal, normal_width, normal_height = load_pixels("normal")
save_pixels("normal", normal, normal_width, normal_height, "Non-Color")
emissive = np.array([[0.0, 0.0, 0.0, 1.0]], dtype=np.float32)
save_pixels("emissive", emissive, 1, 1, "Non-Color")

with open(os.path.join(output_directory, "material-review.json"), "w", encoding="utf-8") as handle:
    json.dump({
        "source": os.path.basename(source_directory),
        "kind": kind,
        "operation": "region-preserving neutral PBR grade",
        "metal": "blackened forged iron, authored metallic mask preserved",
        "dielectric": "dark worn hardwood and char, no baked lighting",
        "roughnessFloor": "0.48 metal to 0.64 dielectric",
        "emissive": "black 1x1",
    }, handle, indent=2)
