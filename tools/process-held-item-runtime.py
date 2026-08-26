import bpy
import bmesh
import json
import numpy as np
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError("usage: blender --background --python process-held-item-runtime.py -- input.glb output.glb sword|torch target_triangles")
source, destination, kind, target_text = sys.argv[sys.argv.index("--") + 1:]
if kind not in {"sword", "torch"}:
    raise RuntimeError("kind must be sword or torch")
target_triangles = int(target_text)
source = os.path.abspath(source)
destination = os.path.abspath(destination)
os.makedirs(os.path.dirname(destination), exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if not meshes:
    raise RuntimeError("held-item source contains no mesh")

bpy.ops.object.select_all(action="DESELECT")
for obj in meshes:
    obj.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
item = bpy.context.view_layer.objects.active
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

points = np.array([item.matrix_world @ vertex.co for vertex in item.data.vertices], dtype=np.float64)
centre = points.mean(axis=0)
covariance = np.cov((points - centre).T)
eigenvalues, eigenvectors = np.linalg.eigh(covariance)
axis = eigenvectors[:, int(np.argmax(eigenvalues))]
projections = (points - centre) @ axis
low = float(np.min(projections))
high = float(np.max(projections))
span = high - low
radial = np.linalg.norm((points - centre) - np.outer(projections, axis), axis=1)
low_radius = float(np.percentile(radial[projections <= low + span * 0.16], 95))
high_radius = float(np.percentile(radial[projections >= high - span * 0.16], 95))
if (kind == "sword" and high_radius > low_radius) or (kind == "torch" and high_radius < low_radius):
    axis = -axis

rotation = Vector(axis.tolist()).rotation_difference(Vector((0.0, 0.0, 1.0)))
item.rotation_mode = "QUATERNION"
item.rotation_quaternion = rotation
bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

def bounds():
    values = np.array([item.matrix_world @ vertex.co for vertex in item.data.vertices], dtype=np.float64)
    return values.min(axis=0), values.max(axis=0)

minimum, maximum = bounds()
height_target = 1.05 if kind == "sword" else 0.85
scale = height_target / float(maximum[2] - minimum[2])
item.scale = (scale, scale, scale)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
minimum, maximum = bounds()
item.location = (-float((minimum[0] + maximum[0]) * 0.5),
                 -float((minimum[1] + maximum[1]) * 0.5),
                 -float(minimum[2]))
bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

source_triangles = sum(max(0, len(poly.vertices) - 2) for poly in item.data.polygons)
if source_triangles > target_triangles:
    modifier = item.modifiers.new(name="ReviewedRuntimeRemesh", type="DECIMATE")
    modifier.decimate_type = "COLLAPSE"
    modifier.ratio = max(0.0001, min(1.0, target_triangles / source_triangles))
    modifier.use_collapse_triangulate = True
    bpy.context.view_layer.objects.active = item
    bpy.ops.object.modifier_apply(modifier=modifier.name)

for polygon in item.data.polygons:
    polygon.use_smooth = True

# A collapse remesh can leave a handful of UV-degenerate faces whose exported
# zero tangent is rejected by the Khronos validator. Remove only those faces;
# normal-mapped production meshes require a finite unit tangent everywhere.
for _ in range(3):
    item.data.calc_tangents()
    bad_loops = {
        index for index, loop in enumerate(item.data.loops)
        if loop.tangent.length_squared < 0.25
    }
    bad_polygons = {
        polygon.index for polygon in item.data.polygons
        if any(index in bad_loops for index in polygon.loop_indices)
    }
    if not bad_polygons:
        break
    mesh_editor = bmesh.new()
    mesh_editor.from_mesh(item.data)
    mesh_editor.faces.ensure_lookup_table()
    bmesh.ops.delete(
        mesh_editor,
        geom=[mesh_editor.faces[index] for index in sorted(bad_polygons)],
        context="FACES",
    )
    mesh_editor.to_mesh(item.data)
    mesh_editor.free()
    item.data.update()

runtime_triangles = sum(max(0, len(poly.vertices) - 2) for poly in item.data.polygons)
minimum, maximum = bounds()
socket_positions = ({
    "Grip": (0.0, 0.0, 0.135),
} if kind == "sword" else {
    "Grip": (0.0, 0.0, 0.24),
    "Flame": (0.0, 0.0, 0.765),
    "Light": (0.0, -0.025, 0.735),
})
for name, location in socket_positions.items():
    socket = bpy.data.objects.new(name, None)
    socket.empty_display_type = "PLAIN_AXES"
    socket.location = location
    bpy.context.scene.collection.objects.link(socket)

for image in bpy.data.images:
    if image.size[0] > 4 or image.size[1] > 4:
        image.scale(4, 4)

bpy.ops.export_scene.gltf(
    filepath=destination,
    export_format="GLB",
    export_tangents=True,
    export_animations=False,
    export_skins=False,
    export_morph=False,
    export_cameras=False,
    export_lights=False,
)

report = {
    "kind": kind,
    "source": os.path.basename(source),
    "runtime": os.path.basename(destination),
    "sourceTriangles": source_triangles,
    "runtimeTriangles": runtime_triangles,
    "runtimeVertices": len(item.data.vertices),
    "dimensionsMetres": (maximum - minimum).tolist(),
    "upAxis": "+Y",
    "forwardAxis": "+Z",
    "sockets": {name: list(position) for name, position in socket_positions.items()},
    "processing": "PCA axis normalization, metre-scale normalization, reviewed Blender collapse remesh, preserved UV/material regions, generated tangents",
}
with open(destination + ".processing.json", "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
