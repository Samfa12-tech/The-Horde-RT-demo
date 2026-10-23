"""Read-only owned GLB/recipe and exact retained native-camera assessment."""
from pathlib import Path
import hashlib
import json
import math
import struct

ROOT = Path(__file__).resolve().parents[3]
model = ROOT / "assets/models/world/runtime/bellwether-shell-v4/bellwether-shell-v4.glb"
recipe = ROOT / "assets/models/world/source/bellwether-shell-v4/authoring-receipt.json"
manifest = ROOT / "docs/evidence/2026-10-23-f03-masonry/lookout-manifest.json"
data = model.read_bytes()
assert struct.unpack_from("<III", data) == (0x46546c67, 2, len(data))
json_size, kind = struct.unpack_from("<II", data, 12)
assert kind == 0x4e4f534a
doc = json.loads(data[20:20 + json_size])
bin_size, kind = struct.unpack_from("<II", data, 20 + json_size)
assert kind == 0x004e4942
binary = data[28 + json_size:28 + json_size + bin_size]
assert len(doc["nodes"]) == 1 and set(doc["nodes"][0]) == {"mesh", "name"}

def accessor(index):
    item = doc["accessors"][index]
    view = doc["bufferViews"][item["bufferView"]]
    code = {5126: "f", 5125: "I", 5123: "H", 5121: "B"}[item["componentType"]]
    count = {"SCALAR": 1, "VEC3": 3}[item["type"]]
    fmt = "<" + code * count
    stride = view.get("byteStride", struct.calcsize(fmt))
    start = view.get("byteOffset", 0) + item.get("byteOffset", 0)
    return [struct.unpack_from(fmt, binary, start + i * stride) for i in range(item["count"])]

vertices, triangles = [], []
for primitive in doc["meshes"][0]["primitives"]:
    points = accessor(primitive["attributes"]["POSITION"])
    indices = [x[0] for x in accessor(primitive["indices"])]
    vertices.extend(points)
    triangles.extend(tuple(points[indices[i + j]] for j in range(3)) for i in range(0, len(indices), 3))
assert len(triangles) == 966

def sub(a, b): return tuple(x - y for x, y in zip(a, b))
def dot(a, b): return sum(x * y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def normal(a): return tuple(x / math.sqrt(dot(a, a)) for x in a)

capture = json.loads(manifest.read_text(encoding="utf-8-sig"))
view = capture["captures"][0]
assert view["honestlyPresentedRtFrame"] and view["checkpoint"] == "chapter-forest-lookout"
eye = view["rescueTraversal"]["renderedEye"]
yaw, pitch = view["camera"]["yaw"], view["camera"]["pitch"]
forward = normal((math.sin(yaw), -.05 + max(-.32, min(.28, pitch)), -math.cos(yaw)))
right = normal(cross(forward, (0, 1, 0)))
up = normal(cross(right, forward))
width, height = capture["presentation"]["dispatchWidth"], capture["presentation"]["dispatchHeight"]

def project(point):
    v = sub(point, eye)
    depth = dot(v, forward)
    if depth <= 0: return None
    return [(1 + 1.22 * dot(v, right) / depth / (width / height)) * width / 2,
            (1 - 1.22 * dot(v, up) / depth / .74) * height / 2]

def town_occludes(point):
    direction = sub(point, eye)
    for a, b, c in triangles:
        edge1, edge2 = sub(b, a), sub(c, a)
        p = cross(direction, edge2)
        determinant = dot(edge1, p)
        if abs(determinant) < 1e-9: continue
        s = sub(eye, a)
        u = dot(s, p) / determinant
        if u < 0 or u > 1: continue
        q = cross(s, edge1)
        v = dot(direction, q) / determinant
        t = dot(edge2, q) / determinant
        if v >= 0 and u + v <= 1 and 1e-6 < t < 1 - 1e-6: return True
    return False

buildings = json.loads(recipe.read_text())["buildings"]
rows = []
for index in (0, 2, 5):
    b = buildings[index]
    x, z, w, d = b["engineX"], b["engineZ"], b["width"], b["depth"]
    eave = b["actualFloorY"] + b["wallHeight"] - .07
    cap = (x + w * .23, eave + 2.3, z - d * .12)
    corners = [(cap[0] + sx * .55, y, cap[2] + sz * .5)
               for sx in (-1, 1) for sz in (-1, 1) for y in (eave, cap[1])]
    errors = [min(math.dist(corner, vertex) for vertex in vertices) for corner in corners]
    assert max(errors) < 2e-5
    roof_at_cap = eave + min(w * .27, 4.2) * (1 - abs(cap[0] - x) / ((w + .5) / 2))
    probes = []
    for above in (0, .3, 1, 4):
        point = (cap[0], cap[1] + above, cap[2])
        probes.append({"aboveCapMetres": above, "point": point, "pixelAt960x540": project(point),
                       "occludedByTownTrianglesOnly": town_occludes(point)})
    distance = math.dist(cap, eye)
    rows.append({"building": b["id"], "label": b["label"], "capCentre": cap,
                 "distanceFromLookoutMetres": distance, "withinCurrent24mEmitterSelection": distance <= 24,
                 "capCornerMaximumNativeVertexErrorMetres": max(errors),
                 "roofMinusCapAtCentreMetres": roof_at_cap - cap[1], "probes": probes})
print(json.dumps({"sourceHead": "b9019b43a555a3f5994bcd603bc862999613a961",
                  "runtimeSha256": hashlib.sha256(data).hexdigest(), "townTriangles": len(triangles),
                  "cameraSource": str(manifest.relative_to(ROOT)), "eye": eye,
                  "scope": "Actual owned runtime vertices plus exact retained RT camera; CPU town-only occlusion excludes terrain/trees and does not prove complete scene visibility or effect quality/cost.",
                  "chimneys": rows}, indent=2))

