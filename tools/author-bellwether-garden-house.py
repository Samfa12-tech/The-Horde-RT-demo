"""Dress House A using the connected village map. Blender 5.2: -- ROOT PARENT_GLB OUTPUT."""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
import numpy as np
from mathutils import Vector

RECIPE = Path(__file__).resolve()
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
PARENT = "b38d8e63d2b227ea88c6ada22462ea8c98523827125e71dcc381447f48eb87c4"
MASTER = "assets/models/world/source/bellwether-shell-v4/bellwether-shell-v4.blend"
MAP = "docs/references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json"
SURVEY = "assets/models/world/source/bellwether-shell-v2/authoring-receipt.json"
INPUTS = {
    MASTER: "a93583bbdcf4e84f90a58dd347ac025a3331b37671cefe47a89db59181802581",
    MAP: "0a4da29bf8d988145c0f7b3f1c7afaf415a5da9b14c34bbd53bd1e00e3a86e59",
    SURVEY: "29b28ad491fd198c58aecd562d058ed1e3a5264fcab0ab4c97de732c3ee89ad1",
}


def relocate_surround(master, doc, blocks, site, entrance, ground):
    """Rotate only the four original north-front masonry pieces onto the mapped east wall."""
    names = ["B03_door_post_-1", "B03_door_post_1", "B03_door_lintel", "B03_front_step"]
    with bpy.data.libraries.load(str(master), link=False) as (_, requested):
        requested.objects = names
    old_x, old_z = site["engineX"], site["engineZ"] - site["depth"] / 2
    ex, ez = entrance
    node = next(n for n in doc["nodes"] if n["name"] == "BellwetherSolid")
    primitive, = [p for p in doc["meshes"][node["mesh"]]["primitives"]
                  if doc["materials"][p["material"]]["name"] == "HordeTownStone"]
    attributes = primitive["attributes"]
    positions = town["read_accessor"](doc, blocks, attributes["POSITION"]).copy()
    normals = town["read_accessor"](doc, blocks, attributes["NORMAL"]).copy()
    tangents = town["read_accessor"](doc, blocks, attributes["TANGENT"]).copy()
    triangles = town["read_accessor"](doc, blocks, primitive["indices"]).reshape(-1, 3)
    keys = {tuple(p) for p in positions}
    moved, seen = 0, set()
    for obj in requested.objects:
        bpy.context.collection.objects.link(obj)
        coords = [(v.co.x, v.co.z, -v.co.y) for v in obj.data.vertices]
        original = set(coords)
        if not original <= keys or seen & original:
            raise ValueError(f"Cannot isolate {obj.name} in the pinned runtime town")
        seen.update(original)
        mask = np.array([tuple(p) in original for p in positions])
        corners = mask[triangles].sum(axis=1)
        obj.data.calc_loop_triangles()
        count = int(np.count_nonzero(corners == 3))
        if np.any((corners != 0) & (corners != 3)) or count != len(obj.data.loop_triangles):
            raise ValueError(f"Partial source ownership for {obj.name}")
        moved += count
        bottom = min(y for _, y, _ in coords)
        new_bottom = min(ground(ex-(z-old_z),ez+(x-old_x)) for x,_,z in coords)-.12
        def transform(x, y, z):
            nx, nz = ex - (z-old_z), ez + (x-old_x)
            if obj.name == "B03_front_step" and y == bottom:
                y = new_bottom
            return nx, y, nz
        for v, p in zip(obj.data.vertices, coords):
            x,y,z = transform(*p)
            v.co = x,-z,y
        obj.data.update()
        positions[mask] = [transform(*map(float, p)) for p in positions[mask]]
        for data in (normals, tangents):
            data[mask,0],data[mask,2] = -data[mask,2],data[mask,0]
    for semantic, data in (("POSITION",positions),("NORMAL",normals),("TANGENT",tangents)):
        town["write_accessor"](doc, blocks, attributes[semantic], data)
    return moved


