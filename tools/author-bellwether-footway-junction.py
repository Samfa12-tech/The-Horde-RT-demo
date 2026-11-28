"""Blend the surveyed mill footway into the river causeway with Blender 5.2."""
import json
from pathlib import Path
import runpy
import sys

import bpy
import numpy as np


RECIPE = Path(__file__).resolve()
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
TOWN_NAME = "bellwether-tavern-v1.glb"
PARENT_SHA256 = "d38c9ce94bc559050b722cd2118a98ae175d5c8d730b4f48345c2eee090ee192"
NORTH, CENTRE, SOUTH = 121.60, 122.55, 123.50
JUNCTION = crossing["X_EAST"]
# East coordinate, centre height, and the rise of the bank-side stones.
GRADE = (
    (134.70, -21.81, 0),
    (JUNCTION, -21.81 - (JUNCTION - 134.70) * (23.48 - 21.81) / (137.50 - 134.70), 0),
    (137.40, -23.055, .195),
    (138.11, -23.48, .195),
    (138.55, -23.48, 0),
)


def remove_old_grade(source, output):
    doc, blocks = town["read_glb"](source)
    for node in doc["nodes"]:
        solid = node["name"] == "BellwetherSolid"
        stone, = (p for p in doc["meshes"][node["mesh"]]["primitives"]
                  if doc["materials"][p["material"]]["name"] == "HordeTownStone")
        attrs = stone["attributes"]
        pos = town["read_accessor"](doc, blocks, attrs["POSITION"]).copy()
        indices = town["read_accessor"](doc, blocks, stone["indices"])
        triangles = indices.reshape(-1, 3)
        xyz = pos[triangles]
        north = np.isclose(xyz[:, :, 2], NORTH, rtol=0, atol=.0001)
        south = np.isclose(xyz[:, :, 2], SOUTH, rtol=0, atol=.0001)
        within_x = (xyz[:, :, 0].min(axis=1) >= 134.6999) & (xyz[:, :, 0].max(axis=1) <= 138.5501)
        between_sections = xyz[:, :, 0].max(axis=1) - xyz[:, :, 0].min(axis=1) > .01
        grade_band = (xyz[:, :, 1].max(axis=1) < -21.79) & (xyz[:, :, 1].max(axis=1) >= -23.4801)
        faces = within_x & between_sections & grade_band
        if solid:
            faces &= ((north.all(axis=1) & (xyz[:, :, 0].min(axis=1) >= JUNCTION - .0001)) | south.all(axis=1))
            # The wall lips already sit inside the crossing deck. Terminate
            # both retaining sides beneath the floor through the shared seam.
            uv = town["read_accessor"](doc, blocks, attrs["TEXCOORD_0"]).copy()
            for edge, old_height, height in ((crossing["X_WEST"], -21.754, -22.16),
                                             (crossing["X_EAST"], -22.615, -22.85)):
                lip = np.all(np.isclose(pos, (edge, old_height, 122.28), rtol=0, atol=.0001), axis=1)
                if np.count_nonzero(lip) != 1:
                    raise ValueError(f"The footway retaining corner at {edge} changed.")
                pos[lip, 1] = height
                uv[lip, 1] += (old_height - height)*.5
            town["write_accessor"](doc, blocks, attrs["POSITION"], pos)
            town["write_accessor"](doc, blocks, attrs["TEXCOORD_0"], uv)
        else:
            faces &= (north | south).all(axis=1)
        if np.count_nonzero(faces) != (9 if solid else 4):
            raise ValueError(f"The old grade changed in {node['name']}.")
        town["write_accessor"](doc, blocks, stone["indices"], triangles[~faces].reshape(-1, 1))
    town["save_glb"](output, doc, blocks)


def build_stonework(terrain):
    material = town["material"]("HordeTownStone", (.22, .20, .16), 1.)
    points = [(x, y+lift if z == NORTH else y, z) for x, y, lift in GRADE for z in (NORTH, CENTRE, SOUTH)]
    faces = [(i, i+1, i+4, i+3) for section in range(len(GRADE)-1) for i in (3*section, 3*section+1)]
    deck = crossing["mesh"]("River causeway grade", points, faces, material)
    walls = []
    for z, north_side in ((SOUTH, False), (NORTH, True)):
        grade = GRADE[1:] if north_side else GRADE
        top = [(x, y+lift if north_side else y, z) for x, y, lift in grade]
        base = [(x, min(y, terrain(x, z))-.20, z) for x, y, z in top]
        if north_side:
            # The cut face at the footway meets the interpolated old wall.
            base[0] = (JUNCTION, -22.9266, NORTH)
        count = len(top)
        faces = [(i, i+count, i+count+1, i+1) for i in range(count-1)]
        if north_side:
            faces = [tuple(reversed(face)) for face in faces]
        walls.append(crossing["mesh"]("North retaining face" if north_side else "South retaining face",
                                      top+base, faces, material))
    return deck, walls


def export(objects, name, output):
    crossing["join_parts"](objects, name)
    bpy.ops.export_scene.gltf(filepath=str(output), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True,
                             export_animations=False)


def main(root, parent, output):
    source = parent/TOWN_NAME
    terrain = root/town["TERRAIN"]
    if output.exists() or town["sha"](source) != PARENT_SHA256 or town["sha"](terrain) != town["TERRAIN_HASH"]:
        raise ValueError("Use the pinned town, surveyed terrain and a new output folder.")
    output.mkdir(parents=True)
    trimmed, solid, walkable = (output/name for name in ("trimmed.glb", "retaining.glb", "grade.glb"))
    remove_old_grade(source, trimmed)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    deck, walls = build_stonework(ground)
    blend = output/"footway-junction.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend), compress=True)
    export(walls, "BellwetherSolid", solid)
    export([deck], "BellwetherWalkable", walkable)
    interim, runtime = output/"town-with-wall.glb", output/TOWN_NAME
    solid_count, _, _ = town["join_town"](trimmed, solid, interim)
    deck_count, vertices, primitives = town["join_town"](interim, walkable, runtime, node_name="BellwetherWalkable")
    solid_delta, ground_delta = solid_count - 9, deck_count - 4
    if (solid_delta, ground_delta, primitives) != (5, 12, 6):
        raise ValueError("Unexpected junction geometry inventory.")
    manifest = json.loads((parent/"asset.manifest.json").read_bytes())
    manifest["budgets"]["maxVertices"] = vertices
    manifest["budgets"]["maxIndices"] += (solid_delta + ground_delta)*3
    manifest["lods"][0]["maxTriangles"] += solid_delta + ground_delta
    town["save_json"](output/"asset.manifest.json", manifest)
    header = output/"geometry.generated.h"
    header.write_text("#pragma once\n#include <cstddef>\n\n"
        "// Generated by tools/author-bellwether-footway-junction.py.\n"
        "namespace horde::gameplay::bellwether {\n"
        f"inline constexpr std::size_t kFootwayJunctionSolidTriangles={solid_delta};\n"
        f"inline constexpr std::size_t kFootwayJunctionGroundTriangles={ground_delta};\n"
        "}\n", encoding="utf-8", newline="\n")
    town["save_json"](output/"authoring-receipt.json", dict(
        parentTownSha256=PARENT_SHA256, parentTerrainSha256=town["sha"](terrain),
        recipeSha256=town["sha"](RECIPE), blender=bpy.app.version_string, grade=GRADE,
        addedSolidTriangles=solid_delta, addedGroundTriangles=ground_delta,
        files={file.name:town["sha"](file) for file in (blend, header, runtime, output/"asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
