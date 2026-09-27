"""Derive modelled arms from the accepted player pipeline, never from screen geometry.

Run with Blender --background --threads 1 --python-exit-code 1 --python this.py -- NEW_OUTPUT_DIRECTORY.
The world-reference and arms-only GLBs share one authored rig/animation evaluation.
No source or currently admitted runtime file is overwritten.

Investigation-only switches (not admission or production defaults):
  --gauntlet-source-hand Right: owner-corrected source anatomy, with paired world export.
  --gauntlet-scale FACTOR: size gauntlet geometry about its authored GripOrigin,
    with the same paired world/viewmodel scale.
  --correct-grip-roll: test a 180-degree Grip roll, with a paired world GLB.
  --grip-roll-degrees LEFT RIGHT: test explicit per-hand authored Grip calibration.
  --stabilize-sleeves: transfer sleeve Hand weights to the same-side ForeArm.
  --blend-elbows: test a continuous elbow-centred sleeve weight field (implies stabilization).
  --fit-sleeves: fit the retained cloth surface to a bounded anatomical arm envelope.
  --close-sleeves: close the authored garment openings with real offline cloth panels.
  --reconcile-sleeve-seams: copy accepted view weights to coincident world cloth
    seam vertices, retaining geometry, UVs and separate world/view ownership.
  --body-remainder: add an explicit world connecting-cloth/torso/legs region
    with a paired WorldBody manifest; preserve the dedicated arm extraction.
  --retain-upper-torso: keep the old near-face torso cloth in that remainder,
    preserving the existing head mask and all geometry/weights.
  --reconcile-segmented-seams: split proven coarse world cloth edges at the
    unchanged view sleeve midpoints before exact boundary-weight transfer.
These retain gameplay sockets/prop authority. None establishes visual acceptance;
the candidates still require anatomical, surface and live-motion validation.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
import bmesh

root = Path(__file__).resolve().parents[1]
MIN_GAUNTLET_SCALE = 0.080
MAX_GAUNTLET_SCALE = 0.105
arguments = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
parser.add_argument('--blend-elbows', action='store_true')
parser.add_argument('--fit-sleeves', action='store_true')
parser.add_argument('--close-sleeves', action='store_true')
parser.add_argument('--reconcile-sleeve-seams', action='store_true')
parser.add_argument('--body-remainder', action='store_true')
parser.add_argument('--retain-upper-torso', action='store_true')
parser.add_argument('--reconcile-segmented-seams', action='store_true')
parser.add_argument('--stabilize-sleeves', action='store_true')
parser.add_argument('--gauntlet-source-hand', choices=('Left', 'Right'),
                    help='Explicit anatomical source handedness; omission reproduces the historical export')
parser.add_argument('--gauntlet-scale', type=float,
                    help='Uniform source scale applied about the authored grip origin')
roll_options = parser.add_mutually_exclusive_group()
roll_options.add_argument('--correct-grip-roll', action='store_true')
roll_options.add_argument('--grip-roll-degrees', nargs=2, type=float, metavar=('LEFT', 'RIGHT'))
options = parser.parse_args(arguments)
if options.body_remainder and not options.reconcile_sleeve_seams:
    parser.error('Body remainder requires the demonstrated shared-seam reconciliation')
if options.retain_upper_torso and not options.body_remainder:
    parser.error('Upper torso retention requires the explicit body remainder')
if options.reconcile_segmented_seams and not options.reconcile_sleeve_seams:
    parser.error('Segmented seam repair requires exact sleeve seam reconciliation')
if options.reconcile_sleeve_seams and (options.fit_sleeves or options.close_sleeves):
    parser.error('Seam reconciliation requires unchanged source positions/topology, not fitted or capped sleeves')
roll_degrees = options.grip_roll_degrees or ([180.0, 180.0] if options.correct_grip_roll else [0.0, 0.0])
if any(not math.isfinite(value) or abs(value) > 360.0 for value in roll_degrees):
    parser.error('Grip roll must be finite and within [-360, 360] degrees')
if options.gauntlet_scale is not None and (
        not math.isfinite(options.gauntlet_scale) or
        not MIN_GAUNTLET_SCALE <= options.gauntlet_scale <= MAX_GAUNTLET_SCALE):
    parser.error(
        f'Gauntlet scale must be finite and within '
        f'[{MIN_GAUNTLET_SCALE:.3f}, {MAX_GAUNTLET_SCALE:.3f}]')
grip_rolls = dict(zip(('Left', 'Right'), map(math.radians, roll_degrees)))
blend_elbows = options.blend_elbows or options.fit_sleeves or options.close_sleeves
stabilize_sleeves = options.stabilize_sleeves or blend_elbows
correct_grip_roll = any(grip_rolls.values())
output = options.output.resolve()
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

paired_gauntlet_world = None
if options.gauntlet_source_hand or options.gauntlet_scale is not None or options.body_remainder:
    world_name = 'world-chirality-corrected.runtime.glb' if options.gauntlet_source_hand else \
        'world-gauntlet-size-candidate.runtime.glb'
    paired_gauntlet_world = output / world_name
    paired_arguments = ['blender', '--', str(source / 'player-rigged.glb'), str(source / 'player-walking.glb'),
                        str(root / 'assets/textures/player/source'),
                        str(root / 'assets/models/player/source/meshy-2026-08-30-viewmodel-gauntlet/right-gauntlet-5k-stripped.glb'),
                        str(paired_gauntlet_world)]
    if options.gauntlet_source_hand:
        paired_arguments.extend(['--gauntlet-source-hand', options.gauntlet_source_hand])
    if options.gauntlet_scale is not None:
        paired_arguments.extend(['--gauntlet-scale', format(options.gauntlet_scale, '.17g')])
    if options.body_remainder:
        paired_arguments.append('--body-remainder')
    if options.retain_upper_torso:
        paired_arguments.append('--retain-upper-torso')
    sys.argv = paired_arguments
    try:
        world = runpy.run_path(str(root / 'tools/process-player-rig-runtime.py'), run_name='__main__')
    finally:
        sys.argv = saved_arguments
player, rig = world['player'], world['rig']
gauntlet_scale = world['gauntlet_scale']
if options.gauntlet_scale is not None and not math.isclose(
        gauntlet_scale, options.gauntlet_scale, rel_tol=0.0, abs_tol=1.0e-12):
    raise RuntimeError('Paired world gauntlet scale disagrees with the viewmodel request')
calibrated_world = paired_gauntlet_world
if correct_grip_roll:
    # Investigation-only paired rig candidate. Keep the accepted world/runtime
    # untouched, and keep Grip origins/axes and gameplay prop transforms fixed.
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode='EDIT')
    for side, roll in grip_rolls.items():
        rig.data.edit_bones[side + 'Grip'].roll += roll
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()
    player.select_set(True)
    calibrated_world = output / 'world-grip-calibrated.runtime.glb'
    bpy.ops.export_scene.gltf(filepath=str(calibrated_world), export_format='GLB', use_selection=True,
                              export_tangents=True, export_animations=True, export_animation_mode='NLA_TRACKS',
                              export_frame_range=True, export_skins=True, export_morph=False,
                              export_cameras=False, export_lights=False, export_extras=True)
# Retain the original mesh, without creating another object or datablock that
# could change default export naming. The viewmodel below always edits a copy.
seam_world_mesh = player.data if options.reconcile_sleeve_seams else None
seam_world_name = player.name
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
elbow_corrections = {}
sleeve_fit = {}
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
    if options.fit_sleeves:
        # Offline model-space tailoring, independent of camera and checkpoint.
        # Preserve topology/UVs and the original wrinkle directions; a monotone
        # radial map avoids collapsing multiple cloth layers onto one cylinder.
        original_normals = [tuple(normal.vector) for normal in player.data.corner_normals]
        inverse_mesh = player.matrix_world.inverted()
        for side in ('Left', 'Right'):
            group_ids = {player.vertex_groups[side + suffix].index for suffix in ('Arm', 'ForeArm', 'Hand')}
            shoulder = rig.matrix_world @ rig.data.bones[side + 'Arm'].head_local
            elbow = rig.matrix_world @ rig.data.bones[side + 'ForeArm'].head_local
            wrist = rig.matrix_world @ rig.data.bones[side + 'Hand'].head_local
            count, max_displacement = 0, 0.0
            for index in sorted(sleeve_vertices):
                vertex = player.data.vertices[index]
                if sum(g.weight for g in vertex.groups if g.group in group_ids) < 0.99:
                    continue
                point = player.matrix_world @ vertex.co
                candidates = []
                for start, end, first_radius, last_radius in (
                        (shoulder, elbow, 0.075, 0.055), (elbow, wrist, 0.055, 0.038)):
                    axis = end - start
                    t = max(0.0, min(1.0, (point - start).dot(axis) / axis.length_squared))
                    centre = start + axis * t
                    candidates.append(((point - centre).length, centre,
                                       first_radius * (1.0 - t) + last_radius * t))
                radius, centre, limit = min(candidates, key=lambda item: item[0])
                if radius > 1.0e-8:
                    fitted = centre + (point - centre) * (limit * math.tanh(radius / limit) / radius)
                    max_displacement = max(max_displacement, (fitted - point).length)
                    vertex.co = inverse_mesh @ fitted
                count += 1
            sleeve_fit[side] = dict(vertices=count, maximumDisplacementMetres=max_displacement)
        player.data.update()
        # Recompute cloth normals for the new real surface, retaining all
        # unmodified gauntlet corner normals rather than changing its shading.
        for polygon in player.data.polygons:
            if polygon.material_index == 0:
                for loop in polygon.loop_indices:
                    original_normals[loop] = (0.0, 0.0, 0.0)
        player.data.normals_split_custom_set(original_normals)
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
    if blend_elbows:
        for side in ('Left', 'Right'):
            arm = player.vertex_groups[side + 'Arm']
            forearm = player.vertex_groups[side + 'ForeArm']
            shoulder = rig.matrix_world @ rig.data.bones[side + 'Arm'].head_local
            elbow = rig.matrix_world @ rig.data.bones[side + 'ForeArm'].head_local
            wrist = rig.matrix_world @ rig.data.bones[side + 'Hand'].head_local
            upper, lower = elbow - shoulder, wrist - elbow
            direction = (upper.normalized() + lower.normalized()).normalized()
            half_width = min(upper.length, lower.length) * 0.25
            if half_width < 0.001 or direction.length < 0.99:
                raise RuntimeError('Invalid anatomical elbow blend frame')
            changed = 0
            for index in sorted(sleeve_vertices):
                weights = {group.group: group.weight for group in player.data.vertices[index].groups}
                total = weights.get(arm.index, 0.0) + weights.get(forearm.index, 0.0)
                if total < 0.99:
                    continue
                point = player.matrix_world @ player.data.vertices[index].co
                t = max(0.0, min(1.0, 0.5 + (point - elbow).dot(direction) / (2.0 * half_width)))
                blend = t * t * (3.0 - 2.0 * t)
                arm.remove([index])
                forearm.remove([index])
                if blend < 1.0:
                    arm.add([index], 1.0 - blend, 'REPLACE')
                if blend > 0.0:
                    forearm.add([index], blend, 'REPLACE')
                changed += 1
            elbow_corrections[side] = dict(vertices=changed, halfWidthMetres=half_width)
sleeve_closure = {}
if options.close_sleeves:
    from player_viewmodel_surface import close_sleeve_openings
    sleeve_closure = close_sleeve_openings(player)
counts = {name: 0 for _, name in parts}
for polygon in player.data.polygons:
    counts[parts[polygon.material_index][1]] += len(polygon.vertices) - 2
if (counts['ViewmodelGauntlets'] != 8838 or
        (not options.close_sleeves and counts['ViewmodelSleeves'] != 5532) or
        (options.close_sleeves and counts['ViewmodelSleeves'] <= 5532)):
    raise RuntimeError(f'Unexpected authored arms partition: {counts}')
viewmodel_manifest = json.loads((root / 'assets/models/player/viewmodel/runtime/asset.manifest.json').read_text(encoding='utf-8'))
if sum(counts.values()) > viewmodel_manifest['lods'][0]['maxTriangles']:
    raise RuntimeError('Viewmodel candidate exceeds the existing manifest triangle budget')
bpy.ops.object.select_all(action='DESELECT')
player.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.gltf(filepath=str(viewmodel_output), export_format='GLB', use_selection=True,
                          export_tangents=True, export_animations=True, export_animation_mode='NLA_TRACKS',
                          export_frame_range=True, export_skins=True, export_morph=False,
                          export_cameras=False, export_lights=False, export_extras=True)
seam_reconciliation = {}
segmented_seam_reconciliation = {}
if options.reconcile_sleeve_seams:
    from player_sleeve_seams import plan_sleeve_seam_weight_transfers

    # Export the accepted viewmodel BEFORE touching the world seam. No new
    # weight field is fitted; only the already chosen view endpoint weights
    # are transferred across demonstrated bind-space cloth adjacencies.
    view_mesh, view_name = player.data, player.name
    deform_names = {bone.name for bone in rig.data.bones}
    group_names = {group.index: group.name for group in player.vertex_groups}

    def seam_input(mesh):
        mesh.calc_loop_triangles()
        positions = [tuple(vertex.co) for vertex in mesh.vertices]
        faces = [(mesh.materials[mesh.polygons[triangle.polygon_index].material_index].name,
                  tuple(triangle.vertices)) for triangle in mesh.loop_triangles]
        weights = [{group_names[item.group]: item.weight for item in vertex.groups
                    if group_names[item.group] in deform_names and item.weight > 0.0}
                   for vertex in mesh.vertices]
        return positions, faces, weights

    view_input = seam_input(view_mesh)
    world_input = seam_input(seam_world_mesh)
    if options.reconcile_segmented_seams:
        from player_sleeve_seams import plan_segmented_sleeve_seam_splits
        from player_segmented_seams_blender import split_segmented_cloth_seams

        # Imported authoring vertices are centimetre-space. The planner's
        # physical tolerances are metres, so evaluate both meshes through the
        # same object transform; never enlarge a tolerance to mask unit errors.
        def metric_positions(mesh):
            return [tuple(player.matrix_world @ vertex.co) for vertex in mesh.vertices]

        world_metric, view_metric = metric_positions(seam_world_mesh), metric_positions(view_mesh)
        split_plan, split_stats = plan_segmented_sleeve_seam_splits(
            world_metric, world_input[1], view_metric, view_input[1])
        print('SEGMENTED_SEAM_PLAN ' + json.dumps(split_stats))
        if split_stats['coveredViewBoundaryEdges'] != split_stats['unmatchedViewBoundaryEdges']:
            (output / 'segmented-seam-plan-failure.json').write_text(json.dumps(dict(
                stats=split_stats, coordinateSpace='Blender world metres',
                worldPositions=world_metric, worldFaces=world_input[1],
                viewPositions=view_metric, viewFaces=view_input[1])), encoding='utf-8')
            raise RuntimeError('Some unmatched sleeve boundaries lack proven coarse cloth coverage')
        for plan in split_plan:
            for point in plan['points']:
                # Copy the exact canonical authoring vertex, avoiding an
                # inverse-transform round trip before subsequent exact matching.
                point['position'] = view_input[0][point['viewVertex']]
        segmented_seam_reconciliation = dict(plan=split_stats,
            applied=split_segmented_cloth_seams(seam_world_mesh, split_plan))
        world_input = seam_input(seam_world_mesh)
        _, remaining = plan_segmented_sleeve_seam_splits(
            metric_positions(seam_world_mesh), world_input[1], view_metric, view_input[1])
        if remaining['unmatchedViewBoundaryEdges'] != 0:
            raise RuntimeError('Segmented seam repair left unmatched sleeve boundaries')
    source_weight_sums = {
        role: {'minimum': min(sum(weights.values()) for weights in data[2] if weights),
               'maximum': max(sum(weights.values()) for weights in data[2] if weights)}
        for role, data in (('world', world_input), ('view', view_input))}
    print('SEAM_INPUT_WEIGHT_SUMS ' + json.dumps(source_weight_sums))
    transfers, seam_reconciliation = plan_sleeve_seam_weight_transfers(
        *world_input, *view_input)
    # Include removed influences in the report, not just target influences.
    maximum_weight_delta = max(
        (abs(target.get(name, 0.0) - world_input[2][index].get(name, 0.0))
         for index, target in transfers.items()
         for name in world_input[2][index].keys() | target.keys()), default=0.0)
    try:
        player.data = seam_world_mesh
        player.name = seam_world_name
        target_indices = sorted(transfers)
        for group in player.vertex_groups:
            if group.name in deform_names:
                group.remove(target_indices)
        for index, weights in transfers.items():
            for name, weight in weights.items():
                player.vertex_groups[name].add([index], weight, 'REPLACE')
        after_positions, after_faces, after_weights = seam_input(seam_world_mesh)
        if after_positions != world_input[0] or after_faces != world_input[1]:
            raise RuntimeError('Seam reconciliation changed world geometry/topology')
        if any(after_weights[index] != weights for index, weights in
               enumerate(world_input[2]) if index not in transfers):
            raise RuntimeError('Seam reconciliation changed a vertex outside its explicit plan')
        if any(after_weights[index] != weights for index, weights in transfers.items()):
            raise RuntimeError('Seam reconciliation did not apply its exact canonical weights')
        calibrated_world = output / 'world-seam-reconciled.runtime.glb'
        bpy.ops.export_scene.gltf(filepath=str(calibrated_world), export_format='GLB', use_selection=True,
                                  export_tangents=True, export_animations=True, export_animation_mode='NLA_TRACKS',
                                  export_frame_range=True, export_skins=True, export_morph=False,
                                  export_cameras=False, export_lights=False, export_extras=True)
    finally:
        player.data = view_mesh
        player.name = view_name
    seam_reconciliation['maximumWeightDelta'] = maximum_weight_delta
    seam_reconciliation['sourceAuthoringWeightSums'] = source_weight_sums
    seam_reconciliation['scope'] = 'Coincident world sleeve/connecting-cloth vertices at matched view sleeve boundary edges only'
    seam_reconciliation['canonicalWeights'] = 'Unchanged exported viewmodel sleeve endpoints'
paired_world_manifest = None
if options.body_remainder:
    manifest = json.loads((accepted_world.parent / 'asset.manifest.json').read_text(encoding='utf-8'))
    manifest['playerAssetRole'] = 'WorldBody'
    manifest['budgets']['maxPrimitives'] = 5
    manifest['budgets']['maxMaterials'] = 5
    # Blender emits a 4x4 texture-identity record per material. The added region
    # shares the Body atlas group at runtime; admit its fifth embedded identity
    # without allocating another production atlas layer.
    manifest['budgets']['maxTextureLayersPerKind'] = 5
    manifest['primitiveSemantics'].append(dict(material='BodyRemainderPrimaryVisible',
        firstPersonPrimary=True, shadow=True, reflection=True))
    paired_world_manifest = output / 'asset.manifest.json'
    paired_world_manifest.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
report = dict(schema=1, role='Viewmodel',
              sourceWorldSha256=sha(accepted_world),
              gauntletScale=gauntlet_scale,
              gauntletScaleAppliedAbout='AuthoredGripOrigin',
              gauntletSourceHandedness=options.gauntlet_source_hand or 'LegacyLeftClassification',
              gripRollCorrectionRadians=grip_rolls['Left'] if grip_rolls['Left'] == grip_rolls['Right'] else None,
              gripRollRadiansBySide=grip_rolls,
              pairedWorldRuntime=calibrated_world.name if calibrated_world else None,
              pairedWorldSha256=sha(calibrated_world) if calibrated_world else None,
              pairedWorldManifest=paired_world_manifest.name if paired_world_manifest else None,
              pairedWorldManifestSha256=sha(paired_world_manifest) if paired_world_manifest else None,
              sleeveWeightMode='ElbowCentredArmForeArm' if blend_elbows else
                  ('ArmForeArm' if stabilize_sleeves else 'OriginalArmForeArmHand'),
              sleeveHandWeightsMovedToForearm=weight_corrections,
              elbowWeightField=elbow_corrections,
              sleeveEnvelopeFit=sleeve_fit,
              sleeveClosure=sleeve_closure,
              sleeveSeamReconciliation=seam_reconciliation,
              **({'segmentedSeamReconciliation': segmented_seam_reconciliation}
                 if options.reconcile_segmented_seams else {}),
              **({'bodyPrimaryPartition': world['report']['bodyPrimaryPartition']}
                 if options.retain_upper_torso else {}),
              runtime=viewmodel_output.name, runtimeSha256=sha(viewmodel_output),
              primitiveSemantics=counts, processingVertices=len(player.data.vertices),
              ownership='Primary-only modelled geometry; world body owns secondary visibility',
              animationAuthority='Same authored rig, Idle/Walking NLA tracks and named grip bones as world reference',
              licence='Derivative of existing licensed Meshy player/gauntlet inputs; see ASSET_LICENSES.md',
              status='Offline candidate only; GPU ownership, grip/pitch/motion and owner acceptance not yet established')
(output / 'viewmodel-processing.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
