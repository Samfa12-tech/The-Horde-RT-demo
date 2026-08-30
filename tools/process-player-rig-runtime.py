import bpy
import bmesh
import json
import math
import os
import sys
from mathutils import Vector


if "--" not in sys.argv:
    raise RuntimeError(
        "usage: blender --background --python process-player-rig-runtime.py -- idle.glb walking.glb pbr-directory gauntlet.glb output.glb"
    )
arguments = sys.argv[sys.argv.index("--") + 1:]
if len(arguments) != 5:
    raise RuntimeError("player runtime processing requires the audited gauntlet source")
idle_source, walking_source, pbr_directory, gauntlet_source, destination = arguments
idle_source = os.path.abspath(idle_source)
walking_source = os.path.abspath(walking_source)
pbr_directory = os.path.abspath(pbr_directory)
gauntlet_source = os.path.abspath(gauntlet_source)
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

# Load the accepted Meshy 7 glove as geometry/UV provenance. It is never
# exported as a second object: each side is transformed into the appropriate
# hand frame, mirrored for chirality, fitted over its retained source sleeve,
# and joined into the one skinned player mesh below.
objects_before_gauntlet = set(bpy.context.scene.objects)
bpy.ops.import_scene.gltf(filepath=gauntlet_source, import_shading="NORMALS")
gauntlet_objects = set(bpy.context.scene.objects) - objects_before_gauntlet
gauntlet_meshes = [obj for obj in gauntlet_objects if obj.type == "MESH"]
if len(gauntlet_meshes) != 1:
    raise RuntimeError("gauntlet source must contain exactly one mesh")
gauntlet = gauntlet_meshes[0]
gauntlet_world_vertices = [gauntlet.matrix_world @ vertex.co
                           for vertex in gauntlet.data.vertices]
gauntlet_faces = [tuple(polygon.vertices)
                  for polygon in gauntlet.data.polygons]
gauntlet_uv_layer = gauntlet.data.uv_layers.active
if gauntlet_uv_layer is None:
    raise RuntimeError("gauntlet source has no authored UV0")
gauntlet_face_uvs = [
    [tuple(gauntlet_uv_layer.data[loop].uv)
     for loop in polygon.loop_indices]
    for polygon in gauntlet.data.polygons
]
if len(gauntlet.material_slots) != 1 or \
        gauntlet.material_slots[0].material is None:
    raise RuntimeError("gauntlet source requires one authored PBR material")
gauntlet_source_material = gauntlet.material_slots[0].material
if sum(max(0, len(face) - 2) for face in gauntlet_faces) < 2500:
    raise RuntimeError("gauntlet source geometry is unexpectedly incomplete")
gauntlet_minimum = Vector(tuple(
    min(point[axis] for point in gauntlet_world_vertices)
    for axis in range(3)))
gauntlet_maximum = Vector(tuple(
    max(point[axis] for point in gauntlet_world_vertices)
    for axis in range(3)))
gauntlet_dimensions = gauntlet_maximum - gauntlet_minimum
if gauntlet.get("hordeGauntletSchema") != 1:
    raise RuntimeError("gauntlet source is missing the stripped-grip contract")
gauntlet_grip_source = gauntlet.matrix_world @ Vector(
    gauntlet["hordeGripOrigin"])
gauntlet_source_z = (
    gauntlet.matrix_world.to_3x3() @ Vector(gauntlet["hordeGripAxis"])
).normalized()
gauntlet_source_x = (
    gauntlet.matrix_world.to_3x3() @ Vector(gauntlet["hordeCuffAxis"])
)
gauntlet_source_x = (
    gauntlet_source_x - gauntlet_source_z *
    gauntlet_source_x.dot(gauntlet_source_z)).normalized()
gauntlet_source_y = gauntlet_source_z.cross(gauntlet_source_x).normalized()
gauntlet_palm_reference = (
    gauntlet.matrix_world.to_3x3() @
    Vector(gauntlet["hordePalmInteriorAxis"])).normalized()
if gauntlet_source_y.dot(gauntlet_palm_reference) < 0.0:
    gauntlet_source_x.negate()
    gauntlet_source_y.negate()


closed_grip_finger_vertices = {}
closed_grip_all_vertices = {}
grip_geometry_frames = {}


