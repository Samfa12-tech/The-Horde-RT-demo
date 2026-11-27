"""Build the original fallen silver cup for the Bellwether roadside cache.

Run from the horde-1.8 repository root with Blender 5.2:
  blender --background --factory-startup --python-exit-code 1 --python \
    assets/models/world/source/bellwether-road-silver-v1/build-road-silver.py -- .
"""
import hashlib
import json
import math
from pathlib import Path
import runpy
import sys

import bpy
import bmesh
from mathutils import Vector
from mathutils.bvhtree import BVHTree


SOURCE = Path("assets/models/world/source/bellwether-road-silver-v1")
RUNTIME = Path("assets/models/world/runtime/bellwether-tavern-v1")
RECIPE = SOURCE / "build-road-silver.py"
BOX = (35.3, 82.95)
TRIANGLES_BUDGET = 500
APPROACH = (33.6, 82.92)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def make_cup():
    """Make one closed, hollow cup lying on its side, mouth facing west."""
    verts, faces, uv_faces = [], [], []
    sides = 16
    cy, cz = .061, 0.0
    outer_profile = ((-.12, .061), (-.09, .055), (-.035, .050),
                     (.02, .0434), (.04, .041))
    inner_profile = ((-.12, .051), (-.09, .046), (-.045, .039), (-.010, .030))

    def ring(x, radius):
        return [(x, cy + radius * math.cos(math.tau * i / sides),
                 cz + radius * math.sin(math.tau * i / sides)) for i in range(sides)]

    def band(a, b, reverse=False, u0=0.0, u1=1.0):
        for i in range(sides):
            j = (i + 1) % sides
            ids = (a[i], a[j], b[j], b[i]) if not reverse else (a[i], b[i], b[j], a[j])
            uvs = ((i / sides, u0), ((i + 1) / sides, u0),
                   ((i + 1) / sides, u1), (i / sides, u1))
            faces.append(ids)
            uv_faces.append(uvs)

    outer = []
    for x, radius in outer_profile:
        ring_ids = []
        for point in ring(x, radius):
            ring_ids.append(len(verts))
            verts.append(point)
        outer.append(ring_ids)
    for index, (a, b) in enumerate(zip(outer, outer[1:])):
        band(a, b, reverse=False, u0=index / (len(outer) - 1), u1=(index + 1) / (len(outer) - 1))

    inner = []
    for x, radius in inner_profile:
        ring_ids = []
        for point in ring(x, radius):
            ring_ids.append(len(verts))
            verts.append(point)
        inner.append(ring_ids)
    for index, (a, b) in enumerate(zip(inner, inner[1:])):
        band(a, b, reverse=True, u0=index / (len(inner) - 1), u1=(index + 1) / (len(inner) - 1))

    # Rolled lip joins the outside and inside mouth rings.
    band(outer[0], inner[0], reverse=True)
    # Closed inner bottom, triangulated to a centre point; this keeps the cup
    # physically hollow while preventing a bright see-through backface.
    centre = len(verts)
    verts.append((-.008, cy, cz))
    for i in range(sides):
        j = (i + 1) % sides
        faces.append((inner[-1][i], inner[-1][j], centre))
        uv_faces.append(((i / sides, .98), ((i + 1) / sides, .98), (.5, .5)))

    # A stout, flattened loop handle is attached on the visible side of the
    # fallen cup. Its curve clears the floor and remains inside the cache.
    path_steps, tube_sides = 16, 6
    handle_x, handle_y = -.20, .076
    handle_z = -.052
    path = []
    for step in range(path_steps):
        angle = math.tau * step / path_steps
        path.append((handle_x + .045 * math.cos(angle),
                     handle_y + .041 * math.sin(angle), handle_z))
    handle_rings = []
    for step, point in enumerate(path):
        previous, following = path[(step - 1) % path_steps], path[(step + 1) % path_steps]
        dx, dy = following[0] - previous[0], following[1] - previous[1]
        length = math.hypot(dx, dy)
        nx, ny = -dy / length, dx / length
        ids = []
        for side in range(tube_sides):
            around = math.tau * side / tube_sides
            ids.append(len(verts))
            verts.append((point[0] + .008 * nx * math.cos(around),
                          point[1] + .008 * ny * math.cos(around),
                          point[2] + .008 * math.sin(around)))
        handle_rings.append(ids)
    for step, a in enumerate(handle_rings):
        b = handle_rings[(step + 1) % path_steps]
        for side in range(tube_sides):
            nxt = (side + 1) % tube_sides
            faces.append((a[side], a[nxt], b[nxt], b[side]))
            uv_faces.append(((step / path_steps, side / tube_sides),
                             (step / path_steps, (side + 1) / tube_sides),
                             ((step + 1) / path_steps, (side + 1) / tube_sides),
                             ((step + 1) / path_steps, side / tube_sides)))

    # The cup rests on its mouth rim and its narrow foot rim. Pitch the axis
    # down by 0.125 m/m toward the foot to match the tapered profile's contact
    # height and keep the intervening wall just clear of the board.
    engine_verts = [(x - .17, y - .125 * (x + .12), z) for x, y, z in verts]
    features = {
        "mouthCrown": engine_verts[outer[0][0]],
        "mouthNearRim": engine_verts[outer[0][12]],
        "shoulderCrown": engine_verts[outer[2][0]],
        "handleCrown": engine_verts[handle_rings[4][3]],
    }
    support = {
        "mouthRimY": min(engine_verts[index][1] for index in outer[0]),
        "footRimY": min(engine_verts[index][1] for index in outer[-1]),
        "intermediateClearanceY": min(
            engine_verts[index][1] for ring in outer[1:-1] for index in ring),
        "mouthRimX": engine_verts[outer[0][0]][0],
        "footRimX": engine_verts[outer[-1][0]][0],
    }
    # Keep the model local to a cache-centred identity node. Convert engine
    # Y-up coordinates to Blender's Z-up; glTF export applies the inverse.
    verts = [(x, -z, y) for x, y, z in engine_verts]
    mesh = bpy.data.meshes.new("WornSilver cup and loop")
    mesh.from_pydata(verts, [], faces)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    uv = mesh.uv_layers.new(name="UVMap")
    for polygon, values in zip(mesh.polygons, uv_faces, strict=True):
        polygon.use_smooth = True
        for loop_index, value in zip(polygon.loop_indices, values, strict=True):
            uv.data[loop_index].uv = value
    material = bpy.data.materials.new("WornSilver")
    material.diffuse_color = (.52, .52, .49, 1.0)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (.52, .52, .49, 1.0)
    principled.inputs["Metallic"].default_value = .82
    principled.inputs["Roughness"].default_value = .62
    mesh.materials.append(material)
    obj = bpy.data.objects.new("BellwetherRoadSilver", mesh)
    bpy.context.collection.objects.link(obj)
    return obj, features, support


