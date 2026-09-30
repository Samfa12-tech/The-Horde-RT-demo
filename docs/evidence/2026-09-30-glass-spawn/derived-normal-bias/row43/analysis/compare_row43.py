"""Compare two captured backend rays to uploaded-float lantern GLB triangles."""
import argparse
import hashlib
import json
import math
import pathlib
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("glb", type=pathlib.Path)
parser.add_argument("--witness-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent.parent)
args = parser.parse_args()
ROOT = args.witness_root
GLB = args.glb
FILES = {"pipeline": ROOT / "native-path.json", "compute": ROOT / "compute-native-path.json"}


def sub(a, b): return tuple(x - y for x, y in zip(a, b))
def dot(a, b): return sum(x * y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def length(a): return math.sqrt(dot(a, a))


blob = GLB.read_bytes()
if hashlib.sha256(blob).hexdigest() != "34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4":
    raise RuntimeError("Unexpected runtime GLB SHA-256")
sha = hashlib.sha256(blob).hexdigest()
if struct.unpack_from("<III", blob) != (0x46546C67, 2, len(blob)):
    raise RuntimeError("Invalid GLB header")
chunks = {}; offset = 12
while offset < len(blob):
    size, kind = struct.unpack_from("<II", blob, offset)
    chunks[kind] = blob[offset+8:offset+8+size]; offset += size+8
doc = json.loads(chunks[0x4E4F534A]); binary = chunks[0x004E4942]
node = next(n for n in doc["nodes"] if n.get("name") == "LanternGlass")
if any(k in node for k in ("rotation", "scale", "matrix")):
    raise RuntimeError("Unexpected GLB node transform")
primitive = doc["meshes"][node["mesh"]]["primitives"][0]


def accessor(index):
    a = doc["accessors"][index]; view = doc["bufferViews"][a["bufferView"]]
    fmt, size = {5126: ("f", 4), 5123: ("H", 2), 5125: ("I", 4)}[a["componentType"]]
    count = {"VEC3": 3, "SCALAR": 1}[a["type"]]
    start = view.get("byteOffset", 0) + a.get("byteOffset", 0)
    stride = view.get("byteStride", count*size)
    return [struct.unpack_from("<"+fmt*count, binary, start+i*stride) for i in range(a["count"])]


translation = node.get("translation", [0, 0, 0])
positions = [struct.unpack("<fff", struct.pack("<fff", *(v[i]+translation[i] for i in range(3))))
             for v in accessor(primitive["attributes"]["POSITION"])]
indices = [int(v[0]) for v in accessor(primitive["indices"])]
triangles = [tuple(positions[indices[3*i+j]] for j in range(3)) for i in range(len(indices)//3)]
if len(triangles) != 72:
    raise RuntimeError("Unexpected admitted lantern glass topology")
groups = [set(range(i*6, i*6+6)) | set(range(36+i*6, 42+i*6)) for i in range(6)]


def ray_hit(origin, direction, tri_index, tri):
    a, b, c = tri; e1 = sub(b, a); e2 = sub(c, a)
    h = cross(direction, e2); det = dot(e1, h)
    if abs(det) < 1e-18: return None
    inv = 1.0/det; s = sub(origin, a); u = dot(s, h)*inv
    q = cross(s, e1); v = dot(direction, q)*inv; t = dot(e2, q)*inv
    if t <= 0: return None
    outward = cross(e1, e2)
    group = next((i for i, g in enumerate(groups) if tri_index in g), None)
    return {"t": t, "u": u, "v": v, "insideTriangle": u >= 0 and v >= 0 and u+v <= 1,
            "entering": dot(outward, direction) < 0, "component": group}


results = {"investigationOnly": True, "glbSha256": sha, "triangles": len(triangles), "backends": {}}
records = {}
for backend, filename in FILES.items():
    witness = json.loads(filename.read_text())
    path = witness["paths"][0]
    r = path["records"][0]; records[backend] = r
    origin = tuple(r["objectOrigin"+c] for c in "xyz")
    direction = tuple(r["objectDirection"+c] for c in "xyz")
    # Compare both backends against the component responsible for the pipeline
    # mismatch, not each backend's returned hit component.
    group_id = 4
    component_faces = sorted(groups[group_id] & set(range(24, 30)))
    # The six unique outward planes are represented by primitive triangles 24-29;
    # Triangles 60-65 are the complementary halves of those six planar faces,
    # not duplicate transparent candidates or a second volume.
    plane_distances = []
    for tri_index in component_faces:
        a, b, c = triangles[tri_index]
        normal = cross(sub(b, a), sub(c, a)); normal = tuple(x/length(normal) for x in normal)
        plane_distances.append({"primitive": tri_index, "signedDistance": dot(normal, sub(origin, a))})
    candidates = []
    for tri_index, tri in enumerate(triangles):
        # Python's loop-local index is assigned before ray_hit uses it.
        h = ray_hit(origin, direction, tri_index, tri)
        if h:
            h["primitive"] = tri_index
            candidates.append(h)
    candidates.sort(key=lambda x: x["t"])
    comp_hits = [h for h in candidates if h["component"] == group_id]
    nearest_valid = next((h for h in candidates if h["insideTriangle"]), None)
    near_edges = [h for h in comp_hits if h["primitive"] in (61, 63, 65)]
    uploaded_hit = next((h for h in candidates if h["primitive"] == int(r["primitive"])), None)
    captured_vertices = [tuple(r["v"+str(i)+c] for c in "xyz") for i in range(3)]
    vertex_error = max(abs(captured_vertices[i][k]-triangles[int(r["primitive"])][i][k])
                       for i in range(3) for k in range(3))
    results["backends"][backend] = {
        "pixel": path["pixel"], "firstHit": {"primitive": int(r["primitive"]), "t": r["rawQueryT"],
            "entering": bool(r["entering"]), "bary": [r["baryU"], r["baryV"]]},
        "capturedTriangleMaxAbsVertexError": vertex_error,
        "nativeHitTMinusDoublePlaneTForReturnedPrimitive":
            (r["rawQueryT"]-uploaded_hit["t"]) if uploaded_hit else None,
        "worldQueryOrigin": [r["queryOrigin"+c] for c in "xyz"],
        "worldQueryDirection": [r["queryDirection"+c] for c in "xyz"],
        "objectOrigin": origin, "objectDirection": direction, "objectDirectionLength": length(direction),
        "component": group_id,
        "componentOutwardPlaneDistances": plane_distances,
        "maximumOutwardPlaneDistance": max(x["signedDistance"] for x in plane_distances),
        "nearestValidUploadedFloatTriangleHit": nearest_valid,
        "nearestValidSameComponentHits": [h for h in comp_hits if h["insideTriangle"]][:4],
        "sameComponentEdgeCandidates": near_edges,
    }

delta = lambda a, b: max(abs(float(x)-float(y)) for x, y in zip(a, b))
pipe = records["pipeline"]; compute = records["compute"]
results["rayComparison"] = {
    "samePixel": json.loads(FILES["pipeline"].read_text())["paths"][0]["pixel"] == json.loads(FILES["compute"].read_text())["paths"][0]["pixel"],
    "sameCapturedWorldCameraOrigin": delta([pipe["queryOrigin"+c] for c in "xyz"], [compute["queryOrigin"+c] for c in "xyz"]) == 0,
    "worldCameraDirectionMaxAbsDelta": delta([pipe["queryDirection"+c] for c in "xyz"], [compute["queryDirection"+c] for c in "xyz"]),
    "objectOriginMaxAbsDelta": delta([pipe["objectOrigin"+c] for c in "xyz"], [compute["objectOrigin"+c] for c in "xyz"]),
    "objectDirectionMaxAbsDelta": delta([pipe["objectDirection"+c] for c in "xyz"], [compute["objectDirection"+c] for c in "xyz"]),
    "matrixMaxAbsDelta": delta([pipe["m"+str(i)+c] for i in range(4) for c in "xyz"], [compute["m"+str(i)+c] for i in range(4) for c in "xyz"]),
}
print(json.dumps(results, indent=2, allow_nan=False))
