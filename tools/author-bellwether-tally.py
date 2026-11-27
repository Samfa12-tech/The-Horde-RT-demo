"""Add the tally case and the two working marks to the exact admitted town."""
import json
from pathlib import Path
import runpy
import sys

import bpy


RECIPE = Path(__file__).resolve()
region = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PARENT = "2ed3b1b980fafe18013265caee40897fa3d67d628d5fca968685c5b55a0ff4fd"


def build_tally_props(ground):
    timber = region["material"]("HordeTavernTimber", (.24, .14, .075), .87)
    iron = region["material"]("HordeHearthChar", (.073, .055, .04), .80)
    clay = region["material"]("HordeTownRoofClay", (.26, .12, .065), .85)
    parts = []

    def block(label, x0, x1, z0, z1, y0, y1, material, bevel=0.):
        parts.append(region["block"](label, x0, x1, z0, z1, y0, y1, material, bevel))

    # Store the impression stamp inside the storm case, against the north rail.
    # The walking centre line and the turnout's east end remain accessible.
    platform = crossing["TURNOUT_HEIGHT"]
    block("Tally case body", 143.39, 144.16, 92.90, 93.33, platform, platform + .37, timber, .022)
    block("Overhanging tally case lid", 143.36, 144.19, 92.88, 93.36,
          platform + .36, platform + .44, timber, .018)
    for x in (143.46, 144.02):
        block(f"Case band {x}", x, x + .055, 92.88, 93.36, platform + .434, platform + .459, iron)
        block(f"Case band end {x}", x, x + .055, 93.317, 93.366, platform + .08, platform + .437, iron)
    block("Tally case clasp", 143.73, 143.82, 93.32, 93.375, platform + .314, platform + .438, iron, .006)

    # A separate small tray on the east end of the existing dealer counter.
    # It is north of the aisle while the ordinary trade gesture faces west.
    counter = ground(60.3, 129.2) + .915
    block("Dealer pledge tray base", 61.282, 61.696, 128.19, 128.53, counter - .01, counter + .044, timber, .008)
    for x in (61.28, 61.65):
        block(f"Pledge tray end {x}", x, x + .049, 128.185, 128.53, counter + .035, counter + .095, iron)
    block("Pledge tray back", 61.298, 61.668, 128.178, 128.222, counter + .03, counter + .095, iron)
    block("Pledge tally slip", 61.34, 61.58, 128.28, 128.476, counter + .041, counter + .050, clay)
    for z in (128.323, 128.375, 128.425):
        block(f"Pledge tally line {z}", 61.38, 61.54, z, z + .016, counter + .049, counter + .057, iron)

    # A fixed marked board beside the mill's west entrance. Its west-facing
    # mark is independently reachable from the established landing.
    mill = -13.764971
    block("Mill tally board fixing", 118.135, 118.28, 112.011, 112.326, mill + .88, mill + 1.31, timber)
    block("Mill tally board", 118.081, 118.163, 111.94, 112.445, mill + .894, mill + 1.30, timber, .009)
    for dz, height in ((112.082, .16), (112.151, .20), (112.224, .16)):
        block(f"Raised mill tally {dz}", 118.07, 118.084, dz, dz + .029,
              mill + 1.015, mill + 1.015 + height, iron)

    marks = {
        "CaseReach": (143.776, platform + .54, 93.35),
        "TrayReach": (61.48, counter + .16, 128.43),
        "BoardReach": (118.028, mill + 1.12, 112.19),
        "CaseView": (143.72, 94.00),
        "TrayView": (61.87, 129.16),
        "BoardView": (117.20, 112.20),
    }
    return parts, marks


def main(root, previous, output):
    terrain = root / region["TERRAIN"]
    if output.exists() or region["sha"](previous) != PARENT or region["sha"](terrain) != region["TERRAIN_HASH"]:
        raise ValueError("Choose a new output and supply the exact lit-grate town and surveyed terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = region["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, marks = build_tally_props(ground)
    output.mkdir(parents=True)
    master = output / "bellwether-tally.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    crossing["join_parts"](parts, "BellwetherTallyProps")
    patch = output / "tally-props.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output / "bellwether-tavern-v1.glb"
    triangles, vertices, primitives = region["join_town"](previous, patch, town)
    parent, _ = region["read_glb"](previous)
    indices = sum(parent["accessors"][p["indices"]]["count"]
                  for mesh in parent["meshes"] for p in mesh["primitives"]) + 3 * triangles
    if not 0 < triangles <= 500 or primitives != 5 or vertices > 51500:
        raise ValueError(f"Tally props exceed town budget: {triangles} new triangles, {vertices} vertices")
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    region["save_json"](output / "asset.manifest.json", manifest)
    header = [
        "#pragma once", "#include <array>", "#include <cstddef>", "",
        "// Generated by tools/author-bellwether-tally.py.",
        "namespace horde::gameplay::bellwether {",
        f"inline constexpr std::size_t kMillTallySolidTriangles={triangles};",
    ]
    for name, coordinates in marks.items():
        values = ", ".join(f"{value:.6f}f" for value in coordinates)
        header.append(f"inline constexpr std::array<float, {len(coordinates)}> kMillTally{name}{{{{{values}}}}};")
    header += ["} // namespace horde::gameplay::bellwether", ""]
    layout = output / "geometry.generated.h"
    layout.write_text("\n".join(header), encoding="utf-8", newline="\n")
    items = (master, patch, town, layout, output / "asset.manifest.json")
    region["save_json"](output / "authoring-receipt.json", dict(
        feature="Mill tally case and service marks", marks=marks, originalTownSha256=PARENT,
        terrainSha256=region["TERRAIN_HASH"], runtimeTownSha256=region["sha"](town),
        addedSolidTriangles=triangles, townVertices=vertices, townTriangles=indices // 3,
        blender=bpy.app.version_string, recipeSha256=region["sha"](RECIPE),
        files={item.name: region["sha"](item) for item in items}))


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) != 3:
        raise ValueError("Expected ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(arg).resolve() for arg in args))
