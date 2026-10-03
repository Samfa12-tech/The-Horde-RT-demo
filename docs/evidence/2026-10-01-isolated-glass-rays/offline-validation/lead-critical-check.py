"""Independent plane/interior check of the two distinguishing native candidates."""
import json
from pathlib import Path

root = Path(__file__).parent
paths = json.loads((root / 'path-analysis.json').read_text())['paths']

def sub(a, b):
    return [x-y for x, y in zip(a, b)]

def dot(a, b):
    return sum(x*y for x, y in zip(a, b))

def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

results = []
for pixel in ([346, 1200], [347, 1201]):
    path = next(p for p in paths if p['selectedPixel'] == pixel)
    record = path['records'][4]
    a, b, c = [[record[f'v{i}{axis}'] for axis in 'XYZ'] for i in range(3)]
    origin = [record['object'+axis] for axis in 'XYZ']
    direction = [record['objectD'+axis] for axis in 'XYZ']
    e1, e2 = sub(b, a), sub(c, a)
    normal = cross(e1, e2)
    t = -dot(normal, sub(origin, a))/dot(normal, direction)
    point = [o+t*d for o, d in zip(origin, direction)]
    relative = sub(point, a)
    aa, ab, bb = dot(e1, e1), dot(e1, e2), dot(e2, e2)
    rhs1, rhs2 = dot(relative, e1), dot(relative, e2)
    determinant = aa*bb-ab*ab
    u = (rhs1*bb-rhs2*ab)/determinant
    v = (rhs2*aa-rhs1*ab)/determinant
    error = abs(t-record['rawDistance'])
    margin = min(u, v, 1-u-v)
    assert t > record['queryMinimum'] and error < 1e-6 and margin > 0.05
    results.append({'pixel': pixel, 'primitive': record['primitive'],
                    'nativeT': record['rawDistance'], 'planeIntersectionT': t,
                    'absoluteTResidual': error, 'interiorMargin': margin})
print(json.dumps({'result': 'PASS', 'method': 'double plane intersection and Gram-system barycentrics; native captured vertices/ray',
                  'notGPUEmulation': True, 'checks': results}, indent=2))
