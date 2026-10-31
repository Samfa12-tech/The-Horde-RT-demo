"""Offline whole-route authoring contracts. No Vulkan/device/application launch."""
import copy
import importlib.util
import json
import math
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
HERE=ROOT/'assets/models/world/source/forest-surroundings-v1'
spec=importlib.util.spec_from_file_location('surroundings',HERE/'author.py')
author=importlib.util.module_from_spec(spec);spec.loader.exec_module(author)


def inside(p,polygon):
    hit=False
    for a,b in zip(polygon,polygon[1:]+polygon[:1]):
        if (a[1]>p[1])!=(b[1]>p[1]) and p[0]<(b[0]-a[0])*(p[1]-a[1])/(b[1]-a[1])+a[0]:hit=not hit
    return hit


def validate(layout):
    original=json.loads(author.PLACEMENTS.read_text())['placements']
    rows=layout['placements'];edges=layout['boundary'];route=layout['route']
    if layout['counts']!={'near-original':64,'middle':96,'far':320} or len(rows)!=480:raise ValueError('Population')
    if any(layout['admission'].values()):raise ValueError('Source package cannot admit gameplay')
    if len(set(v['id'] for v in rows))!=480 or len(set((v['x'],v['z']) for v in rows))!=480:raise ValueError('Duplicate root')
    for i,p in enumerate(original):
        if any(rows[i].get(k)!=v for k,v in p.items()) or rows[i]['sourceIndex']!=i:raise ValueError('Original roster changed')
    def clearance(p):return min([author.project(p,route)[0]-layout['corridorHalfWidth']]+[math.dist(p,q['center'])-q['radius'] for q in layout['pockets']+layout['originalTrunkClearancePockets']])
    if layout['clearTrailWidth']!=2.4:raise ValueError('Clear path changed')
    poly=[(v['a'][0],v['a'][2]) for v in edges]
    if not 50<=len(poly)<=250:raise ValueError('Unbounded boundary')
    for i,edge in enumerate(edges):
        if edge['b']!=edges[(i+1)%len(edges)]['a']:raise ValueError('Boundary gap')
        a,b=poly[i],poly[(i+1)%len(poly)]
        if not 0<math.dist(a,b)<=2.40001:raise ValueError('Panel span')
        if edge['default']!='visible-fence' or edge['collisionAdmitted']:raise ValueError('Unproved bank gap')
        for t in range(11):
            if not -.00001<=clearance(author.add(a,author.mul(author.sub(b,a),t/10)))<=1.50001:raise ValueError('Protected pocket clipped/detail allowance exceeded')
    for a,b in zip(route,route[1:]):
        for t in range(21):
            if not inside(author.add(a,author.mul(author.sub(b,a),t/20)),poly):raise ValueError('Route outside boundary')
    for point in [(-36.8,-12.8),(-37,-16),(-34.8,-19.1),(-32.5,-18.9),(-33.7,-20.8)]:
        if not inside(point,poly):raise ValueError('Grave/shaft/rope access clipped')
    for row in rows[64:]:
        p=(row['x'],row['z']);c=clearance(p);lo,hi=(12,36) if row['band']=='middle' else (38,115)
        bounds=json.loads((HERE/'asset-receipt.json').read_text())['assets'][row['asset']]['bounds']
        radius=max(math.hypot(x,z) for x in (bounds[0][0],bounds[1][0]) for z in (bounds[0][2],bounds[1][2]))
        if not lo-.00001<=c-radius-1.5 or c>hi+.00001 or inside(p,poly):raise ValueError('Detail/reachability band')
        if abs(row['nearestPlayableClearance']-(c-radius-1.5))>.00001:raise ValueError('Geometry clearance receipt')
        if not 0<=row['fittedBurial']<=.65 or row['supportSamples']<3:raise ValueError('Ground contact')
        keep=layout['townKeepout']
        if all(keep['min'][k]<p[k]<keep['max'][k] for k in (0,1)):raise ValueError('Town reserve invaded')
        for window in layout['windows']:
            dist,_,t,_=author.project(p,[window['from'],window['to']])
            if .02<t<1 and dist<window['halfWidth']:raise ValueError('Landmark window invaded')


class SourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout=json.loads((HERE/'composition.json').read_text())
        cls.receipt=json.loads((HERE/'asset-receipt.json').read_text())

    def test_real_composition_constraints(self):validate(self.layout)

    def test_exact_reproduction_and_input_identity(self):
        for name,data in author.produce().items():self.assertEqual((HERE/name).read_bytes(),data,name)
        self.assertEqual(self.layout['terrainSha256'],author.sha(author.TERRAIN.read_bytes()))
        self.assertEqual(self.layout['mapSha256'],author.sha(author.MAP.read_bytes()))

    def test_near_assets_and_all_texture_bytes_preserved(self):
        for species,(name,digest) in author.SOURCES.items():
            raw=(author.TREE/(name+'.glb')).read_bytes();self.assertEqual(author.sha(raw),digest)
            j,b=author.unpack(raw)
            images=[author.sha(b[j['bufferViews'][i['bufferView']].get('byteOffset',0):j['bufferViews'][i['bufferView']].get('byteOffset',0)+j['bufferViews'][i['bufferView']]['byteLength']]) for i in j['images']]
            for band in ('middle','far'):self.assertEqual(self.receipt['assets'][f'{species}-{band}.glb']['embeddedImageSha256'],images)

    def test_actual_geometry_closed_attributes_and_cost(self):
        for name,row in self.receipt['assets'].items():
            self.assertEqual(author.inspect((HERE/'candidates'/name).read_bytes()),row)
            limit=100 if name=='boundary-panel.glb' else 1500 if 'middle' in name else 250
            self.assertLessEqual(row['triangles'],limit)
            self.assertLessEqual(row['vertices'],2*row['triangles'])
        extra=sum(self.receipt['assets'][r['asset']]['triangles'] for r in self.layout['placements'][64:])
        self.assertEqual(self.layout['candidateTreeTriangles'],323264+extra)
        self.assertLessEqual(self.layout['candidateTreeTriangles'],547264)
        self.assertLessEqual(extra*140,31360000)

    def test_both_route_sides_have_grouped_middle_and_far_stands(self):
        for band,n in [('middle',8),('far',15)]:
            rows=[r for r in self.layout['placements'] if r['band']==band]
            self.assertEqual(set(r['stand'] for r in rows),set(range(n)))
            for i in range(n):self.assertGreaterEqual(sum(r['stand']==i for r in rows),12)

    def test_boundary_has_no_proper_self_crossings(self):
        points=[(v['a'][0],v['a'][2]) for v in self.layout['boundary']]
        def orient(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
        for i,a in enumerate(points):
            b=points[(i+1)%len(points)]
            for k in range(i+2,len(points)):
                if i==0 and k==len(points)-1:continue
                c,d=points[k],points[(k+1)%len(points)]
                self.assertFalse(orient(a,b,c)*orient(a,b,d)<-1e-10 and orient(c,d,a)*orient(c,d,b)<-1e-10,(i,k))

    def test_existing_rock_references_preserve_native_authority(self):
        rocks=self.layout['existingRockReferences']
        self.assertEqual([r['nativePlacementIndex'] for r in rocks],list(range(12)))
        self.assertTrue(all(r['referenceOnly'] for r in rocks))
        self.assertEqual(self.layout['existingRockSourceSha256'],author.sha((ROOT/'src/scene/ForestRockDressing.cpp').read_bytes()))
        self.assertTrue(all(r['nearestBoundaryRun'] in {e['id'] for e in self.layout['boundary']} for r in rocks))

    def test_duplicate_changed_original_and_fake_admission_rejected(self):
        d=copy.deepcopy(self.layout);d['placements'][-1].update(x=d['placements'][0]['x'],z=d['placements'][0]['z'])
        with self.assertRaisesRegex(ValueError,'Duplicate'):validate(d)
        d=copy.deepcopy(self.layout);d['placements'][3]['yawRadians']+=.1
        with self.assertRaisesRegex(ValueError,'Original'):validate(d)
        d=copy.deepcopy(self.layout);d['admission']['normalDefault']=True
        with self.assertRaisesRegex(ValueError,'admit'):validate(d)

    def test_open_boundary_and_missing_collision_exemption_proof_rejected(self):
        d=copy.deepcopy(self.layout);d['boundary'].pop()
        with self.assertRaisesRegex(ValueError,'gap'):validate(d)
        d=copy.deepcopy(self.layout);d['boundary'][0]['default']='unproved-terrain'
        with self.assertRaisesRegex(ValueError,'Unproved'):validate(d)
        d=copy.deepcopy(self.layout);d['placements'][64].update(x=-33.7,z=-12.8)
        with self.assertRaises(ValueError):validate(d)


if __name__=='__main__':unittest.main()
