"""Rebuild the hearth table setting and serving dresser from the pinned Bellwether town."""
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
from mathutils import Vector


TOOLS = Path(__file__).resolve().parent
town = runpy.run_path(str(TOOLS / "author-bellwether-mill-works.py"))
geometry = runpy.run_path(str(TOOLS / "author-bellwether-mill-crossing.py"))
PARENT_HASH = "5b679d961594c8b2ce181a979aaca42a018f4699679ff296795b0911ea385269"
TABLE_SOURCE = "assets/models/world/source/bellwether-tavern-v1/authoring-receipt.json"
SHELL_SOURCE = "assets/models/world/source/bellwether-shell-v2/authoring-receipt.json"


def turned(name, base, profile, material, sides=12, depth_scale=1., lean=0., lip=False):
    """Revolve a foot-to-rim-to-inner-floor section in metres, retaining its hollow mouth."""
    points, faces = [], []
    slope = math.radians(lean)
    for level, (height, radius) in enumerate(profile):
        for i in range(sides):
            angle = math.tau * i / sides
            x, z = radius * math.cos(angle), radius * math.sin(angle) * depth_scale
            y = height
            if lip and height >= .28:
                pinch = max(0., -math.sin(angle)) ** 9
                z -= .026 * pinch
                y += .011 * pinch
            points.append((base[0] + x, base[1] + y * math.cos(slope) - z * math.sin(slope),
                           base[2] + y * math.sin(slope) + z * math.cos(slope)))
        if level:
            lo, hi = (level - 1) * sides, level * sides
            for i in range(sides):
                j = (i + 1) % sides
                faces.append((lo + i, hi + i, hi + j, lo + j))
    faces.extend((tuple(range(sides)), tuple(reversed(range(hi, hi + sides)))))
    obj = geometry["mesh"](name, points, faces, material)
    for poly in list(obj.data.polygons)[:-2]:
        poly.use_smooth = True
    return obj


def pitcher_handle(base, material):
    # A clay strap returns into the neck and belly at either end.
    arc = ((.049, .260), (.089, .270), (.140, .244), (.150, .190),
           (.141, .147), (.116, .121), (.092, .118))
    points, faces = [], []
    sides = 6
    for k, (z, y) in enumerate(arc):
        prev, following = arc[max(0, k-1)], arc[min(len(arc)-1, k+1)]
        normal = Vector((0., following[0]-prev[0], prev[1]-following[1])).normalized()
        for i in range(sides):
            angle = math.tau * i / sides
            bulge = .011 * math.sin(angle)
            points.append((base[0] + .017 * math.cos(angle),
                           base[1] + y + bulge * normal.y, base[2] + z + bulge * normal.z))
        if k:
            lo, hi = (k-1)*sides, k*sides
            for i in range(sides):
                j = (i + 1) % sides
                faces.append((lo+i, lo+j, hi+j, hi+i))
    faces.extend((tuple(reversed(range(sides))), tuple(range(hi, hi+sides))))
    obj = geometry["mesh"]("Jug pulled handle", points, faces, material)
    for poly in list(obj.data.polygons)[:-2]:
        poly.use_smooth = True
    return obj