def household(site, entrance, ground):
    oak = town["material"]("HordeTavernTimber", (.27,.16,.085), .84)
    clay = town["material"]("HordeTownRoofClay", (.26,.12,.065), .82)
    stone = bpy.data.materials["HordeTownStone"]
    iron = town["material"]("HordeHearthChar", (.06,.05,.04), .80)
    parts = []
    def block(name, x0,x1,z0,z1,y0,y1,finish,bevel=0.):
        parts.append(town["block"](name,x0,x1,z0,z1,y0,y1,finish,bevel))
    def mesh(name, pts, faces, finish):
        obj = geometry["mesh"](name,pts,faces,finish)
        parts.append(obj)
        return obj
    def beam(name, start, end, width, depth, finish):
        a,b = Vector(start),Vector(end)
        direction = (b-a).normalized()
        ref = Vector((0,1,0)) if abs(direction.y)<.9 else Vector((0,0,1))
        across = direction.cross(ref).normalized()
        up = across.cross(direction)
        corners = [across*x*width/2 + up*y*depth/2 for x,y in ((-1,-1),(1,-1),(1,1),(-1,1))]
        mesh(name, [p+c for p in (a,b) for c in corners],
             [(0,1,2,3),(4,5,1,0),(5,6,2,1),(6,7,3,2),(7,4,0,3),(7,6,5,4)], finish)
    def vessel(name, x,y,z, rings, sides, finish, hoops=()):
        points = [(x+r*math.cos(i*math.tau/sides), y+h, z+r*math.sin(i*math.tau/sides))
                  for h,r in rings for i in range(sides)]
        faces = []
        for row in range(len(rings)-1):
            for i in range(sides):
                a = row*sides+i
                b = row*sides+(i+1)%sides
                faces.append((a,a+sides,b+sides,b))
        faces += [tuple(range(sides)),tuple(reversed(range(len(points)-sides,len(points))))]
        obj = mesh(name, points, faces, finish)
        if hoops:
            obj.data.materials.append(iron)
            for row in hoops:
                for face in obj.data.polygons[row*sides:(row+1)*sides]:
                    face.material_index=1

    floor = site["actualFloorY"]
    ex, ez = entrance
    sill = floor+.02
    for depth, drop in ((1.24,.21),(1.59,.42)):
        front = ex+depth
        block("Dressed stoop tread",front-.35,front,ez-1.28,ez+1.28,
              min(ground(front-.35,ez),ground(front,ez-1.28),ground(front,ez+1.28))-.08,
              sill-drop,stone,.028)
    for i in range(7):
        z = ez-.88 + i*.252
        block("Closed oak door plank",ex+.013,ex+.081,z,z+.247,sill+.012,floor+2.39,oak,.008)
    for h in (.39,1.79):
        block("Forged strap",ex+.078,ex+.117,ez-.90,ez+.07,floor+h,floor+h+.072,iron)
        block("Hinge knuckle",ex+.088,ex+.157,ez-.94,ez-.85,floor+h-.038,floor+h+.12,iron,.012)
    block("Latch plate",ex+.08,ex+.118,ez+.57,ez+.70,floor+.85,floor+1.09,iron,.012)
    beam("Thumb latch",(ex+.147,floor+.97,ez+.39),(ex+.147,floor+.97,ez+.68),.042,.045,iron)
    for z in (ez-5.55,ez-1.45,ez+1.45,ez+5.55):
        block("Oak wall upright",ex-.035,ex+.105,z-.11,z+.11,floor,floor+3.82,oak,.014)
    block("East wall plate",ex-.035,ex+.125,ez-5.94,ez+5.94,floor+3.61,floor+3.83,oak,.018)
    for side in (-1,1):
        beam("Portal knee",(ex+.055,floor+2.46,ez+side*1.45),
             (ex+.055,floor+3.66,ez+side*2.65),.16,.158,oak)
    # A latched pair of shutters and a sloped drip cap face the garden.
    wall = site["engineZ"] + site["depth"]/2
    sx = 39.8
    block("Window surround",sx-1.02,sx+1.02,wall-.02,wall+.107,floor+.86,floor+2.39,stone,.035)
    for i in range(8):
        x = sx-.88+i*.22
        block("Shutter boards",x,x+.211,wall+.105,wall+.154,floor+.99,floor+2.26,oak,.008)
    for h in (1.13,2.02):
        for side in (-1,1):
            x = sx + side*.45
            block("Shutter battens",x-.422,x+.422,wall+.151,wall+.212,floor+h,floor+h+.10,oak)
    for side in (-1,1):
        beam("Shutter diagonal",(sx+side*.78,floor+1.24,wall+.19),
             (sx+side*.13,floor+2.015,wall+.19),.08,.07,oak)
    block("Shutter latch",sx-.19,sx+.22,wall+.21,wall+.246,floor+1.67,floor+1.72,iron)
    block("Projecting stone sill",sx-1.12,sx+1.12,wall-.03,wall+.30,floor+.80,floor+.94,stone,.03)

    # A clay-tiled lean-to shelters the household washing bench. Runoff falls
    # through an open timber gutter and a projecting lip into the rain butt.
    left,right = 31.88,37.18
    eave = wall+1.91
    def roof(z):
        return floor+2.34-.36*(z-wall)
    for x in (left+.21,right-.20):
        base = ground(x,eave-.23)
        block("Stone post foot",x-.13,x+.13,eave-.36,eave-.10,base-.06,base+.15,stone,.024)
        block("Shelter post",x-.085,x+.085,eave-.315,eave-.145,base+.12,roof(eave)-.045,oak,.011)
        beam("Knee brace",(x,roof(eave)-.64,eave-.23),(x,roof(eave)+.09,eave-.93),.105,.115,oak)
    block("Shelter eave plate",left,right,eave-.33,eave-.16,roof(eave)-.14,roof(eave)+.036,oak)
    for i in range(6):
        x = left+.16+(right-left-.32)*i/5
        beam("Exposed rafter",(x,roof(wall)-.024,wall-.04),(x,roof(eave)-.024,eave),.115,.13,oak)
    box_faces=[(0,1,2,3),(4,5,1,0),(5,6,2,1),(6,7,3,2),(7,4,0,3),(7,6,5,4)]
    for row in range(5):
        z0,z1 = wall-.045+.385*row, wall+.415+.385*row
        start,end=left-.11,right+.11
        edges=[start]+[start+.46*i+(.22 if row%2 else 0) for i in range(1,12)]+[end]
        for a,b in zip(edges,edges[1:]):
            y0,y1=roof(z0)+.085,roof(z1)+.11
            plate=[(a+.004,y0,z0),(b-.004,y0,z0),(b-.004,y1,z1),(a+.004,y1,z1)]
            mesh("Overlapping clay pans",plate+[(x,y+.052,z) for x,y,z in plate],box_faces,clay)
    lip_x = right+.56
    lip_z = eave+.13
    def gutter_y(x):
        return roof(eave)-.108-(x-left)*.014
    # Three pitched boards keep the water course visible from underneath.
    for dz,dy,width,height in ((0.,0.,.175,.033),(-.082,.055,.029,.105),(.082,.055,.029,.105)):
        beam("Open timber gutter",(left-.13,gutter_y(left-.13)+dy,lip_z+dz),
             (lip_x,gutter_y(lip_x)+dy,lip_z+dz),width,height,oak)
    for x in (left+.23,(left+right)/2,right-.18):
        beam("Gutter bracket",(x,roof(eave)-.095,eave-.19),(x,gutter_y(x)-.025,lip_z+.10),.072,.067,oak)
    bottom=ground(lip_x,lip_z)-.045
    vessel("Iron-bound rain butt",lip_x,bottom,lip_z,
           [(0,.38),(.10,.405),(.13,.428),(.21,.432),(.23,.417),(.62,.468),(.90,.457),
            (1.01,.44),(1.035,.455),(1.105,.445),(1.13,.421),(1.21,.41),(1.21,.371),(.11,.34)],
           20,oak,(2,8))
    # A bench with a broad open wash bowl, and a smaller mug on the draining end.
    bx,bz = 34.47,wall+.71
    top=ground(bx,bz)+.84
    for dx in (-1.02,1.02):
        for dz in (-.265,.265):
            x,z=bx+dx,bz+dz
            block("Bench leg",x-.052,x+.052,z-.052,z+.052,ground(x,z)-.05,top-.052,oak,.009)
        block("Bench end rail",bx+dx-.065,bx+dx+.065,bz-.32,bz+.32,top-.22,top-.069,oak)
    for i in range(4):
        z=bz-.36+i*.18
        block("Draining bench plank",bx-1.19,bx+1.19,z,z+.174,top-.07,top,oak,.008)
    vessel("Glazed washing basin",bx-.33,top,bz,[(0,.19),(.045,.23),(.19,.355),(.23,.37),
           (.25,.362),(.245,.33),(.07,.19)],18,clay)
    vessel("Dipping cup",bx+.46,top,bz+.02,[(0,.065),(.03,.083),(.19,.095),(.192,.081),(.022,.061)],12,clay)
    return parts


