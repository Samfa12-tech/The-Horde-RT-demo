"""Author the Bellwether service-yard basket and its movable folded linen.

Blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-privy-rag.py -- ROOT PREVIOUS_TOWN_GLB NEW_OUTPUT
"""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy


HERE = Path(__file__).resolve()
work = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
privy = runpy.run_path(str(HERE.with_name("author-bellwether-privy.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PARENT = "0cf8cf09bb73164b7ecb254fda5c4bc0c9bbbc6ecc72e74a2f9629b21561d1a8"
BASKET_XZ = (75.82, 113.09)
SHELF_XZ = (76.207, 111.394)


def create_basket(ground):
    x, z = BASKET_XZ
    floor = ground(x,z)
    timber = work["material"]("HordeTavernTimber", (.24,.14,.075), .87)
    iron = work["material"]("HordeHearthChar", (.073,.055,.04), .80)
    objects = []

    def block(label, x0, x1, z0, z1, y0, y1, finish, foot=False):
        if foot:
            y0 = min(y0, *(ground(x+a,z+b)-floor-.03 for a in (x0,x1) for b in (z0,z1)))
        objects.append(work["block"](label, x+x0,x+x1,z+z0,z+z1,floor+y0,floor+y1,finish))

    def rings(label, profile, finish, caps=False):
        sides = 12
        points = [(x+radius*math.cos(.18+math.tau*i/sides), floor+height,
                   z+.91*radius*math.sin(.18+math.tau*i/sides))
                  for height,radius in profile for i in range(sides)]
        faces = []
        for ring in range(len(profile)-1):
            for i in range(sides):
                a, b = ring*sides+i, ring*sides+(i+1)%sides
                faces.append((a,a+sides,b+sides,b))
        if caps:
            faces.extend((tuple(range(sides)),tuple(reversed(range(len(points)-sides,len(points))))))
        objects.append(crossing["mesh"](label, points, faces, finish))

    for i, dx in enumerate((-.123,.123)):
        for j, dz in enumerate((-.111,.111)):
            block(f"Basket stand leg {i}/{j}", dx-.034,dx+.034,dz-.032,dz+.032,-.09,.753,timber,True)
    block("Basket stand board", -.178,.178,-.157,.157,.709,.774,timber)
    for j, dz in enumerate((-.126,.126)):
        block(f"Basket stand brace {j}", -.13,.13,dz-.022,dz+.022,.255,.307,timber)
    rings("Shallow woven clean-rag basket", ((.773,.122),(.906,.187),(.928,.184),
          (.928,.163),(.861,.104)), timber,True)
    rings("Lower basket binding", ((.804,.143),(.822,.154)), iron)
    rings("Upper basket binding", ((.887,.183),(.905,.191)), iron)
    return objects, (x,floor+.865,z)


def create_linen():
    linen = work["material"]("HordePrivyLinen", (.82,.74,.58), .94)
    points, faces = [], []
    # Two folds and an uneven hem give the cloth a silhouette from above.
    sections = [(-.062,.071,.026),(-.050,.081,.053),(-.025,.079,.042),
                (.008,.076,.063),(.033,.080,.051),(.059,.069,.027)]
    for x, width, height in sections:
        points.extend(((x,.004,-width+.008),(x,.004,width),
                       (x,height,width),(x,height+.006,-.009),(x,height-.008,-width+.008)))
    for i in range(len(sections)-1):
        for a in range(5):
            b=(a+1)%5
            faces.append((5*i+a,5*(i+1)+a,5*(i+1)+b,5*i+b))
    faces.extend((tuple(range(5)),tuple(reversed(range(len(points)-5,len(points))))))
    return crossing["mesh"]("Folded linen", points, faces, linen)


def export(parts, name, path):
    crossing["join_parts"](parts,name)
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)


def main(root, previous, output):
    terrain = root/work["TERRAIN"]
    if output.exists() or work["sha"](previous)!=PARENT or work["sha"](terrain)!=work["TERRAIN_HASH"]:
        raise ValueError("Choose a new output with the exact closed-privy town and admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    basket, basket_mark = create_basket(ground)
    shelf_mark = (SHELF_XZ[0],ground(*privy["CENTRE"])+1.021,SHELF_XZ[1])
    linen = create_linen()
    output.mkdir(parents=True)
    blend = output/"bellwether-privy-rag.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend), compress=True)
    rag = output/"privy-rag.glb"
    export([linen], "BellwetherCleanRag", rag)
    stand = output/"rag-basket.glb"
    export(basket, "BellwetherRagBasket", stand)
    town = output/"bellwether-tavern-v1.glb"
    solid_triangles, vertices, primitives = work["join_town"](previous, stand, town)
    parent, _ = work["read_glb"](previous)
    indices = sum(parent["accessors"][p["indices"]]["count"]
                  for mesh in parent["meshes"] for p in mesh["primitives"]) + 3*solid_triangles
    rag_doc, _ = work["read_glb"](rag)
    rag_mesh, = rag_doc["meshes"]
    rp, = rag_mesh["primitives"]
    rag_indices = rag_doc["accessors"][rp["indices"]]["count"]
    rag_vertices = rag_doc["accessors"][rp["attributes"]["POSITION"]]["count"]
    if not 0 < solid_triangles <= 360 or primitives!=5 or vertices>23000:
        raise ValueError(f"Rag basket exceeds town budget: {solid_triangles} new triangles, {vertices} vertices")
    if not 0 < rag_indices <= 480 or rag_vertices>480:
        raise ValueError(f"Folded rag exceeds its budget: {rag_indices//3} triangles, {rag_vertices} vertices")

    manifest = json.loads((root/TOWN/"asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices,maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices//3
    work["save_json"](output/"asset.manifest.json", manifest)
    manifest["asset"] = "bellwether-privy-rag-v1"
    manifest["budgets"].update(maxVertices=rag_vertices,maxIndices=rag_indices,maxPrimitives=1,maxMaterials=1)
    manifest["lods"][0]["maxTriangles"] = rag_indices//3
    manifest["licenceStatus"] = "Original Horde-owned linen geometry and untextured PBR finish."
    work["save_json"](output/"privy-rag.manifest.json", manifest)

    def mark(name, coords):
        values = ", ".join(f"{value:.6f}f" for value in coords)
        return f"inline constexpr std::array<float, 3> kPrivyRag{name}{{{{{values}}}}};"

    header = ["#pragma once", "#include <array>", "#include <cstddef>", "",
              "// Generated by tools/author-bellwether-privy-rag.py.",
              "namespace horde::gameplay::bellwether {",
              f"inline constexpr std::size_t kPrivyRagBasketSolidTriangles={solid_triangles};",
              f"inline constexpr std::size_t kPrivyRagTriangles={rag_indices//3};",
              mark("Basket", basket_mark), mark("Shelf", shelf_mark),
              "} // namespace horde::gameplay::bellwether", ""]
    (output/"geometry.generated.h").write_text("\n".join(header), encoding="utf-8", newline="\n")
    names = (blend.name, stand.name, rag.name, town.name, "asset.manifest.json",
             "privy-rag.manifest.json", "geometry.generated.h")
    work["save_json"](output/"authoring-receipt.json", dict(
        basket=basket_mark, shelf=shelf_mark, originalTownSha256=PARENT, terrainSha256=work["TERRAIN_HASH"],
        runtimeTownSha256=work["sha"](town), addedSolidTriangles=solid_triangles, townVertices=vertices,
        townTriangles=indices//3, ragTriangles=rag_indices//3, ragVertices=rag_vertices,
        blender=bpy.app.version_string, recipeSha256=work["sha"](HERE),
        files={name: work["sha"](output/name) for name in names}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--")+1:]
    if len(arguments)!=3:
        raise ValueError("Expected repository ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
