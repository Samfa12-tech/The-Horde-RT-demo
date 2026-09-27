import copy
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_sleeve_seams import plan_sleeve_seam_weight_transfers


def weights(**values):
    total = sum(values.values())
    return {bone: value / total for bone, value in values.items()}


def fixture():
    a, b, c, d = (0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, -1.0, 0.0)
    f, g = (3.0, 0.0, 0.0), (3.0, 1.0, 0.0)
    world_positions = [a, b, c, a, b, d, f, g, a, b]
    world_faces = [
        ('BodyPrimaryVisible', (0, 1, 2)),
        ('NearFacePrimaryMasked', (4, 3, 5)),
        ('HeadPrimaryMasked', (8, 2, 5)),
        ('GauntletPrimaryVisible', (9, 6, 7)),
        ('BodyPrimaryVisible', (2, 6, 7)),
    ]
    world_weights = [
        weights(UpperArm=0.7, ForeArm=0.3), weights(ForeArm=1), weights(Spine=1),
        weights(UpperArm=1), weights(Hand=1), weights(Leg=1),
        weights(Leg=1), weights(Leg=1), weights(Neck=1), weights(Hand=1),
    ]
    view_positions = [a, b, (0.0, 0.0, 1.0), a, (0.0, 2.0, 0.0),
                      (1.0, 2.0, 0.0), f, g, (3.0, 0.0, 1.0)]
    view_faces = [
        ('ViewmodelSleeves', (0, 1, 2)),
        ('ViewmodelSleeves', (3, 4, 5)),
        ('ViewmodelSleeves', (6, 7, 8)),
        ('ViewmodelGauntlets', (1, 5, 2)),
    ]
    view_weights = [
        weights(UpperArm=0.25, ForeArm=0.75), weights(ForeArm=0.4, Hand=0.6),
        weights(UpperArm=1), weights(UpperArm=0.25, ForeArm=0.75),
        weights(UpperArm=1), weights(ForeArm=0.4, Hand=0.6),
        weights(Leg=1), weights(Leg=1), weights(Leg=1),
    ]
    return world_positions, world_faces, world_weights, view_positions, view_faces, view_weights


