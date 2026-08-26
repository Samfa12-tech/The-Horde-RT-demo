import bpy
import json
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python process-player-rig-runtime.py -- idle.glb walking.glb pbr-directory output.glb"
    )
idle_source, walking_source, pbr_directory, destination = sys.argv[sys.argv.index("--") + 1:]
idle_source = os.path.abspath(idle_source)
walking_source = os.path.abspath(walking_source)
pbr_directory = os.path.abspath(pbr_directory)
destination = os.path.abspath(destination)
os.makedirs(os.path.dirname(destination), exist_ok=True)


def skinned_mesh(objects):
    matches = [obj for obj in objects if obj.type == "MESH" and
               any(modifier.type == "ARMATURE" for modifier in obj.modifiers)]
    if len(matches) != 1:
        raise RuntimeError("player source must contain exactly one skinned mesh")
    return matches[0]


def armature(objects):
    matches = [obj for obj in objects if obj.type == "ARMATURE"]
    if len(matches) != 1:
        raise RuntimeError("player source must contain exactly one armature")
    return matches[0]


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=idle_source, import_shading="NORMALS")
base_objects = set(bpy.context.scene.objects)
player = skinned_mesh(base_objects)
rig = armature(base_objects)
if rig.animation_data is None or rig.animation_data.action is None:
    raise RuntimeError("rigged player source has no rest action")
source_idle_action = rig.animation_data.action
bpy.context.scene.frame_set(int(round(source_idle_action.frame_range[0])))
rest_pose = {}
for bone in rig.pose.bones:
    rest_pose[bone.name] = (
        bone.location.copy(),
        bone.rotation_mode,
        bone.rotation_quaternion.copy(),
        bone.rotation_euler.copy(),
        bone.scale.copy(),
    )

actions_before = set(bpy.data.actions)
bpy.ops.import_scene.gltf(filepath=walking_source, import_shading="NORMALS")
walking_objects = set(bpy.context.scene.objects) - base_objects
walking_rig = armature(walking_objects)
if walking_rig.animation_data is None or walking_rig.animation_data.action is None:
    raise RuntimeError("walking player source has no action")
walking_action = walking_rig.animation_data.action
walking_action.name = "Walking"
walking_action.use_fake_user = True

# Remove the duplicate walking scene objects while preserving its action. The
# action targets pose-bone paths by exact bone name and is rebound to the
# audited base armature below.
bpy.ops.object.select_all(action="DESELECT")
for obj in walking_objects:
    obj.select_set(True)
bpy.ops.object.delete()

if bpy.data.actions.get(source_idle_action.name) is not None:
    bpy.data.actions.remove(source_idle_action)
idle_action = bpy.data.actions.new("Idle")
idle_action.use_fake_user = True
rig.animation_data.action = idle_action
for bone in rig.pose.bones:
    location, rotation_mode, quaternion, euler, scale = rest_pose[bone.name]
    bone.location = location
    bone.rotation_mode = rotation_mode
    bone.rotation_quaternion = quaternion
    bone.rotation_euler = euler
    bone.scale = scale
    for frame in (1, 31):
        bone.keyframe_insert("location", frame=frame, group=bone.name)
        if bone.rotation_mode == "QUATERNION":
            bone.keyframe_insert("rotation_quaternion", frame=frame, group=bone.name)
        else:
            bone.keyframe_insert("rotation_euler", frame=frame, group=bone.name)
        bone.keyframe_insert("scale", frame=frame, group=bone.name)

# Rebind the retained walk action once so Blender records it as compatible
# with this exact armature. Runtime export still includes both named actions.
rig.animation_data.action = walking_action
bpy.context.scene.frame_set(int(round(walking_action.frame_range[0])))

# Restore the accepted Meshy PBR maps. Auto-rig preserved the remesh UVs but
# emitted a simplified display texture; the authoritative PBR maps come from
# the texture-before-rig remesh and are transferred onto the unchanged UVs.
material = bpy.data.materials.new("BodyPrimaryVisible")
material.use_nodes = True
nodes = material.node_tree.nodes
links = material.node_tree.links
principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")


