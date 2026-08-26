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

def float32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def float32_add(left, right):
    return float32(float32(left) + float32(right))


def float32_subtract(left, right):
    return float32(float32(left) - float32(right))


def float32_multiply(left, right):
    return float32(float32(left) * float32(right))


def cgltf_local_trs_matrix(translation, rotation, scale):
    """Reference cgltf_node_transform_local expression order in float32."""
    tx, ty, tz = map(float32, translation)
    qx, qy, qz, qw = map(float32, rotation)
    sx, sy, sz = map(float32, scale)
    two = float32(2.0)
    one = float32(1.0)

    def twice_product(left, right):
        return float32_multiply(float32_multiply(two, left), right)

    return [
        float32_multiply(float32_subtract(
            float32_subtract(one, twice_product(qy, qy)),
            twice_product(qz, qz)), sx),
        float32_multiply(float32_add(
            twice_product(qx, qy), twice_product(qz, qw)), sx),
        float32_multiply(float32_subtract(
            twice_product(qx, qz), twice_product(qy, qw)), sx), 0.0,
        float32_multiply(float32_subtract(
            twice_product(qx, qy), twice_product(qz, qw)), sy),
        float32_multiply(float32_subtract(
            float32_subtract(one, twice_product(qx, qx)),
            twice_product(qz, qz)), sy),
        float32_multiply(float32_add(
            twice_product(qy, qz), twice_product(qx, qw)), sy), 0.0,
        float32_multiply(float32_add(
            twice_product(qx, qz), twice_product(qy, qw)), sz),
        float32_multiply(float32_subtract(
            twice_product(qy, qz), twice_product(qx, qw)), sz),
        float32_multiply(float32_subtract(
            float32_subtract(one, twice_product(qx, qx)),
            twice_product(qy, qy)), sz), 0.0,
        tx, ty, tz, 1.0,
    ]


def write_glb(output, mesh_indices, node_fields=None, include_volume=True,
              thickness_factor=1.0, fixture_positions=None):
    selected_positions = positions if fixture_positions is None else fixture_positions
    binary = bytearray()
    for values in selected_positions:
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


