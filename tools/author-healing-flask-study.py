"""Create or byte-verify the original source-only flask; no runtime admission.

Standard-library geometry authoring, using the project's existing direct GLB
format. Output is new-only. --verify reads existing bytes without rewriting.
No provider, image exporter, executable launch, renderer or production mutation.
"""

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/models/props/source/healing-flask-study-r1"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def unit(v):
    length = math.sqrt(dot(v, v))
    require(length > 1e-12 and math.isfinite(length), "Degenerate normal")
    return tuple(x / length for x in v)


def geometry(recipe):
    profile, n = recipe["profileYRadiusXRadiusZ"], recipe["segments"]
    require(n == 16 and len(profile) == 11, "Unexpected study profile")
    require(all(math.isfinite(v) for row in profile for v in row), "Nonfinite profile")
    require(all(b[0] > a[0] for a, b in zip(profile, profile[1:])), "Profile not ascending")
    require(all(rx > 0 and rz > 0 for _, rx, rz in profile), "Invalid radius")
    positions, normals, indices = [], [], []
    for row, (y, rx, rz) in enumerate(profile):
        lo, hi = profile[max(0, row-1)], profile[min(len(profile)-1, row+1)]
        dx, dz = (hi[1]-lo[1])/(hi[0]-lo[0]), (hi[2]-lo[2])/(hi[0]-lo[0])
        for k in range(n):
            c, s = math.cos(math.tau*k/n), math.sin(math.tau*k/n)
            positions.append((rx*c, y, rz*s))
            normals.append(unit((rz*c, -(dx*rz*c*c + dz*rx*s*s), rx*s)))
    for row in range(len(profile)-1):
        for k in range(n):
            a, b = row*n+k, (row+1)*n+k
            c, d = (row+1)*n+(k+1)%n, row*n+(k+1)%n
            indices.extend((a, b, c, a, c, d))
    # Separate cap attributes preserve flat end normals; weld by position below.
    for row, sign in ((0, -1),):
        start = len(positions)
        positions.extend(positions[row*n:(row+1)*n])
        normals.extend([(0, sign, 0)]*n)
        centre = len(positions)
        positions.append((0, profile[row][0], 0))
        normals.append((0, sign, 0))
        for k in range(n):
            a, b = start+k, start+(k+1)%n
            indices.extend((centre, a, b) if sign < 0 else (centre, b, a))
    # An opaque recessed throat reads as an open drinking mouth, without glass,
    # liquid, a separate stopper/animation owner, or overlapping fake dark faces.
    y, rx, rz = profile[-1]
    inner_rx, inner_rz = recipe["mouthBottomRadiusXRadiusZ"]
    depth = recipe["mouthRecessMetres"]
    require(0 < inner_rx < rx and 0 < inner_rz < rz and 0 < depth < y-profile[-3][0]+0.01,
            "Invalid mouth recess")
    start = len(positions)
    for ring_y, ring_rx, ring_rz in ((y,rx,rz), (y-depth,inner_rx,inner_rz)):
        for k in range(n):
            c,s = math.cos(math.tau*k/n), math.sin(math.tau*k/n)
            positions.append((ring_rx*c, ring_y, ring_rz*s))
            normals.append(unit((-depth*ring_rz*c,
                -((inner_rx-rx)*ring_rz*c*c+(inner_rz-rz)*ring_rx*s*s),
                -depth*ring_rx*s)))
    for k in range(n):
        a,b,c,d = start+k,start+n+k,start+n+(k+1)%n,start+(k+1)%n
        indices.extend((a,b,c,a,c,d))
    cap = len(positions)
    positions.extend(positions[start+n:start+2*n])
    normals.extend([(0,1,0)]*n)
    centre = len(positions)
    positions.append((0,y-depth,0))
    normals.append((0,1,0))
    for k in range(n):
        indices.extend((centre,cap+(k+1)%n,cap+k))
    return positions, normals, indices


def encode(recipe):
    positions, normals, indices = geometry(recipe)
    binary, views, accessors = bytearray(), [], []
    for values, kind, width, component, code, target in (
            (positions, "VEC3", 3, 5126, "f", 34962),
            (normals, "VEC3", 3, 5126, "f", 34962),
            ([(i,) for i in indices], "SCALAR", 1, 5123, "H", 34963)):
        offset = len(binary)
        binary.extend(struct.pack("<" + code*len(values)*width,
                                  *(v for row in values for v in row)))
        views.append({"buffer": 0, "byteOffset": offset,
                      "byteLength": len(binary)-offset, "target": target})
        accessor = {"bufferView": len(views)-1, "componentType": component,
                    "count": len(values), "type": kind}
        if len(accessors) == 0:
            stored = struct.unpack_from("<" + "f"*len(values)*width, binary, offset)
            accessor["min"] = [min(stored[a::width]) for a in range(width)]
            accessor["max"] = [max(stored[a::width]) for a in range(width)]
        accessors.append(accessor)
    document = {"asset": {"version": "2.0", "generator": recipe.get("generatorLabel", "Horde original healing-flask study r1")},
                "scene": 0, "scenes": [{"nodes": [0, 1]}],
                "nodes": [{"name": "HealingFlaskStudy", "mesh": 0},
                          {"name": "Grip", "translation": recipe["grip"]}],
                "meshes": [{"name": "HealingFlaskStudy", "primitives": [{
                    "attributes": {"POSITION": 0, "NORMAL": 1},
                    "indices": 2, "material": 0, "mode": 4}]}],
                "materials": [recipe["material"]], "accessors": accessors,
                "bufferViews": views, "buffers": [{"byteLength": len(binary)}]}
    payload = json.dumps(document, separators=(",", ":"), allow_nan=False).encode()
    payload += b" "*(-len(payload)%4)
    binary.extend(b"\0"*(-len(binary)%4))
    return (struct.pack("<III", 0x46546c67, 2, 28+len(payload)+len(binary))
            + struct.pack("<II", len(payload), 0x4e4f534a) + payload
            + struct.pack("<II", len(binary), 0x004e4942) + binary)


