"""Add the mapped T01 covered well to Bellwether's admitted static town.

Blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-well.py -- ROOT PREVIOUS_TOWN_GLB NEW_OUTPUT
"""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy


HERE = Path(__file__).resolve()
WORKS = HERE.with_name("author-bellwether-mill-works.py")
CROSSING = HERE.with_name("author-bellwether-mill-crossing.py")
work = runpy.run_path(str(WORKS))
crossing = runpy.run_path(str(CROSSING))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PREVIOUS_TOWN = "18674a3693b2122a1a1df95d37a2373eb1ecc28b5f39dd4bccb66d512f197501"
CENTRE = (78.3, 126.2)
VIEW = (76.9, 130.35)
FACE_BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2),
            (7, 4, 0, 3), (7, 6, 5, 4)]


def build_well(ground):
    stone = work["material"]("HordeTownStone", (.25, .22, .17), 1)
    timber = work["material"]("HordeTavernTimber", (.27, .15, .08), .86)
    clay = work["material"]("HordeTownRoofClay", (.25, .105, .055), .84)
    iron = work["material"]("HordeHearthChar", (.065, .047, .033), .78)
    parts = []
    x, z = CENTRE
    floor = ground(x, z)

    def point(dx, level, dz):
        return x + dx, floor + level, z + dz

    def mesh(name, points, faces, material):
        parts.append(crossing["mesh"](name, points, faces, material))

    def block(name, x0, x1, z0, z1, y0, y1, material, bevel=0.):
        parts.append(work["block"](name, x+x0, x+x1, z+z0, z+z1,
                                   floor+y0, floor+y1, material, bevel))

    def ring_course(label, y0, y1, inner, outer, phase, foot_to_ground=False, jointed=True):
        # Continuous cap/plinth sectors omit the two faces buried between
        # adjoining stones and share their position at each seam.
        faces = FACE_BOX if jointed else [FACE_BOX[i] for i in (0, 2, 4, 5)]
        opening = .008 if jointed else 0.
        bearings = [math.tau * (side / 12 + phase) for side in range(12)]
        for side in range(12):
            begin, end = bearings[side]+opening, bearings[(side+1)%12]-opening
            points = []
            for level in (y0, y1):
                for radius, bearing in ((inner, begin), (outer, begin), (outer, end), (inner, end)):
                    p = point(radius*math.cos(bearing), level, radius*math.sin(bearing))
                    if foot_to_ground and level == y0:
                        p = (p[0], min(p[1], ground(p[0], p[2])-.05), p[2])
                    points.append(p)
            mesh(f"{label} stone {side+1}", points, faces, stone)

    def roof_panel(name, x0, x1, z0, z1, near, far, depth, material):
        bottom = [point(x0, near, z0), point(x1, near, z0),
                  point(x1, far, z1), point(x0, far, z1)]
        mesh(name, bottom+[(a,b+depth,c) for a,b,c in bottom], FACE_BOX, material)

    def barrel_profile(name, rings, sides, material, centre, horizontal=False, closed=False):
        points = []
        for level, radius in rings:
            for i in range(sides):
                theta = math.tau * i/sides
                u, v = radius*math.cos(theta), radius*math.sin(theta)
                if horizontal:
                    points.append(point(centre[0]+level, centre[1]+u, centre[2]+v))
                else:
                    points.append(point(centre[0]+u, centre[1]+level, centre[2]+v))
        faces = []
        for ring in range(len(rings)-1):
            for i in range(sides):
                a, b = i, (i+1)%sides
                faces.append((ring*sides+a,(ring+1)*sides+a,(ring+1)*sides+b,ring*sides+b))
        if closed:
            faces.extend((tuple(range(sides)),tuple(reversed(range(len(points)-sides,len(points))))))
        if horizontal:
            faces = [tuple(reversed(face)) for face in faces]
        mesh(name, points, faces, material)

    ring_course("Plinth", -.18, .23, .75, 1.10, 0., True, False)
    ring_course("Lower course", .224, .505, .72, 1.045, .04)
    ring_course("Upper course", .499, .792, .715, 1.026, 0.)
    ring_course("Dressed cap", .785, .938, .68, 1.084, .04, jointed=False)

    # The water opening is covered between draws. The planks sit directly on
    # the cap, with the winch bucket suspended above them.
    half_width = .707
    for board in range(8):
        dz = -half_width + board*(2*half_width/8)
        z1 = dz + 2*half_width/8-.006
        x_limit = math.sqrt(max(.0,.77**2-max(abs(dz),abs(z1))**2))
        block(f"Covered well plank {board+1}", -x_limit,x_limit,dz,z1,.926,.997,timber)
    for dx in (-.53,.53):
        block(f"Cover strap {dx}", dx-.028,dx+.028,-.44,.44,.991,1.014,iron)
    block("Cover pull", -.19,-.032,-.123,-.086,.989,1.055,iron)

    for dx in (-1.,1.):
        bottom = min(ground(x+dx, z-.12),ground(x+dx,z+.12))-floor-.08
        block(f"Post foot {dx}", dx-.16,dx+.16,-.18,.18,bottom,.37,stone)
        block(f"Oak upright {dx}", dx-.102,dx+.102,-.117,.117,.33,2.62,timber)
        block(f"Spindle collar {dx}", dx-.126,dx+.126,-.137,.137,1.95,2.14,iron)
    block("Square roof ridge", -1.24,1.24,-.072,.072,2.57,2.765,timber)
    for i, offset in enumerate((-1.14,0.,1.14)):
        roof_panel(f"North rafter {i}", offset-.05,offset+.05,-1.064,0.,2.21,2.56,.14,timber)
        roof_panel(f"South rafter {i}", offset-.05,offset+.05,0.,1.064,2.56,2.21,.14,timber)
    for side in (-1,1):
        for strip in range(5):
            start = side*(strip*.205)
            end = side*(strip*.205+.257)
            high, low = 2.722-strip*.074, 2.650-strip*.074
            if side<0:
                roof_panel(f"North clay course {strip}", -1.34,1.34,end,start,low,high,.061,clay)
            else:
                roof_panel(f"South clay course {strip}", -1.34,1.34,start,end,high,low,.061,clay)
        block(f"Fascia {side}", -1.36,1.36,side*1.078-.044,side*1.078+.044,2.31,2.425,timber)
    block("Clay ridge cap", -1.40,1.40,-.11,.11,2.742,2.82,clay)

    barrel_profile("Winch axle", ((-1.22,.059),(1.27,.059)), 8,timber,(0.,2.039,0.), True,True)
    barrel_profile("Rope drum", ((-.24,.119),(.20,.119)), 10,timber,(.10,2.039,0.),True,True)
    for turn in range(3):
        start=-.119+turn*.070
        barrel_profile(f"Wound rope {turn}", ((start,.128),(start+.048,.128)),
                       10,iron,(0.,2.039,0.),True,False)
    block("Winch crank", 1.226,1.291,-.032,.032,1.715,2.077,iron)
    block("Wooden crank grip", 1.264,1.432,-.045,.045,1.69,1.777,timber)
    barrel_profile("Dangling rope", ((1.40,.015),(2.015,.015)), 6,iron,(.105,0.,.095),False,True)
    # A turned bucket has an actual hollow, a timber bottom, two metal bands
    # and a simple attached handle. It hangs well below the crossbar.
    bucket = (.105,0.,.105)
    barrel_profile("Hollow bucket", ((1.095,.13),(1.355,.198),(1.355,.167),(1.117,.103)),
                   10,timber,bucket,False,True)
    for level, radius in ((1.135,.146),(1.285,.185)):
        barrel_profile(f"Bucket band {level}", ((level,radius),(level+.030,radius+.006)),
                       10,iron,bucket)
    for side in (-1,1):
        block(f"Bucket handle upright {side}", .105+side*.16-.013,.105+side*.16+.013,.092,.118,
              1.323,1.415,iron)
    block("Bucket handle", -.061,.271,.092,.118,1.395,1.423,iron)
    return parts, (x,floor,z)


