"""Check the closed forest runtime roster, exact bytes, and native TANGENT attributes."""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / "assets/models/world/runtime/forest-tree-pair-v1"
SOURCE = ROOT / "assets/models/world/source/forest-tree-pair-v1"
EVIDENCE = SOURCE / "evidence/runtime-tangents"
EXPECTED = {
    "horde-irregular-pine-v1-lod1.glb": {
        "bytes": 1402952,
        "sha256": "db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f",
        "originalSha256": "77c793480abff4ed7515b224a1b4fc55ac840f27d0b1ef9a17dd1ca58ee1ee2c",
        "triangles": 5000,
    },
    "horde-upright-alder-v1-lod1.glb": {
        "bytes": 1436640,
        "sha256": "f545d7e08370b5e451597c3fd903d8d93b6703d11c75039181a04f7f5312e0bb",
        "originalSha256": "38333b05a996d46942fb9fddb022e128a0591c372582b520f4d051a308e9282d",
        "triangles": 5102,
    },
}


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def glb_document(path):
    data = path.read_bytes()
    check(data[:4] == b"glTF" and struct.unpack_from("<I", data, 4)[0] == 2,
          f"not a GLB2 file: {path.name}")
    total_length = struct.unpack_from("<I", data, 8)[0]
    check(total_length == len(data), f"GLB length header mismatch: {path.name}")
    json_length, json_type = struct.unpack_from("<II", data, 12)
    check(json_type == 0x4E4F534A, f"first GLB chunk is not JSON: {path.name}")
    return json.loads(data[20:20 + json_length].decode("utf-8").rstrip(" \t\r\n\0"))


def main():
    roster = json.loads((RUNTIME / "runtime-roster.json").read_text(encoding="utf-8-sig"))
    check(roster["policy"] == "closed-roster" and roster["schema"] == 1,
          "runtime roster does not declare a versioned closed set")
    expected_names = set(EXPECTED) | {"asset.manifest.json", "runtime-roster.json"}
    actual_names = {path.name for path in RUNTIME.iterdir() if path.is_file()}
    check(actual_names == expected_names, "runtime directory contains files outside its exact closed roster")
    files = {entry["path"]: entry for entry in roster["files"]}
    check(set(files) == set(EXPECTED), "runtime roster model set differs from the selected LOD1 pair")
    for name, expected in EXPECTED.items():
        path = RUNTIME / name
        entry = files[name]
        check(path.stat().st_size == expected["bytes"] == entry["bytes"] and
              digest(path) == expected["sha256"] == entry["sha256"],
              f"runtime GLB does not match its exact roster: {name}")
        original = SOURCE / "runtime" / name
        check(digest(original) == expected["originalSha256"] == entry["originalLod1Sha256"],
              f"original LOD1 source changed: {name}")
        receipt_name = "pine-lod1-tangent-receipt.json" if "pine" in name else "alder-lod1-tangent-receipt.json"
        receipt_path = EVIDENCE / receipt_name
        receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
        check(receipt["outputSha256"] == expected["sha256"] and receipt["status"] == "pass" and
              receipt["inputSha256"] == expected["originalSha256"] and
              receipt["geometryUnchanged"] and receipt["embeddedImagePixelsUnchanged"],
              f"tangent derivation evidence does not match runtime: {name}")
        document = glb_document(path)
        check(len(document.get("materials", [])) == 3 and len(document.get("meshes", [])) == 3,
              f"runtime GLB material/mesh roster changed: {name}")
        triangles = 0
        tangent_primitives = 0
        for mesh in document["meshes"]:
            for primitive in mesh["primitives"]:
                attributes = primitive["attributes"]
                check("POSITION" in attributes and "NORMAL" in attributes and "TEXCOORD_0" in attributes,
                      f"runtime primitive lacks required PBR attributes: {name}")
                if "TANGENT" in attributes:
                    tangent_primitives += 1
                index_count = document["accessors"][primitive["indices"]]["count"]
                check(index_count % 3 == 0, f"runtime primitive is not triangle-indexed: {name}")
                triangles += index_count // 3
        check(tangent_primitives == 3, f"all three tree families must carry generated tangent attributes: {name}")
        check(triangles == expected["triangles"] == entry["triangles"],
              f"runtime triangle count differs from approved LOD1: {name}")

    manifest = json.loads((RUNTIME / "asset.manifest.json").read_text(encoding="utf-8-sig"))
    check(manifest["asset"] == "forest-tree-pair-v1-lod1" and manifest["distribution"] == "runtime",
          "forest runtime manifest identity differs")
    check(manifest["budgets"] == {"maxVertices": 5632, "maxIndices": 15306,
                                  "maxPrimitives": 3, "maxMaterials": 3,
                                  "maxTextureLayersPerKind": 3},
          "forest runtime manifest limits differ from the imported pair")
    print("Forest runtime closed roster and tangent payload checks passed.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"Forest runtime roster check failed: {exc}", file=sys.stderr)
        raise
