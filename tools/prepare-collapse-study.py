"""Blender CPU study for the gated 1.6.2 collapse; never exports runtime assets.

blender --background --python tools/prepare-collapse-study.py -- --source <gltf> --output <dir>
The JSON contains evaluated mesh projections, not generic plan placeholders.
"""
import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
args.output.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(args.source))
source_objects = [o for o in bpy.context.scene.objects if o.type == "MESH"]
if len(source_objects) != 1:
    raise RuntimeError("Expected one inspected boulder mesh")
rock = source_objects[0]
source_triangles = sum(len(p.vertices) - 2 for p in rock.data.polygons)
source_bounds = list(rock.dimensions)
bpy.context.view_layer.objects.active = rock
rock.select_set(True)
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
# The download splits coincident vertices at exported UV/normal seams. Weld
# geometric duplicates on this study copy so collapse can reduce the connected
# surface; corner UVs remain per-loop. Originals are never modified.
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.remove_doubles(threshold=0.00001)
bpy.ops.object.mode_set(mode="OBJECT")
decimate = rock.modifiers.new("ReferenceOnlyMobileStudy", "DECIMATE")
decimate.ratio = min(1.0, 2200 / source_triangles)
bpy.ops.object.modifier_apply(modifier=decimate.name)
rock.name = "boulder-study"
rock.data.materials.clear()
rock.color = (0.37, 0.37, 0.37, 1)

def normalize(o, dimensions):
    current = o.dimensions
    o.scale = tuple(dimensions[i] / current[i] for i in range(3))
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    minz = min(v.co.z for v in o.data.vertices)
    for v in o.data.vertices:
        v.co.z -= minz

normalize(rock, (1.65, 1.18, 1.48))

def stone(name, dims, skew):
    bpy.ops.mesh.primitive_cube_add()
    o = bpy.context.object
    o.name = name
    o.dimensions = dims
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    # Broken corners and a sheared face are geometry, not a mortar pattern.
    for v in o.data.vertices:
        if v.co.x > 0 and v.co.y > 0:
            v.co.x -= skew
            v.co.z -= skew * 0.8
        v.co.z += dims[2] / 2
    bevel = o.modifiers.new("FractureEdges", "BEVEL")
    bevel.width = 0.025
    bevel.segments = 1
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    o.color = (0.55, 0.55, 0.55, 1)
    return o

masonry = stone("broken-dressed-stone-study", (0.65, 0.42, 0.38), 0.12)
lintel = stone("broken-lintel-study", (1.65, 0.38, 0.42), 0.22)
props = [rock, masonry, lintel]
scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.render.resolution_x = 640
scene.render.resolution_y = 640
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.world = bpy.data.worlds.new("StudyWorld")
scene.world.color = (0.8, 0.8, 0.8)
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "OBJECT"
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.display.shading.cavity_type = "BOTH"
scene.display.shading.background_type = "WORLD"
bpy.ops.object.camera_add(location=(3, -4, 2.5))
camera = bpy.context.object
scene.camera = camera
camera.data.type = "ORTHO"
camera.data.ortho_scale = 2.6
camera.rotation_euler = (Vector((0, 0, 0.65)) - camera.location).to_track_quat("-Z", "Y").to_euler()
for o in props:
    o.hide_render = True
    o.location = (0, 0, 0)
for o in props:
    o.hide_render = False
    scene.render.filepath = str(args.output / (o.name + ".png"))
    bpy.ops.render.render(write_still=True)
    o.hide_render = True

# Coordinates below use Horde x/y(up)/z; Blender is x/-z/y(up).
placements = [
    (rock, "dominant-left", [-0.85, -0.95, 4.10], [1.05, 1.0, 1.0], -0.30),
    (rock, "dominant-right", [0.78, -0.95, 4.37], [0.93, 0.80, 0.95], 0.72),
    (rock, "upper-interlocked", [0.06, -0.20, 4.43], [0.55, 0.64, 0.58], -0.85),
    (lintel, "tilted-lintel", [0.38, -0.42, 3.90], [1, 1, 1], 0.26),
    (masonry, "fallen-left", [-1.25, -0.95, 3.57], [1, 1, 1], -0.3),
    (masonry, "fallen-right", [1.21, -0.95, 3.68], [1, 1, 1], 0.5),
    (masonry, "masonry-interlock", [-0.12, 0.33, 4.05], [1, 1, 0.80], 0.9),
]
projected = []
for template, name, position, scale, rotation in placements:
    o = template.copy()
    o.data = template.data.copy()
    bpy.context.collection.objects.link(o)
    o.name = name
    o.location = (position[0], -position[2], position[1])
    o.scale = (scale[0], scale[2], scale[1])
    o.rotation_euler.z = rotation
    o.hide_render = True
    bpy.context.view_layer.update()
    vertices = [o.matrix_world @ v.co for v in o.data.vertices]
    projected.append({
        "id": name, "assetId": template.name, "position": position,
        "scale": scale, "yawRadians": rotation,
        "vertices": [[v.x, v.z, -v.y] for v in vertices],
        "faces": [list(p.vertices) for p in o.data.polygons],
    })
(args.output / "study-meshes.json").write_text(json.dumps({
    "studyOnly": True, "sourceTriangles": source_triangles,
    "sourceBoundsBlenderMeters": source_bounds,
    "referenceRockTriangles": sum(len(p.vertices)-2 for p in rock.data.polygons),
    "placements": projected,
}, indent=2))
print(json.dumps({"sourceTriangles": source_triangles, "sourceBounds": source_bounds,
                  "studyRockTriangles": sum(len(p.vertices)-2 for p in rock.data.polygons)}))
