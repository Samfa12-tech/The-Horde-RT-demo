#!/usr/bin/env python3
"""Read-only E01 GLB inspection. Write JSON only to an explicit new report path.

This checks the observed narrow native importer contract, not runtime performance.
Geometry metrics describe accessor space; Blender/world-pose QA is separate.
"""
import argparse, hashlib, io, json, pathlib, struct
import numpy as np
from PIL import Image

DTYPES = {5120: '<i1', 5121: '<u1', 5122: '<i2', 5123: '<u2', 5125: '<u4', 5126: '<f4'}
COUNTS = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def load_glb(path):
    data = pathlib.Path(path).read_bytes()
    if len(data) < 20 or data[:4] != b'glTF':
        raise ValueError('Expected real GLB bytes, not an LFS pointer or preview URL')
    magic, version, length = struct.unpack_from('<III', data)
    if version != 2 or length != len(data):
        raise ValueError('Invalid GLB version or declared length')
    cursor, root, binary = 12, None, None
    while cursor < len(data):
        size, kind = struct.unpack_from('<II', data, cursor)
        cursor += 8
        if cursor + size > len(data):
            raise ValueError('Truncated GLB chunk')
        chunk = data[cursor:cursor+size]
        if kind == 0x4e4f534a:
            root = json.loads(chunk)
        elif kind == 0x004e4942:
            binary = chunk
        cursor += size
    if root is None or binary is None:
        raise ValueError('Expected JSON and BIN chunks')
    return data, root, binary


def accessor(root, binary, index):
    a = root['accessors'][index]
    if 'sparse' in a:
        raise ValueError('Sparse accessor unsupported by this inspector/native reader')
    view = root['bufferViews'][a['bufferView']]
    if view.get('buffer', 0) != 0:
        raise ValueError('External buffer unsupported')
    dtype, count = np.dtype(DTYPES[a['componentType']]), COUNTS[a['type']]
    offset = view.get('byteOffset', 0) + a.get('byteOffset', 0)
    stride = view.get('byteStride', dtype.itemsize * count)
    end = offset + max(0, a['count']-1) * stride + dtype.itemsize * count
    if end > len(binary) or end > view.get('byteOffset', 0) + view['byteLength']:
        raise ValueError(f'Accessor {index} outside bufferView')
    return np.ndarray((a['count'], count), dtype=dtype, buffer=binary,
                      offset=offset, strides=(stride, dtype.itemsize)).copy()