def write_component_fixture(output, parts):
    """Write thick-glass triangles split across independently transformed primitives."""
    binary = bytearray()
    for values in positions:
        binary.extend(struct.pack("<3f", *values))
    normal_offset = len(binary)
    for values in normals:
        binary.extend(struct.pack("<3f", *values))
    uv_offset = len(binary)
    for values in uvs:
        binary.extend(struct.pack("<2f", *values))

    buffer_views = [
        {"buffer": 0, "byteOffset": 0, "byteLength": 96, "target": 34962},
        {"buffer": 0, "byteOffset": normal_offset, "byteLength": 96, "target": 34962},
        {"buffer": 0, "byteOffset": uv_offset, "byteLength": 64, "target": 34962},
    ]
    accessors = [
        {"bufferView": 0, "componentType": 5126, "count": 8, "type": "VEC3",
         "min": [-0.5, -0.5, -0.5], "max": [0.5, 0.5, 0.5]},
        {"bufferView": 1, "componentType": 5126, "count": 8, "type": "VEC3"},
        {"bufferView": 2, "componentType": 5126, "count": 8, "type": "VEC2"},
    ]
    meshes = []
    nodes = []
    scene_nodes = []
    for part_index, part in enumerate(parts):
        while len(binary) % 4:
            binary.append(0)
        index_offset = len(binary)
        for value in part["indices"]:
            binary.extend(struct.pack("<H", value))
        view_index = len(buffer_views)
        accessor_index = len(accessors)
        buffer_views.append({
            "buffer": 0, "byteOffset": index_offset,
            "byteLength": len(part["indices"]) * 2, "target": 34963,
        })
        accessors.append({
            "bufferView": view_index, "componentType": 5123,
            "count": len(part["indices"]), "type": "SCALAR",
        })
        meshes.append({"name": part["name"], "primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
            "indices": accessor_index, "material": 0, "mode": 4,
        }]})
        node = {"name": part["name"], "mesh": part_index}
        node.update(part.get("node", {}))
        if "parent" in part:
            parent_index = len(nodes)
            child_index = parent_index + 1
            parent = {"name": part["name"] + "Parent", "children": [child_index]}
            parent.update(part["parent"])
            nodes.extend((parent, node))
            scene_nodes.append(parent_index)
        else:
            scene_nodes.append(len(nodes))
            nodes.append(node)
    while len(binary) % 4:
        binary.append(0)

    document = {
        "asset": {"version": "2.0", "generator": "Horde RT dielectric component fixtures"},
        "extensionsUsed": [
            "KHR_materials_transmission", "KHR_materials_volume", "KHR_materials_ior"],
        "extensionsRequired": [
            "KHR_materials_transmission", "KHR_materials_volume", "KHR_materials_ior"],
        "buffers": [{"byteLength": len(binary)}],
        "bufferViews": buffer_views,
        "accessors": accessors,
        "materials": [{
            "name": "GenericClosedGlass", "doubleSided": True,
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.82, 0.94, 1.0, 1.0],
                "metallicFactor": 0.0, "roughnessFactor": 0.12,
            },
            "extensions": {
                "KHR_materials_transmission": {"transmissionFactor": 0.94},
                "KHR_materials_volume": {
                    "thicknessFactor": 1.0, "attenuationDistance": 2.4,
                    "attenuationColor": [0.72, 0.90, 1.0],
                },
                "KHR_materials_ior": {"ior": 1.52},
            },
        }],
        "meshes": meshes,
        "nodes": nodes,
        "scenes": [{"nodes": scene_nodes}],
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
        "maxVertices": 32, "maxIndices": 96, "maxPrimitives": 4,
        "maxMaterials": 1, "maxTextureLayersPerKind": 1,
    },
    "lods": [{"name": "lod0", "maxTriangles": 32}],
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
write_component_fixture(
    TEST_OUTPUT / "split-shell-dielectric-lod0.runtime.glb", [
        {"name": "SplitShellA", "indices": indices[:18]},
        {"name": "SplitShellB", "indices": indices[18:],
         "node": {"translation": [0.00000004, 0, 0]}},
    ])
write_component_fixture(
    TEST_OUTPUT / "disconnected-shells-dielectric-lod0.runtime.glb", [
        {"name": "NearbyShellA", "indices": indices,
         "node": {"scale": [0.001, 0.001, 0.001]}},
        {"name": "NearbyShellB", "indices": indices,
         "node": {"scale": [0.001, 0.001, 0.001],
                  "translation": [0.00105, 0, 0]}},
    ])
write_component_fixture(
    TEST_OUTPUT / "mixed-orientation-shells-dielectric-lod0.runtime.glb", [
        {"name": "LargeOutwardShell", "indices": indices,
         "node": {"scale": [2, 2, 2]}},
        {"name": "SmallInwardShell", "indices": inward_indices,
         "node": {"scale": [0.5, 0.5, 0.5], "translation": [4, 0, 0]}},
    ])
split_flipped = list(indices)
split_flipped[1], split_flipped[2] = split_flipped[2], split_flipped[1]
write_component_fixture(
    TEST_OUTPUT / "split-flipped-face-dielectric-lod0.runtime.glb", [
        {"name": "SplitFlippedA", "indices": split_flipped[:18]},
        {"name": "SplitFlippedB", "indices": split_flipped[18:]},
    ])
