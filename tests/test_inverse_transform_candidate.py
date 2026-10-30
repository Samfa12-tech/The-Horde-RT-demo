"""CPU normal/tangent regressions for the isolated shader experiment, not GPU proof."""
import importlib.util
import math
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('inverse_candidate', ROOT/'tools/prototype-inverse-transform.py')
candidate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(candidate)


def transpose(m): return list(map(list, zip(*m)))
def dot(a, b): return sum(x*y for x, y in zip(a, b))
def cross(a, b): return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
def apply(m, v): return [dot(row, v) for row in m]
def normalize(v): return [x/math.sqrt(dot(v, v)) for x in v]
def f32(x): return struct.unpack('<f', struct.pack('<f', x))[0]


def inverse_cofactor(m):
    # Independent ALU reference for the old GLSL inverse path.
    cofactors = [cross(m[1], m[2]), cross(m[2], m[0]), cross(m[0], m[1])]
    det = dot(m[0], cofactors[0])
    return [[x/det for x in row] for row in transpose(cofactors)]


def inverse_elimination(m):
    # Independent construction of the supplied inverse, not the cofactor path.
    rows = [list(row)+[float(i == j) for j in range(3)] for i, row in enumerate(m)]
    for c in range(3):
        pivot = max(range(c, 3), key=lambda r: abs(rows[r][c]))
        rows[c], rows[pivot] = rows[pivot], rows[c]
        divisor = rows[c][c]
        rows[c] = [x/divisor for x in rows[c]]
        for r in range(3):
            if r != c:
                scale = rows[r][c]
                rows[r] = [a-scale*b for a, b in zip(rows[r], rows[c])]
    return [row[3:] for row in rows]


MATRICES = {
    'identity': [[1, 0, 0], [0, 1, 0], [0, 0, 1]],
    'rotated-nonuniform': [[0, -3, 0], [2, 0, 0], [0, 0, .5]],
    'shear': [[2, .3, -.2], [0, 1.5, .4], [0, 0, .7]],
    'mirrored-shear': [[-2, .3, -.2], [0, 1.5, .4], [0, 0, .7]],
    'wide-scale': [[.02, .01, 0], [0, 20, 1], [0, 0, 2]],
}


def frame(m, normal_matrix, handedness):
    tangent = normalize([1, 2, .5])
    second = normalize([-.2, .3, 1])
    local_normal = normalize(cross(tangent, second))
    geometric = normalize(cross(apply(m, tangent), apply(m, second)))
    normal = normalize(apply(normal_matrix, local_normal))
    if dot(normal, geometric) < 0:
        normal = [-v for v in normal]
    world_tangent = apply(m, tangent)
    along = dot(normal, world_tangent)
    world_tangent = normalize([a-along*b for a, b in zip(world_tangent, normal)])
    bitangent = [handedness*x for x in normalize(cross(normal, world_tangent))]
    mapped = normalize([.35*t-.2*b+.91*n for t, b, n in zip(world_tangent, bitangent, normal)])
    return normal, world_tangent, bitangent, mapped, geometric


class InverseTransformTests(unittest.TestCase):
    def test_inverse_transpose_and_tangent_frame(self):
        for name, m in MATRICES.items():
            supplied = [[f32(x) for x in row] for row in inverse_elimination(m)]
            for sign in (-1, 1):
                with self.subTest(matrix=name, tangent_w=sign):
                    old = frame(m, transpose(inverse_cofactor(m)), sign)
                    new = frame(m, transpose(supplied), sign)
                    for a, b in zip(old, new):
                        self.assertLess(max(abs(x-y) for x, y in zip(a, b)), 2e-6)
                    n, t, b, mapped, geometric = new
                    self.assertGreater(dot(n, geometric), .999999)
                    self.assertAlmostEqual(dot(n, t), 0, places=6)
                    self.assertAlmostEqual(dot(n, b), 0, places=6)
                    self.assertAlmostEqual(dot(t, b), 0, places=6)
                    self.assertGreater(dot(mapped, geometric), 0)

    def test_nonuniform_fixture_rejects_wrong_transpose(self):
        m = MATRICES['shear']
        good = frame(m, transpose(inverse_elimination(m)), 1)[0]
        wrong = frame(m, inverse_elimination(m), 1)[0]
        self.assertGreater(max(abs(a-b) for a, b in zip(good, wrong)), .1)

    def test_candidate_is_one_committed_query_change(self):
        original = (ROOT/candidate.HIT_DECODE).read_text(encoding='utf-8')
        self.assertEqual(original.count(candidate.OLD), 1)
        changed = original.replace(candidate.OLD, candidate.NEW)
        self.assertEqual(changed.replace(candidate.NEW, candidate.OLD), original)
        self.assertIn('transpose(mat3(rayQueryGetIntersectionWorldToObjectEXT(query, true)))', changed)
        # Position/geometric normal/tangent transforms keep their forward matrix.
        for text in ['objectToWorld * localTangent.xyz', 'objectToWorld * v0.position.xyz',
                     'dot(worldNormal, h.geometricNormal)', 'localTangent.w < 0.0']:
            self.assertIn(text, changed)


if __name__ == '__main__': unittest.main()
