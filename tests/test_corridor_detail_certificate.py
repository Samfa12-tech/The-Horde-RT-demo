"""Exact continuous coverage of the runtime certificate; no sampled rays.

The production guard separately matches these binary32 recipes to the actual
traced opaque BLAS. This test establishes that the recipes cover the complete
north, roof and floor faces. It does not certify other geometry or phone cost.
"""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]


def f32_bits(bits):
    return Q(struct.unpack("<f", struct.pack("<I", bits))[0])


def f32(value):
    return Q(struct.unpack("<f", struct.pack("<f", value))[0])


def cross(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def crossing_x(a, b, c, d):
    r, s = (b[0]-a[0], b[1]-a[1]), (d[0]-c[0], d[1]-c[1])
    denominator = r[0]*s[1]-r[1]*s[0]
    if denominator == 0:
        return None
    ca = (c[0]-a[0], c[1]-a[1])
    t = (ca[0]*s[1]-ca[1]*s[0])/denominator
    u = (ca[0]*r[1]-ca[1]*r[0])/denominator
    return a[0]+t*r[0] if 0 <= t <= 1 and 0 <= u <= 1 else None


def vertical(poly, x):
    ys = [a[1]+(x-a[0])*(b[1]-a[1])/(b[0]-a[0])
          for a, b in zip(poly, poly[1:]+poly[:1])
          if min(a[0], b[0]) < x < max(a[0], b[0])]
    return (min(ys), max(ys)) if len(ys) >= 2 else None


def covers(rect, triangles):
    """Partition at every vertex/edge intersection; order is fixed per slab.

    All boundaries are affine inside each open slab. Exact interval union at
    its midpoint therefore decides the whole slab. Closed triangle unions
    cover the remaining boundaries by continuity, including zero-width seams.
    """
    x0, x1, y0, y1 = rect
    poly = ((x0, y0), (x1, y0), (x1, y1), (x0, y1))
    triangles = [t for t in triangles if cross(*t) != 0]
    segments = list(zip(poly, poly[1:]+poly[:1]))
    for t in triangles:
        segments.extend(zip(t, t[1:]+t[:1]))
    xs = {x0, x1}
    xs.update(p[0] for edge in segments for p in edge if x0 <= p[0] <= x1)
    for (a, b), (c, d) in combinations(segments, 2):
        x = crossing_x(a, b, c, d)
        if x is not None and x0 <= x <= x1:
            xs.add(x)
    ordered = sorted(xs)
    for left, right in zip(ordered, ordered[1:]):
        x = (left+right)/2
        intervals = sorted(i for t in triangles if (i := vertical(t, x)) is not None)
        cursor = y0
        for low, high in intervals:
            if high <= cursor:
                continue
            if low > cursor:
                return False
            cursor = high
            if cursor >= y1:
                break
        if cursor < y1:
            return False
    return True


def quad(x0, x1, y0, y1):
    a, b, c, d = ((Q(x0), Q(y0)), (Q(x1), Q(y0)),
                  (Q(x1), Q(y1)), (Q(x0), Q(y1)))
    return [(a, b, c), (a, c, d)]


def main():
    rect = (Q(0), Q(2), Q(0), Q(2))
    assert covers(rect, quad(0, 2, 0, 2))
    assert covers(rect, quad(0, "1.1", 0, 2)+quad(".9", 2, 0, 2))
    # A very thin gap and a disconnected interior defeat bounds/area checks.
    assert not covers(rect, quad(0, "0.999999", 0, 2)+quad(1, 2, 0, 2))
    assert not covers(rect, quad(0, ".5", 0, ".5")+quad("1.5", 2, "1.5", 2))
    assert not covers(rect, quad(0, 2, 0, 2)[:1])
    assert not covers(rect, [])

    text = (ROOT/"src/scene/CorridorStaticDetailFaces.h").read_text()
    specs = (("kNorthPlusZ", 2, f32(-13.6), (0, 1),
              (Q(-30.5), Q(-8.5), f32(-.95), f32(1.35))),
             ("kRoofPlusY", 1, f32(1.35), (0, 2),
              (Q(-30.5), Q(-8.5), f32(-16.8), f32(-13.6))),
             ("kFloorMinusY", 1, f32(-.95), (0, 2),
              (Q(-30.5), Q(-8.5), f32(-16.8), f32(-13.6))))
    results = []
    for name, axis, plane, uv, bounds in specs:
        body = re.search(r"\b"+name+r"\{\{(.*?)\}\};", text, re.S).group(1)
        records = re.findall(r"\{(\d+)u, \{([^}]+)\}, (\d+)u\}", body)
        triangles = []
        for _, raw, material in records:
            bits = [int(x.rstrip("u"), 16) for x in raw.split(", ")]
            assert len(bits) == 9 and int(material) in (0, 1, 2)
            points = [tuple(f32_bits(v) for v in bits[i:i+3]) for i in (0, 3, 6)]
            assert all(p[axis] == plane for p in points)
            triangles.append(tuple((p[uv[0]], p[uv[1]]) for p in points))
        assert len(triangles) == (2 if name == "kNorthPlusZ" else 4)
        assert covers(bounds, triangles), name+" has an open gap"
        for i in range(len(triangles)):
            assert not covers(bounds, triangles[:i]+triangles[i+1:]), name+" deletion escaped"
        results.append({"face": name, "exactCoverage": True,
                        "triangles": len(triangles), "deletionNegatives": len(triangles)})
    print(json.dumps({"method": "exact rational arrangement of closed binary32 triangles",
                      "syntheticCases": 6, "faces": results}, indent=2))


if __name__ == "__main__":
    main()
