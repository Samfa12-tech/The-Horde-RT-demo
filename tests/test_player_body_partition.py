import math
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_body_partition import upper_torso_partition_limits


class BodyPartitionTests(unittest.TestCase):
    def test_retains_the_original_head_mask(self):
        upper, lower = upper_torso_partition_limits(0., 1.8, 1.61908)
        self.assertAlmostEqual(lower, 1.548)
        self.assertEqual(upper, 1.61908)
        for height in (1.422, 1.47, 1.5479):
            self.assertLess(height, lower)  # previously masked torso is retained
        for height in (1.548, 1.60, 1.61908, 1.8):
            # Head and NearFace remain masked, so their union is unchanged.
            self.assertTrue(height >= upper or lower <= height < upper)

    def test_origin_and_scale_are_explicit(self):
        upper, lower = upper_torso_partition_limits(-2., 1.6, 1.23816)
        self.assertAlmostEqual(lower, 1.096)
        self.assertEqual(upper, 1.23816)

    def test_invalid_or_incompatible_landmarks_reject(self):
        for values in ((0,0,0), (1,0,.5), (0,1.8,1.4), (0,1.8,1.8),
                       (math.nan,1.8,1.61908), (0,math.inf,1.61908),
                       (0,1.8,math.nan)):
            with self.subTest(values=values), self.assertRaises(ValueError):
                upper_torso_partition_limits(*values)


if __name__ == '__main__':
    unittest.main()
