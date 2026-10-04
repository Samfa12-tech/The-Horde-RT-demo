"""Export the exact Form-approved core through ordinary opaque static glTF PBR.

Blender --background --threads 2 --python-exit-code 1 --python this_file --
    --review-root <private/revision-2>
No review source is mutated; no lights, cameras or inspection context is exported.
"""
import argparse
import hashlib
import json
import math
import struct
import sys
import zlib
from pathlib import Path

import bmesh
import bpy

parser = argparse.ArgumentParser()
parser.add_argument("--review-root", type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
review = args.review_root.resolve()
repo = Path(__file__).resolve().parent.parent
source_dir = repo / "assets/models/world/source/collapsed-entry"
runtime_dir = repo / "assets/models/world/runtime/collapsed-entry"
texture_dir = repo / "assets/textures/props/source/collapsed-entry"
source_dir.mkdir(parents=True, exist_ok=True)
runtime_dir.mkdir(parents=True, exist_ok=True)
approval = json.loads((repo / "docs/design/1.6.2-collapse/milestone-reviews.json").read_text(encoding="utf-8-sig"))
if approval["form"]["status"] != "approved":
    raise RuntimeError("Explicit owner Form approval is required")
blend = review / "final.blend"
approved_blend_sha = "f03e8a175b20c60f444a133831586a533d7c1de2a73ceda916af6e521ecd547a"
approved_sheet_sha = "0fa7fca2b8e403b8c8f33b9adc70381a751e8e2c9e397e3dd1efe06547807486"
if hashlib.sha256(blend.read_bytes()).hexdigest() != approved_blend_sha:
    raise RuntimeError("Editable source differs from the exact Form-approved source")
if hashlib.sha256((review / "final-form-contact-sheet.png").read_bytes()).hexdigest() != approved_sheet_sha:
    raise RuntimeError("Form-approved review receipt differs")
inputs = json.loads((texture_dir / "input-receipt.json").read_text())
for item in inputs["maps"]:
    for path, expected in ((texture_dir / item["file"], item["sha256"]),
                           (repo / item["source"], item["sourceSha256"])):
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise RuntimeError("Texture receipt differs: " + item["file"])

bpy.ops.wm.open_mainfile(filepath=str(blend))
excluded = {"Continuous solid floor", "Existing chamber left", "Existing chamber right", "Existing chamber roof"}
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
if len(meshes) != 67 or not excluded.issubset({o.name for o in meshes}):
    raise RuntimeError("Reviewed mesh roster differs")
core = [o for o in meshes if o.name not in excluded]
if sum(len(p.vertices)-2 for o in core for p in o.data.polygons) != 8422:
    raise RuntimeError("Reviewed core triangle count differs")

# New portable export graph describes the same maps, with explicit nonmetallic
# factors and glTF ORM routing. It does not multiply AO into base colour.
group = bpy.data.node_groups.new("glTF Material Output", "ShaderNodeTree")
group.interface.new_socket(name="Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
materials = []
for name, prefix, strength in (("Boulder01Rock", "boulder01", .42), ("MedievalWall02", "medieval-wall02", .34)):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    bsdf.inputs["Metallic"].default_value = 0
    bsdf.inputs["Roughness"].default_value = 1
    bsdf.inputs["IOR"].default_value = 1.5
    bsdf.inputs["Alpha"].default_value = 1
    textures = {}
    for category in ("base-color", "normal", "orm"):
        image = bpy.data.images.load(str(texture_dir / (prefix + "-" + category + ".png")), check_existing=False)
        image.colorspace_settings.name = "sRGB" if category == "base-color" else "Non-Color"
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = image
        textures[category] = texture
    links.new(textures["base-color"].outputs["Color"], bsdf.inputs["Base Color"])
    normal = nodes.new("ShaderNodeNormalMap")
    normal.inputs["Strength"].default_value = strength
    links.new(textures["normal"].outputs["Color"], normal.inputs["Color"])
    links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    separate = nodes.new("ShaderNodeSeparateColor")
    links.new(textures["orm"].outputs["Color"], separate.inputs["Color"])
    links.new(separate.outputs["Green"], bsdf.inputs["Roughness"])
    links.new(separate.outputs["Blue"], bsdf.inputs["Metallic"])
    output = nodes.new("ShaderNodeGroup")
    output.node_tree = group
    links.new(separate.outputs["Red"], output.inputs["Occlusion"])
    materials.append(material)

for o in core:
    if len(o.data.materials) != 1:
        raise RuntimeError("Core mesh does not have one reviewed material: " + o.name)
    family = o.data.materials[0].name
    index = 0 if family == "Boulder01 shared 1K PBR - CC0" else 1 if family == "Existing admitted MedievalWall02" else -1
    if index < 0:
        raise RuntimeError("Inspection or unreviewed material entered core: " + family)
    if o.matrix_world.to_3x3().determinant() <= 0:
        raise RuntimeError("Reflected source transform requires explicit winding repair")
    o.data.materials.clear()
    o.data.materials.append(materials[index])
    bpy.ops.object.select_all(action="DESELECT")
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    if not all(e.is_manifold for e in bm.edges):
        raise RuntimeError("Core export mesh lost manifold topology: " + o.name)
    bm.to_mesh(o.data)
    bm.free()

bpy.ops.object.select_all(action="DESELECT")
for o in core:
    o.select_set(True)
bpy.context.view_layer.objects.active = core[0]
bpy.ops.object.join()
joined = bpy.context.object
joined.name = "CollapsedEntryWorld"
joined.data.name = "Approved collapsed entry - metre world geometry"
material_indices = [0 if joined.data.materials[p.material_index].name == "Boulder01Rock" else 1 for p in joined.data.polygons]
joined.data.materials.clear()
for material in materials:
    joined.data.materials.append(material)
for p, index in zip(joined.data.polygons, material_indices):
    p.material_index = index
if len(joined.data.uv_layers) != 1:
    raise RuntimeError("Export must retain exactly one complete UV0 set")
joined.data.calc_tangents(uvmap=joined.data.uv_layers.active.name)
if any(not all(math.isfinite(x) for x in loop.tangent) or loop.bitangent_sign not in (-1, 1)
       for loop in joined.data.loops):
    raise RuntimeError("Authored tangent is non-finite or has invalid handedness")

source_path = source_dir / "collapsed-entry-lod0.glb"
bpy.ops.export_scene.gltf(filepath=str(source_path), export_format="GLB", use_selection=True,
                          export_yup=True, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_materials="EXPORT", export_image_format="AUTO",
                          export_animations=False, export_skins=False, export_cameras=False,
                          export_lights=False, export_extras=False)

def read_glb(path):
    data = path.read_bytes()
    json_length = struct.unpack_from("<I", data, 12)[0]
    document = json.loads(data[20:20+json_length])
    binary_offset = 20+json_length
    binary_length = struct.unpack_from("<I", data, binary_offset)[0]
    return document, data[binary_offset+8:binary_offset+8+binary_length]

def write_glb(path, document, binary):
    encoded = json.dumps(document, separators=(",", ":")).encode()
    encoded += b" "*((-len(encoded))%4)
    binary += b"\0"*((-len(binary))%4)
    length = 12+8+len(encoded)+8+len(binary)
    path.write_bytes(struct.pack("<III", 0x46546c67,2,length)+struct.pack("<II",len(encoded),0x4e4f534a)+encoded+
                     struct.pack("<II",len(binary),0x004e4942)+binary)

def placeholder(category):
    colour = {"base-color":(255,255,255,255), "normal":(128,128,255,255), "orm":(255,255,0,255)}[category]
    pixels = b"".join(b"\0"+bytes(colour)*4 for _ in range(4))
    def chunk(kind, payload):
        return struct.pack(">I",len(payload))+kind+payload+struct.pack(">I",zlib.crc32(kind+payload)&0xffffffff)
    return b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",4,4,8,6,0,0,0))+chunk(b"IDAT",zlib.compress(pixels,9))+chunk(b"IEND",b"")

document, binary = read_glb(source_path)
if [m["name"] for m in document["materials"]] != ["Boulder01Rock", "MedievalWall02"]:
    raise RuntimeError("Export material ordering differs from atlas admission")
primitives = [p for mesh in document["meshes"] for p in mesh["primitives"]]
if len(primitives) != 2 or sorted(p["material"] for p in primitives) != [0,1]:
    raise RuntimeError("Export requires exactly two material primitives")
for p in primitives:
    if set(p["attributes"]) != {"POSITION","NORMAL","TANGENT","TEXCOORD_0"}:
        raise RuntimeError("Static PBR attribute contract differs")
tangent_corrections = []
authored_binary = bytearray(binary)
for primitive_index,p in enumerate(primitives):
    tangent_accessor = document["accessors"][p["attributes"]["TANGENT"]]
    normal_accessor = document["accessors"][p["attributes"]["NORMAL"]]
    tangent_view = document["bufferViews"][tangent_accessor["bufferView"]]
    normal_view = document["bufferViews"][normal_accessor["bufferView"]]
    for vertex in range(tangent_accessor["count"]):
        offset = tangent_view.get("byteOffset",0)+tangent_accessor.get("byteOffset",0)+vertex*tangent_view.get("byteStride",16)
        tangent = struct.unpack_from("<4f",authored_binary,offset)
        if not all(math.isfinite(v) for v in tangent) or tangent[3] not in (-1,1):
            raise RuntimeError("Non-finite or reflected-invalid exported tangent")
        if sum(v*v for v in tangent[:3]) > 1e-12:
            continue
        # A singular authored UV corner has no Mikk derivative. Author a stable
        # perpendicular unit frame here, preserving its reviewed geometry/UVs.
        # This is export-time authoring, never a loader or object-shader repair.
        normal_offset = normal_view.get("byteOffset",0)+normal_accessor.get("byteOffset",0)+vertex*normal_view.get("byteStride",12)
        n = struct.unpack_from("<3f",authored_binary,normal_offset)
        axis = min(range(3),key=lambda i:abs(n[i]))
        reference = tuple(1.0 if i == axis else 0.0 for i in range(3))
        t = (reference[1]*n[2]-reference[2]*n[1],reference[2]*n[0]-reference[0]*n[2],reference[0]*n[1]-reference[1]*n[0])
        length = math.sqrt(sum(v*v for v in t))
        if not math.isfinite(length) or length < .5:
            raise RuntimeError("Singular normal cannot supply an authored tangent")
        authored = tuple(v/length for v in t)+(tangent[3],)
        struct.pack_into("<4f",authored_binary,offset,*authored)
        tangent_corrections.append({"primitive":primitive_index,"vertex":vertex,"reason":"singular Mikk UV derivative",
                                    "normal":list(n),"authoredUnitTangent":list(authored),"positionsAndUvsUnchanged":True})
if len(tangent_corrections)>8:
    raise RuntimeError("Unexpected broad UV/tangent damage requires new asset investigation")
binary = bytes(authored_binary)
image_roles = {}
for index, material in enumerate(document["materials"]):
    material["pbrMetallicRoughness"]["metallicFactor"] = 0
    material["pbrMetallicRoughness"]["roughnessFactor"] = 1
    material["normalTexture"]["scale"] = (.42,.34)[index]
    material["alphaMode"] = "OPAQUE"
    material["doubleSided"] = False
    if material.get("emissiveTexture") or any(material.get("emissiveFactor",[0,0,0])):
        raise RuntimeError("Unexpected emissive material")
    for category, view in (("base-color",material["pbrMetallicRoughness"]["baseColorTexture"]),
                           ("normal",material["normalTexture"]),
                           ("orm",material["pbrMetallicRoughness"]["metallicRoughnessTexture"]),
                           ("orm",material["occlusionTexture"])):
        image_index = document["textures"][view["index"]]["source"]
        role = (index,category)
        if image_index in image_roles and image_roles[image_index] != role:
            raise RuntimeError("Two different map roles share one image unexpectedly")
        image_roles[image_index] = role
if len(image_roles) != 6 or len(document["images"]) != 6:
    raise RuntimeError("Export must describe exactly six approved maps")

def replace_images(document, old_binary, use_placeholders):
    document = json.loads(json.dumps(document))
    image_views = {image["bufferView"] for image in document["images"]}
    views,remap,new_binary = [],{},bytearray()
    for index, view in enumerate(document["bufferViews"]):
        if index in image_views:
            continue
        new_binary.extend(b"\0"*((-len(new_binary))%4))
        remap[index] = len(views)
        copied = dict(view)
        copied["byteOffset"] = len(new_binary)
        views.append(copied)
        offset = view.get("byteOffset",0)
        new_binary.extend(old_binary[offset:offset+view["byteLength"]])
    for accessor in document["accessors"]:
        accessor["bufferView"] = remap[accessor["bufferView"]]
    for index,image in enumerate(document["images"]):
        material_index,category = image_roles[index]
        record = next(m for m in inputs["maps"] if m["material"] == document["materials"][material_index]["name"] and m["category"] == category)
        encoded = placeholder(category) if use_placeholders else (repo / record["source"]).read_bytes()
        new_binary.extend(b"\0"*((-len(new_binary))%4))
        image["bufferView"] = len(views)
        image["mimeType"] = "image/png" if use_placeholders else "image/jpeg"
        views.append({"buffer":0,"byteOffset":len(new_binary),"byteLength":len(encoded)})
        new_binary.extend(encoded)
    document["bufferViews"] = views
    document["buffers"] = [{"byteLength":len(new_binary)}]
    return document,bytes(new_binary)

# Full source keeps the exact original JPEG payloads. The runtime embeds only
# tiny routing placeholders; actual production pixels come from the shared atlas.
source_document, source_binary = replace_images(document,binary,False)
write_glb(source_path,source_document,source_binary)
runtime_document, runtime_binary = replace_images(document,binary,True)
runtime_path = runtime_dir / "collapsed-entry-lod0.runtime.glb"
write_glb(runtime_path,runtime_document,runtime_binary)
triangles = sum(runtime_document["accessors"][p["indices"]]["count"]//3 for p in primitives)
vertices = sum(runtime_document["accessors"][p["attributes"]["POSITION"]]["count"] for p in primitives)
if triangles != 8422 or vertices > 25266:
    raise RuntimeError("Actual exported geometry exceeds reviewed budget")
report = {"schema":"horde.collapse-export.v1","form":"owner-approved-revision2","runtimeAcceptance":"pending",
          "approvedBlendSha256":approved_blend_sha,"approvedContactSheetSha256":approved_sheet_sha,
          "excludedContextMeshes":sorted(excluded),"coreSourceMeshes":63,"triangles":triangles,"vertices":vertices,
          "indices":triangles*3,"primitives":2,"materials":["Boulder01Rock","MedievalWall02"],
          "worldTransform":"baked, identity node/TLAS placement; glTF metre+Yup maps to Horde",
          "lightingBaked":False,"exportedCamerasOrLights":False,"runtimeEmbeddedImageDimensions":[4,4],
          "source":{"path":source_path.relative_to(repo).as_posix(),"bytes":source_path.stat().st_size,
                    "sha256":hashlib.sha256(source_path.read_bytes()).hexdigest()},
          "runtime":{"path":runtime_path.relative_to(repo).as_posix(),"bytes":runtime_path.stat().st_size,
                     "sha256":hashlib.sha256(runtime_path.read_bytes()).hexdigest()},
          "textureInputReceipt":"assets/textures/props/source/collapsed-entry/input-receipt.json",
          "authoredTangentCorrections":tangent_corrections,
          "acceptanceGaps":["Khronos validation","Real StaticMeshAsset loader validation","Atlas/native integration",
                            "Native RT owner visual/resource/performance/device acceptance"]}
(source_dir / "export-receipt.json").write_text(json.dumps(report,indent=2)+"\n")
print(json.dumps(report))
