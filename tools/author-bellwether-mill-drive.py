"""Author the replacement drive for the existing B05 mill workbench cradle.

Run Blender headlessly with -- ROOT OUTPUT. The output directory must be new.
The mesh lives in metres about the centre of the underside of the drive.
"""
import json
import math
from pathlib import Path
import sys

import bpy


def main(root, output):
    output.mkdir()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    # Four samples per tooth leave a short flat crown and a relieved root.
    tooth_radii = (.141, .181, .181, .141)
    teeth = 12
    segments = teeth * len(tooth_radii)
    radii = [tooth_radii[i % len(tooth_radii)] for i in range(segments)]
    vertices = []
    for height, inner in ((0., False), (0., True), (.084, False), (.084, True)):
        for i, radius in enumerate(radii):
            angle = i * math.tau / segments
            r = .041 if inner else radius
            vertices.append((r * math.cos(angle), r * math.sin(angle), height))
    faces = []
    outer, inner = segments*2, segments*3
    for i in range(segments):
        j = (i+1) % segments
        faces.extend(((i, segments+i, segments+j, j),
                      (outer+i, outer+j, inner+j, inner+i),
                      (i, j, outer+j, outer+i),
                      (segments+i, inner+i, inner+j, segments+j)))
    geometry = bpy.data.meshes.new("Twelve-tooth replacement drive")
    geometry.from_pydata(vertices, [], faces)
    geometry.update()
    # Every quad has a non-degenerate UV chart, including the vertical tooth faces.
    uv = geometry.uv_layers.new(name="Mill drive UV").data
    grid = math.ceil(math.sqrt(len(faces)))
    for poly in geometry.polygons:
        column, row = poly.index % grid, poly.index // grid
        for loop, (u, v) in zip(poly.loop_indices, ((.1, .1), (.9, .1), (.9, .9), (.1, .9))):
            uv[loop].uv = ((column+u)/grid, (row+v)/grid)
    drive = bpy.data.objects.new("BellwetherMillBenchDrive", geometry)
    bpy.context.collection.objects.link(drive)
    bronze = bpy.data.materials.new("HordeMillDriveBronze")
    bronze.use_nodes = True
    surface = bronze.node_tree.nodes.get("Principled BSDF")
    surface.inputs["Base Color"].default_value = (.43, .23, .083, 1.)
    surface.inputs["Metallic"].default_value = .57
    surface.inputs["Roughness"].default_value = .38
    drive.data.materials.append(bronze)
    bpy.context.view_layer.objects.active = drive
    drive.select_set(True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / "mill-bench-drive.blend"), compress=True)
    bpy.ops.export_scene.gltf(filepath=str(output / "mill-bench-drive.glb"), export_format="GLB",
                              use_selection=True, export_normals=True, export_tangents=True, export_animations=False)
    original = root / "assets/models/world/runtime/bellwether-tavern-v1/asset.manifest.json"
    manifest = json.loads(original.read_text(encoding="utf-8"))
    triangles = len(faces)*2
    manifest.update(asset="bellwether-mill-bench-drive", materialOverrides=[],
                    licenceStatus="Original Horde workbench mechanism authored with Blender",
                    budgets=dict(maxVertices=1600, maxIndices=3*triangles,
                                 maxPrimitives=1, maxMaterials=1, maxTextureLayersPerKind=1),
                    lods=[dict(name="lod0", maxTriangles=triangles)])
    (output / "mill-bench-drive.manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--") + 1:]))
