"""Prepare an editable Bellwether resident rig and native clips from local Meshy exports.

blender --background --python-exit-code 1 --python tools/author-bellwether-keeper.py -- SOURCE_DIR NEW_OUTPUT_DIR [PROFILE_JSON]
Without a profile this reproduces the tavern keeper. Other residents supply an asset ID and two source hashes.
"""

import hashlib
import json
from pathlib import Path
import struct
import sys

import bpy


FPS = 30
# Match TavernKeeperPlacement, whose clock advances only with admitted travel.
WALK_SPEED = 1.08
ARMS = [side + part for side in ("Left", "Right")
        for part in ("Shoulder", "Arm", "ForeArm", "Hand")]
KEEPER_SOURCE_HASHES = {
    "rigged.glb": "408b338c343e6d2ea22ffc501a47a86f96607e764a5aabc1af5d659e540b0c4e",
    "walking.glb": "c8f22a6761966fc5b59adf8916faae714b3ab54ecd3fa6dada800793f1463eba",
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = FPS
    bpy.ops.import_scene.gltf(filepath=str(path), import_shading="NORMALS")
    armature, = [obj for obj in scene.objects if obj.type == "ARMATURE"]
    mesh, = [obj for obj in scene.objects if obj.type == "MESH" and obj.parent == armature]
    return scene, armature, mesh


def bounds(mesh):
    evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    points = [evaluated.matrix_world @ vertex.co for vertex in evaluated.data.vertices]
    return [[limit(point[axis] for point in points) for axis in range(3)]
            for limit in (min, max)]


def sample(scene, frame):
    whole = int(frame)
    scene.frame_set(whole, subframe=frame - whole)


def bake(armature, name, poses):
    armature.animation_data.action = None
    for frame, pose in enumerate(poses, 1):
        for bone in armature.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.matrix_basis = pose[bone.name]
            for channel in ("location", "rotation_quaternion", "scale"):
                bone.keyframe_insert(data_path=channel, frame=frame, group=bone.name)
    action = armature.animation_data.action
    action.name = name
    action.use_fake_user = True
    channelbag = action.layers[0].strips[0].channelbag(armature.animation_data.action_slot)
    for curve in channelbag.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = "LINEAR"
    return action


def write_manifest(model, asset):
    with model.open("rb") as stream:
        magic, version, length, size, kind = struct.unpack("<4sIII4s", stream.read(20))
        if magic != b"glTF" or version != 2 or length != model.stat().st_size or kind != b"JSON":
            raise ValueError("Unexpected GLB container.")
        gltf = json.loads(stream.read(size))
    mesh, = gltf["meshes"]
    primitive, = mesh["primitives"]
    material, = gltf["materials"]
    pbr = material["pbrMetallicRoughness"]
    texture_indices = {pbr["baseColorTexture"]["index"], pbr["metallicRoughnessTexture"]["index"],
                       material["normalTexture"]["index"]}
    if len(texture_indices) != 3 or "TANGENT" not in primitive["attributes"] or len(gltf["skins"]) != 1:
        raise ValueError("Expected a single textured skin with authored tangents.")
    vertices = gltf["accessors"][primitive["attributes"]["POSITION"]]["count"]
    indices = gltf["accessors"][primitive["indices"]]["count"]
    if indices % 3 or indices // 3 > 16000:
        raise ValueError(f"{asset} exceeds the triangle budget.")
    manifest = {
        "schema": 1, "asset": asset, "metresPerUnit": 1.0,
        "coordinateSystem": {"up": "+Y", "forward": "+Z"},
        "budgets": {"maxVertices": vertices, "maxIndices": indices,
                    "maxPrimitives": 1, "maxMaterials": 1, "maxTextureLayersPerKind": 1},
        "lods": [{"name": "lod0", "maxTriangles": indices // 3}],
        "requiredSockets": [],
        "runtimeTextureProfile": {"android": "astc", "windows": "rgba8",
                                  "mipmapped": True, "resolution": 1024},
        "materialOverrides": [],
        "distribution": "Private diagnostics",
        "licenceStatus": "Meshy paid API output, exact distribution rights pending",
    }
    model.with_name("asset.manifest.json").write_text(json.dumps(manifest, indent=2) + "\n",
                                                      encoding="utf-8")


def main(source, output, asset, source_hashes):
    if output.exists():
        raise ValueError("Choose an unused resident output directory.")
    for name, sha256 in source_hashes.items():
        if digest(source / name) != sha256:
            raise ValueError(f"Unexpected bytes for {name}.")

    scene, armature, _ = load(source / "rigged.glb")
    scene.frame_set(1)
    neutral = {bone.name: bone.matrix_basis.copy() for bone in armature.pose.bones}
    bind = {bone.name: bone.matrix_local.copy() for bone in armature.data.bones}

    scene, armature, mesh = load(source / "walking.glb")
    if set(bind) != set(armature.data.bones.keys()) or any(
        abs(actual - expected) > 0.005
        for bone in armature.data.bones
        for actual_row, expected_row in zip(bone.matrix_local, bind[bone.name])
        for actual, expected in zip(actual_row, expected_row)
    ):
        raise ValueError("The Meshy pose and walking clip use different bind poses.")
    stock = armature.animation_data.action
    first, last = [round(frame) for frame in stock.frame_range]
    if (first, last) != (1, 32):
        raise ValueError("Re-measure stance contact for a different walking clip.")

    # Mid-stance is the part of each stock step where the planted boot travels
    # backward relative to the hip at nearly constant speed. Blender forward is -Y.
    speeds = []
    for side, start, end in (("Left", 10, 16), ("Right", 25, 31)):
        foot = armature.pose.bones[side + "Foot"]
        sample(scene, start)
        initial = (armature.matrix_world @ foot.head).y
        sample(scene, end)
        speeds.append(((armature.matrix_world @ foot.head).y - initial) * FPS / (end - start))
    cycle = round((last - first) * (sum(speeds) / len(speeds)) / WALK_SPEED)
    if not 31 <= cycle <= 60:
        raise ValueError(f"Unexpected walking pace: {speeds}")

    relaxed = []
    for frame in (first, 16):
        sample(scene, frame)
        relaxed.append({name: armature.pose.bones[name].matrix.to_quaternion() for name in ARMS})
    walk_poses = []
    for index in range(cycle):
        sample(scene, first + (last - first) * index / cycle)
        walk_poses.append({bone.name: bone.matrix_basis.copy() for bone in armature.pose.bones})
    walk_poses.append(walk_poses[0])
    walk = bake(armature, "Warden|walk", walk_poses)
    bpy.data.actions.remove(stock)

    # Use the grounded stance from the original rig and the relaxed arms from
    # two passing poses. Keep a fixed idle so stationary frames reuse the pose.
    armature.animation_data.action = None
    for bone in armature.pose.bones:
        bone.matrix_basis = neutral[bone.name]
    bpy.context.view_layer.update()
    for name in ARMS:
        bone = armature.pose.bones[name]
        target = relaxed[0][name].slerp(relaxed[1][name], 0.5).to_matrix().to_4x4()
        target.translation = bone.matrix.translation
        bone.matrix = target
        bone.location = neutral[name].to_translation()
        bone.scale = neutral[name].to_scale()
        bpy.context.view_layer.update()
    idle_pose = {bone.name: bone.matrix_basis.copy() for bone in armature.pose.bones}
    idle = bake(armature, "Warden|idle", (idle_pose, idle_pose))
    scene.frame_set(1)
    idle_bounds = bounds(mesh)
    low, high = idle_bounds
    if not (abs(low[2]) < 0.004 and 1.63 < high[2] < 1.78 and high[0] - low[0] < 0.59):
        raise ValueError(f"Standing pose does not fit {asset}: {idle_bounds}")
    armature.animation_data.action = walk
    walk_bounds = []
    for frame in range(1, cycle + 2):
        scene.frame_set(frame)
        walk_bounds.append(bounds(mesh))
    if any(not 0.0 <= limits[0][2] < 0.04 or limits[1][2] > 1.78 for limits in walk_bounds):
        raise ValueError("Walking pose is outside the native actor's grounded height envelope.")
    armature.animation_data.action = idle
    scene.frame_start, scene.frame_end = 1, cycle + 1
    scene.frame_set(1)

    output.mkdir(parents=True)
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    mesh.select_set(True)
    bpy.context.view_layer.objects.active = armature
    name = asset.removeprefix("bellwether-")
    bpy.ops.wm.save_as_mainfile(filepath=str(output / f"{name}.blend"))
    model = output / "resident.glb"
    bpy.ops.export_scene.gltf(filepath=str(model), export_format="GLB",
                              use_selection=True, export_tangents=True,
                              export_animation_mode="ACTIONS", export_frame_range=False,
                              export_anim_slide_to_zero=True)
    write_manifest(model, asset)
    (output / "pose-report.json").write_text(json.dumps({
        "inputs": source_hashes, "runtimeGlbSha256": digest(model),
        "walkSeconds": cycle / FPS, "walkMetresPerCycle": cycle * WALK_SPEED / FPS,
        "stockMidStanceMetresPerSecond": speeds, "idleBounds": idle_bounds,
        "walkBounds": walk_bounds,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) not in (2, 3):
        raise ValueError("Expected SOURCE_DIR NEW_OUTPUT_DIR [PROFILE_JSON].")
    source, output = map(Path, args[:2])
    asset, source_hashes = "bellwether-tavern-keeper", KEEPER_SOURCE_HASHES
    if len(args) == 3:
        profile = json.loads(Path(args[2]).read_text(encoding="utf-8"))
        asset, source_hashes = profile["asset"], profile["sourceHashes"]
        if not asset.startswith("bellwether-") or not asset.replace("-", "").isalnum():
            raise ValueError("The resident asset ID must be a Bellwether slug.")
        if set(source_hashes) != set(KEEPER_SOURCE_HASHES):
            raise ValueError("The resident profile must pin rigged.glb and walking.glb.")
    main(source.resolve(), output.resolve(), asset, source_hashes)