def main(root, previous, output):
    terrain = root / work["TERRAIN"]
    if output.exists() or work["sha"](previous) != PREVIOUS_TOWN or work["sha"](terrain) != work["TERRAIN_HASH"]:
        raise ValueError("Choose a new output with the exact road-cache town and admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, centre = build_well(ground)
    output.mkdir(parents=True)
    master = output / "bellwether-well.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    crossing["join_parts"](parts, "BellwetherWell")
    patch = output / "well.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output / "bellwether-tavern-v1.glb"
    added, vertices, primitives = work["join_town"](previous, patch, town)
    original, _ = work["read_glb"](previous)
    indices = sum(original["accessors"][p["indices"]]["count"]
                  for mesh in original["meshes"] for p in mesh["primitives"]) + 3*added
    if not 0 < added <= 1250 or primitives != 5 or vertices > 19000:
        raise ValueError(f"Well exceeds town budget: {added} new triangles, {vertices} vertices")
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices//3
    work["save_json"](output / "asset.manifest.json", manifest)

    def mark(name, position):
        values = ", ".join(f"{value:.6f}f" for value in position)
        return f"inline constexpr std::array<float, {len(position)}> kBellwetherWell{name}{{{{{values}}}}};"

    stage = ["#pragma once", "#include <array>", "#include <cstddef>", "",
             "// Generated by tools/author-bellwether-well.py.",
             "namespace horde::gameplay::bellwether {",
             f"inline constexpr std::size_t kBellwetherWellSolidTriangles={added};",
             mark("Centre", centre), mark("View", VIEW),
             "} // namespace horde::gameplay::bellwether", ""]
    (output / "geometry.generated.h").write_text("\n".join(stage), encoding="utf-8", newline="\n")
    work["save_json"](output / "authoring-receipt.json", dict(
        mapFeature="T01", originalTownSha256=PREVIOUS_TOWN, terrainSha256=work["TERRAIN_HASH"],
        runtimeTownSha256=work["sha"](town), townVertices=vertices, townTriangles=indices//3,
        addedSolidTriangles=added, centre=centre, view=VIEW, roofHeight=2.82,
        recipeSha256=work["sha"](HERE), blender=bpy.app.version_string,
        files={name: work["sha"](output / name) for name in (
            master.name, patch.name, "asset.manifest.json", "geometry.generated.h", town.name)}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1:]
    if len(arguments) != 3:
        raise ValueError("Expected repository ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
