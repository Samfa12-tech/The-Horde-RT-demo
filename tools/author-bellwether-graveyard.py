"""Layer the mapped T03 churchyard into the admitted Bellwether town.

Run with Blender --background --factory-startup --python-exit-code 1
--python tools/author-bellwether-graveyard.py -- ROOT PARENT_TOWN NEW_OUTPUT.
"""

import json
import math
from pathlib import Path
import random
import runpy
import sys
from tempfile import TemporaryDirectory

import bpy
from mathutils import Vector


HERE = Path(__file__).resolve()
work = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
MARKERS = Path("assets/models/world/runtime/grave-markers-r03")
PARENT = "6dc0cb41da5c670e169220f401bcc050f3b9b6e12bdf71a7e9e727589f4b2a9a"
MARKER_PINS = {
    "round": "2eec59d80f737a90c2e5f22cec020e2fc227ebf7ac40118e136ef7b8ac656ba1",
    "damaged": "b937d5165187b22728901e63b37f72ffc872af9bf521de08cd7edb67f67d3e92",
}
BOUNDS = (89.70, 109.00, 49.80, 71.25)
ENTRANCE = (94.35, 96.85)
APPROACH = (
    (71.7, 75.28), (73.65, 75.35), (85.8, 75.68), (90.0, 74.94),
    (93.8, 73.65), (95.6, 72.52), (95.6, 69.7), (95.6, 62.1), (95.6, 54.7),
)
PLOTS = tuple((x, z) for z in (54.6, 60.8, 67.0) for x in (92.65, 100.15, 105.4))


def path_surface(ground, stone):
    centres = [Vector(station) for station in APPROACH]
    normals = []
    for a, b in zip(centres, centres[1:]):
        dx, dz = (b-a).normalized()
        normals.append(Vector((-dz, dx)))
    offsets = [normals[0]]
    offsets.extend((a+b)/(1+a.dot(b)) for a, b in zip(normals, normals[1:]))
    offsets.append(normals[-1])
    samples = []
    for index, (start, end) in enumerate(zip(centres, centres[1:])):
        intervals = math.ceil((end-start).length / .35)
        for step in range(intervals):
            blend = step/intervals
            samples.append((start.lerp(end,blend), offsets[index].lerp(offsets[index+1],blend)))
    samples.append((centres[-1], offsets[-1]))
    points = []
    for index, (centre, offset) in enumerate(samples):
        # Taper the toes into existing support. The centre follows the survey,
        # and the shoulders sink into it so no raised lip borders the aisle.
        toe = min(1., index / 3., (len(samples)-1-index)/3.)
        half = (1.02+.05*math.sin(index*.29))*max(.65, toe)
        for along in (-1., -.68, -.34, 0., .34, .68, 1.):
            x, z = centre+offset*(half*along)
            crown = (.078 - .056*abs(along)**3)*toe - .021
            points.append((x, ground(x, z)+crown, z))
    stride = 7
    faces = [(row*stride+column, row*stride+column+1, (row+1)*stride+column+1, (row+1)*stride+column)
             for row in range(len(samples)-1) for column in range(stride-1)]
    return crossing["mesh"]("Stone path from the church to the grave rows", points, faces, stone)


def wall_parts(ground, stone, rng):
    left, right, north, south = BOUNDS
    opening_left, opening_right = ENTRANCE
    parts = []

    def wall(label, start, end, fixed, along_x):
        def point(along, across):
            return (along, fixed+across) if along_x else (fixed+across, along)

        for course, (thickness, low, high, offset) in enumerate((
                (.61, -.16, .28, 0.), (.48, .20, .56, .41))):
            count = math.ceil((end-start)/.99)
            step = (end-start)/count
            joints = [start, *(start+(i+offset)*step for i in range(1, count)), end]
            for index, (a, b) in enumerate(zip(joints, joints[1:])):
                a += .007
                b -= .007
                middle = point((a+b)*.5, 0.)
                side = thickness*.5 + rng.uniform(-.021, .021)
                x0,z0 = point(a,-side)
                x1,z1 = point(b,side)
                local = ground(*middle)
                base = min(ground(x0,z0),ground(x0,z1),ground(x1,z0),ground(x1,z1)) if not course else local
                parts.append(work["block"](
                    f"{label} course {course+1} stone {index+1}",
                    x0,x1,z0,z1,base+low,local+high+rng.uniform(-.027,.026),stone,.044))

    wall("West boundary", north, south, left, False)
    wall("East boundary", north, south, right, False)
    wall("North boundary", left, right, north, True)
    wall("South boundary west of entrance", left, opening_left, south, True)
    wall("South boundary east of entrance", opening_right, right, south, True)
    for side, x in (("West", opening_left-.25), ("East", opening_right+.25)):
        foot = ground(x,south)
        for label, width, low, high in (("footing",.74,-.23,.25), ("jamb",.58,.22,1.01), ("cap",.72,.98,1.13)):
            parts.append(work["block"](f"{side} entry {label}", x-width/2,x+width/2,
                south-width/2,south+width/2,foot+low,foot+high,stone,.057))
    return parts


