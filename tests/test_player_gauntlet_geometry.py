import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_gauntlet_geometry import authored_face_and_uvs, mirror_for_hand


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


if __name__ == '__main__':
    unittest.main()
