import json
import math
import os
import pathlib
import struct
import sys

import bpy
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python process-production-gothic-props.py -- REPO_ROOT")

ROOT = pathlib.Path(sys.argv[sys.argv.index("--") + 1]).resolve()
TEXTURES = ROOT / "assets" / "textures" / "props" / "source"
CHEST_SOURCE = ROOT / "assets" / "models" / "props" / "meshy" / "production-gothic-chest-2026-08-27"
LANTERN_SOURCE = ROOT / "assets" / "models" / "props" / "meshy" / "production-reward-lantern-2026-08-27"
RUNTIME = ROOT / "assets" / "models" / "props" / "runtime"


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def image(path):
    return bpy.data.images.load(str(TEXTURES / path), check_existing=True)


def set_input(node, names, value):
    for name in names:
        if name in node.inputs:
            node.inputs[name].default_value = value
            return


def textured_material(name, prefix, metallic, roughness):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    principled = nodes.new("ShaderNodeBsdfPrincipled")
    principled.location = (360, 0)
    output.location = (650, 0)
    links.new(principled.outputs["BSDF"], output.inputs["Surface"])
    set_input(principled, ("Metallic",), metallic)
    set_input(principled, ("Roughness",), roughness)

    base = nodes.new("ShaderNodeTexImage")
    base.image = image(f"{prefix}-base-color.png")
    base.image.colorspace_settings.name = "sRGB"
    base.location = (-600, 180)
    links.new(base.outputs["Color"], principled.inputs["Base Color"])

    orm = nodes.new("ShaderNodeTexImage")
    orm.image = image(f"{prefix}-orm.png")
    orm.image.colorspace_settings.name = "Non-Color"
    orm.location = (-600, -70)
    separate = nodes.new("ShaderNodeSeparateColor")
    separate.location = (-340, -70)
    links.new(orm.outputs["Color"], separate.inputs["Color"])
    links.new(separate.outputs["Green"], principled.inputs["Roughness"])
    links.new(separate.outputs["Blue"], principled.inputs["Metallic"])

    normal_texture = nodes.new("ShaderNodeTexImage")
    normal_texture.image = image(f"{prefix}-normal.png")
    normal_texture.image.colorspace_settings.name = "Non-Color"
    normal_texture.location = (-600, -340)
    normal = nodes.new("ShaderNodeNormalMap")
    normal.location = (-330, -320)
    normal.inputs["Strength"].default_value = 0.55
    links.new(normal_texture.outputs["Color"], normal.inputs["Color"])
    links.new(normal.outputs["Normal"], principled.inputs["Normal"])
    return material


def simple_material(name, base, metallic, roughness, transmission=0.0, ior=1.5,
                    emission=None, emission_strength=1.0):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    principled = next(node for node in material.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
    set_input(principled, ("Base Color",), (*base, 1.0))
    set_input(principled, ("Metallic",), metallic)
    set_input(principled, ("Roughness",), roughness)
    set_input(principled, ("Transmission Weight", "Transmission"), transmission)
    set_input(principled, ("IOR",), ior)
    if emission is not None:
        set_input(principled, ("Emission Color", "Emission"), (*emission, 1.0))
        set_input(principled, ("Emission Strength",), emission_strength)
    return material


def apply_modifier(obj, modifier):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def bevelled_box(name, location, dimensions, material, bevel=0.008, segments=1):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel > 0.0:
        modifier = obj.modifiers.new("ForgedEdge", "BEVEL")
        modifier.width = bevel
        modifier.segments = segments
        modifier.limit_method = "ANGLE"
        apply_modifier(obj, modifier)
    obj.data.materials.append(material)
    return obj


def cylinder_between(name, start, end, radius, material, vertices=10):
    start_vector = Vector(start)
    end_vector = Vector(end)
    delta = end_vector - start_vector
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=delta.length,
                                       location=(start_vector + end_vector) * 0.5)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0.0, 0.0, 1.0)).rotation_difference(delta.normalized())
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    obj.data.materials.append(material)
    return obj