def bake_closed_grip(hand_name):
    """Curl the rigid Meshy glove around an asset-owned palm grip axis.

    The generated rig has one leaf hand joint and no finger joints.  This
    deterministic source-space correction keeps the weighted wrist seam and
    authored finger topology, bends the fingers along hand-local -Z around the
    hand-local X handle axis, and leaves runtime IK responsible only for the
    complete rigid hand/Grip transform.
    """
    group = player.vertex_groups.get(hand_name)
    hand_bone = rig.data.bones.get(hand_name)
    if group is None or hand_bone is None:
        raise RuntimeError(f"player source is missing {hand_name} grip data")
    armature_from_mesh = rig.matrix_world.inverted_safe() @ player.matrix_world
    bone_from_armature = hand_bone.matrix_local.inverted_safe()
    weighted = []
    for vertex in player.data.vertices:
        weight = next((assignment.weight for assignment in vertex.groups
                       if assignment.group == group.index), 0.0)
        if weight >= 0.10:
            point = bone_from_armature @ (armature_from_mesh @ vertex.co)
            weighted.append((vertex, weight, point))
    if len(weighted) < 300:
        raise RuntimeError(f"{hand_name} has too few weighted glove vertices")
    minimum_y = min(point.y for _, _, point in weighted)
    maximum_y = max(point.y for _, _, point in weighted)
    minimum_z = min(point.z for _, _, point in weighted)
    maximum_z = max(point.z for _, _, point in weighted)
    extent_y = maximum_y - minimum_y
    extent_z = maximum_z - minimum_z
    grip_y = minimum_y + extent_y * 0.50
    centre_z = minimum_z + extent_z * 0.58
    hand_from_world = rig.matrix_world @ hand_bone.matrix_local
    handle_centre_world = hand_from_world @ Vector((0.0, grip_y, centre_z))
    handle_axis_world = (
        hand_from_world.to_3x3() @ Vector((1.0, 0.0, 0.0))).normalized()
    world_up = Vector((0.0, 0.0, 1.0))
    wrap_up = (world_up - handle_axis_world *
               world_up.dot(handle_axis_world)).normalized()
    palm_direction = wrap_up.cross(handle_axis_world).normalized()
    wrist_direction = hand_from_world.translation - handle_centre_world
    if palm_direction.dot(wrist_direction) < 0.0:
        palm_direction.negate()
    mesh_from_world = player.matrix_world.inverted_safe()
    weighted_world = [
        (vertex, hand_weight, player.matrix_world @ vertex.co)
        for vertex, hand_weight, _ in weighted
    ]
    weighted_radial_distances = []
    for _, hand_weight, point in weighted_world:
        if hand_weight < 0.50:
            continue
        from_handle = point - handle_centre_world
        longitudinal = from_handle.dot(handle_axis_world)
        weighted_radial_distances.append(
            (from_handle - handle_axis_world * longitudinal).length)
    finger_start_radius = 0.026
    maximum_source_radius = max(weighted_radial_distances)
    finger_extension = max(
        maximum_source_radius - finger_start_radius, 0.0001)
    maximum_angle = math.radians(245.0)
    viewmodel_fingers = []
    for vertex, hand_weight, point_world in weighted_world:
        from_handle = point_world - handle_centre_world
        longitudinal = from_handle.dot(handle_axis_world)
        original_radial = from_handle - handle_axis_world * longitudinal
        progress = min(max(
            (original_radial.length - finger_start_radius) /
            finger_extension, 0.0), 1.0)
        if progress >= 0.20 and hand_weight >= 0.20:
            viewmodel_fingers.append(vertex.index)
        palm_centre_radial = palm_direction * 0.032 - wrap_up * 0.004
        palm_offset = original_radial - palm_centre_radial
        palm_axis = palm_offset.dot(palm_direction)
        palm_up = palm_offset.dot(wrap_up)
        ellipse_measure = math.sqrt(
            (palm_axis / 0.043) ** 2 + (palm_up / 0.034) ** 2)
        clamped_palm_offset = palm_offset
        if ellipse_measure > 1.0:
            clamped_palm_offset = palm_offset / ellipse_measure
        palm_target_world = (
            handle_centre_world +
            handle_axis_world * max(-0.058, min(0.058, longitudinal)) +
            palm_centre_radial + clamped_palm_offset)
        palm_progress = min(max(progress / 0.30, 0.0), 1.0)
        palm_weight = hand_weight * (1.0 - palm_progress * palm_progress *
                                     (3.0 - 2.0 * palm_progress))
        corrected_world = point_world.lerp(palm_target_world, palm_weight)
        if progress <= 0.0:
            vertex.co = mesh_from_world @ corrected_world
            continue
        surface_radius = min(max(original_radial.length, 0.024), 0.034)
        angle = maximum_angle * progress
        target_world = (
            handle_centre_world + handle_axis_world * longitudinal +
            palm_direction * (math.cos(angle) * surface_radius) +
            wrap_up * (math.sin(angle) * surface_radius))
        # Preserve the wrist seam and authored thumb asymmetry while bending
        # the furthest source surface from palm side, over the top, and beneath
        # a 24-34 mm glove shell around the audited 18 mm prop handle. Radial
        # extension is the stable finger-length signal for both mirrored hands;
        # the generated hand-bone and world-up axes are not.
        correction_weight = hand_weight * (
            progress * progress * (3.0 - 2.0 * progress))
        corrected_world = corrected_world.lerp(target_world, correction_weight)
        vertex.co = mesh_from_world @ corrected_world
    closed_grip_finger_vertices[hand_name] = viewmodel_fingers
    closed_grip_all_vertices[hand_name] = [vertex.index for vertex, _, _ in weighted]
    grip_geometry_frames[hand_name] = {
        "handleCentreWorld": handle_centre_world,
        "handleAxisWorld": handle_axis_world,
        "palmDirectionWorld": palm_direction,
        "wrapUpWorld": wrap_up,
    }
    return {
        "gripY": grip_y,
        "centreZ": centre_z,
        "fingerAxis": "radial extension from asset-owned handle axis",
        "handleAxis": "asset-owned Grip local +Y / hand-local +X",
        "maximumAngleDegrees": 245.0,
        "surfaceRadiusMetres": [0.024, 0.034],
        "maximumSourceRadiusMetres": maximum_source_radius,
        "weightedVertices": len(weighted),
    }


