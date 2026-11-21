"""Author housing and a raised sluice around the existing Bellwether wheel.

Blender --background --factory-startup --python-exit-code 1
  --python tools/author-bellwether-mill-works.py -- ROOT WHEEL_GLB ORIGINAL_TAVERN_GLB NEW_OUTPUT
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree


SOURCE = Path("assets/models/world/source/bellwether-waterwheel-v1")
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
TERRAIN = Path("assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb")
CHEST = Path("assets/models/props/runtime/gothic-chest-base/gothic-chest-base-lod0.runtime.glb")
BASE_TAVERN = "71739dbab1045a219e04ff6fd9da9a7dfb881fa04aba77055e73490ccfd3077d"
TERRAIN_HASH = "f031a850a7e8e661cebe64dadd221e514cf466ac80b9e9ab757b393fecf65a2d"
WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}
TYPES = {5123: "<u2", 5125: "<u4", 5126: "<f4"}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_glb(path):
    data = path.read_bytes()
    magic, version, length, json_size, kind = struct.unpack_from("<4sIII4s", data)
    end = 20 + json_size
    size, binary = struct.unpack_from("<I4s", data, end)
    if (magic, version, length, kind, binary, size) != (b"glTF", 2, len(data), b"JSON", b"BIN\0", len(data)-end-8):
        raise ValueError(f"Expected one GLB 2.0 mesh container: {path}")
    doc = json.loads(data[20:end])
    payload = data[end+8:]
    views = [payload[v.get("byteOffset", 0):v.get("byteOffset", 0)+v["byteLength"]] for v in doc["bufferViews"]]
    return doc, views


def save_glb(path, doc, views):
    data = bytearray()
    for view, block in zip(doc["bufferViews"], views, strict=True):
        view["byteOffset"], view["byteLength"] = len(data), len(block)
        data.extend(block)
        data.extend(b"\0" * (-len(data) % 4))
    doc["buffers"] = [{"byteLength": len(data)}]
    metadata = json.dumps(doc, separators=(",", ":")).encode()
    metadata += b" " * (-len(metadata) % 4)
    path.write_bytes(struct.pack("<4sIII4s", b"glTF", 2, 28+len(metadata)+len(data), len(metadata), b"JSON")
                     + metadata + struct.pack("<I4s", len(data), b"BIN\0") + data)


def read_accessor(doc, blocks, index):
    record = doc["accessors"][index]
    view = doc["bufferViews"][record["bufferView"]]
    if "byteStride" in view or record.get("byteOffset", 0):
        raise ValueError("Expected one tightly packed accessor per view")
    return np.frombuffer(blocks[record["bufferView"]], TYPES[record["componentType"]]).reshape(
        record["count"], WIDTHS[record["type"]])


def write_accessor(doc, blocks, index, values):
    record = doc["accessors"][index]
    record["count"] = len(values)
    if "min" in record:
        record["min"] = values.min(axis=0).tolist()
        record["max"] = values.max(axis=0).tolist()
    blocks[record["bufferView"]] = values.tobytes()


def wheel_with_admitted_finishes(wheel_path, provider_path, output):
    wheel, blocks = read_glb(wheel_path)
    chest, chest_blocks = read_glb(provider_path)
    if [m["name"] for m in wheel["materials"]] != ["BellwetherWheelOak", "BellwetherWheelWroughtIron"]:
        raise ValueError("Unexpected authored wheel finishes")
    if [m["name"] for m in chest["materials"]] != ["ChestWood", "BlackIron"]:
        raise ValueError("Unexpected admitted opaque texture providers")
    wheel["materials"] = chest["materials"]
    wheel["textures"] = chest["textures"]
    wheel["samplers"] = chest["samplers"]
    # The admitted provider holds stripped placeholders. Copy each description
    # and byte sequence exactly; they route to the existing platform arrays.
    images = []
    for source in chest["images"]:
        source_view = source["bufferView"]
        if len(chest_blocks[source_view]) > 1024:
            raise ValueError("A wheel finish would carry a new embedded image")
        images.append(dict(source, bufferView=len(blocks)))
        wheel["bufferViews"].append(dict(chest["bufferViews"][source_view]))
        blocks.append(chest_blocks[source_view])
    wheel["images"] = images
    live = sorted({record["bufferView"] for record in wheel["accessors"]+images})
    remap = {old: new for new, old in enumerate(live)}
    for record in wheel["accessors"]+images:
        record["bufferView"] = remap[record["bufferView"]]
    wheel["bufferViews"] = [wheel["bufferViews"][i] for i in live]
    save_glb(output, wheel, [blocks[i] for i in live])
    return sum(wheel["accessors"][p["indices"]]["count"]//3 for p in wheel["meshes"][0]["primitives"])


def join_town(town_path, supports_path, output):
    town, blocks = read_glb(town_path)
    supports, additions = read_glb(supports_path)
    solid = next(node for node in town["nodes"] if node["name"] == "BellwetherSolid")
    targets = town["meshes"][solid["mesh"]]["primitives"]
    names = {material["name"]: index for index, material in enumerate(town["materials"])}
    support_mesh, = supports["meshes"]
    extra_triangles = 0
    for addition in support_mesh["primitives"]:
        name = supports["materials"][addition["material"]]["name"]
        target, = (p for p in targets if p["material"] == names[name])
        old_vertices = town["accessors"][target["attributes"]["POSITION"]]["count"]
        for semantic, index in target["attributes"].items():
            old = read_accessor(town, blocks, index)
            new = read_accessor(supports, additions, addition["attributes"][semantic])
            write_accessor(town, blocks, index, np.concatenate((old, new)))
        old = read_accessor(town, blocks, target["indices"])
        added = read_accessor(supports, additions, addition["indices"])
        if old_vertices + int(added.max()) > np.iinfo(old.dtype).max:
            raise ValueError("Mill hardware exceeds town index range")
        indices = np.concatenate((old, added.astype(old.dtype) + old_vertices))
        write_accessor(town, blocks, target["indices"], indices)
        extra_triangles += len(added)//3
    save_glb(output, town, blocks)
    return extra_triangles, sum(town["accessors"][p["attributes"]["POSITION"]]["count"]
                                for mesh in town["meshes"] for p in mesh["primitives"]), len(targets)+1


def terrain_probe(path):
    bpy.ops.import_scene.gltf(filepath=str(path))
    points, faces = [], []
    for obj in list(bpy.context.scene.objects):
        if obj.type != "MESH":
            continue
        offset = len(points)
        points.extend(obj.matrix_world @ vert.co for vert in obj.data.vertices)
        faces.extend(tuple(offset+i for i in polygon.vertices) for polygon in obj.data.polygons)
        bpy.data.objects.remove(obj, do_unlink=True)
    surface = BVHTree.FromPolygons(points, faces)

    def ground(x, z):
        hit, _, _, _ = surface.ray_cast(Vector((x,-z,100)), Vector((0,0,-1)), 300)
        if hit is None:
            raise ValueError(f"Mill hardware has no terrain near {x}, {z}")
        return hit.z
    return ground


def material(name, colour, roughness):
    finish = bpy.data.materials.new(name)
    finish.use_nodes = True
    pbr = finish.node_tree.nodes["Principled BSDF"]
    pbr.inputs["Base Color"].default_value = (*colour, 1.)
    pbr.inputs["Roughness"].default_value = roughness
    return finish


def block(name, x0, x1, z0, z1, y0, y1, finish, chamfer=0.):
    if min(x1-x0,z1-z0,y1-y0) <= 0 or 2*chamfer >= min(x1-x0,z1-z0):
        raise ValueError(f"Bad dimensions for {name}")
    ring = [(x0,z0),(x0,z1),(x1,z1),(x1,z0)]
    if chamfer:
        c = chamfer
        ring = [(x0+c,z0),(x0,z0+c),(x0,z1-c),(x0+c,z1),
                (x1-c,z1),(x1,z1-c),(x1,z0+c),(x1-c,z0)]
    n = len(ring)
    points = [(x,-z,y) for y in (y0,y1) for x,z in ring]
    faces = [tuple(reversed(range(n))), tuple(range(n,2*n))]
    for i in range(n):
        j = (i+1) % n
        faces.append((i,j,j+n,i+n))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(points, [], faces)
    mesh.materials.append(finish)
    mesh.update()
    uv = mesh.uv_layers.new(name="Metre coordinates")
    for face in mesh.polygons:
        axis = max(range(3), key=lambda d: abs(face.normal[d]))
        u, v = ((1,2),(0,2),(0,1))[axis]
        for loop in face.loop_indices:
            p = mesh.vertices[mesh.loops[loop].vertex_index].co
            uv.data[loop].uv = (.5*p[u],.5*p[v])
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    return obj


def mill_works(layout, ground):
    x,y,z = layout["wheel"]["axleCentre"]
    half_face = layout["wheel"]["faceWidth"]/2
    radius = layout["wheel"]["radius"]
    masonry = material("HordeTownStone", (.22,.20,.16), 1.)
    oak = material("HordeTavernTimber", (.25,.115,.065), .84)
    parts = []
    footprints = []
    for name, left, right in (("Mill-side",x-1.72,x-1.05),("River-side",x+1.05,x+1.72)):
        margin=.065
        floor=min(ground(xx,zz) for xx in (left,right) for zz in (z-.49,z+.49))-.55
        cap=y-.23
        if floor > cap-.32:
            # The mill-side shoulder embeds part of its bearing; cut the
            # exposed stone footing down into that bank for a continuous seat.
            floor=cap-.46
        split=cap-.23
        parts.extend((
            block(name+" pier footing",left,right,z-.51,z+.51,floor,split,masonry,.055),
            block(name+" dressed bearing cap",left+margin,right-margin,z-.43,z+.43,split,cap,masonry,.046),
            block(name+" oak journal bed",left+.13,right-.13,z-.29,z+.29,cap,y-.17,oak,.026),
        ))
        for offset in (-.232,.232):
            parts.append(block(name+" journal cheek",left+.15,right-.15,z+offset-.057,z+offset+.057,
                               y-.17,y+.055,oak,.02))
        if not (right < x-half_face-.10 or left > x+half_face+.10):
            raise ValueError("Bearing pier intersects the rotating wheel face")
        footprints.append([left,right,z-.51,z+.51,round(floor,3),round(cap,3)])

    gate_z=z-radius-1.11
    head_end=layout["headrace"][-1]
    previous=layout["headrace"][-2]
    t=(gate_z-previous[2])/(head_end[2]-previous[2])
    gate_x, water_y = [a+(b-a)*t for a,b in zip(previous,head_end)][:2]
    half_race=layout["channelWidth"]/2
    gate_bottom=water_y+.24
    frame_top=water_y+2.08
    for sign in (-1,1):
        post=gate_x+sign*(half_race+.15)
        sill=min(ground(post+dx,gate_z+dz) for dx in (-.16,.16) for dz in (-.22,.22))-.33
        parts.append(block("Sluice shoe",post-.22,post+.22,gate_z-.26,gate_z+.26,sill,water_y+.40,masonry,.035))
        inside, outside = sorted((post-sign*.23,post+sign*.10))
        parts.append(block("Sluice guide",inside,outside,gate_z-.10,gate_z+.10,
                           water_y+.31,frame_top,oak,.018))
    parts.append(block("Sluice top beam",gate_x-half_race-.32,gate_x+half_race+.32,
                       gate_z-.14,gate_z+.14,frame_top-.19,frame_top,oak,.027))
    for plank in range(7):
        start=gate_x-1.11+.32*plank
        parts.append(block(f"Raised gate board {plank+1}",start,start+.315,gate_z-.063,gate_z+.063,
                           gate_bottom,gate_bottom+.89,oak,.008))
    for level in (.13,.73):
        parts.append(block("Gate cross brace",gate_x-1.11,gate_x+1.125,gate_z-.13,gate_z-.065,
                           gate_bottom+level,gate_bottom+level+.105,oak,.009))
    parts.append(block("Sluice lifting rack",gate_x-.071,gate_x+.071,gate_z-.074,gate_z+.073,
                       gate_bottom+.79,frame_top+.22,oak,.009))
    if gate_z+.26 >= z-radius or not 0.23 < gate_bottom-water_y < 0.25:
        raise ValueError("Sluice must clear the wheel and leave the race open")
    return parts, {"bearingFootprints":footprints,
                   "sluiceCentreXZ":[round(gate_x,3),round(gate_z,3)],
                   "sluiceClearanceOverWater":round(gate_bottom-water_y,3)}


def save_json(path,value):
    path.write_text(json.dumps(value,indent=2)+"\n",encoding="utf-8",newline="\n")


def main(root, wheel, original, output):
    if output.exists():
        raise ValueError("Choose a new output directory")
    layout = json.loads((root / SOURCE / "layout.json").read_bytes())
    wheel_pin = json.loads((root / SOURCE / "authoring-receipt.json").read_bytes())["glbSha256"]
    if sha(wheel) != wheel_pin or sha(original) != BASE_TAVERN or sha(root / TERRAIN) != TERRAIN_HASH:
        raise ValueError("Use the exact staged wheel source, original town GLB and admitted wheel-bay terrain")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = terrain_probe(root / TERRAIN)
    bpy.data.orphans_purge(do_recursive=True)
    parts, fitting = mill_works(layout, ground)
    output.mkdir(parents=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / "millworks.blend"), compress=True)
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    combined = bpy.context.object
    combined.name = "BellwetherMillworks"
    combined.modifiers.new("Export triangles", "TRIANGULATE")
    fittings = output / "millworks.glb"
    bpy.ops.export_scene.gltf(filepath=str(fittings), export_format="GLB", use_selection=True,
                              export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output / original.name
    added, vertices, primitives = join_town(original, fittings, town)
    wheel_target = output / "mill-waterwheel.glb"
    wheel_triangles = wheel_with_admitted_finishes(wheel, root / CHEST, wheel_target)
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    if vertices > 10_000 or added > 800 or wheel_triangles > 2200:
        raise ValueError("Mill works exceed the local geometry budgets")
    manifest["budgets"]["maxVertices"] = 10_000
    manifest["budgets"]["maxIndices"] = (3856+added)*3
    manifest["lods"] = [{"name":"lod0","maxTriangles":3856+added}]
    save_json(output / "asset.manifest.json",manifest)
    manifest.update(asset="bellwether-waterwheel",licenceStatus="Original Horde geometry with admitted ChestWood and BlackIron finishes",
                    budgets=dict(maxVertices=4800,maxIndices=wheel_triangles*3,maxPrimitives=2,maxMaterials=2,maxTextureLayersPerKind=2),
                    lods=[{"name":"lod0","maxTriangles":wheel_triangles}])
    save_json(output / "mill-waterwheel.manifest.json",manifest)
    (output / "geometry.generated.h").write_text(
        "#pragma once\n#include <cstddef>\n\n"
        "// Generated by tools/author-bellwether-mill-works.py.\n"
        "namespace horde::scene {\n"
        f"inline constexpr std::size_t kBellwetherMillWorksSolidTriangles={added};\n"
        "} // namespace horde::scene\n",
        encoding="utf-8",newline="\n")
    files = ("asset.manifest.json",town.name,wheel_target.name,"mill-waterwheel.manifest.json",
             "millworks.blend","millworks.glb","geometry.generated.h")
    save_json(output / "authoring-receipt.json", dict(originalTownSha256=sha(original), wheelSourceSha256=sha(wheel),
        terrainSha256=TERRAIN_HASH, finishProviderSha256=sha(root / CHEST), authoringRecipeSha256=sha(Path(__file__)),
        solidTrianglesAdded=added, townVertices=vertices,
        townPrimitives=primitives, wheelTriangles=wheel_triangles, fit=fitting,
        outputs={name:dict(bytes=(output/name).stat().st_size,sha256=sha(output/name)) for name in files}))
    print(f"Millworks adds {added} solid triangles. Waterwheel is {wheel_triangles} triangles.")


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
