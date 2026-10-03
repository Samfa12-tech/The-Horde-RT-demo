import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def read_glb(path):
    raw = path.read_bytes()
    magic, version, length = struct.unpack_from('<III', raw, 0)
    if magic != 0x46546C67 or version != 2:
        raise ValueError(f'unsupported GLB: {path}')
    offset = 12
    document = None
    binary = None
    while offset < length:
        chunk_length, chunk_type = struct.unpack_from('<II', raw, offset)
        offset += 8
        chunk = raw[offset:offset + chunk_length]
        offset += chunk_length
        if chunk_type == 0x4E4F534A:
            document = json.loads(chunk.decode('utf-8'))
        elif chunk_type == 0x004E4942:
            binary = chunk
    if document is None or binary is None:
        raise ValueError(f'incomplete GLB: {path}')

    def accessor(index):
        item = document['accessors'][index]
        view = document['bufferViews'][item['bufferView']]
        components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[item['type']]
        formats = {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}
        fmt = formats[item['componentType']]
        element_size = struct.calcsize('<' + fmt) * components
        stride = view.get('byteStride', element_size)
        start = view.get('byteOffset', 0) + item.get('byteOffset', 0)
        values = []
        for row in range(item['count']):
            value = struct.unpack_from(
                '<' + fmt * components, binary, start + row * stride)
            values.append(value[0] if components == 1 else value)
        return values

    primitives = []
    for primitive in document['meshes'][0]['primitives']:
        attributes = primitive['attributes']
        material = document['materials'][primitive.get('material', 0)]
        primitives.append({
            'name': material.get('name', 'primitive'),
            'positions': accessor(attributes['POSITION']),
            'indices': accessor(primitive['indices']),
        })
    return primitives


def read_obj(path):
    positions = []
    normals = []
    faces = []
    group = ''
    for line in path.read_text(encoding='utf-8').splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == 'v':
            positions.append(tuple(float(value) for value in fields[1:4]))
        elif fields[0] == 'vn':
            normals.append(tuple(float(value) for value in fields[1:4]))
        elif fields[0] == 'g':
            group = ' '.join(fields[1:])
        elif fields[0] == 'f':
            values = []
            for token in fields[1:4]:
                indices = token.split('/')
                values.append((int(indices[0]) - 1, int(indices[2]) - 1))
            faces.append((group, values))
    return positions, normals, faces


def length(vector):
    return math.sqrt(sum(value * value for value in vector))


def subtract(left, right):
    return tuple(a - b for a, b in zip(left, right))


def cross(left, right):
    return (
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    )


def dot(left, right):
    return sum(a * b for a, b in zip(left, right))


def edge(left, right):
    return length(subtract(left, right))


def percentile(values, fraction):
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int(fraction * (len(ordered) - 1)))]


def primitive_summary(positions, normals, faces, bind_positions):
    edges = []
    stretch = []
    normal_dots = []
    reversed_faces = 0
    degenerate_faces = 0
    outliers = []
    for face_index, face in enumerate(faces):
        vertex_indices = [value[0] for value in face]
        points = [positions[index] for index in vertex_indices]
        face_edges = [edge(points[0], points[1]), edge(points[1], points[2]), edge(points[2], points[0])]
        edges.extend(face_edges)
        geometric_normal = cross(subtract(points[1], points[0]), subtract(points[2], points[0]))
        if length(geometric_normal) * 0.5 < 1e-10 or len(set(vertex_indices)) != 3:
            degenerate_faces += 1
        supplied_normal = tuple(
            sum(normals[value[1]][axis] for value in face) / 3.0
            for axis in range(3)
        )
        if length(geometric_normal) > 1e-12 and length(supplied_normal) > 1e-12:
            alignment = dot(geometric_normal, supplied_normal) / (length(geometric_normal) * length(supplied_normal))
            normal_dots.append(alignment)
            if alignment < 0.0:
                reversed_faces += 1
        bind_points = [bind_positions[index] for index in vertex_indices]
        bind_edge = max(edge(bind_points[0], bind_points[1]), edge(bind_points[1], bind_points[2]), edge(bind_points[2], bind_points[0]))
        posed_edge = max(face_edges)
        ratio = posed_edge / bind_edge if bind_edge > 1e-12 else float('inf')
        stretch.append(ratio)
        outliers.append({
            'face': face_index,
            'vertices': vertex_indices,
            'posedEdgeMetres': posed_edge,
            'bindEdgeMetres': bind_edge,
            'ratio': ratio,
        })
    return {
        'faceCount': len(faces),
        'vertexCount': len(positions),
        'bounds': [[min(point[axis] for point in positions), max(point[axis] for point in positions)] for axis in range(3)],
        'posedEdgeMetres': {
            'max': max(edges),
            'p99': percentile(edges, 0.99),
            'median': percentile(edges, 0.5),
        },
        'stretchRatio': {
            'max': max(stretch),
            'p99': percentile(stretch, 0.99),
            'median': percentile(stretch, 0.5),
            'gt2': sum(value > 2.0 for value in stretch),
            'gt3': sum(value > 3.0 for value in stretch),
            'gt4': sum(value > 4.0 for value in stretch),
            'gt5': sum(value > 5.0 for value in stretch),
        },
        'normalAlignment': {
            'count': len(normal_dots),
            'negative': reversed_faces,
            'lt0_5': sum(value < 0.5 for value in normal_dots),
            'lt0_9': sum(value < 0.9 for value in normal_dots),
            'minimum': min(normal_dots),
            'median': percentile(normal_dots, 0.5),
        },
        'degenerateFaces': degenerate_faces,
        'topStretch': sorted(outliers, key=lambda value: value['ratio'], reverse=True)[:5],
    }


def capture_summary(directory, bind_primitives, captures=('player-viewmodel-grips', 'player-viewmodel-lantern-high')):
    result = {}
    for capture in captures:
        obj = next(directory.joinpath(capture).glob('*.obj'))
        positions, normals, all_faces = read_obj(obj)
        primitives = {}
        offset = 0
        for bind in bind_primitives:
            local_faces = []
            for group, face in all_faces:
                if group == bind['name']:
                    local_faces.append([(vertex - offset, normal - offset) for vertex, normal in face])
            count = len(bind['positions'])
            primitives[bind['name']] = primitive_summary(
                positions[offset:offset + count],
                normals[offset:offset + count],
                local_faces,
                bind['positions'],
            )
            offset += count
        result[capture] = {
            'objSha256': sha256(obj),
            'vertexCount': len(positions),
            'faceCount': len(all_faces),
            'primitives': primitives,
        }
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bind', type=Path, required=True)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--corrected', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    bind_primitives = read_glb(args.bind)
    result = {
        'bindGlbSha256': sha256(args.bind),
        'baseline': capture_summary(args.baseline, bind_primitives),
        'correctedGrip': capture_summary(args.corrected, bind_primitives),
    }
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
