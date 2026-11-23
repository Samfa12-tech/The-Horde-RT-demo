"""Build the dry, locked garden-edge recess from the exact admitted town."""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy


RECIPE = Path(__file__).resolve()
world = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
model = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PARENT_SHA = "d08a45df8a6ab34792a6918833074f05bfd94d82ea1a4dd84bad4fcb00fbb72a"
CENTRE = (39.2, 130.6)
BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2), (7, 4, 0, 3), (7, 6, 5, 4)]


def build_recess(ground):
    stone = world["material"]("HordeTownStone", (.23, .21, .15), .95)
    timber = world["material"]("HordeTavernTimber", (.24, .14, .075), .87)
    clay = world["material"]("HordeTownRoofClay", (.26, .12, .065), .85)
    iron = world["material"]("HordeHearthChar", (.073, .055, .04), .80)

    x, z = CENTRE
    floor = ground(x, z)
    parts = []

    def point(dx, y, dz):
        return x + dx, floor + y, z + dz

    def box(label, x0, x1, z0, z1, bottom, top, material, footing=False):
        corners = ((x0, z0), (x1, z0), (x1, z1), (x0, z1))
        points = [point(dx, y(dz) if callable(y) else y, dz)
                  for y in (bottom, top) for dx, dz in corners]
        if footing:
            points[:4] = [(a, min(y, ground(a, c) - .03), c) for a, y, c in points[:4]]
        parts.append(model["mesh"](label, points, BOX, material))

    # The chamber floor sits just above the terrain, behind the raised lip.
    box("Dry stone floor", -.89, .89, -1.24, .85, -.18, .10, stone, True)
    box("Front retaining wall", -1.01, 1.01, .64, .86, -.12, .87, stone, True)
    box("Back shelter wall", -1.01, 1.01, -1.29, -1.09, -.13, 1.69, stone, True)
    for i, side in enumerate((-1, 1)):
        x0, x1 = sorted((side * .77, side * 1.02))
        box(f"Side cheek {i}", x0, x1, -1.19, .81, -.17, .89, stone, True)
        box(f"Side coping {i}", x0 - .035, x1 + .035, -.18, .87, .863, .927, stone)
        box(f"Shelter post {i}", x0, x1, -.27, -.15, .86, 1.69, timber)

    for row in range(3):
        for course in range(5):
            left = -1.0 + course * .40
            offset = .033 if row % 2 and (course == 0 or course == 4) else 0.
            box(f"Face stone {row}/{course}", left + .007 + offset, left + .391 - offset,
                .842, .887 + .005 * ((row + course) % 3),
                .045 + row * .269, .289 + row * .269, stone)
    box("Outer coping", -1.07, 1.07, .62, .923, .84, .944, stone)

    def eave(dz):
        return 1.73 - .26 * dz

    box("Overlapping shelter roof", -1.17, 1.17, -1.44, -.11, lambda dz: eave(dz) - .077, eave, timber)
    for index in range(5):
        z0 = -1.395 + index * .256
        box(f"Clay shelter course {index}", -1.20, 1.20, z0, z0 + .285,
            lambda dz: eave(dz) - .004, lambda dz: eave(dz) + .043, clay)

    for index in range(7):
        bar = -.63 + index * .21
        box(f"Grate bar {index}", bar - .022, bar + .022, -.30, .685, .896, .954, iron)
    for index, z0 in enumerate((-.29, .08, .654)):
        box(f"Grate cross strap {index}", -.79, .79, z0 - .029, z0 + .029, .928, .982, iron)
    box("Grate hinge strap", -.74, -.56, -.375, -.20, .95, .995, iron)
    box("Lock keeper", .29, .45, .636, .837, .95, .993, iron)
    for x0 in (.313, .405):
        box(f"Lock shackle foot {x0}", x0, x0 + .021, .786, .819, .965, 1.067, iron)
    box("Lock shackle crown", .309, .43, .782, .822, 1.052, 1.083, iron)
    box("Closed iron lock body", .299, .438, .798, .873, .94, 1.019, iron)

    box("Dry bed board", -.69, .13, -1.08, -.30, .13, .255, timber)
    box("Folded blanket", -.65, .09, -.62, -.338, .249, .322, clay)
    box("Folded blanket hem", -.658, .098, -.409, -.339, .285, .334, clay)
    box("Meal board shelf", .31, .754, -.83, -.20, .393, .461, timber)
    box("Meal board", .38, .71, -.50, -.245, .463, .493, clay)
    box("Covered water vessel", .452, .646, -.782, -.586, .459, .729, clay)
    box("Water vessel lid", .465, .633, -.768, -.601, .721, .757, timber)
    box("Water vessel knob", .522, .578, -.711, -.650, .747, .787, timber)

    # Bring the shared RT flame in through the open sheltered side. The
    # carrier remains fixed to the wall below the iron bars.
    box("Lamp stone ledge", -.78, -.42, -.16, .06, .532, .597, stone)
    box("Lamp oil tray", -.67, -.46, -.125, .035, .59, .641, clay)
    box("Lamp lip", -.684, -.446, -.127, -.109, .629, .675, clay)
    box("Lamp lip outer", -.684, -.446, .021, .038, .629, .675, clay)
    box("Lamp wick", -.566, -.536, -.052, -.022, .64, .681, iron)
    return (parts, point(.11, .68, -.42), point(0., 1.26, 1.03), point(0., 0., 1.65),
            point(-.55, .683, -.037), point(-.47, .849, -.065))