min_clamp_reference = {
    "name": "MinClampReference", "indices": indices,
    "node": {"scale": [0.1, 0.1, 0.1], "translation": [-1, -1, -1]},
}
min_clamp_seam_a = {
    "name": "MinClampSeamA", "indices": indices[:18],
    "node": {"scale": [0.1, 0.1, 0.1],
             "translation": [0.0000049, 0.0000049, 0.0000049]},
}
min_clamp_seam_b = {
    "name": "MinClampSeamB", "indices": indices[18:],
    "node": {"scale": [0.1, 0.1, 0.1],
             "translation": [0.0000051, 0.0000051, 0.0000051]},
}
write_component_fixture(
    TEST_OUTPUT / "non-unit-min-seam-dielectric-lod0.runtime.glb", [
        min_clamp_reference, min_clamp_seam_a, min_clamp_seam_b,
    ])
write_component_fixture(
    TEST_OUTPUT / "non-unit-min-seam-reordered-dielectric-lod0.runtime.glb", [
        min_clamp_seam_b, min_clamp_reference, min_clamp_seam_a,
    ])
write_component_fixture(
    TEST_OUTPUT / "non-unit-min-scale-seam-dielectric-lod0.runtime.glb", [
        min_clamp_reference,
        {"name": "MinScaleSeamA", "indices": indices[:18],
         "node": {"scale": [0.1, 0.1, 0.1]}},
        {"name": "MinScaleSeamB", "indices": indices[18:],
         "node": {"scale": [0.1, 0.1, 0.1],
                  "translation": [0.000004, 0, 0]}},
    ])
write_component_fixture(
    TEST_OUTPUT / "non-unit-min-diagonal-dielectric-lod0.runtime.glb", [
        min_clamp_reference,
        {
            "name": "MinClampDiagonalA", "indices": indices[:18],
            "node": {"scale": [0.1, 0.1, 0.1],
                     "translation": [-0.0000049, -0.0000049, -0.0000049]},
        },
        {
            "name": "MinClampDiagonalB", "indices": indices[18:],
            "node": {"scale": [0.1, 0.1, 0.1],
                     "translation": [0.0000049, 0.0000049, 0.0000049]},
        },
    ])
write_component_fixture(
    TEST_OUTPUT / "non-unit-max-disconnected-dielectric-lod0.runtime.glb", [
        {"name": "MaxClampPaneA", "indices": indices},
        {"name": "MaxClampPaneB", "indices": indices,
         "node": {"translation": [1.000005, 0, 0]}},
    ])
write_component_fixture(
    TEST_OUTPUT / "non-unit-max-scale-seam-dielectric-lod0.runtime.glb", [
        {"name": "MaxScaleReference", "indices": indices,
         "node": {"translation": [-1, 0, 0]}},
        {"name": "MaxScaleSeamA", "indices": indices[:18],
         "node": {"translation": [1, 0, 0]}},
        {"name": "MaxScaleSeamB", "indices": indices[18:],
         "node": {"translation": [1.0000015, 0, 0]}},
    ])
write_component_fixture(
    TEST_OUTPUT / "mixed-orientation-shells-reordered-dielectric-lod0.runtime.glb", [
        {"name": "SmallInwardShell", "indices": inward_indices,
         "node": {"scale": [0.5, 0.5, 0.5], "translation": [4, 0, 0]}},
        {"name": "LargeOutwardShell", "indices": indices,
         "node": {"scale": [2, 2, 2]}},
    ])

