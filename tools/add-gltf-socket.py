import bpy
import pathlib
import sys


def arguments_after_separator():
    if "--" not in sys.argv:
        raise RuntimeError("usage: blender --background --python add-gltf-socket.py -- input.glb output.glb socket x y z")
    return sys.argv[sys.argv.index("--") + 1:]


source, destination, socket_name, x, y, z = arguments_after_separator()
bpy.ops.wm.read_factory_settings(use_empty=True)
result = bpy.ops.import_scene.gltf(filepath=str(pathlib.Path(source).resolve()))
if "FINISHED" not in result:
    raise RuntimeError(f"Blender failed to import {source}")

socket = bpy.data.objects.new(socket_name, None)
socket.empty_display_type = "PLAIN_AXES"
socket.location = (float(x), float(y), float(z))
bpy.context.scene.collection.objects.link(socket)

# Runtime textures live in audited KTX2 arrays. Keep tiny valid placeholders in
# the GLB so material texture indices survive without carrying 2K source art.
for image in bpy.data.images:
    if image.size[0] > 4 or image.size[1] > 4:
        image.scale(4, 4)

result = bpy.ops.export_scene.gltf(
    filepath=str(pathlib.Path(destination).resolve()),
    export_format="GLB",
    export_tangents=True,
    export_animations=False,
    export_skins=False,
    export_morph=False,
    export_cameras=False,
    export_lights=False,
)
if "FINISHED" not in result:
    raise RuntimeError(f"Blender failed to export {destination}")
