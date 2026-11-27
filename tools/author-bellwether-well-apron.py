"""Author a terrain-fitted fetching apron beside Bellwether's covered well.

Blender 5.2:
  blender -b --factory-startup --python-exit-code 1 \
    --python tools/author-bellwether-well-apron.py -- ROOT PARENT_TOWN OUTPUT LAYOUT

The layout and exports are authored separately from the admitted town so a
review can inspect the exact stones before touching runtime pins.
"""
import hashlib
import json
import math
import runpy
import shutil
import sys
from pathlib import Path

import bpy
import numpy as np


HERE = Path(__file__).resolve()
town = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
PARENT = "a41ae7e5ced3961f58f2d9c0a38a88b1ea3eba889503e9e442d0a8c023f754b5"
BASE_COMMIT = "3aa4a3ff8996e06beffe4cbc3eff147597e2d8bd"
TERRAIN_HASH = "f031a850a7e8e661cebe64dadd221e514cf466ac80b9e9ab757b393fecf65a2d"
SURVEY = Path("assets/models/world/source/bellwether-shell-v2/authoring-receipt.json")
PATH_LAYOUT = Path("assets/models/world/source/bellwether-paths-v1/layout.json")
BENCH_RECIPE = Path("tools/author-bellwether-square-frontage.py")
TAVERN_RECIPE = Path("tools/author-bellwether-tavern.py")
WELL_RECEIPT = Path("assets/models/world/source/bellwether-well-v1/authoring-receipt.json")
TAVERN_RECIPE_BASE_SHA256 = "ecc966a6d2f86f55148a5896722a90d7d13858afbc72dc49138b610d81628dfb"
FACE_BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2),
            (7, 4, 0, 3), (7, 6, 5, 4)]
SOURCE_DIR = Path("assets/models/world/source/bellwether-well-apron-v1")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def walkable_surface(doc, blocks):
    node = next(n for n in doc["nodes"] if n["name"] == "BellwetherWalkable")
    triangles = []
    for primitive in doc["meshes"][node["mesh"]]["primitives"]:
        position = town["read_accessor"](doc, blocks, primitive["attributes"]["POSITION"])
        index = town["read_accessor"](doc, blocks, primitive["indices"]).reshape(-1, 3)
        tri = position[index].astype(np.float64)
        normal = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
        length = np.linalg.norm(normal, axis=1)
        upward = (length > 1e-10) & (normal[:, 1] / np.maximum(length, 1e-12) > .75)
        triangles.extend(tri[upward])
    if not triangles:
        raise ValueError("Pinned town has no upward BellwetherWalkable faces")
    return np.asarray(triangles, dtype=np.float64)


def support_y(triangles, x, z):
    """Height of the highest upward walkable face under an XZ sample."""
    a = triangles[:, 0]
    v0x, v0z = triangles[:, 1, 0] - a[:, 0], triangles[:, 1, 2] - a[:, 2]
    v1x, v1z = triangles[:, 2, 0] - a[:, 0], triangles[:, 2, 2] - a[:, 2]
    v2x, v2z = float(x) - a[:, 0], float(z) - a[:, 2]
    denominator = v0x * v1z - v1x * v0z
    nondegenerate = np.abs(denominator) > 1e-12
    u = np.zeros(len(a), dtype=np.float64)
    v = np.zeros(len(a), dtype=np.float64)
    u[nondegenerate] = (v2x[nondegenerate] * v1z[nondegenerate] -
                        v1x[nondegenerate] * v2z[nondegenerate]) / denominator[nondegenerate]
    v[nondegenerate] = (v0x[nondegenerate] * v2z[nondegenerate] -
                        v2x[nondegenerate] * v0z[nondegenerate]) / denominator[nondegenerate]
    hit = nondegenerate & (u >= -1e-7) & (v >= -1e-7) & (u + v <= 1.0000001)
    if not hit.any():
        return None
    tri = triangles[hit]
    uh, vh = u[hit], v[hit]
    y = tri[:, 0, 1] + uh * (tri[:, 1, 1] - tri[:, 0, 1]) + vh * (tri[:, 2, 1] - tri[:, 0, 1])
    return float(y.max())


