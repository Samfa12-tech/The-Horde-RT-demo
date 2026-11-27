"""Derive registered B01 detail candidates from the editable tavern.

blender --background --python-exit-code 1 --python tools/author-bellwether-tavern-lods.py -- ROOT OUTPUT
OUTPUT must be new. The original Blender and runtime files are read-only.
"""

import hashlib
import json
from pathlib import Path
import struct
import sys

import bmesh
import bpy


root_arg, output_arg = sys.argv[sys.argv.index("--") + 1:]
root, output = Path(root_arg).resolve(), Path(output_arg).resolve()
source = root / "assets/models/world/source/bellwether-tavern-v1"
runtime = root / "assets/models/world/runtime/bellwether-tavern-v1"
master = source / "bellwether-tavern-v1.blend"
if output.exists() or output.is_relative_to(runtime) or output.is_relative_to(source):
    raise RuntimeError("Choose a fresh output directory outside the source and runtime folders.")

bpy.ops.wm.open_mainfile(filepath=str(master))
building = [obj for obj in bpy.context.scene.objects
            if obj.name.startswith("B01_") and obj.type == "MESH" and not obj.hide_get()]
leaf = [obj for obj in building if obj.parent and obj.parent.name == "B01_SouthDoorHinge"]
pane = bpy.data.objects["B01_window_pane"]
furniture = [obj for obj in building if obj.name.startswith(("B01_chair_", "B01_table_"))
             and not obj.name.startswith("B01_table_candle_")]
architecture = [obj for obj in building if obj not in furniture and obj not in leaf and obj != pane]
if len(furniture) != 23 or len(leaf) != 22 or pane not in building or any(obj.parent for obj in architecture + furniture):
    raise RuntimeError("The B01 roles changed. Review the authored objects before deriving new candidates.")
timber = bpy.data.materials["HordeTavernTimber"]
if any(list(obj.data.materials) != [timber] for obj in furniture):
    raise RuntimeError("Furniture must retain the existing timber finish.")


