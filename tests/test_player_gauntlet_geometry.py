import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_gauntlet_geometry import authored_face_and_uvs, mirror_for_hand, fit_cuff_to_forearm


class GauntletGeometryTests(unittest.TestCase):
    def test_right_source_is_mirrored_only_for_left_hand(self):
        self.assertTrue(mirror_for_hand('Right', 'Left'))
        self.assertFalse(mirror_for_hand('Right', 'Right'))

    def test_mirror_preserves_vertex_to_uv_correspondence(self):
        face = (7, 11, 13, 17)
        uvs = ((0, 0), (1, 0), (1, 1), (0, 1))
        mirrored, mapped = authored_face_and_uvs(face, uvs, mirror=True)
        self.assertEqual(mirrored, tuple(reversed(face)))
        self.assertEqual(dict(zip(mirrored, mapped)), dict(zip(face, uvs)))

    def test_unmirrored_source_preserves_all_corners(self):
        face, uvs = (3, 5, 9), ((.1, .2), (.3, .4), (.5, .6))
        self.assertEqual(authored_face_and_uvs(face, uvs, mirror=False), (face, uvs))

    def test_legacy_uv_mode_is_explicit_not_default(self):
        face, uvs = (3, 5, 9), ((.1, .2), (.3, .4), (.5, .6))
        legacy = authored_face_and_uvs(face, uvs, mirror=True, legacy_uv_order=True)
        self.assertEqual(legacy, (tuple(reversed(face)), uvs))
        self.assertNotEqual(legacy, authored_face_and_uvs(face, uvs, mirror=True))

    def test_invalid_chirality_is_rejected(self):
        for source, target in (('right', 'Left'), ('Right', ''), ('Unknown', 'Left')):
            with self.assertRaises(ValueError):
                mirror_for_hand(source, target)

    def test_mismatched_corner_roster_is_rejected(self):
        with self.assertRaises(ValueError):
            authored_face_and_uvs((0, 1, 2), ((0, 0), (1, 1)), mirror=True)


class CuffFitTests(unittest.TestCase):
    axes = ((1, 0, 0), (0, 1, 0), (0, 0, 1))
    wrist, elbow = (0, 0, 0), (.25, 0, 0)
    points = ((-.08, .02, 0), (0, .02, 0), (.03, .05, .02),
              (.06, .02, -.02), (.06, .08, .02))

    def fit(self, points=None, **kwargs):
        return fit_cuff_to_forearm(points or self.points, self.wrist, self.elbow,
                                  kwargs.get('axes', self.axes))

    def test_distal_hand_and_grip_are_exactly_unchanged(self):
        result, shares, report = self.fit()
        self.assertEqual(result[:2], list(self.points[:2]))
        self.assertEqual(shares[:2], [0, 0])
        self.assertEqual(report['distalHandVerticesPreserved'], 2)

    def test_terminal_centre_fits_forearm_without_changing_section_shape(self):
        result, shares, report = self.fit()
        self.assertEqual(shares[-2:], [1, 1])
        centre = tuple((result[-1][i] + result[-2][i]) * .5 for i in range(3))
        for actual, expected in zip(centre, (.06, 0, 0)):
            self.assertAlmostEqual(actual, expected, places=12)
        for i in range(3):
            self.assertAlmostEqual(result[-1][i] - result[-2][i],
                                   self.points[-1][i] - self.points[-2][i], places=12)
        self.assertAlmostEqual(report['radialCorrectionMetres'], .05)

    def test_smooth_normalized_hand_forearm_share(self):
        _, shares, _ = self.fit()
        self.assertAlmostEqual(shares[2], .5)
        self.assertTrue(all(0 <= s <= 1 for s in shares))
        self.assertTrue(all((1-s) + s == 1 for s in shares))

    def test_fit_is_translation_equivariant_not_camera_relative(self):
        delta = (.3, 1.2, -.6)
        move = lambda p: tuple(p[i] + delta[i] for i in range(3))
        a, shares_a, _ = self.fit()
        b, shares_b, _ = fit_cuff_to_forearm(list(map(move, self.points)), move(self.wrist),
                                            move(self.elbow), self.axes)
        for original, translated in zip(a, b):
            for expected, actual in zip(move(original), translated):
                self.assertAlmostEqual(expected, actual)
        for expected, actual in zip(shares_a, shares_b):
            self.assertAlmostEqual(expected, actual)

    def test_malformed_and_wrong_way_frames_are_rejected(self):
        for frame in (((2, 0, 0), (0, 1, 0), (0, 0, 1)),
                      ((-1, 0, 0), (0, -1, 0), (0, 0, 1))):
            with self.assertRaises(ValueError):
                self.fit(axes=frame)
        with self.assertRaises(ValueError):
            self.fit(points=((float('nan'), 0, 0),))

    def test_missing_or_excessively_offset_cuff_rejects(self):
        with self.assertRaises(ValueError):
            self.fit(points=((-1, 0, 0), (0, 0, 0)))
        with self.assertRaises(ValueError):
            self.fit(points=((-1, 0, 0), (.06, .12, 0)))

    def test_source_contact_mask_cannot_be_moved_out_of_protected_region(self):
        with self.assertRaisesRegex(ValueError, 'protected.*grip contact'):
            fit_cuff_to_forearm(self.points, self.wrist, self.elbow, self.axes,
                                protected_indices=(2,))
        result, shares, _ = fit_cuff_to_forearm(self.points, self.wrist, self.elbow,
                                               self.axes, protected_indices=(0, 1))
        self.assertEqual(result[:2], list(self.points[:2]))
        self.assertEqual(shares[:2], [0, 0])

    def test_invalid_contact_roster_and_vector_dimensions_reject(self):
        with self.assertRaises(ValueError):
            fit_cuff_to_forearm(self.points, self.wrist, self.elbow, self.axes,
                                protected_indices=(99,))
        with self.assertRaises(ValueError):
            self.fit(points=((0, 0),))


if __name__ == '__main__':
    unittest.main()
