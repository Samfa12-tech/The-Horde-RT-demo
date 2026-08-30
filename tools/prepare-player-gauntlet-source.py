import bpy
import bmesh
import json
import math
import os
import sys
from array import array
import numpy as np
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python prepare-player-gauntlet-source.py -- input.glb output.glb")
source, destination = sys.argv[sys.argv.index("--") + 1:]
source = os.path.abspath(source)
destination = os.path.abspath(destination)
os.makedirs(os.path.dirname(destination), exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source, import_shading="NORMALS")
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if len(meshes) != 1:
    raise RuntimeError("gauntlet source must contain exactly one mesh")
gauntlet = meshes[0]
if len(gauntlet.material_slots) != 1 or not gauntlet.data.uv_layers:
    raise RuntimeError("gauntlet source requires one textured material and UV0")
material = gauntlet.material_slots[0].material
if material is None or not material.use_nodes:
    raise RuntimeError("gauntlet source material has no PBR nodes")
principled = next((node for node in material.node_tree.nodes
                   if node.type == "BSDF_PRINCIPLED"), None)
if principled is None or not principled.inputs["Base Color"].is_linked:
    raise RuntimeError("gauntlet source has no linked base-colour image")
base_node = principled.inputs["Base Color"].links[0].from_node
if base_node.type != "TEX_IMAGE" or base_node.image is None:
    raise RuntimeError("gauntlet base colour is not an image texture")
base_image = base_node.image
width, height = base_image.size
uv_layer = gauntlet.data.uv_layers.active.data
base_pixels = array("f", [0.0]) * (width * height * 4)
base_image.pixels.foreach_get(base_pixels)


def sample(uv):
    x = min(width - 1, max(0, int((uv.x % 1.0) * width)))
    y = min(height - 1, max(0, int((uv.y % 1.0) * height)))
    offset = (y * width + x) * 4
    return tuple(base_pixels[offset + channel] for channel in range(3))


cyan_faces = []
cyan_triangles = 0
for polygon in gauntlet.data.polygons:
    loop_uvs = [uv_layer[loop].uv for loop in polygon.loop_indices]
    centre_uv = (
        sum((uv.copy() for uv in loop_uvs), loop_uvs[0] * 0.0) /
        len(loop_uvs))
    colours = [sample(uv) for uv in loop_uvs] + [sample(centre_uv)]
    if any(green >= 0.30 and blue >= 0.34 and
           min(green, blue) - red >= 0.18
           for red, green, blue in colours):
        cyan_faces.append(polygon.index)
        cyan_triangles += max(0, len(polygon.vertices) - 2)
if cyan_triangles < 400:
    raise RuntimeError(
        f"cyan training hilt selection is unexpectedly small: {cyan_triangles}")

# The disposable cylinder is also the most accurate socket authoring guide.
# Derive its centre line before deletion and retain that compact semantic data
# as glTF extras on the stripped glove.
cyan_vertex_indices = sorted({
    vertex_index for polygon_index in cyan_faces
    for vertex_index in gauntlet.data.polygons[polygon_index].vertices
})
cyan_points = np.array([
    tuple(gauntlet.data.vertices[index].co)
    for index in cyan_vertex_indices
], dtype=np.float64)
grip_origin = cyan_points.mean(axis=0)
covariance = np.cov(cyan_points - grip_origin, rowvar=False)
eigenvalues, eigenvectors = np.linalg.eigh(covariance)
grip_axis = eigenvectors[:, int(np.argmax(eigenvalues))]
if grip_axis[2] < 0.0:
    grip_axis *= -1.0
grip_axis /= np.linalg.norm(grip_axis)
if float(np.max(eigenvalues)) < 0.05:
    raise RuntimeError("training hilt PCA did not find a stable long axis")
gauntlet["hordeGauntletSchema"] = 1
gauntlet["hordeGripOrigin"] = [float(value) for value in grip_origin]
gauntlet["hordeGripAxis"] = [float(value) for value in grip_axis]
gauntlet["hordeCuffAxis"] = [1.0, 0.0, 0.0]
gauntlet["hordePalmInteriorAxis"] = [0.0, 1.0, 0.0]

