"""Read-only CPU-upload triangle witness for three recorded native RT rays."""
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / 'probe/near-field'
rays = json.loads((ROOT/'geometric-shadow-peak-probe.json').read_text())['records']

def normalized(v):
    length = math.sqrt(sum(x*x for x in v))
    return tuple(x/length for x in v)
forward = normalized((math.sin(math.pi/2), .23, -math.cos(math.pi/2)))
right = normalized((-forward[2], 0, forward[0]))
up = (-forward[1], forward[0], 0)
screen = (((577.5/960)*2-1)*(960/540), ((534.5/540)*2-1)*-.74)
primary_direction = normalized(tuple(forward[i]*1.22+right[i]*screen[0]+up[i]*screen[1] for i in range(3)))
rays.append(dict(zip(('ox','oy','oz','dx','dy','dz','distance','t','slot'),
                    (-35.5,.70,-15.2,*primary_direction,10000,-1,-1))))

def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def witness(path):
    vertices, triangles = [], []
    group = None
    for line in path.read_text().splitlines():
        if line.startswith('# model_to_world_row_major_3x4 '):
            matrix = list(map(float, line.split()[2:]))
        elif line.startswith('v '):
            v = list(map(float, line.split()[1:]))
            vertices.append(tuple(sum(matrix[r*4+c]*v[c] for c in range(3))+matrix[r*4+3] for r in range(3)))
        elif line.startswith('g '):
            group = line[2:]
        elif line.startswith('f '):
            ids = [int(value.split('/')[0])-1 for value in line.split()[1:]]
            assert len(ids) == 3
            triangles.append((group, ids))
    results = []
    for ray in rays:
        origin = tuple(ray[name] for name in ('ox','oy','oz'))
        direction = tuple(ray[name] for name in ('dx','dy','dz'))
        hits = []
        for face_index, (group, ids) in enumerate(triangles):
            if ray['slot'] < 0 and 'player-world-body' in path.name and group != 'BodyRemainderPrimaryVisible':
                continue
            a,b,c = (vertices[i] for i in ids)
            e1,e2 = sub(b,a),sub(c,a)
            p = cross(direction,e2)
            determinant = dot(e1,p)
            if abs(determinant) < 1e-14: continue
            q = sub(origin,a)
            u = dot(q,p)/determinant
            if not 0 <= u <= 1: continue
            v = dot(direction,cross(q,e1))/determinant
            if v < 0 or u+v > 1: continue
            t = dot(e2,cross(q,e1))/determinant
            if t <= 1e-6 or t > ray['distance']: continue
            normal = normalized(cross(e1,e2))
            hits.append(dict(t=t, face=face_index, group=group, vertices=[a,b,c],
                             barycentric=[1-u-v,u,v], frontFace=determinant>0,
                             geometricNormal=normal,
                             position=tuple(origin[i]+direction[i]*t for i in range(3))))
        hits.sort(key=lambda hit: hit['t'])
        results.append(dict(slot=ray['slot'], nativeT=ray['t'],
                            closest=hits[0] if hits else None,
                            nativeDeltaMetres=abs(hits[0]['t']-ray['t']) if hits and ray['slot'] >= 0 else None))
    return dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                note='CPU-upload witness, not GPU readback. Native query records remain primary device evidence.',
                results=results)

meshes = [witness(ROOT/'11-finale-roof.obj'), witness(ROOT/'11-finale-roof.player-world-body.obj')]
receiver = meshes[1]['results'][-1]['closest']
relationships = [dict(slot=ray['slot'],
                      geometricNormalDotLightDirection=dot(receiver['geometricNormal'], tuple(ray[x] for x in ('dx','dy','dz'))),
                      originOffsetAlongOutwardNormal=dot(sub(tuple(ray[x] for x in ('ox','oy','oz')), receiver['position']), receiver['geometricNormal']))
                 for ray in rays if ray['slot'] >= 0]
result = dict(investigationOnly=True, meshes=meshes, receiverLightRelationships=relationships)
print(json.dumps(result, indent=2))
