"""Read-only bind-correspondence check of separate exact native player uploads.

Reports which viewmodel sleeve boundary edges have retained world-body cloth,
and whether that cloth still meets the posed viewmodel. No geometry is edited.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
from analyze_viewmodel_grip_calibration import read_glb


def key(point):
    return tuple(round(float(value), 6) for value in point)


def load(bind, posed):
    parts = read_glb(bind)
    source = []
    faces = []
    for part in parts:
        base = len(source)
        source.extend(part['positions'])
        for offset in range(0, len(part['indices']), 3):
            faces.append((part['name'], tuple(base + index for index in
                         part['indices'][offset:offset + 3])))
    positions = []
    actual_faces = []
    group = None
    transform = None
    for line in posed.read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if line.startswith('# model_to_world_row_major_3x4'):
            transform = tuple(map(float, fields[2:]))
        if fields[0] == 'v':
            positions.append(tuple(map(float, fields[1:4])))
        elif fields[0] == 'g':
            group = fields[1]
        elif fields[0] == 'f':
            actual_faces.append((group, tuple(int(i.split('/')[0]) - 1 for i in fields[1:])))
    if len(source) != len(positions) or faces != actual_faces:
        raise ValueError('Bind primitive order/indices disagree with exact native upload: ' + str(posed))
    if transform is None or len(transform) != 12:
        raise ValueError('Missing exact model-to-world transform')
    edges = defaultdict(list)
    for material, indices in faces:
        for i, j in zip(indices, indices[1:] + indices[:1]):
            a, b = key(source[i]), key(source[j])
            if a == b:
                raise ValueError('Degenerate geometric edge')
            edge = tuple(sorted((a, b)))
            edges[edge].append((material, {a: i, b: j}))
    return source, positions, edges, transform


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('world_bind', type=Path)
    parser.add_argument('view_bind', type=Path)
    parser.add_argument('world_obj', type=Path)
    parser.add_argument('view_obj', type=Path)
    args = parser.parse_args()
    _, world_pose, world_edges, world_transform = load(args.world_bind, args.world_obj)
    _, view_pose, view_edges, view_transform = load(args.view_bind, args.view_obj)
    if world_transform != view_transform:
        raise ValueError('World and view upload transforms differ; compare in world space first')
    boundary = {}
    graph = defaultdict(set)
    for edge, records in view_edges.items():
        sleeve = [record for record in records if record[0] == 'ViewmodelSleeves']
        if len(sleeve) == 1:
            boundary[edge] = sleeve[0][1]
            a, b = edge
            graph[a].add(b)
            graph[b].add(a)
        elif len(sleeve) > 2:
            raise ValueError('Nonmanifold sleeve edge')
    loops = []
    unseen = set(graph)
    while unseen:
        seed = min(unseen)
        unseen.remove(seed)
        component, stack = {seed}, [seed]
        while stack:
            for adjacent in graph[stack.pop()] & unseen:
                unseen.remove(adjacent)
                component.add(adjacent)
                stack.append(adjacent)
        edges = [edge for edge in boundary if edge[0] in component]
        adjacency = Counter()
        distances = []
        for edge in edges:
            world_records = [record for record in world_edges.get(edge, [])
                             if record[0] != 'BodyPrimaryVisible']
            adjacency[','.join(sorted({record[0] for record in world_records})) or 'missing'] += 1
            for _, indices in world_records:
                for endpoint in edge:
                    distances.append(math.dist(world_pose[indices[endpoint]],
                                               view_pose[boundary[edge][endpoint]]))
        ordered = sorted(distances)
        loops.append(dict(
            bindCentre=[sum(p[axis] for p in component) / len(component) for axis in range(3)],
            vertices=len(component), edges=len(edges),
            degreeCounts=dict(Counter(len(graph[p]) for p in component)),
            retainedWorldAdjacency=dict(adjacency),
            posedEndpointComparisons=len(ordered),
            maximumSeparationMetres=max(ordered) if ordered else None,
            medianSeparationMetres=ordered[len(ordered) // 2] if ordered else None))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    print(json.dumps(dict(
        schema=1, geometricMatchRoundingMetres=1e-6,
        source='Exact CPU uploads, not GPU readback or image acceptance',
        inputs={name: dict(path=str(path), sha256=sha(path)) for name, path in vars(args).items()},
        sharedModelToWorld=world_transform, boundaryEdges=len(boundary),
        loops=loops), indent=2))


if __name__ == '__main__':
    main()
