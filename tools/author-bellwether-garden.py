"""Author the T02 kitchen plots as terrain-fitted beds and opaque crop geometry.

Blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-garden.py -- ROOT PREVIOUS_TOWN_GLB NEW_OUTPUT
"""
import json
import math
from pathlib import Path
import random
import runpy
import sys

import bmesh
import bpy


HERE = Path(__file__).resolve()
work = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PARENT = "a3c37483acfee86676fffbdcc5d9c5acc4e1d99c6b168b0b025eb6ce7a16e74f"
CENTRE = (35.3, 136.2)
BED_X = (-1.90, 1.92)
BEDS = ((-1.78, -.63), (.69, 1.84))
CORNER_FACES = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2),
                (7, 4, 0, 3), (7, 6, 5, 4)]


def author_plots(ground):
    timber = work["material"]("HordeTavernTimber", (.24, .15, .084), .88)
    soil = work["material"]("HordeTownRoofClay", (.17, .089, .052), .93)
    stone = work["material"]("HordeTownStone", (.24, .22, .17), .91)
    leaf = work["material"]("HordeGardenLeaf", (.15, .26, .087), .82)
    rng = random.Random(180202)
    structure = []
    planting = []
    centre_x, centre_z = CENTRE

    def position(x, h, z):
        world_x, world_z = centre_x+x, centre_z+z
        return world_x, ground(world_x, world_z)+h, world_z

    def fitted_box(name, x0, x1, z0, z1, low, high, finish):
        corners = ((x0,z0), (x1,z0), (x1,z1), (x0,z1))
        points = [position(x, level, z) for level in (low, high) for x, z in corners]
        structure.append(crossing["mesh"](name, points, CORNER_FACES, finish))

    def furrow(name, x0, x1, z, width, phase):
        points, faces = [], []
        for i in range(10):
            x = x0+(x1-x0)*i/9
            offset = math.sin(phase+i*.67)*.021
            for j, (dz, height) in enumerate(((-width,.091), (0.,.153), (width,.091))):
                taper = .025 if i in (0,9) else 0.
                points.append(position(x, height-taper+math.sin(i*1.2+j)*.012, z+dz+offset))
        for i in range(9):
            for j in range(2):
                a = i*3+j
                faces.append((a, a+1,a+4,a+3))
        structure.append(crossing["mesh"](name, points, faces, soil))

    def crop(name, x, z, broad):
        points, faces, uv = [], [], []
        origin_x, origin_y, origin_z = position(x, .125, z)
        angle = rng.uniform(0., math.tau)
        blades = 7 if broad else 5
        for i in range(blades):
            yaw = angle+i*2.39996323
            direction = (math.cos(yaw),math.sin(yaw))
            across = (-direction[1],direction[0])
            reach = rng.uniform(.175,.274) if broad else rng.uniform(.075,.139)
            rise = rng.uniform(.145,.245) if broad else rng.uniform(.254,.415)
            width = rng.uniform(.051,.084) if broad else rng.uniform(.018,.033)
            # The raised rib and tapered edges give the leaf a closed profile.
            sketch = ((0.,0.,0.), (.46,.48,-width*.58), (.64,.76,-width),
                      (.64,.82,0.), (.64,.76,width), (.46,.48,width*.58),
                      (1.,.89 if broad else 1.,0.), (.58,.61,0.))
            start = len(points)
            for distance, elevation, side in sketch:
                dx = direction[0]*reach*distance+across[0]*side
                dz = direction[1]*reach*distance+across[1]*side
                points.append((origin_x+dx, -(origin_z+dz), origin_y+rise*elevation))
                uv.append((.5+side/(2*width), distance))
            top = ((0,1,3),(1,2,3),(2,6,3),(3,6,4),(3,4,5),(3,5,0))
            bottom = ((0,7,1),(1,7,2),(2,7,6),(6,7,4),(4,7,5),(5,7,0))
            faces.extend(tuple(start+j for j in face) for face in (*top,*bottom))
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(points, [], faces)
        mesh.materials.append(leaf)
        normals = bmesh.new()
        normals.from_mesh(mesh)
        bmesh.ops.recalc_face_normals(normals, faces=list(normals.faces))
        normals.to_mesh(mesh)
        normals.free()
        layer = mesh.uv_layers.new(name="Alder leaf coordinates")
        for loop in mesh.loops:
            layer.data[loop.index].uv = uv[loop.vertex_index]
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.collection.objects.link(obj)
        planting.append(obj)

    x0, x1 = BED_X
    for bed, (z0, z1) in enumerate(BEDS, start=1):
        rows = (z0+.265, z1-.265)
        for index, (left, right) in enumerate(((x0,-.14),(-.15,x1)), start=1):
            for side, z in enumerate((z0,z1-.11), start=1):
                fitted_box(f"Bed {bed} side {side} plank {index}", left,right,z,z+.11,-.07,.195,timber)
        for side, x in enumerate((x0,x1-.11), start=1):
            fitted_box(f"Bed {bed} headboard {side}", x,x+.11,z0+.11,z1-.11,-.065,.187,timber)
        for index, (x, z) in enumerate(((x0+.135,z0+.135),(x1-.135,z0+.135),
                                         (x0+.135,z1-.135),(x1-.135,z1-.135)), start=1):
            fitted_box(f"Bed {bed} peg {index}", x-.044,x+.044,z-.046,z+.046,-.075,.254,timber)
        fitted_box(f"Bed {bed} seedbed", x0+.07,x1-.07,z0+.07,z1-.07,-.035,.113,soil)
        for row, z in enumerate(rows, start=1):
            furrow(f"Bed {bed} turned row {row}", x0+.155,x1-.15,z,.173,bed*row*.71)
            # One recently lifted plant leaves a deliberate gap in each bed.
            for plant in range(7):
                if plant == 5 and row == bed:
                    continue
                x = -1.43+plant*.461+rng.uniform(-.054,.054)
                crop(f"Bed {bed} row {row} plant {plant+1}",x,z+rng.uniform(-.043,.043),bed==1)
    # A small dry standing stone at the outside end keeps muddy tools off the rows.
    fitted_box("Tool rest stone", -2.28,-1.96,1.041,1.552,-.085,.215,stone)
    return structure, planting