def outline_for(stone):
    x, z = stone["centre"]
    half_x, half_z = (float(v) / 2 for v in stone["size"])
    if stone["shape"] == "east-west":
        factors = ((-1., -.60), (-.55, -1.), (.55, -1.), (1., -.58),
                   (1., .60), (.53, 1.), (-.53, 1.), (-1., .58))
    elif stone["shape"] == "north-south":
        factors = ((-.60, -1.), (.60, -1.), (1., -.55), (1., .55),
                   (.60, 1.), (-.60, 1.), (-1., .58), (-1., -.55))
    else:
        raise ValueError(f"Unknown slab shape: {stone['shape']}")
    outline = [(x + fx * half_x, z + fz * half_z) for fx, fz in factors]
    area = sum(a[0] * b[1] - a[1] * b[0] for a, b in zip(outline, outline[1:] + outline[:1]))
    # Clockwise in XZ gives +Y normals in Horde's native world coordinates.
    if area > 0:
        outline.reverse()
    return outline


def inside_polygon(x, z, polygon):
    inside = False
    for a, b in zip(polygon, polygon[1:] + polygon[:1]):
        if (a[1] > z) != (b[1] > z):
            crossing = (b[0] - a[0]) * (z - a[1]) / (b[1] - a[1]) + a[0]
            if x < crossing:
                inside = not inside
    return inside


def distance_to_edges(x, z, polygon):
    best = math.inf
    for a, b in zip(polygon, polygon[1:] + polygon[:1]):
        dx, dz = b[0] - a[0], b[1] - a[1]
        length2 = dx * dx + dz * dz
        t = 0. if length2 <= 1e-12 else max(0., min(1., ((x-a[0])*dx+(z-a[1])*dz)/length2))
        best = min(best, math.hypot(x-(a[0]+t*dx), z-(a[1]+t*dz)))
    return best


def build_stone(stone, ground, stone_material, lift, wear, buried_edge, skirt_width):
    outline = outline_for(stone)
    upper = [(x, ground(x, z) + lift, z) for x, z in outline]
    cx, cz = map(float, stone["centre"])
    centre = (cx, ground(cx, cz) + lift - wear, cz)
    lower = []
    for x, _, z in upper:
        dx,dz=x-cx,z-cz
        distance=math.hypot(dx,dz)
        bx,bz=x+skirt_width*dx/distance,z+skirt_width*dz/distance
        lower.append((bx,ground(bx,bz)-buried_edge,bz))
    count=len(outline)
    points=upper+[centre]+lower
    faces=[(count,i,(i+1)%count) for i in range(count)]
    for i in range(count):
        following=(i+1)%count
        # The exposed bevel slopes down and outward. It belongs to the walkable
        # surface and ends below surveyed terrain, leaving no curb wall.
        faces.extend(((following,i,i+count+1),(following,i+count+1,following+count+1)))
    tile=geometry["mesh"](stone["name"] + " worn stone and buried bevel", points, faces, stone_material)
    expected_top = (0., 1., 0.)
    native_normals=[(p.normal.x,p.normal.z,-p.normal.y) for p in tile.data.polygons]
    if any(sum(a*b for a,b in zip(n,expected_top))<.95 for n in native_normals[:count]):
        raise ValueError(f"Upward flagstone winding changed for {stone['name']}: {native_normals[:count]}")
    for i, polygon in enumerate(tile.data.polygons[count:]):
        normal=np.asarray(native_normals[count+i],dtype=np.float64)
        normal = np.asarray((polygon.normal.x, polygon.normal.z, -polygon.normal.y), dtype=np.float64)
        if not np.isfinite(normal).all() or np.linalg.norm(normal) < .99:
            raise ValueError(f"Degenerate slab bevel normal in {stone['name']}")
        if normal[1] <= 0 or np.hypot(normal[0],normal[2])>.65*normal[1]:
            raise ValueError(f"Paver skirt exceeds walkable grade in {stone['name']}: {normal.tolist()}")
        face=faces[count+i]
        centroid=np.mean(np.asarray([points[index] for index in face]),axis=0)
        outward=np.asarray((centroid[0]-cx,0.,centroid[2]-cz))
        if normal[0]*outward[0]+normal[2]*outward[2] <= 0:
            raise ValueError(f"Inward paver bevel winding in {stone['name']}: {normal.tolist()}")
    return tile, len(faces)


