"""Check exact CPU viewmodel uploads for parry arm/sleeve and sword/sleeve crossings.

Run in Blender:
  blender --background --threads 1 --python-exit-code 1 \
    --python reports/check-viewmodel-parry-intersections.py -- \
    <gauntlet-source.glb> <bound-viewmodel.glb> <sword.glb> <capture.obj>

The script is read-only. It validates OBJ vertex/UV/index correspondence to the
bound GLB, recovers the named RightGrip pose from exact Hand skin samples, mounts
the sword by its authored Grip socket, then confirms BVH candidates with strict
segment/triangle crossings. Coplanar overlap is reported as unclassified.
"""
import contextlib
import io
import json
import runpy
import sys
from collections import Counter
from pathlib import Path

import numpy as np
from mathutils.bvhtree import BVHTree


source_path, bind_path, sword_path, obj_path = \
    sys.argv[sys.argv.index('--') + 1:]


def read_obj(path):
    vertices, uvs, groups = [], [], {}
    group = None
    model_to_world = None
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        if line.startswith('# model_to_world_row_major_3x4 '):
            model_to_world = np.array(
                list(map(float, line.split()[2:])), dtype=np.float64
            ).reshape(3, 4)
        elif line.startswith('v '):
            vertices.append(list(map(float, line.split()[1:4])))
        elif line.startswith('vt '):
            uvs.append(list(map(float, line.split()[1:3])))
        elif line.startswith('g '):
            group = line.split(maxsplit=1)[1]
            groups.setdefault(group, [])
        elif line.startswith('f '):
            if group is None:
                raise RuntimeError('OBJ face appeared before a material group')
            corners = []
            for token in line.split()[1:]:
                fields = token.split('/')
                if len(fields) < 2 or not fields[0] or not fields[1]:
                    raise RuntimeError('OBJ face lacks a vertex/UV index')
                vertex, uv = int(fields[0]) - 1, int(fields[1]) - 1
                if vertex != uv:
                    raise RuntimeError(
                        f'OBJ vertex/UV index mismatch: {vertex} != {uv}')
                corners.append(vertex)
            if len(corners) != 3:
                raise RuntimeError('Expected triangulated native OBJ faces')
            groups[group].append(tuple(corners))
    if model_to_world is None:
        raise RuntimeError('Native OBJ is missing its model-to-world header')
    return (np.asarray(vertices, dtype=np.float64),
            np.asarray(uvs, dtype=np.float64), groups, model_to_world)


def triangle_counter(faces):
    return Counter(tuple(sorted(face)) for face in faces)


def rigid_bone_transform(matrix):
    """Match runtime RigidPlayerBoneInModel's basis orthogonalisation."""
    result = np.eye(4, dtype=np.float64)
    x = matrix[:3, 0] / np.linalg.norm(matrix[:3, 0])
    raw_y = matrix[:3, 1]
    y = raw_y - x * np.dot(raw_y, x)
    y /= np.linalg.norm(y)
    z = np.cross(x, y)
    z /= np.linalg.norm(z)
    if np.dot(z, matrix[:3, 2]) < 0.0:
        y, z = -y, -z
    result[:3, :3] = np.column_stack((x, y, z))
    result[:3, 3] = matrix[:3, 3]
    return result


def segment_triangle(start, end, triangle):
    direction = end - start
    a, b, c = triangle
    edge1, edge2 = b - a, c - a
    h = np.cross(direction, edge2)
    determinant = edge1 @ h
    if abs(determinant) < 1.0e-12:
        return False
    inverse = 1.0 / determinant
    offset = start - a
    u = inverse * (offset @ h)
    if not 1.0e-7 < u < 1.0 - 1.0e-7:
        return False
    q = np.cross(offset, edge1)
    v = inverse * (direction @ q)
    distance = inverse * (edge2 @ q)
    return (v > 1.0e-7 and u + v < 1.0 - 1.0e-7 and
            1.0e-7 < distance < 1.0 - 1.0e-7)


def triangles_cross(first, second):
    return any(segment_triangle(first[i], first[(i + 1) % 3], second)
               or segment_triangle(second[i], second[(i + 1) % 3], first)
               for i in range(3))


def inspect_pair(label, vertices_a, triangles_a, vertices_b, triangles_b):
    result = dict(label=label, trianglesA=len(triangles_a),
                  trianglesB=len(triangles_b), broadphasePairs=0,
                  confirmedCrossings=0, examples=[])
    if not len(triangles_a) or not len(triangles_b):
        return result
    tree_a = BVHTree.FromPolygons(vertices_a.tolist(), triangles_a.tolist(),
                                  all_triangles=True, epsilon=0.0)
    tree_b = BVHTree.FromPolygons(vertices_b.tolist(), triangles_b.tolist(),
                                  all_triangles=True, epsilon=0.0)
    candidates = tree_a.overlap(tree_b)
    result['broadphasePairs'] = len(candidates)
    crossing_count = 0
    for index_a, index_b in candidates:
        tri_a = vertices_a[triangles_a[index_a]]
        tri_b = vertices_b[triangles_b[index_b]]
        if not triangles_cross(tri_a, tri_b):
            continue
        crossing_count += 1
        if len(result['examples']) < 8:
            result['examples'].append([int(index_a), int(index_b)])
    result['confirmedCrossings'] = crossing_count
    return result


