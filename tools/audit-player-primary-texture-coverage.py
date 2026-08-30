import bpy
import json
import os
import sys


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python audit-player-primary-texture-coverage.py -- player.glb body-base-color.png gauntlet-base-color.png output.json"
    )
source, body_texture_path, gauntlet_texture_path, destination = \
    sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=os.path.abspath(source), import_shading="NORMALS")
player = next(
    obj for obj in bpy.context.scene.objects
    if obj.type == "MESH" and any(modifier.type == "ARMATURE" for modifier in obj.modifiers)
)
images = {}
for material_name, texture_path in {
        "BodyPrimaryVisible": body_texture_path,
        "GauntletPrimaryVisible": gauntlet_texture_path,
}.items():
    image = bpy.data.images.load(os.path.abspath(texture_path), check_existing=False)
    images[material_name] = {
        "width": image.size[0],
        "height": image.size[1],
        "pixels": list(image.pixels),
        "texture": os.path.basename(texture_path),
    }
uv_layer = player.data.uv_layers.active
if uv_layer is None:
    raise RuntimeError("runtime player has no active UV layer")


def sample(uv, image):
    # Match glTF repeat addressing and Blender's bottom-left UV origin.
    u = uv[0] % 1.0
    v = uv[1] % 1.0
    width = image["width"]
    height = image["height"]
    x = min(width - 1, max(0, int(u * (width - 1) + 0.5)))
    y = min(height - 1, max(0, int(v * (height - 1) + 0.5)))
    offset = (y * width + x) * 4
    return image["pixels"][offset:offset + 4]


group_indices = {group.name: group.index for group in player.vertex_groups}


def weights(vertex_index, side):
    assignments = {
        assignment.group: assignment.weight
        for assignment in player.data.vertices[vertex_index].groups
    }
    return {
        "upper": assignments.get(group_indices.get(side + "Arm", -1), 0.0),
        "lower": assignments.get(group_indices.get(side + "ForeArm", -1), 0.0),
        "hand": assignments.get(group_indices.get(side + "Hand", -1), 0.0),
    }


def category(polygon):
    totals = {}
    for side in ("Left", "Right"):
        side_weights = [weights(index, side) for index in polygon.vertices]
        totals[side] = {
            key: sum(value[key] for value in side_weights) / len(side_weights)
            for key in ("upper", "lower", "hand")
        }
    side = max(totals, key=lambda value: sum(totals[value].values()))
    segment = max(totals[side], key=totals[side].get)
    return side + segment.capitalize()


def empty_metrics():
    return {
        "polygons": 0,
        "samples": 0,
        "blackSamples": 0,
        "nearBlackSamples": 0,
        "cyanSamples": 0,
        "degenerateUvPolygons": 0,
        "minimumRgb": [1.0, 1.0, 1.0],
        "minimumCornerNormalDotGeometric": 1.0,
        "reversedCornerNormals": 0,
    }


report = {
    "schema": 1,
    "source": os.path.basename(source),
    "textures": {
        name: {"file": value["texture"],
               "size": [value["width"], value["height"]]}
        for name, value in images.items()
    },
    "categories": {},
    "worstPolygons": [],
}
for polygon in player.data.polygons:
    if polygon.material_index >= len(player.material_slots):
        continue
    material = player.material_slots[polygon.material_index].material
    material_name = material.name if material is not None else ""
    if material_name not in images:
        continue
    name = category(polygon)
    metrics = report["categories"].setdefault(name, empty_metrics())
    loop_uvs = [uv_layer.data[index].uv.copy() for index in polygon.loop_indices]
    centroid = sum(loop_uvs, loop_uvs[0] * 0.0) / len(loop_uvs)
    colours = [sample(uv, images[material_name])
               for uv in loop_uvs + [centroid]]
    corner_dots = [
        polygon.normal.dot(player.data.corner_normals[index].vector)
        for index in polygon.loop_indices
    ]
    black = 0
    near_black = 0
    cyan = 0
    for colour in colours:
        rgb = colour[:3]
        black += int(max(rgb) <= 0.003)
        near_black += int(max(rgb) <= 0.025)
        cyan += int(rgb[1] >= 0.30 and rgb[2] >= 0.34 and
                    min(rgb[1], rgb[2]) - rgb[0] >= 0.18)
        metrics["minimumRgb"] = [
            min(metrics["minimumRgb"][axis], rgb[axis]) for axis in range(3)
        ]
    metrics["polygons"] += 1
    metrics["samples"] += len(colours)
    metrics["blackSamples"] += black
    metrics["nearBlackSamples"] += near_black
    metrics["cyanSamples"] += cyan
    if len(loop_uvs) == 3:
        uv_edge_a = loop_uvs[1] - loop_uvs[0]
        uv_edge_b = loop_uvs[2] - loop_uvs[0]
        uv_twice_area = abs(uv_edge_a.x * uv_edge_b.y -
                            uv_edge_a.y * uv_edge_b.x)
        metrics["degenerateUvPolygons"] += int(uv_twice_area <= 1.0e-10)
    metrics["minimumCornerNormalDotGeometric"] = min(
        metrics["minimumCornerNormalDotGeometric"], min(corner_dots)
    )
    metrics["reversedCornerNormals"] += sum(dot < 0.0 for dot in corner_dots)
    if black:
        report["worstPolygons"].append({
            "polygon": polygon.index,
            "category": name,
            "blackSamples": black,
            "sampleCount": len(colours),
            "vertices": list(polygon.vertices),
            "uv": [[float(uv[0]), float(uv[1])] for uv in loop_uvs],
            "positions": [list(player.data.vertices[index].co) for index in polygon.vertices],
        })

report["worstPolygons"].sort(
    key=lambda value: (value["blackSamples"] / value["sampleCount"], value["blackSamples"]),
    reverse=True,
)
report["worstPolygons"] = report["worstPolygons"][:64]
for metrics in report["categories"].values():
    samples = max(1, metrics["samples"])
    metrics["blackFraction"] = metrics["blackSamples"] / samples
    metrics["nearBlackFraction"] = metrics["nearBlackSamples"] / samples
    metrics["cyanFraction"] = metrics["cyanSamples"] / samples

os.makedirs(os.path.dirname(os.path.abspath(destination)), exist_ok=True)
with open(destination, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
print(json.dumps(report["categories"], indent=2))
failures = []
for name, metrics in report["categories"].items():
    if metrics["blackSamples"] != 0:
        failures.append(f"{name} samples pure-black texture texels")
    if metrics["cyanSamples"] != 0:
        failures.append(f"{name} samples removed training-hilt cyan texels")
    if metrics["degenerateUvPolygons"] != 0:
        failures.append(f"{name} contains zero-area UV triangles")
    if metrics["reversedCornerNormals"] != 0:
        failures.append(f"{name} contains reversed bind-pose shading normals")
if failures:
    raise RuntimeError("; ".join(failures))
