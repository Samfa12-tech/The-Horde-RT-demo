"""Build the pegged rail on the square side of the tavern. Blender 5.2."""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy

RECIPE = Path(__file__).resolve()
PARENT = "35decf1c875b08bbf212a114c7e71fb7d9da87635aaa1333d59bf7aa30fb5337"
SURVEY = "ea61d89fcf753652b4e7ea4d7833988a5895eca9c1d6e21ed7fb3d91c2ee1585"
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))


def build_rail(survey):
    door_x, sill_y, sill_z = survey["southDoor"]
    _, toe_y, toe_z = survey["rampToe"]
    rail_x = door_x + survey["rampWidth"] / 2 - .19
    grade = (toe_y - sill_y) / (toe_z - sill_z)
    oak = town["material"]("HordeTavernTimber", (.27, .16, .085), .84)
    stone = town["material"]("HordeTownStone", (.30, .27, .23), .90)
    iron = town["material"]("HordeHearthChar", (.06, .05, .04), .80)
    parts = []

    def deck(z):
        return sill_y + (z - sill_z) * grade

    def extrusion(name, rings, finish):
        count = len(rings[0])
        faces = [tuple(reversed(range(count)))]
        for section in range(len(rings) - 1):
            start = section * count
            for i in range(count):
                a, b = start + i, start + (i + 1) % count
                faces.append((a, b, b + count, a + count))
        faces.append(tuple(range((len(rings) - 1) * count, len(rings) * count)))
        parts.append(geometry["mesh"](name, [point for ring in rings for point in ring], faces, finish))

    def rail(name, start, end, height, width, depth, bevel, end_cut):
        # Vertical end cuts and a planed crown. Each length meets its neighbour
        # over a post; the two exposed ends are eased back from the main section.
        rings = []
        for z, taper in ((start, end_cut[0]), (start + .095, 1.),
                         (end - .095, 1.), (end, end_cut[1])):
            w, h = width / 2 * taper, depth / 2 * taper
            section = [(-w+bevel,-h), (w-bevel,-h), (w,-h+bevel), (w,h-bevel),
                       (w-bevel,h), (-w+bevel,h), (-w,h-bevel), (-w,-h+bevel)]
            rings.append([(rail_x + x, deck(z) + height + y, z) for x, y in section])
        extrusion(name, rings, oak)

    stations = [sill_z + offset for offset in (1.02, 3.12, 5.35, 7.60, 9.88)]
    for i, z in enumerate(stations):
        y = deck(z)
        for name, width, bottom, top, finish, chamfer in (
                ("Socket stone", .285, y-.11, y+.105, stone, .035),
                ("Oak upright", .156, y+.06, y+.895, oak, .018)):
            half = width / 2
            parts.append(town["block"](name, rail_x-half, rail_x+half, z-half, z+half,
                                       bottom, top, finish, chamfer))
        for height in ((.80, .47) if i < 4 else (.80,)):
            # The head is visible from the walking side of the post.
            rings = []
            for dx, radius in ((-.099, .025), (-.091, .032), (.085, .027)):
                rings.append([(rail_x+dx, y+height+radius*math.cos(j*math.tau/8),
                               z+radius*math.sin(j*math.tau/8)) for j in range(8)])
            extrusion("Draw pin", rings, iron)

    for i, (a, b) in enumerate(zip(stations, stations[1:])):
        first, last = i == 0, i == len(stations)-2
        # Keep end grain at the joints without overlapping end faces.
        rail("Planed handrail", a - .22 if first else a + .003,
             b + .22 if last else b - .003, .89, .155, .135, .025,
             (.72 if first else 1., .72 if last else 1.))
        if not last:
            rail("Lower guard rail", a+.075, b-.075, .47, .08, .10, .012, (1.,1.))
    return parts


def main(root, previous, output):
    survey_file = root / "assets/models/world/source/bellwether-tavern-v1/authoring-receipt.json"
    if output.exists() or town["sha"](previous) != PARENT or town["sha"](survey_file) != SURVEY:
        raise ValueError("Supply the pinned west-trade town, ramp survey and an unused output directory")
    survey = json.loads(survey_file.read_bytes())
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = build_rail(survey)
    output.mkdir(parents=True)
    master = output / "tavern-rail.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    geometry["join_parts"](parts, "BellwetherTavernRail")
    patch = output / "tavern-rail.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                              export_apply=True, export_tangents=True, export_normals=True, export_animations=False)
    runtime = output / previous.name
    added, vertices, primitives = town["join_town"](previous, patch, runtime)
    if not 0 < added <= 1300 or primitives != 5:
        raise ValueError(f"Unexpected ramp rail: {added} triangles, {primitives} primitives")
    manifest = json.loads((previous.parent / "asset.manifest.json").read_bytes())
    indices = manifest["budgets"]["maxIndices"] + 3 * added
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    town["save_json"](output / "asset.manifest.json", manifest)
    header = output / "geometry.generated.h"
    header.write_text("#pragma once\n#include <cstddef>\n\nnamespace horde::gameplay::bellwether {\n"
                      f"inline constexpr std::size_t kTavernRailSolidTriangles={added};\n"
                      "}\n", encoding="utf-8", newline="\n")
    town["save_json"](output / "authoring-receipt.json", dict(feature="South tavern ramp handrail",
        originalTownSha256=PARENT, rampSurveySha256=town["sha"](survey_file), runtimeTownSha256=town["sha"](runtime),
        addedSolidTriangles=added, townVertices=vertices, townTriangles=indices // 3,
        recipeSha256=town["sha"](RECIPE), blender=bpy.app.version_string,
        files={p.name: town["sha"](p) for p in (master, patch, runtime, header, output/"asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
