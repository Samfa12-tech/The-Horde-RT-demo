import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from player_sleeve_seams import plan_segmented_sleeve_seam_splits


def fixture():
    positions = [(0.,0.,0.), (1.,0.,0.), (0.,1.,0.), (.5,0.,0.), (0.,-1.,0.)]
    world = [('BodyPrimaryVisible',(0,3,2)), ('BodyPrimaryVisible',(3,1,2)),
             ('BodyRemainderPrimaryVisible',(1,0,4))]
    view = [('ViewmodelSleeves',(0,3,2)), ('ViewmodelSleeves',(3,1,2))]
    return positions,world,positions[:4],view


class SegmentedSeamTests(unittest.TestCase):
    def test_plans_only_canonical_midpoint_without_mutating_inputs(self):
        inputs=fixture()
        before=copy.deepcopy(inputs)
        plan,stats=plan_segmented_sleeve_seam_splits(*inputs)
        self.assertEqual(inputs,before)
        self.assertEqual(stats,dict(unmatchedViewBoundaryEdges=4,
            coveredViewBoundaryEdges=2,coarseWorldEdges=1,insertedWorldVertices=1))
        self.assertEqual(plan,[dict(worldEdge=(1,0),worldFace=2,
            material='BodyRemainderPrimaryVisible',points=[dict(
                fraction=.5,viewVertex=3,position=(.5,0.,0.))])])

    def test_already_matching_segments_need_no_split(self):
        positions,world,vp,vf=fixture()
        world[-1:]=[('BodyRemainderPrimaryVisible',(1,3,4)),
                    ('BodyRemainderPrimaryVisible',(3,0,4))]
        plan,stats=plan_segmented_sleeve_seam_splits(positions,world,vp,vf)
        self.assertEqual(plan,[])
        self.assertEqual(stats['coarseWorldEdges'],0)

    def test_nearby_but_not_coincident_edge_is_not_welded(self):
        positions,world,vp,vf=fixture()
        positions.extend([(0.,0.,.001),(1.,0.,.001)])
        world[-1]=('BodyRemainderPrimaryVisible',(6,5,4))
        plan,_=plan_segmented_sleeve_seam_splits(positions,world,vp,vf)
        self.assertEqual(plan,[])

    def test_partial_cloth_coverage_rejects(self):
        positions,world,vp,vf=fixture()
        with self.assertRaisesRegex(ValueError,'partially covers'):
            plan_segmented_sleeve_seam_splits(positions,world,vp,vf[:1])

    def test_same_winding_rejects(self):
        positions,world,vp,vf=fixture()
        world[-1]=('BodyRemainderPrimaryVisible',(0,1,4))
        with self.assertRaisesRegex(ValueError,'inconsistent winding'):
            plan_segmented_sleeve_seam_splits(positions,world,vp,vf)

    def test_protected_vertex_rejects(self):
        positions,world,vp,vf=fixture()
        world.append(('HeadPrimaryMasked',(0,2,4)))
        with self.assertRaisesRegex(ValueError,'protected head or gauntlet'):
            plan_segmented_sleeve_seam_splits(positions,world,vp,vf)

    def test_no_body_primary_backing_does_not_authorize_a_split(self):
        positions,world,vp,vf=fixture()
        plan,_=plan_segmented_sleeve_seam_splits(positions,world[2:],vp,vf)
        self.assertEqual(plan,[])


if __name__=='__main__':
    unittest.main()
