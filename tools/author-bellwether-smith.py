"""Author B06's exterior smith yard over the registered Bellwether terrain.

Blender --background --factory-startup --python-exit-code 1
  --python tools/author-bellwether-smith.py -- ROOT PARENT_TOWN NEW_OUTPUT
"""
import json
import math
from pathlib import Path
import random
import runpy
import sys
from tempfile import TemporaryDirectory

import bmesh
import bpy

HERE = Path(__file__).resolve()
work = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
crossing = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))
PARENT = "60d21c865290ce170a5e812d6944b3c35d50ba9a57463668ae9b606f73f6e10e"
PARENT_MANIFEST = "a6b1d42b9df9edc5d8ed1018a54b10a9bfba2146918cabb308bad5dcd988b774"
APPROACH = ((82.30, 131.70), (86.80, 138.50), (89.70, 145.00),
            (91.15, 150.00), (93.15, 151.64), (94.72, 152.03))
DOOR_APPROACH = ((94.66, 154.05), (96.65, 155.20))
ANVIL = (95.62, 151.23)
DOOR = (98.30, 155.20)
PAD = (92.97, 98.27, 148.95, 153.88)
FOOTING_DEPTH = 0.23


def pavers(ground, stone):
    left, right, north, south = PAD
    nx, nz = 15, 14
    rng = random.Random(180606)
    points = []
    for j in range(nz + 1):
        for i in range(nx + 1):
            x = left + (right-left)*i/nx + rng.uniform(-.10,.10)
            z = north + (south-north)*j/nz + rng.uniform(-.10,.10)
            # Seat the boundary in the original ground and keep the seam flush.
            blend = min(i, j, nx-i, nz-j, 2)/2
            y = ground(x,z) + blend*(.053+rng.uniform(-.009,.009)) - .016
            points.append((x, y, z))
    faces = [(j*(nx+1)+i, (j+1)*(nx+1)+i, (j+1)*(nx+1)+i+1, j*(nx+1)+i+1)
             for j in range(nz) for i in range(nx)]
    return crossing["mesh"]("Flush stone smithing hardstand", points, faces, stone)


def shelter(ground, stone, oak, clay):
    def roof(x):
        return -10.23 + (x-93.72)*.232

    block = work["block"]

    parts = []
    for z in (149.23, 153.33):
        x = 94.02
        base = min(ground(x+dx,z+dz) for dx in (-.20,.20) for dz in (-.20,.20))
        top = max(ground(x+dx,z+dz) for dx in (-.20,.20) for dz in (-.20,.20))+.22
        parts += [
            block("Dressed stone post shoe",x-.22,x+.22,z-.22,z+.22,base-FOOTING_DEPTH,top,stone,.034),
            block("Hewn canopy upright",x-.13,x+.13,z-.13,z+.13,top-.035,roof(x)-.13,oak,.020),
        ]
        z1 = z+(.65 if z < 151 else -.65)
        brace = [(xx,y,zz) for xx in (x-.065,x+.065) for y,zz in (
            (roof(x)-.86,z), (roof(x)-.23,z1), (roof(x)-.08,z1), (roof(x)-.66,z))]
        parts.append(crossing["mesh"]("Pegged canopy knee brace",brace,
                     [(3,2,1,0),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],oak))
    parts += [
        block("West eave beam",93.89,94.17,148.88,153.68,roof(94.02)-.27,roof(94.02)-.055,oak,.030),
        block("Ledger against smith wall",98.13,98.33,148.88,153.68,roof(98.23)-.25,roof(98.23)-.08,oak,.018),
    ]

    def sloped(name,x0,x1,z0,z1,height,thickness,finish):
        points = [(x, roof(x)+height+lift, z) for lift in (-thickness, 0.) for x,z in (
            (x0,z0), (x0,z1), (x1,z1), (x1,z0))]
        return crossing["mesh"](name,points,[(3,2,1,0),(4,5,6,7),(0,1,5,4),
                     (1,2,6,5),(2,3,7,6),(3,0,4,7)],finish)

    for z in (148.91, 149.71, 150.51, 151.31, 152.11, 152.91, 153.61):
        parts.append(sloped("Exposed rafter",93.63,98.31,z-.067,z+.067,-.065,.16,oak))
    parts.append(sloped("Continuous roof boarding",93.61,98.32,148.76,153.77,-.022,.078,oak))
    rng = random.Random(606)
    for i in range(9):
        x0 = 93.61+i*.522
        x1 = min(98.32, x0+.586)
        z0 = 148.76
        while z0 < 153.76:
            width = rng.uniform(.48,.66) if z0 > 148.77 or i%2==0 else .24
            z1 = min(z0+width,153.77)
            parts.append(sloped("Overlapping clay shingle",x0,x1,z0,z1,.014+i*.003,.029,clay))
            z0 = z1+.009
    return parts