def tube_path(name, points, radius, material, cyclic=False, resolution=0, bevel_resolution=0):
    curve = bpy.data.curves.new(name + "Curve", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = resolution
    curve.bevel_depth = radius
    curve.bevel_resolution = bevel_resolution
    curve.resolution_u = 1
    curve.resolution_v = 0
    spline = curve.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for point, value in zip(spline.points, points):
        point.co = (*value, 1.0)
    spline.use_cyclic_u = cyclic
    obj = bpy.data.objects.new(name, curve)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    return bpy.context.object


def torus(name, location, major_radius, minor_radius, material,
          major_segments=28, minor_segments=8, rotation=(math.pi / 2.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major_radius, minor_radius=minor_radius,
                                    major_segments=major_segments, minor_segments=minor_segments,
                                    location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(material)
    return obj


def uv_smart(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.012)
    bpy.ops.object.mode_set(mode="OBJECT")


def join(objects, name, material):
    if not objects:
        raise RuntimeError(f"no objects supplied for {name}")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    if len(objects) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name + "Mesh"
    while len(obj.data.materials) > 0:
        obj.data.materials.pop(index=0)
    obj.data.materials.append(material)
    triangulate = obj.modifiers.new("RuntimeTriangles", "TRIANGULATE")
    apply_modifier(obj, triangulate)
    uv_smart(obj)
    for polygon in obj.data.polygons:
        polygon.use_smooth = False
    return obj


def triangles(objects):
    return sum(sum(max(0, len(polygon.vertices) - 2) for polygon in obj.data.polygons)
               for obj in objects if obj.type == "MESH")


def write_glb(path, document, binary):
    json_bytes = json.dumps(document, separators=(",", ":")).encode("utf-8")
    while len(json_bytes) % 4:
        json_bytes += b" "
    while len(binary) % 4:
        binary += b"\0"
    total = 12 + 8 + len(json_bytes) + (8 + len(binary) if binary else 0)
    payload = bytearray(struct.pack("<III", 0x46546C67, 2, total))
    payload.extend(struct.pack("<II", len(json_bytes), 0x4E4F534A))
    payload.extend(json_bytes)
    if binary:
        payload.extend(struct.pack("<II", len(binary), 0x004E4942))
        payload.extend(binary)
    path.write_bytes(payload)


def patch_material_extensions(path):
    payload = path.read_bytes()
    offset = 12
    document = None
    binary = b""
    while offset < len(payload):
        length, chunk_type = struct.unpack_from("<II", payload, offset)
        offset += 8
        chunk = payload[offset:offset + length]
        offset += length
        if chunk_type == 0x4E4F534A:
            document = json.loads(chunk.decode("utf-8").rstrip(" \0"))
        elif chunk_type == 0x004E4942:
            binary = chunk
    if document is None:
        raise RuntimeError(f"{path}: missing GLB JSON")
    used = document.setdefault("extensionsUsed", [])
    for material in document.get("materials", []):
        if material.get("name") == "LanternGlass":
            extensions = material.setdefault("extensions", {})
            extensions["KHR_materials_transmission"] = {"transmissionFactor": 0.94}
            extensions["KHR_materials_volume"] = {
                "thicknessFactor": 1.0,
                "attenuationDistance": 1.8,
                "attenuationColor": [0.94, 0.90, 0.78],
            }
            extensions["KHR_materials_ior"] = {"ior": 1.52}
            material.setdefault("pbrMetallicRoughness", {})["baseColorFactor"] = [0.96, 0.97, 0.98, 1.0]
            # A closed volume has a defined outward surface; double-sided volume
            # shading is both physically ambiguous and rejected by Khronos review.
            material["doubleSided"] = False
            for extension in ("KHR_materials_transmission", "KHR_materials_volume", "KHR_materials_ior"):
                if extension not in used:
                    used.append(extension)
        elif material.get("name") == "FlameCore":
            extensions = material.setdefault("extensions", {})
            extensions["KHR_materials_emissive_strength"] = {"emissiveStrength": 6.0}
            material["emissiveFactor"] = [1.0, 0.22, 0.025]
            if "KHR_materials_emissive_strength" not in used:
                used.append("KHR_materials_emissive_strength")
    if not used:
        document.pop("extensionsUsed", None)
    write_glb(path, document, binary)


def export_component(label, objects, source_path, runtime_path, report_extra=None):
    source_path.parent.mkdir(parents=True, exist_ok=True)
    runtime_path.parent.mkdir(parents=True, exist_ok=True)
    mesh_objects = [obj for obj in objects if obj.type == "MESH"]
    count = triangles(mesh_objects)
    bpy.ops.export_scene.gltf(
        filepath=str(source_path), export_format="GLB", export_tangents=True,
        export_animations=False, export_skins=False, export_morph=False,
        export_cameras=False, export_lights=False,
    )
    patch_material_extensions(source_path)
    for loaded_image in bpy.data.images:
        if loaded_image.size[0] > 4 or loaded_image.size[1] > 4:
            loaded_image.scale(4, 4)
    bpy.ops.export_scene.gltf(
        filepath=str(runtime_path), export_format="GLB", export_tangents=True,
        export_animations=False, export_skins=False, export_morph=False,
        export_cameras=False, export_lights=False,
    )
    patch_material_extensions(runtime_path)
    report = {
        "schema": 1,
        "component": label,
        "source": source_path.name,
        "runtime": runtime_path.name,
        "triangles": count,
        "vertices": sum(len(obj.data.vertices) for obj in mesh_objects),
        "materials": sorted({slot.material.name for obj in mesh_objects for slot in obj.material_slots}),
        "nodes": sorted(obj.name for obj in objects),
        "axes": {"up": "+Y", "forward": "+Z"},
        "processing": "Meshy-silhouette-guided deterministic rigid reauthor; applied transforms; triangulated; smart UV; reviewed tangents; 4x4 runtime texture identities",
    }
    if report_extra:
        report.update(report_extra)
    with open(runtime_path.with_suffix(".glb.processing.json"), "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
    print(json.dumps(report))
    return report


def barrel_lid(material):
    segments = 40
    vertices = []
    for x in (-0.49, 0.49):
        for index in range(segments + 1):
            angle = math.pi * index / segments
            y = -0.28 + 0.28 * math.cos(angle)
            z = 0.045 + 0.22 * math.sin(angle)
            vertices.append((x, y, z))
    faces = []
    ring = segments + 1
    for index in range(segments):
        faces.append((index, index + 1, ring + index + 1, ring + index))
    faces.append(tuple(range(segments, -1, -1)))
    faces.append(tuple(range(ring, ring + segments + 1)))
    faces.append((0, ring, ring + segments, segments))
    mesh = bpy.data.meshes.new("ChestLidWoodMesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    obj = bpy.data.objects.new("ChestLid", mesh)
    bpy.context.scene.collection.objects.link(obj)
    bevel = obj.modifiers.new("LidEdge", "BEVEL")
    bevel.width = 0.006
    bevel.segments = 1
    apply_modifier(obj, bevel)
    triangulate = obj.modifiers.new("RuntimeTriangles", "TRIANGULATE")
    apply_modifier(obj, triangulate)
    uv_smart(obj)
    return obj


def create_frustum(name, z_bottom, z_top, radius_bottom, radius_top, material, sides=6):
    vertices = []
    for z, radius in ((z_bottom, radius_bottom), (z_top, radius_top)):
        for index in range(sides):
            angle = 2.0 * math.pi * index / sides + math.pi / 6.0
            vertices.append((math.cos(angle) * radius, math.sin(angle) * radius, z))
    faces = []
    for index in range(sides):
        nxt = (index + 1) % sides
        faces.append((index, nxt, sides + nxt, sides + index))
    faces.append(tuple(reversed(range(sides))))
    faces.append(tuple(range(sides, sides * 2)))
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    triangulate = obj.modifiers.new("RuntimeTriangles", "TRIANGULATE")
    apply_modifier(obj, triangulate)
    uv_smart(obj)
    return obj


def build_chest_base():
    reset_scene()
    wood = textured_material("ChestWood", "chest-wood", 0.03, 0.72)
    iron = textured_material("BlackIron", "chest-iron", 0.92, 0.62)
    wood_parts = [
        bevelled_box("BaseBottom", (0.0, 0.0, 0.035), (0.98, 0.54, 0.07), wood, 0.01, 2),
        bevelled_box("BaseFront", (0.0, -0.255, 0.205), (0.98, 0.055, 0.35), wood, 0.009, 2),
        bevelled_box("BaseRear", (0.0, 0.255, 0.205), (0.98, 0.055, 0.35), wood, 0.009, 2),
        bevelled_box("BaseLeft", (-0.462, 0.0, 0.205), (0.055, 0.47, 0.35), wood, 0.009, 2),
        bevelled_box("BaseRight", (0.462, 0.0, 0.205), (0.055, 0.47, 0.35), wood, 0.009, 2),
    ]
    chest_base = join(wood_parts, "ChestBase", wood)

    iron_parts = []
    for y in (-0.286, 0.286):
        for z in (0.085, 0.325):
            iron_parts.append(bevelled_box("Band", (0.0, y, z), (1.02, 0.026, 0.038), iron, 0.006, 2))
        for x in (-0.37, 0.37):
            iron_parts.append(bevelled_box("Band", (x, y, 0.205), (0.045, 0.026, 0.30), iron, 0.006, 2))
    for x in (-0.49, 0.49):
        for z in (0.10, 0.31):
            iron_parts.append(bevelled_box("Corner", (x, 0.0, z), (0.036, 0.55, 0.055), iron, 0.006, 1))
    for x in (-0.42, -0.21, 0.21, 0.42):
        for z in (0.095, 0.315):
            bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.014,
                                                 location=(x, -0.307, z))
            rivet = bpy.context.object
            rivet.scale = (1.0, 0.45, 1.0)
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            rivet.data.materials.append(iron)
            iron_parts.append(rivet)
    for side in (-1.0, 1.0):
        points = [
            (side * 0.31, -0.306, 0.10),
            (side * 0.25, -0.306, 0.18),
            (side * 0.17, -0.306, 0.25),
            (side * 0.08, -0.306, 0.29),
        ]
        iron_parts.append(tube_path("PointedMotif", points, 0.010, iron, bevel_resolution=1))
    chest_iron = join(iron_parts, "ChestBaseIron", iron)

    latch_parts = [
        bevelled_box("LatchPlate", (0.0, -0.318, 0.235), (0.085, 0.026, 0.17), iron, 0.008, 2),
        torus("LatchRing", (0.0, -0.346, 0.305), 0.040, 0.010, iron,
              major_segments=20, minor_segments=6, rotation=(math.pi / 2.0, 0.0, 0.0)),
    ]
    latch = join(latch_parts, "Latch", iron)
    socket = bpy.data.objects.new("LanternSocket", None)
    socket.empty_display_type = "PLAIN_AXES"
    socket.location = (0.0, 0.0, 0.18)
    bpy.context.scene.collection.objects.link(socket)
    # This is an authored chest-base anchor, not a renderer staging offset.
    # The lid source's local origin is the rear hinge, so the runtime only
    # supplies an open angle after composing this socket.
    lid_hinge = bpy.data.objects.new("ChestLidHinge", None)
    lid_hinge.empty_display_type = "PLAIN_AXES"
    # Blender +Z becomes glTF +Y and Blender +Y becomes glTF -Z.
    # This exports the released rear pivot as (0, +0.34, -0.286) metres.
    lid_hinge.location = (0.0, 0.286, 0.34)
    bpy.context.scene.collection.objects.link(lid_hinge)
    return [chest_base, chest_iron, latch, socket, lid_hinge]


def build_chest_lid():
    reset_scene()
    wood = textured_material("ChestWood", "chest-wood", 0.03, 0.72)
    iron = textured_material("BlackIron", "chest-iron", 0.92, 0.62)
    lid = barrel_lid(wood)
    iron_parts = []
    for x in (-0.36, 0.0, 0.36):
        points = []
        for index in range(17):
            angle = math.pi * index / 16.0
            points.append((x, -0.28 + 0.286 * math.cos(angle), 0.047 + 0.226 * math.sin(angle)))
        iron_parts.append(tube_path("LidRib", points, 0.014, iron, bevel_resolution=1))
    for y in (-0.55, -0.01):
        iron_parts.append(bevelled_box("LidEdge", (0.0, y, 0.055), (1.01, 0.035, 0.045), iron, 0.006, 2))
    for x in (-0.31, 0.31):
        iron_parts.append(cylinder_between("HingeBar", (x - 0.08, 0.005, 0.045),
                                           (x + 0.08, 0.005, 0.045), 0.020, iron, 12))
    iron_parts.append(bevelled_box("LatchStrike", (0.0, -0.575, 0.080),
                                   (0.095, 0.030, 0.105), iron, 0.007, 2))
    for x in (-0.43, -0.22, 0.22, 0.43):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.013,
                                             location=(x, -0.573, 0.060))
        rivet = bpy.context.object
        rivet.scale = (1.0, 0.45, 1.0)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        rivet.data.materials.append(iron)
        iron_parts.append(rivet)
    for x in (-0.43, -0.22, 0.22, 0.43):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.011,
                                             location=(x, -0.012, 0.060))
        rivet = bpy.context.object
        rivet.scale = (1.0, 0.55, 1.0)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        rivet.data.materials.append(iron)
        iron_parts.append(rivet)
    lid_iron = join(iron_parts, "ChestLidIron", iron)
    return [lid, lid_iron]


def build_lantern_ring():
    reset_scene()
    iron = textured_material("BlackIron", "lantern-iron", 0.94, 0.67)
    ring_parts = [
        torus("GripRingTorus", (0.0, 0.0, 0.085), 0.060, 0.012, iron,
              major_segments=32, minor_segments=8),
        cylinder_between("RingStem", (0.0, 0.0, 0.022), (0.0, 0.0, 0.050), 0.012, iron, 12),
    ]
    grip = join(ring_parts, "GripRing", iron)
    hinge_parts = [
        cylinder_between("HingePin", (-0.035, 0.0, -0.012), (0.035, 0.0, -0.012), 0.016, iron, 12),
        bevelled_box("HingeTab", (0.0, 0.0, 0.005), (0.052, 0.040, 0.040), iron, 0.006, 2),
    ]
    hinge = join(hinge_parts, "Hinge", iron)
    return [grip, hinge]


def face_point(angle, horizontal, z, apothem=0.205):
    radial = Vector((math.cos(angle), math.sin(angle), 0.0))
    tangent = Vector((-math.sin(angle), math.cos(angle), 0.0))
    value = radial * apothem + tangent * horizontal
    return (value.x, value.y, z)


def build_lantern_body():
    reset_scene()
    iron = textured_material("BlackIron", "lantern-iron", 0.94, 0.67)
    glass = simple_material("LanternGlass", (0.96, 0.97, 0.98), 0.0, 0.12, 0.94, 1.52)
    core = simple_material("FlameCore", (0.09, 0.015, 0.002), 0.0, 0.38,
                           emission=(1.0, 0.22, 0.025), emission_strength=6.0)
    iron_parts = [create_frustum("UpperCanopy", -0.25, -0.055, 0.255, 0.048, iron)]
    for z, radius in ((-0.255, 0.245), (-0.295, 0.222), (-0.705, 0.222), (-0.735, 0.205)):
        for side in range(6):
            first = 2.0 * math.pi * side / 6.0 + math.pi / 6.0
            second = 2.0 * math.pi * (side + 1) / 6.0 + math.pi / 6.0
            iron_parts.append(cylinder_between(
                "HexRail",
                (math.cos(first) * radius, math.sin(first) * radius, z),
                (math.cos(second) * radius, math.sin(second) * radius, z),
                0.012, iron, 10))
    for side in range(6):
        angle = 2.0 * math.pi * side / 6.0 + math.pi / 6.0
        x = math.cos(angle) * 0.222
        y = math.sin(angle) * 0.222
        iron_parts.append(cylinder_between("CornerPost", (x, y, -0.292), (x, y, -0.712),
                                           0.013, iron, 10))
        for z in (-0.285, -0.722):
            bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.012,
                                                 location=(x, y, z))
            rivet = bpy.context.object
            rivet.data.materials.append(iron)
            iron_parts.append(rivet)
    for z in (-0.270, -0.748):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=4, radius=0.010,
                                             location=(0.0, -0.235, z))
        rivet = bpy.context.object
        rivet.scale = (1.0, 0.55, 1.0)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        rivet.data.materials.append(iron)
        iron_parts.append(rivet)
    for face in range(6):
        angle = 2.0 * math.pi * face / 6.0 + math.pi / 3.0
        left = [
            face_point(angle, -0.083, -0.675),
            face_point(angle, -0.083, -0.545),
            face_point(angle, -0.072, -0.455),
            face_point(angle, -0.045, -0.385),
            face_point(angle, 0.0, -0.335),
        ]
        right = [face_point(angle, -point[0], point[2]) for point in []]
        iron_parts.append(tube_path("OgeeLeft", left, 0.0085, iron, bevel_resolution=1))
        mirrored = [face_point(angle, -horizontal, z) for horizontal, z in
                    [(-0.083, -0.675), (-0.083, -0.545), (-0.072, -0.455),
                     (-0.045, -0.385), (0.0, -0.335)]]
        iron_parts.append(tube_path("OgeeRight", mirrored, 0.0085, iron, bevel_resolution=1))
        quatrefoil = []
        for index in range(32):
            theta = 2.0 * math.pi * index / 32.0
            radius = 0.031 * (1.0 + 0.34 * math.cos(4.0 * theta))
            quatrefoil.append(face_point(angle, radius * math.cos(theta),
                                         -0.470 + radius * math.sin(theta)))
        iron_parts.append(tube_path("Quatrefoil", quatrefoil, 0.0075, iron,
                                    cyclic=True, bevel_resolution=1))
        iron_parts.append(cylinder_between("WindowMullion",
                                           face_point(angle, 0.0, -0.675),
                                           face_point(angle, 0.0, -0.49), 0.0065, iron, 8))
    iron_parts.extend([
        create_frustum("LowerCanopy", -0.810, -0.735, 0.060, 0.205, iron),
        cylinder_between("LowerFinial", (0.0, 0.0, -0.925), (0.0, 0.0, -0.800), 0.018, iron, 12),
        create_frustum("FinialPoint", -0.975, -0.915, 0.0, 0.035, iron),
        cylinder_between("HingeStem", (0.0, 0.0, -0.055), (0.0, 0.0, -0.015), 0.014, iron, 12),
    ])
    body = join(iron_parts, "LanternBody", iron)

    glass_parts = []
    # Keep the closed glass volume wholly behind the front-mounted iron
    # tracery. The old 0.194 m centre put the 7 mm slab's outer face at
    # 0.1975 m while the 8.5 mm tracery reached inward to 0.1965 m, embedding
    # opaque triangles inside the medium. A 0.190 m centre retains the pane's
    # physical thickness with 3 mm radial clearance to the cage.
    apothem = 0.190
    for face in range(6):
        angle = 2.0 * math.pi * face / 6.0 + math.pi / 3.0
        bpy.ops.mesh.primitive_cube_add(location=(math.cos(angle) * apothem,
                                                  math.sin(angle) * apothem, -0.505),
                                        rotation=(0.0, 0.0, angle - math.pi / 2.0))
        pane = bpy.context.object
        pane.name = "GlassPane"
        pane.dimensions = (0.185, 0.007, 0.345)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        pane.data.materials.append(glass)
        glass_parts.append(pane)
    lantern_glass = join(glass_parts, "LanternGlass", glass)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=1.0, location=(0.0, 0.0, -0.545))
    flame = bpy.context.object
    flame.name = "FlameCore"
    flame.scale = (0.048, 0.048, 0.125)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    flame.data.materials.append(core)
    triangulate = flame.modifiers.new("RuntimeTriangles", "TRIANGULATE")
    apply_modifier(flame, triangulate)
    uv_smart(flame)

    flame_socket = bpy.data.objects.new("Flame", None)
    flame_socket.empty_display_type = "PLAIN_AXES"
    flame_socket.location = (0.0, 0.0, -0.545)
    bpy.context.scene.collection.objects.link(flame_socket)
    light_socket = bpy.data.objects.new("Light", None)
    light_socket.empty_display_type = "PLAIN_AXES"
    light_socket.location = (0.0, 0.0, -0.515)
    bpy.context.scene.collection.objects.link(light_socket)
    return [body, lantern_glass, flame, flame_socket, light_socket]


