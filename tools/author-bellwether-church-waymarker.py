"""Turn the existing CHURCH waymarker into the fork. Run in Blender with
-- ROOT PARENT_TOWN_GLB NEW_OUTPUT_DIRECTORY.
"""
import math
from pathlib import Path
import runpy
import sys

import bpy
import numpy as np

RECIPE = Path(__file__).resolve()
tools = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
MASTER = Path("assets/models/world/source/bellwether-tavern-v1/bellwether-tavern-v1.blend")
PARENT = "22e26474135dbb4ff95a548015ac9c34c4fd91dfa3e5de493edaf144288e8ad8"
MASTER_HASH = "e821abb54129952512c4a92cf83d87b4cc021c111be505427930e6c257fe3c13"
PARTS = ("B02_waymarker_post", "B02_church_arrow_board", "B02_church_arrow_letters")
PIVOT = (34.65, 82.10)
# From the west the lettering remains broadside enough to read. The arrow
# points into the north-east branch, away from the south-east tavern road.
ANGLE = math.radians(-45)


def turn(x, z, *, pivot=(0., 0.)):
    dx, dz = x-pivot[0], z-pivot[1]
    c, s = math.cos(ANGLE), math.sin(ANGLE)
    return pivot[0] + c*dx + s*dz, pivot[1] - s*dx + c*dz


def main(root, previous, output):
    master = root / MASTER
    if output.exists() or tools["sha"](previous) != PARENT or tools["sha"](master) != MASTER_HASH:
        raise ValueError("Expected the original editable marker, square-frontage town and a fresh output.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    with bpy.data.libraries.load(str(master), link=False) as (_, requested):
        requested.objects = list(PARTS)
    # Match exact authored positions per material. GLB vertices can split at
    # UV seams, so move every matching corner and verify complete triangles.
    keys, counts = {}, {}
    for obj in requested.objects:
        bpy.context.collection.objects.link(obj)
        mesh = obj.data
        finish = mesh.materials[0].name
        positions = keys.setdefault(finish, set())
        mesh.calc_loop_triangles()
        counts[finish] = counts.get(finish, 0) + len(mesh.loop_triangles)
        for v in mesh.vertices:
            x, y, z = v.co
            positions.add((x, z, -y))
            tx, tz = turn(x, -y, pivot=PIVOT)
            v.co = (tx, -tz, z)
        mesh.update()

    doc, blocks = tools["read_glb"](previous)
    node = next(n for n in doc["nodes"] if n["name"] == "BellwetherSolid")
    moved = {}
    for primitive in doc["meshes"][node["mesh"]]["primitives"]:
        finish = doc["materials"][primitive["material"]]["name"]
        if finish not in keys:
            continue
        attrs = primitive["attributes"]
        positions = tools["read_accessor"](doc, blocks, attrs["POSITION"])
        mask = np.array([tuple(p) in keys[finish] for p in positions])
        found = {tuple(p) for p in positions[mask]}
        tri = tools["read_accessor"](doc, blocks, primitive["indices"]).reshape(-1,3)
        corners = mask[tri].sum(axis=1)
        if found != keys[finish] or np.any((corners != 0) & (corners != 3)):
            raise ValueError(f"Incomplete authored marker ownership in {finish}")
        moved[finish] = int(np.count_nonzero(corners == 3))
        for semantic in ("POSITION", "NORMAL", "TANGENT"):
            index = attrs[semantic]
            data = tools["read_accessor"](doc, blocks, index).copy()
            x, z = data[mask,0].astype(float), data[mask,2].astype(float)
            data[mask,0], data[mask,2] = turn(x, z, pivot=PIVOT if semantic == "POSITION" else (0.,0.))
            tools["write_accessor"](doc, blocks, index, data)
    if moved != counts:
        raise ValueError(f"Expected {counts} marker triangles, found {moved}")
    output.mkdir(parents=True)
    editable = output / "church-waymarker.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(editable), compress=True)
    runtime = output / previous.name
    tools["save_glb"](runtime, doc, blocks)
    tools["save_json"](output / "authoring-receipt.json", dict(
        feature="Church lane waymarker", originalTownSha256=PARENT, originalMasterSha256=MASTER_HASH,
        runtimeTownSha256=tools["sha"](runtime), recipeSha256=tools["sha"](RECIPE),
        editableSha256=tools["sha"](editable), blender=bpy.app.version_string,
        pivotXZ=PIVOT, yawDegrees=math.degrees(ANGLE), movedTriangles=moved))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
