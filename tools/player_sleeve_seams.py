"""Plan bind-boundary subdivisions and skin-weight transfers for sleeve seams.

The helper is intentionally data-only: it identifies already-coincident seam
vertices and returns explicit split/weight plans without mutating input data.
"""

from collections import defaultdict
import math
from typing import Mapping, Sequence


POSITION_QUANTUM_METRES = 1.0e-6
WEIGHT_SUM_TOLERANCE = 1.0e-5
# Retained authoring data reaches 0.999941647 before Blender's GLB exporter
# normalizes skin weights. This tolerance applies only to OLD world maps,
# never to the canonical view weights copied by the plan or to runtime gates.
WORLD_AUTHORING_WEIGHT_SUM_TOLERANCE = 1.0e-4
WEIGHT_AGREEMENT_TOLERANCE = 1.0e-5

_SLEEVE = "ViewmodelSleeves"
_WORLD_CONTINUATION_MATERIALS = frozenset({"NearFacePrimaryMasked", "BodyRemainderPrimaryVisible"})
_WORLD_TARGET_MATERIALS = _WORLD_CONTINUATION_MATERIALS | {"BodyPrimaryVisible"}


def _positions_and_keys(positions: Sequence[Sequence[float]], label: str):
    keys = []
    for vertex, position in enumerate(positions):
        if len(position) != 3:
            raise ValueError(f"{label} vertex {vertex} position must have three coordinates")
        coordinates = tuple(float(value) for value in position)
        if not all(math.isfinite(value) for value in coordinates):
            raise ValueError(f"{label} vertex {vertex} position is not finite")
        keys.append(tuple(round(value / POSITION_QUANTUM_METRES) for value in coordinates))
    return keys


def _validated_weights(weights: Sequence[Mapping[str, float]], vertex_count: int,
                       referenced_vertices, label: str,
                       sum_tolerance=WEIGHT_SUM_TOLERANCE):
    if len(weights) != vertex_count:
        raise ValueError(f"{label} weight roster does not match its vertex count")
    result = []
    for vertex, weight_map in enumerate(weights):
        if not isinstance(weight_map, Mapping):
            raise ValueError(f"{label} vertex {vertex} deform weights must be a mapping")
        if not weight_map:
            if vertex in referenced_vertices:
                raise ValueError(f"{label} referenced vertex {vertex} has no deform weights")
            result.append({})
            continue
        copied = {}
        total = 0.0
        for bone, raw_weight in weight_map.items():
            if not isinstance(bone, str) or not bone.strip():
                raise ValueError(f"{label} vertex {vertex} has an invalid bone name")
            if isinstance(raw_weight, bool):
                raise ValueError(f"{label} vertex {vertex} has a non-numeric deform weight")
            value = float(raw_weight)
            if not math.isfinite(value) or value < 0.0:
                raise ValueError(f"{label} vertex {vertex} has a non-finite or negative weight")
            copied[bone] = value
            total += value
        if abs(total - 1.0) > sum_tolerance:
            raise ValueError(f"{label} vertex {vertex} weights are not normalized (sum={total:g})")
        result.append(copied)
    return result


def _validated_faces(faces, vertex_count: int, label: str):
    result = []
    for face_index, face in enumerate(faces):
        if len(face) != 2:
            raise ValueError(f"{label} face {face_index} must be (material_name, triangle_indices)")
        material, indices = face
        if not isinstance(material, str):
            raise ValueError(f"{label} face {face_index} material name must be a string")
        if len(indices) != 3:
            raise ValueError(f"{label} face {face_index} is not a triangle")
        if any(type(index) is not int or index < 0 or index >= vertex_count for index in indices):
            raise ValueError(f"{label} face {face_index} has an invalid vertex index")
        result.append((material, tuple(indices)))
    return result


def _edge_key(first, second):
    return tuple(sorted((first, second)))


