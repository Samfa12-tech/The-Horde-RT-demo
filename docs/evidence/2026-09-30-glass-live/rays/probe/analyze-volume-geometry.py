"""Independent double-precision intersections using exact captured GPU inputs."""
import hashlib
import json
import struct
import sys
from pathlib import Path
import numpy as np

glb = Path(sys.argv[1]); witness = Path(sys.argv[2])
blob = glb.read_bytes()
if struct.unpack_from("<III", blob) != (0x46546C67, 2, len(blob)):
    raise ValueError("Invalid GLB header")
chunks = {}; offset = 12
while offset < len(blob):
    size, kind = struct.unpack_from("<II", blob, offset)
    chunks[kind] = blob[offset+8:offset+8+size]; offset += 8+size
document = json.loads(chunks[0x4E4F534A]); binary = chunks[0x004E4942]
node = next(n for n in document["nodes"] if n.get("name") == "LanternGlass")
if "rotation" in node or "scale" in node or "matrix" in node:
    raise ValueError("Unexpected node transform; do not silently approximate")
primitive = document["meshes"][node["mesh"]]["primitives"][0]
def accessor(index):
    a = document["accessors"][index]; view = document["bufferViews"][a["bufferView"]]
    fmt, size = {5126: ("f",4), 5123: ("H",2), 5125: ("I",4)}[a["componentType"]]
    count = {"VEC3":3,"SCALAR":1}[a["type"]]
    start = view.get("byteOffset",0)+a.get("byteOffset",0)
    stride = view.get("byteStride", count*size)
    return np.array([struct.unpack_from("<"+fmt*count, binary, start+i*stride)
        for i in range(a["count"])])
positions = np.float32(accessor(primitive["attributes"]["POSITION"]))
# StaticMeshAsset.cpp applies this translation with float operands before upload.
positions = np.float32(positions + np.float32(node.get("translation", [0,0,0])))
indices = accessor(primitive["indices"]).astype(int).reshape(-1,3)
triangles = positions[indices].astype(float)
if triangles.shape != (72,3,3): raise ValueError("Unexpected admitted glass topology")
components = [set(range(i*6, i*6+6)) | set(range(36+i*6, 42+i*6)) for i in range(6)]
def component(primitive_id):
    return next(i for i, group in enumerate(components) if int(primitive_id) in group)
def vec(record, prefix): return np.array([record[prefix+c] for c in "xyz"])
def ray_triangle(origin, direction, triangle):
    a,b,c = triangle; e1=b-a; e2=c-a; h=np.cross(direction,e2); determinant=np.dot(e1,h)
    if abs(determinant) < 1e-15: return None
    s=origin-a; u=np.dot(s,h)/determinant
    q=np.cross(s,e1); v=np.dot(direction,q)/determinant; t=np.dot(e2,q)/determinant
    if u < -1e-9 or v < -1e-9 or u+v > 1+1e-9 or t <= 1e-9: return None
    return float(t)
result=[]
for path in json.loads(witness.read_text())["paths"]:
    if len(sys.argv) > 3 and sys.argv[3] == "--object-rays":
        source, returned = path["records"]
        matrix = np.column_stack([vec(source,"m"+str(i)) for i in range(3)])
        translation = vec(source,"m3")
        world_origin = np.array([source["origin"+c] for c in "XYZ"])
        object_origin = vec(returned,"objectOrigin")
        object_direction = vec(returned,"objectDirection")
        inverse_origin = np.linalg.solve(matrix, world_origin-translation)
        group = component(source["primitive"])
        local_normals = np.cross(triangles[:,1]-triangles[:,0],triangles[:,2]-triangles[:,0])
        local_normals /= np.linalg.norm(local_normals,axis=1)[:,None]
        planes = sorted((float(np.dot(local_normals[i],object_origin-triangles[i,0])),i) for i in components[group])
        result.append({"pixel":path["pixel"],"recordRow":path["recordRow"],
            "sourcePrimitive":source["primitive"],"bias":source["epsilon"],
            "minimum":source["entering"],"returnedPrimitive":returned["primitive"],"nativeRawT":returned["t"],
            "objectOrigin":object_origin.tolist(),"inverseOriginDouble":inverse_origin.tolist(),
            "objectOriginDifferenceLocal":(object_origin-inverse_origin).tolist(),
            "actualObjectOriginMaximumOutsideLocalMetres":planes[-1][0],"limitingFace":planes[-1][1],
            "objectRayDoubleIntersections":sorted(
                ({"t":t,"primitive":i,"component":component(i),"entering":bool(np.dot(local_normals[i],object_direction)<0)}
                 for i,tri in enumerate(triangles) if (t:=ray_triangle(object_origin,object_direction,tri)) is not None),
                key=lambda x:x["t"])[:5]})
        continue
    records=[]
    for r in path["records"]:
        primitive_id=int(r["primitive"]); group=component(primitive_id)
        captured=np.array([vec(r,"v"+str(i)) for i in range(3)])
        error=float(np.max(np.abs(captured-triangles[primitive_id])))
        if error != 0: raise ValueError(f"CPU/GPU triangle disagreement: {error}")
        matrix=np.column_stack([vec(r,"m"+str(i)) for i in range(3)])
        translation=vec(r,"m3"); world=triangles @ matrix.T + translation
        normals=np.cross(world[:,1]-world[:,0], world[:,2]-world[:,0])
        normals/=np.linalg.norm(normals,axis=1)[:,None]
        position=vec(r,"p"); origin=np.array([r["origin"+c] for c in "XYZ"])
        outgoing=np.array([r["outgoing"+c] for c in "XYZ"])
        item={"primitive":primitive_id,"component":group,"loadedTriangleMaxError":error}
        if r["epsilon"] > 0:
            planes=sorted((float(np.dot(normals[i], origin-world[i,0])),i) for i in components[group])
            item["biasedOriginMaximumOutsideMetres"], item["limitingFace"]=planes[-1]
            item["hitSignedDistanceToLimitingFaceMetres"]=float(np.dot(normals[planes[-1][1]],position-world[planes[-1][1],0]))
            item["unbiasedIntersections"] = sorted(
                ({"t":t,"primitive":i,"component":component(i),"entering":bool(np.dot(normals[i],outgoing)<0)}
                 for i, tri in enumerate(world) if (t:=ray_triangle(position,outgoing,tri)) is not None),
                key=lambda x:x["t"])[:5]
            item["biasedIntersections"] = sorted(
                ({"t":t,"primitive":i,"component":component(i),"entering":bool(np.dot(normals[i],outgoing)<0)}
                 for i, tri in enumerate(world) if (t:=ray_triangle(origin,outgoing,tri)) is not None),
                key=lambda x:x["t"])[:5]
        records.append(item)
    result.append({"pixel":path["pixel"],"interfaces":records})
print(json.dumps({"investigationOnly":True,"precision":"double using captured float inputs",
    "glbSha256":hashlib.sha256(blob).hexdigest(),"paths":result},indent=2))