class PlayerSleeveSeamTests(unittest.TestCase):
    def test_transfers_canonical_shared_boundary_weights_without_mutating_inputs(self):
        inputs = fixture()
        before = copy.deepcopy(inputs)
        transfers, stats = plan_sleeve_seam_weight_transfers(*inputs)
        canonical_a = weights(UpperArm=0.25, ForeArm=0.75)
        canonical_b = weights(ForeArm=0.4, Hand=0.6)
        self.assertEqual(transfers, {0: canonical_a, 1: canonical_b,
                                     3: canonical_a, 4: canonical_b})
        self.assertEqual(stats, {
            'viewSleeveBoundaryEdges': 9,
            'matchedWorldSeamEdges': 1,
            'transferredWorldVertices': 4,
        })
        self.assertEqual(inputs, before)

    def test_unshared_and_no_nearface_edges_do_not_transfer(self):
        inputs = fixture()
        _, world_faces, _, _, _, _ = inputs
        world_faces[:] = [face for face in world_faces
                          if face[0] != 'NearFacePrimaryMasked']
        with self.assertRaisesRegex(ValueError, 'no ViewmodelSleeves boundary edge'):
            plan_sleeve_seam_weight_transfers(*inputs)

        inputs = fixture()
        transfers, _ = plan_sleeve_seam_weight_transfers(*inputs)
        self.assertNotIn(6, transfers)
        self.assertNotIn(7, transfers)
        self.assertNotIn(8, transfers)  # coincident head-only vertex
        self.assertNotIn(9, transfers)  # coincident gauntlet-only vertex

    def test_vertices_shared_with_head_or_gauntlet_are_excluded(self):
        inputs = fixture()
        inputs[1].extend([
            ('HeadPrimaryMasked', (0, 2, 5)),
            ('GauntletPrimaryVisible', (1, 6, 7)),
        ])
        transfers, _ = plan_sleeve_seam_weight_transfers(*inputs)
        self.assertNotIn(0, transfers)
        self.assertNotIn(1, transfers)
        self.assertIn(3, transfers)
        self.assertIn(4, transfers)

    def test_conflicting_coincident_view_endpoint_records_fail(self):
        inputs = fixture()
        inputs[5][3] = weights(UpperArm=0.9, ForeArm=0.1)
        with self.assertRaisesRegex(ValueError, 'view sleeve endpoint records conflict'):
            plan_sleeve_seam_weight_transfers(*inputs)

    def test_invalid_face_index_fails(self):
        inputs = fixture()
        inputs[1][0] = ('BodyPrimaryVisible', (0, 1, 999))
        with self.assertRaisesRegex(ValueError, 'invalid vertex index'):
            plan_sleeve_seam_weight_transfers(*inputs)

    def test_nonfinite_position_fails(self):
        inputs = fixture()
        inputs[0][0] = (float('nan'), 0.0, 0.0)
        with self.assertRaisesRegex(ValueError, 'position is not finite'):
            plan_sleeve_seam_weight_transfers(*inputs)

    def test_negative_or_unnormalized_weight_fails(self):
        inputs = fixture()
        inputs[2][0] = {'UpperArm': 1.1, 'ForeArm': -0.1}
        with self.assertRaisesRegex(ValueError, 'negative weight'):
            plan_sleeve_seam_weight_transfers(*inputs)
        inputs = fixture()
        inputs[5][0] = {'UpperArm': 0.6, 'ForeArm': 0.2}
        with self.assertRaisesRegex(ValueError, 'not normalized'):
            plan_sleeve_seam_weight_transfers(*inputs)

    def test_unknown_view_bone_name_fails(self):
        inputs = fixture()
        inputs[5][0] = {'UnmatchedArm': 1.0}
        with self.assertRaisesRegex(ValueError, 'absent from the world rig'):
            plan_sleeve_seam_weight_transfers(*inputs)

    def test_retained_world_authoring_roundoff_does_not_relax_canonical_weights(self):
        inputs = fixture()
        inputs[2][0] = {'UpperArm': 0.999942}
        before = copy.deepcopy(inputs)
        transfers, _ = plan_sleeve_seam_weight_transfers(*inputs)
        self.assertEqual(transfers[0], inputs[5][0])
        self.assertEqual(inputs, before)
        inputs[2][0] = {'UpperArm': 0.999}
        with self.assertRaisesRegex(ValueError, 'world.*not normalized'):
            plan_sleeve_seam_weight_transfers(*inputs)
        inputs = fixture()
        inputs[5][0] = {'UpperArm': 0.999942}
        with self.assertRaisesRegex(ValueError, 'view.*not normalized'):
            plan_sleeve_seam_weight_transfers(*inputs)

    def test_unused_vertices_may_have_no_deform_weights(self):
        inputs = fixture()
        inputs[0].append((20.0, 20.0, 20.0))
        inputs[2].append({})
        inputs[3].append((30.0, 30.0, 30.0))
        inputs[5].append({})
        transfers, _ = plan_sleeve_seam_weight_transfers(*inputs)
        self.assertEqual(set(transfers), {0, 1, 3, 4})

    def test_nonmanifold_and_degenerate_sleeve_edges_fail(self):
        inputs = fixture()
        inputs[4].extend([
            ('ViewmodelSleeves', (0, 1, 2)),
            ('ViewmodelSleeves', (0, 1, 2)),
        ])
        with self.assertRaisesRegex(ValueError, 'nonmanifold edge'):
            plan_sleeve_seam_weight_transfers(*inputs)

        inputs = fixture()
        inputs[3][1] = inputs[3][0]
        with self.assertRaisesRegex(ValueError, 'degenerate keyed edge'):
            plan_sleeve_seam_weight_transfers(*inputs)


if __name__ == '__main__':
    unittest.main()
