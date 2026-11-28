"""Build Bellwether's market, parcel edges and village greenery in Blender 5.2."""
import json
import math
from pathlib import Path
import random
import runpy
import sys

import bpy
from mathutils import Vector


RECIPE = Path(__file__).resolve()
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
mesh_tools = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
paths = runpy.run_path(str(RECIPE.with_name("author-bellwether-paths.py")))
waymarker = runpy.run_path(str(RECIPE.with_name("author-bellwether-square-waymarker.py")))
paving = runpy.run_path(str(RECIPE.with_name("author-bellwether-well-apron.py")))
SOURCE = Path("assets/models/world/source/bellwether-working-square-v1")
MAP = Path("docs/references/horde-1.7/d37f1638/docs/design/world/connected")
PARENTS = {
    "bellwether-tavern-v1.glb": "b6e87a722f0697b049b5cb42c9226cfb7284470e02044f1765f1cfae56e61fa0",
    "asset.manifest.json": "a17a5049cda4593a896b7fb920e60183ed6796847782dd375c0fa3d71c8aeb81",
    "garden-crops.glb": "ea55200b061adcab443b7859cd7874f3552c94b908bef808509f371b8287cf8e",
    "garden-crops.manifest.json": "4312b85a97302f39935d10d3b2cac1f4ceba7412344ddca82293610cfa6bb9b3",
}
BOX_FACES = [(3,2,1,0), (4,5,6,7), (0,1,5,4), (1,2,6,5), (2,3,7,6), (3,0,4,7)]


def export(parts, name, destination):
    if len(parts) == 1:
        bpy.ops.object.select_all(action="DESELECT")
        parts[0].select_set(True)
    else:
        mesh_tools["join_parts"](parts, name)
    bpy.ops.export_scene.gltf(filepath=str(destination), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True,
                             export_animations=False)