def prove_visibility(root, helper, ground, feature_points, position, local_object):
    """Ray-check cup features from the standing eye against town, terrain and cache."""
    town_path = root / RUNTIME / "bellwether-tavern-v1.glb"
    terrain_path = root / helper["TERRAIN"]
    bpy.ops.import_scene.gltf(filepath=str(town_path))
    bpy.ops.import_scene.gltf(filepath=str(terrain_path))
    cache_recipe = runpy.run_path(str(root / "tools/author-bellwether-road-cache.py"))
    crossing = runpy.run_path(str(root / "tools/author-bellwether-mill-crossing.py"))
    cache_parts, _ = cache_recipe["build_box"](ground)
    cache_object = crossing["join_parts"](cache_parts, "VisibilityProofRoadCache")

    points, faces, offset = [], [], 0
    for item in bpy.context.scene.objects:
        if item.type != "MESH" or item in (local_object, cache_object):
            continue
        evaluated = item.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        matrix = evaluated.matrix_world
        points.extend(matrix @ vertex.co for vertex in mesh.vertices)
        faces.extend(tuple(offset + index for index in polygon.vertices)
                     for polygon in mesh.polygons)
        offset += len(mesh.vertices)
        evaluated.to_mesh_clear()
    evaluated = cache_object.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    matrix = evaluated.matrix_world
    points.extend(matrix @ vertex.co for vertex in mesh.vertices)
    faces.extend(tuple(offset + index for index in polygon.vertices)
                 for polygon in mesh.polygons)
    evaluated.to_mesh_clear()
    tree = BVHTree.FromPolygons(points, faces)

    eye_world = (APPROACH[0], ground(*APPROACH) + 1.65, APPROACH[1])
    eye_blender = Vector((eye_world[0], -eye_world[2], eye_world[1]))
    results = []
    for name, local in feature_points.items():
        world = (position[0] + local[0], position[1] + local[1], position[2] + local[2])
        target = Vector((world[0], -world[2], world[1]))
        delta = target - eye_blender
        distance = delta.length
        direction = delta.normalized()
        hit, _, _, hit_distance = tree.ray_cast(eye_blender, direction, distance - .002)
        if hit is not None:
            raise ValueError(f"Cache/town blocks {name} visibility ray at {hit_distance:.4f} m")
        results.append({"feature": name, "targetWorld": world,
                        "rayLengthMetres": round(distance, 6), "unoccluded": True})
    return {"eyeWorld": eye_world, "eyeHeightAboveApproachGround": 1.65,
            "opaqueMeshTriangleCount": sum(len(face)-2 for face in faces),
            "rays": results,
            "townGlbSha256": helper["sha"](town_path),
            "cacheRecipeSha256": helper["sha"](root / "tools/author-bellwether-road-cache.py")}