grip_corrections = {
    "LeftHand": bake_closed_grip("LeftHand"),
    "RightHand": bake_closed_grip("RightHand"),
}


def add_grip_bone(hand_name, grip_name, correction):
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    hand = rig.data.edit_bones.get(hand_name)
    if hand is None:
        raise RuntimeError(f"player edit rig is missing {hand_name}")
    grip = rig.data.edit_bones.new(grip_name)
    local_head = Vector((0.0, correction["gripY"], correction["centreZ"]))
    local_tail = local_head + Vector((0.045, 0.0, 0.0))
    grip.head = hand.matrix @ local_head
    grip.tail = hand.matrix @ local_tail
    grip.parent = hand
    grip.use_connect = False
    # Keep a deterministic roll by aligning the child Z axis with the parent
    # hand's local +Z direction. The child Y axis is the handle axis.
    grip.align_roll(hand.matrix.to_3x3() @ Vector((0.0, 0.0, 1.0)))
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.select_set(False)


add_grip_bone("LeftHand", "LeftGrip", grip_corrections["LeftHand"])
add_grip_bone("RightHand", "RightGrip", grip_corrections["RightHand"])


def point_segment_distance(point, start, end):
    extent = end - start
    parameter = max(0.0, min(1.0,
        (point - start).dot(extent) / max(extent.length_squared, 1.0e-9)))
    return (point - (start + extent * parameter)).length


def prepare_source_glove_reference(side):
    """Keep the authored glove stable as the viewmodel bake reference.

    The remeshed garment is an unwelded triangle soup and its glove vertices
    carry small neighbouring-bone weights. Runtime arm IK magnifies those
    weights into spikes. Rigid weighting makes the posed left/right source
    gloves a stable shrinkwrap/data-transfer reference for the welded arm bake.
    """
    hand_name = side + "Hand"
    hand_group = player.vertex_groups[hand_name]
    selected = closed_grip_all_vertices[hand_name]
    if len(selected) < 500:
        raise RuntimeError(f"{side} authored glove selection became incomplete")
    deform_names = {bone.name for bone in rig.data.bones}
    for group in player.vertex_groups:
        if group.name in deform_names:
            group.remove(selected)
    hand_group.add(selected, 1.0, "REPLACE")
    return len(selected)


source_glove_reference_vertices = {
    side: prepare_source_glove_reference(side) for side in ("Left", "Right")
}


def smoothstep01(value):
    amount = max(0.0, min(1.0, value))
    return amount * amount * (3.0 - 2.0 * amount)


