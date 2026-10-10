"""Build the original Horde tomb dressing candidate family with Blender.

Run from the repository root:
  blender --background --factory-startup --python tools/author-tomb-dressing.py

The build is deterministic: no random numbers, external assets, textures, paid
generation or environment-dependent input is used. All dimensions are metres.
"""
from __future__ import annotations

import hashlib
import json
import math
import struct
from pathlib import Path

import bpy
from mathutils import Vector, Matrix


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/models/world/source/tomb-dressing-v01"
RUNTIME = ROOT / "assets/models/world/runtime/tomb-dressing-v01"
BLEND_PATH = SOURCE / "tomb-dressing-v01.blend"

STONE = "MedievalWall02"
BONE = "TombBone"
WAX = "TombWax"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.collections, bpy.data.meshes, bpy.data.curves,
                       bpy.data.materials):
        for block in list(datablocks):
            if block.users == 0 and block.name != "Collection":
                datablocks.remove(block)
    scene = bpy.context.scene
    for child in list(scene.collection.children):
        scene.collection.children.unlink(child)


def collection(name):
    result = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(result)
    return result


def material(name, colour, roughness=0.84):
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*colour, 1.0)
    result.use_nodes = True
    bsdf = result.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*colour, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Emission Color"].default_value = (0.0, 0.0, 0.0, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 0.0
    return result


def move_to_collection(obj, target):
    for current in list(obj.users_collection):
        current.objects.unlink(obj)
    target.objects.link(obj)


