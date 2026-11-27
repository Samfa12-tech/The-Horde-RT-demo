"""Author the S6 stall and add its solid meshes to the pinned Bellwether town.

blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-relic-stall.py -- ROOT PREVIOUS_TOWN_GLB NEW_OUTPUT
"""
import json
from pathlib import Path
import runpy
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree


HERE = Path(__file__).resolve()
WORKS = HERE.with_name("author-bellwether-mill-works.py")
CROSSING = HERE.with_name("author-bellwether-mill-crossing.py")
work = runpy.run_path(str(WORKS))
crossing = runpy.run_path(str(CROSSING))
PREVIOUS_TOWN = "4a970dc48d0d7028310ddc0cf4dd26c00c1325a77136ce52470f9287e784a4d6"
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
DEALER = (60.3, 129.2)
STOCK_WING = (63.34, 63.49, 128.27, 130.43)
TURN = (62.1, 131.70)
APPROACH = (62.1, 129.2)
LAMP = (61.04, 128.25)


def build_stall(ground):
    stone = work["material"]("HordeTownStone", (.24, .21, .17), 1)
    timber = work["material"]("HordeTavernTimber", (.27, .16, .085), .84)
    clay = work["material"]("HordeTownRoofClay", (.26, .108, .058), .82)
    iron = work["material"]("HordeHearthChar", (.06, .046, .036), .72)
    parts = []
    floor = ground(*DEALER)

    def box(name, x0, x1, z0, z1, lower, upper, finish, bevel=0.):
        parts.append(work["block"](name, x0, x1, z0, z1, floor + lower, floor + upper, finish, bevel))

    def wedge(name, x0, x1, z0, z1, lower_west, lower_east, depth, finish):
        points = [(x0, floor + lower_west, z0), (x1, floor + lower_east, z0),
                  (x1, floor + lower_east, z1), (x0, floor + lower_west, z1)]
        parts.append(crossing["mesh"](name, points + [(x, y + depth, z) for x, y, z in points],
            [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2), (7, 4, 0, 3), (7, 6, 5, 4)], finish))

    # Stone feet support the pegged frame. The roof drains to the rear and
    # protects the east-facing opening into the stall.
    for x0, z0 in ((58.21, 127.38), (58.21, 130.68), (63.30, 127.38), (63.30, 130.68)):
        bottom = ground(x0 + .10, z0 + .10) - floor - .16
        box(f"Pier {x0} {z0}", x0-.10, x0+.31, z0-.10, z0+.31, bottom, .27, stone, .025 if x0 > 60 else 0.)
        box(f"Oak upright {x0} {z0}", x0, x0+.19, z0, z0+.19, .22, 2.53 if x0 < 60 else 2.77, timber)
    box("Rear stone skirting", 58.22, 58.54, 127.45, 130.85, -.32, .62, stone)
    box("North stone skirting", 58.45, 63.38, 127.43, 127.71, -.34, .62, stone)
    box("Rear timber closure", 58.26, 58.42, 127.53, 130.73, .58, 2.42, timber)
    box("North timber closure", 58.39, 63.36, 127.47, 127.62, .58, 2.44, timber)
    for z in (127.53, 128.27, 129.08, 129.90, 130.66):
        box(f"Rear closing batten {z}", 58.39, 58.49, z, z+.055, .60, 2.39, timber)
    for x in (58.6, 59.65, 60.71, 61.79, 62.85):
        box(f"North closing batten {x}", x, x+.07, 127.59, 127.67, .60, 2.43, timber)
    box("East lintel", 63.29, 63.52, 127.40, 130.89, 2.58, 2.80, timber)
    box("Rear lintel", 58.20, 58.44, 127.40, 130.89, 2.31, 2.55, timber)
    wedge("Rafter bed", 58.14, 63.64, 127.29, 131.02, 2.44, 2.73, .14, timber)
    for z in (127.38, 128.11, 129.16, 130.14, 130.81):
        wedge(f"Visible rafter {z}", 58.17, 63.68, z, z+.12, 2.34, 2.63, .13, timber)
    # Overlapping clay courses and fascia preserve the readable roof edge.
    for course in range(10):
        x = 57.98 + course * .596
        wedge(f"Overlapping clay course {course}", x, x+.651, 127.17, 131.08,
              2.585+course*.031, 2.634+course*.031, .067, clay)
    box("Front fascia", 63.63, 63.97, 127.20, 131.06, 2.75, 2.93, timber)

    # The tall back of the stock display shades the cell from the square.
    # The player comes around its south end to reach the open aisle.
    x0, x1, z0, z1 = STOCK_WING
    box("Stock wing sill", x0-.09, x1+.09, z0-.07, z1+.07, -.19, .29, stone, .018)
    for board in range(6):
        lo = z0 + board*(z1-z0)/6
        hi = z0 + (board+1)*(z1-z0)/6
        box(f"Display back board {board}", x0, x1, lo-.006, hi+.006, .24,
            2.25 if board < 4 else 2.18, timber)
    for z in (z0-.055, z1-.045):
        box(f"Stock wing upright {z}", x0-.055, x1+.045, z, z+.11, .18, 2.29, timber)
    for level in (.32, 1.39, 2.08):
        box(f"Display cross cleat {level}", x1-.012, x1+.057, z0-.05, z1+.05, level, level+.065, iron)

    # A carved weighing sign marks the entrance without adding typography or
    # a bespoke material. The three bars and pans sit on an oak backboard.
    box("Sign backboard", 63.49, 63.58, 129.36, 130.21, 1.49, 2.06, timber, .025)
    for z, width, lo, hi in ((129.785, .045, 1.60, 1.96), (129.785, .61, 1.885, 1.932),
                            (129.785, .21, 1.59, 1.635), (129.54, .023, 1.716, 1.90),
                            (130.03, .023, 1.716, 1.90), (129.54, .19, 1.690, 1.735),
                            (130.03, .19, 1.690, 1.735)):
        box(f"Scale sign bar {z} {lo}", 63.57, 63.599, z-width/2, z+width/2, lo, hi, iron)

    # Counter and stock stay along the north side. Walking and conversation
    # share one clear strip of the original ground on the south side.
    for x in (59.38, 61.45):
        box(f"Counter end support {x}", x, x+.20, 127.80, 128.56, -.06, .82, timber)
    box("Counter rail", 59.48, 61.59, 128.44, 128.54, .21, .39, timber)
    for z in (127.75, 128.02, 128.29):
        box(f"Counter top plank {z}", 59.26, 61.72, z, z+.286, .80, .915, timber, .012)
    box("Trader's chest", 59.56, 60.18, 127.88, 128.28, .918, 1.24, timber, .018)
    for x in (59.62, 60.04):
        box(f"Chest strap {x}", x, x+.055, 127.86, 128.30, 1.229, 1.251, iron)
    box("Chest lock", 59.80, 59.94, 128.269, 128.304, 1.063, 1.178, iron, .008)
    box("Weight tray", 60.28, 60.78, 128.15, 128.40, .916, .951, timber, .010)
    for x, height in ((60.33, .054), (60.47, .076), (60.61, .106)):
        box(f"Trade weight {x}", x, x+.080, 128.225, 128.315, .951, .951+height, iron)
    box("Wick support foot", 60.91, 61.17, 128.12, 128.38, .914, .953, iron, .012)
    box("Wick stem", 61.011, 61.069, 128.218, 128.282, .95, 1.476, iron)
    box("Ceramic wick bowl", 60.93, 61.15, 128.15, 128.35, 1.424, 1.493, clay, .020)
    for x, z, size in ((58.62, 129.71, .49), (58.39, 130.26, .58)):
        box(f"Rear stock crate {x}", x, x+size, z, z+size, -.08, .48, timber)
        box(f"Crate band {x}", x+.10, x+.15, z-.012, z+size+.012, .45, .495, iron)

    flame = (LAMP[0], floor + 1.507, LAMP[1])
    light = (LAMP[0] + .05, floor + 1.66, LAMP[1] + .10)
    return parts, flame, light


