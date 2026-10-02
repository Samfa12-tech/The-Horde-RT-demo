"""One anatomical diagnostic contrast on cloned GLBs, no production admission.

Only proximal Right gauntlet weights change. All other bytes stay exact.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
import numpy as np

root, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
output.mkdir(parents=True, exist_ok=False)
rows = []
for relative, result_name, material_name in (
        ('assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb', 'viewmodel.glb', 'ViewmodelGauntlets'),
        ('assets/models/player/runtime/gothic-traveller-lod0.runtime.glb', 'world.glb', 'GauntletPrimaryVisible')):
    original = (root / relative).read_bytes()
    data = bytearray(original)
    n = struct.unpack_from('<I', data, 12)[0]
    doc = json.loads(data[20:20+n])
    start = 28+n
    def accessor(index):
        a = doc['accessors'][index]
        v = doc['bufferViews'][a['bufferView']]
        width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
        dtype = {5121: '<u1', 5123: '<u2', 5125: '<u4', 5126: '<f4'}[a['componentType']]
        size = np.dtype(dtype).itemsize
        return np.ndarray((a['count'], width), dtype=dtype, buffer=data,
                          offset=start+v.get('byteOffset',0)+a.get('byteOffset',0),
                          strides=(v.get('byteStride',size*width),size))
    skin = doc['skins'][0]
    names = [doc['nodes'][i]['name'] for i in skin['joints']]
    hand, forearm = names.index('RightHand'), names.index('RightForeArm')
    inverse = accessor(skin['inverseBindMatrices'])
    wrist = np.linalg.inv(inverse[hand].reshape((4,4),order='F'))[:3,3]
    elbow = np.linalg.inv(inverse[forearm].reshape((4,4),order='F'))[:3,3]
    axis = (elbow-wrist)/np.linalg.norm(elbow-wrist)
    p = next(p for p in doc['meshes'][0]['primitives'] if doc['materials'][p['material']]['name'] == material_name)
    position = accessor(p['attributes']['POSITION'])
    joints = accessor(p['attributes']['JOINTS_0'])
    weights = accessor(p['attributes']['WEIGHTS_0'])
    projection = (position-wrist) @ axis
    right = (joints[:,0] == hand) & (weights[:,0] == 1)
    assert np.all(weights[right,1:] == 0)
    selected = right & (projection > 0)
    t = np.clip(projection[selected]/0.04,0,1)
    influence = (t*t*(3-2*t)).astype(np.float32)
    joints[selected,1] = forearm
    weights[selected,0] = 1-influence
    weights[selected,1] = influence
    # No JSON, geometry, UV, normals, tangents, animation or node mutation.
    destination = output/result_name
    destination.write_bytes(data)
    rows.append({'source': relative, 'inputSha256': hashlib.sha256(original).hexdigest(),
                 'candidate':result_name,'sha256':hashlib.sha256(data).hexdigest(),
                 'changedVertices':int(selected.sum()), 'distalRigidVerticesPreserved':int((right & (projection <= 0)).sum()),
                 'changedBytes':sum(a != b for a,b in zip(original,data)),
                 'blendMetres':0.04})
print(json.dumps({'scope':'Investigation only; not admitted or visually accepted', 'rows':rows},indent=2))
