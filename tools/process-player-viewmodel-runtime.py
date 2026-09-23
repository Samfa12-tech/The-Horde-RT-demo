"""Derive modelled arms from the accepted player pipeline, never from screen geometry.

Run with Blender --background --threads 1 --python-exit-code 1 --python this.py -- NEW_OUTPUT_DIRECTORY.
The world-reference and arms-only GLBs share one authored rig/animation evaluation.
No source or currently admitted runtime file is overwritten.

Investigation-only switches (not admission or production defaults):
  --correct-grip-roll: test a 180-degree Grip roll, with a paired world GLB.
  --stabilize-sleeves: transfer sleeve Hand weights to the same-side ForeArm.
Both retain gameplay sockets/prop authority. Neither establishes visual acceptance;
the combined candidate still has visible sleeve defects in native RT captures.
"""
import hashlib
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
import bmesh

root = Path(__file__).resolve().parents[1]
arguments = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
stabilize_sleeves = '--stabilize-sleeves' in arguments
correct_grip_roll = '--correct-grip-roll' in arguments
arguments = [argument for argument in arguments if argument not in ('--stabilize-sleeves', '--correct-grip-roll')]
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
calibrated_world = None
if correct_grip_roll:
    # Investigation-only paired rig candidate. Keep the accepted world/runtime
    # untouched, and keep Grip origins/axes and gameplay prop transforms fixed.
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode='EDIT')
    for name in ('LeftGrip', 'RightGrip'):
        rig.data.edit_bones[name].roll += math.pi
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()
    player.select_set(True)
    calibrated_world = output / 'world-grip-calibrated.runtime.glb'
    bpy.ops.export_scene.gltf(filepath=str(calibrated_world), export_format='GLB', use_selection=True,
                              export_tangents=True, export_animations=True, export_animation_mode='NLA_TRACKS',
                              export_frame_range=True, export_skins=True, export_morph=False,
                              export_cameras=False, export_lights=False, export_extras=True)
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
weight_corrections = {}
if stabilize_sleeves:
    # A rigid anatomical glove follows Hand; cloth stops at the wrist and
    # follows ForeArm. Mixing those palettes stretched the old wrist seam >5x
    # under the real grip orientation. Change only this independent viewmodel.
    sleeve_vertices = {index for polygon in player.data.polygons
                       if polygon.material_index == 0 for index in polygon.vertices}
    gauntlet_vertices = {index for polygon in player.data.polygons
                         if polygon.material_index == 1 for index in polygon.vertices}
    if sleeve_vertices & gauntlet_vertices:
        raise RuntimeError('Sleeve reweight must not touch a gauntlet vertex')
    for side in ('Left', 'Right'):
        hand = player.vertex_groups[side + 'Hand']
        forearm = player.vertex_groups[side + 'ForeArm']
        changed = 0
        for index in sorted(sleeve_vertices):
            weights = {group.group: group.weight for group in player.data.vertices[index].groups}
            amount = weights.get(hand.index, 0.0)
            if amount > 0.0:
                forearm.add([index], weights.get(forearm.index, 0.0) + amount, 'REPLACE')
                hand.remove([index])
                changed += 1
        weight_corrections[side] = changed
    if not all(weight_corrections.values()):
        raise RuntimeError('Expected both sleeves to contain the diagnosed hand-weight seam')
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
              gripRollCorrectionRadians=math.pi if correct_grip_roll else 0.0,
              pairedWorldRuntime=calibrated_world.name if calibrated_world else None,
              pairedWorldSha256=sha(calibrated_world) if calibrated_world else None,
              sleeveWeightMode='ArmForeArm' if stabilize_sleeves else 'OriginalArmForeArmHand',
              sleeveHandWeightsMovedToForearm=weight_corrections,
              runtime=viewmodel_output.name, runtimeSha256=sha(viewmodel_output),
              primitiveSemantics=counts, processingVertices=len(player.data.vertices),
              ownership='Primary-only modelled geometry; world body owns secondary visibility',
              animationAuthority='Same authored rig, Idle/Walking NLA tracks and named grip bones as world reference',
              licence='Derivative of existing licensed Meshy player/gauntlet inputs; see ASSET_LICENSES.md',
              status='Offline candidate only; GPU ownership, grip/pitch/motion and owner acceptance not yet established')
(output / 'viewmodel-processing.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
