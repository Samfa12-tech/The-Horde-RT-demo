import bpy
import numpy as np
import pathlib
import sys


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python compile-orm-texture.py -- roughness.png metallic.png output.png")
roughness_path, metallic_path, output_path = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
roughness = bpy.data.images.load(str(pathlib.Path(roughness_path).resolve()), check_existing=False)
metallic = bpy.data.images.load(str(pathlib.Path(metallic_path).resolve()), check_existing=False)
width, height = roughness.size
if tuple(metallic.size) != (width, height):
    raise RuntimeError("roughness and metallic texture dimensions do not match")

rough_pixels = np.empty(width * height * 4, dtype=np.float32)
metal_pixels = np.empty(width * height * 4, dtype=np.float32)
roughness.pixels.foreach_get(rough_pixels)
metallic.pixels.foreach_get(metal_pixels)
rough_pixels = rough_pixels.reshape((-1, 4))
metal_pixels = metal_pixels.reshape((-1, 4))
packed = np.empty((width * height, 4), dtype=np.float32)
packed[:, 0] = 1.0
packed[:, 1] = rough_pixels[:, 0]
packed[:, 2] = metal_pixels[:, 0]
packed[:, 3] = 1.0

output = bpy.data.images.new("runtime-orm", width=width, height=height, alpha=True, float_buffer=False)
output.colorspace_settings.name = "Non-Color"
output.pixels.foreach_set(packed.ravel())
output.filepath_raw = str(pathlib.Path(output_path).resolve())
output.file_format = "PNG"
output.save()