def export_parts(parts, name, path):
    crossing["join_parts"](parts, name)
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)


def main(root, previous, output):
    terrain = root/work["TERRAIN"]
    if output.exists() or work["sha"](previous) != PARENT or work["sha"](terrain) != work["TERRAIN_HASH"]:
        raise ValueError("Choose a new output with the exact covered-well town and admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    structure, planting = author_plots(ground)
    output.mkdir(parents=True)
    blend = output/"bellwether-garden.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend), compress=True)
    structure_glb, crops = output/"garden-beds.glb", output/"garden-crops.glb"
    export_parts(structure, "BellwetherGardenBeds", structure_glb)
    export_parts(planting, "BellwetherGardenCrops", crops)
    town = output/"bellwether-tavern-v1.glb"
    solid_triangles, vertices, primitives = work["join_town"](previous, structure_glb, town)
    parent, _ = work["read_glb"](previous)
    indices = sum(parent["accessors"][p["indices"]]["count"]
                  for mesh in parent["meshes"] for p in mesh["primitives"]) + 3*solid_triangles
    crop_doc, _ = work["read_glb"](crops)
    crop_mesh, = crop_doc["meshes"]
    crop_primitive, = crop_mesh["primitives"]
    crop_indices = crop_doc["accessors"][crop_primitive["indices"]]["count"]
    crop_vertices = crop_doc["accessors"][crop_primitive["attributes"]["POSITION"]]["count"]
    if not 0 < solid_triangles <= 500 or primitives != 5 or vertices > 19000:
        raise ValueError(f"Garden beds exceed town budget: {solid_triangles} new triangles, {vertices} vertices")
    if not 0 < crop_indices <= 6000 or crop_vertices > 6200:
        raise ValueError(f"Garden crops exceed budget: {crop_indices//3} triangles, {crop_vertices} vertices")

    manifest = json.loads((root/TOWN/"asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices//3
    work["save_json"](output/"asset.manifest.json", manifest)
    manifest["asset"] = "bellwether-garden-crops-v1"
    manifest["budgets"].update(maxVertices=crop_vertices, maxIndices=crop_indices, maxPrimitives=1, maxMaterials=1)
    manifest["lods"][0]["maxTriangles"] = crop_indices//3
    manifest["licenceStatus"] = "Original Horde-owned crop models. The world reuses the admitted ForestAlder material."
    work["save_json"](output/"garden-crops.manifest.json", manifest)

    header = ["#pragma once", "#include <array>", "#include <cstddef>", "",
              "// Generated by tools/author-bellwether-garden.py.",
              "namespace horde::gameplay::bellwether {",
              f"inline constexpr std::size_t kBellwetherGardenSolidTriangles={solid_triangles};",
              f"inline constexpr std::size_t kBellwetherGardenCropTriangles={crop_indices//3};",
              "inline constexpr std::array<float, 2> kBellwetherGardenCentre{{35.3f, 136.2f}};",
              "} // namespace horde::gameplay::bellwether", ""]
    (output/"geometry.generated.h").write_text("\n".join(header), encoding="utf-8", newline="\n")
    names = (blend.name, structure_glb.name, crops.name, town.name, "asset.manifest.json",
             "garden-crops.manifest.json", "geometry.generated.h")
    work["save_json"](output/"authoring-receipt.json", dict(
        mapFeature="T02", centre=CENTRE, plotX=BED_X, plotZ=BEDS, aisleWidth=BEDS[1][0]-BEDS[0][1],
        originalTownSha256=PARENT, terrainSha256=work["TERRAIN_HASH"], runtimeTownSha256=work["sha"](town),
        addedSolidTriangles=solid_triangles, townVertices=vertices, townTriangles=indices//3,
        cropTriangles=crop_indices//3, cropVertices=crop_vertices, cropMaterialFamily="ForestAlder",
        blender=bpy.app.version_string, recipeSha256=work["sha"](HERE),
        files={name: work["sha"](output/name) for name in names}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--")+1:]
    if len(arguments) != 3:
        raise ValueError("Expected repository ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
