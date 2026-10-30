"""Original three-vertex skin fixture with analytically known tangent frames."""
import json
import math
import struct
from pathlib import Path

binary = bytearray()
views, accessors = [], []

def add(values, fmt, kind, count, component=5126):
    binary.extend(bytes((-len(binary)) % 4))
    data = struct.pack('<' + fmt, *values)
    views.append({'buffer': 0, 'byteOffset': len(binary), 'byteLength': len(data)})
    binary.extend(data)
    accessors.append({'bufferView': len(views)-1, 'componentType': component,
                      'count': count, 'type': kind})
    return len(accessors)-1

attrs = {
    'POSITION': add([0,0,0, 1,0,0, 0,0,1], '9f', 'VEC3', 3),
    'NORMAL': add([0,1,0]*3, '9f', 'VEC3', 3),
    'TANGENT': add([1,0,0,1]*3, '12f', 'VEC4', 3),
    'TEXCOORD_0': add([0,0,1,0,0,1], '6f', 'VEC2', 3),
    'JOINTS_0': add([0]*12, '12B', 'VEC4', 3, 5121),
    'WEIGHTS_0': add([1,0,0,0]*3, '12f', 'VEC4', 3),
}
indices = add([0,2,1], '3H', 'SCALAR', 3, 5123)
bind = add([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1], '16f', 'MAT4', 1)
times = add([0,1], '2f', 'SCALAR', 2)
rotation = add([0,0,0,1, 0,0,math.sqrt(.5),math.sqrt(.5)], '8f', 'VEC4', 2)
reflection = add([1,1,1, -1,1,1], '6f', 'VEC3', 2)
document = {'asset': {'version': '2.0'}, 'scene': 0, 'scenes': [{'nodes': [0,1]}],
    'nodes': [{'name': 'Joint'}, {'name': 'Mesh', 'mesh': 0, 'skin': 0}],
    'meshes': [{'primitives': [{'attributes': attrs, 'indices': indices}]}],
    'skins': [{'joints': [0], 'inverseBindMatrices': bind}],
    'buffers': [{'byteLength': len(binary)}], 'bufferViews': views, 'accessors': accessors,
    'animations': [{'name': name, 'samplers': [{'input': times, 'output': output,
                    'interpolation': 'LINEAR'}], 'channels': [{'sampler': 0,
                    'target': {'node': 0, 'path': path}}]}
                   for name, output, path in [('rotate',rotation,'rotation'),
                                               ('reflect',reflection,'scale')]]}
text = json.dumps(document,separators=(',',':')).encode()
text += b' '*((-len(text)) % 4)
binary.extend(bytes((-len(binary)) % 4))
payload = (struct.pack('<III',0x46546c67,2,28+len(text)+len(binary)) +
           struct.pack('<II',len(text),0x4e4f534a)+text+
           struct.pack('<II',len(binary),0x004e4942)+binary)
path = Path(__file__).with_name('tangent-frames.glb')
if path.exists() and path.read_bytes() != payload:
    raise RuntimeError('Existing fixture differs; preserve it before regenerating')
if not path.exists():
    path.write_bytes(payload)