def plan_segmented_sleeve_seam_splits(world_positions, world_faces,
                                     view_positions, view_faces):
    """Plan coarse cloth-edge splits at existing canonical sleeve vertices.

    Only a complete, oppositely wound sleeve boundary backed by the world's
    same BodyPrimaryVisible edges can authorize a split. No proximity welds,
    new surface panels, head/gauntlet edits, or fitted weight fields are planned.
    Apply the existing exact weight-transfer planner AFTER splitting topology.
    """
    world_keys = _positions_and_keys(world_positions, 'world')
    view_keys = _positions_and_keys(view_positions, 'view')
    world_triangles = _validated_faces(world_faces, len(world_positions), 'world')
    view_triangles = _validated_faces(view_faces, len(view_positions), 'view')
    world_edges, view_edges = defaultdict(list), defaultdict(list)
    world_vertex_materials = defaultdict(set)
    for face_index, (material, indices) in enumerate(world_triangles):
        for index in indices:
            world_vertex_materials[index].add(material)
        for a, b in zip(indices, indices[1:] + indices[:1]):
            if world_keys[a] == world_keys[b]:
                raise ValueError('world has a degenerate keyed edge')
            world_edges[_edge_key(world_keys[a], world_keys[b])].append(
                (material, face_index, a, b))
    for material, indices in view_triangles:
        if material != _SLEEVE:
            continue
        for a, b in zip(indices, indices[1:] + indices[:1]):
            if view_keys[a] == view_keys[b]:
                raise ValueError('view sleeve has a degenerate keyed edge')
            view_edges[_edge_key(view_keys[a], view_keys[b])].append((a, b))
    if any(len(records) > 2 for records in view_edges.values()):
        raise ValueError('view sleeve has a nonmanifold edge')
    boundaries = {}
    for edge, records in view_edges.items():
        if len(records) != 1:
            continue
        materials = {record[0] for record in world_edges.get(edge, ())}
        if 'BodyPrimaryVisible' in materials and not (_WORLD_CONTINUATION_MATERIALS & materials):
            boundaries[edge] = records[0]
    tolerance = 2 * POSITION_QUANTUM_METRES
    plans, covered_edges = [], set()
    for edge, records in sorted(world_edges.items()):
        if len(records) != 1 or records[0][0] not in _WORLD_CONTINUATION_MATERIALS:
            continue
        material, face_index, a_index, b_index = records[0]
        a, b = world_positions[a_index], world_positions[b_index]
        delta = tuple(y-x for x,y in zip(a,b))
        length_squared = sum(value*value for value in delta)
        if length_squared <= tolerance*tolerance:
            continue
        length = math.sqrt(length_squared)
        intervals, points, matches = [], {}, []
        for view_edge, (va, vb) in boundaries.items():
            endpoints = (view_positions[va], view_positions[vb])
            if any(value < min(a[axis], b[axis])-tolerance or
                   value > max(a[axis], b[axis])+tolerance
                   for point in endpoints for axis,value in enumerate(point)):
                continue
            parameters = []
            for point in endpoints:
                t = sum((value-origin)*direction for value,origin,direction in zip(point,a,delta))/length_squared
                distance_squared = sum((value-(origin+t*direction))**2
                                       for value,origin,direction in zip(point,a,delta))
                if distance_squared > tolerance*tolerance:
                    break
                parameters.append(t)
            if len(parameters) != 2:
                continue
            if parameters[1] >= parameters[0]:
                raise ValueError('segmented sleeve and retained cloth edges have inconsistent winding')
            lo, hi = max(0., parameters[1]), min(1., parameters[0])
            if hi <= lo:
                continue
            intervals.append((lo, hi))
            matches.append(view_edge)
            for index,t in zip((va,vb),parameters):
                if view_keys[index] not in edge and tolerance/length < t < 1-tolerance/length:
                    points[view_keys[index]] = (t, index)
        if not points:
            continue
        through = 0.
        for lo,hi in sorted(intervals):
            if lo > through+tolerance/length:
                raise ValueError('segmented sleeve only partially covers the retained cloth edge')
            through = max(through,hi)
        if through < 1-tolerance/length:
            raise ValueError('segmented sleeve only partially covers the retained cloth edge')
        if any(not world_vertex_materials[index] <= _WORLD_TARGET_MATERIALS
               for index in (a_index,b_index)):
            raise ValueError('segmented cloth edge shares protected head or gauntlet vertices')
        if covered_edges.intersection(matches):
            raise ValueError('segmented sleeve boundary ambiguously covers multiple cloth edges')
        covered_edges.update(matches)
        plans.append(dict(worldEdge=(a_index,b_index), worldFace=face_index,
            material=material, points=[dict(fraction=t, viewVertex=index,
                position=tuple(view_positions[index])) for t,index in sorted(points.values())]))
    stats = dict(unmatchedViewBoundaryEdges=len(boundaries),
                 coveredViewBoundaryEdges=len(covered_edges),
                 coarseWorldEdges=len(plans),
                 insertedWorldVertices=sum(len(plan['points']) for plan in plans))
    return plans, stats