def survey_stall(parts, ground, light):
    vertices, faces = [], []
    for obj in parts:
        offset = len(vertices)
        vertices.extend(obj.matrix_world @ point.co for point in obj.data.vertices)
        faces.extend(tuple(offset + index for index in face.vertices) for face in obj.data.polygons)
    tree = BVHTree.FromPolygons(vertices, faces)

    def point(x, y, z):
        return Vector((x, -z, y))

    def clear(source, target):
        direction = target - source
        return tree.ray_cast(source, direction.normalized(), direction.length - .01)[0] is None

    dealer_floor = ground(*DEALER)
    targets = [point(DEALER[0], dealer_floor + h, DEALER[1]) for h in (.35, .82, 1.24, 1.61)]

    def sightlines(x, z):
        eye = point(x, ground(x, z) + 1.6675, z)
        return [clear(eye, target) for target in targets]

    talk = sightlines(*APPROACH)
    square = sightlines(82.3, 131.7)
    east = sightlines(96.0, 127.5)
    wick_to_face = clear(point(*light), targets[-1])
    if not all(talk) or any(square) or any(east) or not wick_to_face:
        raise ValueError(f"Review stall cover or conversation: {talk=}, {square=}, {east=}, {wick_to_face=}")
    return {"talk": talk, "square": square, "eastLane": east, "lightToFace": wick_to_face}