def workshop(ground, stone, oak, iron):
    parts = []

    def prop(name,x0,x1,z0,z1,y0,y1,finish,bevel=0.):
        parts.append(work["block"](name,x0,x1,z0,z1,y0,y1,finish,bevel))

    def pedestal(x, z, dimensions, levels, name, finish):
        points = [(x+rx*math.cos(math.tau*i/10),y,z+rz*math.sin(math.tau*i/10))
                  for (rx,rz),y in zip(dimensions,levels) for i in range(10)]
        faces = [tuple(range(9,-1,-1)), tuple(range(len(points)-10,len(points)))]
        for ring in range(len(levels)-1):
            for i in range(10):
                a, b = ring*10+i, ring*10+(i+1)%10
                faces.append((a,b,b+10,a+10))
        # The polygon ring is clockwise after conversion to Blender's axes.
        parts.append(crossing["mesh"](name,points,[tuple(reversed(face)) for face in faces],finish))

    # The firebox reads from the west. The raised hearth and three solid sides
    # remain connected to the work surface around the open forge mouth.
    y = max(ground(x,z) for x in (96.44,98.23) for z in (149.19,150.67))+.045
    prop("Forge masonry plinth",96.44,98.27,149.17,150.75,y-.27,y+.67,stone,.093)
    prop("Worn hearth sill",96.33,98.27,149.11,150.81,y+.62,y+.84,stone,.047)
    prop("Forge rear wall",97.86,98.27,149.20,150.73,y+.82,y+2.36,stone,.05)
    for z0,z1 in ((149.15,149.51),(150.40,150.77)):
        prop("Dry stone firebox jamb",96.65,98.27,z0,z1,y+.82,y+1.57,stone,.053)
    prop("Hearth backplate",97.73,97.88,149.50,150.40,y+.82,y+1.56,iron,.025)
    prop("Unlit bed of charcoal",96.76,97.78,149.55,150.35,y+.82,y+.92,iron,.034)
    prop("Dressed stone forge lintel",96.59,98.27,149.11,150.81,y+1.56,y+1.80,stone,.048)
    prop("Stepped forge hood",96.85,98.27,149.25,150.67,y+1.77,y+2.12,stone,.065)
    prop("Forge hood flue shoulder",97.19,98.27,149.37,150.55,y+2.08,y+2.44,stone,.064)
    prop("Rear smoke throat",97.70,98.27,149.61,150.34,y+2.38,y+3.57,stone,.067)

    x,z = ANVIL
    y = ground(x,z)+.045
    pedestal(x,z,((.37,.28),(.34,.27),(.35,.275)),(y-.16,y+.52,y+.66),"End-grain anvil stump",oak)
    prop("Spread iron anvil feet",x-.31,x+.31,z-.22,z+.22,y+.65,y+.76,iron,.042)
    prop("Forged anvil waist",x-.18,x+.17,z-.135,z+.135,y+.72,y+.96,iron,.040)
    prop("Worn flat striking surface",x-.30,x+.37,z-.19,z+.19,y+.91,y+1.035,iron,.031)
    horn = [(x+dx,y+dy,z+dz) for dx,dy,dz in ((-.25,.92,-.17),(-.25,.92,.17),(-.25,1.02,-.17),
            (-.25,1.02,.17),(-.65,.985,-.03),(-.65,.985,.03))]
    parts.append(crossing["mesh"]("Tapering anvil horn",horn,
                 [(0,2,3,1),(0,1,5,4),(0,4,2),(1,3,5),(2,4,5,3)],iron))

    y = max(ground(x,z) for x in (97.08,97.91) for z in (151.52,153.26))+.045
    for x in (97.13,97.83):
        for z in (151.54,153.27):
            prop("Workbench leg",x-.083,x+.083,z-.083,z+.083,
                 ground(x,z)-.13,y+.81,oak,.016)
    prop("Pinned heavy worktop",96.97,98.01,151.34,153.40,y+.77,y+.90,oak,.042)
    prop("Workbench stretcher",97.73,97.93,151.49,153.30,y+.25,y+.36,oak,.020)
    for z in (151.63,151.75,151.87):
        prop("Unworked iron stock",97.16,97.87,z-.025,z+.025,y+.90,y+.948,iron,.007)
    prop("Hammer haft",97.22,97.60,152.54,152.60,y+.90,y+.951,oak,.011)
    prop("Hammer face",97.55,97.67,152.44,152.70,y+.90,y+1.025,iron,.020)

    x,z = 95.84,153.29
    y = ground(x,z)+.044
    # A hollow, empty quench tub: raised stave rim and a visible inner bottom.
    rings = ((.34,.30,y-.12),(.40,.34,y+.14),(.35,.30,y+.56),
             (.29,.25,y+.56),(.314,.264,y+.14))
    points = [(x+rx*math.cos(math.tau*i/12),height,z+rz*math.sin(math.tau*i/12))
              for rx,rz,height in rings for i in range(12)]
    faces = [tuple(range(12)),tuple(reversed(range(48,60)))]
    for ring in range(4):
        for i in range(12):
            a,b=12*ring+i,12*ring+(i+1)%12
            faces.append((a,a+12,b+12,b))
    parts.append(crossing["mesh"]("Open quench tub",points,faces,oak))
    for height in (.04,.37):
        rx,rz = ((.395,.337) if height < .1 else (.392,.333))
        band = [(x+radius*math.cos(math.tau*i/12),y+height+lift,
                 z+depth*math.sin(math.tau*i/12)) for radius,depth,lift in (
                 (rx,rz,0.),(rx,rz,.047),(rx-.025,rz-.025,.047),(rx-.025,rz-.025,0.)) for i in range(12)]
        band_faces = []
        for ring in range(4):
            for i in range(12):
                a,b=12*ring+i,12*ring+(i+1)%12
                band_faces.append((a,12*((ring+1)%4)+i,12*((ring+1)%4)+(i+1)%12,b))
        parts.append(crossing["mesh"]("Riveted quench-tub hoop",band,band_faces,iron))
    return parts


