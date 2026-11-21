"""Fit the loose road stone to the existing Kit take and native terrain.

blender --background --disable-autoexec --python-exit-code 1 \
    --python tools/author-bellwether-loose-stone.py -- OUTPUT
"""

import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Quaternion, Vector
from mathutils.bvhtree import BVHTree


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path(sys.argv[sys.argv.index("--") + 1]).resolve()
TAKE = ROOT / "assets/models/characters/source/kit-ankle-approach-v1/kit-ankle-approach.blend"
TERRAIN = ROOT / "assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb"

# Mirrored from the Windows route's presented snapshot and from the sound
# marker in KitAnkleAudio.h. Do not use a nominal planning-map altitude.
KIT_POSITION = (59.300003, -9.848501, 108.199997)
KIT_YAW = -3.744680
FPS, CONTACT, SETTLED, LAST = 60, 9, 32, 97
UP = Vector((0, 0, 1))


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def engine(vector):
    return [round(vector.x, 7), round(vector.z, 7), round(-vector.y, 7)]


if sha256(TERRAIN) != "c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774":
    raise RuntimeError("Resurvey the road before using changed terrain.")
if sha256(TAKE) != "9e19eb14e08c3d1527e0eba15d88971513b56b6b7706749dacb89bce74f27330":
    raise RuntimeError("Resurvey Kit's boot before using a changed take.")

bpy.ops.wm.open_mainfile(filepath=str(TAKE))
bpy.context.preferences.filepaths.save_version = 0
scene = bpy.context.scene
scene.name = "Bellwether lower road - loose stone contact"
arm = next(obj for obj in scene.objects if obj.type == "ARMATURE")
skin = scene.objects["briarhold-warden"]
action = bpy.data.actions["Kit|ankle_assess"]
arm.animation_data.action = action
arm.animation_data.action_slot = action.slots[0]
assert scene.render.fps == FPS and tuple(action.frame_range) == (1, LAST)
bpy.data.objects.remove(scene.objects["STUDY ONLY - local ground plane"], do_unlink=True)
actor_objects = [obj for obj in scene.objects if obj.type != "CAMERA" and obj.parent is None]
actor = bpy.data.objects.new("Kit - witnessed native road placement", None)
scene.collection.objects.link(actor)
for obj in actor_objects:
    obj.parent = actor
actor.location = Vector((KIT_POSITION[0], -KIT_POSITION[2], KIT_POSITION[1]))
actor.rotation_euler.z = math.pi - KIT_YAW

initial = set(scene.objects)
initial_surfaces = set(bpy.data.materials) | set(bpy.data.images) | set(bpy.data.meshes)
bpy.ops.import_scene.gltf(filepath=str(TERRAIN))
terrain_objects = set(scene.objects) - initial
points, faces = [], []
for obj in sorted(terrain_objects, key=lambda item: item.name):
    if obj.type != "MESH":
        continue
    world = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    remap = {}
    for face in obj.data.polygons:
        corners = [world[i] for i in face.vertices]
        if any(min(p[axis] for p in corners) > actor.location[axis] + 3 or
               max(p[axis] for p in corners) < actor.location[axis] - 3 for axis in (0, 1)):
            continue
        indices = []
        for i in face.vertices:
            if i not in remap:
                remap[i] = len(points)
                points.append(world[i])
            indices.append(remap[i])
        faces.append(indices)
for obj in terrain_objects:
    bpy.data.objects.remove(obj, do_unlink=True)
bpy.data.batch_remove(ids=(set(bpy.data.materials) | set(bpy.data.images) | set(bpy.data.meshes)) - initial_surfaces)
terrain = BVHTree.FromPolygons(points, faces)


def mesh(name, vertices, polygons, color):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], polygons)
    data.update()
    obj = bpy.data.objects.new(name, data)
    obj.color = (*color, 1)
    scene.collection.objects.link(obj)
    return obj


mesh("REVIEW ONLY - exact chapter road triangles", points, faces, (.24, .21, .16))


def ground(x, y):
    hit, normal, _, _ = terrain.ray_cast(Vector((x, y, 50)), -UP, 120)
    if hit is None or normal.z < .6:
        raise RuntimeError(f"No suitable road support at {(x, y)}")
    return hit, normal


groups = {side: {skin.vertex_groups[side + joint].index for joint in ("Foot", "ToeBase")}
          for side in ("Left", "Right")}