def image_node(filename, colour_space):
    image = bpy.data.images.load(os.path.join(pbr_directory, filename), check_existing=False)
    image.colorspace_settings.name = colour_space
    # Runtime GLB retains only tiny texture-identity placeholders. Audited KTX2
    # arrays carry the bounded Windows/Android PBR payloads.
    image.scale(4, 4)
    node = nodes.new("ShaderNodeTexImage")
    node.image = image
    return node


base = image_node("base-color.png", "sRGB")
normal = image_node("normal.png", "Non-Color")
roughness = image_node("roughness.png", "Non-Color")
metallic = image_node("metallic.png", "Non-Color")
normal_map = nodes.new("ShaderNodeNormalMap")
links.new(base.outputs["Color"], principled.inputs["Base Color"])
links.new(normal.outputs["Color"], normal_map.inputs["Color"])
links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])
links.new(roughness.outputs["Color"], principled.inputs["Roughness"])
links.new(metallic.outputs["Color"], principled.inputs["Metallic"])

player.data.materials.clear()
player.data.materials.append(material)
for name in ("HeadPrimaryMasked", "NearFacePrimaryMasked"):
    copy = material.copy()
    copy.name = name
    player.data.materials.append(copy)

points = [player.matrix_world @ vertex.co for vertex in player.data.vertices]
minimum = Vector(tuple(min(point[axis] for point in points) for axis in range(3)))
maximum = Vector(tuple(max(point[axis] for point in points) for axis in range(3)))
height = maximum.z - minimum.z
head_start = minimum.z + height * 0.86
near_face_start = minimum.z + height * 0.79
semantic_triangles = [0, 0, 0]
for polygon in player.data.polygons:
    centre = sum((player.matrix_world @ player.data.vertices[index].co
                  for index in polygon.vertices), Vector()) / len(polygon.vertices)
    if centre.z >= head_start:
        polygon.material_index = 1
    elif centre.z >= near_face_start and centre.y < 0.02:
        polygon.material_index = 2
    else:
        polygon.material_index = 0
    semantic_triangles[polygon.material_index] += max(0, len(polygon.vertices) - 2)
    polygon.use_smooth = True
if semantic_triangles[1] == 0 or semantic_triangles[2] == 0:
    raise RuntimeError("player semantic material split produced an empty masked primitive")
if not player.data.uv_layers:
    raise RuntimeError("player runtime has no UV set")
player.data.calc_tangents()

# Meshy adds a small non-skinned marker mesh. It is provenance/debug data, not
# part of the runtime player and must not create another primitive or BLAS.
for obj in list(bpy.context.scene.objects):
    if obj.type == "MESH" and obj is not player:
        bpy.data.objects.remove(obj, do_unlink=True)

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 31
rig.animation_data.action = None
for track in list(rig.animation_data.nla_tracks):
    rig.animation_data.nla_tracks.remove(track)
idle_track = rig.animation_data.nla_tracks.new()
idle_track.name = "Idle"
idle_track.strips.new("Idle", 1, idle_action)
walking_track = rig.animation_data.nla_tracks.new()
walking_track.name = "Walking"
walking_track.strips.new("Walking", 1, walking_action)
bpy.ops.export_scene.gltf(
    filepath=destination,
    export_format="GLB",
    export_tangents=True,
    export_animations=True,
    export_animation_mode="NLA_TRACKS",
    export_frame_range=True,
    export_skins=True,
    export_morph=False,
    export_cameras=False,
    export_lights=False,
)

report = {
    "sourceRig": os.path.basename(idle_source),
    "sourceWalk": os.path.basename(walking_source),
    "runtime": os.path.basename(destination),
    "triangles": sum(semantic_triangles),
    "vertices": len(player.data.vertices),
    "heightMetres": height,
    "upAxis": "+Y",
    "forwardAxis": "+Z",
    "origin": "ground-centred within rigging tolerance",
    "bones": [bone.name for bone in rig.data.bones],
    "clips": ["Idle", "Walking"],
    "clipFrames": {"Idle": [1, 31], "Walking": list(walking_action.frame_range)},
    "primitiveSemantics": {
        "BodyPrimaryVisible": semantic_triangles[0],
        "HeadPrimaryMasked": semantic_triangles[1],
        "NearFacePrimaryMasked": semantic_triangles[2],
    },
    "processing": "texture-before-rig PBR UV transfer, idle/walk-only clip packaging, semantic material primitives, generated tangents, 4x4 embedded identity textures",
}
with open(destination + ".processing.json", "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