def mesh_topology_metrics(mesh):
    graph = [set() for _ in mesh.vertices]
    for edge in mesh.edges:
        left, right = edge.vertices
        graph[left].add(right)
        graph[right].add(left)
    active = {vertex.index for vertex in mesh.vertices
              if graph[vertex.index]}
    component_sizes = []
    while active:
        component_size = 1
        stack = [active.pop()]
        while stack:
            for neighbour in graph[stack.pop()]:
                if neighbour in active:
                    active.remove(neighbour)
                    stack.append(neighbour)
                    component_size += 1
        component_sizes.append(component_size)
    boundary_edges = sum(1 for edge in mesh.edges if edge.is_loose or
                         len(edge.link_faces) == 1) if False else 0
    # MeshEdge does not expose linked faces through the RNA API. Count edge
    # uses directly so the processing report can enforce a closed result.
    uses = {}
    for polygon in mesh.polygons:
        vertices = tuple(polygon.vertices)
        for index in range(len(vertices)):
            edge = tuple(sorted((vertices[index],
                                 vertices[(index + 1) % len(vertices)])))
            uses[edge] = uses.get(edge, 0) + 1
    boundary_edges = sum(1 for count in uses.values() if count != 2)
    component_sizes.sort(reverse=True)
    return len(component_sizes), boundary_edges, component_sizes


