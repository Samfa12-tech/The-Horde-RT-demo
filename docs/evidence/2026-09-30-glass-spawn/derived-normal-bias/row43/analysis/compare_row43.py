"""Compare two captured backend rays to uploaded-float lantern GLB triangles."""
import argparse
import hashlib
import json
import math
import pathlib
import struct
from fractions import Fraction

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("glb", type=pathlib.Path)
parser.add_argument("--witness-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent.parent)
parser.add_argument("--source-world", action="store_true",
                    help="Also check exact dyadic source transforms against each captured world ray; no GPU predicate.")
parser.add_argument("--relative-world", action="store_true",
                    help="One bounded relative-world interval feasibility check; requires --source-world.")
args = parser.parse_args()
if args.relative_world and not args.source_world:
    parser.error("--relative-world requires --source-world")
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


def source_world_check(record):
    # JSON numbers must be exactly the recorded binary32 inputs, not arbitrary
    # decimal approximations. Fraction then preserves every source product/sum.
    def exact_f32(value):
        rounded = struct.unpack("<f", struct.pack("<f", value))[0]
        if rounded != value or not math.isfinite(value):
            raise RuntimeError("Witness is not a finite exact binary32 value")
        return Fraction.from_float(value)

    origin = tuple(exact_f32(record["queryOrigin"+c]) for c in "xyz")
    direction = tuple(exact_f32(record["queryDirection"+c]) for c in "xyz")
    transform = [tuple(exact_f32(record["m"+str(i)+c]) for c in "xyz") for i in range(4)]

    def world_vertex(vertex):
        source = tuple(exact_f32(v) for v in vertex)
        return tuple(sum(transform[col][row] * source[col] for col in range(3)) +
                     transform[3][row] for row in range(3))

    def exact_hit(index, triangle):
        a, b, c = tuple(world_vertex(v) for v in triangle)
        e1, e2 = sub(b, a), sub(c, a)
        p = cross(direction, e2)
        det = dot(e1, p)
        if det == 0:
            return None
        s = sub(origin, a)
        u = dot(s, p)
        q = cross(s, e1)
        v, t = dot(direction, q), dot(e2, q)
        inside = (u >= 0 and v >= 0 and u+v <= det) if det > 0 else (
            u <= 0 and v <= 0 and u+v >= det)
        return {"primitive": index, "determinant": det, "uNumerator": u,
                "vNumerator": v, "t": t/det, "u": u/det, "v": v/det,
                "insideTriangle": inside, "entering": det > 0}

    hits = [h for i, tri in enumerate(triangles) if (h := exact_hit(i, tri))]
    valid = sorted((h for h in hits if h["insideTriangle"] and h["t"] > 0), key=lambda h: h["t"])
    corner = next(h for h in hits if h["primitive"] == 63)
    entry = next(h for h in hits if h["primitive"] == 9)
    # Finite retained regression, not an epsilon or a general hardware bound.
    if corner["determinant"] >= 0 or corner["vNumerator"] <= 0 or corner["insideTriangle"]:
        raise RuntimeError("Source-world primitive63 outside-edge witness changed")
    if not entry["insideTriangle"] or not entry["entering"] or valid[0]["primitive"] != 9:
        raise RuntimeError("Source-world nearest valid primitive9 entry changed")

    def receipt(hit):
        return {key: float(value) if isinstance(value, Fraction) else value for key, value in hit.items()} | {
            "exactVNumeratorSign": (hit["vNumerator"] > 0) - (hit["vNumerator"] < 0),
            "exactVNumeratorBits": abs(hit["vNumerator"].numerator).bit_length(),
            "exactVDenominatorBits": hit["vNumerator"].denominator.bit_length(),
        }

    # Test whether even ideal outward-rounded binary32 arithmetic could certify
    # this sign via the straightforward world-triangle calculation. This is a
    # reference feasibility check, NOT a Vulkan arithmetic guarantee/predicate.
    def endpoint(value, lower):
        rounded = struct.unpack("<f", struct.pack("<f", float(value)))[0]
        bits = struct.unpack("<I", struct.pack("<f", rounded))[0]
        if lower and Fraction.from_float(rounded) > value:
            bits = 0x80000001 if rounded == 0 else bits + (1 if rounded < 0 else -1)
        elif not lower and Fraction.from_float(rounded) < value:
            bits = 1 if rounded == 0 else bits + (-1 if rounded < 0 else 1)
        return Fraction.from_float(struct.unpack("<f", struct.pack("<I", bits))[0])

    def enclose(low, high): return (endpoint(low, True), endpoint(high, False))
    def add(a, b): return enclose(a[0]+b[0], a[1]+b[1])
    def subtract(a, b): return enclose(a[0]-b[1], a[1]-b[0])
    def multiply(a, b):
        products = [x*y for x in a for y in b]
        return enclose(min(products), max(products))
    def interval_cross(a, b):
        return tuple(subtract(multiply(a[j], b[k]), multiply(a[k], b[j]))
                     for j, k in ((1, 2), (2, 0), (0, 1)))
    def interval_dot(a, b):
        return add(add(multiply(a[0], b[0]), multiply(a[1], b[1])), multiply(a[2], b[2]))

    imatrix = [[(x, x) for x in column] for column in transform]
    def interval_world_vertex(vertex):
        source = tuple((x, x) for x in map(exact_f32, vertex))
        return tuple(add(add(add(multiply(imatrix[0][row], source[0]),
                                 multiply(imatrix[1][row], source[1])),
                             multiply(imatrix[2][row], source[2])), imatrix[3][row]) for row in range(3))

    a, b, c = tuple(interval_world_vertex(v) for v in triangles[63])
    e1 = tuple(subtract(y, x) for x, y in zip(a, b))
    e2 = tuple(subtract(y, x) for x, y in zip(a, c))
    d = tuple((x, x) for x in direction)
    s = tuple(subtract((x, x), y) for x, y in zip(origin, a))
    det_interval = interval_dot(e1, interval_cross(d, e2))
    v_interval = interval_dot(d, interval_cross(s, e1))
    if not (det_interval[0] <= corner["determinant"] <= det_interval[1] and
            v_interval[0] <= corner["vNumerator"] <= v_interval[1]):
        raise RuntimeError("Binary32 interval missed exact rational reference")
    outside_proved = ((det_interval[1] < 0 and v_interval[0] > 0) or
                      (det_interval[0] > 0 and v_interval[1] < 0))

    result = {"arithmetic": "exact rational operations on finite captured/uploaded binary32 inputs",
            "geometry": "source triangle plus captured 3x4 objectToWorld; no rounded worldToObject ray",
            "scope": "mathematical source-world reference, not native hardware error bounds or runtime filtering",
            "corner63": receipt(corner), "entry9": receipt(entry),
            "nearestValidPrimitive": valid[0]["primitive"],
            "straightforwardBinary32Interval": {
                "arithmetic": "ideal directed binary32 endpoints; feasibility only, no GPU arithmetic assumption",
                "determinant": list(map(float, det_interval)), "vNumerator": list(map(float, v_interval)),
                "containsExactReference": True, "outsideVProved": outside_proved}}

    if args.relative_world:
        # Predeclared alternative, not an expression/epsilon search: translate
        # the ray first and transform source edges directly, avoiding subtracting
        # two absolute world vertices. This is exactly the same geometric input.
        local = [tuple((x, x) for x in map(exact_f32, v)) for v in triangles[63]]
        def linear(vector):
            return tuple(add(add(multiply(imatrix[0][row], vector[0]),
                                 multiply(imatrix[1][row], vector[1])),
                             multiply(imatrix[2][row], vector[2])) for row in range(3))
        relative_edges = [linear(tuple(subtract(y, x) for x, y in zip(local[0], vertex)))
                          for vertex in local[1:]]
        transformed_a = linear(local[0])
        relative_s = tuple(subtract(subtract((x, x), imatrix[3][row]), transformed_a[row])
                           for row, x in enumerate(origin))
        relative_det = interval_dot(relative_edges[0], interval_cross(d, relative_edges[1]))
        relative_v = interval_dot(d, interval_cross(relative_s, relative_edges[0]))
        contains = (relative_det[0] <= corner["determinant"] <= relative_det[1] and
                    relative_v[0] <= corner["vNumerator"] <= relative_v[1])
        if not contains:
            raise RuntimeError("Relative-world binary32 interval missed exact rational reference")
        relative_outside = ((relative_det[1] < 0 and relative_v[0] > 0) or
                            (relative_det[0] > 0 and relative_v[1] < 0))
        result["relativeWorldBinary32Interval"] = {
            "arithmetic": "ideal directed binary32 endpoints; feasibility only, no GPU arithmetic assumption",
            "expression": "M*(b-a), M*(c-a), (origin-translation)-M*a",
            "determinant": list(map(float, relative_det)), "vNumerator": list(map(float, relative_v)),
            "containsExactReference": contains, "outsideVProved": relative_outside,
        }
    return result


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
    if args.source_world:
        results["backends"][backend]["sourceWorldWitness"] = {
            "file": filename.name, "sha256": hashlib.sha256(filename.read_bytes()).hexdigest(),
            "decodedPngSha256": witness["sha256"],
        }
        results["backends"][backend]["sourceWorldExactReference"] = source_world_check(r)

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
