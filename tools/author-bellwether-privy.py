"""Author the closed privy and its washing stand beside Bellwether's tavern.

Blender --background --factory-startup --python-exit-code 1 --python
  tools/author-bellwether-privy.py -- ROOT PREVIOUS_TOWN_GLB NEW_OUTPUT
"""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy


HERE = Path(__file__).resolve()
work = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PARENT = "d744239755677d24ee51af756a0ac0dd1639cdf4555c7ad277281eb4299a921f"
CENTRE = (77., 112.)
YAW = -math.tau/4
BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2), (7, 4, 0, 3), (7, 6, 5, 4)]


def build_privy(ground):
    timber = work["material"]("HordeTavernTimber", (.24,.14,.075), .87)
    stone = work["material"]("HordeTownStone", (.23,.21,.15), .95)
    clay = work["material"]("HordeTownRoofClay", (.26,.12,.065), .85)
    iron = work["material"]("HordeHearthChar", (.073,.055,.04), .80)
    parts = []
    x, z = CENTRE
    sine, cosine = math.sin(YAW), math.cos(YAW)
    floor = ground(x, z)

    def point(dx, level, dz):
        return x+cosine*dx+sine*dz, floor+level, z-sine*dx+cosine*dz

    def mesh(label, points, faces, finish):
        parts.append(crossing["mesh"](label, points, faces, finish))

    def box(label, x0, x1, z0, z1, low, high, finish, fit_footing=False):
        corners = ((x0,z0),(x1,z0),(x1,z1),(x0,z1))
        points = [point(dx, y(dz) if callable(y) else y, dz) for y in (low,high) for dx,dz in corners]
        if fit_footing:
            points[:4] = [(a,min(y,ground(a,c)-.045),c) for a,y,c in points[:4]]
        mesh(label, points, BOX, finish)

    def rim(label, cx, cz, profile, finish, sides):
        rings = [[point(cx+r*math.cos(math.tau*j/sides), y, cz+r*math.sin(math.tau*j/sides))
                  for j in range(sides)] for y,r in profile]
        faces = [(i*sides+a,(i+1)*sides+a,(i+1)*sides+b,i*sides+b)
                 for i in range(len(profile)-1) for a in range(sides) for b in ((a+1)%sides,)]
        faces += [tuple(range(sides)), tuple(reversed(range((len(profile)-1)*sides,len(profile)*sides)))]
        mesh(label, [vertex for ring in rings for vertex in ring], faces, finish)

    def eave(dz):
        return 2.19+.19*dz

    box("Stone sill", -.76,.76,-.67,.72,-.18,.155,stone,True)
    box("Low door step", -.47,.46,.685,.99,-.15,.108,stone,True)
    # The wall lining closes the shaded seams. The door and four walls are solid.
    for side in (-1,1):
        cx = side*.714
        box(f"Side lining {side}", cx-.038,cx+.038,-.647,.657,.13,eave,iron)
        for i in range(7):
            a = -.665+i*.189
            outer = cx+side*(.016+.007*(i%3))
            box(f"Side plank {side}/{i+1}", outer-.032,outer+.032,a,a+.178,.139,eave,timber)
    box("Rear lining", -.70,.70,-.683,-.628,.133,eave,iron)
    for i in range(8):
        a = -.696+i*.174
        box(f"Rear plank {i+1}", a,a+.168,-.702,-.65,.135,eave,timber)
    for lo,hi in ((-.763,-.451),(.451,.763)):
        box(f"Jamb {lo}", lo,hi,.602,.726,.133,eave,timber)
    box("Door head", -.462,.462,.608,.727,1.913,eave,timber)
    box("Closed door core", -.456,.456,.638,.689,.15,1.923,iron)
    for i in range(6):
        a = -.453+i*.151
        box(f"Closed door plank {i+1}", a,a+.147,.683,.724,.154,1.91-.006*(i%2),timber)
    box("Lower strap hinge", -.438,-.11,.718,.753,.503,.548,iron)
    box("Upper strap hinge", -.438,-.12,.718,.753,1.56,1.609,iron)
    box("Latch plate", .295,.425,.719,.772,1.034,1.144,iron)
    box("Latch handle", .215,.379,.757,.801,1.071,1.104,iron)
    for i in range(6):
        a = -.85+i*.275
        box(f"Clay roof course {i+1}", -.927,.927,a,a+.305,
            lambda dz: eave(dz)-.006, lambda dz: eave(dz)+.068,clay)
    box("Rain drip edge", -.932,.932,-.884,-.815,
        lambda dz: eave(dz)-.07, lambda dz: eave(dz)+.032,timber)
    box("Over-door drip board", -.906,.905,.75,.811,
        lambda dz: eave(dz)-.075, lambda dz: eave(dz)+.01,timber)

    # Keep clean water and the small dry shelf off the entry and delivery line.
    for dx in (.91,1.22):
        box(f"Wash stand leg {dx}", dx-.065,dx+.065,.31,.49,-.14,.881,timber,True)
    box("Wash stand top", .823,1.31,.156,.553,.841,.908,timber)
    rim("Open hand-washing basin", 1.054,.35,
        ((.902,.112),(1.003,.219),(1.048,.214),(1.048,.189),(.941,.094)), clay,12)
    jug_x, _, jug_z = point(.992,0.,-.032)
    jug_level = ground(jug_x,jug_z)-floor
    jug_profile = ((-.012,.106),(.074,.142),(.245,.125),(.326,.061),(.353,.064),(.361,.082),(.389,.076))
    rim("Covered carried-water jug", .992,-.032,
        [(jug_level+y,r) for y,r in jug_profile], clay,10)
    box("Jug stopper grip", .967,1.019,-.059,.002,jug_level+.38,jug_level+.42,timber)
    box("Sheltered shelf beside door", -.719,-.493,.715,.872,.949,1.018,timber)
    return parts, point(0.,1.4,.35), point(0.,1.4,.83), (sine,cosine)