def main(root):
    root = root.resolve()
    source, runtime = root / SOURCE, root / RUNTIME
    source.mkdir(parents=True, exist_ok=True)
    if not runtime.is_dir():
        raise ValueError(f"Missing admitted runtime asset directory: {runtime}")
    generated = {"blend": source / "BellwetherRoadSilver.blend",
                 "glb": runtime / "road-silver.glb",
                 "manifest": runtime / "road-silver.manifest.json",
                 "header": source / "geometry.generated.h",
                 "receipt": source / "authoring-receipt.json"}
    existing = [path.exists() for path in generated.values()]
    if any(existing):
        if not all(existing):
            raise ValueError("Refusing to overwrite a partial existing generated asset set.")
        prior = json.loads(generated["receipt"].read_text(encoding="utf-8"))
        expected = {"blendSha256": generated["blend"], "glbSha256": generated["glb"],
                    "manifestSha256": generated["manifest"], "geometryHeaderSha256": generated["header"]}
        if any(prior.get(field) != sha(path) for field, path in expected.items()):
            raise ValueError("Refusing to overwrite generated artifacts changed since their receipt.")

    helper = runpy.run_path(str(root / "tools/author-bellwether-mill-works.py"))
    ground = helper["terrain_probe"](root / helper["TERRAIN"])
    floor = ground(*BOX)
    # The model origin is the cache centre at the broken-lid top board.
    # The board's authored upper surface is terrain + 0.561 m.
    position = (BOX[0] + .39, floor + .561, BOX[1])

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0
    obj, feature_points, support = make_cup()
    # The cup sits on the high broken-lid board, with its lip near the west
    # edge and facing the player approach. It pitches toward the foot so both
    # rim edges contact the board.
    triangles = sum(len(polygon.vertices) - 2 for polygon in obj.data.polygons)
    vertices = len(obj.data.vertices)
    if triangles > TRIANGLES_BUDGET or len(bpy.data.objects) != 1 or len(obj.data.materials) != 1:
        raise ValueError(f"Unexpected prop budget or scene contents: {vertices} vertices, {triangles} triangles")
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source / "BellwetherRoadSilver.blend"), compress=True)

    glb = runtime / "road-silver.glb"
    bpy.ops.export_scene.gltf(filepath=str(glb), export_format="GLB", use_selection=True,
                              export_apply=True, export_normals=True, export_tangents=True,
                              export_animations=False)
    exported, _ = helper["read_glb"](glb)
    primitive = exported["meshes"][0]["primitives"][0]
    runtime_vertices = exported["accessors"][primitive["attributes"]["POSITION"]]["count"]
    runtime_indices = exported["accessors"][primitive["indices"]]["count"]
    positions = exported["accessors"][primitive["attributes"]["POSITION"]]
    material = exported["materials"][primitive["material"]]
    if (runtime_indices // 3 != triangles or len(exported["meshes"]) != 1
            or len(exported["meshes"][0]["primitives"]) != 1
            or exported["nodes"] != [{"mesh": 0, "name": "BellwetherRoadSilver"}]
            or not {"POSITION", "NORMAL", "TANGENT", "TEXCOORD_0"} <= set(primitive["attributes"])
            or len(exported["materials"]) != 1 or material["name"] != "WornSilver"
            or exported.get("textures") or material.get("emissiveFactor")
            or abs(positions["min"][1]) > 1e-6 or positions["max"][1] < .1):
        raise ValueError("Exported GLB failed its static-import, material, or floor-bound checks.")
    visibility = prove_visibility(root, helper, ground, feature_points, position, obj)
    manifest = {
        "schema": 1, "asset": "road-silver", "metresPerUnit": 1.0,
        "coordinateSystem": {"up": "+Y", "forward": "+Z"},
        "budgets": {"maxVertices": runtime_vertices, "maxIndices": runtime_indices,
                    "maxPrimitives": 1, "maxMaterials": 1, "maxTextureLayersPerKind": 1},
        "lods": [{"name": "lod0", "maxTriangles": triangles}], "requiredSockets": [],
        "runtimeTextureProfile": {"android": "astc", "windows": "rgba8", "mipmapped": True, "resolution": 1024},
        "materialOverrides": [], "distribution": "runtime",
        "licenceStatus": "Original Horde silverware geometry and opaque WornSilver finish authored with Blender"
    }
    manifest_path = runtime / "road-silver.manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")

    header = source / "geometry.generated.h"
    header.write_text(
        "#pragma once\n#include <array>\n#include <cstddef>\n\n"
        "// Generated by build-road-silver.py; translation is the broken-lid top board.\n"
        "namespace horde::gameplay::bellwether {\n"
        f"inline constexpr std::array<float, 3> kRoadSilverPosition{{{{{position[0]:.6f}f, {position[1]:.6f}f, {position[2]:.6f}f}}}};\n"
        f"inline constexpr std::size_t kRoadSilverTriangles={triangles};\n"
        f"inline constexpr std::size_t kRoadSilverVertices={runtime_vertices};\n"
        "} // namespace horde::gameplay::bellwether\n", encoding="utf-8", newline="\n")
    receipt = {
        "source": "Original geometry authored locally in Blender; no external model or texture input.",
        "terrainSha256": helper["sha"](root / helper["TERRAIN"]),
        "recipeSha256": sha(root / RECIPE), "blender": bpy.app.version_string,
        "cacheCentre": [BOX[0], floor + .64, BOX[1]], "cacheSupportBoardTop": position,
        "playerApproach": list(APPROACH), "placement": "On the broken-lid board near its west end; mouth faces west.",
        "visibilityProof": visibility,
        "sourceVertices": vertices, "runtimeVertices": runtime_vertices,
        "runtimeIndices": runtime_indices, "triangles": triangles,
        "supportContacts": support,
        "glbSha256": sha(glb), "manifestSha256": sha(manifest_path),
        "blendSha256": sha(source / "BellwetherRoadSilver.blend"),
        "geometryHeaderSha256": sha(header)
    }
    (source / "authoring-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n",
                                                    encoding="utf-8", newline="\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) != 1:
        raise ValueError("Expected repository root path.")
    main(Path(args[0]))
