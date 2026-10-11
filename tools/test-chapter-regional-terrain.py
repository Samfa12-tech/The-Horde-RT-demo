"""Run with the existing Blender or bundled NumPy Python; no asset writes."""
import math
from pathlib import Path
import unittest
import numpy as np
from chapter_regional_terrain import RegionalTerrain, bilinear, gaussian10, ring_coordinates, stitch_sides

ROOT=Path(__file__).resolve().parents[1]


class RegionalTerrainTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        baseline=np.load(ROOT/'assets/source/beyond_the_tomb/terrain/topo-r2/proposed_terrain_heightfield.npz')
        cls.terrain=RegionalTerrain(baseline,ROOT/'docs/references/horde-1.7/d37f1638/docs/design/world/dungeons/baseline/shared-coordinate-layout.json')

    def test_axis_and_interpolation(self):
        h=np.array([[1.,5.],[11.,15.]])
        self.assertAlmostEqual(float(bilinear(np.array([2.,4.]),np.array([10.,12.]),h,np.array(3.),np.array(10.5))),5.5)

    def test_authored_macro_landmarks(self):
        for east,north,height in ((230,-40,30),(306,308,80),(-20,-248,3)):
            self.assertAlmostEqual(float(self.terrain.sample(np.array(float(east)),np.array(float(north)))),height,places=6)
        self.assertAlmostEqual(float(self.terrain.sample(np.array(50.),np.array(-350.))),
                               -7+1.3*math.cos(1)*math.cos(-350/60),places=6)

    def test_extended_land_is_finite_and_not_local_edge_clamping(self):
        points=np.array([-12000.,-1000.,-150.,0.,410.,1000.,12000.])
        result=self.terrain.sample(points,np.full(points.shape,350.))
        self.assertTrue(np.isfinite(result).all())
        # A clamp returns exactly the edge value. There is no requirement for
        # a fabricated one-metre relief change at this arbitrary sample pair.
        self.assertNotEqual(float(result[1]),float(result[2]))
        expected=20+.064*350+5*math.sin(-1000/57)*math.cos(350/76)
        self.assertAlmostEqual(float(result[1]),expected,places=6)
        # No height jump at the reviewed macro boundary.
        west=self.terrain.sample(np.array([-150.000001,-150.]),np.array([100.,100.]))
        self.assertLess(abs(float(west[0]-west[1])),.00001)

    def test_gaussian_constant_and_impulse_symmetry(self):
        self.assertTrue(np.allclose(gaussian10(np.ones((85,85))),1,atol=1e-14))
        impulse=np.zeros((101,101));impulse[50,50]=1
        filtered=gaussian10(impulse)
        self.assertAlmostEqual(float(filtered.sum()),1,places=12)
        self.assertTrue(np.array_equal(filtered,filtered[::-1,:]))
        self.assertTrue(np.array_equal(filtered,filtered[:,::-1]))

    def test_stitched_ring_winding_area_and_closed_inner_seams(self):
        # Deliberately unequal side sample counts and nonuniform inner spacing.
        coords=[[(0.,0.),(.1,0.),(2.,0.)],[(2.,0.),(2.,1.)],
                [(2.,1.),(0.,1.)],[(0.,1.),(0.,0.)]]
        vertices=[];lookup={}
        def indices(sides):
            result=[]
            for side in sides:
                row=[]
                for x,z in side:
                    if (x,z) not in lookup:
                        lookup[(x,z)]=len(vertices);vertices.append((x,-z,0.))
                    row.append(lookup[(x,z)])
                result.append(row)
            return result
        inner=indices(coords);outer=indices(ring_coordinates((-3,5,-2,4),.7))
        triangles=stitch_sides(inner,outer,vertices);area=0;edges={}
        for triangle in triangles:
            p,q,r=(vertices[i] for i in triangle)
            a=((q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0]))/2
            self.assertGreater(a,0);area+=a
            for a,b in zip(triangle,triangle[1:]+triangle[:1]):
                key=tuple(sorted((a,b)));edges.setdefault(key,[]).append((a,b))
        self.assertAlmostEqual(area,8*6-2,places=12)
        boundary={tuple(sorted((a,b))) for ring in (inner,outer) for side in ring for a,b in zip(side,side[1:])}
        for key,directions in edges.items():
            self.assertEqual(len(directions),1 if key in boundary else 2)
            if len(directions)==2:self.assertEqual(directions[0],directions[1][::-1])


if __name__=='__main__':unittest.main()