def author(layout, ground):
    rng = random.Random(184608)
    stone = town["material"]("HordeTownStone", (.28, .25, .20), .93)
    oak = town["material"]("HordeTavernTimber", (.27, .16, .085), .88)
    clay = town["material"]("HordeTownRoofClay", (.34, .22, .12), .93)
    dark = town["material"]("HordeHearthChar", (.065, .045, .036), .88)
    leaf = town["material"]("HordeGardenLeaf", (.16, .25, .087), .9)
    solid, planting, sprouts = [], [], []

    def box(name, x, z, width, depth, bottom, top, finish, chamfer=0.):
        solid.append(town["block"](name, x-width/2, x+width/2, z-depth/2, z+depth/2,
                                  bottom, top, finish, chamfer))

    def timber(name, start, end, first, last, finish=oak, sides=7):
        a, b = Vector(start), Vector(end)
        axis = (b-a).normalized()
        right = axis.cross(Vector((0, 1, 0)))
        if right.length < .1:
            right = axis.cross(Vector((0, 0, 1)))
        right.normalize()
        up = axis.cross(right)
        points = [tuple(centre + radius*(math.cos(phase)*right + math.sin(phase)*up))
                  for centre, radius in ((a, first), (b, last))
                  for phase in (index*math.tau/sides for index in range(sides))]
        faces = [tuple(reversed(range(sides))), tuple(range(sides, 2*sides))]
        for i in range(sides):
            j = (i+1) % sides
            faces.append((i, j, sides+j, sides+i))
        solid.append(mesh_tools["mesh"](name, points, faces, finish))

    # Build a broad base course and low irregular coping. Each stone
    # follows the surveyed slope across its own footprint.
    for name, start, end in layout["walls"]:
        ax, az = start
        length = math.dist(start, end)
        ux, uz = (end[0]-ax)/length, (end[1]-az)/length
        for course, step, height in ((0., .98, .36), (.33, .91, .26)):
            count = math.ceil(length/step)
            joints = [0.] + [length*i/count+rng.uniform(-.09, .09) for i in range(1, count)] + [length]
            for i, (lo, hi) in enumerate(zip(joints, joints[1:])):
                half = rng.uniform(.23, .28) if course == 0 else rng.uniform(.20, .25)
                outline = [(ax + u*ux - s*uz, az + u*uz + s*ux)
                           for u, s in ((lo+.008, -half), (lo, half), (hi, half*.90), (hi-.008, -half*.92))]
                points = [(x, ground(x, z)+raise_y, z) for raise_y in
                          (course-.10 if course == 0 else course-.014, course+height+rng.uniform(-.033, .033))
                          for x, z in outline]
                solid.append(mesh_tools["mesh"](f"{name} stone {course:.2f}/{i+1}", points, BOX_FACES, stone))

    def crate(name, x, z, width, depth, base):
        box(name, x, z, width, depth, base, base+.40, oak)

    def produce_tray(x, y, z):
        box("Vegetable tray base", x,z,.60,.34,y,y+.034,oak)
        for side in (-1,1):
            box("Vegetable tray long slat", x,z+side*.157,.61,.034,y+.030,y+.095,oak)
            box("Vegetable tray end slat", x+side*.284,z,.036,.30,y+.030,y+.092,oak)
        for row in (-1,1):
            for i, dx in enumerate((-.173, -.006, .162)):
                px, pz = x+dx+row*.012,z+row*.063
                top = y+.155-(i%2)*.018
                bulb = ((-.012,.030),(.032,.067),(.071,.061),(top-y-.035,.025))
                waymarker["turned"](solid,"Root vegetable",(px,y+.035,pz),bulb,6,clay if (i+row)%2 else stone)
                sprouts.append((px,top,pz))

    for index, (name, x, z, width) in enumerate(layout["marketStalls"]):
        y = ground(x, z)
        front, back = z-.72, z+.74
        for side in (-1, 1):
            post_x = x+side*(width*.49-.05)
            for post_z, head in ((front, 2.28), (back, 1.94)):
                box(f"{name} upright", post_x, post_z, .12, .12, ground(post_x, post_z)-.08, y+head+.04, oak, .02)
            timber(f"{name} sloped brace", (post_x,y+2.27,front-.22), (post_x,y+1.93,back+.22), .056, .047)
        # Close the sloped cover so its shadow is stable from all rays.
        ring = [(x-width*.58,front-.22), (x-width*.58,back+.22),
                (x+width*.58,back+.22), (x+width*.58,front-.22)]
        cover = [(vx,y+level+(.34 if vz < z else 0.),vz) for level in (1.92,2.00) for vx,vz in ring]
        solid.append(mesh_tools["mesh"](f"{name} weather cover", cover, BOX_FACES, clay))
        for side in (-1,1):
            for end in (-1,1):
                box(f"{name} counter leg", x+side*width*.35, z-.24+end*.26, .11, .11, y-.12, y+.83, oak)
        for plank in range(4):
            box(f"{name} counter plank", x, z-.53+plank*.18, width*.83, .176, y+.78, y+.87, oak)
        if index == 0:
            for offset in (-.61, .29):
                produce_tray(x+offset,y+.868,z-.31)
        else:
            for offset, size in ((-.65,.83),(-.25,.68),(.32,1.0)):
                profile = ((0.,.087),(.033,.133),(.18,.169),(.29,.136),(.34,.085),(.365,.093),(.366,.071),(.22,.066))
                waymarker["turned"](solid,"Travelling supplier's earthenware jar",(x+offset,y+.866,z-.30),
                    tuple((h*size,r*size) for h,r in profile),10,clay)
        crate(f"{name} packed stock", x+.40, z+.47, .70, .51, ground(x+.40,z+.47))

    # The post sits in the gap between the pitches. The bracket carries the
    # oil cup on the apron side, with the wick and its light clear of timber.
    x, z = layout["marketLamp"]
    y = ground(x, z)
    arm_z = z-.45
    box("Market lamp footing", x,z,.32,.29,y-.08,y+.18,stone)
    box("Market lamp post", x,z,.144,.144,y+.10,y+2.14,oak)
    timber("Market lamp arm",(x,y+1.93,z),(x,y+1.93,arm_z),.041,.037,sides=4)
    timber("Market lamp brace",(x,y+1.57,z-.032),(x,y+1.90,arm_z+.05),.031,.027,sides=4)
    waymarker["turned"](solid,"Market clay oil cup",(x,y+1.96,arm_z),
        ((-.05,.065),(-.016,.105),(.050,.094),(.027,.066)),8,clay)
    waymarker["turned"](solid,"Market lamp wick",(x,y+2.012,arm_z),((-.019,.020),(.037,.015)),6,oak)
    flame, light = (x,y+2.074,arm_z), (x,y+2.21,arm_z-.025)

    for name, x, z in layout["loadingPlaces"]:
        y = ground(x,z)
        crate(f"{name} parcel", x,z,.78,.60,y-.045)
        crate(f"{name} smaller parcel", x+.065,z-.07,.56,.46,y+.36)
        crate(f"{name} loose parcel", x+.88,z+.27,.61,.55,ground(x+.88,z+.27)-.04)
        for row in range(3):
            for n in range(3-row):
                px, pz = x-.78+(n+row*.5)*.23, z+.52
                py = ground(px,pz)+.115+row*.17
                timber(f"{name} split fuel", (px,py,pz-.34), (px+.028,py+.018,pz+.31+rng.uniform(-.055,.10)),
                       .124,.105,sides=5)

    x, z = layout["storeLoadingBench"]
    y = ground(x,z)
    for dx in (-.53,.53):
        for dz in (-.22,.22):
            px,pz=x+dx,z+dz
            box("Store loading bench leg",px,pz,.14,.14,ground(px,pz)-.08,y+.53,oak)
    for dz in (-.20,0.,.20):
        box("Store loading bench plank",x,z+dz,1.43,.192,y+.49,y+.57,oak)
    crate("Store bench parcel",x-.28,z+.02,.48,.37,y+.57)

    for x,z,scale in layout["storeCasks"]:
        y = ground(x,z)+.021
        for dx in (-.17,.17):
            px=x+dx
            box("Cask dunnage",px,z,.11,.64,ground(px,z)-.025,y+.095,oak)
        y += .091
        profile = ((0.,.228),(.07,.254),(.23,.30),(.45,.315),(.62,.290),(.78,.251),(.82,.229))
        waymarker["turned"](solid,"Store staved cask",(x,y,z),
            tuple((h*scale,r*scale) for h,r in profile),12,oak)
        for height, radius in ((.09,.262),(.41,.321),(.75,.265)):
            waymarker["turned"](solid,"Cask iron hoop",(x,y+scale*height,z),
                ((-.021,radius*scale),(.021,radius*scale)),12,dark)

    def spray(name, x, y, z, spread, rise, count):
        for i in range(count):
            yaw = i*2.39996+rng.uniform(-.4,.4)
            reach = spread*rng.uniform(.45,1.03)
            h = rise*rng.uniform(.49,1.02)
            width = reach*rng.uniform(.15,.24)
            dx,dz = math.cos(yaw),math.sin(yaw)
            points = [(x,y,z), (x+dx*reach*.48-dz*width,y+h*.70,z+dz*reach*.48+dx*width),
                      (x+dx*reach,y+h*.88,z+dz*reach),
                      (x+dx*reach*.51+dz*width,y+h*.72,z+dz*reach*.51-dx*width),
                      (x+dx*reach*.49,y+h*.60,z+dz*reach*.49)]
            faces = [(0,1,4),(1,2,4),(2,3,4),(3,0,4),(0,3,2),(0,2,1)]
            planting.append(mesh_tools["mesh"](f"{name} leaf {i+1}", points, faces, leaf))

    for name,x,z,spread,rise in layout["verges"]:
        spray(name,x,ground(x,z)-.035,z,spread,rise,10)

    x,z = layout["commonTree"]
    y = ground(x,z)
    timber("Common pollard trunk", (x,y-.14,z), (x+.15,y+2.37,z-.09), .215,.15, sides=9)
    for index,(dx,dz,ht) in enumerate(((-1.0,.38,3.37), (.99,.24,3.18), (.18,-1.07,3.51), (.16,.72,3.73))):
        tip = (x+dx,y+ht,z+dz)
        timber(f"Pollard branch {index+1}",(x+.07,y+1.99,z),tip,.087,.024)
        spray(f"Pollard canopy {index+1}", *tip, .87, .60, 12)
    for x,y,z in sprouts:
        spray("Market root greens",x,y-.004,z,.095,.083,1)
    return solid, planting, stone, flame, light


