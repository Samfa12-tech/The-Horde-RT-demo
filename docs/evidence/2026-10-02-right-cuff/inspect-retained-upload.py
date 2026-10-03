"""One-shot CPU inspection of the admitted asset and retained native upload.

Not a production tool; fits rigid bone maps from unchanged GLB/OBJ vertex order.
No assets or runtime poses are modified.
"""
import json
from pathlib import Path
import struct
import sys
import numpy as np

root = Path(sys.argv[sys.argv.index('--') + 1])
payload = (root / 'assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb').read_bytes()
length = struct.unpack_from('<I', payload, 12)[0]
doc = json.loads(payload[20:20 + length])
binary = payload[28 + length:]

def accessor(index):
    a = doc['accessors'][index]
    v = doc['bufferViews'][a['bufferView']]
    width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
    dtype = {5121: '<u1', 5123: '<u2', 5125: '<u4', 5126: '<f4'}[a['componentType']]
    size = np.dtype(dtype).itemsize
    return np.ndarray((a['count'], width), dtype=dtype, buffer=binary,
                      offset=v.get('byteOffset', 0) + a.get('byteOffset', 0),
                      strides=(v.get('byteStride', size * width), size)).copy()

parts = doc['meshes'][0]['primitives']
bind = np.concatenate([accessor(p['attributes']['POSITION']) for p in parts])
joints = np.concatenate([accessor(p['attributes']['JOINTS_0']) for p in parts])
weights = np.concatenate([accessor(p['attributes']['WEIGHTS_0']) for p in parts])
skin = doc['skins'][0]
names = [doc['nodes'][i]['name'] for i in skin['joints']]
inverse = accessor(skin['inverseBindMatrices'])
report = {'scope': 'Exact retained CPU upload, not owner frame, GPU readback or visual acceptance', 'poses': {}}
default_doc, default_binary = doc, binary
source_path = root / 'assets/models/player/source/meshy-2026-08-30-viewmodel-gauntlet/right-gauntlet-5k-stripped.glb'
source_payload = source_path.read_bytes()
source_length = struct.unpack_from('<I', source_payload, 12)[0]
doc = json.loads(source_payload[20:20 + source_length])
binary = source_payload[28 + source_length:]
source_primitive = doc['meshes'][0]['primitives'][0]
source_pos = accessor(source_primitive['attributes']['POSITION'])
source_uv = accessor(source_primitive['attributes']['TEXCOORD_0'])
source_extras = next(n.get('extras') for n in doc['nodes'] if 'mesh' in n)
doc, binary = default_doc, default_binary
target_uv = accessor(parts[1]['attributes']['TEXCOORD_0'])
source_by_uv = {tuple(np.round(uv, 6)): pos for uv, pos in zip(source_uv, source_pos)}
right_glove = np.any((joints == names.index('RightHand')) & (weights > 0.99999), axis=1)
offset = len(accessor(parts[0]['attributes']['POSITION']))
uv_pairs = [(source_by_uv.get(tuple(np.round(uv, 6))), bind[i + offset]) for i,uv in enumerate(target_uv) if right_glove[i+offset]]
uv_pairs = [(s,t) for s,t in uv_pairs if s is not None]
source_matches = np.array([s for s,t in uv_pairs])
target_matches = np.array([t for s,t in uv_pairs])
source_map = np.linalg.lstsq(np.c_[source_matches, np.ones(len(source_matches))], target_matches, rcond=None)[0]
source_residuals = np.linalg.norm(np.c_[source_matches, np.ones(len(source_matches))] @ source_map - target_matches, axis=1)
report['sourceCuffFit'] = {'uvPairs': len(uv_pairs), 'maxResidualMetres': float(source_residuals.max()),
                          'sourceExtras': source_extras, 'sourceToBindRowVectorMap': source_map.tolist(),
                          'rawSourcePositionBounds': [source_matches.min(axis=0).tolist(), source_matches.max(axis=0).tolist()]}