def create_authored_viewmodel_gauntlet(side):
    """Bake one rigid authored grip over the character's fitted sleeve.

    The accepted Meshy 7 hand supplies the visible fingers, thumb, palm and
    cuff without any destructive voxel remesh or decimation.  It is rigid to
    the actual Hand bone.  The character's original fitted sleeve is retained
    and reweighted separately so the visible arm follows shoulder, elbow and
    wrist anatomy instead of a synthetic straight cylinder.
    """
    hand_name = side + "Hand"
    frame = grip_geometry_frames[hand_name]
    selected = set(closed_grip_all_vertices[hand_name])
    glove_polygons = [polygon for polygon in player.data.polygons
                      if all(index in selected for index in polygon.vertices)]
    if sum(max(0, len(polygon.vertices) - 2)
           for polygon in glove_polygons) < 350:
        raise RuntimeError(f"{side} source glove reference became incomplete")

    vertices = []
    faces = []

    elbow = rig.matrix_world @ rig.data.bones[side + "ForeArm"].head_local
    wrist = rig.matrix_world @ rig.data.bones[side + "Hand"].head_local
    forearm = wrist - elbow
    forearm_direction = forearm.normalized()

    # Reorient the accepted Meshy 7 glove into this asset-owned grip frame.
    # The generated concept is anatomically a left palm despite the text label;
    # preserve it for Left and mirror the local X axis for a true Right copy.
    # Its long local +Z axis follows the cylindrical power grip/forearm.
    handle_centre = frame["handleCentreWorld"]
    handle_axis = frame["handleAxisWorld"]
    palm_direction = frame["palmDirectionWorld"]
    wrap_up = frame["wrapUpWorld"]
    target_z = handle_axis
    target_x = (-forearm_direction - target_z *
                (-forearm_direction).dot(target_z))
    if target_x.length_squared < 1.0e-6:
        target_x = wrap_up - target_z * wrap_up.dot(target_z)
    target_x.normalize()
    target_y = target_z.cross(target_x).normalized()
    if target_y.dot(palm_direction) < 0.0:
        target_x.negate()
        target_y.negate()
    gauntlet_scale = 0.090
    gauntlet_first = len(vertices)
    for source_point in gauntlet_world_vertices:
        relative = source_point - gauntlet_grip_source
        local = Vector((
            relative.dot(gauntlet_source_x),
            relative.dot(gauntlet_source_y),
            relative.dot(gauntlet_source_z),
        ))
        if side == "Right":
            local.y = -local.y
        vertices.append(tuple(
            handle_centre +
            target_x * (local.x * gauntlet_scale) +
            target_y * (local.y * gauntlet_scale) +
            target_z * (local.z * gauntlet_scale)))
    for source_face in gauntlet_faces:
        face = tuple(gauntlet_first + index for index in source_face)
        faces.append(tuple(reversed(face)) if side == "Right" else face)

    mesh = bpy.data.meshes.new(f"{side}AuthoredViewmodelGauntletMesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    authored_uv = mesh.uv_layers.new(name="UVMap")
    if len(mesh.polygons) != len(gauntlet_face_uvs):
        raise RuntimeError(f"{side} gauntlet polygon order changed before UV copy")
    for polygon, source_uvs in zip(mesh.polygons, gauntlet_face_uvs):
        if len(polygon.loop_indices) != len(source_uvs):
            raise RuntimeError(f"{side} gauntlet loop order changed before UV copy")
        for loop, uv in zip(polygon.loop_indices, source_uvs):
            authored_uv.data[loop].uv = uv
    gauntlet_object = bpy.data.objects.new(
        f"{side}AuthoredViewmodelGauntlet", mesh)
    bpy.context.collection.objects.link(gauntlet_object)
    bpy.ops.object.select_all(action="DESELECT")
    gauntlet_object.select_set(True)
    bpy.context.view_layer.objects.active = gauntlet_object
    edit_mesh = bmesh.new()
    edit_mesh.from_mesh(gauntlet_object.data)
    bmesh.ops.triangulate(edit_mesh, faces=list(edit_mesh.faces))
    edit_mesh.to_mesh(gauntlet_object.data)
    edit_mesh.free()
    gauntlet_object.data.update()

    hand_group = gauntlet_object.vertex_groups.new(name=hand_name)
    primary_group = gauntlet_object.vertex_groups.new(
        name="ViewmodelPrimary" + side)
    gauntlet_group = gauntlet_object.vertex_groups.new(
        name="ViewmodelGauntlet" + side)
    rigid_gauntlet_vertices = 0
    for vertex in gauntlet_object.data.vertices:
        # Preserve the reviewed finger/palm silhouette exactly.  A hand has
        # no finger bones in this compact rig, so neighbouring arm weights
        # would tear the grip apart under IK.
        hand_group.add([vertex.index], 1.0, "REPLACE")
        primary_group.add([vertex.index], 1.0, "REPLACE")
        gauntlet_group.add([vertex.index], 1.0, "REPLACE")
        rigid_gauntlet_vertices += 1
    for polygon in gauntlet_object.data.polygons:
        polygon.use_smooth = True

    components, boundary_edges, component_sizes = mesh_topology_metrics(
        gauntlet_object.data)
    if rigid_gauntlet_vertices != len(gauntlet_world_vertices):
        raise RuntimeError(f"{side} authored gauntlet lost rigid hand weights")
    return gauntlet_object, {
        "triangles": sum(max(0, len(polygon.vertices) - 2)
                         for polygon in gauntlet_object.data.polygons),
        "vertices": len(gauntlet_object.data.vertices),
        "components": components,
        "componentVertexCounts": component_sizes,
        "boundaryEdges": boundary_edges,
        "rigidGauntletVertices": rigid_gauntlet_vertices,
        "sleeveVertices": 0,
        "uvSource": "accepted Meshy glove/bracer authored loop UV0 preserved through fitted transform",
        "gripConstruction": "accepted Meshy 7 anatomical gauntlet rigid to Hand; side-mirrored by chirality; authored fitted character sleeve retained beneath cuff",
        "gauntletScale": gauntlet_scale,
        "handleForearmDot": handle_axis.dot(forearm_direction),
    }


authored_viewmodel_gauntlets = {}
authored_viewmodel_gauntlet_metrics = {}
for side in ("Left", "Right"):
    (authored_viewmodel_gauntlets[side],
     authored_viewmodel_gauntlet_metrics[side]) = \
        create_authored_viewmodel_gauntlet(side)


def remove_replaced_source_glove_faces():
    """Remove only closed faces wholly owned by either replaced source hand."""
    replaced = {
        index for hand in ("LeftHand", "RightHand")
        for index in closed_grip_all_vertices[hand]
    }
    bpy.context.view_layer.objects.active = player
    player.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    mesh = bmesh.from_edit_mesh(player.data)
    mesh.verts.ensure_lookup_table()
    faces = [face for face in mesh.faces
             if all(vertex.index in replaced for vertex in face.verts)]
    if len(faces) < 700:
        raise RuntimeError(
            "source glove replacement did not find the audited face envelope")
    removed = len(faces)
    bmesh.ops.delete(mesh, geom=faces, context="FACES")
    bmesh.update_edit_mesh(player.data, loop_triangles=True, destructive=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    player.select_set(False)
    return removed


replaced_source_glove_faces = remove_replaced_source_glove_faces()


def prepare_source_viewmodel_sleeves():
    """Retain and stabilise the character's fitted anatomical sleeves.

    The source character already has a convincing shoulder-to-wrist garment.
    Keep faces unambiguously owned by each arm, mark them for primary rays and
    replace fragmented auto-rig weights with a bounded two-segment anatomical
    blend.  This eliminates both the old tearing fans and the later synthetic
    cylinder arms while leaving shoulder/torso coat panels untouched.
    """
    group_indices = {
        side: {
            player.vertex_groups[side + "Arm"].index,
            player.vertex_groups[side + "ForeArm"].index,
        }
        for side in ("Left", "Right")
    }
    # The source sleeves contain a few 110 mm bind-pose triangle edges.  They
    # look fine in the authored walk but can stretch beyond 200 mm under the
    # strongest wall-retracted IK pose.  One offline subdivision of only the
    # unambiguous sleeve faces preserves the fitted silhouette and interpolates
    # UVs while giving the bounded two-bone blend local edges to deform.
    source_sleeve_faces = []
    for face in player.data.polygons:
        if any(all(sum(assignment.weight
                       for assignment in player.data.vertices[index].groups
                       if assignment.group in indices) >= 0.82
                   for index in face.vertices)
               for indices in group_indices.values()):
            source_sleeve_faces.append(face.index)
    if len(source_sleeve_faces) < 500:
        raise RuntimeError("source sleeve subdivision envelope became incomplete")
    bpy.context.view_layer.objects.active = player
    player.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="DESELECT")
    edit_mesh = bmesh.from_edit_mesh(player.data)
    edit_mesh.faces.ensure_lookup_table()
    for index in source_sleeve_faces:
        edit_mesh.faces[index].select = True
    bmesh.update_edit_mesh(player.data, loop_triangles=True, destructive=False)
    bpy.ops.mesh.subdivide(number_cuts=1, smoothness=0.0)
    bpy.ops.object.mode_set(mode="OBJECT")
    player.select_set(False)

    deform_names = {bone.name for bone in rig.data.bones}
    metrics = {}
    for side, indices in group_indices.items():
        face_indices = []
        vertex_indices = set()
        triangles = 0
        for face in player.data.polygons:
            if not all(sum(assignment.weight
                           for assignment in player.data.vertices[index].groups
                           if assignment.group in indices) >= 0.82
                       for index in face.vertices):
                continue
            face_indices.append(face.index)
            vertex_indices.update(face.vertices)
            triangles += max(0, len(face.vertices) - 2)
        if triangles < 120:
            raise RuntimeError(
                f"{side} source sleeve envelope became incomplete: {triangles}")

        for group in player.vertex_groups:
            if group.name in deform_names:
                group.remove(list(vertex_indices))

        arm_group = player.vertex_groups[side + "Arm"]
        forearm_group = player.vertex_groups[side + "ForeArm"]
        hand_group = player.vertex_groups[side + "Hand"]
        primary_group = player.vertex_groups.get("ViewmodelPrimary" + side)
        if primary_group is None:
            primary_group = player.vertex_groups.new(
                name="ViewmodelPrimary" + side)

        shoulder = rig.matrix_world @ rig.data.bones[side + "Arm"].head_local
        elbow = rig.matrix_world @ rig.data.bones[side + "ForeArm"].head_local
        wrist = rig.matrix_world @ rig.data.bones[side + "Hand"].head_local
        upper = elbow - shoulder
        lower = wrist - elbow
        mixed_elbow_vertices = 0
        mixed_wrist_vertices = 0
        for index in sorted(vertex_indices):
            point = player.matrix_world @ player.data.vertices[index].co
            upper_t = max(0.0, min(1.0,
                (point - shoulder).dot(upper) /
                max(upper.length_squared, 1.0e-9)))
            lower_t = max(0.0, min(1.0,
                (point - elbow).dot(lower) /
                max(lower.length_squared, 1.0e-9)))
            upper_distance = point_segment_distance(point, shoulder, elbow)
            lower_distance = point_segment_distance(point, elbow, wrist)
            if upper_distance <= lower_distance:
                forearm_amount = smoothstep01((upper_t - 0.72) / 0.28)
                arm_amount = 1.0 - forearm_amount
                hand_amount = 0.0
            else:
                hand_amount = smoothstep01((lower_t - 0.78) / 0.22)
                forearm_amount = 1.0 - hand_amount
                arm_amount = 0.0
            if arm_amount > 0.0001:
                arm_group.add([index], arm_amount, "REPLACE")
            if forearm_amount > 0.0001:
                forearm_group.add([index], forearm_amount, "REPLACE")
            if hand_amount > 0.0001:
                hand_group.add([index], hand_amount, "REPLACE")
            primary_group.add([index], 1.0, "REPLACE")
            if arm_amount > 0.05 and forearm_amount > 0.05:
                mixed_elbow_vertices += 1
            if forearm_amount > 0.05 and hand_amount > 0.05:
                mixed_wrist_vertices += 1

        metrics[side] = {
            "triangles": triangles,
            "vertices": len(vertex_indices),
            "subdivision": "one global fitted-sleeve cut for bounded two-bone deformation",
            "mixedArmForeArmVertices": mixed_elbow_vertices,
            "mixedForeArmHandVertices": mixed_wrist_vertices,
            "construction": "retained fitted Meshy sleeve with bounded shoulder-elbow-wrist reweight",
        }
    return metrics


source_viewmodel_sleeve_metrics = prepare_source_viewmodel_sleeves()


gauntlet_objects = list(authored_viewmodel_gauntlets.values())

bpy.ops.object.select_all(action="DESELECT")
player.select_set(True)
for gauntlet_object in gauntlet_objects:
    gauntlet_object.select_set(True)
bpy.context.view_layer.objects.active = player
bpy.ops.object.join()

if bpy.data.actions.get(source_idle_action.name) is not None:
    bpy.data.actions.remove(source_idle_action)
idle_action = bpy.data.actions.new("Idle")
idle_action.use_fake_user = True
rig.animation_data.action = idle_action
for bone in rig.pose.bones:
    if bone.name in rest_pose:
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

# The accepted gauntlet has its own non-overlapping authored UV atlas. Keep a
# distinct material/texture identity so runtime arrays can route its genuine
# leather-and-metal PBR payload instead of projecting the traveller atlas onto
# unrelated hand topology. Embedded GLB images remain 4x4 identity markers;
# the audited Windows/Android KTX2 arrays carry the real 1K layers.
gauntlet_material = gauntlet_source_material.copy()
gauntlet_material.name = "GauntletPrimaryVisible"
if not gauntlet_material.use_nodes:
    raise RuntimeError("gauntlet PBR material lost its node graph")
for node in gauntlet_material.node_tree.nodes:
    if node.type == "TEX_IMAGE" and node.image is not None:
        placeholder = node.image.copy()
        placeholder.name = "RuntimeGauntlet_" + node.image.name
        placeholder.scale(4, 4)
        node.image = placeholder


player.data.materials.clear()
player.data.materials.append(material)
player.data.materials.append(gauntlet_material)
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
viewmodel_group_indices = {}
gauntlet_group_indices = {}
for side in ("Left", "Right"):
    marker = player.vertex_groups.get("ViewmodelPrimary" + side)
    if marker is None:
        raise RuntimeError(f"player is missing {side} viewmodel marker")
    viewmodel_group_indices[side] = marker.index
    gauntlet_marker = player.vertex_groups.get("ViewmodelGauntlet" + side)
    if gauntlet_marker is None:
        raise RuntimeError(f"player is missing {side} gauntlet marker")
    gauntlet_group_indices[side] = gauntlet_marker.index


semantic_triangles = [0, 0, 0, 0]
primary_side_triangles = {"Left": 0, "Right": 0}
gauntlet_side_triangles = {"Left": 0, "Right": 0}
for polygon in player.data.polygons:
    polygon_points = [player.matrix_world @ player.data.vertices[index].co
                      for index in polygon.vertices]
    centre = sum(polygon_points, Vector()) / len(polygon_points)
    primary_side = next((
        side for side, group_index in viewmodel_group_indices.items()
        if all(any(assignment.group == group_index and assignment.weight >= 0.99
                   for assignment in player.data.vertices[index].groups)
               for index in polygon.vertices)
    ), None)
    gauntlet_side = next((
        side for side, group_index in gauntlet_group_indices.items()
        if all(any(assignment.group == group_index and assignment.weight >= 0.99
                   for assignment in player.data.vertices[index].groups)
               for index in polygon.vertices)
    ), None)
    maximum_edge_metres = max(
        (polygon_points[(edge + 1) % len(polygon_points)] -
         polygon_points[edge]).length
        for edge in range(len(polygon_points)))
    if gauntlet_side is not None and maximum_edge_metres <= 0.12:
        polygon.material_index = 1
        gauntlet_side_triangles[gauntlet_side] += max(
            0, len(polygon.vertices) - 2)
        primary_side_triangles[gauntlet_side] += max(
            0, len(polygon.vertices) - 2)
    elif primary_side is not None and maximum_edge_metres <= 0.12:
        # Primary rays see only the explicit anatomical grip surfaces and the
        # compact bone-bound leather bracers. The complete Meshy body is
        # retained below for reflection and shadow rays without exposing its
        # cross-weighted coat panels or distorted source glove to the camera.
        polygon.material_index = 0
        primary_side_triangles[primary_side] += max(
            0, len(polygon.vertices) - 2)
    elif centre.z >= head_start:
        polygon.material_index = 2
    else:
        # The camera remains inside the complete, boot-grounded body. Torso,
        # pelvis and legs stay available to shadow/reflection rays while only
        # the two bounded authored viewmodel arm surfaces enter primary rays.
        polygon.material_index = 3
    semantic_triangles[polygon.material_index] += max(0, len(polygon.vertices) - 2)
    polygon.use_smooth = True
if semantic_triangles[1] == 0 or semantic_triangles[2] == 0 or \
        semantic_triangles[3] == 0:
    raise RuntimeError("player semantic material split produced an empty masked primitive")
if (primary_side_triangles["Left"] < 500 or
        primary_side_triangles["Right"] < 450):
    raise RuntimeError(
        "player authored same-side arm partition became incomplete: " +
        repr(primary_side_triangles))
if not player.data.uv_layers:
    raise RuntimeError("player runtime has no UV set")
player.data.calc_tangents()


def sample_boot_minimum_profile(first, last, sample_count=64):
    """Bake the asset-owned sole offset used before runtime arm IK.

    Blender is Z-up while the exported GLB is Y-up, so the evaluated world-Z
    minimum is the runtime model-Y minimum. Sampling the complete authored
    mesh avoids a brittle boot vertex list and is an offline-only cost.
    """
    span = max(last - first, 0.0001)
    profile = []
    for sample in range(sample_count):
        frame = first + span * sample / sample_count
        integer_frame = math.floor(frame)
        bpy.context.scene.frame_set(
            integer_frame, subframe=frame - integer_frame)
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated_object = player.evaluated_get(depsgraph)
        evaluated_mesh = evaluated_object.to_mesh()
        minimum_up = min(
            (player.matrix_world @ vertex.co).z
            for vertex in evaluated_mesh.vertices)
        evaluated_object.to_mesh_clear()
        if not math.isfinite(minimum_up) or abs(minimum_up) > 0.075:
            raise RuntimeError(
                "player boot grounding profile exceeded +/-75 mm: " +
                repr(minimum_up))
        profile.append(minimum_up)
    return profile


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
idle_strip = idle_track.strips.new("Idle", 1, idle_action)
walking_track = rig.animation_data.nla_tracks.new()
walking_track.name = "Walking"
walking_strip = walking_track.strips.new("Walking", 1, walking_action)

# Evaluate the exact NLA tracks that the GLB exporter will package. Assigning
# the imported walking Action directly before the NLA bind left the base rest
# pose active and incorrectly produced an all-zero profile.
walking_track.mute = True
idle_boot_minimum_y = sample_boot_minimum_profile(
    idle_strip.frame_start, idle_strip.frame_end)
idle_track.mute = True
walking_track.mute = False
walking_boot_minimum_y = sample_boot_minimum_profile(
    walking_strip.frame_start, walking_strip.frame_end)
idle_track.mute = False
walking_track.mute = False
rig["hordeBootGroundingSchema"] = 1
rig["hordeIdleBootMinimumY"] = idle_boot_minimum_y
rig["hordeWalkingBootMinimumY"] = walking_boot_minimum_y

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
    export_extras=True,
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
    "gripCorrection": grip_corrections,
    "sourceGloveReferenceVertices": source_glove_reference_vertices,
    "replacedSourceGloveFaces": replaced_source_glove_faces,
    "sourceViewmodelSleeves": source_viewmodel_sleeve_metrics,
    "authoredViewmodelGauntlets": authored_viewmodel_gauntlet_metrics,
    "authoredPrimarySideTriangles": primary_side_triangles,
    "authoredGauntletSideTriangles": gauntlet_side_triangles,
    "authoredPrimaryTriangles": semantic_triangles[0],
    "clips": ["Idle", "Walking"],
    "clipFrames": {"Idle": [1, 31], "Walking": list(walking_action.frame_range)},
    "bootGrounding": {
        "schema": 1,
        "samplesPerLoop": len(idle_boot_minimum_y),
        "idleMinimumYRangeMetres": [
            min(idle_boot_minimum_y), max(idle_boot_minimum_y)],
        "walkingMinimumYRangeMetres": [
            min(walking_boot_minimum_y), max(walking_boot_minimum_y)],
    },
    "primitiveSemantics": {
        "BodyPrimaryVisible": semantic_triangles[0],
        "GauntletPrimaryVisible": semantic_triangles[1],
        "HeadPrimaryMasked": semantic_triangles[2],
        "NearFacePrimaryMasked": semantic_triangles[3],
    },
    "processing": "texture-before-rig PBR UV transfer, retained fitted character sleeves with bounded shoulder-elbow-wrist reweighting, accepted Meshy 7 grip surfaces rigid to Hand and mirrored by chirality without voxel remesh or decimation, gauntlet authored loop UV0 and distinct PBR material retained, asset-owned LeftGrip/RightGrip sockets, replaced unstable source glove surfaces, complete boot-grounded body retained for reflection and shadow rays, idle/walk-only clip packaging, semantic material primitives, generated tangents, 4x4 embedded identity textures",
}
with open(destination + ".processing.json", "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
