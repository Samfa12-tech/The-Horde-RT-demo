"""Derive modelled arms from the accepted player pipeline, never from screen geometry.

Run with Blender --background --threads 1 --python-exit-code 1 --python this.py -- NEW_OUTPUT_DIRECTORY.
The world-reference and arms-only GLBs share one authored rig/animation evaluation.
No source or currently admitted runtime file is overwritten.
"""
import hashlib
import json
from pathlib import Path
import runpy
import sys

import bpy
import bmesh

root = Path(__file__).resolve().parents[1]
arguments = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
if len(arguments) != 1:
    raise RuntimeError('Expected one new output directory')
output = Path(arguments[0]).resolve()
if output.exists():
    raise RuntimeError('Output directory already exists; source and prior results are never overwritten')
output.mkdir(parents=True)
world_reference = output / 'world-reference.glb'
viewmodel_output = output / 'gothic-traveller-viewmodel.runtime.glb'
source = root / 'assets/models/player/source/meshy-2026-08-26-gothic-traveller-candidate-2'
saved_arguments = sys.argv
sys.argv = ['blender', '--', str(source / 'player-rigged.glb'), str(source / 'player-walking.glb'),
            str(root / 'assets/textures/player/source'),
            str(root / 'assets/models/player/source/meshy-2026-08-30-viewmodel-gauntlet/right-gauntlet-5k-stripped.glb'),
            str(world_reference)]
try:
    world = runpy.run_path(str(root / 'tools/process-player-rig-runtime.py'), run_name='__main__')
finally:
    sys.argv = saved_arguments
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
accepted_world = root / 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
if sha(world_reference) != sha(accepted_world):
    raise RuntimeError('World reference differs from the admitted rig; reconcile inputs/Blender threading first')

player, rig = world['player'], world['rig']
parts = [('BodyPrimaryVisible', 'ViewmodelSleeves'), ('GauntletPrimaryVisible', 'ViewmodelGauntlets')]
old_names = [material.name for material in player.data.materials]
kept_indices = [old_names.index(name) for name, _ in parts]
materials = [player.data.materials[index].copy() for index in kept_indices]
player.data = player.data.copy()
mesh = bmesh.new()
try:
    mesh.from_mesh(player.data)
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face.material_index not in kept_indices], context='FACES')
    for face in mesh.faces:
        face.material_index = kept_indices.index(face.material_index)
    mesh.to_mesh(player.data)
finally:
    mesh.free()
polygon_materials = [polygon.material_index for polygon in player.data.polygons]
player.data.materials.clear()
for material, (_, new_name) in zip(materials, parts):
    material.name = new_name
    player.data.materials.append(material)
for polygon, material_index in zip(player.data.polygons, polygon_materials):
    polygon.material_index = material_index
player.name = 'PlayerViewmodel'
player.data.update()
counts = {name: 0 for _, name in parts}
for polygon in player.data.polygons:
    counts[parts[polygon.material_index][1]] += len(polygon.vertices) - 2
if counts != {'ViewmodelSleeves': 5532, 'ViewmodelGauntlets': 8838}:
    raise RuntimeError(f'Unexpected authored arms partition: {counts}')
bpy.ops.object.select_all(action='DESELECT')
player.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.gltf(filepath=str(viewmodel_output), export_format='GLB', use_selection=True,
                          export_tangents=True, export_animations=True, export_animation_mode='NLA_TRACKS',
                          export_frame_range=True, export_skins=True, export_morph=False,
                          export_cameras=False, export_lights=False, export_extras=True)
report = dict(schema=1, role='Viewmodel', sourceWorldSha256=sha(accepted_world),
              runtime=viewmodel_output.name, runtimeSha256=sha(viewmodel_output),
              primitiveSemantics=counts, processingVertices=len(player.data.vertices),
              ownership='Primary-only modelled geometry; world body owns secondary visibility',
              animationAuthority='Same authored rig, Idle/Walking NLA tracks and named grip bones as world reference',
              licence='Derivative of existing licensed Meshy player/gauntlet inputs; see ASSET_LICENSES.md',
              status='Offline candidate only; GPU ownership, grip/pitch/motion and owner acceptance not yet established')
(output / 'viewmodel-processing.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
