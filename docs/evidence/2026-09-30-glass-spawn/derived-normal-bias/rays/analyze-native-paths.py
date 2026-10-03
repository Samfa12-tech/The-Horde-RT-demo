"""Reuse the retained GLB reader and independently intersect captured native rays."""
import contextlib
import hashlib
import io
import json
import math
from pathlib import Path
import runpy
import sys

reader, glb, witness = map(Path, sys.argv[1:4])
sys.argv = [str(reader), str(glb), str(witness)]
with contextlib.redirect_stdout(io.StringIO()):
    geometry = runpy.run_path(str(reader))
np = geometry["np"]
triangles, components = geometry["triangles"], geometry["components"]
vec, component, intersect = geometry["vec"], geometry["component"], geometry["ray_triangle"]
normals = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
normals /= np.linalg.norm(normals, axis=1)[:, None]
result = []
for path in json.loads(witness.read_text())["paths"]:
    source, returned = path["records"][:2]
    source_component = component(source["primitive"])
    origin, direction = vec(returned, "objectOrigin"), vec(returned, "objectDirection")
    planes = sorted((float(np.dot(normals[i], origin - triangles[i, 0])), i)
                    for i in components[source_component])
    hits = sorted(({"t": t, "primitive": i, "component": component(i),
                    "entering": bool(np.dot(normals[i], direction) < 0)}
                   for i, triangle in enumerate(triangles)
                   if (t := intersect(origin, direction, triangle)) is not None), key=lambda row: row["t"])
    tis = []
    for slot, record in enumerate(path["records"][1:], start=1):
        d, n = vec(record, "d"), vec(record, "n")
        cosine = abs(float(np.dot(n, d))) / float(np.linalg.norm(n) * np.linalg.norm(d))
        critical_cosine = math.sqrt(1 - 1 / (record["ior"] ** 2))
        tis.append({"slot": slot, "primitive": int(record["primitive"]),
                    "component": component(record["primitive"]), "incomingTirCount": record["tir"],
                    "cosine": cosine, "criticalCosine": critical_cosine,
                    "totalInternalReflection": cosine < critical_cosine,
                    "processedBeforeEightInterfaceCeiling": slot < 8})
    result.append({"pixel": path["pixel"], "sourcePrimitive": source["primitive"],
                   "sourceComponent": source_component, "guarded": source["guarded"],
                   "minimumNormalBias": source.get("minimumNormalBias"),
                   "nativeOriginMaximumOutsideLocalMetres": planes[-1][0], "limitingFace": planes[-1][1],
                   "nativeReturnedPrimitive": returned["primitive"], "nativeRawT": returned["rawQueryT"],
                   "doubleIntersectionsOfNativeObjectRay": hits[:5], "followingInterfaces": tis})
print(json.dumps({"investigationOnly": True, "precision": "double using captured GPU float inputs",
                  "glbSha256": hashlib.sha256(glb.read_bytes()).hexdigest(),
                  "witnessSha256": hashlib.sha256(witness.read_bytes()).hexdigest(), "paths": result}, indent=2))
