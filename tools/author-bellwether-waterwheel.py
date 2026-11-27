"""Fit an original rigid mill wheel to the surveyed B05 riverbank.

Blender --background --factory-startup --python-exit-code 1
  --python tools/author-bellwether-waterwheel.py -- ROOT NEW_OUTPUT
"""
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
from chapter_water_conformance import ribbon_geometry


SOURCE = Path("assets/models/world/source/bellwether-waterwheel-v1")
TERRAIN = Path("assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb")
BASELINE = "d37f1638:docs/design/world/dungeons/baseline/shared-coordinate-layout.json"
ATLAS_HASH = "0a4da29bf8d988145c0f7b3f1c7afaf415a5da9b14c34bbd53bd1e00e3a86e59"
TEXTURES = Path("assets/textures/props/source")
AXLE = Vector((1, 0, 0))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def flat_material(name, rgba, metallic, roughness):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    surface = material.node_tree.nodes["Principled BSDF"]
    surface.inputs["Base Color"].default_value = rgba
    surface.inputs["Metallic"].default_value = metallic
    surface.inputs["Roughness"].default_value = roughness
    return material


def oak_material(root):
    oak = flat_material("BellwetherWheelOak", (.18, .085, .036, 1), .03, .75)
    nodes, links = oak.node_tree.nodes, oak.node_tree.links
    surface = nodes["Principled BSDF"]
    images = {}
    for kind in ("base-color", "normal", "orm"):
        image = bpy.data.images.load(str(root / TEXTURES / f"chest-wood-{kind}.png"))
        image.colorspace_settings.name = "sRGB" if kind == "base-color" else "Non-Color"
        image.pack()
        images[kind] = nodes.new("ShaderNodeTexImage")
        images[kind].image = image
    links.new(images["base-color"].outputs["Color"], surface.inputs["Base Color"])
    channels = nodes.new("ShaderNodeSeparateColor")
    links.new(images["orm"].outputs["Color"], channels.inputs["Color"])
    links.new(channels.outputs["Green"], surface.inputs["Roughness"])
    links.new(channels.outputs["Blue"], surface.inputs["Metallic"])
    normal = nodes.new("ShaderNodeNormalMap")
    normal.inputs["Strength"].default_value = .45
    links.new(images["normal"].outputs["Color"], normal.inputs["Color"])
    links.new(normal.outputs["Normal"], surface.inputs["Normal"])
    return oak


def radial(angle):
    return Vector((0, math.sin(angle), math.cos(angle)))


class WheelMesh:
    def __init__(self):
        self.vertices = []
        self.faces = []
        self.materials = []
        self.uv = []

    def face(self, vertices, indices, material, uv):
        offset = len(self.vertices)
        self.vertices.extend(vertices)
        self.faces.extend(tuple(offset + i for i in face) for face in indices)
        self.materials.extend([material] * len(indices))
        self.uv.extend(uv)

    def box(self, centre, axes, half_sizes, material, grain_axis, phase=0.):
        corners = ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),
                   (-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1))
        local = [tuple(c * size for c, size in zip(signs, half_sizes)) for signs in corners]
        vertices = [centre + sum((axis * n for axis, n in zip(axes, p)), Vector())
                    for p in local]
        faces = ((3,2,1,0),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7))
        charts = []
        for face in faces:
            u_axis, v_axis = sorted((i for i in range(3) if local[face[0]][i] !=
                                    local[face[1]][i] or local[face[0]][i] != local[face[2]][i]),
                                   key=lambda i: i == grain_axis)
            charts.append([(local[index][u_axis] * 1.9 + phase,
                            local[index][v_axis] * .7 + phase) for index in face])
        self.face(vertices, faces, material, charts)

    def rim(self, west, east, inside, outside, segments, material):
        rings = ((west, inside), (east, inside), (east, outside), (west, outside))
        vertices = [AXLE*x + radial(math.tau*i/segments)*radius
                    for x, radius in rings for i in range(segments)]
        faces, charts = [], []
        for side in range(4):
            next_side = (side+1) % 4
            width = (east-west) if side % 2 == 0 else outside-inside
            for index in range(segments):
                after = (index+1) % segments
                faces.append((side*segments+index, side*segments+after,
                              next_side*segments+after, next_side*segments+index))
                u = 1.9 * width
                v, w = 6.*index/segments, 6.*(index+1)/segments
                charts.append(((0.,v),(0.,w),(u,w),(u,v)))
        self.face(vertices, faces, material, charts)

    def journal(self, west, east, radius, segments, material):
        vertices = [AXLE*x + radial(math.tau*i/segments)*radius
                    for x in (west, east) for i in range(segments)]
        faces = [tuple(reversed(range(segments))), tuple(range(segments, 2*segments))]
        caps = [(math.cos(math.tau*i/segments), math.sin(math.tau*i/segments))
                for i in range(segments)]
        charts = [caps[::-1], caps]
        for i in range(segments):
            after = (i+1) % segments
            faces.append((i, after, after+segments, i+segments))
            charts.append(((0.,i/segments),(0.,(i+1)/segments),
                           (1.,(i+1)/segments),(1.,i/segments)))
        self.face(vertices, faces, material, charts)

    def build(self, materials):
        mesh = bpy.data.meshes.new("Waterwheel timbers and forged fittings")
        mesh.from_pydata(self.vertices, [], self.faces)
        for material in materials:
            mesh.materials.append(material)
        mesh.update()
        uv = mesh.uv_layers.new(name="Grain").data
        for face, material, chart in zip(mesh.polygons, self.materials, self.uv):
            face.material_index = material
            for loop, coords in zip(face.loop_indices, chart):
                uv[loop].uv = coords
        editable = bmesh.new()
        editable.from_mesh(mesh)
        bmesh.ops.recalc_face_normals(editable, faces=list(editable.faces))
        if any(not edge.is_manifold for edge in editable.edges):
            raise ValueError("Every wheel timber and fitting must be a closed solid")
        editable.to_mesh(mesh)
        editable.free()
        mesh.calc_loop_triangles()
        obj = bpy.data.objects.new("BellwetherWaterwheel", mesh)
        bpy.context.collection.objects.link(obj)
        return obj


