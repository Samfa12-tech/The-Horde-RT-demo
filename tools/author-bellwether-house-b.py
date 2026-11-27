"""Open the surveyed B04 east entrance and furnish a private walkable store room.

Blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-house-b.py -- ROOT PARENT_TOWN NEW_OUTPUT
"""
import json
from pathlib import Path
import runpy
import sys

import bpy
import numpy as np


RECIPE = Path(__file__).resolve()
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
mesh_tools = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
paths = runpy.run_path(str(RECIPE.with_name("author-bellwether-paths.py")))
MASTER = Path("assets/models/world/source/bellwether-shell-v4/bellwether-shell-v4.blend")
MAP = Path("docs/references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json")
LANES = Path("assets/models/world/source/bellwether-paths-v1/layout.json")
PARENT = "71874a3d4b203f84045c1bd807eb3232ba7ba7246493735f12475591056cec58"
INPUTS = {
    MASTER: "a93583bbdcf4e84f90a58dd347ac025a3331b37671cefe47a89db59181802581",
    MASTER.with_name("authoring-receipt.json"): "a93c72970511f41a783c12fcce986b78d9ddf4fc8f38d1a036edf767cad07b85",
    MAP: "0a4da29bf8d988145c0f7b3f1c7afaf415a5da9b14c34bbd53bd1e00e3a86e59",
    town["TERRAIN"]: town["TERRAIN_HASH"],
}
REPLACED = {
    "B04_stone_foundation": False,
    "B04_closed_walls": True,
    "B04_door_post_-1": True,
    "B04_door_post_1": True,
    "B04_door_lintel": True,
    "B04_front_step": True,
}


def open_shell(master, original, edited):
    """Remove the wall block, old north doorway and floor-contact masonry."""
    doc, blocks = town["read_glb"](original)
    node, = (node for node in doc["nodes"] if node["name"] == "BellwetherSolid")
    stone, = (p for p in doc["meshes"][node["mesh"]]["primitives"]
              if doc["materials"][p["material"]]["name"] == "HordeTownStone")
    positions = town["read_accessor"](doc, blocks, stone["attributes"]["POSITION"])
    indices = town["read_accessor"](doc, blocks, stone["indices"]).reshape(-1, 3)
    removed = np.zeros(len(indices), dtype=bool)
    with bpy.data.libraries.load(str(master), link=False) as (source, selected):
        if not set(REPLACED) <= set(source.objects):
            raise ValueError("Missing B04 masonry source objects")
        selected.objects = list(REPLACED)
    ownership = {}
    for obj in selected.objects:
        points = {(v.co.x, v.co.z, -v.co.y) for v in obj.data.vertices}
        corners = np.fromiter((tuple(p) in points for p in positions), dtype=bool, count=len(positions))
        owned = corners[indices].all(axis=1)
        if np.count_nonzero(owned) != 12:
            raise ValueError(f"Expected twelve exact B04 shell faces for {obj.name}")
        if not REPLACED[obj.name]:
            height = max(p[1] for p in points)
            east = max(p[0] for p in points)
            owned &= ((positions[indices, 1] == height).all(axis=1) |
                      (positions[indices, 0] == east).all(axis=1))
            if np.count_nonzero(owned) != 4:
                raise ValueError("The B04 foundation cap and east face must each have two triangles")
        if (removed & owned).any():
            raise ValueError(f"Two B04 source blocks claim the same faces: {obj.name}")
        removed |= owned
        ownership[obj.name] = int(np.count_nonzero(owned))
    town["write_accessor"](doc, blocks, stone["indices"], indices[~removed].reshape(-1))
    town["save_glb"](edited, doc, blocks)
    bpy.data.orphans_purge(do_recursive=True)
    return ownership


