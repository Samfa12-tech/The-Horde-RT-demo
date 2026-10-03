"""Compare expanded bind-surface attributes, independent of GLB vertex ordering."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys


def read(path):
    raw = Path(path).read_bytes()
    magic, version, length = struct.unpack_from('<III', raw)
    if magic != 0x46546c67 or version != 2 or length != len(raw):
        raise ValueError('Invalid GLB container')
    offset, doc, binary = 12, None, None
    while offset < length:
        size, kind = struct.unpack_from('<II', raw, offset)
        chunk = raw[offset + 8:offset + 8 + size]
        if kind == 0x4e4f534a:
            doc = json.loads(chunk)
        elif kind == 0x004e4942:
            binary = chunk
        offset += size + 8

    def accessor(index):
        a = doc['accessors'][index]
        v = doc['bufferViews'][a['bufferView']]
        count = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        fmt = '<' + {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}[a['componentType']] * count
        stride = v.get('byteStride', struct.calcsize(fmt))
        start = v.get('byteOffset', 0) + a.get('byteOffset', 0)
        return [struct.unpack_from(fmt, binary, start + i * stride) for i in range(a['count'])]

    surfaces = {}
    for primitive in doc['meshes'][0]['primitives']:
        name = doc['materials'][primitive['material']]['name']
        attrs = [accessor(primitive['attributes'][name])
                 for name in ('POSITION', 'NORMAL', 'TANGENT', 'TEXCOORD_0')]
        indices = [value[0] for value in accessor(primitive['indices'])]
        triangles = []
        for start in range(0, len(indices), 3):
            corners = tuple(tuple(value for attr in attrs for value in attr[index])
                            for index in indices[start:start + 3])
            # Cyclic order changes are equivalent; reversed winding is not.
            triangles.append(min(corners[i:] + corners[:i] for i in range(3)))
        if name in surfaces:
            raise ValueError('Duplicate semantic primitive')
        surfaces[name] = Counter(triangles)
    return surfaces, hashlib.sha256(raw).hexdigest()


before, before_sha = read(sys.argv[1])
after, after_sha = read(sys.argv[2])
after_counts = {name: sum(triangles.values()) for name, triangles in after.items()}
partition_check = len(sys.argv) == 4 and sys.argv[3] == '--rejoin-body-remainder'
if partition_check:
    if 'BodyRemainderPrimaryVisible' in before or 'BodyRemainderPrimaryVisible' not in after:
        raise ValueError('Expected legacy before and extended after profiles')
    after['NearFacePrimaryMasked'].update(after.pop('BodyRemainderPrimaryVisible'))
if before != after:
    raise ValueError('Bind position/normal/tangent/UV/winding/triangle roster changed')
print(json.dumps(dict(beforeSha256=before_sha, afterSha256=after_sha,
    exactExpandedSurfaceAgreement=True,
    triangles=after_counts, rejoinedBodyRemainderForPartitionCheck=partition_check,
    scope='Bind surface only; skin weights intentionally excluded'), indent=2))