def load_sword_in_grip_space(glb):
    grip_nodes = [i for i, node in enumerate(glb.doc['nodes'])
                  if node.get('name') == 'Grip']
    if len(grip_nodes) != 1:
        raise RuntimeError(f'Expected one named sword Grip node; found {len(grip_nodes)}')
    grip = glb.global_node(grip_nodes[0])
    grip_inverse = np.linalg.inv(grip)
    positions, triangles = [], []
    for node_index, node in enumerate(glb.doc['nodes']):
        if 'mesh' not in node:
            continue
        node_matrix = grip_inverse @ glb.global_node(node_index)
        mesh = glb.doc['meshes'][node['mesh']]
        for primitive in mesh['primitives']:
            local_positions = glb.acc(primitive['attributes']['POSITION'])
            transformed = (node_matrix @ np.column_stack(
                (local_positions, np.ones(len(local_positions)))).T).T[:, :3]
            offset = len(positions)
            positions.extend(transformed)
            if 'indices' in primitive:
                indices = glb.acc(primitive['indices']).reshape(-1)
            else:
                indices = np.arange(len(local_positions))
            if len(indices) % 3:
                raise RuntimeError('Sword primitive index count is not triangular')
            triangles.extend((indices.reshape(-1, 3) + offset).tolist())
    if not positions or not triangles:
        raise RuntimeError('Sword GLB contains no triangle mesh')
    return np.asarray(positions), np.asarray(triangles, dtype=np.int64), grip


# Reuse the existing source-to-native affine trace and its strict fit checks.
trace_args = sys.argv
trace_output = io.StringIO()
try:
    sys.argv = ['trace-gauntlet-source-mount.py', '--',
                source_path, bind_path, obj_path]
    with contextlib.redirect_stdout(trace_output):
        trace = runpy.run_path(
            str(Path(__file__).with_name('trace-gauntlet-source-mount.py')),
            run_name='__main__')
finally:
    sys.argv = trace_args

obj_vertices, obj_uvs, obj_groups, model_to_world = read_obj(obj_path)
bind = trace['bind']
if len(obj_vertices) != len(trace['bind_positions']):
    raise RuntimeError('Native OBJ vertex count does not match bound GLB primitives')
if len(obj_uvs) != len(trace['bind_uvs']):
    raise RuntimeError('Native OBJ UV count does not match bound GLB primitives')
uv_error = float(np.max(np.abs(obj_uvs - trace['bind_uvs'])))
if uv_error > 1.0e-6:
    raise RuntimeError(f'Native OBJ UV ordering does not match bound GLB: {uv_error}')

vertex_offset = 0
expected_faces = {}
for primitive in bind.doc['meshes'][0]['primitives']:
    material_name = bind.doc['materials'][primitive['material']]['name']
    primitive_positions = bind.acc(primitive['attributes']['POSITION'])
    count = len(primitive_positions)
    if 'indices' in primitive:
        indices = bind.acc(primitive['indices']).reshape(-1)
    else:
        indices = np.arange(count)
    if len(indices) % 3:
        raise RuntimeError(f'{material_name} index count is not triangular')
    indices = indices.reshape(-1, 3) + vertex_offset
    expected_faces.setdefault(material_name, []).extend(map(tuple, indices.tolist()))
    vertex_offset += count
if vertex_offset != len(obj_vertices):
    raise RuntimeError('Bound GLB primitive vertex ranges do not cover the native OBJ')
for material_name in ('ViewmodelSleeves', 'ViewmodelGauntlets'):
    if material_name not in obj_groups or material_name not in expected_faces:
        raise RuntimeError(f'Missing expected material group {material_name}')
    if triangle_counter(obj_groups[material_name]) != triangle_counter(
            expected_faces[material_name]):
        raise RuntimeError(f'OBJ triangle membership differs from bound GLB {material_name}')

# Confirm the fitted RightHand affine predicts all rigid RightHand vertices,
# then apply it to the bound named RightGrip transform.
bone_ids = trace['bone_ids']
right_hand_id = bone_ids['RightHand']
right_mask = trace['is_gauntlet'] & np.any(
    (trace['joints'] == right_hand_id) & (trace['weights'] > .99999), axis=1)
right_hand_pose, right_fit = trace['fit'](
    trace['bind_positions'][right_mask], trace['posed'][right_mask],
    'RightHand capture fit')
node_names = {node.get('name'): i for i, node in enumerate(bind.doc['nodes'])}
if 'RightGrip' not in node_names:
    raise RuntimeError('Bound GLB has no named RightGrip node')