def wick(ground, iron, clay):
    x,z = 94.02,152.96
    y = ground(94.02,153.33)+1.64
    block = work["block"]
    parts = [
        block("Wick bracket mounting plate",x-.071,x+.071,153.175,153.213,y-.22,y+.03,iron,.014),
        block("Wick bracket arm",x-.039,x+.039,z-.031,153.19,y-.06,y+.002,iron,.014),
        block("Ceramic wick bowl",x-.112,x+.112,z-.103,z+.103,y-.015,y+.076,clay,.033),
    ]
    return parts, (x,y+.09,z), (x+.045,y+.24,z+.035)


def main(root, previous, output):
    terrain = root / work["TERRAIN"]
    previous_manifest = previous.with_name("asset.manifest.json")
    if (output.exists() or work["sha"](previous)!=PARENT or
            work["sha"](previous_manifest)!=PARENT_MANIFEST or work["sha"](terrain)!=work["TERRAIN_HASH"]):
        raise ValueError("Choose a fresh output and the pinned churchyard town, manifest and terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = work["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    material = work["material"]
    stone = material("HordeTownStone",(.23,.20,.18),.94)
    oak = material("HordeTavernTimber",(.22,.105,.064),.86)
    iron = material("HordeHearthChar",(.084,.075,.067),.93)
    clay = material("HordeTownRoofClay",(.22,.12,.084),.95)
    mounting, flame, light = wick(ground,iron,clay)
    solids = shelter(ground,stone,oak,clay)+workshop(ground,stone,oak,iron)+mounting
    for solid in solids:
        mesh = bmesh.new()
        mesh.from_mesh(solid.data)
        bmesh.ops.recalc_face_normals(mesh, faces=list(mesh.faces))
        if any(not edge.is_manifold for edge in mesh.edges) or mesh.calc_volume(signed=True) <= 0.:
            raise ValueError(f"Expected an outward-wound closed solid: {solid.name}")
        mesh.to_mesh(solid.data)
        mesh.free()
    pad = pavers(ground,stone)
    output.mkdir(parents=True)
    master = output / "bellwether-smith.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master),compress=True)
    town = output / "bellwether-tavern-v1.glb"
    with TemporaryDirectory() as temp:
        scratch = Path(temp)

        def export(parts, name, path):
            crossing["join_parts"](parts,name)
            bpy.ops.export_scene.gltf(filepath=str(path),export_format="GLB",use_selection=True,
                export_apply=True,export_normals=True,export_tangents=True,export_animations=False)

        export(solids,"BellwetherSmithSolids",scratch/"smith.glb")
        export([pad],"BellwetherSmithHardstand",scratch/"hardstand.glb")
        added_solid, _, _ = work["join_town"](previous,scratch/"smith.glb",scratch/"with-smith.glb")
        added_ground,vertices,primitives = work["join_town"](
            scratch/"with-smith.glb",scratch/"hardstand.glb",town,node_name="BellwetherWalkable")
    doc,_=work["read_glb"](town)
    indices = sum(doc["accessors"][p["indices"]]["count"] for mesh in doc["meshes"] for p in mesh["primitives"])
    if not (0 < added_solid <= 2500 and added_ground==420 and vertices <= 49000 and primitives==5):
        raise ValueError(f"Smith mesh exceeds its bounds: {added_solid=} {added_ground=} {vertices=}")
    manifest = json.loads(previous_manifest.read_bytes())
    manifest["budgets"].update(maxVertices=vertices,maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices//3
    work["save_json"](output/"asset.manifest.json",manifest)

    def pairs(points):
        return ", ".join(f"{{{x:.4f}f, {z:.4f}f}}" for x,z in points)

    header = ["#pragma once","#include <array>","#include <cstddef>","",
              "// Generated by tools/author-bellwether-smith.py.",
              "namespace horde::gameplay::bellwether {",
              f"inline constexpr std::size_t kBellwetherSmithSolidTriangles={added_solid};",
              f"inline constexpr std::size_t kBellwetherSmithGroundTriangles={added_ground};",
              f"inline constexpr std::array<std::array<float,2>,{len(APPROACH)}> kBellwetherSmithApproachXZ{{{{{pairs(APPROACH)}}}}};",
              f"inline constexpr std::array<std::array<float,2>,{len(DOOR_APPROACH)}> kBellwetherSmithDoorApproachXZ{{{{{pairs(DOOR_APPROACH)}}}}};",
              f"inline constexpr std::array<float,2> kBellwetherSmithAnvilXZ{pairs((ANVIL,))};",
              "inline constexpr std::array<float,3> kBellwetherSmithLampFlame{" +
                  ", ".join(f"{v:.4f}f" for v in flame) + "};",
              "inline constexpr std::array<float,3> kBellwetherSmithLampLight{" +
                  ", ".join(f"{v:.4f}f" for v in light) + "};",
              "} // namespace horde::gameplay::bellwether",""]
    (output/"geometry.generated.h").write_text("\n".join(header),encoding="utf-8",newline="\n")
    names = (master.name,town.name,"asset.manifest.json","geometry.generated.h")
    work["save_json"](output/"authoring-receipt.json",dict(
        feature="B06 exterior smith work yard", doorXZ=DOOR, approachXZ=APPROACH,
        doorApproachXZ=DOOR_APPROACH, anvilXZ=ANVIL, hardstandBoundsXZ=PAD,
        lampFlame=flame, lampLight=light,
        originalTownSha256=PARENT,originalManifestSha256=PARENT_MANIFEST,
        terrainSha256=work["TERRAIN_HASH"],runtimeTownSha256=work["sha"](town),
        addedSolidTriangles=added_solid,addedGroundTriangles=added_ground,townVertices=vertices,
        townTriangles=indices//3,blender=bpy.app.version_string,recipeSha256=work["sha"](HERE),
        files={name:work["sha"](output/name) for name in names}))


if __name__=="__main__":
    args=sys.argv[sys.argv.index("--")+1:]
    if len(args)!=3:
        raise ValueError("Expected ROOT, PARENT_TOWN and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in args))