def marker_parts(root, ground, stone, rng):
    meshes = {}
    for variant in MARKER_PINS:
        path = root / MARKERS / f"grave-marker-{variant}.runtime.glb"
        if work["sha"](path) != MARKER_PINS[variant]:
            raise ValueError(f"Unexpected grave marker source {path}")
        before = set(bpy.context.scene.objects)
        bpy.ops.import_scene.gltf(filepath=str(path))
        loaded = set(bpy.context.scene.objects) - before
        models = sorted((obj for obj in loaded if obj.type == "MESH"), key=lambda obj: obj.name)
        new_nodes = [obj.name for obj in loaded if obj.type != "MESH"]
        joined = crossing["join_parts"](models, f"Template {variant}")
        mesh = joined.data.copy()
        mesh.transform(joined.matrix_world)
        mesh.materials.clear()
        mesh.materials.append(stone)
        for face in mesh.polygons:
            face.material_index = 0
        bpy.data.objects.remove(joined, do_unlink=True)
        for name in new_nodes:
            bpy.data.objects.remove(bpy.data.objects[name], do_unlink=True)
        meshes[variant] = mesh

    parts = []
    for index, (x, z) in enumerate(PLOTS):
        variant = "damaged" if index in (1, 5, 6) else "round"
        name = f"Grave {index+1:02d} {variant} marker"
        marker = bpy.data.objects.new(name, meshes[variant])
        bpy.context.collection.objects.link(marker)
        marker.location = (x, -z, ground(x,z)-.055)
        marker.rotation_euler.z = math.pi+rng.uniform(-.064,.064)
        width = rng.uniform(.89,1.08)
        marker.scale = (width, width, rng.uniform(.83,1.09))
        parts.append(marker)
        if index in (0, 4, 7):
            foot_z = z+1.75
            foot = ground(x,foot_z)
            parts.append(work["block"](f"Grave {index+1:02d} short footstone",
                x-.21,x+.22,foot_z-.12,foot_z+.12,foot-.10,foot+.18,stone,.047))
    return parts


def export_parts(parts, name, output):
    crossing["join_parts"](parts, name)
    bpy.ops.export_scene.gltf(filepath=str(output), export_format="GLB", use_selection=True,
        export_apply=True, export_normals=True, export_tangents=True, export_animations=False)


def main(root, previous, output):
    terrain = root / work["TERRAIN"]
    if output.exists() or work["sha"](previous) != PARENT or work["sha"](terrain) != work["TERRAIN_HASH"]:
        raise ValueError("Choose a new output with the pinned clean-rag town and admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    natural = work["terrain_probe"](terrain)
    built = work["terrain_probe"](previous)
    bpy.data.orphans_purge(do_recursive=True)

    def grade(x, z):
        # The first metres meet the buried end of the existing church ramp.
        if x < 73.65:
            try:
                return max(natural(x,z), built(x,z))
            except ValueError:
                pass
        return natural(x,z)

    stone = work["material"]("HordeTownStone", (.24,.23,.20), .94)
    rng = random.Random(180303)
    solid = wall_parts(natural, stone, rng) + marker_parts(root, natural, stone, rng)
    path = path_surface(grade, stone)
    output.mkdir(parents=True)
    master = output / "bellwether-graveyard.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    town = output / "bellwether-tavern-v1.glb"
    with TemporaryDirectory() as temp:
        scratch = Path(temp)
        export_parts(solid, "BellwetherChurchyardSolids", scratch/"walls-and-rows.glb")
        export_parts([path], "BellwetherChurchyardPath", scratch/"path.glb")
        added_solid, _, _ = work["join_town"](previous, scratch/"walls-and-rows.glb", scratch/"with-walls.glb")
        added_ground, vertices, primitives = work["join_town"](
            scratch/"with-walls.glb", scratch/"path.glb", town, node_name="BellwetherWalkable")
    layered, _ = work["read_glb"](town)
    indices = sum(layered["accessors"][p["indices"]]["count"]
                  for mesh in layered["meshes"] for p in mesh["primitives"])
    if not (0 < added_solid <= 7600 and 0 < added_ground <= 1800 and primitives == 5 and vertices <= 43000):
        raise ValueError(f"Churchyard exceeds its bounds: {added_solid=} {added_ground=} {vertices=}")
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices//3
    work["save_json"](output/"asset.manifest.json", manifest)

    def pair_list(points):
        return ",\n    ".join(f"{{{x:.4f}f, {z:.4f}f}}" for x, z in points)

    header = ["#pragma once", "#include <array>", "#include <cstddef>", "",
              "// Generated by tools/author-bellwether-graveyard.py.",
              "namespace horde::gameplay::bellwether {",
              f"inline constexpr std::size_t kBellwetherGraveyardSolidTriangles={added_solid};",
              f"inline constexpr std::size_t kBellwetherGraveyardGroundTriangles={added_ground};",
              f"inline constexpr std::array<std::array<float,2>,{len(APPROACH)}> kBellwetherGraveyardPath{{{{\n    {pair_list(APPROACH)}\n}}}};",
              f"inline constexpr std::array<std::array<float,2>,{len(PLOTS)}> kBellwetherGraveyardMarkers{{{{\n    {pair_list(PLOTS)}\n}}}};",
              "} // namespace horde::gameplay::bellwether", ""]
    (output/"geometry.generated.h").write_text("\n".join(header), encoding="utf-8", newline="\n")
    names = (master.name, town.name, "asset.manifest.json", "geometry.generated.h")
    work["save_json"](output/"authoring-receipt.json", dict(
        feature="T03 active graveyard", mapBoundsEastAndNorth=(11,31,54,78),
        worldBoundsXZ=BOUNDS, entranceX=ENTRANCE, approachXZ=APPROACH, plotsXZ=PLOTS,
        reusedMarkers=MARKER_PINS, originalTownSha256=PARENT, terrainSha256=work["TERRAIN_HASH"],
        runtimeTownSha256=work["sha"](town), addedSolidTriangles=added_solid,
        addedGroundTriangles=added_ground, townVertices=vertices, townTriangles=indices//3,
        blender=bpy.app.version_string, recipeSha256=work["sha"](HERE),
        files={name: work["sha"](output/name) for name in names}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--")+1:]
    if len(arguments) != 3:
        raise ValueError("Expected repository ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