def make_setting(table, floor, north_face):
    oak = town["material"]("HordeTavernTimber", (.25, .115, .065), .84)
    clay = town["material"]("HordeTownRoofClay", (.28, .12, .06), .84)
    x, y, z = table
    jug = (x+.29, y, z-.17)
    beaker = (x-.34, y, z+.16)
    trencher = (x-.13, y, z+.04)
    cup_section = ((0.,.035), (.014,.040), (.103,.054), (.120,.055), (.120,.046), (.017,.029))
    bowl_section = ((0.,.055), (.013,.065), (.033,.118), (.054,.143), (.054,.131), (.019,.060))
    jug_section = ((0.,.056), (.014,.062), (.085,.101), (.146,.106), (.198,.091),
                   (.249,.050), (.286,.054), (.309,.060), (.308,.047), (.251,.039),
                   (.155,.088), (.030,.046))
    parts = [turned("Clay water jug", jug, jug_section, clay, sides=16, lip=True),
             pitcher_handle(jug, clay), turned("Drinking beaker", beaker, cup_section, clay),
             turned("Turned trencher", trencher, bowl_section, oak, depth_scale=.77)]

    cx, back = 84.40, north_face + .012
    front, top = back+.568, floor+.90
    def board(name, x0, x1, z0, z1, low, high, chamfer=0.):
        parts.append(town["block"](name, x0, x1, z0, z1, floor+low, floor+high, oak, chamfer))

    # The freestanding frame bears on four feet. Its plate rack faces the room.
    for side in (-1, 1):
        leg = cx+side*.97
        for leg_z in (back+.088, front-.09):
            board("Dresser upright",leg-.0525,leg+.0525,leg_z-.0525,leg_z+.0525,0.,.835,.008)
        board("Dresser side rail",leg-.043,leg+.043,back+.09,front-.09,.68,.84)
        board("Platter rack standard",leg-.048,leg+.048,back+.015,back+.09,.88,1.49,.006)
    for i in range(3):
        z0=back+i*.19
        board("Serving board",cx-1.05,cx+1.05,z0,z0+.188,.834,.90,.007)
        board("Storage shelf plank",cx-1.,cx+1.,z0+.021,z0+.182,.229,.270,.005)
    for edge in (back+.074,front-.075):
        board("Bench apron",cx-.975,cx+.975,edge-.025,edge+.025,.70,.84)
        board("Shelf stretcher",cx-.975,cx+.975,edge-.022,edge+.022,.178,.23)
    board("Rack crest",cx-1.02,cx+1.02,back+.012,back+.095,1.40,1.50,.012)
    board("Platter back rest",cx-.98,cx+.98,back+.033,back+.125,1.095,1.155,.008)
    board("Platter foot stop",cx-.91,cx+.295,back+.224,back+.255,.90,.948,.005)
    for i,(r,tilt) in enumerate(((.164,72.),(.177,75.),(.151,73.))):
        thickness=.029
        plate=((0.,r*.40),(.013,r*.76),(thickness,r),
               (thickness+.010,r*.91),(.030,r*.82),(.018,r*.80),(.012,r*.39))
        height=top+r*math.sin(math.radians(tilt))-.012
        parts.append(turned(f"Upright serving platter {i+1}",(cx-.70+i*.375,height,back+.15),
                            plate, oak if i==1 else clay, lean=tilt, sides=14))
    for i in range(2):
        parts.append(turned(f"Spare beaker {i+1}",(cx+.55+.24*i,top,back+.37-.10*i),
                            cup_section,oak if i==0 else clay,sides=10))
    jar=((0.,.102),(.065,.149),(.164,.182),(.240,.175),(.331,.102),(.361,.105),
         (.379,.111),(.379,.093),(.326,.085),(.155,.155),(.040,.087))
    parts.append(turned("Storage crock",(cx+.42,floor+.270,back+.30),jar,clay))
    lid=((0.,.092),(.010,.111),(.021,.110),(.031,.079),(.037,.027),(.060,.023))
    parts.append(turned("Crock lid and knob",(cx+.42,floor+.643,back+.30),lid,oak,sides=12))
    marks={"JugBase":jug,"BeakerBase":beaker,"TrencherBase":trencher,
           "DresserTop":(cx,top,back+.30),"DresserFoot":(cx-.97,floor,front-.09)}
    return parts,marks


def main(root, parent, out):
    if out.exists() or town["sha"](parent)!=PARENT_HASH:
        raise ValueError("Supply the town and sibling manifest from dcb1f4fe and a fresh output directory")
    stage=json.loads((root/TABLE_SOURCE).read_bytes())
    shell=json.loads((root/SHELL_SOURCE).read_bytes())
    site=next(b for b in shell["buildings"] if b["id"]=="B01")
    north_face=site["engineZ"]-site["depth"]/2+.42
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts,marks=make_setting(stage["tableTop"],site["actualFloorY"],north_face)
    out.mkdir(parents=True)
    master=out/"hearth-service.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master),compress=True)
    geometry["join_parts"](parts,"Hearth service")
    patch=out/"hearth-service.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch),export_format="GLB",use_selection=True,
                              export_apply=True,export_normals=True,export_tangents=True,export_animations=False)
    runtime=out/"bellwether-tavern-v1.glb"
    added,vertices,primitives=town["join_town"](parent,patch,runtime)
    doc,_=town["read_glb"](runtime)
    triangles=sum(doc["accessors"][p["indices"]]["count"]//3 for m in doc["meshes"] for p in m["primitives"])
    if primitives!=5 or len(doc["materials"])!=4 or triangles-added!=37329:
        raise ValueError("Unexpected parent geometry or material change")
    manifest=json.loads((parent.parent/"asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices,maxIndices=triangles*3)
    manifest["lods"][0]["maxTriangles"]=triangles
    town["save_json"](out/"asset.manifest.json",manifest)
    header=out/"geometry.generated.h"
    vectors="".join("inline constexpr std::array<float,3> kTavern"+name+"{{"+
                    ", ".join(f"{v:.6f}f" for v in p)+"}};\n" for name,p in marks.items())
    header.write_text("#pragma once\n#include <array>\n#include <cstddef>\n\nnamespace horde::gameplay::bellwether {\n"+
                      f"inline constexpr std::size_t kTavernServiceSolidTriangles={added};\n"+vectors+
                      "} // namespace horde::gameplay::bellwether\n",encoding="utf-8",newline="\n")
    digest=town["sha"]
    town["save_json"](out/"authoring-receipt.json",dict(originalTownSha256=PARENT_HASH,runtimeTownSha256=digest(runtime),
        tavernStagingSha256=digest(root/TABLE_SOURCE),shellSurveySha256=digest(root/SHELL_SOURCE),
        addedSolidTriangles=added,townVertices=vertices,townTriangles=triangles,marks=marks,
        blender=bpy.app.version_string,files={p.name:digest(p) for p in (master,patch,header,out/"asset.manifest.json")},
        recipeSha256=digest(Path(__file__))))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--")+1:]))
