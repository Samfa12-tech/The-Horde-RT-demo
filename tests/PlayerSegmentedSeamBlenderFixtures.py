"""Run with Blender --background --threads 1 --python-exit-code 1 --python this-file.py."""
from pathlib import Path
import sys
import unittest

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_sleeve_seams import plan_segmented_sleeve_seam_splits
from player_segmented_seams_blender import split_segmented_cloth_seams


BODY = 'BodyRemainderPrimaryVisible'
NEAR = 'NearFacePrimaryMasked'
HEAD = 'HeadPrimaryMasked'
UNTOUCHED = 'UntouchedCustomNormals'
BOOKKEEPING = {
    'hordeSeamSourceFace', 'hordeSeamSourceVertex', 'hordeSeamNewVertex'
}


def close_tuple(actual, expected, tolerance=1.0e-6):
    return len(actual) == len(expected) and all(
        abs(a - b) <= tolerance for a, b in zip(actual, expected))


def normalized(values):
    length = sum(value * value for value in values) ** 0.5
    return tuple(value / length for value in values)


class SegmentedSeamBlenderFixtures(unittest.TestCase):
    def setUp(self):
        bpy.ops.wm.read_factory_settings(use_empty=True)

    def make_mesh(self, face_materials, untouched=False):
        points = [(0.0, 0.0, 0.0), (2.0, 0.0, 0.0),
                  (0.0, 2.0, 0.0), (0.0, -2.0, 0.0)]
        faces = [(0, 1, 2)]
        if len(face_materials) == 2:
            faces.append((1, 0, 3))
        if untouched:
            offset = len(points)
            points.extend([(10.0, 0.0, 0.0), (12.0, 0.0, 0.0),
                           (10.0, 2.0, 0.0)])
            faces.append((offset, offset + 1, offset + 2))
            face_materials = list(face_materials) + [UNTOUCHED]
        mesh = bpy.data.meshes.new('segmented-cloth-fixture')
        mesh.from_pydata(points, [], faces)
        mesh.materials.clear()
        for name in dict.fromkeys(face_materials):
            mesh.materials.append(bpy.data.materials.new(name))
        material_indices = {material.name: index
                            for index, material in enumerate(mesh.materials)}
        for polygon, name in zip(mesh.polygons, face_materials):
            polygon.material_index = material_indices[name]
            polygon.use_smooth = True

        uv_layer = mesh.uv_layers.new(name='UVMap')
        uv_by_material = {
            BODY: {0: (0.0, 0.0), 1: (2.0, 0.0), 2: (0.0, 2.0)},
            NEAR: {0: (10.0, 10.0), 1: (14.0, 10.0), 3: (10.0, 12.0)},
            HEAD: {0: (20.0, 20.0), 1: (22.0, 20.0), 2: (20.0, 22.0)},
            UNTOUCHED: {4: (30.0, 30.0), 5: (32.0, 30.0), 6: (30.0, 32.0)},
        }
        normal_by_material = {
            BODY: {0: (1.0, 0.0, 0.0), 1: (0.0, 1.0, 0.0),
                   2: (0.0, 0.0, 1.0)},
            NEAR: {0: (0.0, 0.0, 1.0), 1: (0.0, 1.0, 0.0),
                   3: (1.0, 0.0, 0.0)},
            HEAD: {0: (0.0, 1.0, 0.0), 1: (1.0, 0.0, 0.0),
                   2: (0.0, 0.0, 1.0)},
            UNTOUCHED: {4: (1.0, 0.173, 0.231),
                        5: (0.23, 1.0, 0.117),
                        6: (0.191, 0.291, 1.0)},
        }
        split_normals = [None] * len(mesh.loops)
        for polygon, name in zip(mesh.polygons, face_materials):
            for loop_index in polygon.loop_indices:
                vertex_index = mesh.loops[loop_index].vertex_index
                uv_layer.data[loop_index].uv = uv_by_material[name][vertex_index]
                split_normals[loop_index] = normal_by_material[name][vertex_index]
        mesh.normals_split_custom_set(split_normals)
        mesh.update()

        obj = bpy.data.objects.new('segmented-cloth-fixture', mesh)
        bpy.context.collection.objects.link(obj)
        groups = {name: obj.vertex_groups.new(name=name)
                  for name in ('LeftArm', 'LeftForeArm', 'Torso')}
        groups['LeftArm'].add([0], 1.0, 'REPLACE')
        groups['LeftForeArm'].add([1], 1.0, 'REPLACE')
        groups['Torso'].add([2, 3], 1.0, 'REPLACE')
        mesh.update()
        return obj

    @staticmethod
    def vertex_weights(obj, vertex_index):
        group_names = {group.index: group.name for group in obj.vertex_groups}
        return {group_names[item.group]: item.weight
                for item in obj.data.vertices[vertex_index].groups}

    @staticmethod
    def capture_originals(obj):
        mesh = obj.data
        vertices = {
            tuple(vertex.co): SegmentedSeamBlenderFixtures.vertex_weights(
                obj, vertex.index)
            for vertex in mesh.vertices
        }
        corners = {}
        for polygon in mesh.polygons:
            material = mesh.materials[polygon.material_index].name
            for loop_index in polygon.loop_indices:
                vertex_index = mesh.loops[loop_index].vertex_index
                point = tuple(mesh.vertices[vertex_index].co)
                corners[(material, point)] = (
                    tuple(mesh.uv_layers['UVMap'].data[loop_index].uv),
                    tuple(mesh.corner_normals[loop_index].vector),
                )
        return vertices, corners

    def plan(self, fractions, material=BODY):
        return [{
            'worldEdge': (0, 1),
            'worldFace': 0,
            'material': material,
            'points': [
                {'fraction': fraction,
                 'viewVertex': 100 + index,
                 'position': (2.0 * fraction, 0.0, 0.0)}
                for index, fraction in enumerate(fractions)
            ],
        }]

    def assert_success_case(self, face_materials, fractions):
        obj = self.make_mesh(face_materials)
        mesh = obj.data
        before_vertices, before_corners = self.capture_originals(obj)
        original_triangle_count = len(mesh.polygons)
        report = split_segmented_cloth_seams(mesh, self.plan(fractions))

        self.assertEqual(report['coarseEdges'], 1)
        self.assertEqual(report['verticesAdded'], len(fractions))
        self.assertEqual(report['trianglesAdded'], len(face_materials) * len(fractions))
        self.assertEqual(len(mesh.polygons),
                         original_triangle_count + len(face_materials) * len(fractions))
        self.assertTrue(all(len(polygon.vertices) == 3 for polygon in mesh.polygons))
        self.assertFalse(BOOKKEEPING & {attribute.name for attribute in mesh.attributes})

        after_points = {tuple(vertex.co): self.vertex_weights(obj, vertex.index)
                        for vertex in mesh.vertices}
        for point, weights in before_vertices.items():
            self.assertIn(point, after_points)
            self.assertEqual(after_points[point], weights)
        for fraction in fractions:
            point = (2.0 * fraction, 0.0, 0.0)
            self.assertIn(point, after_points)
            self.assertAlmostEqual(after_points[point].get('LeftArm', 0.0), 1.0 - fraction, places=6)
            self.assertAlmostEqual(after_points[point].get('LeftForeArm', 0.0), fraction, places=6)

        material_corner_records = {}
        new_corner_records = {}
        for polygon in mesh.polygons:
            material = mesh.materials[polygon.material_index].name
            for loop_index in polygon.loop_indices:
                point = tuple(mesh.vertices[mesh.loops[loop_index].vertex_index].co)
                record = (tuple(mesh.uv_layers['UVMap'].data[loop_index].uv),
                          tuple(mesh.corner_normals[loop_index].vector))
                if point in before_vertices:
                    expected_corner = before_corners[(material, point)]
                    self.assertEqual(record[0], expected_corner[0])
                    self.assertTrue(close_tuple(record[1], expected_corner[1], 1.0e-5),
                                    (material, point, record[1], expected_corner[1]))
                elif abs(point[1]) < 1.0e-7 and abs(point[2]) < 1.0e-7:
                    new_corner_records.setdefault((material, point), []).append(record)
                material_corner_records[(material, point)] = record

        uv_by_material = {
            BODY: {0: (0.0, 0.0), 1: (2.0, 0.0)},
            NEAR: {0: (10.0, 10.0), 1: (14.0, 10.0)},
        }
        for fraction in fractions:
            point = (2.0 * fraction, 0.0, 0.0)
            for material in face_materials:
                records = new_corner_records[(material, point)]
                expected_uv = tuple(
                    uv_by_material[material][0][axis] * (1.0 - fraction) +
                    uv_by_material[material][1][axis] * fraction
                    for axis in range(2))
                first_normal = before_corners[(material, (0.0, 0.0, 0.0))][1]
                last_normal = before_corners[(material, (2.0, 0.0, 0.0))][1]
                expected_normal = normalized(tuple(
                    first_normal[axis] * (1.0 - fraction) +
                    last_normal[axis] * fraction for axis in range(3)))
                for actual_uv, actual_normal in records:
                    self.assertTrue(close_tuple(actual_uv, expected_uv),
                                    (material, point, actual_uv, expected_uv))
                    self.assertTrue(close_tuple(actual_normal, expected_normal, 2.0e-4),
                                    (material, point, actual_normal, expected_normal))

    def test_one_face_edge_with_one_midpoint(self):
        self.assert_success_case([BODY], [0.5])

    def test_one_face_edge_with_two_ordered_midpoints(self):
        self.assert_success_case([BODY], [0.25, 0.75])

    def test_two_face_edge_with_one_midpoint(self):
        self.assert_success_case([BODY, NEAR], [0.5])

    def test_two_face_edge_with_two_ordered_midpoints(self):
        self.assert_success_case([BODY, NEAR], [0.25, 0.75])

    def test_protected_head_material_is_rejected_without_mutation(self):
        obj = self.make_mesh([HEAD])
        before_vertices, before_corners = self.capture_originals(obj)
        with self.assertRaisesRegex(RuntimeError, 'protected surface'):
            split_segmented_cloth_seams(obj.data, self.plan([0.5], HEAD))
        self.assertEqual(len(obj.data.vertices), len(before_vertices))
        self.assertFalse(BOOKKEEPING & {attribute.name for attribute in obj.data.attributes})
        self.assertEqual(self.capture_originals(obj), (before_vertices, before_corners))

    def test_malformed_fraction_order_is_rejected_without_mutation(self):
        obj = self.make_mesh([BODY])
        before = self.capture_originals(obj)
        with self.assertRaisesRegex(RuntimeError, 'ordered and interior'):
            split_segmented_cloth_seams(obj.data, self.plan([0.75, 0.5]))
        self.assertFalse(BOOKKEEPING & {attribute.name for attribute in obj.data.attributes})
        self.assertEqual(self.capture_originals(obj), before)

    def make_centimetre_planner_pair(self):
        world_positions = [
            (0.0, 0.0, 0.0), (100.0, 0.0, 0.0), (200.0, 0.0, 0.0),
            (0.0, 200.0, 0.0), (200.0, 200.0, 0.0), (100.0, -200.0, 0.0),
        ]
        world_faces = [(0, 1, 3), (1, 2, 4), (2, 0, 5)]
        world_materials = [
            'BodyPrimaryVisible', 'BodyPrimaryVisible', BODY,
        ]
        view_positions = [
            (0.0, 0.0, 0.0), (100.0, 0.00004, 0.0), (200.0, 0.0, 0.0),
            (0.0, 100.0, 20.0), (200.0, 100.0, 20.0),
        ]
        view_faces = [(0, 1, 3), (1, 2, 4)]
        view_materials = ['ViewmodelSleeves', 'ViewmodelSleeves']

        def make_obj(name, positions, faces, materials):
            mesh = bpy.data.meshes.new(name)
            mesh.from_pydata(positions, [], faces)
            for material_name in dict.fromkeys(materials):
                mesh.materials.append(bpy.data.materials.new(material_name))
            material_indices = {material.name: index
                                for index, material in enumerate(mesh.materials)}
            for polygon, material in zip(mesh.polygons, materials):
                polygon.material_index = material_indices[material]
                polygon.use_smooth = True
            uv_layer = mesh.uv_layers.new(name='UVMap')
            for polygon in mesh.polygons:
                for loop_index in polygon.loop_indices:
                    vertex = mesh.loops[loop_index].vertex_index
                    uv_layer.data[loop_index].uv = (
                        float(vertex) * 0.25, float(polygon.index) * 0.5)
            mesh.normals_split_custom_set(
                [(0.0, 0.0, 1.0)] * len(mesh.loops))
            mesh.update()
            obj = bpy.data.objects.new(name, mesh)
            bpy.context.collection.objects.link(obj)
            obj.scale = (0.01, 0.01, 0.01)
            bpy.context.view_layer.update()
            return obj

        return (
            make_obj('centimetre-world', world_positions, world_faces,
                     world_materials),
            make_obj('centimetre-view', view_positions, view_faces,
                     view_materials),
        )

    @staticmethod
    def metric_positions(obj):
        return [tuple(obj.matrix_world @ vertex.co) for vertex in obj.data.vertices]

    @staticmethod
    def planner_faces(obj):
        mesh = obj.data
        mesh.calc_loop_triangles()
        return [
            (mesh.materials[mesh.polygons[triangle.polygon_index].material_index].name,
             tuple(triangle.vertices))
            for triangle in mesh.loop_triangles
        ]

    def test_centimetre_mesh_uses_metric_planning_and_exact_raw_point_copy(self):
        world, view = self.make_centimetre_planner_pair()
        world_mesh, view_mesh = world.data, view.data
        world_faces, view_faces = self.planner_faces(world), self.planner_faces(view)

        raw_plan, raw_stats = plan_segmented_sleeve_seam_splits(
            [tuple(vertex.co) for vertex in world_mesh.vertices], world_faces,
            [tuple(vertex.co) for vertex in view_mesh.vertices], view_faces)
        self.assertEqual(raw_stats['coarseWorldEdges'], 0)

        metric_world = self.metric_positions(world)
        metric_view = self.metric_positions(view)
        plan, stats = plan_segmented_sleeve_seam_splits(
            metric_world, world_faces, metric_view, view_faces)
        self.assertEqual(stats['unmatchedViewBoundaryEdges'], 2)
        self.assertEqual(stats['coveredViewBoundaryEdges'], 2)
        self.assertEqual(stats['coarseWorldEdges'], 1)
        self.assertEqual(stats['insertedWorldVertices'], 1)
        point = plan[0]['points'][0]
        self.assertEqual(point['viewVertex'], 1)
        self.assertLess(abs(point['position'][1]), 2.0e-6)

        canonical_raw_point = tuple(
            view_mesh.vertices[point['viewVertex']].co)
        self.assertGreater(abs(canonical_raw_point[1]), 2.0e-6)
        self.assertLess(abs(canonical_raw_point[1]) * 0.01, 2.0e-6)
        point['position'] = canonical_raw_point
        original_view_uvs = [tuple(layer.data[i].uv)
                             for layer in view_mesh.uv_layers for i in range(len(layer.data))]
        original_world_vertices, original_world_corners = self.capture_originals(world)
        result = split_segmented_cloth_seams(world_mesh, plan)
        self.assertEqual(result['verticesAdded'], 1)
        self.assertEqual(result['trianglesAdded'], 1)
        self.assertTrue(any(tuple(vertex.co) == canonical_raw_point
                            for vertex in world_mesh.vertices))
        self.assertEqual(original_view_uvs,
                         [tuple(layer.data[i].uv) for layer in view_mesh.uv_layers
                          for i in range(len(layer.data))])
        after_world_vertices, after_world_corners = self.capture_originals(world)
        for position, weights in original_world_vertices.items():
            self.assertEqual(after_world_vertices[position], weights)
        for corner, (uv, normal) in original_world_corners.items():
            self.assertEqual(after_world_corners[corner][0], uv)
            self.assertTrue(close_tuple(after_world_corners[corner][1], normal, 1.0e-5))

    def test_disconnected_nonaxis_custom_normals_keep_exact_packed_and_evaluated_values(self):
        obj = self.make_mesh([BODY], untouched=True)
        mesh = obj.data
        custom_normals = mesh.attributes.get('custom_normal')
        self.assertIsNotNone(custom_normals)
        self.assertEqual(custom_normals.domain, 'CORNER')
        self.assertEqual(custom_normals.data_type, 'INT16_2D')
        untouched_face = next(
            polygon for polygon in mesh.polygons
            if mesh.materials[polygon.material_index].name == UNTOUCHED)
        original_by_vertex = {}
        for loop_index in untouched_face.loop_indices:
            vertex_index = mesh.loops[loop_index].vertex_index
            original_by_vertex[vertex_index] = {
                'packed': tuple(custom_normals.data[loop_index].value),
                'normal': tuple(mesh.corner_normals[loop_index].vector),
            }
        self.assertTrue(all(all(abs(value) > 1.0e-3 for value in record['normal'])
                            for record in original_by_vertex.values()))

        split_segmented_cloth_seams(mesh, self.plan([0.5]))
        restored = mesh.attributes.get('custom_normal')
        self.assertIsNotNone(restored)
        untouched_after = next(
            polygon for polygon in mesh.polygons
            if mesh.materials[polygon.material_index].name == UNTOUCHED)
        for loop_index in untouched_after.loop_indices:
            vertex_index = mesh.loops[loop_index].vertex_index
            self.assertEqual(tuple(restored.data[loop_index].value),
                             original_by_vertex[vertex_index]['packed'])
            self.assertEqual(tuple(mesh.corner_normals[loop_index].vector),
                             original_by_vertex[vertex_index]['normal'])


unittest.main(argv=[__file__])