def author_wheel(root, dimensions):
    radius, half_width = dimensions["radius"], dimensions["faceWidth"] / 2
    wheel = WheelMesh()
    oak = oak_material(root)
    iron = flat_material("BellwetherWheelWroughtIron", (.058, .046, .036, 1), .65, .63)
    for sign in (-1, 1):
        shoulder = half_width - .025
        west, east = sorted((sign*(shoulder-.13), sign*shoulder))
        wheel.rim(west, east, radius-.17, radius-.028, 32, 0)
        narrow_west, narrow_east = sorted((sign*shoulder, sign*half_width))
        wheel.rim(narrow_west, narrow_east, radius-.09, radius, 32, 1)

    cartesian = (AXLE, Vector((0, 1, 0)), Vector((0, 0, 1)))
    wheel.box(Vector(), cartesian, (1.42,.145,.145), 0, 0)
    for sign in (-1,1):
        journal = sorted((sign*1.38, sign*dimensions["axleHalfLength"]))
        wheel.journal(*journal, .166, 16, 1)
        wheel.rim(*sorted((sign*.73, sign*.81)), .11, .258, 16, 1)
        for index in range(8):
            angle = index * math.tau / 8
            outward, tangent = radial(angle), radial(angle+math.pi/2)
            axes = (AXLE, tangent, outward)
            wheel.box(AXLE*(sign*.755)+outward*.765, axes, (.085,.086,.604), 0, 2, index*.13)
            wheel.box(AXLE*(sign*.817)+outward*1.362, axes, (.034,.069,.049), 1, 2)
    for index in range(16):
        angle = (index + .5) * math.tau / 16
        outward = radial(angle)
        tangent = radial(angle+math.pi/2)
        offset = index*.19
        wheel.box(outward*1.309, (AXLE,tangent,outward), (.726,.157,.037), 0, 0, offset)
        lip_axes = (AXLE, radial(angle+1.16), radial(angle-.411))
        wheel.box(outward*1.387+tangent*.105, lip_axes, (.726,.036,.107), 0, 0, offset)
    return wheel.build((oak, iron))


def surface_height(triangles, x, z):
    heights = []
    for a, b, c in triangles:
        area = (b[0]-a[0])*(c[2]-a[2]) - (b[2]-a[2])*(c[0]-a[0])
        u = ((x-a[0])*(c[2]-a[2]) - (z-a[2])*(c[0]-a[0])) / area
        v = ((b[0]-a[0])*(z-a[2]) - (b[2]-a[2])*(x-a[0])) / area
        if u >= 0 and v >= 0 and u+v <= 1:
            heights.append(a[1]+u*(b[1]-a[1])+v*(c[1]-a[1]))
    return max(heights, default=None)