def main(root, previous, output):
    terrain = root / world["TERRAIN"]
    if output.exists() or world["sha"](previous) != PARENT_SHA or world["sha"](terrain) != world["TERRAIN_HASH"]:
        raise ValueError("Choose a new output and supply the exact smith-yard town and admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = world["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, mouth, listen, approach, flame, light = build_recess(ground)
    output.mkdir(parents=True)
    master = output / "bellwether-grate.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    model["join_parts"](parts, "BellwetherGardenGrate")
    patch = output / "garden-grate.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output / "bellwether-tavern-v1.glb"
    added, vertices, primitives = world["join_town"](previous, patch, town)
    parent, _ = world["read_glb"](previous)
    indices = sum(parent["accessors"][p["indices"]]["count"]
                  for mesh in parent["meshes"] for p in mesh["primitives"]) + 3 * added
    if not 0 < added <= 850 or primitives != 5 or vertices > 51000:
        raise ValueError(f"Garden grate exceeds town budget: {added} new triangles, {vertices} vertices")
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    world["save_json"](output / "asset.manifest.json", manifest)
    header = [
        "#pragma once", "#include <array>", "#include <cstddef>", "",
        "// Generated by tools/author-bellwether-grate.py.",
        "namespace horde::gameplay::bellwether {",
        f"inline constexpr std::size_t kBellwetherGrateSolidTriangles={added};",
    ]
    marks = (("Mouth", mouth), ("ListenPoint", listen), ("Approach", approach[::2]),
             ("LampFlame", flame), ("LampLight", light))
    for name, coordinates in marks:
        values = ", ".join(f"{value:.6f}f" for value in coordinates)
        header.append(f"inline constexpr std::array<float, {len(coordinates)}> kBellwetherGrate{name}{{{{{values}}}}};")
    header += ["} // namespace horde::gameplay::bellwether", ""]
    (output / "geometry.generated.h").write_text("\n".join(header), encoding="utf-8", newline="\n")
    files = (master, patch, town, output / "asset.manifest.json", output / "geometry.generated.h")
    world["save_json"](output / "authoring-receipt.json", dict(
        feature="Dry garden-edge grate", centre=CENTRE, mouth=mouth, listenPoint=listen,
        approachXZ=approach[::2], lampFlame=flame, lampLight=light, originalTownSha256=PARENT_SHA,
        terrainSha256=world["TERRAIN_HASH"],
        runtimeTownSha256=world["sha"](town), addedSolidTriangles=added, townVertices=vertices,
        townTriangles=indices // 3, blender=bpy.app.version_string, recipeSha256=world["sha"](RECIPE),
        files={path.name: world["sha"](path) for path in files}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1:]
    if len(arguments) != 3:
        raise ValueError("Expected ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
