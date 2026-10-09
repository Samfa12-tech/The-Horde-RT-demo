"""Author the original player sword sheath source and texture-free-size GLB."""

from __future__ import annotations

import bpy
import math
import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/models/props/source/player-sword-scabbard"
TEXTURES = ROOT / "assets/textures/props/source/sword-scabbard"
SHEATH = "PlayerSwordScabbard"


def make_mesh() -> bpy.types.Object:
    # A closed, tapered outer shell. The top remains open around the sword guard;
    # the distal cap extends beyond the measured sword tip, hiding its point.
    profile = [
        (0.300, 0.0440, 0.0225),
        (0.315, 0.0435, 0.0220),
        (0.345, 0.0415, 0.0210),
        (0.390, 0.0405, 0.0205),
        (0.520, 0.0390, 0.0195),
        (0.700, 0.0375, 0.0185),
        (0.900, 0.0345, 0.0165),
        (1.035, 0.0300, 0.0145),
        (1.065, 0.0270, 0.0135),
        (1.074, 0.0200, 0.0100),
    ]
    segments = 16
    verts: list[tuple[float, float, float]] = []
    faces: list[tuple[int, ...]] = []
    uv_faces: list[list[tuple[float, float]]] = []
    for y, rx, rz in profile:
        for segment in range(segments):
            angle = math.tau * segment / segments
            # Mildly flattened rounded ellipse follows the double-edged blade.
            # Blender exports its local +Z as glTF +Y. Keep the sheath axis
            # aligned with the runtime manifest's declared +Y contract instead
            # of exporting the earlier Blender +Y axis as glTF -Z.
            verts.append((rx * math.cos(angle), -rz * math.sin(angle), y))

    # The authored map is an atlas: leather at U<.735 and iron at U>.765.
    # U walks within that material region around the sheath, while V follows
    # the sword axis so the procedural grain runs lengthwise.
    for row in range(len(profile) - 1):
        y0, _, _ = profile[row]
        y1, _, _ = profile[row + 1]
        is_iron = (y0 < 0.355) or (0.955 <= y0 < 1.005) or y0 >= 1.035
        for segment in range(segments):
            nxt = (segment + 1) % segments
            a = row * segments + segment
            b = row * segments + nxt
            c = (row + 1) * segments + nxt
            d = (row + 1) * segments + segment
            faces.append((a, d, c, b))
            u_start, u_span = (0.79, 0.17) if is_iron else (0.08, 0.58)
            u0 = u_start + u_span * segment / segments
            u1 = u_start + u_span * (segment + 1) / segments
            v0 = (y0 - profile[0][0]) / (profile[-1][0] - profile[0][0])
            v1 = (y1 - profile[0][0]) / (profile[-1][0] - profile[0][0])
            uv_faces.append([(u0, v0), (u0, v1), (u1, v1), (u1, v0)])

    # Close only the distal tip. The throat is intentionally open at Y=.300.
    tip_center = len(verts)
    verts.append((0.0, 0.0, profile[-1][0]))
    last = len(profile) - 1
    for segment in range(segments):
        nxt = (segment + 1) % segments
        faces.append((last * segments + nxt, last * segments + segment, tip_center))
        u0 = 0.79 + 0.17 * segment / segments
        u1 = 0.79 + 0.17 * (segment + 1) / segments
        uv_faces.append([(u1, 1.0), (u0, 1.0), ((u0 + u1) * 0.5, 1.0)])

    mesh = bpy.data.meshes.new(SHEATH + "Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(SHEATH, mesh)
    bpy.context.collection.objects.link(obj)

    uv = mesh.uv_layers.new(name="UVMap")
    for polygon, coords in zip(mesh.polygons, uv_faces):
        polygon.material_index = 0
        polygon.use_smooth = True
        for loop_index, (u, v) in zip(polygon.loop_indices, coords):
            uv.data[loop_index].uv = (u, v)

    leather = bpy.data.materials.new("Dark oxblood leather and black iron fittings")
    leather.use_nodes = True
    nodes = leather.node_tree.nodes
    principled = nodes.get("Principled BSDF")
    principled.inputs["Metallic"].default_value = 0.0
    principled.inputs["Roughness"].default_value = 0.72

    base = bpy.data.images.load(str(TEXTURES / "sword-scabbard-base-color.png"), check_existing=False)
    base.name = "ScabbardBaseColor"
    base.pack()
    base_node = nodes.new("ShaderNodeTexImage")
    base_node.image = base
    base_node.interpolation = "Linear"
    base_node.location = (-600, 220)
    leather.node_tree.links.new(base_node.outputs["Color"], principled.inputs["Base Color"])

    normal = bpy.data.images.load(str(TEXTURES / "sword-scabbard-normal.png"), check_existing=False)
    normal.name = "ScabbardOpenGLNormal"
    normal.colorspace_settings.name = "Non-Color"
    normal.pack()
    normal_node = nodes.new("ShaderNodeTexImage")
    normal_node.image = normal
    normal_node.interpolation = "Linear"
    normal_node.location = (-600, -20)
    normal_map = nodes.new("ShaderNodeNormalMap")
    normal_map.space = "TANGENT"
    normal_map.inputs["Strength"].default_value = 0.45
    normal_map.location = (-320, -20)
    leather.node_tree.links.new(normal_node.outputs["Color"], normal_map.inputs["Color"])
    leather.node_tree.links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])

    orm = bpy.data.images.load(str(TEXTURES / "sword-scabbard-orm.png"), check_existing=False)
    orm.name = "ScabbardORM"
    orm.colorspace_settings.name = "Non-Color"
    orm.pack()
    orm_node = nodes.new("ShaderNodeTexImage")
    orm_node.image = orm
    orm_node.interpolation = "Linear"
    orm_node.location = (-600, -280)
    separate = nodes.new("ShaderNodeSeparateColor")
    separate.mode = "RGB"
    separate.location = (-320, -280)
    leather.node_tree.links.new(orm_node.outputs["Color"], separate.inputs["Color"])
    leather.node_tree.links.new(separate.outputs["Green"], principled.inputs["Roughness"])
    leather.node_tree.links.new(separate.outputs["Blue"], principled.inputs["Metallic"])
    obj.data.materials.append(leather)
    return obj


def export_source_glb(obj: bpy.types.Object) -> None:
    SOURCE.mkdir(parents=True, exist_ok=True)
    # Save authoring source with full 1K image data packed.
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE / "player-sword-scabbard.blend"))

    # Runtime GLBs carry only tiny preview images because the renderer resolves
    # authored texture slots through its measured shared PBR arrays.
    for image in bpy.data.images:
        if image.name not in {"ScabbardBaseColor", "ScabbardOpenGLNormal", "ScabbardORM"}:
            continue
        image.scale(4, 4)
        image.pack()
        image.filepath = ""
    bpy.ops.export_scene.gltf(
        filepath=str(SOURCE / "player-sword-scabbard-lod0.glb"),
        export_format="GLB",
        export_tangents=True,
        export_apply=True,
        export_materials="EXPORT",
        export_skins=False,
        export_animations=False,
        export_cameras=False,
        export_lights=False,
        export_extras=True,
    )


def main() -> None:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    obj = make_mesh()
    export_source_glb(obj)
    print("AUTHORED_SHEATH", len(obj.data.vertices), len(obj.data.polygons))
    print("BOUNDS", [list(obj.bound_box[i]) for i in range(8)])


if __name__ == "__main__":
    main()