def validate_layout(layout, parent_walkable, well_receipt, survey):
    anchors = layout["anchors"]
    if anchors["wellCentre"] != [well_receipt["centre"][0], well_receipt["centre"][2]] or \
       anchors["benchCentre"] != [77.75, 128.15]:
        raise ValueError("Well/bench anchors differ from their authored sources")
    b01 = next(item for item in survey["buildings"] if item["id"] == "B01")
    south = b01["engineZ"] + b01["depth"] / 2
    if abs(anchors["rampToe"][0] - 82.3) > 1e-6 or abs(anchors["rampToe"][1] - (south + 11.5)) > .01:
        raise ValueError("Ramp toe differs from the surveyed B01 ramp")
    west, east = anchors["westLaneContact"], anchors["rampLaneContact"]
    lane, bench = anchors["publicLaneContact"], anchors["benchCentre"]
    for point, name in ((west, "west lane"), (east, "ramp edge"), (lane, "public lane")):
        if support_y(parent_walkable, *point) is None:
            raise ValueError(f"Apron endpoint does not touch existing {name} geometry: {point}")
    if support_y(parent_walkable, *anchors["view"]) is None:
        raise ValueError(f"Combined-view mark is not on the public lane: {anchors['view']}")
    if not any(stone["centre"] == anchors["fetching"] for stone in layout["stones"]):
        raise ValueError("Fetching mark must sit on one of the authored slabs")
    well_view = anchors["existingWellView"]
    if support_y(parent_walkable, *well_view) is not None or not any(
        inside_polygon(*well_view, outline_for(stone)) for stone in layout["stones"]):
        raise ValueError("Fetching apron must bridge the former unsupported covered-well view")
    for point, name in ((bench, "bench"), (anchors["fetching"], "fetching stance"),
                        *((stone["centre"], stone["name"]) for stone in layout["stones"])):
        if support_y(parent_walkable, *point) is not None:
            raise ValueError(f"An apron surface is already paved at {name} {point}; stop before generating")
    # Reject any overlapping walkable ribbon area, while allowing the intended
    # shared seam where a new slab meets the existing path boundary.
    for stone in layout["stones"]:
        polygon = outline_for(stone)
        min_x, max_x = min(p[0] for p in polygon), max(p[0] for p in polygon)
        min_z, max_z = min(p[1] for p in polygon), max(p[1] for p in polygon)
        for ix in np.arange(min_x+.03, max_x-.02, .04):
            for iz in np.arange(min_z+.03, max_z-.02, .04):
                if (inside_polygon(float(ix), float(iz), polygon) and
                    distance_to_edges(float(ix), float(iz), polygon) > .025 and
                    support_y(parent_walkable, float(ix), float(iz)) is not None):
                    raise ValueError(f"{stone['name']} duplicates the admitted lane at {(float(ix),float(iz))}")


