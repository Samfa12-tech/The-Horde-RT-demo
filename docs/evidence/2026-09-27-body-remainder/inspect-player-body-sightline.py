"""Exact CPU-upload sightline diagnostic, not a GPU visibility test."""
import json
from pathlib import Path
import sys
import numpy as np

lines = Path(sys.argv[1]).read_text().splitlines()
matrix = np.array([float(v) for v in lines[1].split()[2:]]).reshape(3, 4)
vertices, faces, groups = [], [], []
group = None
for line in lines:
    parts = line.split()
    if parts and parts[0] == 'v':
        vertices.append(list(map(float, parts[1:4])))
    elif parts and parts[0] == 'g':
        group = parts[1]
    elif parts and parts[0] == 'f':
        faces.append([int(v.split('/')[0]) - 1 for v in parts[1:4]])
        groups.append(group)
world = np.array(vertices) @ matrix[:, :3].T + matrix[:, 3]
origin = np.array([-10.65, .70, -15.20])
target = np.array([-11.527283, .073996, -15.07])
direction = target - origin
target_distance = np.linalg.norm(direction)
direction /= target_distance
hits = []
for index, face in enumerate(faces):
    a, b, c = world[face]
    edge1, edge2 = b-a, c-a
    cross = np.cross(direction, edge2)
    determinant = np.dot(edge1, cross)
    if abs(determinant) < 1e-12:
        continue
    relative = origin-a
    u = np.dot(relative, cross)/determinant
    cross2 = np.cross(relative, edge1)
    v = np.dot(direction, cross2)/determinant
    distance = np.dot(edge2, cross2)/determinant
    if u >= 0 and v >= 0 and u+v <= 1 and 0.002 < distance < target_distance:
        hits.append(dict(triangle=index, semantic=groups[index], distanceMetres=float(distance),
                         worldPosition=list(origin+direction*distance),
                         facing='front' if determinant > 0 else 'back'))
camera_model = (origin-matrix[:, 3]) @ matrix[:, :3]
print(json.dumps(dict(source=sys.argv[1], cameraModel=list(camera_model),
                     targetDistanceMetres=float(target_distance),
                     intersections=sorted(hits, key=lambda h:h['distanceMetres'])), indent=2))
