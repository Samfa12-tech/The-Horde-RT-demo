"""Add the loaded dealer's cart and counter balance to the west lane.
Blender 5.2: -- ROOT PARENT_TOWN_GLB NEW_OUTPUT
"""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
from mathutils import Vector

RECIPE = Path(__file__).resolve()
town = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
PARENT = "f63849a8c080ad3e8ea0457739648d46d02620da4562a41a6f2b864cd9cc2a10"
BOX = [(0,1,2,3),(4,5,1,0),(5,6,2,1),(6,7,3,2),(7,4,0,3),(7,6,5,4)]
CART = (65.12, 129.36)


def loadout(ground):
    oak = town["material"]("HordeTavernTimber", (.27,.16,.085), .84)
    dark = town["material"]("HordeHearthChar", (.06,.05,.04), .80)
    clay = town["material"]("HordeTownRoofClay", (.26,.12,.065), .82)
    pale = town["material"]("HordeTownStone", (.30,.27,.23), .90)
    parts = []
    cx, cz = CART
    track, radius = 1.44, .49

    # Fit a rigid axle to both wheel contacts. Rolling the entire load keeps
    # the spokes, axles and deck in register on the lane's cross slope.
    sine, cosine = 0., 1.
    for _ in range(6):
        left, right = [ground(cx + side*track*.5*cosine + radius*sine,cz) for side in (-1,1)]
        sine = (right-left)/track
        if abs(sine) > .35:
            raise ValueError("The surveyed loading bay is too steep for the cart")
        cosine = math.sqrt(1-sine*sine)
    axle_y = (left+right)*.5 + radius*cosine - .012

    def cart(point):
        x,y,z = point
        return cx+x*cosine-y*sine, axle_y+x*sine+y*cosine, cz+z

    counter_y = ground(60.3,129.2)+.915

    def counter(point):
        x,y,z = point
        return 60.51+x,counter_y+y,128.13+z

    def mesh(name, points, faces, finish, place):
        obj = geometry["mesh"](name,[place(p) for p in points],faces,finish)
        parts.append(obj)
        return obj

    def block(name, x0,x1, y0,y1, z0,z1, finish, place=cart):
        points = [(x,y,z) for y in (y0,y1) for x,z in ((x0,z0),(x1,z0),(x1,z1),(x0,z1))]
        return mesh(name,points,BOX,finish,place)

    def bar(name, a,b, width,depth, finish, place=cart):
        a,b = Vector(a),Vector(b)
        axis = (b-a).normalized()
        ref = Vector((0,1,0)) if abs(axis.y)<.9 else Vector((0,0,1))
        side = axis.cross(ref).normalized()*width*.5
        top = side.normalized().cross(axis)*depth*.5
        corners = [-side-top,side-top,side+top,-side+top]
        mesh(name,[p+offset for p in (a,b) for offset in corners],BOX,finish,place)

    def profile(name, centre, rings, sides, finish, place=cart, *, axle=False, hollow_ring=False):
        x,y,z = centre
        points=[]
        for h,r in rings:
            for i in range(sides):
                angle = i*math.tau/sides
                u,v = r*math.cos(angle),r*math.sin(angle)
                points.append((x+h,y-u,z+v) if axle else (x+u,y+h,z+v))
        faces=[]
        for level in range(len(rings) if hollow_ring else len(rings)-1):
            following=(level+1)%len(rings)
            for i in range(sides):
                j=(i+1)%sides
                faces.append((level*sides+i,following*sides+i,following*sides+j,level*sides+j))
        if not hollow_ring:
            faces += [tuple(range(sides)),tuple(reversed(range(len(points)-sides,len(points))))]
        return mesh(name,points,faces,finish,place)

    # Wheel felloes, shrunk tyres, shouldered hubs and tapered draw shafts.
    block("Axle timber",-.83,.83,-.06,.06,-.06,.06,oak)
    for side in (-1,1):
        x=side*track*.5
        wheel=profile("Iron-tyred oak rim",(x,0,0),
                      [(-.055,.414),(-.055,.476),(-.058,radius),(.058,radius),(.055,.476),(.055,.414)],
                      20,oak,axle=True,hollow_ring=True)
        wheel.data.materials.append(dark)
        for i in range(20,80):
            wheel.data.polygons[i].material_index=1
        profile("Linchpin hub",(x,0,0),[(-.12,.077),(-.08,.105),(.08,.105),(.12,.077)],12,oak,axle=True)
        bar("Iron linchpin",(x+side*.099,-.114,0),(x+side*.099,.115,0),.026,.033,dark)
        for i in range(8):
            a=i*math.tau/8
            bar("Wheel spoke",(x,.084*math.cos(a),.084*math.sin(a)),
                (x,.429*math.cos(a),.429*math.sin(a)),.045,.052,oak)
        bar("Draw shaft",(side*.43,.04,.73),(side*.37,.24,-1.64),.092,.095,oak)
        bar("Worn grip",(side*.37,.24,-1.62),(side*.35,.26,-1.88),.069,.073,oak)
        for z in (-.66,.66):
            block("Side stake",side*.49-.04,side*.49+.04,.14,.64,z-.046,z+.046,oak)
        for y in (.24,.44):
            block("Cart sideboards",side*.53-.028,side*.53+.028,y,y+.15,-.775,.775,oak)
        # Grounded parking legs near the shafts. Their lower ends sit in the soil.
        x=side*.43
        gx,_,gz=cart((x,0,-.80))
        foot=(ground(gx,gz)-axle_y-x*sine-.045)/cosine
        bar("Parking leg",(x,foot,-.80),(x,.14,-.73),.075,.086,oak)
    for i in range(6):
        a=-.54+i*.18
        block("Cart bed plank",a,a+.175,.091,.151,-.77,.77,oak)
    for y in (.19,.405):
        block("Tail board",-.52,.52,y,y+.17,.73,.788,oak)
    # Sealed pottery for transit, with a distinct neck, handles and banded shoulder.
    for x,z,scale in ((-.22,.22,1.),(.22,-.23,.80)):
        rings=[(0,.12),(.06,.165),(.30,.232),(.44,.225),(.46,.227),(.50,.205),
               (.60,.108),(.68,.089),(.695,.115),(.73,.115)]
        jar=profile("Banded trade jar",(x,.151,z),[(h*scale,r*scale) for h,r in rings],14,clay)
        jar.data.materials.append(pale)
        for i in range(3*14,4*14):
            jar.data.polygons[i].material_index=1
        profile("Stopper",(x,.151+.73*scale,z),[(0,.084*scale),(.052*scale,.091*scale)],10,oak)
        for sign in (-1,1):
            grip=[(.095,.646),(.21,.63),(.245,.535),(.21,.456)]
            for (r0,h0),(r1,h1) in zip(grip,grip[1:]):
                bar("Jar handle",(x,.151+h0*scale,z+sign*r0*scale),(x,.151+h1*scale,z+sign*r1*scale),
                    .036*scale,.042*scale,clay)
    # The dealer's existing weight tray sits just ahead of this equal-arm scale.
    block("Balance foot",-.13,.13,0,.065,-.13,.13,pale,counter)
    profile("Balance upright",(0,.06,0),[(0,.063),(.055,.068),(.09,.038),(.57,.029),(.59,.051)],
            10,oak,counter)
    bar("Balance beam",(0,.643,-.31),(0,.647,.31),.047,.035,dark,counter)
    bar("Balance pointer",(0,.637,0),(0,.771,0),.022,.022,pale,counter)
    for side in (-1,1):
        z=side*.29
        profile("Shallow balance pan",(0,.258,z),[(0,.047),(.052,.111),(.065,.126),(.07,.12),(.014,.05)],
                12,clay,counter)
        for i in range(3):
            a=i*math.tau/3
            bar("Pan suspension",(0,.639,z),(.118*math.cos(a),.323,z+.118*math.sin(a)),.012,.014,dark,counter)
    return parts


