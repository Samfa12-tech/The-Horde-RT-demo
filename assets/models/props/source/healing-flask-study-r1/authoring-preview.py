"""Save an editable Blender source and isolated authoring renders, new files only.

This imports the generated source GLB. It neither uses the glTF image exporter
nor produces a runtime asset/package. The studio lights/floor never enter Horde.
"""

import argparse
import hashlib
import json
from pathlib import Path
import sys

import bpy
from mathutils import Vector

SOURCE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output-directory", type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
output = args.output_directory.resolve()
source_glb = SOURCE / "healing-flask-study-r1.glb"
editable = SOURCE / "healing-flask-study-r1.blend"
names = ("three-quarter.png", "front.png", "side.png")
if editable.exists() or any((output/name).exists() for name in (*names, "preview-receipt.json")):
    raise RuntimeError("New-only authoring: preserve existing source/preview outputs")
output.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(source_glb))
model = bpy.data.objects.get("HealingFlaskStudy")
grip = bpy.data.objects.get("Grip")
if model is None or grip is None or len(model.data.polygons) != 384:
    raise RuntimeError("Unexpected imported source mesh/Grip")
grip.empty_display_type = "ARROWS"
grip.empty_display_size = 0.025
model["admission"] = "source-only; native/hand/RT validation pending"
model["source_glb_sha256"] = hashlib.sha256(source_glb.read_bytes()).hexdigest()
bpy.ops.mesh.primitive_plane_add(size=2, location=(0, 0, -0.0752))
floor = bpy.context.object
floor.name = "AuthoringStudioFloor_NOT_RUNTIME"
material = bpy.data.materials.new("AuthoringStudioFloor")
material.diffuse_color = (0.075, 0.085, 0.10, 1)
material.use_nodes = True
shader = material.node_tree.nodes.get("Principled BSDF")
shader.inputs["Base Color"].default_value = material.diffuse_color
shader.inputs["Roughness"].default_value = 0.72
floor.data.materials.append(material)


def aim(obj, target):
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat("-Z", "Y").to_euler()


for name, location, energy, size in (
        ("StudioKey", (0.15, -0.24, 0.30), 12.0, 0.24),
        ("StudioFill", (-0.22, -0.05, 0.08), 6.0, 0.20),
        ("StudioRim", (0.0, 0.20, 0.19), 15.0, 0.15)):
    data = bpy.data.lights.new(name, "AREA")
    data.energy, data.shape, data.size = energy, "DISK", size
    light = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(light)
    light.location = location
    aim(light, (0,0,0))
camera_data = bpy.data.cameras.new("AuthoringCamera")
camera_data.type, camera_data.ortho_scale = "ORTHO", 0.225
camera = bpy.data.objects.new("AuthoringCamera", camera_data)
bpy.context.collection.objects.link(camera)
scene = bpy.context.scene
scene.camera = camera
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 24
scene.render.resolution_x, scene.render.resolution_y = 512, 640
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.world.use_nodes = True
scene.world.node_tree.nodes["Background"].inputs[0].default_value = (0.12,0.14,0.18,1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value = 0.35
views = (("three-quarter.png", (0.24,-0.32,0.12)),
         ("front.png", (0,-0.32,0.012)), ("side.png", (0.32,0,0.012)))
for name, location in views:
    camera.location = location
    aim(camera, (0,0,0.006))
    scene.render.filepath = str(output/name)
    bpy.ops.render.render(write_still=True)
camera.location = views[0][1]
aim(camera, (0,0,0.006))
scene.render.filepath = "//authoring-preview.png"
bpy.ops.object.select_all(action="DESELECT")
model.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.wm.save_as_mainfile(filepath=str(editable))
receipt = {"scope": "Blender authoring preview only, not Horde RT evidence",
           "sourceGlbSha256": hashlib.sha256(source_glb.read_bytes()).hexdigest(),
           "editableBlendSha256": hashlib.sha256(editable.read_bytes()).hexdigest(),
           "engine": "Cycles CPU", "resolution": [512,640], "samples": 24,
           "views": [{"file": name, "bytes": (output/name).stat().st_size,
                      "sha256": hashlib.sha256((output/name).read_bytes()).hexdigest()}
                     for name,_ in views],
           "glTfExporterInvoked": False, "gameOrPhoneInvoked": False}
(output/"preview-receipt.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
print(json.dumps(receipt,indent=2))