def main(root, previous, output):
    terrain = root / work["TERRAIN"]
    if output.exists() or work["sha"](previous) != PREVIOUS_TOWN or work["sha"](terrain) != work["TERRAIN_HASH"]:
        raise ValueError("Choose a new output, the pinned fork-lamp town GLB, and the admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, flame, light = build_stall(ground)
    visibility = survey_stall(parts, ground, light)
    output.mkdir(parents=True)
    master = output / "bellwether-relic-stall.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    crossing["join_parts"](parts, "BellwetherRelicStall")
    patch = output / "relic-stall.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output / "bellwether-tavern-v1.glb"
    added, vertex_count, primitives = work["join_town"](previous, patch, town)
    original, _ = work["read_glb"](previous)
    original_count = sum(original["accessors"][p["indices"]]["count"] // 3
                         for mesh in original["meshes"] for p in mesh["primitives"])
    triangles = original_count + added
    if not 0 < added <= 1200 or primitives != 5 or vertex_count > 16000:
        raise ValueError(f"Stall exceeds town geometry budget: {added} new triangles, {vertex_count} total vertices")
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertex_count, maxIndices=triangles * 3)
    manifest["lods"][0]["maxTriangles"] = triangles
    work["save_json"](output / "asset.manifest.json", manifest)

    def mark(name, position):
        values = ", ".join(f"{value:.6f}f" for value in position)
        return f"inline constexpr std::array<float, {len(position)}> kRelicStall{name}{{{{{values}}}}};"

    stage = ["#pragma once", "#include <array>", "#include <cstddef>", "",
             "// Generated by tools/author-bellwether-relic-stall.py.",
             "namespace horde::gameplay::bellwether {",
             f"inline constexpr std::size_t kRelicStallSolidTriangles={added};",
             mark("ApproachTurn", TURN), mark("LampFlame", flame), mark("LampLight", light),
             "} // namespace horde::gameplay::bellwether", ""]
    (output / "geometry.generated.h").write_text("\n".join(stage), encoding="utf-8", newline="\n")
    work["save_json"](output / "authoring-receipt.json", dict(
        mapService="S6", originalTownSha256=PREVIOUS_TOWN, terrainSha256=work["TERRAIN_HASH"],
        runtimeTownSha256=work["sha"](town), townVertices=vertex_count, townTriangles=triangles,
        addedSolidTriangles=added, westTurn=TURN, talkMark=APPROACH, dealerMark=DEALER,
        stockWingXZ=STOCK_WING, lampFlame=flame, lampLight=light, visibility=visibility,
        recipeSha256=work["sha"](HERE), workshopRecipeSha256=work["sha"](WORKS),
        crossingRecipeSha256=work["sha"](CROSSING), blender=bpy.app.version_string,
        files={name: work["sha"](output / name) for name in (
            master.name, patch.name, "asset.manifest.json", "geometry.generated.h", town.name)}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1:]
    if len(arguments) != 3:
        raise ValueError("Expected repository ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