edit_mesh = bmesh.new()
edit_mesh.from_mesh(gauntlet.data)
edit_mesh.faces.ensure_lookup_table()
bmesh.ops.delete(
    edit_mesh, geom=[edit_mesh.faces[index] for index in cyan_faces],
    context="FACES")
loose_vertices = [vertex for vertex in edit_mesh.verts
                  if not vertex.link_faces]
bmesh.ops.delete(edit_mesh, geom=loose_vertices, context="VERTS")
bmesh.ops.remove_doubles(
    edit_mesh, verts=list(edit_mesh.verts), dist=1.0e-5)

# Meshy may leave a couple of detached single-digit vertex islands after the
# disposable training hilt is removed.  They are floating generation debris,
# not fingers or cuff panels (the accepted glove is a >2,000 vertex island).
# Remove only these tightly bounded islands before the source enters the
# character bake; otherwise they float above the first-person hand.
remaining = set(edit_mesh.verts)
debris = []
while remaining:
    seed = remaining.pop()
    island = [seed]
    stack = [seed]
    while stack:
        current = stack.pop()
        for edge in current.link_edges:
            neighbour = edge.other_vert(current)
            if neighbour in remaining:
                remaining.remove(neighbour)
                island.append(neighbour)
                stack.append(neighbour)
    if len(island) <= 8:
        debris.extend(island)
removed_debris_vertices = len(debris)
if debris:
    bmesh.ops.delete(edit_mesh, geom=debris, context="VERTS")
bmesh.ops.recalc_face_normals(edit_mesh, faces=list(edit_mesh.faces))
edit_mesh.to_mesh(gauntlet.data)
edit_mesh.free()
gauntlet.data.update()

gauntlet.data.validate(clean_customdata=False)


def topology(mesh):
    graph = [set() for _ in mesh.vertices]
    uses = {}
    for edge in mesh.edges:
        left, right = edge.vertices
        graph[left].add(right)
        graph[right].add(left)
    for polygon in mesh.polygons:
        vertices = tuple(polygon.vertices)
        for index in range(len(vertices)):
            edge = tuple(sorted((vertices[index],
                                 vertices[(index + 1) % len(vertices)])))
            uses[edge] = uses.get(edge, 0) + 1
    remaining = {index for index, neighbours in enumerate(graph) if neighbours}
    sizes = []
    while remaining:
        size = 1
        stack = [remaining.pop()]
        while stack:
            for neighbour in graph[stack.pop()]:
                if neighbour in remaining:
                    remaining.remove(neighbour)
                    stack.append(neighbour)
                    size += 1
        sizes.append(size)
    sizes.sort(reverse=True)
    return sizes, sum(1 for count in uses.values() if count != 2)


component_sizes, boundary_edges = topology(gauntlet.data)
remaining_triangles = sum(max(0, len(polygon.vertices) - 2)
                          for polygon in gauntlet.data.polygons)
if remaining_triangles < 2500:
    raise RuntimeError(
        f"training-hilt removal damaged the glove silhouette: {remaining_triangles}")

bpy.ops.export_scene.gltf(
    filepath=destination,
    export_format="GLB",
    export_tangents=True,
    export_animations=False,
    export_cameras=False,
    export_lights=False,
    export_extras=True,
)
with open(destination + ".processing.json", "w", encoding="utf-8") as handle:
    json.dump({
        "source": os.path.basename(source),
        "runtimeSource": os.path.basename(destination),
        "baseColourImage": os.path.basename(base_image.filepath),
        "removedCyanFaces": len(cyan_faces),
        "removedCyanTriangles": cyan_triangles,
        "removedDebrisVertices": removed_debris_vertices,
        "gripOrigin": [float(value) for value in grip_origin],
        "gripAxis": [float(value) for value in grip_axis],
        "gripAxisPrincipalVariance": float(np.max(eigenvalues)),
        "remainingTriangles": remaining_triangles,
        "remainingVertices": len(gauntlet.data.vertices),
        "componentVertexCounts": component_sizes,
        "boundaryEdges": boundary_edges,
        "processing": "UV-sampled bright-cyan training-hilt removal; coincident source vertices welded; <=8-vertex detached debris islands removed; rigid glove PBR and authored grip silhouette retained",
    }, handle, indent=2)