def bevelled_box(name, location, dimensions, mat, target, bevel=0.012):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    if bevel > 0:
        modifier = obj.modifiers.new("Hand softened arrises", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    move_to_collection(obj, target)
    return obj


def ellipsoid(name, location, scale, mat, target, segments=20, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings,
                                         radius=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    for face in obj.data.polygons:
        face.use_smooth = True
    move_to_collection(obj, target)
    return obj


def torus(name, location, major, minor, mat, target, rotation=(math.pi / 2, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(major_segments=24, minor_segments=8,
                                    major_radius=major, minor_radius=minor,
                                    location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    for face in obj.data.polygons:
        face.use_smooth = True
    move_to_collection(obj, target)
    return obj


def custom_mesh(name, vertices, faces, mat, target, smooth=False):
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    target.objects.link(obj)
    obj.data.materials.append(mat)
    deselect_all()
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.object.mode_set(mode="OBJECT")
    # Texture-free assets still need UV0 for the production static GLB contract.
    # Project directly from stable local coordinates; no stochastic island packing.
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for polygon in mesh.polygons:
        for loop_index in polygon.loop_indices:
            vertex = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            uv_layer.data[loop_index].uv = (vertex.x, vertex.z)
    if smooth:
        for face in obj.data.polygons:
            face.use_smooth = True
    return obj


def arch_stone(name, theta0, theta1, inner, outer, zc, front_y, depth,
               mat, target):
    points = [(inner * math.cos(theta0), zc + inner * math.sin(theta0)),
              (outer * math.cos(theta0), zc + outer * math.sin(theta0)),
              (outer * math.cos(theta1), zc + outer * math.sin(theta1)),
              (inner * math.cos(theta1), zc + inner * math.sin(theta1))]
    # Preserve front-face winding across the left half of the arch.
    if (theta0 + theta1) / 2 > math.pi / 2:
        points.reverse()
    vertices = [(x, y, z) for y in (front_y, front_y + depth)
                for x, z in points]
    faces = [(0, 1, 2, 3), (7, 6, 5, 4), (0, 4, 5, 1),
             (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
    obj = custom_mesh(name, vertices, faces, mat, target)
    bevel = obj.modifiers.new("Worn voussoir edges", "BEVEL")
    bevel.width = 0.003
    bevel.segments = 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj


def make_rect_niche(target, stone):
    # Overall module: 1.28 W x .54 D x 1.60 H; real .30 m deep opening.
    bevelled_box("rect_rear_wall", (0, 0.21, 0.80), (1.28, 0.12, 1.60),
                 stone, target, 0.008)
    bevelled_box("rect_left_mass", (-0.49, -0.025, 0.80), (0.30, 0.39, 1.60),
                 stone, target, 0.012)
    bevelled_box("rect_right_mass", (0.49, -0.025, 0.80), (0.30, 0.39, 1.60),
                 stone, target, 0.012)
    bevelled_box("rect_lintel", (0, -0.025, 1.3425), (0.68, 0.39, 0.515),
                 stone, target, 0.014)
    bevelled_box("rect_sill", (0, -0.025, 0.12), (0.68, 0.39, 0.24),
                 stone, target, 0.018)
    # The returns create visible, ray-occluding side walls and a shelf, not a quad.
    bevelled_box("rect_left_reveal", (-0.34, 0.01, 0.66), (0.035, 0.29, 0.84),
                 stone, target, 0.006)
    bevelled_box("rect_right_reveal", (0.34, 0.01, 0.66), (0.035, 0.29, 0.84),
                 stone, target, 0.006)
    bevelled_box("rect_cavity_ceiling", (0, 0.01, 1.085), (0.68, 0.29, 0.035),
                 stone, target, 0.006)
    bevelled_box("rect_shelf", (0, -0.015, 0.255), (0.70, 0.34, 0.055),
                 stone, target, 0.009)
    bevelled_box("rect_jamb_left", (-0.352, -0.23, 0.67), (0.055, 0.035, 0.83),
                 stone, target, 0.009)
    bevelled_box("rect_jamb_right", (0.352, -0.23, 0.67), (0.055, 0.035, 0.83),
                 stone, target, 0.009)
    return target


def make_arch_niche(target, stone):
    # Same outer envelope as the rectangular module, with an actual arched void.
    bevelled_box("arch_rear_wall", (0, 0.21, 0.80), (1.28, 0.12, 1.60),
                 stone, target, 0.008)
    bevelled_box("arch_left_mass", (-0.50, -0.025, 0.80), (0.28, 0.39, 1.60),
                 stone, target, 0.012)
    bevelled_box("arch_right_mass", (0.50, -0.025, 0.80), (0.28, 0.39, 1.60),
                 stone, target, 0.012)
    bevelled_box("arch_lower_mass", (0, -0.025, 0.12), (0.74, 0.39, 0.24),
                 stone, target, 0.014)
    # Opening has .66 m spring width and a .33 m radius vaulted head.
    spring_z, radius = 0.91, 0.33
    for segment in range(7):
        a0 = segment * math.pi / 7
        a1 = (segment + 1) * math.pi / 7
        arch_stone(f"arch_voussoir_{segment + 1:02d}", a0, a1,
                   radius, radius + 0.12, spring_z, -0.22, 0.39,
                   stone, target)
    # Actual side/top returns and the recessed back plane make the void 0.30 m deep.
    bevelled_box("arch_left_reveal", (-0.33, 0.01, 0.59), (0.035, 0.29, 0.66),
                 stone, target, 0.006)
    bevelled_box("arch_right_reveal", (0.33, 0.01, 0.59), (0.035, 0.29, 0.66),
                 stone, target, 0.006)
    bevelled_box("arch_shelf", (0, -0.015, 0.255), (0.70, 0.34, 0.055),
                 stone, target, 0.009)
    bevelled_box("arch_jamb_left", (-0.343, -0.23, 0.58), (0.055, 0.035, 0.68),
                 stone, target, 0.009)
    bevelled_box("arch_jamb_right", (0.343, -0.23, 0.58), (0.055, 0.035, 0.68),
                 stone, target, 0.009)
    return target


def make_skull(target, bone):
    # A compact 0.33 m skull with open, inward-cupped orbital sockets.
    ellipsoid("skull_cranium", (0, 0.025, 0.245), (0.138, 0.145, 0.155),
              bone, target, 24, 16)
    ellipsoid("skull_occiput", (0, 0.112, 0.205), (0.112, 0.080, 0.116),
              bone, target, 20, 12)
    for side in (-1, 1):
        x = side * 0.061
        # Socket cup is an open bowl of narrowing rings, recessed toward the cranium.
        vertices, faces = [], []
        rings = [(-0.132, 0.043), (-0.108, 0.036), (-0.082, 0.024),
                 (-0.060, 0.010)]
        sides = 20
        for y, radius in rings:
            for i in range(sides):
                angle = 2 * math.pi * i / sides
                vertices.append((x + radius * math.cos(angle), y,
                                 0.247 + radius * math.sin(angle)))
        for ring in range(len(rings) - 1):
            for i in range(sides):
                a = ring * sides + i
                b = ring * sides + (i + 1) % sides
                c = (ring + 1) * sides + (i + 1) % sides
                d = (ring + 1) * sides + i
                faces.append((a, b, c, d))
        faces.append(tuple(range((len(rings) - 1) * sides,
                                 len(rings) * sides)))
        custom_mesh("skull_orbit_cup_L" if side < 0 else "skull_orbit_cup_R",
                    vertices, faces, bone, target, smooth=True)
        torus("skull_orbital_rim_L" if side < 0 else "skull_orbital_rim_R",
              (x, -0.134, 0.247), 0.042, 0.006, bone, target)
        ellipsoid("skull_cheek_L" if side < 0 else "skull_cheek_R",
                  (side * 0.083, -0.078, 0.169), (0.050, 0.035, 0.037),
                  bone, target, 16, 10)
        ellipsoid("skull_brow_L" if side < 0 else "skull_brow_R",
                  (x, -0.119, 0.302), (0.062, 0.023, 0.016), bone, target, 16, 8)
    # Nasal bridge and open triangular nasal aperture; no dark decal or backing quad.
    ellipsoid("skull_nasal_bridge", (0, -0.126, 0.215),
              (0.020, 0.024, 0.052), bone, target, 14, 8)
    for side in (-1, 1):
        ellipsoid("skull_upper_jaw_arch_L" if side < 0 else "skull_upper_jaw_arch_R",
                  (side * 0.039, -0.106, 0.113), (0.047, 0.027, 0.020),
                  bone, target, 16, 8)
        ellipsoid("skull_mandible_L" if side < 0 else "skull_mandible_R",
                  (side * 0.067, -0.025, 0.070), (0.026, 0.062, 0.029),
                  bone, target, 16, 8)
    ellipsoid("skull_mandible_front", (0, -0.084, 0.070),
              (0.077, 0.028, 0.023), bone, target, 20, 8)
    # Individually shaped, low-profile tooth crowns remain part of the skull family.
    for i in range(8):
        x = (i - 3.5) * 0.014
        y = -0.127 + 0.010 * (abs(i - 3.5) / 3.5)
        ellipsoid(f"skull_tooth_{i + 1:02d}", (x, y, 0.118),
                  (0.0055, 0.006, 0.008), bone, target, 8, 6)
    return target


def make_femur(target, bone):
    # A curved shaft and enlarged articular ends, 0.39 m long.
    points = [(-0.13, 0.031), (-0.112, 0.044), (-0.082, 0.039),
              (-0.045, 0.024), (0.018, 0.019), (0.076, 0.024),
              (0.112, 0.038), (0.139, 0.035), (0.154, 0.022)]
    n = 20
    vertices, faces = [], []
    for z, radius in points:
        cx = 0.012 * math.sin((z + 0.13) * 8)
        cy = 0.006 * math.sin((z + 0.13) * 5)
        for i in range(n):
            a = 2 * math.pi * i / n
            vertices.append((cx + radius * math.cos(a), cy + radius * math.sin(a), z))
    for ring in range(len(points) - 1):
        for i in range(n):
            a, b = ring * n + i, ring * n + (i + 1) % n
            c, d = (ring + 1) * n + (i + 1) % n, (ring + 1) * n + i
            faces.append((a, b, c, d))
    faces.append(tuple(range(n - 1, -1, -1)))
    faces.append(tuple((len(points) - 1) * n + i for i in range(n)))
    custom_mesh("femur_shaft", vertices, faces, bone, target, smooth=True)
    ellipsoid("femur_head", (-0.027, 0.0, 0.163), (0.049, 0.047, 0.050),
              bone, target, 18, 12)
    ellipsoid("femur_condyle_L", (-0.033, 0, -0.133), (0.039, 0.042, 0.038),
              bone, target, 16, 10)
    ellipsoid("femur_condyle_R", (0.039, 0, -0.133), (0.039, 0.042, 0.038),
              bone, target, 16, 10)
    return target


def make_rib_bundle(target, bone):
    # Three hollow U-like ribs attach to a short vertebral column. No enemy rigging.
    for rib_index, z in enumerate((0.27, 0.17, 0.07)):
        radius = 0.009
        half_width = 0.105 - rib_index * 0.008
        points = []
        for step in range(17):
            t = step / 16
            angle = math.pi * t
            x = -half_width * math.cos(angle)
            y = -0.008 - 0.035 * math.sin(angle)
            rz = z - 0.047 * math.sin(angle)
            points.append(Vector((x, y, rz)))
        n = 8
        vertices, faces = [], []
        for index, point in enumerate(points):
            tangent = (points[min(index + 1, 16)] - points[max(index - 1, 0)]).normalized()
            ref = Vector((0, 1, 0))
            normal = tangent.cross(ref).normalized()
            binormal = tangent.cross(normal).normalized()
            taper = 0.72 + 0.28 * math.sin(math.pi * (index / 16))
            for side in range(n):
                angle = 2 * math.pi * side / n
                p = point + radius * taper * (math.cos(angle) * normal + math.sin(angle) * binormal)
                vertices.append(tuple(p))
        for ring in range(16):
            for side in range(n):
                a, b = ring * n + side, ring * n + (side + 1) % n
                c, d = (ring + 1) * n + (side + 1) % n, (ring + 1) * n + side
                faces.append((a, b, c, d))
        custom_mesh(f"rib_{rib_index + 1:02d}", vertices, faces, bone, target, smooth=True)
    for index, z in enumerate((0.27, 0.17, 0.07)):
        ellipsoid(f"vertebra_{index + 1:02d}", (0, 0.015, z),
                  (0.026, 0.030, 0.027), bone, target, 12, 8)
    return target


def make_cold_candle(target, wax, bone):
    # Restrained original non-emissive candle geometry for a grouped offering set.
    profile = [(0.0, 0.0), (0.034, 0.0), (0.036, 0.014),
               (0.032, 0.095), (0.030, 0.112), (0.025, 0.117),
               (0.017, 0.105), (0.008, 0.111), (0.0, 0.103)]
    vertices, faces = [], []
    sides = 20
    for radius, z in profile:
        for i in range(sides):
            a = 2 * math.pi * i / sides
            vertices.append((radius * math.cos(a), radius * math.sin(a), z))
    for ring in range(len(profile) - 1):
        for i in range(sides):
            a, b = ring * sides + i, ring * sides + (i + 1) % sides
            c, d = (ring + 1) * sides + (i + 1) % sides, (ring + 1) * sides + i
            faces.append((a, b, c, d))
    custom_mesh("cold_beeswax_stub", vertices, faces, wax, target, smooth=True)
    ellipsoid("cold_charred_wick", (0, 0, 0.111), (0.003, 0.003, 0.013),
              bone, target, 8, 6)
    return target


def create_assets(stone, bone, wax):
    definitions = {}
    rect = collection("TombNiche_Rectangular_RealRecess")
    make_rect_niche(rect, stone)
    definitions["tomb-niche-rect"] = (rect, "A rect-headed recessed burial alcove with stone returns and load-bearing shelf.")

    arch = collection("TombNiche_Arched_RealRecess")
    make_arch_niche(arch, stone)
    definitions["tomb-niche-arched"] = (arch, "A voussoir-framed arched burial alcove with deep side returns and a stone shelf.")

    skull = collection("TombSkull_Original")
    make_skull(skull, bone)
    definitions["tomb-skull"] = (skull, "Compact original human skull with open inset orbital cups, nasal aperture and separate mandible.")

    femur = collection("TombBone_Femur_Original")
    make_femur(femur, bone)
    # Place this reusable fragment horizontally on its base, with the origin above the floor.
    lying = Matrix.Translation(Vector((0.0, 0.0, 0.078))) @ Matrix.Rotation(math.pi / 2, 4, "Y")
    for obj in femur.objects:
        obj.matrix_world = lying @ obj.matrix_world
    definitions["tomb-bone-femur"] = (femur, "Reusable original femur with shaped shaft and paired condyles.")

    ribs = collection("TombBone_RibBundle_Original")
    make_rib_bundle(ribs, bone)
    definitions["tomb-bone-rib-bundle"] = (ribs, "Reusable three-rib bundle around vertebrae, arranged as a funerary fragment.")

    group = collection("Funerary_Group_Original_Skull_Bones_Cold_Candle")
    # The group uses fresh authored forms in a measured tabletop arrangement.
    group_objects = []
    for create, mat in ((make_skull, bone), (make_femur, bone), (make_rib_bundle, bone)):
        sub = collection("_temporary_group_component")
        create(sub, mat)
        for obj in list(sub.objects):
            group_objects.append(obj)
            move_to_collection(obj, group)
        bpy.data.collections.remove(sub)
    # Local transforms keep the floor centered; rotations remain baked at export.
    for obj in group.objects:
        if obj.name.startswith("skull_"):
            obj.location.x += 0.22
            obj.location.y -= 0.015
            obj.location.z += 0.025
        elif obj.name.startswith("femur_"):
            laying = Matrix.Translation(Vector((-0.20, -0.02, 0.10))) @ Matrix.Rotation(math.pi / 2, 4, "Y")
            obj.matrix_world = laying @ obj.matrix_world
        elif obj.name.startswith("rib_") or obj.name.startswith("vertebra_"):
            obj.location.x -= 0.21
            obj.location.y += 0.09
            obj.location.z += 0.045
    candle = collection("_temporary_group_candle")
    make_cold_candle(candle, wax, bone)
    for obj in list(candle.objects):
        obj.location.x -= 0.37
        obj.location.y -= 0.10
        obj.location.z += 0.05
        move_to_collection(obj, group)
    bpy.data.collections.remove(candle)
    bevelled_box("group_low_limestone_plinth", (0, 0.10, 0.025),
                 (0.92, 0.44, 0.05), stone, group, 0.012)
    definitions["funerary-group"] = (group, "Compact grouped funerary dressing: skull, femur, ribs, and one extinguished non-emissive candle on a low stone plinth.")
    return definitions


def deselect_all():
    bpy.ops.object.select_all(action="DESELECT")


def canonicalize_glb(path):
    """Sort vertex rows and triangles so Blender's unordered exporter buffers hash reproducibly."""
    payload = path.read_bytes()
    if payload[:4] != b"glTF":
        raise RuntimeError(f"{path.name} is not a GLB")
    json_length, json_type = struct.unpack_from("<II", payload, 12)
    if json_type != 0x4E4F534A:
        raise RuntimeError(f"{path.name} has no leading JSON chunk")
    document = json.loads(payload[20:20 + json_length].decode("utf-8"))
    bin_header = 20 + json_length
    bin_length, bin_type = struct.unpack_from("<II", payload, bin_header)
    if bin_type != 0x004E4942:
        raise RuntimeError(f"{path.name} has no binary chunk")
    original_bin = payload[bin_header + 8:bin_header + 8 + bin_length]
    views = document["bufferViews"]
    accessors = document["accessors"]
    rewritten_views = {}
    rewritten_accessor_offsets = set()

    component_formats = {5121: ("<B", 1), 5123: ("<H", 2),
                         5125: ("<I", 4), 5126: ("<f", 4)}
    component_counts = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}

    def read_rows(accessor_index):
        accessor = accessors[accessor_index]
        view_index = accessor["bufferView"]
        view = views[view_index]
        fmt, width = component_formats[accessor["componentType"]]
        components = component_counts[accessor["type"]]
        row_size = width * components
        stride = view.get("byteStride", row_size)
        start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
        rows = [original_bin[start + i * stride:start + i * stride + row_size]
                for i in range(accessor["count"])]
        if any(len(row) != row_size for row in rows):
            raise RuntimeError(f"{path.name} has a truncated accessor")
        return view_index, rows, fmt, width

    for mesh in document.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            attributes = sorted(primitive["attributes"].items())
            row_sets = []
            for semantic, accessor_index in attributes:
                view_index, rows, _, _ = read_rows(accessor_index)
                row_sets.append((semantic, accessor_index, view_index, rows))
            vertex_count = len(row_sets[0][3])
            if any(len(rows) != vertex_count for _, _, _, rows in row_sets):
                raise RuntimeError(f"{path.name} has mismatched vertex attributes")
            keys = [b"".join(rows[i] for _, _, _, rows in row_sets)
                    for i in range(vertex_count)]
            ordered_keys = sorted(set(keys))
            key_to_index = {key: index for index, key in enumerate(ordered_keys)}
            remap = [key_to_index[key] for key in keys]
            for _, accessor_index, view_index, rows in row_sets:
                canonical_rows = [rows[key_to_index[key]] for key in ordered_keys]
                if view_index in rewritten_views:
                    raise RuntimeError(f"{path.name} shares a vertex bufferView unexpectedly")
                rewritten_views[view_index] = b"".join(canonical_rows)
                accessors[accessor_index]["count"] = len(ordered_keys)
                accessors[accessor_index]["byteOffset"] = 0
                rewritten_accessor_offsets.add(accessor_index)

            index_accessor_index = primitive["indices"]
            index_view, index_rows, index_fmt, index_width = read_rows(index_accessor_index)
            old_indices = [struct.unpack(index_fmt, row)[0] for row in index_rows]
            if len(old_indices) % 3:
                raise RuntimeError(f"{path.name} has a non-triangle index count")
            triangles = []
            for offset in range(0, len(old_indices), 3):
                tri = tuple(remap[index] for index in old_indices[offset:offset + 3])
                rotations = (tri, (tri[1], tri[2], tri[0]), (tri[2], tri[0], tri[1]))
                triangles.append(min(rotations))
            triangles.sort()
            sorted_indices = [index for triangle in triangles for index in triangle]
            rewritten_views[index_view] = b"".join(struct.pack(index_fmt, index)
                                                       for index in sorted_indices)
            accessors[index_accessor_index]["byteOffset"] = 0
            rewritten_accessor_offsets.add(index_accessor_index)

    canonical_bin = bytearray()
    for view_index, view in enumerate(views):
        while len(canonical_bin) % 4:
            canonical_bin.append(0)
        data = rewritten_views.get(view_index)
        if data is None:
            start = view.get("byteOffset", 0)
            end = start + view["byteLength"]
            data = original_bin[start:end]
        view["byteOffset"] = len(canonical_bin)
        view["byteLength"] = len(data)
        if view_index in rewritten_views:
            view.pop("byteStride", None)
        canonical_bin.extend(data)
    document["buffers"][0]["byteLength"] = len(canonical_bin)
    json_chunk = json.dumps(document, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    while len(json_chunk) % 4:
        json_chunk += b" "
    while len(canonical_bin) % 4:
        canonical_bin.append(0)
    output = bytearray(b"glTF")
    output.extend(struct.pack("<II", 2, 12 + 8 + len(json_chunk) + 8 + len(canonical_bin)))
    output.extend(struct.pack("<II", len(json_chunk), 0x4E4F534A))
    output.extend(json_chunk)
    output.extend(struct.pack("<II", len(canonical_bin), 0x004E4942))
    output.extend(canonical_bin)
    path.write_bytes(output)


def export_collection(name, target, output):
    deselect_all()
    ordered_objects = sorted(target.objects, key=lambda obj: obj.name)
    for obj in ordered_objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = ordered_objects[0]
    output.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=str(output), export_format="GLB",
                              use_selection=True, export_apply=True,
                              export_texcoords=True, export_normals=True,
                              export_tangents=False, export_materials="EXPORT",
                              export_yup=True, export_lights=False,
                              export_cameras=False, export_animations=False,
                              export_extras=False)
    canonicalize_glb(output)


def mesh_summary(collection_obj):
    triangles = 0
    vertices = 0
    materials = set()
    mins = [math.inf, math.inf, math.inf]
    maxs = [-math.inf, -math.inf, -math.inf]
    for obj in sorted(collection_obj.objects, key=lambda item: item.name):
        if obj.type != "MESH":
            continue
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        triangles += sum(max(0, len(poly.vertices) - 2) for poly in mesh.polygons)
        vertices += len(mesh.vertices)
        for mat in obj.data.materials:
            if mat:
                materials.add(mat.name)
        for vertex in mesh.vertices:
            p = obj.matrix_world @ vertex.co
            for axis in range(3):
                mins[axis] = min(mins[axis], p[axis])
                maxs[axis] = max(maxs[axis], p[axis])
        evaluated.to_mesh_clear()
    return {"triangles": triangles, "vertices": vertices,
            "materials": sorted(materials), "bounds_blender_xyz_m": {
                "min": [round(v, 6) for v in mins],
                "max": [round(v, 6) for v in maxs],
                "size": [round(maxs[i] - mins[i], 6) for i in range(3)]}}


def write_manifests(definitions, source_hash, script_hash):
    for asset_id, (group, role) in definitions.items():
        stats = mesh_summary(group)
        directory = RUNTIME / asset_id
        directory.mkdir(parents=True, exist_ok=True)
        glb = directory / f"{asset_id}-lod0.runtime.glb"
        export_collection(asset_id, group, glb)
        max_triangles = max(stats["triangles"], 1)
        max_vertices = max(stats["vertices"], stats["triangles"] * 3)
        materials = stats["materials"]
        manifest = {
            "schema": 1, "asset": asset_id + "-lod0", "metresPerUnit": 1.0,
            "coordinateSystem": {"up": "+Y", "forward": "+Z"},
            "budgets": {"maxVertices": max_vertices, "maxIndices": max_triangles * 3,
                        "maxPrimitives": max(len(group.objects), 1),
                        "maxMaterials": len(materials), "maxTextureLayersPerKind": 1},
            "lods": [{"name": "lod0", "maxTriangles": max_triangles}],
            "requiredSockets": [],
            "runtimeTextureProfile": {"android": "astc", "windows": "rgba8",
                                      "mipmapped": True, "resolution": 1024},
            "materialOverrides": [], "distribution": "candidate",
            "licenceStatus": "Original project-authored Blender geometry and PBR parameters; no third-party model, texture, font, or provider bytes."
        }
        (directory / "asset.manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        receipt = {
            "schema": 1, "asset": asset_id, "stage": "authored candidate; runtime admission pending",
            "role": role, "sourceBlend": "../../../source/tomb-dressing-v01/tomb-dressing-v01.blend",
            "sourceBlendSha256": source_hash, "authoringScriptSha256": script_hash,
            "triangleCount": stats["triangles"], "vertexCount": stats["vertices"],
            "materials": materials, "boundsBlenderXyzMetres": stats["bounds_blender_xyz_m"],
            "runtimeGlbSha256": hashlib.sha256(glb.read_bytes()).hexdigest(),
            "runtimeGlbBytes": glb.stat().st_size, "textureCount": 0,
            "texturesOrExternalDependencies": False
        }
        (directory / "candidate-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")


def render_previews(definitions):
    previews = SOURCE / "previews"
    previews.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 20
    scene.render.resolution_x = 960
    scene.render.resolution_y = 640
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "AgX"
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    if scene.world is None:
        scene.world = bpy.data.worlds.new("TombPreviewWorld")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.045, 0.050, 0.052, 1.0)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.25
    stage = bpy.data.collections.new("_PreviewOnly")
    scene.collection.children.link(stage)
    preview_assets = []
    for obj in scene.objects:
        if obj.type == "MESH":
            obj.hide_render = True

    def duplicate_group(asset_id, transform):
        source_group = definitions[asset_id][0]
        for original in source_group.objects:
            clone = original.copy()
            if original.data is not None:
                clone.data = original.data
            stage.objects.link(clone)
            preview_assets.append(clone)
            clone.matrix_world = transform @ original.matrix_world
            clone.hide_render = False

    ground_material = material("PreviewGround", (0.105, 0.112, 0.106), 0.95)
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -0.012))
    ground = bpy.context.object
    ground.name = "_PreviewGround"
    ground.data.materials.append(ground_material)
    move_to_collection(ground, stage)
    ground.hide_render = False
    bpy.ops.object.camera_add(location=(2.8, -5.0, 2.35))
    camera = bpy.context.object
    camera.name = "_PreviewCamera"
    stage.objects.link(camera)
    for current in list(camera.users_collection):
        if current != stage:
            current.objects.unlink(camera)
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 3.55
    target = Vector((0.0, 0.0, 0.76))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = camera
    for position, power, size in (((-2.6, -3.2, 4.2), 520.0, 3.0),
                                  ((2.8, -1.0, 2.3), 260.0, 2.0),
                                  ((0.0, 2.0, 3.5), 380.0, 2.0)):
        bpy.ops.object.light_add(type="AREA", location=position)
        light = bpy.context.object
        light.name = "_PreviewAreaLight"
        stage.objects.link(light)
        for current in list(light.users_collection):
            if current != stage:
                current.objects.unlink(light)
        light.data.energy = power
        light.data.shape = "DISK"
        light.data.size = size
        light.rotation_euler = (Vector((0, 0, 0.6)) - light.location).to_track_quat("-Z", "Y").to_euler()

    niche_x = Matrix.Translation(Vector((-0.72, 0.0, 0.0)))
    niche_y = Matrix.Translation(Vector((0.72, 0.0, 0.0)))
    duplicate_group("tomb-niche-rect", niche_x)
    duplicate_group("tomb-niche-arched", niche_y)
    scene.render.filepath = str(previews / "tomb-niche-family.png")
    bpy.ops.render.render(write_still=True)

    for obj in preview_assets:
        if obj.name in bpy.data.objects:
            stage.objects.unlink(obj)
            bpy.data.objects.remove(obj)
    preview_assets.clear()
    camera.location = Vector((0.45, -3.7, 1.10))
    camera.data.ortho_scale = 2.65
    target = Vector((0.45, 0.0, 0.17))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    duplicate_group("tomb-skull", Matrix.Translation(Vector((-0.50, 0.0, 0.0))))
    duplicate_group("tomb-bone-femur", Matrix.Translation(Vector((0.08, 0.0, 0.0))))
    duplicate_group("tomb-bone-rib-bundle", Matrix.Translation(Vector((0.53, 0.0, 0.0))))
    duplicate_group("funerary-group", Matrix.Translation(Vector((1.20, 0.0, 0.0))))
    scene.render.filepath = str(previews / "tomb-bone-family.png")
    bpy.ops.render.render(write_still=True)
    print("TOMB_DRESSING_PREVIEWS=" + str(previews))


def main():
    SOURCE.mkdir(parents=True, exist_ok=True)
    RUNTIME.mkdir(parents=True, exist_ok=True)
    clear_scene()
    stone = material(STONE, (0.34, 0.37, 0.34), 0.92)
    bone = material(BONE, (0.68, 0.61, 0.46), 0.78)
    wax = material(WAX, (0.57, 0.48, 0.31), 0.84)
    definitions = create_assets(stone, bone, wax)
    deselect_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    source_hash = hashlib.sha256(BLEND_PATH.read_bytes()).hexdigest()
    script_hash = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    write_manifests(definitions, source_hash, script_hash)
    render_previews(definitions)
    print("TOMB_DRESSING_SOURCE_SHA256=" + source_hash)
    print("TOMB_DRESSING_SCRIPT_SHA256=" + script_hash)
    for asset_id, (group, _) in definitions.items():
        print("TOMB_DRESSING_ASSET=" + json.dumps({"asset": asset_id, **mesh_summary(group)}))


if __name__ == "__main__":
    main()