boot_ids = {side: [vertex.index for vertex in skin.data.vertices
                  if sum(group.weight for group in vertex.groups if group.group in ids) > .95]
            for side, ids in groups.items()}


def sole_points():
    evaluated = skin.evaluated_get(bpy.context.evaluated_depsgraph_get())
    boots = {side: [evaluated.matrix_world @ evaluated.data.vertices[i].co for i in ids]
             for side, ids in boot_ids.items()}
    return {side: [p for p in vertices if p.z <= min(v.z for v in vertices) + .017]
            for side, vertices in boots.items()}


def bone_world(bone):
    return arm.matrix_world @ arm.pose.bones[bone].matrix


def set_bone_world(bone, value):
    arm.pose.bones[bone].matrix = arm.matrix_world.inverted() @ value
    bpy.context.view_layer.update()


def leg_names(side):
    return tuple(side + joint for joint in ("UpLeg", "Leg", "Foot"))


def fit_leg(names, foot):
    upper, lower, end = [bone_world(name) for name in names]
    base, knee, ankle = [matrix.translation.copy() for matrix in (upper, lower, end)]
    upper_delta, lower_delta = knee - base, ankle - knee
    delta = foot.translation - base
    axis = delta.normalized()
    bend = (upper_delta - axis * axis.dot(upper_delta)).normalized()
    upper_length, lower_length = upper_delta.length, lower_delta.length
    reached = min(delta.length, upper_length + lower_length - .00005)
    along = (upper_length ** 2 - lower_length ** 2 + reached ** 2) / (2 * reached)
    position = base + axis * along + bend * math.sqrt(max(0, upper_length ** 2 - along ** 2))
    for name, original, location, change in (
        (names[0], upper, base, upper_delta.rotation_difference(position - base)),
        (names[1], lower, position, lower_delta.rotation_difference(base + axis * reached - position)),
    ):
        matrix = change.to_matrix().to_4x4() @ original
        matrix.translation = location
        set_bone_world(name, matrix)
    result = foot.copy()
    result.translation = base + axis * reached
    set_bone_world(names[2], result)


scene.frame_set(CONTACT)
bpy.context.view_layer.update()
boots = sole_points()
right = sum(boots["Right"], Vector()) / len(boots["Right"])
left = sum(boots["Left"], Vector()) / len(boots["Left"])
sideways = right - left
sideways.z = 0
forward = actor.matrix_world.to_quaternion() @ Vector((0, -1, 0))
# Remove the stagger from the side vector. The clip begins as the boot
# loads the inside edge, breaking the stone out toward the outer toe.
sideways = (sideways - forward * sideways.dot(forward)).normalized()
start = right + sideways * .043 + forward * .025

# An irregular chamfered stone, with a low footprint and a chipped crown.
# No texture layer, deck geometry, or baked lighting is required for the mesh.
outline = [(-.060, -.033), (-.026, -.061), (.052, -.050), (.070, -.009),
           (.044, .052), (-.025, .056), (-.063, .022)]
vertices = [Vector((x * .87, y * .85, .002)) for x, y in outline]
vertices += [Vector((x, y, height)) for (x, y), height in zip(outline,
            (.019, .024, .027, .023, .026, .023, .018))]
vertices += [Vector((x * .53 - .007, y * .56 + .002, height)) for (x, y), height in zip(outline,
            (.040, .046, .041, .034, .043, .048, .037))]
count = len(outline)
vertices += [Vector((-.008, .004, .047)), Vector((0, 0, .002))]
triangles = []
for i in range(count):
    j = (i + 1) % count
    triangles.extend([(count * 3 + 1, j, i), (count * 3, i + 2 * count, j + 2 * count)])
    for row in (0, count):
        triangles.extend([(row + i, row + j, row + i + count),
                          (row + j, row + j + count, row + i + count)])
stone = mesh("LooseRoadStone", vertices, triangles, (.40, .35, .27))
material = bpy.data.materials.new("Horde weathered brown limestone")
material.diffuse_color = (.26, .22, .17, 1)
bsdf = material.node_tree.nodes.get("Principled BSDF")
bsdf.inputs["Base Color"].default_value = material.diffuse_color
bsdf.inputs["Roughness"].default_value = .87
stone.data.materials.append(material)
uv = stone.data.uv_layers.new(name="Face projection")
for face in stone.data.polygons:
    axes = [i for i in range(3) if i != max(range(3), key=lambda i: abs(face.normal[i]))]
    for i in face.loop_indices:
        p = vertices[stone.data.loops[i].vertex_index]
        uv.data[i].uv = (p[axes[0]] / .15 + .5, p[axes[1]] / .15 + .5)