def furniture_box(name, parts):
    corners = [obj.matrix_world @ vertex.co for obj in parts for vertex in obj.data.vertices]
    x0, y0, z0 = (min(point[i] for point in corners) for i in range(3))
    x1, y1, z1 = (max(point[i] for point in corners) for i in range(3))
    vertices = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
                (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
    faces = [(3, 2, 1, 0), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(timber)
    mesh.update()
    uv = mesh.uv_layers.new(name="metre_projection")
    for face in mesh.polygons:
        axis = max(range(3), key=lambda i: abs(face.normal[i]))
        u, v = ((1, 2), (0, 2), (0, 1))[axis]
        for loop in face.loop_indices:
            point = mesh.vertices[mesh.loops[loop].vertex_index].co
            uv.data[loop].uv = (point[u] * .5, point[v] * .5)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8", newline="\n")


def export(parts, asset_name, node_name):
    bpy.ops.object.select_all(action="DESELECT")
    copies = []
    for original in parts:
        obj = original.copy()
        obj.data = original.data.copy()
        bpy.context.collection.objects.link(obj)
        obj.select_set(True)
        copies.append(obj)
    bpy.context.view_layer.objects.active = copies[0]
    bpy.ops.object.join()
    merged = bpy.context.object
    merged.name = node_name
    # Bake the existing world registration into the vertices. All tiers use
    # the same identity node, regardless of the first source object's origin.
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    mesh = bmesh.new()
    mesh.from_mesh(merged.data)
    bmesh.ops.triangulate(mesh, faces=list(mesh.faces))
    mesh.to_mesh(merged.data)
    mesh.free()
    path = output / (asset_name + ".glb")
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                              export_apply=True, export_texcoords=True, export_normals=True,
                              export_tangents=True, export_animations=False)
    bytes_ = path.read_bytes()
    size = struct.unpack_from("<I", bytes_, 12)[0]
    model = json.loads(bytes_[20:20 + size])
    if model["nodes"] != [{"mesh": 0, "name": node_name}] or model.get("textures") or model.get("animations"):
        raise RuntimeError(f"Unexpected transform or payload in {path.name}")
    primitives = model["meshes"][0]["primitives"]
    accessor = model["accessors"]
    positions = [accessor[part["attributes"]["POSITION"]] for part in primitives]
    vertex_count = sum(item["count"] for item in positions)
    index_count = sum(accessor[part["indices"]]["count"] for part in primitives)
    materials = [item["name"] for item in model["materials"]]
    minimum = [min(item["min"][i] for item in positions) for i in range(3)]
    maximum = [max(item["max"][i] for item in positions) for i in range(3)]
    manifest = json.loads((runtime / "asset.manifest.json").read_text())
    manifest.update(asset=asset_name, distribution="source-review",
                    budgets=dict(maxVertices=vertex_count, maxIndices=index_count,
                                 maxPrimitives=len(primitives), maxMaterials=len(materials), maxTextureLayersPerKind=1),
                    lods=[dict(name=asset_name.rsplit("-", 1)[1], maxTriangles=index_count // 3)])
    manifest_path = output / (asset_name + ".manifest.json")
    dump(manifest_path, manifest)
    bpy.data.objects.remove(merged, do_unlink=True)
    return dict(mesh=path.name, sha256=sha(path), manifest=manifest_path.name,
                manifestSha256=sha(manifest_path), bounds=[minimum, maximum],
                vertices=vertex_count, triangles=index_count // 3, materials=materials,
                objects=[obj.name for obj in parts])


mid = [furniture_box("mid_" + obj.name, [obj]) for obj in furniture]
far_primary = [obj for obj in furniture
               if obj.name.startswith(("B01_chair_front_leg_", "B01_chair_back_post_",
                                       "B01_chair_back_lower", "B01_chair_back_upper", "B01_table_leg_"))]
far = [furniture_box("far_" + obj.name, [obj]) for obj in far_primary]
for part in ("chair_seat", "table_top"):
    planks = [obj for obj in furniture if obj.name.startswith("B01_" + part + "_")]
    if len(planks) != 3:
        raise RuntimeError(f"Review the {part} boards before merging.")
    far.append(furniture_box("far_B01_" + part, planks))

output.mkdir(parents=True)
shared = export(architecture, "tavern-architecture", "BellwetherTavernArchitecture")
tiers = {name: export(parts, f"tavern-furniture-{name}", "BellwetherTavernFurniture")
         for name, parts in (("lod0", furniture), ("lod1", mid), ("lod2", far))}
reference = tiers["lod0"]
for candidate in tiers.values():
    if candidate["bounds"] != reference["bounds"] or candidate["materials"] != reference["materials"]:
        raise RuntimeError("Furniture bounds or material identity changed between detail levels.")
if not reference["triangles"] > tiers["lod1"]["triangles"] > tiers["lod2"]["triangles"]:
    raise RuntimeError("Furniture detail levels must reduce triangle count.")

inputs = [master, source / "authoring-receipt.json"] + [runtime / name for name in (
    "bellwether-tavern-v1.glb", "asset.manifest.json", "bellwether-window.glb", "window.manifest.json",
    "bellwether-south-door.glb", "south-door.manifest.json")]
dump(output / "authoring-receipt.json", dict(
    building="B01", status="source review", blender=bpy.app.version_string,
    inputSha256={path.relative_to(root).as_posix(): sha(path) for path in inputs},
    shared=shared, furniture=tiers,
    sharedProps=dict(window="bellwether-window.glb", hingedLeaf="bellwether-south-door.glb"),
    coordinates="glTF Y-up, authored world metres, identity node for all four files"))
print("B01 source-review triangles", shared["triangles"], {key: value["triangles"] for key, value in tiers.items()})