# The two halves below have bit-identical local transforms in cgltf's float32
# arithmetic. One route uses TRS, the other supplies the resulting matrix. A
# similarly paired parent transform exercises cgltf's complete world-composition
# loop while the large legal scale makes even a one-ULP regrouping visible at
# the documented 10 um weld ceiling.
large_trs = {
    "translation": [-17.0, 9.0, 31.0],
    "rotation": [
        0.034700105941401764, 0.08930405965449124,
        -0.8398654691199716, 0.534272104228522,
    ],
    "scale": [1772.607412245919, 80123.46646814236, 304.36350542842496],
}
large_parent_trs = {
    "translation": [2048.0, -1024.0, 512.0],
    "rotation": [0.18257418583505536, -0.3651483716701107,
                 0.5477225575051661, 0.7302967433402214],
    "scale": [1.25, 0.75, 2.0],
}
write_component_fixture(
    TEST_OUTPUT / "large-trs-matrix-seam-dielectric-lod0.runtime.glb", [
        {"name": "LargeTrsSeamA", "indices": indices[:18],
         "node": large_trs, "parent": large_parent_trs},
        {"name": "LargeMatrixSeamB", "indices": indices[18:],
         "node": {"matrix": cgltf_local_trs_matrix(
             large_trs["translation"], large_trs["rotation"], large_trs["scale"])},
         "parent": {"matrix": cgltf_local_trs_matrix(
             large_parent_trs["translation"], large_parent_trs["rotation"],
             large_parent_trs["scale"])}},
    ])

for label, invalid_value in (
        ("nan", math.nan), ("positive-infinity", math.inf),
        ("negative-infinity", -math.inf)):
    invalid_positions = list(positions)
    invalid_positions[0] = (invalid_value, positions[0][1], positions[0][2])
    write_glb(TEST_OUTPUT / f"position-{label}-dielectric-lod0.runtime.glb",
              indices, fixture_positions=invalid_positions)

# Float POSITION data cannot represent the mathematical boundary itself at this
# magnitude. These GLBs therefore use the nearest float32 immediately inside,
# the first float32 crossing, and a clearly outside value on both signs. The
# companion exact-double case file below covers nextafter/at/nextafter itself.
weld_safe_limit = (2**63) * 1.0e-7
first_crossing = float32(weld_safe_limit)
crossing_bits = struct.unpack("<I", struct.pack("<f", first_crossing))[0]
cell_boundary_centres = {
    "upper-inside": struct.unpack("<f", struct.pack("<I", crossing_bits - 3))[0],
    "upper-at": first_crossing,
    "upper-outside": struct.unpack("<f", struct.pack("<I", crossing_bits + 3))[0],
    "lower-inside": -struct.unpack("<f", struct.pack("<I", crossing_bits - 3))[0],
    "lower-at": -first_crossing,
    "lower-outside": -struct.unpack("<f", struct.pack("<I", crossing_bits + 3))[0],
}
for label, centre in cell_boundary_centres.items():
    write_glb(TEST_OUTPUT / f"weld-domain-{label}-dielectric-lod0.runtime.glb",
              indices, {"scale": [131072.0, 131072.0, 131072.0],
                        "translation": [centre, 0.0, 0.0]})

TEST_OUTPUT.mkdir(parents=True, exist_ok=True)
(TEST_OUTPUT / "asset.manifest.json").write_text(
    json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
for manifest_name, metres_per_unit in (
        ("centimetre-units.manifest.json", 0.01),
        ("ten-metre-units.manifest.json", 10.0),
        ("zero-units.manifest.json", 0.0),
        ("underflow-float-units.manifest.json", 1.0e-50),
        ("nonfinite-float-units.manifest.json", 3.5e38),
        ("nan-units.manifest.json", math.nan)):
    non_unit_manifest = dict(manifest)
    non_unit_manifest["metresPerUnit"] = metres_per_unit
    (TEST_OUTPUT / manifest_name).write_text(
        json.dumps(non_unit_manifest, indent=2) + "\n", encoding="utf-8")

(TEST_OUTPUT / "weld-cell-domain-cases.txt").write_text(
    "# name scaled-coordinate accepted\n"
    "upper-inside 0x1.fffffffffffffp+62 true\n"
    "upper-at 0x1.0000000000000p+63 false\n"
    "upper-outside 0x1.0000000000001p+63 false\n"
    "lower-inside -0x1.fffffffffffffp+62 true\n"
    "lower-at -0x1.0000000000000p+63 false\n"
    "lower-outside -0x1.0000000000001p+63 false\n",
    encoding="utf-8")
