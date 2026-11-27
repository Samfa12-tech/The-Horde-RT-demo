"""Author the square water bench and the working frontage of Bellwether tavern.

Run in Blender 5.2 with -- ROOT PARENT_TOWN_GLB NEW_OUTPUT.
The parent manifest belongs beside the supplied GLB.
"""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve()
town = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
PARENT = "6263a6e735397bdc9fb3ca261c66ba4af8e6a2e69f8c7999a678797058987dc7"
BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2),
       (7, 4, 0, 3), (7, 6, 5, 4)]


def furnishings(ground):
    oak = town["material"]("HordeTavernTimber", (.27, .15, .08), .86)
    clay = town["material"]("HordeTownRoofClay", (.28, .12, .06), .84)
    stone = town["material"]("HordeTownStone", (.31, .28, .22), .9)
    iron = town["material"]("HordeHearthChar", (.06, .05, .04), .8)
    parts = []

    def block(name, x0, x1, z0, z1, y0, y1, finish, bevel=0):
        parts.append(town["block"](name, x0, x1, z0, z1, y0, y1, finish, bevel))

    def mesh(name, points, faces, finish):
        parts.append(geometry["mesh"](name, points, faces, finish))

    def beam(name, start, end, width, depth, finish):
        start, end = Vector(start), Vector(end)
        direction = (end - start).normalized()
        guide = Vector((0, 1, 0)) if abs(direction.y) < .9 else Vector((0, 0, 1))
        across = direction.cross(guide).normalized()
        up = across.cross(direction)
        corners = [across * (a * width / 2) + up * (b * depth / 2)
                   for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        mesh(name, [tuple(p + offset) for p in (start, end) for offset in corners], BOX, finish)

    def turned(name, centre, profile, sides, finish, *, bands=()):
        x, y, z = centre
        points = []
        for height, radius in profile:
            for index in range(sides):
                angle = math.tau * index / sides
                u, v = radius * math.cos(angle), radius * math.sin(angle)
                points.append((x + u, y + height, z + v))
        faces = []
        for level in range(len(profile) - 1):
            following = level + 1
            for index in range(sides):
                a, b = level * sides + index, level * sides + (index + 1) % sides
                faces.append((a, following*sides + index, following*sides + (index+1)%sides, b))
        faces.extend((tuple(range(sides)), tuple(reversed(range(len(points)-sides,len(points))))))
        mesh(name, points, faces, finish)
        if bands:
            data = parts[-1].data
            data.materials.append(iron)
            for level in bands:
                for index in range(sides):
                    data.polygons[level*sides + index].material_index = 1

    # A bench beside the draw point holds fetching vessels above the mud.
    bx, bz = 77.75, 128.15
    feet = [(bx + dx, bz + dz) for dx in (-.68, .68) for dz in (-.22, .22)]
    seat = max(ground(x, z) for x, z in feet) + .54
    for i, (x, z) in enumerate(feet):
        block(f"Water bench foot {i}", x-.08, x+.08, z-.08, z+.08,
              ground(x, z)-.07, seat-.08, oak, .018)
    for dz in (-.19, .19):
        block("Bench stretcher", bx-.76, bx+.76, bz+dz-.04, bz+dz+.04, seat-.21, seat-.08, oak)
    for i in range(3):
        z = bz-.32+i*.217
        block(f"Worn seat board {i}", bx-.88, bx+.88, z, z+.208, seat-.09, seat, oak, .024)
    pail = (bx-.42, seat, bz)
    turned("Open coopered pail", pail, [(0,.165),(.065,.19),(.10,.196),(.31,.238),
                                        (.345,.244),(.39,.245),(.39,.208),(.045,.14)],
           12, oak, bands=(1,3))
    for side in (-1,1):
        beam("Pail bail", (pail[0]+side*.225,seat+.33,bz), (pail[0]+side*.135,seat+.64,bz), .028,.035,iron)
    beam("Bail grip", (pail[0]-.15,seat+.64,bz), (pail[0]+.15,seat+.64,bz), .042,.05,oak)
    turned("Open earthenware water jar", (bx+.30,seat,bz),
           [(0,.13),(.055,.17),(.24,.225),(.35,.20),(.44,.112),(.49,.105),(.51,.135),
            (.545,.136),(.545,.096),(.47,.082),(.37,.098)], 14, clay)

    # The canopy bears into the masonry to leave the entire walking ramp free.
    floor = -8.709551
    for x in (80.81, 83.79):
        block("Canopy wall plate", x-.09,x+.09,119.17,119.35,floor+1.98,floor+3.22,oak)
        beam("Canopy knee brace", (x,floor+2.04,119.29),(x,floor+2.81,120.40),.13,.14,oak)
        beam("Canopy rafter", (x,floor+3.13,119.17),(x,floor+2.76,120.84),.15,.15,oak)
    block("Canopy eave",80.52,84.08,120.64,120.82,floor+2.67,floor+2.84,oak)
    # The lamp hangs forward of the sign, lighting its face and the ramp below.
    lx, lz = 84.34, 121.03
    cup = floor+2.64
    beam("Lamp hook", (84.72,floor+3.4,120.40),(84.52,cup,lz),.045,.045,iron)
    beam("Lamp cradle", (84.52,cup,lz),(lx,cup,lz),.045,.045,iron)
    turned("Sheltered oil cup",(lx,cup,lz),[(0,.075),(.09,.14),(.113,.14),(.113,.114),(.069,.097)],8,clay)
    turned("Oil-soaked wick",(lx,cup,lz),[(.065,.018),(.126,.014)],6,iron)
    flame = (lx, cup+.125, lz)
    light = (lx, cup+.24, lz+.055)
    for row in range(4):
        z0 = 119.12 + .411*row
        z1 = z0 + .475
        edges = [80.45] + [80.45+.455*i+(.215 if row%2 else 0) for i in range(1,8)] + [84.15]
        for tile, (x0,x1) in enumerate(zip(edges,edges[1:])):
            y0 = floor+3.28-.091*row
            bottom = [(x0+.005,y0,z0),(x1-.005,y0,z0),(x1-.005,y0-.11,z1),(x0+.005,y0-.11,z1)]
            mesh(f"Canopy clay course {row+1} tile {tile+1}", bottom+[(x,y+.052,z) for x,y,z in bottom], BOX, clay)

    # The tankard relief reads from the square, and its bracket casts a real shadow.
    sx, sz = 84.72, 120.27
    block("Sign wall mount",sx-.11,sx+.11,119.16,119.36,floor+2.66,floor+3.49,oak)
    beam("Sign arm",(sx,floor+3.4,119.20),(sx,floor+3.4,sz+.16),.145,.14,oak)
    beam("Sign gallows brace",(sx,floor+2.77,119.30),(sx,floor+3.37,sz-.12),.105,.105,oak)
    block("Sign crossbar",sx-.5,sx+.5,sz-.06,sz+.06,floor+3.30,floor+3.41,iron)
    for dx in (-.34,.34):
        block("Hanger",sx+dx-.022,sx+dx+.022,sz-.021,sz+.021,floor+3.04,floor+3.30,iron)
    for i in range(3):
        x0 = sx-.50+i*.334
        block("Signboard",x0,x0+.330,sz-.06,sz+.06,floor+2.17+.085*abs(i-1),floor+3.06,oak,.014)
    block("Tankard relief",sx-.28,sx+.10,sz+.058,sz+.108,floor+2.35,floor+2.87,stone,.022)
    for y in (2.37,2.84):
        block("Tankard rim",sx-.29,sx+.112,sz+.106,sz+.14,floor+y,floor+y+.048,stone)
    for y in (2.48,2.78):
        block("Tankard ear",sx+.08,sx+.268,sz+.061,sz+.112,floor+y,floor+y+.06,stone)
    block("Tankard handle",sx+.221,sx+.285,sz+.061,sz+.112,floor+2.50,floor+2.82,stone)

    # A low firewood rack sits beneath the south wall, clear of the sill and door.
    cx, cz = 86.85, 119.90
    rack = max(ground(cx+dx,cz+dz) for dx in (-.95,.95) for dz in (-.42,.42))+.16
    for dx in (-.98,.98):
        for dz in (-.34,.34):
            x,z = cx+dx,cz+dz
            block("Firewood rack upright",x-.065,x+.065,z-.065,z+.065,
                  ground(x,z)-.08,rack+.92,oak,.012)
    for dz in (-.26,.26):
        block("Rack runner",cx-1.05,cx+1.05,cz+dz-.07,cz+dz+.07,rack-.115,rack,oak)
    for row,count in enumerate((7,6,5,4)):
        for i in range(count):
            x = cx+(i-(count-1)/2)*.257
            y = rack+.128+row*.22
            end = .375+.035*math.sin(i*2.6+row)
            # A split face and the rounded bark make different silhouettes end-on.
            turn = (i*1.7+row)*.47
            section = [(.134*math.cos(a+turn),.134*math.sin(a+turn))
                       for a in (0,.85,1.70,2.55,3.60)]
            ends = [(x+u,y+v,cz+dz) for dz in (-.36,end) for u,v in section]
            n = len(section)
            faces = [tuple(reversed(range(n))),tuple(range(n,2*n))]
            faces.extend((j,(j+1)%n,(j+1)%n+n,j+n) for j in range(n))
            mesh(f"Split hearth billet {row+1}-{i+1}", ends, faces, oak)
    return parts, flame, light


def main(root, previous, output):
    terrain = root / town["TERRAIN"]
    digest = town["sha"]
    if output.exists() or digest(previous) != PARENT or digest(terrain) != town["TERRAIN_HASH"]:
        raise ValueError("Use the pinned town and terrain, and a fresh output directory.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, flame, light = furnishings(ground)
    output.mkdir(parents=True)
    master = output / "square-frontage.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    geometry["join_parts"](parts, "BellwetherSquareFrontage")
    patch = output / "square-frontage.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                              export_apply=True, export_tangents=True, export_normals=True, export_animations=False)
    result = output / previous.name
    added, vertices, primitives = town["join_town"](previous, patch, result)
    if not 0 < added <= 2100 or primitives != 5:
        raise ValueError(f"Unexpected frontage geometry: {added} triangles, {primitives} primitives")
    doc, _ = town["read_glb"](result)
    indices = sum(doc["accessors"][p["indices"]]["count"] for m in doc["meshes"] for p in m["primitives"])
    manifest = json.loads((previous.parent / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    town["save_json"](output / "asset.manifest.json", manifest)
    header = output / "geometry.generated.h"
    def anchor(name, position):
        values = ", ".join(f"{v:.7f}f" for v in position)
        return f"inline constexpr std::array<float,3> kSquarePorchLamp{name}{{{{{values}}}}};\n"

    header.write_text("#pragma once\n#include <array>\n#include <cstddef>\n\nnamespace horde::gameplay::bellwether {\n"
                      f"inline constexpr std::size_t kSquareFrontageSolidTriangles={added};\n"
                      + anchor("Flame", flame) + anchor("Light", light) + "}\n", encoding="utf-8", newline="\n")
    town["save_json"](output / "authoring-receipt.json", dict(
        feature="Square water bench and tavern south frontage", originalTownSha256=PARENT,
        terrainSha256=digest(terrain), runtimeTownSha256=digest(result), addedSolidTriangles=added,
        townVertices=vertices, townTriangles=indices//3, recipeSha256=digest(HERE), blender=bpy.app.version_string,
        files={p.name:digest(p) for p in (master, patch, result, header, output/"asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