def main(root, previous, output):
    digest=town["sha"]
    if output.exists() or digest(previous)!=PARENT or any(digest(root/p)!=h for p,h in INPUTS.items()):
        raise ValueError("Use the pinned town, survey, connected map, v4 master and a fresh output directory")
    terrain=root/town["TERRAIN"]
    if digest(terrain)!=town["TERRAIN_HASH"]:
        raise ValueError("Town terrain differs from the registered input")
    mapped=next(b for b in json.loads((root/MAP).read_bytes())["buildings"] if b["id"]=="B03")
    site=next(b for b in json.loads((root/SURVEY).read_bytes())["buildings"] if b["id"]=="B03")
    entrance=(site["engineX"]+mapped["door_x"]-mapped["x"],site["engineZ"]-mapped["door_y"]+mapped["y"])
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground=town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    doc,blocks=town["read_glb"](previous)
    moved=relocate_surround(root/MASTER,doc,blocks,site,entrance,ground)
    parts=household(site,entrance,ground)
    output.mkdir(parents=True)
    master=output/"garden-house.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master),compress=True)
    geometry["join_parts"](parts,"BellwetherGardenHouse")
    patch=output/"garden-house.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch),export_format="GLB",use_selection=True,export_apply=True,
                              export_tangents=True,export_normals=True,export_animations=False)
    aligned=output/"east-surround.glb"
    town["save_glb"](aligned,doc,blocks)
    runtime=output/previous.name
    added,vertices,primitives=town["join_town"](aligned,patch,runtime)
    if moved!=48 or not 0<added<=3200 or primitives!=5:
        raise ValueError(f"Unexpected house ownership: {moved} relocated, {added} added, {primitives} primitives")
    manifest=json.loads((previous.parent/"asset.manifest.json").read_bytes())
    indices=manifest["budgets"]["maxIndices"]+added*3
    manifest["budgets"].update(maxVertices=vertices,maxIndices=indices)
    manifest["lods"][0]["maxTriangles"]=indices//3
    town["save_json"](output/"asset.manifest.json",manifest)
    header=output/"geometry.generated.h"
    header.write_text("#pragma once\n#include <cstddef>\n\nnamespace horde::gameplay::bellwether {\n"
                      f"inline constexpr std::size_t kGardenHouseSolidTriangles={added};\n}}\n",
                      encoding="utf-8",newline="\n")
    town["save_json"](output/"authoring-receipt.json",dict(feature="Garden-side household and mapped east door",
        originalTownSha256=PARENT,terrainSha256=digest(terrain),sources=INPUTS,runtimeTownSha256=digest(runtime),
        eastEntranceXZ=entrance,relocatedMasonryTriangles=moved,addedSolidTriangles=added,
        townVertices=vertices,townTriangles=indices//3,recipeSha256=digest(RECIPE),blender=bpy.app.version_string,
        files={p.name:digest(p) for p in (master,patch,runtime,header,output/"asset.manifest.json")}))


if __name__=="__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
