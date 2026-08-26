import json
import math
import pathlib
import struct


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets/models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb"
TEST_OUTPUT = ROOT / "tests/fixtures/dielectric-topology"

positions = [
    (-0.50, -0.50, -0.50), (0.50, -0.50, -0.50),
    (0.50, 0.50, -0.50), (-0.50, 0.50, -0.50),
    (-0.50, -0.50, 0.50), (0.50, -0.50, 0.50),
    (0.50, 0.50, 0.50), (-0.50, 0.50, 0.50),
]
indices = [
    0, 2, 1, 0, 3, 2,
    4, 5, 6, 4, 6, 7,
    0, 4, 7, 0, 7, 3,
    1, 2, 6, 1, 6, 5,
    0, 1, 5, 0, 5, 4,
    3, 7, 6, 3, 6, 2,
]
normals = []
for position in positions:
    length = math.sqrt(sum(value * value for value in position))
    normals.append(tuple(value / length for value in position))
uvs = [
    (0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0),
    (0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0),
]

def write_glb(output, mesh_indices, node_fields=None, include_volume=True,
              thickness_factor=1.0):
    binary = bytearray()
    for values in positions:
        binary.extend(struct.pack("<3f", *values))
    normal_offset = len(binary)
    for values in normals:
        binary.extend(struct.pack("<3f", *values))
    uv_offset = len(binary)
    for values in uvs:
        binary.extend(struct.pack("<2f", *values))
    index_offset = len(binary)
    for value in mesh_indices:
        binary.extend(struct.pack("<H", value))
    while len(binary) % 4:
        binary.append(0)

    material_extensions = {
        "KHR_materials_transmission": {"transmissionFactor": 0.94},
    }
    extension_names = ["KHR_materials_transmission", "KHR_materials_ior"]
    if include_volume:
        extension_names.insert(1, "KHR_materials_volume")
        material_extensions["KHR_materials_volume"] = {
            "thicknessFactor": thickness_factor,
            "attenuationDistance": 2.4,
            "attenuationColor": [0.72, 0.90, 1.0],
        }
    material_extensions["KHR_materials_ior"] = {"ior": 1.52}
    node = {"name": "GenericDielectricFixture", "mesh": 0}
    if node_fields:
        node.update(node_fields)
    document = {
        "asset": {"version": "2.0", "generator": "Horde RT deterministic dielectric fixture"},
        "extensionsUsed": extension_names,
        "extensionsRequired": extension_names,
        "buffers": [{"byteLength": len(binary)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 96, "target": 34962},
            {"buffer": 0, "byteOffset": normal_offset, "byteLength": 96, "target": 34962},
            {"buffer": 0, "byteOffset": uv_offset, "byteLength": 64, "target": 34962},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": len(mesh_indices) * 2, "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 8, "type": "VEC3",
             "min": [-0.5, -0.5, -0.5], "max": [0.5, 0.5, 0.5]},
            {"bufferView": 1, "componentType": 5126, "count": 8, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": 8, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5123, "count": len(mesh_indices), "type": "SCALAR"},
        ],
        "materials": [{
            "name": "GenericClosedGlass",
            "doubleSided": True,
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.82, 0.94, 1.0, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.12,
            },
            "extensions": material_extensions,
        }],
        "meshes": [{"name": "GenericDielectricFixture", "primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
            "indices": 3, "material": 0, "mode": 4,
        }]}],
        "nodes": [node],
        "scenes": [{"nodes": [0]}],
        "scene": 0,
    }
    json_bytes = json.dumps(document, separators=(",", ":")).encode("utf-8")
    while len(json_bytes) % 4:
        json_bytes += b" "
    total_length = 12 + 8 + len(json_bytes) + 8 + len(binary)
    glb = bytearray(struct.pack("<III", 0x46546C67, 2, total_length))
    glb.extend(struct.pack("<II", len(json_bytes), 0x4E4F534A))
    glb.extend(json_bytes)
    glb.extend(struct.pack("<II", len(binary), 0x004E4942))
    glb.extend(binary)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(glb)
    print(output)


manifest = {
    "schema": 1,
    "asset": "generic-dielectric-topology-fixtures",
    "metresPerUnit": 1.0,
    "coordinateSystem": {"up": "+Y", "forward": "+Z"},
    "budgets": {
        "maxVertices": 8, "maxIndices": 48, "maxPrimitives": 1,
        "maxMaterials": 1, "maxTextureLayersPerKind": 1,
    },
    "lods": [{"name": "lod0", "maxTriangles": 16}],
    "requiredSockets": [],
    "runtimeTextureProfile": {"android": "astc", "windows": "rgba8", "mipmapped": True},
    "materialOverrides": [],
}

write_glb(OUTPUT, indices)
write_glb(TEST_OUTPUT / "closed-dielectric-lod0.runtime.glb", indices)
write_glb(TEST_OUTPUT / "open-dielectric-lod0.runtime.glb", indices[3:])
write_glb(TEST_OUTPUT / "non-manifold-dielectric-lod0.runtime.glb", indices + indices[:3])
inward_indices = []
for triangle_offset in range(0, len(indices), 3):
    inward_indices.extend((indices[triangle_offset], indices[triangle_offset + 2],
                           indices[triangle_offset + 1]))
single_face_flipped = list(indices)
single_face_flipped[1], single_face_flipped[2] = (
    single_face_flipped[2], single_face_flipped[1])
write_glb(TEST_OUTPUT / "inward-dielectric-lod0.runtime.glb", inward_indices)
write_glb(TEST_OUTPUT / "single-face-flipped-dielectric-lod0.runtime.glb",
          single_face_flipped)
write_glb(TEST_OUTPUT / "valid-transformed-dielectric-lod0.runtime.glb", indices,
          {"matrix": [2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 1, 2, 3, 1]})
write_glb(TEST_OUTPUT / "negative-scale-dielectric-lod0.runtime.glb", indices,
          {"scale": [-1, 1, 1]})
write_glb(TEST_OUTPUT / "millimetre-dielectric-lod0.runtime.glb", indices,
          {"matrix": [0.001, 0, 0, 0, 0, 0.006, 0, 0,
                      0, 0, 0.004, 0, -9.1, -0.3, -15.2, 1]})
write_glb(TEST_OUTPUT / "transmission-only-lod0.runtime.glb", indices,
          include_volume=False)
write_glb(TEST_OUTPUT / "zero-thickness-lod0.runtime.glb", indices,
          thickness_factor=0.0)
TEST_OUTPUT.mkdir(parents=True, exist_ok=True)
(TEST_OUTPUT / "asset.manifest.json").write_text(
    json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