right_grip_fit = right_hand_pose @ bind.global_node(node_names['RightGrip'])
right_grip_model = rigid_bone_transform(right_grip_fit)

# Recover joint-chain labels from actual skin weights, not screen-side guesses.
joint_node_names = [bind.doc['nodes'][i].get('name', '')
                    for i in bind.doc['skins'][0]['joints']]
weights = trace['weights']
joint_names = np.asarray(joint_node_names, dtype=object)
left_ids = np.flatnonzero(np.char.startswith(joint_names.astype(str), 'Left'))
right_ids = np.flatnonzero(np.char.startswith(joint_names.astype(str), 'Right'))
left_score = np.zeros(len(weights), dtype=np.float64)
right_score = np.zeros(len(weights), dtype=np.float64)
for slot in range(trace['joints'].shape[1]):
    left_score += weights[:, slot] * np.isin(trace['joints'][:, slot], left_ids)
    right_score += weights[:, slot] * np.isin(trace['joints'][:, slot], right_ids)
side = np.where(left_score > right_score, 1,
                np.where(right_score > left_score, -1, 0))
dominant_slot = np.argmax(weights, axis=1)
dominant_joint = trace['joints'][np.arange(len(weights)), dominant_slot]
dominant_name = joint_names[dominant_joint]


def selected_faces(group_name, predicate):
    selected = []
    excluded = 0
    for face in obj_groups[group_name]:
        if all(predicate(np.asarray(face, dtype=np.int64))):
            selected.append(face)
        else:
            excluded += 1
    return np.asarray(selected, dtype=np.int64).reshape(-1, 3), excluded


left_sleeve, left_ambiguous = selected_faces(
    'ViewmodelSleeves', lambda ids: side[ids] == 1)
right_forearm, forearm_ambiguous = selected_faces(
    'ViewmodelSleeves', lambda ids: (side[ids] == -1) &
    np.isin(dominant_name[ids], ('RightArm', 'RightForeArm')))
right_hand, hand_ambiguous = selected_faces(
    'ViewmodelGauntlets', lambda ids: (side[ids] == -1) &
    np.isin(dominant_name[ids], ('RightHand', 'RightGrip')))
if min(len(left_sleeve), len(right_forearm), len(right_hand)) == 0:
    raise RuntimeError('Skin-weight material partition failed to classify requested regions')

# Place the sword so its asset-owned Grip coincides with the fitted, named
# RightGrip in viewmodel model space. Runtime uses this same +Y 135 mm socket.
sys.argv = ['load-sword']
sword_glb = trace['Glb'](sword_path)
sword_vertices, sword_triangles, sword_grip = load_sword_in_grip_space(sword_glb)
runtime_grip = np.eye(4)
runtime_grip[1, 3] = .135
socket_error = float(np.max(np.abs(sword_grip - runtime_grip)))
if socket_error > 2.0e-4:
    raise RuntimeError(
        f'Sword asset Grip differs from runtime 135 mm socket: {socket_error}')
sword_in_model = (right_grip_model @ np.column_stack(
    (sword_vertices, np.ones(len(sword_vertices)))).T).T[:, :3]

left_points = obj_vertices
right_arm_triangles = np.concatenate((right_forearm, right_hand), axis=0)
right_arm_vertices = obj_vertices
results = [
    inspect_pair('right_sleeve_vs_left_sleeve', right_arm_vertices,
                 right_forearm, left_points, left_sleeve),
    inspect_pair('right_hand_vs_left_sleeve', right_arm_vertices,
                 right_hand, left_points, left_sleeve),
    inspect_pair('right_arm_union_vs_left_sleeve', right_arm_vertices,
                 right_arm_triangles, left_points, left_sleeve),
    inspect_pair('sword_vs_left_sleeve', sword_in_model,
                 sword_triangles, left_points, left_sleeve),
]

print('VIEWMODEL_PARRY_INTERSECTION_REPORT ' + json.dumps(dict(
    capture=str(obj_path), bindViewmodel=str(bind_path), sword=str(sword_path),
    mapping=dict(vertices=len(obj_vertices), uvs=len(obj_uvs),
                 maximumUvError=uv_error,
                 triangleMembership='exact per material',
                 leftSleeveTriangles=len(left_sleeve),
                 rightForearmTriangles=len(right_forearm),
                 rightHandTriangles=len(right_hand),
                 excludedSleeveFaces=dict(left=left_ambiguous,
                                          rightForearm=forearm_ambiguous),
                 excludedGauntletFaces=hand_ambiguous,
                 rightHandFit=right_fit,
                 rightGripScaleRemovedForRigidMount=right_grip_fit[:3, :3].tolist(),
                 rightGripModelMatrix=right_grip_model.tolist(),
                 swordGripRuntimeSocketMaximumError=socket_error),
    intersections=results), sort_keys=True))