def author_room(site, entrance, ground):
    stone = town["material"]("HordeTownStone", (.35, .30, .22), .92)
    oak = town["material"]("HordeTavernTimber", (.29, .18, .094), .84)
    clay = town["material"]("HordeTownRoofClay", (.24, .125, .072), .9)
    iron = town["material"]("HordeHearthChar", (.06, .05, .04), .81)
    ex, ez = entrance
    west, north, south = ex - site["width"], ez - site["depth"]/2, ez + site["depth"]/2
    floor = site["actualFloorY"]
    top = floor + site["wallHeight"] - .02
    thickness = .39
    left, right = ez - 1.10, ez + 1.10
    solids = []

    def box(name, x0, x1, z0, z1, y0, y1, finish=stone, bevel=0.):
        solids.append(town["block"](name, x0, x1, z0, z1, y0, y1, finish, bevel))

    box("West rubble wall", west, west+thickness, north, south, floor, top)
    box("North rubble wall", west+thickness, ex-thickness, north, north+thickness, floor, top)
    box("South rubble wall", west+thickness, ex-thickness, south-thickness, south, floor, top)
    box("East wall north of entrance", ex-thickness, ex, north, left, floor, top)
    box("East wall south of entrance", ex-thickness, ex, right, south, floor, top)
    box("East stone lintel and upper wall", ex-thickness, ex, left, right, floor+2.63, top)
    # The two foundation faces finish against the stair cheeks. A continuous
    # foundation face across the door catches the leading edge of the player
    # capsule while its centre is still on the ramp below the threshold.
    for name, z0, z1 in (("North", north, left), ("South", right, south)):
        base = site["foundationBottomY"]
        points = [(ex,base,z0),(ex,floor,z0),(ex,floor,z1),(ex,base,z1)]
        solids.append(mesh_tools["mesh"](f"{name} east foundation", points, [(0,1,2,3)], stone))

    for z, name in ((left-.115, "North"), (right+.115, "South")):
        box(f"{name} oak jamb", ex-.16, ex+.13, z-.106, z+.106, floor, floor+2.64, oak, .018)
    box("Oak door head", ex-.16, ex+.13, left-.22, right+.22, floor+2.51, floor+2.71, oak, .018)
    # The open leaf rests against the north jamb, leaving an honest invitation.
    for i in range(6):
        x = ex-.11-i*.321
        box("Door leaf board", x-.306, x, left-.019, left+.053, floor+.036, floor+2.49, oak, .009)
    for y in (.47, 1.94):
        box("Door strap iron", ex-1.84, ex-.12, left+.046, left+.074, floor+y, floor+y+.067, iron, .008)

    lip_x = ex + 2.79
    across = [left+i*(right-left)/8. for i in range(9)]
    mid_x = ex + 1.67
    mid_y = max(ground(mid_x,z) for z in across) + .08
    lip_heights = [ground(lip_x,z)+paths["SURFACE_LIFT"] for z in across]
    grade = [(ex,[floor]*len(across)),(ex+.70,[floor]*len(across)),
             (mid_x,[mid_y]*len(across)),(lip_x,lip_heights)]
    landing = [(x,y,z) for x,heights in grade for y,z in zip(heights,across,strict=True)]
    width = len(across)
    steps = [(i,i+1,i+1+width,i+width) for row in range(len(grade)-1)
             for i in range(row*width,(row+1)*width-1)]
    # The graded landing clears the terrain rib through the centre of the
    # ramp. Stone cheeks run from its outer edges down to surveyed terrain.
    for side, z, edge_index in (("North", left, 0), ("South", right, -1)):
        points = [(x,h,z) for x,heights in grade for h in (heights[edge_index],ground(x,z)-.12)]
        faces = [(i,i+2,i+3,i+1) for i in range(0,len(points)-2,2)]
        if side == "South":
            faces = [face[::-1] for face in faces]
        solids.append(mesh_tools["mesh"](f"{side} ramp cheek", points, faces, stone))
    for side, z, edge_index in (("North", left-.115, 0), ("South", right+.115, -1)):
        start = (ex+.32,floor,z)
        lip_y = lip_heights[edge_index]
        end = (lip_x-.11,lip_y+.11*(mid_y-lip_y)/(lip_x-mid_x),z)
        for x,y,_ in (start,end):
            box(f"{side} approach post", x-.079, x+.079, z-.081, z+.081, ground(x,z)-.09, y+.95, oak, .016)
        edge = [start] + [(x,heights[edge_index],z) for x,heights in grade[1:-1]] + [end]
        solids.extend(mesh_tools["rail"](f"{side} approach", edge, oak, omit_posts=[(x,z) for x,_,z in edge]))

    # Closed timber bins give the west end an occupied edge. Keep the main
    # aisle and an open turning area clear of both the door and furniture.
    bin_front = west + 3.25
    for z0,z1 in ((north+1.65,ez-.7),(ez+.7,south-1.65)):
        box("Raised grain-bin front", bin_front-.16, bin_front, z0, z1, floor, floor+1.09, oak, .021)
        box("Grain-bin cap", west+.55, bin_front+.08, z0-.05, z1+.05, floor+1.07, floor+1.16, oak, .015)
        box("Grain-bin north return", west+.55, bin_front, z0, z0+.13, floor, floor+1.07, oak)
        box("Grain-bin south return", west+.55, bin_front, z1-.13, z1, floor, floor+1.07, oak)
        for z in (z0+.15,z1-.15):
            box("Bin face upright", bin_front-.01, bin_front+.095, z-.062, z+.062, floor+.02, floor+1.14, oak)

    bench_x, bench_z = ex-3.55, ez+2.65
    for x in (bench_x-.83,bench_x+.83):
        for z in (bench_z-.30,bench_z+.30):
            box("Sorting table leg", x-.058, x+.058, z-.06,z+.06, floor,floor+.84,oak,.012)
        box("Sorting table trestle", x-.067,x+.067,bench_z-.37,bench_z+.37,floor+.25,floor+.37,oak)
    for i in range(4):
        z = bench_z-.42+i*.207
        box("Sorting table board", bench_x-1.06,bench_x+1.06,z,z+.203,floor+.81,floor+.889,oak,.008)
    box("Shallow sorting tray",bench_x-.72,bench_x+.30,bench_z-.30,bench_z+.24,floor+.89,floor+.96,oak,.011)
    for x in (bench_x-.70,bench_x+.27):
        box("Sorting tray end",x-.024,x+.024,bench_z-.30,bench_z+.24,floor+.95,floor+1.07,oak)
    for z in (bench_z-.28,bench_z+.22):
        box("Sorting tray rim",bench_x-.70,bench_x+.27,z-.024,z+.024,floor+.95,floor+1.07,oak)

    # A low oil cup sits on an iron plate beside the sorting tray. The bowl
    # connects the visible wick to the shared world-fire flame and light.
    lamp_x, lamp_z = bench_x+.61, bench_z-.21
    lamp_y = floor+.91
    box("Oil lamp spill plate",lamp_x-.16,lamp_x+.16,lamp_z-.155,lamp_z+.155,
        floor+.885,lamp_y,iron,.011)
    rings = ((.059,0.),(.094,.026),(.115,.080),(.104,.104),(.079,.101),(.053,.047))
    count = 12
    angle = 2*np.pi*np.arange(count)/count
    points = [(lamp_x+r*np.cos(a),lamp_y+h,lamp_z+r*np.sin(a)) for r,h in rings for a in angle]
    faces = [(r*count+i,(r+1)*count+i,(r+1)*count+(i+1)%count,r*count+(i+1)%count)
             for r in range(len(rings)-1) for i in range(count)]
    faces += [tuple(range(count)),tuple(reversed(range((len(rings)-1)*count,len(rings)*count)))]
    solids.append(mesh_tools["mesh"]("Earthenware oil cup",points,faces,clay))
    box("Oil lamp wick holder",lamp_x-.022,lamp_x+.022,lamp_z-.019,lamp_z+.019,
        lamp_y+.035,lamp_y+.110,iron)
    box("Oil lamp wick",lamp_x-.006,lamp_x+.006,lamp_z-.006,lamp_z+.006,
        lamp_y+.104,lamp_y+.142,iron)
    lamp_flame = (lamp_x,lamp_y+.142,lamp_z)
    lamp_light = (lamp_x,lamp_y+.208,lamp_z)

    shelf_x = ex - 5.05
    shelf_z = north + .65
    for x in (shelf_x-1.62,shelf_x+1.62):
        for z in (shelf_z,shelf_z+.43):
            box("Rack upright",x-.055,x+.055,z-.058,z+.058,floor,floor+1.88,oak,.009)
    for deck in (.35,.98,1.60):
        box("Rack shelf",shelf_x-1.69,shelf_x+1.69,shelf_z-.084,shelf_z+.52,floor+deck-.075,floor+deck,oak,.013)

    # Unglazed jars use the same earthenware finish as the town prop kit.
    for index, (x, y, radius) in enumerate(((shelf_x-.85,floor+.35,.22),
                                             (shelf_x-.25,floor+.35,.19),
                                             (shelf_x+.76,floor+.98,.16))):
        count = 10
        rings = ((.74,0.),(1.,radius*.62),(.86,radius*1.47),(.65,radius*2.02),
                 (.51,radius*2.02),(.50,radius*1.71))
        angle = 2*np.pi*np.arange(count)/count
        points = [(x+radius*r*np.cos(a),y+h,shelf_z+.24+radius*r*np.sin(a)) for r,h in rings for a in angle]
        faces = [(r*count+i,(r+1)*count+i,(r+1)*count+(i+1)%count,r*count+(i+1)%count)
                 for r in range(len(rings)-1) for i in range(count)]
        faces += [tuple(range(count)),tuple(reversed(range((len(rings)-1)*count,len(rings)*count)))]
        solids.append(mesh_tools["mesh"](f"Storage jar {index+1}", points, faces, clay))

    slab = [(west,floor,north),(west,floor,south),(ex,floor,south),(ex,floor,north)]
    walkable = [mesh_tools["mesh"]("Dressed stone floor", slab, [(0,1,2,3)], stone),
                mesh_tools["mesh"]("East entry ramp", landing, steps, stone)]
    return solids, walkable, (lip_x, lip_heights[len(across)//2], ez), stone, lamp_flame, lamp_light


def export_layer(parts, name, file):
    mesh_tools["join_parts"](parts, name)
    bpy.ops.export_scene.gltf(filepath=str(file), export_format="GLB", use_selection=True,
                              export_apply=True, export_normals=True, export_tangents=True, export_animations=False)


def main(root, original, output):
    sha = town["sha"]
    if output.exists() or sha(original) != PARENT or any(sha(root/path) != digest for path,digest in INPUTS.items()):
        raise ValueError("Use the exact B04 map, terrain, shell master, town parent and a fresh output directory")
    surveyed = json.loads((root / MASTER.with_name("authoring-receipt.json")).read_bytes())
    site, = (site for site in surveyed["buildings"] if site["id"] == "B04")
    mapped, = (site for site in json.loads((root/MAP).read_bytes())["buildings"] if site["id"] == "B04")
    ex = site["engineX"] + mapped["door_x"] - mapped["x"]
    ez = site["engineZ"] - mapped["door_y"] + mapped["y"]
    if abs(ex-(site["engineX"]+site["width"]/2)) > .001 or ez != site["engineZ"]:
        raise ValueError("B04 layout no longer has the centred east entrance")

    output.mkdir(parents=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    survey = root / town["TERRAIN"]
    ground = town["terrain_probe"](survey)
    shell = output / "house-b-open-shell.glb"
    ownership = open_shell(root/MASTER, original, shell)
    solids, walkable, lip, stone, flame, light = author_room(site, (ex,ez), ground)
    branch = {"name": "Lower road to House B", "stations": [
        [48.3,92.2,.85], [45.2,92.35,.88], [43.7,93.85,.86], [42.8,96.0,.84], [lip[0]-.06,ez,.82],
    ]}
    lanes = json.loads((root/LANES).read_bytes())["routes"]
    fitted = paths["fit_routes"](survey, lanes+[branch])[-1]
    walkable.append(paths["make_part"](branch["name"],fitted,stone))
    source = output / "house-b.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(source),compress=True)

    solid_patch, floor_patch = output / "house-b-solids.glb", output / "house-b-ground.glb"
    export_layer(solids, "BellwetherHouseBStructure", solid_patch)
    export_layer(walkable, "BellwetherHouseBWalkable", floor_patch)
    paths["write_planar_tangents"](floor_patch)
    with_solids = output / "house-b-town-solids.glb"
    solid_triangles, _, _ = town["join_town"](shell,solid_patch,with_solids)
    runtime = output / original.name
    ground_triangles, vertices, primitives = town["join_town"](
        with_solids,floor_patch,runtime,node_name="BellwetherWalkable")
    removed = sum(ownership.values())
    if not 0 < solid_triangles-removed <= 1800 or not 4 < ground_triangles <= 180 or primitives != 6:
        raise ValueError(f"B04 exceeds its bounded patch: {solid_triangles}, {ground_triangles}, {primitives}")
    manifest = json.loads((original.parent/"asset.manifest.json").read_bytes())
    triangles = manifest["lods"][0]["maxTriangles"] + solid_triangles - removed + ground_triangles
    manifest["budgets"].update(maxVertices=vertices,maxIndices=triangles*3)
    manifest["lods"][0]["maxTriangles"] = triangles
    town["save_json"](output/"asset.manifest.json",manifest)

    stations = [(x,z) for x,z,_ in branch["stations"]] + [(ex-.6,ez), (ex-4.3,ez), (ex-7.0,ez+.75)]
    station_lines = ", ".join(f"{{{{{x:.3f}f,{z:.3f}f}}}}" for x,z in stations)
    flame_values = ", ".join(f"{value:.7f}f" for value in flame)
    light_values = ", ".join(f"{value:.7f}f" for value in light)
    header = output / "geometry.generated.h"
    header.write_text(
        "#pragma once\n#include <array>\n#include <cstddef>\n\n"
        "// Generated by tools/author-bellwether-house-b.py.\n"
        "namespace horde::gameplay::bellwether {\n"
        f"inline constexpr std::size_t kHouseBSolidTriangles={solid_triangles-removed};\n"
        f"inline constexpr std::size_t kHouseBGroundTriangles={ground_triangles};\n"
        f"inline constexpr float kHouseBFloorY={site['actualFloorY']:.8f}f;\n"
        f"inline constexpr std::array<float,2> kHouseBEastDoorXZ{{{{{ex:.3f}f,{ez:.3f}f}}}};\n"
        f"inline constexpr std::array<float,3> kHouseBLampFlame{{{{{flame_values}}}}};\n"
        f"inline constexpr std::array<float,3> kHouseBLampLight{{{{{light_values}}}}};\n"
        f"inline constexpr std::array<std::array<float,2>,{len(stations)}> kHouseBStations"
        f"{{{{{station_lines}}}}};\n"
        "} // namespace horde::gameplay::bellwether\n",
        encoding="utf-8",newline="\n")
    town["save_json"](output/"authoring-receipt.json",dict(
        building="B04",eastEntranceXZ=(ex,ez),floorY=site["actualFloorY"],rampToe=lip,lampFlame=flame,lampLight=light,
        removedSolidTriangles=ownership,addedSolidTriangles=solid_triangles,addedGroundTriangles=ground_triangles,
        runtimeTownSha256=sha(runtime),originalTownSha256=PARENT,blender=bpy.app.version_string,
        townVertices=vertices,townTriangles=triangles,recipeSha256=sha(RECIPE),
        sources={path.as_posix():digest for path,digest in INPUTS.items()},lanesSha256=sha(root/LANES),
        files={path.name:sha(path) for path in (source,solid_patch,floor_patch,runtime,header,output/"asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