def inspect(data, recipe):
    require(struct.unpack_from("<III", data) == (0x46546c67, 2, len(data)), "Bad GLB")
    size, kind = struct.unpack_from("<II", data, 12)
    require(kind == 0x4e4f534a, "Missing JSON")
    doc = json.loads(data[20:20+size])
    binary_size, kind = struct.unpack_from("<II", data, 20+size)
    binary = data[28+size:]
    require(kind == 0x004e4942 and binary_size == len(binary), "Bad BIN")
    def read(index):
        a = doc["accessors"][index]
        view = doc["bufferViews"][a["bufferView"]]
        width = 1 if a["type"] == "SCALAR" else 3
        code = "H" if a["componentType"] == 5123 else "f"
        step = struct.calcsize("<"+code*width)
        return [struct.unpack_from("<"+code*width, binary, view["byteOffset"]+i*step)
                for i in range(a["count"])]
    positions, normals = read(0), read(1)
    indices = [i[0] for i in read(2)]
    require(len(positions) <= recipe["budgets"]["maxVertices"], "Vertex budget")
    require(len(indices)//3 <= recipe["budgets"]["maxTriangles"], "Triangle budget")
    require(len(doc["meshes"]) == len(doc["meshes"][0]["primitives"]) == len(doc["materials"]) == 1,
            "Expected one mesh/primitive/material")
    require(doc["nodes"][1] == {"name": "Grip", "translation": recipe["grip"]}, "Grip changed")
    require(doc["materials"] == [recipe["material"]], "Unexpected material")
    require(not any(k in doc for k in ("images", "textures", "skins", "animations", "extensionsUsed")),
            "Unexpected payload")
    require(all(math.isfinite(v) for p in positions+normals for v in p), "Nonfinite attribute")
    require(all(abs(dot(n,n)-1) < 1e-6 for n in normals), "Nonunit normal")
    directed, volume, min_area = Counter(), 0.0, math.inf
    for i in range(0, len(indices), 3):
        triangle = indices[i:i+3]
        require(max(triangle) < len(positions) and len(set(triangle)) == 3, "Bad index")
        a,b,c = [positions[v] for v in triangle]
        face = cross(sub(b,a), sub(c,a))
        area = math.sqrt(dot(face,face))/2
        require(area > 1e-10, "Degenerate triangle")
        require(dot(face, tuple(sum(normals[v][axis] for v in triangle) for axis in range(3))) > 0,
                "Normal disagrees with winding")
        min_area = min(min_area, area)
        volume += dot(a,cross(b,c))/6
        for u,v in ((a,b),(b,c),(c,a)):
            directed[(u,v)] += 1
    require(all(count == 1 and directed[(v,u)] == 1 for (u,v),count in directed.items()),
            "Not closed/consistently wound after exact position weld")
    require(volume > 0, "Inward volume")
    low = [min(p[a] for p in positions) for a in range(3)]
    high = [max(p[a] for p in positions) for a in range(3)]
    require(doc["accessors"][0]["min"] == low and doc["accessors"][0]["max"] == high, "Bad bounds")
    return {"sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data),
            "vertices": len(positions), "triangles": len(indices)//3,
            "primitives": 1, "materials": 1, "texturePayloads": 0,
            "minimumMetres": low, "maximumMetres": high,
            "dimensionsMetres": [b-a for a,b in zip(low,high)],
            "closedAndConsistentlyWound": True, "signedVolumeM3": volume,
            "minimumTriangleAreaM2": min_area, "grip": recipe["grip"],
            "mouthRecessMetres": recipe["mouthRecessMetres"],
            "scope": "Static bytes/topology only; native import, hand fit, animation and RT acceptance pending"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=SOURCE/"healing-flask-study-r1.glb")
    parser.add_argument("--recipe", type=Path, default=SOURCE/"recipe.json")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    recipe = json.loads(args.recipe.read_text(encoding="utf-8"))
    require(recipe["status"] == "source-only-unadmitted" and recipe["metresPerUnit"] == 1,
            "Source-only convention changed")
    data = encode(recipe)
    require(data == encode(recipe), "Not deterministic")
    result = inspect(data, recipe)
    if args.verify:
        require(args.output.read_bytes() == data, "Exact regeneration differs")
    else:
        with args.output.open("xb") as stream:
            stream.write(data)
    result["operation"] = "verify" if args.verify else "new-only-author"
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