def build_all():
    chest_base_objects = build_chest_base()
    chest_base_report = export_component(
        "gothic-chest-base", chest_base_objects,
        CHEST_SOURCE / "gothic-chest-base-cleaned-source.glb",
        RUNTIME / "gothic-chest-base" / "gothic-chest-base-lod0.runtime.glb",
        {"sourceLineage": "Meshy accepted chest candidate 01a03f69-2b15-7c6b-b17f-de4741cdafba"})

    chest_lid_objects = build_chest_lid()
    chest_lid_report = export_component(
        "gothic-chest-lid", chest_lid_objects,
        CHEST_SOURCE / "gothic-chest-lid-cleaned-source.glb",
        RUNTIME / "gothic-chest-lid" / "gothic-chest-lid-lod0.runtime.glb",
        {"pivot": "local origin is the rear hinge; lid extends toward +Z after glTF axis conversion"})

    lantern_ring_objects = build_lantern_ring()
    lantern_ring_report = export_component(
        "reward-lantern-ring", lantern_ring_objects,
        LANTERN_SOURCE / "reward-lantern-ring-cleaned-source.glb",
        RUNTIME / "reward-lantern-ring" / "reward-lantern-ring-lod0.runtime.glb",
        {"sourceLineage": "Meshy accepted lantern candidate 01a03f6b-e99a-7ee2-9738-d6d236f89c2b"})

    lantern_body_objects = build_lantern_body()
    lantern_body_report = export_component(
        "reward-lantern-body", lantern_body_objects,
        LANTERN_SOURCE / "reward-lantern-body-cleaned-source.glb",
        RUNTIME / "reward-lantern-body" / "reward-lantern-body-lod0.runtime.glb",
        {"pivot": "local origin is Hinge; body extends down -Y",
         "glass": "six separate closed 7 mm outward-wound panes; KHR transmission/volume/IOR/attenuation",
         "removed": "Meshy interior bulb/flame-like geometry and all source pane fill were removed"})

    aggregate = {
        "chestTriangles": chest_base_report["triangles"] + chest_lid_report["triangles"],
        "lanternTriangles": lantern_ring_report["triangles"] + lantern_body_report["triangles"],
    }
    print(json.dumps(aggregate))


build_all()