# Export the same compact physical mesh at its attachment origin. The data
# receipt carries world-space poses separately for engine use.
scene.frame_set(1)
bpy.ops.object.select_all(action="DESELECT")
stone.select_set(True)
bpy.context.view_layer.objects.active = stone
OUTPUT.mkdir(parents=True, exist_ok=False)
bpy.ops.export_scene.gltf(filepath=str(OUTPUT / "loose-road-stone.glb"),
    export_format="GLB", use_selection=True, export_animations=False, export_tangents=True)


def ease(t):
    t = min(1, max(0, t))
    return t * t * (3 - 2 * t)


stone.rotation_mode = "QUATERNION"
road_take = action.copy()
road_take.name = "Kit|ankle_assess_road_fit"
road_take.use_fake_user = True
arm.animation_data.action = road_take
arm.animation_data.action_slot = road_take.slots[0]
samples = []
for frame in range(1, LAST + 1):
    scene.frame_set(frame)
    progress = (frame - CONTACT) / (SETTLED - CONTACT)
    travel = ease(progress)
    toss = math.sin(math.pi * ease(progress))
    centre = start + (sideways * .257 + forward * .122) * travel
    support, normal = ground(centre.x, centre.y)
    pose = (Quaternion(normal, actor.rotation_euler.z - .18 + .63 * travel) @ UP.rotation_difference(normal) @
            Quaternion(Vector((0, -1, 0)), -.68 * toss - .14 * travel))
    # Terrain under the stone's full rotated footprint, including its rim.
    # A thin contact margin avoids unsupported frames at the slope break.
    relative = [pose @ p for p in vertices]
    support_heights = [ground(centre.x + v.x, centre.y + v.y)[0].z - v.z for v in relative]
    position = Vector((centre.x, centre.y, max(support_heights) - .001 + .017 * toss))
    stone.location = position
    stone.rotation_quaternion = pose
    stone.keyframe_insert("location", frame=frame)
    stone.keyframe_insert("rotation_quaternion", frame=frame)
    boots = sole_points()
    bottom_clearance = min(position.z - h for h in support_heights)
    stone_surface = BVHTree.FromPolygons([position + vertex for vertex in relative], triangles, all_triangles=True)

    def support_at(point):
        road = ground(point.x, point.y)[0].z
        hit, _, _, _ = stone_surface.ray_cast(Vector((point.x, point.y, 50)), -UP, 120)
        return max(road, hit.z) if hit is not None else road

    original_clearance = {side: min(p.z - ground(p.x, p.y)[0].z for p in sole)
                          for side, sole in boots.items()}
    expected_clearance = {side: max(.002, min(p.z for p in sole) - actor.location.z)
                          for side, sole in boots.items()}
    strike = ease((frame - CONTACT + 5) / 5) * (1 - ease((frame - CONTACT - 2) / 7))
    expected_clearance["Right"] += (.0015 - expected_clearance["Right"]) * strike
    targets, hip_drop = {}, 0.0
    for side, sole in boots.items():
        names = leg_names(side)
        base, knee, target = [bone_world(name) for name in names]
        chain_length = (knee.translation - base.translation).length + (target.translation - knee.translation).length
        target.translation.z += expected_clearance[side] - min(p.z - support_at(p) for p in sole)
        delta = base.translation - target.translation
        vertical_reach = math.sqrt(max(0, (chain_length - .00012) ** 2 - delta.x ** 2 - delta.y ** 2))
        hip_drop = max(hip_drop, delta.z - vertical_reach)
        targets[side] = target
    hips = bone_world("Hips")
    hips.translation.z -= hip_drop
    set_bone_world("Hips", hips)
    for side, target in targets.items():
        fit_leg(leg_names(side), target)
    for name in ("Hips", *leg_names("Left"), *leg_names("Right")):
        bone = arm.pose.bones[name]
        bone.keyframe_insert("location", frame=frame)
        bone.keyframe_insert("rotation_quaternion", frame=frame)
    fitted = sole_points()
    boot_clearance = {side: min(p.z - support_at(p) for p in sole) for side, sole in fitted.items()}
    contact_distance = min((p - (position + v)).length for p in fitted["Right"] for v in relative)
    axes = [engine(pose @ vec) for vec in (Vector((1, 0, 0)), UP, Vector((0, -1, 0)))]
    samples.append({"frame": frame, "worldPosition": engine(position), "engineBasisColumns": axes,
                    "supportWorldY": round(support.z, 7), "closestBootSampleMetres": round(contact_distance, 6),
                    "originalBootToRoadClearanceMetres": {side: round(value, 6) for side, value in original_clearance.items()},
                    "fittedBootToSupportClearanceMetres": {side: round(value, 6) for side, value in boot_clearance.items()},
                    "bootFitMaxErrorMetres": round(max(abs(boot_clearance[side] - target)
                        for side, target in expected_clearance.items()), 6),
                    "hipDropMetres": round(hip_drop, 6),
                    "minimumTerrainClearanceMetres": round(bottom_clearance, 6)})

