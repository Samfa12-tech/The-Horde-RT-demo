"""Geometry-only Blender inspection of a native CPU-upload OBJ; not game RT proof."""
import json
from pathlib import Path
import sys
import bpy
from mathutils import Vector

arguments = sys.argv[sys.argv.index('--') + 1:]
source, destination = map(Path, arguments[:2])
gauntlets_only = len(arguments) > 2 and arguments[2] == 'gauntlets'
if destination.exists():
    raise RuntimeError('Use a new inspection output directory')
destination.mkdir(parents=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
positions, groups, active = [], {}, None
for line in source.read_text().splitlines():
    parts = line.split()
    if not parts:
        continue
    if parts[0] == 'v':
        x, y, z = map(float, parts[1:4])
        positions.append((x, -z, y))
    elif parts[0] == 'g':
        active = parts[1]
        groups[active] = []
    elif parts[0] == 'f':
        groups[active].append(tuple(int(value.split('/')[0]) - 1 for value in parts[1:]))
if gauntlets_only:
    groups = {name: faces for name, faces in groups.items() if 'Gauntlets' in name}
for name, faces in groups.items():
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(positions, [], faces)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    material = bpy.data.materials.new(name)
    material.diffuse_color = ((0.13, 0.42, 0.65, 1)
        if 'Sleeves' in name or name == 'BodyPrimaryVisible' else
        (0.70, 0.37, 0.12, 1) if name == 'NearFacePrimaryMasked' else
        (0.42, 0.20, 0.52, 1) if name == 'HeadPrimaryMasked' else
        (0.45, 0.45, 0.45, 1))
    obj.data.materials.append(material)
    for face in mesh.polygons:
        face.use_smooth = False
used = {index for faces in groups.values() for face in faces for index in face}
minimum = Vector([min(positions[index][axis] for index in used) for axis in range(3)])
maximum = Vector([max(positions[index][axis] for index in used) for axis in range(3)])
center = (minimum + maximum) * .5
scene = bpy.context.scene
scene.render.engine = 'BLENDER_WORKBENCH'
scene.render.resolution_x = 800
scene.render.resolution_y = 700
scene.render.resolution_percentage = 100
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_shadows = True
scene.display.shading.show_cavity = True
scene.display.shading.background_type = 'WORLD'
scene.world.color = (.08, .08, .08)
bpy.ops.object.camera_add()
camera = bpy.context.object
scene.camera = camera
camera.data.type = 'ORTHO'
camera.data.ortho_scale = max(maximum - minimum) * 1.3
for name, direction in [('front', Vector((0, -4, 1))), ('side', Vector((4, 0, .7))), ('player-side', Vector((0, 4, 1.0)))]:
    camera.location = center + direction
    camera.rotation_euler = (center - camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = str(destination / (name + '.png'))
    bpy.ops.render.render(write_still=True)
print(json.dumps(dict(source=str(source), space='model metres converted Y-up to Blender Z-up',
    minimum=list(minimum), maximum=list(maximum), groups={key: len(value) for key, value in groups.items()})))