def market_pavers(layout, ground, stone):
    rng = random.Random(15351)
    paving_layout = layout["marketPaving"]
    centres = [(x+pitch*i+rng.uniform(-.023,.023), z+rng.uniform(-.023,.023))
               for x,z,count,pitch in paving_layout["rows"] for i in range(count)]
    centres.extend(paving_layout["steps"])
    parts = []
    for i, centre in enumerate(centres):
        slab = {"name": f"Market apron slab {i+1}", "centre": centre,
                "size": (rng.uniform(.62,.68),rng.uniform(.53,.58)),
                "shape": "east-west" if i%4 != 3 else "north-south"}
        part, _ = paving["build_stone"](slab,ground,stone,.017,.008,.008,.075)
        parts.append(part)
    return parts


def main(root, parent, output):
    if output.exists() or any(town["sha"](parent/name) != digest for name,digest in PARENTS.items()):
        raise ValueError("Use the exact parent town and crop package and a fresh output folder.")
    terrain = root / town["TERRAIN"]
    if town["sha"](terrain) != town["TERRAIN_HASH"]:
        raise ValueError("Wrong registered terrain.")
    layout_path = root / SOURCE / "layout.json"
    layout = json.loads(layout_path.read_text(encoding="utf-8"))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    solid, planting, stone, flame, light = author(layout, ground)
    previous_paths = json.loads((root/"assets/models/world/source/bellwether-paths-v1/layout.json").read_bytes())["routes"]
    routes = [layout["marketSpur"],layout["storePath"]]
    footprints = paths["fit_routes"](terrain,previous_paths+routes)[-len(routes):]
    road = [paths["make_part"](route["name"],footprint,stone) for route,footprint in zip(routes,footprints)]
    road.extend(market_pavers(layout,ground,stone))
    output.mkdir(parents=True)
    blend = output/"working-square.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend), compress=True)
    solid_glb, planting_glb, road_glb = (output/name for name in ("square-structures.glb","square-planting.glb","square-spur.glb"))
    export(solid, "BellwetherWorkingSquare", solid_glb)
    export(planting, "BellwetherVergePlanting", planting_glb)
    export(road, "BellwetherMarketSpur", road_glb)
    paths["write_planar_tangents"](road_glb)
    interim, runtime = output/"town-structures.glb", output/"bellwether-tavern-v1.glb"
    solid_count, _, _ = town["join_town"](parent/runtime.name, solid_glb, interim)
    ground_count, vertex_count, primitive_count = town["join_town"](interim,road_glb,runtime,node_name="BellwetherWalkable")
    foliage = output/"garden-crops.glb"
    leaf_count, leaf_vertices, leaf_primitives = town["join_town"](parent/foliage.name, planting_glb, foliage,
                                                                  node_name="BellwetherGardenCrops")
    if not 0 < solid_count <= 4700 or not 0 < ground_count <= 720 or not 0 < leaf_count <= 850:
        raise ValueError((solid_count, ground_count, leaf_count))
    if (primitive_count,leaf_primitives) != (6,1) or leaf_vertices > 8192:
        raise ValueError("Unexpected town primitive or foliage vertex counts.")
    for name, vertices, delta in (("asset.manifest.json",vertex_count,solid_count+ground_count),
                                   ("garden-crops.manifest.json",leaf_vertices,leaf_count)):
        manifest = json.loads((parent/name).read_bytes())
        manifest["budgets"]["maxVertices"] = vertices
        manifest["budgets"]["maxIndices"] += delta*3
        manifest["lods"][0]["maxTriangles"] += delta
        town["save_json"](output/name,manifest)
    header = output/"geometry.generated.h"
    flame_coordinates = ", ".join(f"{value:.6f}f" for value in flame)
    light_coordinates = ", ".join(f"{value:.6f}f" for value in light)
    store_stations = layout["storePath"]["stations"]
    store_coordinates = ",\n    ".join(f"{{{{{x:.6f}f, {z:.6f}f}}}}" for x,z,_ in store_stations)
    header.write_text("#pragma once\n#include <array>\n#include <cstddef>\n\n"
                      "// Generated by tools/author-bellwether-working-square.py.\n"
                      "namespace horde::gameplay::bellwether {\n"
                      f"inline constexpr std::size_t kWorkingSquareSolidTriangles={solid_count};\n"
                      f"inline constexpr std::size_t kWorkingSquareGroundTriangles={ground_count};\n"
                      f"inline constexpr std::size_t kWorkingSquareLeafTriangles={leaf_count};\n"
                      f"inline constexpr std::array<float,3> kWorkingSquareLampFlame{{{{{flame_coordinates}}}}};\n"
                      f"inline constexpr std::array<float,3> kWorkingSquareLampLight{{{{{light_coordinates}}}}};\n"
                      f"inline constexpr std::array<std::array<float,2>,{len(store_stations)}> kWorkingSquareStorePath{{{{\n"
                      f"    {store_coordinates}\n}}}};\n"
                      "}\n", encoding="utf-8", newline="\n")
    town["save_json"](output/"authoring-receipt.json", dict(parents=PARENTS, terrainSha256=town["sha"](terrain),
        mapSha256={name:town["sha"](root/MAP/name) for name in ("02a-bellwether-terrain-1-8.webp","02b-bellwether-use-1-8.webp")},
        layoutSha256=town["sha"](layout_path), recipeSha256=town["sha"](RECIPE), blender=bpy.app.version_string,
        addedSolidTriangles=solid_count, addedGroundTriangles=ground_count, addedLeafTriangles=leaf_count,
        marketLampFlame=flame, marketLampLight=light,
        files={p.name:town["sha"](p) for p in (blend,header,runtime,foliage,output/"asset.manifest.json",output/"garden-crops.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
