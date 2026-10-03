"""Editable, CPU-only collapse Form review; deliberately has no runtime export.

Run Blender --background --threads 2 --python this_file -- --source <gltf>
    --approved <docs/design/1.6.2-collapse> --output <private/source-art/final>
Original assets and approved studies are read-only. Horde coordinates are X/Yup/Z;
Blender coordinates are X/-Z/Yup. Inspection illumination is never baked or shipped.
"""
import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--approved", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--views", default="all", help="Comma-separated view names or all/none")
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
args.source = args.source.resolve()
args.approved = args.approved.resolve()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
repo = Path(__file__).resolve().parent.parent
approval = json.loads((args.approved / "milestone-reviews.json").read_text())
if approval["function"]["status"] != "approved":
    raise RuntimeError("Explicit approved Function review is required before composition")
receipt = json.loads((args.source.parent / "download-receipt.json").read_text())
for item in receipt:
    path = args.source.parent / item["path"]
    if path.stat().st_size != item["bytes"] or hashlib.sha256(path.read_bytes()).hexdigest() != item["sha256"]:
        raise RuntimeError("Preserved source receipt mismatch: " + item["path"])

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 20
scene.cycles.use_denoising = True
scene.render.threads_mode = "FIXED"
scene.render.threads = 2
scene.render.resolution_x = 960
scene.render.resolution_y = 540
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False
scene.view_settings.view_transform = "AgX"
scene.world = bpy.data.worlds.new("Neutral inspection world - not runtime lighting")
scene.world.use_nodes = True
scene.world.node_tree.nodes["Background"].inputs[0].default_value = (.25, .28, .33, 1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value = .035
scene["review_only"] = True
scene["runtime_export_authorized"] = False
scene["native_lights_modified"] = False

def vec(p):
    return Vector((p[0], -p[2], p[1]))

def select(o):
    bpy.ops.object.select_all(action="DESELECT")
    o.select_set(True)
    bpy.context.view_layer.objects.active = o

def apply_transforms(o):
    select(o)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

def floor_origin(o):
    minimum = min(v.co.z for v in o.data.vertices)
    for v in o.data.vertices:
        v.co.z -= minimum

def tris(o):
    return sum(len(p.vertices) - 2 for p in o.data.polygons)

def bounds(o):
    bpy.context.view_layer.update()
    vertices = [o.matrix_world @ v.co for v in o.data.vertices]
    return [[min(getattr(v, a) for v in vertices) for a in ("x", "z")]
            + [min(-v.y for v in vertices)],
            [max(getattr(v, a) for v in vertices) for a in ("x", "z")]
            + [max(-v.y for v in vertices)]]

def closed(o):
    bm = bmesh.new()
    bm.from_mesh(o.data)
    result = all(e.is_manifold for e in bm.edges)
    bm.free()
    return result

def clip(o, co, normal):
    """Actual sealed fracture plane, retaining original per-corner UVs."""
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.bisect_plane(bm, geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                           dist=1e-6, plane_co=co, plane_no=normal, clear_outer=True)
    boundary = [e for e in bm.edges if e.is_boundary]
    if boundary:
        faces = bmesh.ops.holes_fill(bm, edges=boundary, sides=0)["faces"]
        uv = bm.loops.layers.uv.active
        if uv:
            axis = max(range(3),key=lambda i:abs(normal[i]))
            axes = [i for i in range(3) if i != axis]
            for f in faces:
                for loop in f.loops:
                    loop[uv].uv = (loop.vert.co[axes[0]] * .65 + .5, loop.vert.co[axes[1]] * .65 + .5)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(o.data)
    bm.free()
    o.data.update()

def material(name, family, rock=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    bsdf.inputs["Roughness"].default_value = .8
    suffixes = ("diff", "nor_gl", "arm") if rock else ("diff", "normal", "arm")
    files = [family / ("boulder_01_" + s + "_1k.jpg" if rock else s + ".jpg") for s in suffixes]
    textures = []
    for index, path in enumerate(files):
        image = bpy.data.images.load(str(path), check_existing=True)
        if index:
            image.colorspace_settings.name = "Non-Color"
        node = nodes.new("ShaderNodeTexImage")
        node.image = image
        node.location = (-650, 250-index*260)
        textures.append(node)
    links.new(textures[0].outputs["Color"], bsdf.inputs["Base Color"])
    normal = nodes.new("ShaderNodeNormalMap")
    normal.inputs["Strength"].default_value = .42 if rock else .34
    links.new(textures[1].outputs["Color"], normal.inputs["Color"])
    links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    separate = nodes.new("ShaderNodeSeparateColor")
    links.new(textures[2].outputs["Color"], separate.inputs["Color"])
    links.new(separate.outputs["Green"], bsdf.inputs["Roughness"])
    # ARM AO is surface information, never substituted for scene lighting.
    mat["source_family"] = "Poly Haven Boulder 01 CC0" if rock else family.name
    return mat

rock_material = material("Boulder01 shared 1K PBR - CC0", args.source.parent / "textures", True)
wall_material = material("Existing admitted MedievalWall02", repo / "assets/textures/polyhaven/mobile_1k/medieval_wall_02")
floor_material = material("Existing admitted CobblestoneFloor08", repo / "assets/textures/polyhaven/mobile_1k/cobblestone_floor_08")

def box(name, hmin, hmax, mat=wall_material, role="structure", bevel=0):
    center = [(hmin[i]+hmax[i])/2 for i in range(3)]
    bpy.ops.mesh.primitive_cube_add(location=vec(center))
    o = bpy.context.object
    o.name = name
    o.dimensions = (hmax[0]-hmin[0], hmax[2]-hmin[2], hmax[1]-hmin[1])
    apply_transforms(o)
    o.data.materials.append(mat)
    o["role"] = role
    if bevel:
        modifier = o.modifiers.new("Chipped arris", "BEVEL")
        modifier.width = bevel
        modifier.segments = 1
        select(o)
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    # World-metric UVs keep matching stone courses continuous, with no box-face stretching.
    uv = o.data.uv_layers.active or o.data.uv_layers.new()
    for polygon in o.data.polygons:
        axis = max(range(3), key=lambda i: abs(polygon.normal[i]))
        axes = [i for i in range(3) if i != axis]
        for index in polygon.loop_indices:
            world = o.matrix_world @ o.data.vertices[o.data.loops[index].vertex_index].co
            uv.data[index].uv = (world[axes[0]]*.42, world[axes[1]]*.42)
    return o

# Closed structural substrate is cut before rubble/damage dressing. The approved
# door is a blocked former entrance; collision authority stays z=3.4 in runtime.
frame = box("Original rear substrate - real cut opening", (-1.92,-.99,3.4),(1.92,1.42,3.66))
cutter = box("Temporary approved rectangular cutter",(-1.4,-1.10,3.28),(1.4,1.15,3.78))
select(frame)
boolean = frame.modifiers.new("Approved 2.8m by 2.1m threshold opening", "BOOLEAN")
boolean.operation = "DIFFERENCE"
boolean.solver = "EXACT"
boolean.object = cutter
bpy.ops.object.modifier_apply(modifier=boolean.name)
bpy.data.objects.remove(cutter, do_unlink=True)
box("Sealed backing 6.35m",(-1.55,-1.10,6.35),(1.55,2.35,6.52))
box("Recess left wall",(-1.55,-1.10,3.66),(-1.4,2.35,6.4))
box("Recess right wall",(1.4,-1.10,3.66),(1.55,2.35,6.4))
roof = box("Sealed recess roof",(-1.55,2.20,3.55),(1.55,2.35,6.52))
box("Continuous solid floor",(-1.92,-1.10,-6.4),(1.92,-.95,6.52),floor_material)
box("Existing chamber left",(-2.0,-1.10,-6.4),(-1.85,1.50,3.66))
box("Existing chamber right",(1.85,-1.10,-6.4),(2.0,1.50,3.66))
chamber_roof = box("Existing chamber roof",(-2,1.35,-6.4),(2,1.50,3.66))
for i in range(5):
    z = 4.78 + i*.28
    # The lower flight is buried by the settled collapse. Its exposed five-tread
    # remnant climbs above the low upper cavities rather than disappearing behind
    # the approved interlock. No accessible route or collision change is implied.
    box("Stair remnant tread %d" % (i+1),(-.48,-.95,z),(.48,.28+i*.21,z+.28),wall_material, "stair", .007)

before_import = set(scene.objects)
bpy.ops.import_scene.gltf(filepath=str(args.source))
imported = [o for o in scene.objects if o.type == "MESH" and o not in before_import]
if len(imported) != 1:
    raise RuntimeError("Expected one preserved source rock")
template = imported[0]
source_triangles = tris(template)
apply_transforms(template)
select(template)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.remove_doubles(threshold=.00001)
bpy.ops.object.mode_set(mode="OBJECT")
modifier = template.modifiers.new("Measured static mobile reduction", "DECIMATE")
modifier.ratio = 2200/source_triangles
bpy.ops.object.modifier_apply(modifier=modifier.name)
template.scale = tuple(t/d for t,d in zip((1.65,1.18,1.48),template.dimensions))
apply_transforms(template)
floor_origin(template)
template.data.materials.clear()
template.data.materials.append(rock_material)

placements = [
    ("dominant-left",[-.85,-.95,4.10],[1.05,1,1],-.30,((.18,.0,1.24),(.32,.15,1))),
    ("dominant-right",[.78,-.95,4.37],[.93,.80,.95],.72,((.46,.0,.68),(1,-.28,.20))),
    ("upper-interlocked",[.06,-.20,4.43],[.55,.64,.58],-.85,((-.20,.0,1.16),(-.30,.38,1))),
]
rocks = []
for name, position, scale, yaw, fracture in placements:
    o = template.copy()
    o.data = template.data.copy()
    scene.collection.objects.link(o)
    o.name = name
    o.data.name = name + " fractured surface"
    o["role"] = "collapse"
    clip(o, fracture[0],fracture[1])
    # A broad actual attachment plane, rather than contact at one lowest toe:
    # the neutral ground view exposed daylight beneath the original thin cap.
    clip(o,(0,0,.16 if name != "upper-interlocked" else .018),(0,0,-1))
    floor_origin(o)
    o.scale = (scale[0],scale[2],scale[1])
    o.location = vec(position)
    o.location.z -= .002 if name != "upper-interlocked" else 0
    o.rotation_euler.z = yaw
    # Deliberate planar fracture normals; remaining original curved facets retain smooth shading.
    for polygon in o.data.polygons:
        polygon.use_smooth = len(polygon.vertices) == 3
    rocks.append(o)
bpy.data.objects.remove(template,do_unlink=True)

def broken_stone(name, dims, position, yaw, scale=(1,1,1), skew=.12):
    o = box(name,(-dims[0]/2,0,-dims[2]/2),(dims[0]/2,dims[1],dims[2]/2),wall_material,"collapse")
    # box location is compensated before placement, preserving approved floor origin.
    for v in o.data.vertices:
        v.co.z += dims[1]/2
        if v.co.x > 0 and v.co.y > 0:
            v.co.x -= skew
            v.co.z -= skew*.8
    modifier = o.modifiers.new("Fractured masonry arris", "BEVEL")
    modifier.width = .025
    modifier.segments = 1
    select(o)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    floor_origin(o)
    o.location = vec(position)
    o.scale = (scale[0],scale[2],scale[1])
    o.rotation_euler.z = yaw
    # Approved study brick tips straddled the collision plane: settle only just
    # behind it, preserving the spawn/gallery reserve rather than moving the cap.
    minimum_z = bounds(o)[0][2]
    if minimum_z < 3.4:
        o.location.y -= 3.402-minimum_z
        o["approved_study_depth_correction_m"] = 3.402-minimum_z
    return o

lintel = broken_stone("tilted-lintel",(1.65,.42,.38),[.38,-.42,3.90],.26,skew=.22)
left = broken_stone("fallen-left",(.65,.38,.42),[-1.25,-.952,3.57],-.3)
right = broken_stone("fallen-right",(.65,.38,.42),[1.21,-.952,3.68],.5)
interlock = broken_stone("masonry-interlock",(.65,.38,.42),[-.12,.33,4.05],.9,(1,.80,1))
clip(interlock,(0,0,.25),(-.8,-.10,1))
clip(lintel,(.55,0,.35),(.8,.20,1))
# The approved fallen lintel reads as broken masonry rather than an intact bar:
# two closed angular halves share its exact original transform and footprint.
lintel_right = lintel.copy()
lintel_right.data = lintel.data.copy()
scene.collection.objects.link(lintel_right)
lintel_right.name = "tilted-lintel right fracture"
clip(lintel,(.05,0,.21),(1,.18,.35))
clip(lintel_right,(.085,0,.21),(-1,-.18,-.35))

# Damage is subtracted from the real substrate, not regular loose cubes hung on
# its edge. These bounded missing chunks expose the deeper masonry behind them.
for name, position, yaw, dims in [
    ("left-jamb-fracture",[-1.43,.56,3.53],-.24,(.35,.32,.48)),
    ("right-jamb-fracture",[1.45,.91,3.53],.27,(.30,.38,.48)),
    ("header-fracture",[-.48,1.13,3.53],-.16,(.60,.28,.48)),
]:
    chip = broken_stone(name,dims,position,yaw,skew=.08)
    # Fractured sheared quads must be explicitly triangulated before CSG. Their
    # four vertices need not be coplanar; relying on implicit Boolean tessellation
    # can leave a non-manifold sliver on the thin original substrate.
    for csg_object in (frame,chip):
        bm = bmesh.new()
        bm.from_mesh(csg_object.data)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(csg_object.data)
        bm.free()
    select(frame)
    cut = frame.modifiers.new(name,"BOOLEAN")
    cut.operation = "DIFFERENCE"
    cut.solver = "EXACT"
    cut.object = chip
    bpy.ops.object.modifier_apply(modifier=cut.name)
    bpy.data.objects.remove(chip,do_unlink=True)
# Rebuild valid metric UVs on all new fracture/reveal faces. Boolean-generated
# corner UVs otherwise inherit unsuitable cutter projections.
uv = frame.data.uv_layers.active
for polygon in frame.data.polygons:
    axis = max(range(3),key=lambda i:abs(polygon.normal[i]))
    axes = [i for i in range(3) if i != axis]
    for index in polygon.loop_indices:
        world = frame.matrix_world @ frame.data.vertices[frame.data.loops[index].vertex_index].co
        uv.data[index].uv = (world[axes[0]]*.42,world[axes[1]]*.42)

def bvh(o):
    vertices = [o.matrix_world @ v.co for v in o.data.vertices]
    return BVHTree.FromPolygons(vertices,[list(p.vertices) for p in o.data.polygons],all_triangles=False,epsilon=.00001)

# All interlocked pieces must contact real supporting geometry. Overlap checks
# are geometric witnesses, not a substitute for owner judgment of natural placement.
bpy.context.view_layer.update()
support_checks = []
lintel_right_settlement = 0
while not any(bvh(lintel_right).overlap(bvh(s)) for s in rocks):
    if lintel_right_settlement >= .25:
        raise RuntimeError("Broken right lintel cannot settle on approved supporting pile")
    lintel_right.location.z -= .002
    lintel_right_settlement += .002
    bpy.context.view_layer.update()
for o in (rocks[2],lintel,lintel_right,interlock):
    candidates = rocks[:2] if o == rocks[2] else rocks
    touching = [s.name for s in candidates if s != o and bvh(o).overlap(bvh(s))]
    support_checks.append({"object":o.name,"support_meshes":touching,"intersects_real_support":bool(touching),
                           "gravitySettlementMeters":lintel_right_settlement if o == lintel_right else 0})
    if not touching:
        raise RuntimeError("Floating interlock without real geometric support: " + o.name)
# Keep one admitted authored lintel mesh, with two independently supported closed
# components; splitting the shape does not propose another runtime instance.
bpy.ops.object.select_all(action="DESELECT")
lintel.select_set(True)
lintel_right.select_set(True)
bpy.context.view_layer.objects.active = lintel
bpy.ops.object.join()
floor_contacts = []
for o in rocks[:2]:
    area = sum(p.area for p in o.data.polygons
               if all(abs(o.data.vertices[v].co.z)<1e-5 for v in p.vertices)) * o.scale.x * o.scale.y
    inset = -.95-bounds(o)[0][1]
    floor_contacts.append({"object":o.name,"planarFootAreaSquareMeters":area,"floorInsetMeters":inset})
    if area < .12 or not 0 <= inset <= .005:
        raise RuntimeError("Grounded rock lacks broad controlled floor attachment: "+json.dumps(floor_contacts[-1]))

# Inspection lights live only in this editable review file. They never enter
# runtime exports; existing game illumination must be accepted later in native RT.
inspection_lights = []
def area(name, position, target, energy, size, color):
    data = bpy.data.lights.new(name,"AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    o = bpy.data.objects.new(name,data)
    scene.collection.objects.link(o)
    o.location = vec(position)
    o.rotation_euler = (vec(target)-o.location).to_track_quat("-Z","Y").to_euler()
    o["authoring_inspection_only"] = True
    inspection_lights.append(o)
    return o

area("Inspection key - no runtime light",(-1.10,.90,2.65),(0,.15,4.65),105,1.1,(1,.76,.52))
area("Inspection fill - no runtime light",(1.25,.65,2.30),(0,.0,4.2),45,1.2,(.62,.72,1))
area("Inspection overhead - no runtime light",(0,1.8,4.75),(0,-.50,4.4),20,.8,(.9,.92,1))
torch_data = bpy.data.lights.new("Torch-position authoring review - not a new runtime light","POINT")
torch_data.energy = 75
torch_data.color = (1,.47,.16)
torch_data.shadow_soft_size = .045
torch_review = bpy.data.objects.new(torch_data.name,torch_data)
scene.collection.objects.link(torch_review)
# Shared GameSimulation at the approved backward-looking camera and AnatomicalBody
# mount supplies this CPU socket. Final skin/IK and exact native radiometry still
# require production RT acceptance; Blender watts are not renderer intensity units.
torch_position = (.159999937,.806602836,2.310449123)
torch_review.location = vec(torch_position)
torch_review["authoring_inspection_only"] = True

views = {
    "production":((0,.7,1.85),(0,.25,4.4),60,False),
    "neutral":((0,.7,1.85),(0,.25,4.4),60,False),
    "contact":((-.3,-.60,2.85),(0,-.60,4.15),75,False),
    "support":((1.10,.48,3.05),(-.25,.02,4.50),82,False),
    "opening-left":((-1.65,.72,2.5),(.30,.30,4.80),120,False),
    "opening-right":((1.65,.72,2.5),(-.30,.30,4.80),120,False),
    "wide-left":((-1.65,.72,-.25),(.35,.20,4.70),120,False),
    "wide-right":((1.65,.72,-.25),(-.35,.20,4.70),120,False),
    "ceiling":((0,.68,3.0),(0,1.68,3.0),90,False),
    "ceiling-left":((-1.10,.65,3.00),(.25,1.70,4.90),95,False),
    "ceiling-right":((1.10,.65,3.00),(-.25,1.70,4.90),95,False),
    "overhead-cutaway":((0,6,4.25),(0,-.50,4.25),60,True),
    "stairs-oblique":((-.85,1.05,2.95),(.12,.70,5.30),75,False),
}
bpy.ops.object.camera_add()
camera = bpy.context.object
camera.name = "Production and inspection camera"
scene.camera = camera
camera.data.clip_start = .02
camera.data.clip_end = 50
camera.data.sensor_fit = "VERTICAL"
camera.data.sensor_height = 24
original_energies = [o.data.energy for o in inspection_lights]
rendered = []
for name,(position,target,fov,cutaway) in views.items():
    if args.views not in ("all","none") and name not in args.views.split(","):
        continue
    if args.views == "none":
        continue
    camera.location = vec(position)
    camera.rotation_euler = (vec(target)-camera.location).to_track_quat("-Z","Y").to_euler()
    camera.data.lens = 12/math.tan(math.radians(fov)/2)
    roof.hide_render = chamber_roof.hide_render = cutaway
    scene.world.node_tree.nodes["Background"].inputs[1].default_value = .002 if name == "production" else .035
    torch_data.energy = 75 if name == "production" else 0
    for o,energy in zip(inspection_lights,original_energies):
        o.data.energy = 0 if name == "production" else energy*(1.5 if name == "neutral" else 1)
    scene.render.filepath = str(args.output/(name+".png"))
    bpy.ops.render.render(write_still=True)
    rendered.append(name+".png")
roof.hide_render = chamber_roof.hide_render = False
for o,energy in zip(inspection_lights,original_energies):
    o.data.energy = 0
torch_data.energy = 75
scene.world.node_tree.nodes["Background"].inputs[1].default_value = .002
camera.location = vec(views["production"][0])
camera.rotation_euler = (vec(views["production"][1])-camera.location).to_track_quat("-Z","Y").to_euler()
camera.data.lens = 12/math.tan(math.radians(60)/2)

meshes = [o for o in scene.objects if o.type == "MESH"]
geometry = [{"name":o.name,"role":o.get("role"),"triangles":tris(o),"boundsHorde":bounds(o),
             "closedManifold":closed(o),"uv":bool(o.data.uv_layers),
             "studyDepthCorrectionMeters":o.get("approved_study_depth_correction_m",0)} for o in meshes]
total = sum(item["triangles"] for item in geometry)
if total >= 10000:
    raise RuntimeError("Collapse source exceeds approved 10k triangle budget")
if not all(item["closedManifold"] and item["uv"] for item in geometry):
    raise RuntimeError("Open substrate/prop or missing UV: " + json.dumps([
        item for item in geometry if not item["closedManifold"] or not item["uv"]]))
if any(bounds(o)[0][2] < 3.3999 for o in meshes if o.get("role") == "collapse"):
    raise RuntimeError("Dressing protrudes into live collision reserve")
if not all(1000 <= tris(o) <= 3000 for o in rocks):
    raise RuntimeError("Dominant rock exceeds 1-3k triangle range")
stair_visibility = []
depsgraph = bpy.context.evaluated_depsgraph_get()
eye = vec(views["production"][0])
for i in range(5):
    samples = []
    for x in (-.30,0,.30):
        target = vec((x,.28+i*.21-.06,4.78+i*.28))
        direction = target-eye
        hit,location,normal,index,obj,matrix = scene.ray_cast(depsgraph,eye,direction.normalized(),distance=direction.length+.01)
        samples.append(bool(hit and obj.name == "Stair remnant tread %d" % (i+1)))
    stair_visibility.append({"tread":i+1,"visibleRiserSamples":sum(samples),"samples":3})
if sum(item["visibleRiserSamples"]>0 for item in stair_visibility)<3:
    raise RuntimeError("Fewer than three remnant risers visible from approved production eye: "+json.dumps(stair_visibility))

# Pack preserved surface textures for a genuinely editable, portable file. Remove
# orphan imported/template meshes/materials; no proxy or hidden high-poly source.
for image in bpy.data.images:
    if image.source == "FILE" and image.users:
        image.pack()
bpy.ops.outliner.orphans_purge(do_local_ids=True,do_linked_ids=False,do_recursive=True)
images = [{"name":image.name,"dimensions":list(image.size),"packed":bool(image.packed_file)}
          for image in bpy.data.images if image.users and image.source == "FILE"]
report = {
    "schema":"horde.collapse-form-authoring.v1", "function":"approved", "form":"pending-owner-review",
    "runtime":"not-exported-not-validated", "sourceTriangles":source_triangles,
    "totalTrianglesIncludingStructuralContext":total,"geometry":geometry,"supportChecks":support_checks,
    "floorContacts":floor_contacts,
    "productionRiserVisibility":stair_visibility,
    "unsharedVertexIndexPrimitiveBytesUpperBound":total*(3*64+3*4+16),
    "gpuMemoryDisclosure":"Upper bound uses current64Bvertex/4Bindex/16Bprimitive records; excludes unmeasured BLAS/TLAS and textures, no runtime asset admitted",
    "textureImages":images,"newSharedTextureFamily":"Poly Haven Boulder 01 1K CC0",
    "retainedExistingFamilies":["MedievalWall02","CobblestoneFloor08"],
    "inspectionLightsOnly":True,"nativeLightingModified":False,"lightingBaked":False,
    "productionReviewLight":{"sharedCpuTorchPositionHorde":list(torch_position),"powerWatts":75,
                             "color":[1,.47,.16],"sourceRadiusMeters":.045,"exactNativeLighting":False},
    "cpuRenderer":{"engine":"Cycles","device":"CPU","threads":2,"samples":20},
    "productionCameraHorde":{"position":list(views["production"][0]),"target":list(views["production"][1]),
                             "verticalFovDegrees":60,"viewport":[960,540]},
    "sealedBackingFrontZ":6.35,"unchangedCollisionCapZ":3.4,"stairDepthRange":[4.78,6.18],
    "renderedViews":rendered,"cutawayDisclosure":"overhead-cutaway hides two roof meshes for inspection only",
    "provenance":{"asset":"Boulder 01","license":"CC0 1.0","receipt":"../download-receipt.json",
                  "licenseSnapshot":"../polyhaven-license.html","verifiedSourceFiles":receipt},
    "acceptanceGaps":["Owner Form approval","No runtime GLB export before Form approval",
                      "Native RT visual/asset-resource validation","Owner/device acceptance"]}
(args.output/"audit.json").write_text(json.dumps(report,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(args.output/"final.blend"))
print(json.dumps({"finalBlend":str(args.output/"final.blend"),"totalTriangles":total,
                  "rocks":[{"name":o.name,"triangles":tris(o)} for o in rocks],"views":rendered}))
