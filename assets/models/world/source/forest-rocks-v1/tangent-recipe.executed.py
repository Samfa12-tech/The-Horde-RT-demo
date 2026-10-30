"""Add only GLTF TANGENT attributes to a forest runtime derivative via Blender."""
import hashlib
import json
from pathlib import Path
import struct
import sys

import bpy
def args_after_separator():
    if "--" not in sys.argv:
        raise SystemExit("Usage: blender -b --python prepare-forest-runtime-tangents.py -- --input IN.glb --output OUT.glb")
    args = sys.argv[sys.argv.index("--") + 1:]
    values = {}
    for index in range(0, len(args), 2):
        if index + 1 >= len(args) or not args[index].startswith("--"):
            raise SystemExit("Arguments must be --input PATH --output PATH")
        values[args[index][2:]] = args[index + 1]
    if set(values) != {"input", "output"}:
        raise SystemExit("Required arguments: --input and --output")
    return Path(values["input"]).resolve(), Path(values["output"]).resolve()


def reset_and_import(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"No mesh objects imported from {path}")
    return meshes


def image_roster():
    rows = []
    for image in bpy.data.images:
        if image.source != "FILE" or image.size[0] <= 0 or image.size[1] <= 0:
            continue
        pixels = tuple(image.pixels[:])
        payload = struct.pack(f"<{len(pixels)}f", *pixels)
        rows.append((int(image.size[0]), int(image.size[1]), hashlib.sha256(payload).hexdigest()))
    return sorted(rows)


def geometry_roster(meshes):
    rows = {}
    for obj in meshes:
        mesh = obj.data
        if not mesh.uv_layers:
            raise RuntimeError(f"Mesh {obj.name} has no UV0")
        uv_layer = mesh.uv_layers.active.data
        mesh.calc_loop_triangles()
        normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
        per_material = {}
        for triangle in mesh.loop_triangles:
            polygon = mesh.polygons[triangle.polygon_index]
            material = obj.material_slots[polygon.material_index].material
            material_name = material.name if material else "<missing>"
            corners = []
            for loop_index in triangle.loops:
                loop = mesh.loops[loop_index]
                point = obj.matrix_world @ mesh.vertices[loop.vertex_index].co
                normal = normal_matrix @ loop.normal
                uv = uv_layer[loop_index].uv
                position = tuple(round(float(value), 6) for value in (point.x, point.y, point.z))
                shading = tuple(round(float(value), 6) for value in (normal.x, normal.y, normal.z))
                texture = tuple(round(float(value), 6) for value in (uv.x, uv.y))
                corners.append((position, shading, texture))
            key = tuple(sorted(corner[0] for corner in corners))
            per_material.setdefault(material_name, {}).setdefault(key, []).append(tuple(corners))
        for material_name, triangles in per_material.items():
            destination = rows.setdefault(material_name, {})
            for key, copies in triangles.items():
                destination.setdefault(key, []).extend(copies)
    return rows