for pose in ('00-opening', '09-mirror'):
    path = root / f'reports/final-integration-20261002/run-20261002-183151/captures/windows/{pose}.obj'
    if not path.exists():
        continue
    posed = np.array([list(map(float, line.split()[1:])) for line in path.read_text().splitlines() if line.startswith('v ')])
    assert posed.shape == bind.shape
    row = {}
    for side in ('Left', 'Right'):
        matrices = {}
        errors = {}
        for suffix in ('ForeArm', 'Hand'):
            name = side + suffix
            joint = names.index(name)
            mask = np.any((joints == joint) & (weights > 0.99999), axis=1)
            x = np.c_[bind[mask], np.ones(mask.sum())]
            matrix = np.linalg.lstsq(x, posed[mask], rcond=None)[0]
            matrices[suffix] = matrix
            errors[suffix] = float(np.linalg.norm(x @ matrix - posed[mask], axis=1).max())
        # A bone's inverse bind translation locates its wrist in mesh space.
        wrist = np.linalg.inv(inverse[names.index(side + 'Hand')].reshape((4, 4), order='F'))[:3, 3]
        elbow = np.linalg.inv(inverse[names.index(side + 'ForeArm')].reshape((4, 4), order='F'))[:3, 3]
        axis = (elbow - wrist) / np.linalg.norm(elbow - wrist)
        source_cuff_axis = np.array(source_extras['hordeCuffAxis']) @ source_map[:3]
        source_cuff_axis /= np.linalg.norm(source_cuff_axis)
        source_wrist = (wrist - source_map[3]) @ np.linalg.inv(source_map[:3])
        source_cuff = source_matches[source_matches[:,0] > 0.65]
        source_cuff_centre = (source_cuff.min(axis=0) + source_cuff.max(axis=0)) / 2
        cuff_centre = np.r_[source_cuff_centre, 1] @ source_map
        cuff_offset = cuff_centre - wrist
        cuff_axial = float(cuff_offset @ axis)
        cuff_radial = cuff_offset - cuff_axial * axis
        hand_origin = np.r_[wrist, 1] @ matrices['Hand']
        forearm_origin = np.r_[wrist, 1] @ matrices['ForeArm']
        hand_axis = axis @ matrices['Hand'][:3]
        forearm_axis = axis @ matrices['ForeArm'][:3]
        angle = float(np.degrees(np.arccos(np.clip(hand_axis @ forearm_axis / np.linalg.norm(hand_axis) / np.linalg.norm(forearm_axis), -1, 1))))
        rotations = {}
        scales = {}
        for k, m in matrices.items():
            u,s,vt = np.linalg.svd(m[:3])
            rotations[k] = u @ vt
            scales[k] = s.tolist()
        relative_angle = float(np.degrees(np.arccos(np.clip((np.trace(rotations['Hand'] @ rotations['ForeArm'].T) - 1) / 2, -1, 1))))
        cloth_count = len(accessor(parts[0]['attributes']['POSITION']))
        cloth = np.arange(cloth_count)
        own = np.any((joints[:cloth_count] == names.index(side + 'ForeArm')) & (weights[:cloth_count] > 0.99), axis=1)
        offsets = bind[:cloth_count] - wrist
        projection = offsets @ axis
        distances = np.linalg.norm(offsets, axis=1)
        glove = np.arange(cloth_count, len(bind))
        glove = glove[np.any((joints[glove] == names.index(side + 'Hand')) & (weights[glove] > 0.99999), axis=1)]
        glove_offsets = bind[glove] - wrist
        glove_projections = glove_offsets @ axis
        glove_radials = np.linalg.norm(glove_offsets - glove_projections[:, None] * axis, axis=1)
        cuff_glove = glove[glove_projections > 0.03]
        cuff_cloth = cloth[own & (projection < 0.09)]
        separation = np.linalg.norm(posed[cuff_glove, None] - posed[cuff_cloth][None], axis=2).min(axis=0)
        # One anatomical contrast, not a weight/roll search. Only proximal
        # gauntlet cuff participates; all distal hand/finger geometry stays Hand.
        t = np.clip(glove_projections / 0.04, 0, 1)
        forearm_weight = t * t * (3 - 2 * t)
        altered = posed.copy()
        glove_forearm = np.c_[bind[glove], np.ones(len(glove))] @ matrices['ForeArm']
        altered[glove] = posed[glove] * (1 - forearm_weight[:, None]) + glove_forearm * forearm_weight[:, None]
        # The sleeve's terminal portion alone, not the whole 90-mm cloth band.
        terminal = cloth[own & (projection < 0.04) & (distances < 0.06)]
        before = np.linalg.norm(posed[glove, None] - posed[terminal][None], axis=2).min(axis=0)
        after = np.linalg.norm(altered[glove, None] - posed[terminal][None], axis=2).min(axis=0)
        in_bind = np.linalg.norm(bind[glove, None] - bind[terminal][None], axis=2).min(axis=0)
        nearest = cloth[own][np.argsort(distances[own])[:12]]
        row[side] = {'fitMaxResidualMetres': errors, 'wristBindMeshUnits': wrist.tolist(), 'elbowBindMeshUnits': elbow.tolist(),
                     'wristPositionDisagreementMetres': float(np.linalg.norm(hand_origin - forearm_origin)),
                     'forearmAxisHandVsForeArmDegrees': angle,
                     'authoredRightCuffToBindForearmDegrees': float(np.degrees(np.arccos(np.clip(axis @ source_cuff_axis, -1, 1)))) if side == 'Right' else None,
                     'rightSourceCuffAttachment': {'wristSource':source_wrist.tolist(),
                                                  'sourceEndCentre': source_cuff_centre.tolist(),
                                                  'bindEndCentre':cuff_centre.tolist(),
                                                  'axialFromWristMetres':cuff_axial,
                                                  'radialFromForearmAxisMetres':float(np.linalg.norm(cuff_radial))} if side == 'Right' else None,
                     'fullHandVsForeArmRotationDegrees': relative_angle,
                     'boneMapSingularValues': scales,
                     'gauntletProjectionRangeMetres': [float(glove_projections.min()), float(glove_projections.max())],
                     'gauntletCuffRadialRangeMetres': [float(glove_radials[glove_projections > 0.03].min()), float(glove_radials[glove_projections > 0.03].max())],
                     'sleeveCuffNearestGloveDistanceMetres': [float(separation.min()), float(np.median(separation)), float(separation.max())],
                     'contrast': {'gauntletVerticesChanged': int((forearm_weight > 0).sum()),
                                  'distalHandVerticesUnchanged': int((glove_projections <= 0).sum()),
                                  'terminalSleeveVertices': len(terminal),
                                  'terminalNearestGloveBeforeMetres': [float(before.min()), float(np.median(before)), float(before.max())],
                                  'terminalNearestGloveAfterMetres': [float(after.min()), float(np.median(after)), float(after.max())],
                                  'terminalNearestGloveBindMetres': [float(in_bind.min()), float(np.median(in_bind)), float(in_bind.max())],
                                  'maximumGauntletDisplacementMetres': float(np.linalg.norm(altered[glove] - posed[glove], axis=1).max())},
                     'nearestSleeveVertices':
                     [{'index': int(i), 'distanceMeshUnits': float(distances[i]), 'projectionMeshUnits': float(projection[i]),
                       'weights': {names[int(j)]: float(w) for j,w in zip(joints[i], weights[i]) if w > 0}}
                      for i in nearest], 'boneMapsRowVectors': {k: v.tolist() for k,v in matrices.items()}}
    report['poses'][pose] = row
print(json.dumps(report, indent=2))