def main(root, previous, output):
    terrain = root/work["TERRAIN"]
    if output.exists() or work["sha"](previous) != PARENT or work["sha"](terrain) != work["TERRAIN_HASH"]:
        raise ValueError("Choose a new output with the exact garden town and admitted terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, mouth, listen, front = build_privy(ground)
    output.mkdir(parents=True)
    master = output/"bellwether-privy.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    crossing["join_parts"](parts, "BellwetherClosedPrivy")
    patch = output/"privy.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output/"bellwether-tavern-v1.glb"
    added, vertices, primitives = work["join_town"](previous, patch, town)
    parent, _ = work["read_glb"](previous)
    indices = sum(parent["accessors"][p["indices"]]["count"]
                  for mesh in parent["meshes"] for p in mesh["primitives"]) + 3*added
    if not 0 < added <= 1000 or primitives != 5 or vertices > 22000:
        raise ValueError(f"Privy exceeds town budget: {added} new triangles, {vertices} vertices")
    manifest = json.loads((root/TOWN/"asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices//3
    work["save_json"](output/"asset.manifest.json", manifest)

    def mark(name, coords):
        values = ", ".join(f"{value:.6f}f" for value in coords)
        return f"inline constexpr std::array<float, {len(coords)}> kBellwetherPrivy{name}{{{{{values}}}}};"

    header = ["#pragma once", "#include <array>", "#include <cstddef>", "",
              "// Generated by tools/author-bellwether-privy.py.",
              "namespace horde::gameplay::bellwether {",
              f"inline constexpr std::size_t kBellwetherPrivySolidTriangles={added};",
              mark("Mouth", mouth), mark("ListenPoint", listen), mark("Front", front),
              "} // namespace horde::gameplay::bellwether", ""]
    (output/"geometry.generated.h").write_text("\n".join(header), encoding="utf-8", newline="\n")
    names = (master.name, patch.name, town.name, "asset.manifest.json", "geometry.generated.h")
    work["save_json"](output/"authoring-receipt.json", dict(
        feature="B01 service yard", centre=CENTRE, yawRadians=YAW, mouth=mouth, listenPoint=listen,
        originalTownSha256=PARENT, terrainSha256=work["TERRAIN_HASH"], runtimeTownSha256=work["sha"](town),
        addedSolidTriangles=added, townVertices=vertices, townTriangles=indices//3,
        blender=bpy.app.version_string, recipeSha256=work["sha"](HERE),
        files={name: work["sha"](output/name) for name in names}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--")+1:]
    if len(arguments) != 3:
        raise ValueError("Expected repository ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