def main(root, previous, output):
    terrain=root/town["TERRAIN"]
    digest=town["sha"]
    if output.exists() or digest(previous)!=PARENT or digest(terrain)!=town["TERRAIN_HASH"]:
        raise ValueError("Use the pinned church-fork town, its terrain and a fresh output directory")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground=town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts=loadout(ground)
    output.mkdir(parents=True)
    master=output/"west-trade.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master),compress=True)
    geometry["join_parts"](parts,"BellwetherWestTrade")
    patch=output/"west-trade.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch),export_format="GLB",use_selection=True,export_apply=True,
                              export_tangents=True,export_normals=True,export_animations=False)
    runtime=output/previous.name
    added,vertices,primitives=town["join_town"](previous,patch,runtime)
    if not 0<added<=2400 or primitives!=5:
        raise ValueError(f"Unexpected trade furnishings: {added} triangles, {primitives} primitives")
    doc,_=town["read_glb"](runtime)
    indices=sum(doc["accessors"][p["indices"]]["count"] for m in doc["meshes"] for p in m["primitives"])
    manifest=json.loads((previous.parent/"asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices,maxIndices=indices)
    manifest["lods"][0]["maxTriangles"]=indices//3
    town["save_json"](output/"asset.manifest.json",manifest)
    header=output/"geometry.generated.h"
    header.write_text("#pragma once\n#include <cstddef>\n\nnamespace horde::gameplay::bellwether {\n"
                      f"inline constexpr std::size_t kWestTradeSolidTriangles={added};\n"
                      "}\n",encoding="utf-8",newline="\n")
    town["save_json"](output/"authoring-receipt.json",dict(feature="Relic dealer's loading cart and counter balance",
        originalTownSha256=PARENT,terrainSha256=digest(terrain),runtimeTownSha256=digest(runtime),
        addedSolidTriangles=added,townVertices=vertices,townTriangles=indices//3,
        recipeSha256=digest(RECIPE),blender=bpy.app.version_string,
        files={p.name:digest(p) for p in (master,patch,runtime,header,output/"asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