def terrain_survey(root, layout):
    if digest(root / TERRAIN) != layout["terrainSha256"]:
        raise ValueError("Re-survey against the changed terrain before placing the millrace")
    raw = subprocess.check_output(["git","show",BASELINE], cwd=root)
    if hashlib.sha256(raw).hexdigest() != ATLAS_HASH:
        raise ValueError("Unexpected regional river atlas identity")
    river = SimpleNamespace(river_water=[json.loads(raw)["river_surface"]])
    _, _, water_triangles = ribbon_geometry(river, 5.)
    bpy.ops.import_scene.gltf(filepath=str(root / TERRAIN))
    objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    vertices, faces = [], []
    for obj in objects:
        offset = len(vertices)
        vertices.extend(obj.matrix_world @ vertex.co for vertex in obj.data.vertices)
        faces.extend(tuple(offset+i for i in face.vertices) for face in obj.data.polygons)
    ground = BVHTree.FromPolygons(vertices,faces)

    def sample(x, z):
        hit, _, _, _ = ground.ray_cast(Vector((x,-z,100)), Vector((0,0,-1)), 300)
        if hit is None:
            raise ValueError(f"Terrain support missing at {x}, {z}")
        return hit.z

    depth = .40
    half_width = layout["channelWidth"] / 2
    survey = {}
    for name in ("headrace", "tailrace"):
        stations = layout[name]
        if any(b[1] >= a[1] or b[2] <= a[2] for a,b in zip(stations,stations[1:])):
            raise ValueError("Race stations must descend downstream")
        cuts = []
        elevations = []
        for x,y,z in stations:
            floor = [sample(x+dx,z) for dx in (-half_width,0.,half_width)]
            cuts.extend(ground_y-(y-depth) for ground_y in floor)
            elevations.append([round(h,3) for h in floor])
        survey[name] = dict(stationTerrainWestCentreEast=elevations,
                            maxSampledCutToBed=round(max(cuts),3),
                            maxSampledFillToBed=round(max(0,-min(cuts)),3))
    lips = (layout["headrace"][0], layout["tailrace"][-1])
    junctions = []
    for x,y,z in lips:
        main = [surface_height(water_triangles,x+dx,z) for dx in (-half_width,0.,half_width)]
        if any(h is None or not .004 < h-y < .08 for h in main):
            raise ValueError("Intake and return lips must meet the existing river")
        junctions.append([round(h-y,3) for h in main])
    x,y,z = layout["wheel"]["axleCentre"]
    radius = layout["wheel"]["radius"]
    head = layout["headrace"][-1]
    tail = layout["tailrace"][0]
    head_reach = math.hypot(head[1]-y, head[2]-z)
    if head_reach >= radius or not 0 < tail[1]-(y-radius) <= .45 or tail[2] != z:
        raise ValueError("Wheel needs upper water contact and shallow tailwater engagement")
    if layout["protectedPracticeLanding"][1] - lips[-1][2] < 3.:
        raise ValueError("Return lip moves too close to the practice landing")
    if any(min(station[0]-half_width for station in layout[name]) <= 134.3
           for name in ("headrace","tailrace")):
        raise ValueError("Race intersects the B05 eastern footprint")
    # The editable wheel sits at zero. Keep the large admitted terrain out of its source file.
    return dict(survey, lipsBelowExistingRiver=junctions, assumedBedDepth=depth,
                wheelStationTerrainY=round(sample(x,z),3),
                wheelHeadContactRadius=round(head_reach,3),
                wheelTailwaterEngagement=round(tail[1]-(y-radius),3),
                dropAtWheel=round(head[1]-tail[1],3))


def main(root, output):
    if output.exists():
        raise ValueError("Choose a new output directory")
    layout_path = root / SOURCE / "layout.json"
    layout = json.loads(layout_path.read_text(encoding="utf-8"))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    survey = terrain_survey(root, layout)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    wheel = author_wheel(root, layout["wheel"])
    if len(wheel.data.loop_triangles) > 2600:
        raise ValueError("Wheel exceeds the source study triangle budget")
    bpy.context.view_layer.objects.active = wheel
    wheel.select_set(True)
    output.mkdir(parents=True)
    master = output / "waterwheel.blend"
    candidate = output / "waterwheel.glb"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    wheel.modifiers.new("GLB triangulation", 'TRIANGULATE')
    bpy.ops.export_scene.gltf(filepath=str(candidate), export_format="GLB", use_selection=True,
                              export_apply=True,
                              export_normals=True, export_tangents=True, export_animations=False)
    receipt = dict(layoutSha256=digest(layout_path), recipeSha256=digest(Path(__file__)),
                   terrainSha256=layout["terrainSha256"],
                   atlasSha256=ATLAS_HASH, masterSha256=digest(master), glbSha256=digest(candidate),
                   textureSha256={p.name:digest(p) for p in sorted((root / TEXTURES).glob("chest-wood-*.png"))},
                   triangles=len(wheel.data.loop_triangles), materials=len(wheel.data.materials),
                   origin="Axle centre, +X rigid rotation axis", survey=survey)
    (output / "authoring-receipt.json").write_text(
        json.dumps(receipt, indent=2)+"\n", encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--") + 1:]))