def compare_geometry(before, after):
    if set(before) != set(after):
        return False, "material-group roster changed", None
    max_normal_delta = 0.0
    minimum_normal_dot = 1.0
    for material_name in before:
        before_groups = before[material_name]
        after_groups = after[material_name]
        if set(before_groups) != set(after_groups):
            return False, f"triangle position sets changed for {material_name}", None
        for key in before_groups:
            expected_copies = before_groups[key]
            candidate_copies = after_groups[key]
            if len(expected_copies) != len(candidate_copies):
                return False, f"triangle multiplicity changed for {material_name}", None
            remaining = list(candidate_copies)
            for expected in expected_copies:
                match = None
                nearest_normal = float("inf")
                nearest_uv = False
                for index, candidate in enumerate(remaining):
                    # Match only the same winding, allowing a cyclic choice of the
                    # first corner. A reversed triangle is a material geometry change.
                    for rotation in range(3):
                        rotated = candidate[rotation:] + candidate[:rotation]
                        if [corner[0] for corner in expected] != [corner[0] for corner in rotated]:
                            continue
                        deltas = [max(abs(expected[i][1][axis] - rotated[i][1][axis])
                                      for axis in range(3)) for i in range(3)]
                        nearest_normal = min(nearest_normal, max(deltas))
                        uv_matches = not any(expected[i][2] != rotated[i][2] for i in range(3))
                        nearest_uv = nearest_uv or uv_matches
                        if not uv_matches:
                            continue
                        dots = []
                        for corner in range(3):
                            left = expected[corner][1]
                            right = rotated[corner][1]
                            left_length = sum(value * value for value in left) ** 0.5
                            right_length = sum(value * value for value in right) ** 0.5
                            dots.append(sum(left[axis] * right[axis] for axis in range(3)) /
                                        max(left_length * right_length, 1.0e-12))
                        if min(dots) < 0.99999:
                            continue
                        max_normal_delta = max(max_normal_delta, *deltas)
                        minimum_normal_dot = min(minimum_normal_dot, *dots)
                        match = index
                        break
                    if match is not None:
                        break
                if match is None:
                    print(f"  unmatched triangle in {material_name}: nearestNormalDelta={nearest_normal} exactUvMatch={nearest_uv}")
                    return False, f"triangle winding, exact positions/UVs, or normal direction changed for {material_name}", None
                remaining.pop(match)
    return True, "positions/winding/UV0 exact at 1e-6; normal direction dot >= 0.99999", (max_normal_delta, minimum_normal_dot)


def tangent_counts(meshes):
    counts = {}
    for obj in meshes:
        mesh = obj.data
        if not mesh.uv_layers:
            raise RuntimeError(f"Mesh {obj.name} has no UV0")
        mesh.calc_tangents(uvmap=mesh.uv_layers.active.name)
        for polygon in mesh.polygons:
            material = obj.material_slots[polygon.material_index].material
            if not material:
                continue
            for node in material.node_tree.nodes if material.use_nodes else ():
                if node.type == "NORMAL_MAP" and any(link.to_node == node for link in material.node_tree.links):
                    name = material.name
                    counts[name] = counts.get(name, 0) + len(polygon.loop_indices)
        if len(mesh.loops) and not all(len(loop.tangent) == 3 and all(map(lambda x: abs(float(x)) < 1.00001, loop.tangent))
                                      for loop in mesh.loops):
            raise RuntimeError(f"Mesh {obj.name} produced invalid tangent data")
    if not counts:
        raise RuntimeError("No material with an authored normal map was found")
    return counts


def main():
    source, output = args_after_separator()
    if not source.is_file() or source == output:
        raise RuntimeError("Input must exist and output must be a separate derivative path")
    before_meshes = reset_and_import(source)
    before_geometry = geometry_roster(before_meshes)
    before_images = image_roster()
    mapped_tangent_counts = tangent_counts(before_meshes)
    output.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=str(output), export_format="GLB", export_tangents=True,
        export_materials="EXPORT", export_image_format="AUTO",
        export_cameras=False, export_lights=False, export_animations=False,
    )
    if not output.is_file() or output.stat().st_size == 0:
        raise RuntimeError("Blender did not produce the tangent derivative")
    after_meshes = reset_and_import(output)
    after_geometry = geometry_roster(after_meshes)
    after_images = image_roster()
    geometry_equal, geometry_diagnostic, maximum_normal_delta = compare_geometry(before_geometry, after_geometry)
    if not geometry_equal:
        raise RuntimeError("Re-export " + geometry_diagnostic)
    if before_images != after_images:
        raise RuntimeError("Re-export changed the set or decoded pixels of embedded source images")
    result = {
        "status": "pass",
        "input": str(source),
        "output": str(output),
        "inputSha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "outputSha256": hashlib.sha256(output.read_bytes()).hexdigest(),
        "outputBytes": output.stat().st_size,
        "geometryUnchanged": True,
        "geometryComparison": geometry_diagnostic,
        "maximumNormalComponentDelta": maximum_normal_delta[0],
        "minimumNormalDirectionDot": maximum_normal_delta[1],
        "embeddedImagePixelsUnchanged": True,
        "tangentLoopCountsByMappedMaterial": mapped_tangent_counts,
        "materialTriangles": {name: sum(len(copies) for copies in groups.values())
                              for name, groups in before_geometry.items()},
    }
    output.with_suffix(output.suffix + ".tangent-receipt.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