def inspect(path):
    data, root, binary = load_glb(path)
    fails, notes = [], []
    def gate(ok, message):
        if not ok:
            fails.append(message)
    meshes, skins, nodes = [root.get(key, []) for key in ('meshes', 'skins', 'nodes')]
    gate(len(meshes) == 1, 'Requires exactly one mesh')
    gate(len(skins) == 1, 'Requires exactly one skin')
    gate(len(root.get('materials', [])) == 2, 'E05 host trial requires body and shell material records')
    gate(not root.get('extensionsRequired'), 'Required glTF extensions are not admitted')
    gate(all('matrix' not in n for n in nodes), 'Node matrix unsupported: export TRS')
    gate(not any('sparse' in a for a in root.get('accessors', [])), 'Sparse accessors unsupported')
    gate(not any('uri' in b for b in root.get('buffers', [])), 'External buffers not admitted')
    joint_count = len(skins[0].get('joints', [])) if skins else 0
    gate(0 < joint_count <= 256, 'Skin must contain 1–256 joints addressable by uint8')
    parents = {}
    for ni, node in enumerate(nodes):
        for child in node.get('children', []):
            gate(isinstance(child, int) and 0 <= child < len(nodes), f'node{ni}: invalid child')
            gate(child not in parents, f'node{child}: multiple parents')
            parents[child] = ni
        for field, count in [('translation', 3), ('rotation', 4), ('scale', 3)]:
            if field in node:
                value = np.asarray(node[field])
                gate(value.shape == (count,) and bool(np.isfinite(value).all()), f'node{ni}: invalid {field}')
    for ni in range(len(nodes)):
        seen, current = set(), ni
        while current in parents:
            if current in seen:
                gate(False, f'node{ni}: cyclic hierarchy')
                break
            seen.add(current)
            current = parents[current]
    primitives, total_vertices, total_triangles = [], 0, 0
    layouts = {'POSITION': (5126, 'VEC3'), 'NORMAL': (5126, 'VEC3'),
               'TEXCOORD_0': (5126, 'VEC2'), 'TANGENT': (5126, 'VEC4'),
               'JOINTS_0': (5121, 'VEC4'), 'WEIGHTS_0': (5126, 'VEC4')}
    for mi, mesh in enumerate(meshes):
        for pi, primitive in enumerate(mesh.get('primitives', [])):
            tag = f'mesh{mi}/primitive{pi}'
            attrs = primitive.get('attributes', {})
            gate(primitive.get('mode', 4) == 4, f'{tag}: requires triangle mode')
            gate(not primitive.get('targets'), f'{tag}: morph targets unsupported')
            gate(not primitive.get('extensions'), f'{tag}: primitive extensions unsupported')
            gate('JOINTS_1' not in attrs and 'WEIGHTS_1' not in attrs, f'{tag}: extra skin influences unsupported')
            record = {'mesh': mi, 'primitive': pi, 'attributes': {}}
            arrays = {}
            for semantic, expected in layouts.items():
                gate(semantic in attrs, f'{tag}: missing {semantic}')
                if semantic not in attrs:
                    continue
                a = root['accessors'][attrs[semantic]]
                record['attributes'][semantic] = {k: a.get(k) for k in ('componentType', 'type', 'count', 'normalized')}
                gate((a['componentType'], a['type']) == expected, f'{tag}: {semantic} requires {expected}')
                gate(not a.get('normalized', False), f'{tag}: unexpected normalized accessor {semantic}')
                arrays[semantic] = accessor(root, binary, attrs[semantic])
                gate(bool(np.isfinite(arrays[semantic]).all()), f'{tag}: nonfinite {semantic}')
            pos = arrays.get('POSITION')
            if pos is None:
                continue
            record['vertices'] = len(pos)
            total_vertices += len(pos)
            record['bounds_accessor_space'] = {'min': pos.min(axis=0).tolist(), 'max': pos.max(axis=0).tolist()}
            for semantic, arr in arrays.items():
                gate(len(arr) == len(pos), f'{tag}: {semantic} count mismatch')
            gate('indices' in primitive, f'{tag}: missing indices')
            if 'indices' in primitive:
                ia = root['accessors'][primitive['indices']]
                inds = accessor(root, binary, primitive['indices']).reshape(-1)
                gate(ia['componentType'] == 5123 and ia['type'] == 'SCALAR', f'{tag}: indices must be uint16 scalar')
                gate(len(inds) % 3 == 0, f'{tag}: incomplete triangle')
                gate(not len(inds) or int(inds.max()) < len(pos), f'{tag}: index outside positions')
                record['index_component_type'] = ia['componentType']
                record['triangles'] = len(inds)//3
                total_triangles += len(inds)//3
                if len(inds) % 3 == 0 and len(inds) and inds.max() < len(pos):
                    triangles = inds.reshape(-1, 3)
                    points = pos[triangles]
                    double_area = np.linalg.norm(np.cross(points[:,1]-points[:,0], points[:,2]-points[:,0]), axis=1)
                    record['degenerate_triangles_area_lt_1e-12'] = int((double_area < 1e-12).sum())
                    gate(not bool((double_area < 1e-12).any()), f'{tag}: degenerate triangles')
                    # Exact-position weld is for reporting only, never an asset edit.
                    welded, mapping = np.unique(pos, axis=0, return_inverse=True)
                    wi = mapping[triangles]
                    edges = np.sort(np.concatenate([wi[:,[0,1]], wi[:,[1,2]], wi[:,[2,0]]]), axis=1)
                    _, counts = np.unique(edges, axis=0, return_counts=True)
                    record['exact_weld_vertices'] = len(welded)
                    record['exact_weld_boundary_edges'] = int((counts == 1).sum())
                    record['exact_weld_nonmanifold_edges'] = int((counts > 2).sum())
                    parent = np.arange(len(welded))
                    def find(v):
                        while parent[v] != v:
                            parent[v] = parent[parent[v]]
                            v = parent[v]
                        return v
                    for a, b in edges:
                        ra, rb = find(a), find(b)
                        if ra != rb: parent[rb] = ra
                    roots = np.array([find(v) for v in range(len(welded))])
                    _, component_counts = np.unique(roots, return_counts=True)
                    record['exact_weld_components'] = len(component_counts)
                    record['exact_weld_component_vertex_sizes'] = sorted(component_counts.tolist(), reverse=True)
            if 'WEIGHTS_0' in arrays:
                w = arrays['WEIGHTS_0']
                sums = w.sum(axis=1)
                record['weights'] = {'min': float(w.min()), 'max_sum_error': float(np.abs(sums-1).max()),
                    'zero_weight_vertices': int((sums <= 1e-8).sum()), 'positive_influences_max': int((w > 0).sum(axis=1).max())}
                gate(w.shape[1] == 4 and bool((w >= 0).all()) and bool((np.abs(sums-1) <= 1e-5).all()), f'{tag}: invalid four normalized nonnegative weights')
                if 'JOINTS_0' in arrays:
                    j = arrays['JOINTS_0']
                    gate(bool(((j < joint_count) | (w == 0)).all()), f'{tag}: weighted joint outside skin')
            if 'NORMAL' in arrays:
                lengths = np.linalg.norm(arrays['NORMAL'], axis=1)
                record['normal_length_range'] = [float(lengths.min()), float(lengths.max())]
                gate(bool((np.abs(lengths-1) < 1e-3).all()), f'{tag}: normals not normalized')
            primitives.append(record)
    gate(total_vertices <= 65535, 'More than 65,535 exported vertices')
    for si, skin in enumerate(skins):
        ids = skin.get('joints', [])
        gate(len(ids) == len(set(ids)) and all(0 <= n < len(nodes) for n in ids), f'skin{si}: invalid joint node indices')
        if 'inverseBindMatrices' not in skin:
            gate(False, f'skin{si}: missing inverse binds')
        else:
            a = root['accessors'][skin['inverseBindMatrices']]
            gate(a['componentType'] == 5126 and a['type'] == 'MAT4' and a['count'] == len(ids), f'skin{si}: invalid inverse bind layout')
            ib = accessor(root, binary, skin['inverseBindMatrices'])
            gate(bool(np.isfinite(ib).all()), f'skin{si}: nonfinite inverse binds')
    animations = []
    for ai, animation in enumerate(root.get('animations', [])):
        record = {'name': animation.get('name', ''), 'channels': len(animation.get('channels', [])), 'duration': 0, 'translation_ranges_local': []}
        for sampler in animation.get('samplers', []):
            gate(sampler.get('interpolation', 'LINEAR') == 'LINEAR', f'animation{ai}: must be baked LINEAR')
            ina, outa = [root['accessors'][sampler[k]] for k in ('input', 'output')]
            gate(ina['componentType'] == outa['componentType'] == 5126 and ina['count'] == outa['count'], f'animation{ai}: unequal or nonfloat keys')
            times = accessor(root, binary, sampler['input']).reshape(-1)
            gate(bool(np.isfinite(times).all()) and bool((np.diff(times) > 0).all()) and bool(len(times)), f'animation{ai}: invalid key times')
            if len(times): record['duration'] = max(record['duration'], float(times.max()))
        for channel in animation.get('channels', []):
            target = channel['target']
            gate(target.get('path') in ('translation', 'rotation', 'scale'), f'animation{ai}: non-TRS channel')
            if target.get('path') == 'translation':
                out = accessor(root, binary, animation['samplers'][channel['sampler']]['output'])
                record['translation_ranges_local'].append({'node': nodes[target['node']].get('name', target['node']),
                    'range': np.ptp(out, axis=0).tolist(), 'end_minus_start': (out[-1]-out[0]).tolist()})
        gate(record['duration'] > 0 and record['channels'] > 0, f'animation{ai}: empty clip')
        animations.append(record)
    gate(bool(animations), 'No usable animation clips')
    names = [a['name'] for a in animations]
    gate(len(set(names)) == len(names) and all(names), 'Animation names must be nonempty and unique')
    images = []
    for ii, item in enumerate(root.get('images', [])):
        record = {'index': ii, 'name': item.get('name'), 'mimeType': item.get('mimeType')}
        if 'bufferView' in item:
            v = root['bufferViews'][item['bufferView']]
            image_bytes = binary[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']]
            with Image.open(io.BytesIO(image_bytes)) as img:
                record.update(size=list(img.size), format=img.format, bytes=len(image_bytes))
                gate(max(img.size) <= 1024, f'image{ii}: exceeds 1K runtime cap')
        else:
            record['uri'] = item.get('uri')
            gate(False, f'image{ii}: image bytes not embedded; companion validation needed')
        images.append(record)
    materials = root.get('materials', [])
    textures = root.get('textures', [])
    for i, material in enumerate(materials):
        pbr = material.get('pbrMetallicRoughness', {})
        gate('baseColorTexture' in pbr and 'metallicRoughnessTexture' in pbr and 'normalTexture' in material, f'material{i}: missing base/normal/metallic-roughness PBR texture')
        for kind, info in [('baseColor', pbr.get('baseColorTexture')), ('normal', material.get('normalTexture')),
                           ('metallicRoughness', pbr.get('metallicRoughnessTexture')), ('occlusion', material.get('occlusionTexture'))]:
            if info is None: continue
            idx = info.get('index', -1)
            valid = 0 <= idx < len(textures)
            gate(valid, f'material{i}: invalid {kind} texture link')
            if valid:
                source = textures[idx].get('source', -1)
                gate(0 <= source < len(images), f'material{i}: missing {kind} image source')
            gate(info.get('texCoord', 0) == 0 and not info.get('extensions'), f'material{i}: {kind} must use untransformed UV0')
    if any('matrix' in n for n in nodes):
        notes.append('Accessor-space bounds are not world bounds; matrix nodes require normalization')
    return {'source': str(pathlib.Path(path).resolve()), 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
        'meshes': len(meshes), 'skins': len(skins), 'joints': joint_count,
        'joint_names': [nodes[i].get('name', str(i)) for i in skins[0].get('joints', [])] if skins else [],
        'vertices': total_vertices, 'triangles': total_triangles, 'materials': materials, 'primitives': primitives,
        'animations': animations, 'images': images, 'host_format_pass': not fails,
        'host_format_failures': fails, 'notes': notes,
        'not_tested': ['world-scale/axis visual acceptance', 'posed deformation', 'global root-motion/feet analysis',
                       'actual native importer execution', 'native RT material rendering', 'GPU skinning', 'mobile sustained performance']}


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('source')
    p.add_argument('--output')
    args = p.parse_args()
    result = inspect(args.source)
    text = json.dumps(result, indent=2) + '\n'
    if args.output:
        with pathlib.Path(args.output).open('x') as stream: stream.write(text)
    else:
        print(text)
