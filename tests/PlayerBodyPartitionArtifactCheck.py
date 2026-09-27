"""Compare actual GLBs: only reclassify torso; preserve every triangle attribute.

Usage: python PlayerBodyPartitionArtifactCheck.py old-world new-world old-view new-view
This is offline evidence, not a native image, live-motion or owner acceptance gate.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys


def triangles(path):
    raw = path.read_bytes()
    assert struct.unpack_from('<III', raw) == (0x46546c67, 2, len(raw)), 'Invalid GLB header'
    offset, document, binary = 12, None, None
    while offset < len(raw):
        length, kind = struct.unpack_from('<II', raw, offset)
        chunk = raw[offset+8:offset+8+length]
        if kind == 0x4e4f534a:
            document = json.loads(chunk)
        elif kind == 0x004e4942:
            binary = chunk
        offset += 8 + length
    assert document is not None and binary is not None

    def values(index):
        accessor = document['accessors'][index]
        assert 'sparse' not in accessor, 'Sparse accessors need explicit comparison support'
        view = document['bufferViews'][accessor['bufferView']]
        component_count = {'SCALAR':1, 'VEC2':2, 'VEC3':3, 'VEC4':4}[accessor['type']]
        fmt = '<' + {5121:'B', 5123:'H', 5125:'I', 5126:'f'}[accessor['componentType']]*component_count
        start = view.get('byteOffset',0) + accessor.get('byteOffset',0)
        stride = view.get('byteStride',struct.calcsize(fmt))
        return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(accessor['count'])]

    result = {}
    assert len(document['meshes']) == 1
    for primitive in document['meshes'][0]['primitives']:
        assert primitive.get('mode',4) == 4
        material = document['materials'][primitive['material']]['name']
        assert material not in result, 'Duplicate semantic primitive'
        attributes = {name:values(index) for name,index in sorted(primitive['attributes'].items())}
        assert set(attributes) == {'POSITION','NORMAL','TANGENT','TEXCOORD_0','JOINTS_0','WEIGHTS_0'}
        indices = [value[0] for value in values(primitive['indices'])]
        assert len(indices)%3 == 0
        expanded = Counter()
        for start in range(0,len(indices),3):
            corners = tuple(tuple((name,rows[i]) for name,rows in attributes.items())
                            for i in indices[start:start+3])
            # Cyclic rotation preserves triangle winding; reversal does not.
            expanded[min(corners,corners[1:]+corners[:1],corners[2:]+corners[:2])] += 1
        result[material] = expanded
    return result, [document['nodes'][i]['name'] for i in document['skins'][0]['joints']]


def main(paths):
    assert len(paths)==4, __doc__
    old_world,new_world,old_view,new_view = map(Path, paths)
    old,joints = triangles(old_world)
    new,new_joints = triangles(new_world)
    assert joints == new_joints, 'Skin joint addressing changed'
    assert set(old) == set(new) == {'BodyPrimaryVisible','GauntletPrimaryVisible',
        'HeadPrimaryMasked','NearFacePrimaryMasked','BodyRemainderPrimaryVisible'}
    for name in ('BodyPrimaryVisible','GauntletPrimaryVisible'):
        assert old[name] == new[name], 'Arm/hand triangle attributes changed: '+name
    assert old['HeadPrimaryMasked'] == new['HeadPrimaryMasked'] + new['NearFacePrimaryMasked'], \
        'Existing head mask was exposed, removed or geometrically changed'
    assert old['NearFacePrimaryMasked'] + old['BodyRemainderPrimaryVisible'] == new['BodyRemainderPrimaryVisible'], \
        'Torso retention changed geometry or retained the wrong faces'
    assert old_view.read_bytes() == new_view.read_bytes(), 'Viewmodel bytes changed'
    print(json.dumps(dict(passed=True, scope=__doc__.splitlines()[0],
        hashes={str(path):hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (old_world,new_world,old_view,new_view)},
        trianglesBefore={name:sum(value.values()) for name,value in old.items()},
        trianglesAfter={name:sum(value.values()) for name,value in new.items()},
        checks=['exact expanded position/normal/tangent/UV/joints/weights and winding',
                'unchanged complete head-mask triangle union', 'unchanged arm and hand faces',
                'only old torso band added to body primary', 'byte-identical viewmodel']),indent=2))


if __name__ == '__main__':
    main(sys.argv[1:])
