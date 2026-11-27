"""Fit the record keeper's daybook ledge and oil lamp against the church wall.

Run with Blender 5.2: -- ROOT PARENT_TOWN_GLB NEW_OUTPUT.
The parent manifest belongs beside its GLB.
"""

import json
import math
from pathlib import Path
import runpy
import sys

import bpy


RECIPE = Path(__file__).resolve()
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
meshes = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
PARENT = "aaa0ff5be10214504986238693f6bac9beaea97a9c92383d9a4ff462e2792d6b"
KEEPER = (71.3, 75.2)
WALL = 73.2


def author(ground):
    timber = town["material"]("HordeTavernTimber", (.27, .16, .08), .84)
    stone = town["material"]("HordeTownStone", (.34, .29, .22), .92)
    clay = town["material"]("HordeTownRoofClay", (.27, .13, .075), .87)
    iron = town["material"]("HordeHearthChar", (.068, .054, .043), .81)
    parts = []
    deck = ground(*KEEPER) + .07
    height = deck + 1.02

    def block(name, x0, x1, z0, z1, y0, y1, finish, bevel=0):
        parts.append(town["block"](name, x0, x1, z0, z1, y0, y1, finish, bevel))

    # Stone corbels take the load at the wall. The entire walk stays clear
    # under and in front of the ledge, including the cemetery turn behind it.
    for x in (71.91, 73.09):
        block("Corbel bonded into church wall", x-.13, x+.13, WALL-.08, WALL+.48,
              height-.34, height-.11, stone, .025)
        block("Upper corbel", x-.17, x+.17, WALL-.06, WALL+.74,
              height-.19, height-.075, stone, .018)
    for i in range(3):
        back = WALL + .022 + i*.249
        block("Oak work surface", 71.63, 73.43, back, back+.242,
              height-.085, height, timber, .012)
    block("Daybook lower cover", 72.29, 72.82, WALL+.236, WALL+.600,
          height, height+.043, clay, .011)
    block("Bound page edges", 72.317, 72.800, WALL+.249, WALL+.586,
          height+.042, height+.105, stone, .009)
    block("Daybook upper cover", 72.29, 72.82, WALL+.236, WALL+.600,
          height+.104, height+.134, clay, .010)
    block("Daybook spine", 72.28, 72.345, WALL+.235, WALL+.602,
          height+.024, height+.124, clay, .010)
    block("Writing slate with timber edge", 72.99, 73.23, WALL+.255, WALL+.540,
          height, height+.047, timber, .008)
    block("Inset writing surface", 73.013, 73.207, WALL+.278, WALL+.515,
          height+.045, height+.052, iron, .003)
    block("Writing point", 73.029, 73.045, WALL+.307, WALL+.489,
          height+.053, height+.068, timber, .003)

    # The oil cup sits on the uphill side of the service mark. The flame
    # and lamp light are exported from the same geometry coordinates.
    x, z, bottom = 70.61, WALL+.91, deck+1.59
    block("Wall strap", x-.052, x+.052, WALL-.049, WALL+.052,
          bottom-.27, bottom+.39, iron, .009)
    block("Oil cup support", x-.044, x+.044, WALL+.03, z+.02,
          bottom-.034, bottom+.006, iron, .006)
    block("Hood support", x-.034, x+.034, WALL+.024, z+.018,
          bottom+.337, bottom+.378, iron, .006)

    def turn(name, profile, material):
        sides = 10
        points = [(x + radius*math.cos(i*math.tau/sides), bottom + y,
                   z + radius*math.sin(i*math.tau/sides))
                  for y, radius in profile for i in range(sides)]
        strips = [(j*sides+i, (j+1)*sides+i, (j+1)*sides+(i+1)%sides, j*sides+(i+1)%sides)
                  for j in range(len(profile)-1) for i in range(sides)]
        cap = tuple(reversed(range(len(points)-sides, len(points))))
        parts.append(meshes["mesh"](name, points, [tuple(range(sides)), *strips, cap], material))

    turn("Oil cup and its hollow rim",
         ((0,.065),(.048,.115),(.092,.143),(.112,.143),(.112,.114),(.057,.089)), clay)
    turn("Short charred wick", ((.057,.017),(.127,.012)), iron)
    turn("Hood with a rolled rim",
         ((.329,.151),(.353,.169),(.380,.108),(.404,.088),(.391,.065),(.350,.083)), iron)
    flame = (x, bottom+.125, z)
    light = (x+.038, bottom+.205, z+.07)
    work = (72.49, height+.14, WALL+.414)
    return parts, flame, light, work


def main(root, parent, output):
    terrain = root / town["TERRAIN"]
    if output.exists() or town["sha"](parent) != PARENT or town["sha"](terrain) != town["TERRAIN_HASH"]:
        raise ValueError("Expected the pinned previous town and terrain and a new output directory.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, flame, light, work = author(ground)
    output.mkdir(parents=True)
    master = output / "church-record-station.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    meshes["join_parts"](parts, "BellwetherChurchRecordStation")
    detail = output / "church-record-station.glb"
    bpy.ops.export_scene.gltf(filepath=str(detail), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    runtime = output / "bellwether-tavern-v1.glb"
    added, vertices, primitives = town["join_town"](parent, detail, runtime)
    if not 0 < added <= 850 or primitives != 6:
        raise ValueError(f"Church record station exceeded its solid-face budget: {added}, {primitives} primitives")
    glb, _ = town["read_glb"](runtime)
    indices = sum(glb["accessors"][part["indices"]]["count"] for mesh in glb["meshes"]
                  for part in mesh["primitives"])
    manifest = json.loads(parent.with_name("asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    manifest_path = output / "asset.manifest.json"
    town["save_json"](manifest_path, manifest)
    header = output / "geometry.generated.h"
    lines = ["#pragma once", "#include <array>", "#include <cstddef>", "",
             "// Generated by tools/author-bellwether-church-station.py.",
             "namespace horde::gameplay::bellwether {",
             f"inline constexpr std::size_t kChurchRecordStationTriangles={added};"]
    for name, coordinate in (("Flame", flame), ("Light", light), ("Daybook", work)):
        values = ", ".join(f"{v:.7f}f" for v in coordinate)
        lines.append(f"inline constexpr std::array<float,3> kChurchRecordStation{name}{{{{{values}}}}};")
    header.write_text("\n".join([*lines, "} // namespace horde::gameplay::bellwether", ""]),
                      encoding="utf-8", newline="\n")
    town["save_json"](output / "authoring-receipt.json", dict(
        feature="B02 church record keeper's work ledge and oil cup",
        originalTownSha256=PARENT, terrainSha256=town["TERRAIN_HASH"],
        addedSolidTriangles=added, primitives=primitives, townVertices=vertices, townTriangles=indices//3,
        flame=flame, light=light, daybook=work, blender=bpy.app.version_string,
        recipeSha256=town["sha"](RECIPE),
        files={p.name: town["sha"](p) for p in (master, detail, runtime, manifest_path, header)}))


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--")+1:]
    if len(args) != 3:
        raise ValueError("Expected ROOT, PARENT_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(value).resolve() for value in args))
