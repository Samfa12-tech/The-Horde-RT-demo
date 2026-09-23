"""Run with Blender --background --python-exit-code 1 --python this-file.py."""
from collections import Counter
from pathlib import Path
import sys
import unittest

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_viewmodel_surface import close_sleeve_openings


class SleeveSurfaceTests(unittest.TestCase):
    def setUp(self):
        bpy.ops.wm.read_factory_settings(use_empty=True)

    def make_mesh(self, shared_glove_vertex=False, closed=False, crossing_surfaces=False):
        points = [(-1,-1,0),(1,-1,0),(1,1,0),(-1,1,0),
                  (-1,-1,2),(1,-1,2),(1,1,2),(-1,1,2),
                  (4,0,0),(5,0,0),(4,1,0)]
        faces = [(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
        if crossing_surfaces:
            offset = len(points)
            points += [(x+.5,y+.5,z-.5) for x,y,z in points[:8]]
            faces += [tuple(i+offset for i in face) for face in faces[:4]]
        if closed:
            faces += [(3,2,1,0),(4,5,6,7)]
        faces += [(0 if shared_glove_vertex else 8,9,10)]
        mesh = bpy.data.meshes.new('fixture')
        mesh.from_pydata(points, [], faces)
        obj = bpy.data.objects.new('fixture', mesh)
        bpy.context.collection.objects.link(obj)
        for name in ('ViewmodelSleeves', 'ViewmodelGauntlets'):
            mesh.materials.append(bpy.data.materials.new(name))
        mesh.polygons[-1].material_index = 1
        uv = mesh.uv_layers.new(name='UVMap')
        for face in mesh.polygons:
            for n, loop in enumerate(face.loop_indices):
                uv.data[loop].uv = (n * .13, face.index * .17)
        mesh.update()
        return obj

    @staticmethod
    def glove_corners(obj):
        mesh = obj.data
        return [(tuple(mesh.vertices[mesh.loops[i].vertex_index].co),
                 tuple(mesh.corner_normals[i].vector), tuple(mesh.uv_layers.active.data[i].uv))
                for face in mesh.polygons if face.material_index == 1 for i in face.loop_indices]

    def test_closes_openings_without_changing_glove_corners(self):
        obj = self.make_mesh()
        before = self.glove_corners(obj)
        cloth_normals = {frozenset(tuple(obj.data.vertices[i].co) for i in face.vertices):
                         [tuple(obj.data.corner_normals[i].vector) for i in face.loop_indices]
                         for face in obj.data.polygons if face.material_index == 0}
        report = close_sleeve_openings(obj)
        self.assertEqual(report['boundaryEdgesBefore'], 8)
        self.assertEqual(report['boundaryEdgesAfter'], 0)
        self.assertEqual(report['capFaces'], 2)
        self.assertEqual(report['trianglesAdded'], 8)
        self.assertEqual(self.glove_corners(obj), before)
        self.assertNotIn('hordeOriginalFace', obj.data.attributes)
        self.assertNotIn('hordeOriginalVertex', obj.data.attributes)
        edges = Counter()
        for face in obj.data.polygons:
            if face.material_index != 0:
                continue
            points = [tuple(obj.data.vertices[i].co) for i in face.vertices]
            for a, b in zip(points, points[1:] + points[:1]):
                edges[tuple(sorted((a,b)))] += 1
        self.assertTrue(edges)
        self.assertTrue(all(count == 2 for count in edges.values()))
        for face in obj.data.polygons:
            if face.material_index == 0 and len(face.vertices) == 4:
                key = frozenset(tuple(obj.data.vertices[i].co) for i in face.vertices)
                self.assertEqual([tuple(obj.data.corner_normals[i].vector) for i in face.loop_indices], cloth_normals[key])
            # The source fixture is quads; only the new panels are triangles.
            if face.material_index == 0 and len(face.vertices) == 3:
                for loop in face.loop_indices:
                    self.assertGreater(face.normal.dot(obj.data.corner_normals[loop].vector), .99999)

    def test_shared_glove_vertex_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'may not merge a gauntlet vertex'):
            close_sleeve_openings(self.make_mesh(shared_glove_vertex=True))

    def test_already_closed_source_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'Expected authored sleeve openings'):
            close_sleeve_openings(self.make_mesh(closed=True))

    def test_crossing_panels_are_rejected_even_when_edges_close(self):
        with self.assertRaisesRegex(RuntimeError, 'intersects authored cloth'):
            close_sleeve_openings(self.make_mesh(crossing_surfaces=True))


unittest.main(argv=[__file__])