def main(root, previous, output, layout_path):
    digest = town["sha"]
    terrain = root / town["TERRAIN"]
    if output.exists() or digest(previous) != PARENT or digest(terrain) != TERRAIN_HASH:
        raise ValueError("Use the pinned town, terrain, and a fresh report output directory")
    layout = json.loads(layout_path.read_bytes())
    survey_path = root / SURVEY
    well_path = root / WELL_RECEIPT
    survey = json.loads(survey_path.read_bytes())
    well_receipt = json.loads(well_path.read_bytes())
    parent_doc, parent_blocks = town["read_glb"](previous)
    parent_walkable = walkable_surface(parent_doc, parent_blocks)
    validate_layout(layout, parent_walkable, well_receipt, survey)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    stone_material = town["material"]("HordeTownStone", (.30, .27, .23), .9)
    walkable_parts = []
    ground_count = 0
    lift=float(layout["topLiftMetres"])
    wear=float(layout["wornCentreInsetMetres"])
    buried_edge=float(layout["buriedEdgeMetres"])
    skirt_width=float(layout["bevelWidthMetres"])
    for name, point in (("west", layout["anchors"]["westLaneContact"]),
                        ("ramp side", layout["anchors"]["rampLaneContact"]),
                        ("public lane", layout["anchors"]["publicLaneContact"]),
                        ("view", layout["anchors"]["view"])):
        lane_y=support_y(parent_walkable, *point)
        if lane_y is None or abs(lane_y-(ground(*point)+lift))>.025:
            raise ValueError(f"{name} seam does not match fitted terrain lift: {point}, {lane_y}")
    for stone in layout["stones"]:
        tile, triangles = build_stone(stone, ground, stone_material, lift, wear,
                                      buried_edge, skirt_width)
        walkable_parts.append(tile)
        ground_count += triangles

    output.mkdir(parents=True)
    master = output / "bellwether-well-apron.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)

    geometry["join_parts"](walkable_parts, "BellwetherWellApronWalkable")
    ground_patch = output / "well-apron-walkable.glb"
    bpy.ops.export_scene.gltf(filepath=str(ground_patch), export_format="GLB", use_selection=True,
                              export_apply=True, export_normals=True, export_tangents=True,
                              export_animations=False)

    runtime = output / previous.name
    added_ground, vertices, primitive_count = town["join_town"](
        previous, ground_patch, runtime, node_name="BellwetherWalkable")
    if added_ground != ground_count or primitive_count != 5 or added_ground > 260:
        raise ValueError(f"Apron exceeds its modest geometry budget: {added_ground} walkable triangles")

    doc, _ = town["read_glb"](runtime)
    indices = sum(doc["accessors"][p["indices"]]["count"]
                  for mesh in doc["meshes"] for p in mesh["primitives"])
    manifest = json.loads((previous.parent / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    manifest_path = output / "asset.manifest.json"
    town["save_json"](manifest_path, manifest)

    header = output / "geometry.generated.h"
    view = layout["anchors"]["view"]
    look_at = layout["anchors"]["lookAt"]
    fetching = layout["anchors"]["fetching"]
    header.write_text(
        "#pragma once\n#include <array>\n#include <cstddef>\n\n"
        "namespace horde::gameplay::bellwether {\n"
        "inline constexpr std::size_t kWellApronSolidTriangles=0;\n"
        f"inline constexpr std::size_t kWellApronGroundTriangles={added_ground};\n"
        f"inline constexpr std::array<float,2> kWellApronView{{{{{view[0]:.6f}f,{view[1]:.6f}f}}}};\n"
        f"inline constexpr std::array<float,2> kWellApronLookAt{{{{{look_at[0]:.6f}f,{look_at[1]:.6f}f}}}};\n"
        f"inline constexpr std::array<float,2> kWellApronFetching{{{{{fetching[0]:.6f}f,{fetching[1]:.6f}f}}}};\n"
        "} // namespace horde::gameplay::bellwether\n", encoding="utf-8", newline="\n")

    output_layout = output / "layout.json"
    shutil.copyfile(layout_path, output_layout)
    recipe_copy = output / HERE.name
    shutil.copyfile(HERE, recipe_copy)
    readme = output / "README.md"
    readme.write_text(
        "# Bellwether well paving\n\n"
        "Eight worn flagstones link the water bench to the public lane at the foot of the south tavern ramp. "
        "Each stone follows the terrain. Its centre dips 10 mm below the rim and its sloping edge meets the ground. "
        "The tops and bevels supply 192 walkable triangles and use the town's stone finish.\n\n"
        "The editable master is `bellwether-well-apron.blend`. The stone outlines and the positions of the well, "
        "bench and ramp are in `layout.json`. The export receipt records the source and runtime hashes.\n\n"
        "To regenerate, extract `bellwether-tavern-v1.glb` and its sibling `asset.manifest.json` from "
        "commit `3aa4a3ff`. From the repository root, run Blender 5.2 with an unused output directory.\n\n"
        "```powershell\n"
        "blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-well-apron.py -- "
        ". <parent-town.glb> <new-output> assets/models/world/source/bellwether-well-apron-v1/layout.json\n"
        "```\n", encoding="utf-8", newline="\n")

    receipt = dict(feature="Covered-well fetching apron", originalTownSha256=PARENT,
                   terrainSha256=digest(terrain), surveySha256=digest(survey_path),
                   wellReceiptSha256=digest(well_path), pathLayoutSha256=digest(root / PATH_LAYOUT),
                   benchRecipeSha256=digest(root / BENCH_RECIPE),
                   baseCommit=BASE_COMMIT,
                   historicalTopLevelRecipeSha256=TAVERN_RECIPE_BASE_SHA256,
                   layoutSha256=digest(layout_path), runtimeTownSha256=digest(runtime),
                   addedSolidTriangles=0, addedGroundTriangles=added_ground,
                   townVertices=vertices, townTriangles=indices // 3,
                   wellCentre=layout["anchors"]["wellCentre"], benchCentre=layout["anchors"]["benchCentre"],
                   publicLaneContact=layout["anchors"]["publicLaneContact"],
                   existingWellView=layout["anchors"]["existingWellView"],
                   westLaneContact=layout["anchors"]["westLaneContact"],
                   rampLaneContact=layout["anchors"]["rampLaneContact"], rampToe=layout["anchors"]["rampToe"],
                   view=layout["anchors"]["view"], lookAt=layout["anchors"]["lookAt"],
                   fetching=layout["anchors"]["fetching"],
                   surfaceLiftMetres=layout["topLiftMetres"], wornCentreInsetMetres=layout["wornCentreInsetMetres"],
                   recipeSha256=digest(HERE), blender=bpy.app.version_string,
                   files={p.name: digest(p) for p in (master, ground_patch, runtime,
                       manifest_path, header, output_layout, recipe_copy, readme)})
    town["save_json"](output / "authoring-receipt.json", receipt)


if __name__ == "__main__":
    args = [Path(arg).resolve() for arg in sys.argv[sys.argv.index("--") + 1:]]
    if len(args) != 4:
        raise ValueError("Expected ROOT, PARENT_TOWN_GLB, fresh OUTPUT, and LAYOUT.json")
    main(*args)