if samples[CONTACT - 1]["closestBootSampleMetres"] > .050:
    raise RuntimeError("The stone starts too far from the boot at contact.")
if any(row["bootFitMaxErrorMetres"] > .004 or row["hipDropMetres"] > .075 for row in samples):
    raise RuntimeError("Road fit needs a larger correction than the rig can support.")
if any(row["minimumTerrainClearanceMetres"] < -.002 for row in samples):
    raise RuntimeError("The stone penetrates the authored road.")
if any(row["worldPosition"] != samples[SETTLED - 1]["worldPosition"] for row in samples[SETTLED - 1:]):
    raise RuntimeError("The stone drifted after settling.")

camera = scene.camera
camera.data.type = "ORTHO"
scene.render.resolution_x, scene.render.resolution_y = 840, 615
scene.display.shading.show_cavity = True
scene.display.shading.show_shadows = True
scene.render.image_settings.file_format = "PNG"


def frame_camera(target, offset, scale):
    camera.location = target + offset
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = scale


world_pose = Vector((KIT_POSITION[0], -KIT_POSITION[2], KIT_POSITION[1]))
target = Vector((start.x, start.y, ground(start.x, start.y)[0].z + .18))
frame_camera(target, (1.55 * sideways + .88 * forward + UP * .65), 1.12)
scene.frame_set(CONTACT + 5)
scene.render.filepath = "//road-stone-contact.png"
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "bellwether-loose-stone.blend"), compress=True)
for frame in (CONTACT - 3, CONTACT, CONTACT + 5, SETTLED):
    scene.frame_set(frame)
    scene.render.filepath = str(OUTPUT / f"road-stone-{frame:02d}.png")
    bpy.ops.render.render(write_still=True)
frame_camera(world_pose + UP * .78, Vector((1.7, -2.2, 1.64)), 2.6)
scene.frame_set(CONTACT + 5)
scene.render.filepath = str(OUTPUT / "road-stone-wide.png")
bpy.ops.render.render(write_still=True)

receipt = {"schema": 1, "rigSha256": sha256(TAKE), "terrainSha256": sha256(TERRAIN),
           "meshSha256": sha256(OUTPUT / "loose-road-stone.glb"),
           "kitEnginePosition": KIT_POSITION, "kitYawRadians": KIT_YAW,
           "fps": FPS, "contactFrame": CONTACT, "settledFrame": SETTLED, "triangles": len(triangles),
           "editableRoadTake": road_take.name, "poses": samples}
(OUTPUT / "road-contact.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
manifest = {"schema": 1, "asset": "bellwether-loose-road-stone", "metresPerUnit": 1.0,
            "coordinateSystem": {"up": "+Y", "forward": "+Z"},
            "budgets": {"maxVertices": 128, "maxIndices": 126, "maxPrimitives": 1,
                        "maxMaterials": 1, "maxTextureLayersPerKind": 1},
            "lods": [{"name": "source", "maxTriangles": 42}], "requiredSockets": [],
            "runtimeTextureProfile": {"android": "astc", "windows": "rgba8", "mipmapped": True},
            "materialOverrides": [],
            "distribution": "source-candidate", "licenceStatus": "Original Horde geometry and material"}
(OUTPUT / "asset.manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
