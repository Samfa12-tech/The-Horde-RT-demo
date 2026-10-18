"""Source-only production r21 regression; no Blender export or asset mutation."""
from collections import Counter
import json
from pathlib import Path
import unittest
import numpy as np
from chapter_regional_terrain import RegionalTerrain
from chapter_water_conformance import area,conform_water,cross,triangulate,water_footprints

ROOT=Path(__file__).resolve().parents[1]


def highest(vertices,faces,points):
    triangles=np.asarray(vertices)[np.asarray(faces)]
    a,b,c=triangles[:,0],triangles[:,1],triangles[:,2]
    denominator=(b[:,2]-c[:,2])*(a[:,0]-c[:,0])+(c[:,0]-b[:,0])*(a[:,2]-c[:,2])
    low=triangles[:,:,[0,2]].min(axis=1);high=triangles[:,:,[0,2]].max(axis=1)
    result=[]
    for x,z in points:
        ids=np.flatnonzero((low[:,0]<=x+1e-7)&(high[:,0]>=x-1e-7)&
            (low[:,1]<=z+1e-7)&(high[:,1]>=z-1e-7)&(np.abs(denominator)>1e-10))
        u=((b[ids,2]-c[ids,2])*(x-c[ids,0])+(c[ids,0]-b[ids,0])*(z-c[ids,2]))/denominator[ids]
        v=((c[ids,2]-a[ids,2])*(x-c[ids,0])+(a[ids,0]-c[ids,0])*(z-c[ids,2]))/denominator[ids]
        valid=(u>=-1e-6)&(v>=-1e-6)&(u+v<=1.000001)
        heights=u*a[ids,1]+v*b[ids,1]+(1-u-v)*c[ids,1]
        result.append(float(np.max(heights[valid])) if np.any(valid) else float('nan'))
    return np.asarray(result)


class WaterConformanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.old=json.loads((ROOT/'assets/models/world/source/chapter-terrain-r21/surface.json').read_text())
        cls.regional=RegionalTerrain(np.load(ROOT/'assets/source/beyond_the_tomb/terrain/topo-r2/proposed_terrain_heightfield.npz'),
            ROOT/'docs/references/horde-1.7/d37f1638/docs/design/world/dungeons/baseline/shared-coordinate-layout.json')
        cls.outer=[c==[-2,-2,-2] for c in cls.old['cells']]
        cls.vertices,cls.faces,cls.sources,cls.receipt=conform_water(cls.old['vertices'],cls.old['faces'],cls.outer,cls.regional)
        print('source-only water conformance:',json.dumps(cls.receipt),flush=True)

    def test_original_vertices_protected_core_and_envelope_unchanged(self):
        self.assertEqual(self.vertices[:len(self.old['vertices'])],[tuple(v) for v in self.old['vertices']])
        protected=[tuple(f) for f,outer in zip(self.old['faces'],self.outer) if not outer]
        self.assertEqual([f for f,s in zip(self.faces,self.sources) if not self.outer[s]],protected)
        self.assertLessEqual(len(self.faces),39618)
        self.assertEqual(np.min(self.vertices,axis=0).tolist(),np.min(self.old['vertices'],axis=0).tolist())
        self.assertEqual(np.max(self.vertices,axis=0).tolist(),np.max(self.old['vertices'],axis=0).tolist())

    def test_sampled_heights_are_unchanged_authoritative_heightfield(self):
        inserted=np.asarray(self.vertices[len(self.old['vertices']):])
        expected=self.regional.sample(inserted[:,0]-78.3,126.2-inserted[:,2])-31.55
        np.testing.assert_array_equal(inserted[:,1],expected)

    def test_float32_export_positions_keep_nonzero_triangles(self):
        exported=np.asarray(self.vertices,dtype=np.float32).astype(float)
        t=exported[np.asarray(self.faces)]
        normals=np.cross(t[:,1]-t[:,0],t[:,2]-t[:,0])
        self.assertTrue(np.all(np.linalg.norm(normals,axis=1)>1e-10))
        for normal,source in zip(normals,self.sources):
            if self.outer[source]:self.assertGreater(normal[1],0)

    def test_winding_area_and_no_new_open_seams(self):
        def edges(faces):return Counter(tuple(sorted((a,b))) for f in faces for a,b in zip(f,f[1:]+f[:1]))
        before,after=edges(self.old['faces']),edges(self.faces)
        self.assertEqual({e:n for e,n in after.items() if n!=2},{e:n for e,n in before.items() if n!=2})
        old_areas=[abs(area([(self.old['vertices'][i][0],self.old['vertices'][i][2]) for i in f])) for f in self.old['faces']]
        actual=np.zeros(len(old_areas))
        for face,source in zip(self.faces,self.sources):
            if not self.outer[source]:continue
            a,b,c=[(self.vertices[i][0],self.vertices[i][2]) for i in face]
            self.assertLess(cross(a,b,c),0)
            actual[source]+=abs(cross(a,b,c))*.5
        np.testing.assert_allclose(actual[self.outer],np.asarray(old_areas)[self.outer],rtol=1e-8,atol=1e-7)

    def test_real_river_failures_and_full_width_samples(self):
        # Dense independent orthogonal-width probes, not merely the original
        # diagnostic centreline midpoints.
        points=[];water=[]
        for river in self.regional.river_water:
            source=np.asarray(river,dtype=float)
            for a,b in zip(source,source[1:]):
                normal=np.array([-(b[1]-a[1]),b[0]-a[0]])/np.linalg.norm(b[:2]-a[:2])
                for t in np.linspace(.03,.97,max(3,int(np.linalg.norm(b[:2]-a[:2])/3))):
                    centre=a+(b-a)*t
                    for width in (-2.9,-1.5,0,1.5,2.9):
                        p=centre[:2]+normal*width;points.append((p[0]+78.3,126.2-p[1]));water.append(centre[2]-31.55)
        ground=highest(self.vertices,self.faces,points)
        self.assertTrue(np.isfinite(ground).all())
        maximum=float(np.max(ground-np.asarray(water)))
        print('maximum river terrain minus exact water:',maximum,flush=True)
        self.assertLessEqual(maximum,-.45)

    def test_actual_mitered_water_faces_including_edges_and_bends(self):
        points=[];water=[]
        for river in self.regional.river_water:
            atlas=np.asarray(river,dtype=float)
            centres=np.column_stack((atlas[:,0]+78.3,atlas[:,2]-31.55,126.2-atlas[:,1]))
            directions=np.diff(centres[:,[0,2]],axis=0)
            normals=np.column_stack((-directions[:,1],directions[:,0]))
            normals/=np.linalg.norm(normals,axis=1)[:,None]
            left=[];right=[]
            for i,c in enumerate(centres):
                before=normals[max(0,i-1)];after=normals[min(i,len(normals)-1)]
                m=before+after;m*=3/np.dot(m,before)
                left.append(c+np.array([m[0],0,m[1]]));right.append(c-np.array([m[0],0,m[1]]))
            for i in range(len(centres)-1):
                for a,b,c in ((left[i],right[i+1],right[i]),(left[i],left[i+1],right[i+1])):
                    for u in np.linspace(0,1,9):
                        for v in np.linspace(0,1-u,max(2,int((1-u)*8)+1)):
                            p=a*u+b*v+c*(1-u-v);points.append((p[0],p[2]));water.append(p[1])
        ground=highest(self.vertices,self.faces,points)
        self.assertTrue(np.isfinite(ground).all())
        maximum=float(np.max(ground-water))
        print('maximum actual river face terrain minus water:',maximum,'samples',len(points),flush=True)
        self.assertLessEqual(maximum,-.45)

    def test_concave_lake_interior_not_bridged(self):
        polygon=water_footprints(self.regional)[0]
        points=[]
        for triangle in triangulate(polygon):
            a,b,c=map(np.asarray,triangle)
            for u,v in ((.2,.2),(.6,.2),(.2,.6),(1/3,1/3)):
                points.append(tuple(a*u+b*v+c*(1-u-v)))
        ground=highest(self.vertices,self.faces,points)
        self.assertTrue(np.isfinite(ground).all())
        self.assertLess(float(ground.max()),-32.05)

    def test_lake_inner_shore_samples(self):
        polygon=water_footprints(self.regional)[0]
        points=[]
        for a,b in zip(polygon,polygon[1:]+polygon[:1]):
            a,b=np.asarray(a),np.asarray(b);d=b-a
            normal=np.array([-d[1],d[0]])/np.linalg.norm(d)
            for t in np.linspace(.1,.9,max(3,int(np.linalg.norm(d)/4))):
                points.append(tuple(a+(b-a)*t+normal*2))
        ground=highest(self.vertices,self.faces,points)
        worst=int(np.argmax(ground));p=points[worst]
        authored=float(self.regional.sample(np.array(p[0]-78.3),np.array(126.2-p[1]))-31.55)
        print('maximum lake inner-shore terrain minus datum:',float(ground.max()+31.55),
              'at engineXZ',p,'authoritative',authored+31.55,flush=True)
        self.assertTrue(np.isfinite(ground).all())
        # The unchanged authored river-mouth/shore blend itself reaches only
        # .181m below datum here. Require visible nonburial, not a fabricated
        # half-metre-deep shoreline contradicting the source heightfield.
        self.assertLess(float(ground.max()),-31.55)

    def test_fail_before_publish_at_budget(self):
        with self.assertRaisesRegex(ValueError,'budget'):
            conform_water(self.old['vertices'],self.old['faces'],self.outer,self.regional,maximum_faces=len(self.old['faces']))


if __name__=='__main__':unittest.main()