def plan_sleeve_seam_weight_transfers(
    world_positions: Sequence[Sequence[float]],
    world_faces,
    world_weights: Sequence[Mapping[str, float]],
    view_positions: Sequence[Sequence[float]],
    view_faces,
    view_weights: Sequence[Mapping[str, float]],
):
    """Return ``(world_vertex_to_weights, JSON_safe_stats)`` for proven seams.

    Faces are ``(material_name, (i0, i1, i2))`` triples. Only boundary edges
    of ViewmodelSleeves that coincide with world edges incident to both
    BodyPrimaryVisible and a named world cloth continuation can authorize a
    transfer. The explicit five-region profile splits the historical NearFace
    continuation into NearFace and BodyRemainder without changing this seam.
    """
    world_keys = _positions_and_keys(world_positions, "world")
    view_keys = _positions_and_keys(view_positions, "view")
    world_triangles = _validated_faces(world_faces, len(world_positions), "world")
    view_triangles = _validated_faces(view_faces, len(view_positions), "view")
    world_referenced = {index for _, face in world_triangles for index in face}
    view_referenced = {index for _, face in view_triangles for index in face}
    world_maps = _validated_weights(
        world_weights, len(world_positions), world_referenced, "world",
        WORLD_AUTHORING_WEIGHT_SUM_TOLERANCE)
    view_maps = _validated_weights(
        view_weights, len(view_positions), view_referenced, "view")

    world_bones = {bone for weights in world_maps for bone in weights}
    view_bones = {bone for weights in view_maps for bone in weights}
    unknown_view_bones = view_bones - world_bones
    if unknown_view_bones:
        raise ValueError("view sleeve deform bones are absent from the world rig: " +
                         ", ".join(sorted(unknown_view_bones)))

    sleeve_edge_incidence = defaultdict(int)
    sleeve_endpoint_indices = defaultdict(set)
    for material, indices in view_triangles:
        if material != _SLEEVE:
            continue
        for corner in range(3):
            first = indices[corner]
            second = indices[(corner + 1) % 3]
            first_key = view_keys[first]
            second_key = view_keys[second]
            if first_key == second_key:
                raise ValueError(
                    f"view sleeve face has a degenerate keyed edge: {(material, indices)}")
            key = _edge_key(first_key, second_key)
            sleeve_edge_incidence[key] += 1
            sleeve_endpoint_indices[first_key].add(first)
            sleeve_endpoint_indices[second_key].add(second)
    view_boundary_edges = {edge for edge, count in sleeve_edge_incidence.items() if count == 1}
    if any(count > 2 for count in sleeve_edge_incidence.values()):
        raise ValueError("ViewmodelSleeves contains a nonmanifold edge with more than two incident faces")

    world_edge_materials = defaultdict(set)
    world_vertex_materials = defaultdict(set)
    for material, indices in world_triangles:
        for index in indices:
            world_vertex_materials[index].add(material)
        if material not in _WORLD_TARGET_MATERIALS:
            continue
        for corner in range(3):
            first = indices[corner]
            second = indices[(corner + 1) % 3]
            if world_keys[first] == world_keys[second]:
                raise ValueError(f"world target face has a degenerate keyed edge: {(material, indices)}")
            world_edge_materials[_edge_key(world_keys[first], world_keys[second])].add(material)

    matched_edges = {
        edge for edge in view_boundary_edges
        if 'BodyPrimaryVisible' in world_edge_materials.get(edge, set()) and
           (_WORLD_CONTINUATION_MATERIALS & world_edge_materials.get(edge, set()))
    }
    if not matched_edges:
        raise ValueError("no ViewmodelSleeves boundary edge matches a world edge shared by both target materials")

    matched_endpoint_keys = {point for edge in matched_edges for point in edge}
    canonical = {}
    for point in sorted(matched_endpoint_keys):
        source_indices = sorted(sleeve_endpoint_indices.get(point, ()))
        if not source_indices:
            raise ValueError("matched sleeve seam endpoint has no view vertex record")
        first = view_maps[source_indices[0]]
        for source_index in source_indices[1:]:
            other = view_maps[source_index]
            bones = set(first) | set(other)
            if any(abs(first.get(bone, 0.0) - other.get(bone, 0.0)) >
                   WEIGHT_AGREEMENT_TOLERANCE for bone in bones):
                raise ValueError(
                    f"coincident view sleeve endpoint records conflict at key {point}")
        canonical[point] = dict(first)

    transfers = {}
    for vertex, point in enumerate(world_keys):
        vertex_materials = world_vertex_materials[vertex]
        if point not in canonical or not vertex_materials or not (
            vertex_materials <= _WORLD_TARGET_MATERIALS
        ):
            continue
        transfers[vertex] = dict(canonical[point])
    if not transfers:
        raise ValueError("matched world seam edges contain no transferable target-material vertices")

    stats = {
        "viewSleeveBoundaryEdges": len(view_boundary_edges),
        "matchedWorldSeamEdges": len(matched_edges),
        "transferredWorldVertices": len(transfers),
    }
    return transfers, stats
