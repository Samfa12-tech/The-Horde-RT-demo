import bpy
import os
import sys


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python resize-runtime-texture.py -- input.png output.png width height")
source, destination, width, height = sys.argv[sys.argv.index("--") + 1:]
source = os.path.abspath(source)
destination = os.path.abspath(destination)
image = bpy.data.images.load(source, check_existing=False)
image.scale(int(width), int(height))
image.filepath_raw = destination
image.file_format = "PNG"
image.save()
