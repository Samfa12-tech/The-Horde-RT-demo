"""Retained read-only comparison used for the 2026-10-02 fitted-cuff check.

Expanded triangles are canonicalized by cyclic rotation, preserving winding,
so semantic comparisons do not depend on raw vertex/index export order.
"""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct


CONTROL_VIEW = Path(r"C:\Dev\tmp\horde-right-cuff-rt-20261002-a\assets\models\player\viewmodel\runtime\gothic-traveller-viewmodel.runtime.glb")
CANDIDATE_VIEW = Path(r"C:\Dev\tmp\horde-right-cuff-fit-20261002-guarded\gothic-traveller-viewmodel.runtime.glb")
CONTROL_WORLD = Path(r"C:\Dev\tmp\horde-right-cuff-rt-20261002-a\assets\models\player\runtime\gothic-traveller-lod0.runtime.glb")
CANDIDATE_WORLD = Path(r"C:\Dev\tmp\horde-right-cuff-fit-20261002-guarded\world-seam-reconciled.runtime.glb")


def load(path):
    raw = path.read_bytes()
    magic, version, length = struct.unpack_from('<III', raw)
    if magic != 0x46546c67 or version != 2 or length != len(raw):
        raise ValueError(f'Invalid GLB container: {path}')
    off, doc, binary = 12, None, None
    while off < length:
        size, kind = struct.unpack_from('<II', raw, off)
        chunk = raw[off + 8:off + 8 + size]
        if kind == 0x4e4f534a:
            doc = json.loads(chunk)
        elif kind == 0x004e4942:
            binary = chunk
        off += size + 8

    def accessor(index):
        a = doc['accessors'][index]
        view = doc['bufferViews'][a['bufferView']]
        count = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        fmt = '<' + {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}[a['componentType']] * count
        stride = view.get('byteStride', struct.calcsize(fmt))
        start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
        values = [struct.unpack_from(fmt, binary, start + i * stride) for i in range(a['count'])]
        if a.get('normalized') and a['componentType'] == 5121:
            values = [tuple(x / 255 for x in row) for row in values]
        elif a.get('normalized') and a['componentType'] == 5123:
            values = [tuple(x / 65535 for x in row) for row in values]
        return values

    joint_names = {
        i: doc['nodes'][node].get('name', f'joint{i}')
        for i, node in enumerate(doc['skins'][0]['joints'])
    }
    surfaces = {}
    rows_by_material = {}
    uv_triangles = {}
    for primitive in doc['meshes'][0]['primitives']:
        name = doc['materials'][primitive['material']]['name']
        attrs = primitive['attributes']
        values = {
            key: accessor(attrs[key])
            for key in ('POSITION', 'NORMAL', 'TANGENT', 'TEXCOORD_0', 'JOINTS_0', 'WEIGHTS_0')
        }
        indices = [row[0] for row in accessor(primitive['indices'])]
        rows = []
        for i in range(len(values['POSITION'])):
            weights = tuple(sorted(
                (joint_names[j], weight)
                for j, weight in zip(values['JOINTS_0'][i], values['WEIGHTS_0'][i])
                if weight > 0
            ))
            rows.append((
                values['POSITION'][i], values['NORMAL'][i], values['TANGENT'][i],
                values['TEXCOORD_0'][i], weights,
            ))

        def canonical_triangle(corners):
            return min(corners[i:] + corners[:i] for i in range(3))

        triangles = []
        uv_tris = []
        for start in range(0, len(indices), 3):
            tri_indices = indices[start:start + 3]
            triangles.append(canonical_triangle(tuple(rows[i] for i in tri_indices)))
            uv_corners = tuple(values['TEXCOORD_0'][i] for i in tri_indices)
            uv_tris.append(canonical_triangle(uv_corners))
        surfaces[name] = Counter(triangles)
        rows_by_material[name] = rows
        uv_triangles[name] = Counter(uv_tris)
    return {
        'sha256': hashlib.sha256(raw).hexdigest(),
        'surfaces': surfaces,
        'rows': rows_by_material,
        'uv_triangles': uv_triangles,
    }


def compare_pair(label, control_path, candidate_path):
    control, candidate = load(control_path), load(candidate_path)
    expected = {
        'view': ('6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b',
                 'eaa0db3abb8ff2dae616247054c190031a08c542dd95396cc6dc1e18a7462488'),
        'world': ('f2c3f62b2696c4630309b1d0b0ecb366151054fb48f7c0c6bcc6956980ff81eb',
                  '46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f'),
    }
    if (control['sha256'], candidate['sha256']) != expected[label]:
        raise ValueError('Retained control/candidate identity lost; do not compare new production to itself')
    ca, cb = control['surfaces'], candidate['surfaces']
    if set(ca) != set(cb):
        raise ValueError('Semantic primitive roster changed')
    result = {
        'controlSha256': control['sha256'],
        'candidateSha256': candidate['sha256'],
        'materials': {},
    }
    for material in sorted(set(ca) & set(cb)):
        entry = {
            'controlTriangles': sum(ca[material].values()),
            'candidateTriangles': sum(cb[material].values()),
            'expandedPositionNormalTangentUVWindingAndWeightsExact': ca[material] == cb[material],
        }
        if 'Gauntlet' in material:
            old_rows = control['rows'][material]
            new_rows = candidate['rows'][material]
            left = lambda row: any(name == 'LeftHand' for name, weight in row[4])
            entry['leftHandRows'] = sum(left(row) for row in old_rows)
            entry['leftHandRowsExact'] = (
                Counter(row for row in old_rows if left(row))
                == Counter(row for row in new_rows if left(row))
            )
            entry['uvVertexMultisetExact'] = (
                Counter(row[3] for row in old_rows) == Counter(row[3] for row in new_rows)
            )
            entry['uvTriangleTopologyAndWindingExact'] = (
                control['uv_triangles'][material] == candidate['uv_triangles'][material]
            )
            right_hand_only = lambda row: (
                ('RightHand', 1.0) in row[4]
                and all(name != 'RightForeArm' for name, weight in row[4])
            )
            old_distal = Counter(
                (row[0], row[3], row[4]) for row in old_rows if right_hand_only(row)
            )
            new_distal = Counter(
                (row[0], row[3], row[4]) for row in new_rows if right_hand_only(row)
            )
            entry['controlRightHandOnlyRows'] = sum(old_distal.values())
            entry['candidateRightHandOnlyRows'] = sum(new_distal.values())
            entry['exactProtectedDistalPositionUVWeightRows'] = sum((old_distal & new_distal).values())
            entry['candidateHandOnlyRowsNotInControlExactSet'] = sum((new_distal - old_distal).values())
            entry['candidateRightForeArmInfluencedRows'] = sum(
                1 for row in new_rows if any(name == 'RightForeArm' for name, weight in row[4])
            )
        result['materials'][material] = entry
    return label, result


if __name__ == '__main__':
    results = dict((label, result) for label, result in (
        compare_pair('view', CONTROL_VIEW, CANDIDATE_VIEW),
        compare_pair('world', CONTROL_WORLD, CANDIDATE_WORLD),
    ))
    print(json.dumps(results, indent=2))
