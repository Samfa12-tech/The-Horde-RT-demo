"""Measure admitted LOD1 bark for the authored woodland collision contract.

Reads source candidates only; --write replaces only the generated C++ header.
No Blender masters, GLBs, texture families or material budgets are modified.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'assets/models/world/source/forest-tree-pair-v1/woodland-placements-v3.json'
HEADER = ROOT / 'src/gameplay/simulation/ForestTreePlacementContract.h'


def bark_triangles(spec):
    data = (ROOT / spec['path']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == spec['sha256'], 'source bytes changed'
    assert data[:4] == b'glTF'
    json_size = struct.unpack_from('<I', data, 12)[0]
    doc = json.loads(data[20:20 + json_size])
    binary = data[28 + json_size:]
    assert all(not any(k in node for k in ('matrix', 'scale', 'translation', 'rotation'))
               for node in doc['nodes']), 'source transforms require explicit baking'
    bark = []
    for mesh in doc['meshes']:
        for primitive in mesh['primitives']:
            acc = doc['accessors'][primitive['attributes']['POSITION']]
            view = doc['bufferViews'][acc['bufferView']]
            assert acc['componentType'] == 5126 and acc['type'] == 'VEC3'
            base = view.get('byteOffset', 0) + acc.get('byteOffset', 0)
            points = [struct.unpack_from('<3f', binary, base + i * view.get('byteStride', 12))
                      for i in range(acc['count'])]
            if doc['materials'][primitive['material']]['name'] == 'Original charcoal-gray ridged bark':
                ia = doc['accessors'][primitive['indices']]
                iv = doc['bufferViews'][ia['bufferView']]
                fmt = '<' + {5123:'H',5125:'I'}[ia['componentType']]
                start = iv.get('byteOffset',0) + ia.get('byteOffset',0)
                indices = [struct.unpack_from(fmt,binary,start+i*struct.calcsize(fmt))[0] for i in range(ia['count'])]
                bark.extend(tuple(points[j] for j in indices[i:i+3]) for i in range(0,len(indices),3))
    return bark


def bark_points(spec):
    points = sorted(set(p for triangle in bark_triangles(spec) for p in triangle))
    root_y = min(p[1] for p in points)
    return [p for p in points if p[1] <= root_y + 2.0]


def clip_y(polygon, height, lower):
    result=[]
    for a,b in zip(polygon,polygon[1:]+polygon[:1]):
        inside_a = a[1]>=height if lower else a[1]<=height
        inside_b = b[1]>=height if lower else b[1]<=height
        if inside_a: result.append(a)
        if inside_a != inside_b:
            t=(height-a[1])/(b[1]-a[1])
            result.append(tuple(a[k]+t*(b[k]-a[k]) for k in range(3)))
    return result


def generate(plan):
    sources = {name: bark_triangles(spec) for name, spec in plan['sources'].items()}
    rows = []
    for index,p in enumerate(plan['placements']):
        x, z, yaw = p['x'], p['z'], p['yawRadians']
        c, s = math.cos(yaw), math.sin(yaw)
        triangles=sources[p['species']]
        burial=plan['grounding']['measurements'][index]['afterBurial']
        floor=min(p[1] for triangle in triangles for p in triangle)+burial
        visible=[p for triangle in triangles for p in clip_y(clip_y(list(triangle),floor,True),floor+2,False)]
        points = [(x + c * a + s * b, z - s * a + c * b)
                  for a, _, b in visible]
        values = [x, z, yaw, min(a for a, _ in points), min(b for _, b in points),
                  max(a for a, _ in points), max(b for _, b in points), burial]
        assert all(math.isfinite(v) for v in values)
        rows.append('    {ForestTreeSpecies::' + p['species'] + ', ' +
                    ', '.join(f'{v:.9f}f' for v in values) + '},')
    return '''#pragma once

#include <array>
#include <cstddef>

namespace horde::gameplay::simulation
{
enum class ForestTreeSpecies : unsigned char { Pine, Alder };

// Generated from woodland-placements-v3.json and the exact admitted GLBs by
// tools/build-forest-placement-contract.py. Shared rendering/collision bounds
// enclose bark triangles clipped from local terrain height through 2 m above
// it, after measured root burial. Buried root tips are not invisible blockers.
struct ForestTreePlacementContract
{
    ForestTreeSpecies species;
    float x;
    float z;
    float yawRadians;
    float trunkMinX;
    float trunkMinZ;
    float trunkMaxX;
    float trunkMaxZ;
    float rootBurialMetres;
};

inline constexpr std::array<ForestTreePlacementContract, ''' + str(len(rows)) + '''u> kForestTreePlacementContract{{
''' + '\n'.join(rows) + '''
}};

} // namespace horde::gameplay::simulation
'''


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    expected = generate(json.loads(PLAN.read_text()))
    if args.write:
        HEADER.write_text(expected, encoding='utf-8', newline='\n')
    else:
        assert HEADER.read_text() == expected, 'woodland placement header is stale'
    print('Measured woodland placement contract: PASS')
