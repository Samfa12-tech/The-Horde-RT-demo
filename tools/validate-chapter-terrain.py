"""Independently compare exported GLB triangles to the support surface receipt."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct


def triangle_enters_box(triangle, minimum, maximum):
    polygon=list(triangle)
    for axis in range(3):
        for side,limit in enumerate((minimum[axis],maximum[axis])):
            clipped=[]
            for i,a in enumerate(polygon):
                b=polygon[(i+1)%len(polygon)]
                ai=a[axis]>=limit if side==0 else a[axis]<=limit
                bi=b[axis]>=limit if side==0 else b[axis]<=limit
                if ai: clipped.append(a)
                if ai!=bi:
                    t=(limit-a[axis])/(b[axis]-a[axis])
                    clipped.append(tuple(a[k]+t*(b[k]-a[k]) for k in range(3)))
            polygon=clipped
            if not polygon: return False
    return bool(polygon)


def minimum_height_over_rectangle(triangle, bounds):
    polygon=list(triangle)
    for axis,lo,hi in ((0,bounds[0],bounds[1]),(2,bounds[2],bounds[3])):
        for lower,limit in ((True,lo),(False,hi)):
            clipped=[]
            for i,a in enumerate(polygon):
                b=polygon[(i+1)%len(polygon)]
                ai=a[axis]>=limit if lower else a[axis]<=limit
                bi=b[axis]>=limit if lower else b[axis]<=limit
                if ai:clipped.append(a)
                if ai!=bi:
                    t=(limit-a[axis])/(b[axis]-a[axis])
                    clipped.append(tuple(a[k]+t*(b[k]-a[k]) for k in range(3)))
            polygon=clipped
            if not polygon:return None
    return min(p[1] for p in polygon)


def validate(directory):
    surface=json.loads((directory/'surface.json').read_text())
    glb=directory/(directory.name+'.glb')
    raw=glb.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==surface['sourceGlbSha256'], 'GLB identity changed'
    magic,version,total=struct.unpack_from('<III',raw)
    assert magic==0x46546c67 and version==2 and total==len(raw)
    size,kind=struct.unpack_from('<II',raw,12)
    assert kind==0x4e4f534a
    data=json.loads(raw[20:20+size]); offset=20+size
    size,kind=struct.unpack_from('<II',raw,offset)
    assert kind==0x004e4942
    blob=raw[offset+8:offset+8+size]
    def accessor(index):
        a=data['accessors'][index];v=data['bufferViews'][a['bufferView']]
        fmt={5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']]
        count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
        unpack=struct.Struct('<'+fmt*count)
        start=v.get('byteOffset',0)+a.get('byteOffset',0)
        step=v.get('byteStride',unpack.size)
        return [unpack.unpack_from(blob,start+i*step) for i in range(a['count'])]
    def point(p): return tuple(round(float(x),4) for x in p)
    def triangle(points):
        # Cyclic shifts preserve winding; reversed triangles remain different.
        p=tuple(map(point,points))
        return min(p,p[1:]+p[:1],p[2:]+p[:2])
    x,z,h=surface['x'],surface['z'],surface['heights']
    vertices=surface.get('vertices',[(px,h[j][i],pz) for j,pz in enumerate(z) for i,px in enumerate(x)])
    expected=Counter(triangle([vertices[i] for i in f]) for f in surface['faces'])
    actual=Counter();max_uv_error=0.;min_normal_y=1.;triangles=0
    for node in data['nodes']:
        if 'mesh' not in node: continue
        assert not any(k in node for k in ('matrix','translation','rotation','scale')), 'Unexpected unbaked export transform'
        for prim in data['meshes'][node['mesh']]['primitives']:
            attr=prim['attributes'];p=accessor(attr['POSITION']);n=accessor(attr['NORMAL'])
            repeats=.5 if data['materials'][prim['material']]['name']=='ChapterForestGround' else .42
            uv=accessor(attr['TEXCOORD_0']);t=accessor(attr['TANGENT']);indices=[v[0] for v in accessor(prim['indices'])]
            assert len(p)==len(n)==len(uv)==len(t)
            assert len(indices)%3==0 and max(indices)<len(p)
            for pos,normal,tex,tangent in zip(p,n,uv,t):
                assert all(math.isfinite(v) for v in (*pos,*normal,*tex,*tangent))
                assert abs(sum(v*v for v in normal)-1)<.0002
                assert abs(sum(v*v for v in tangent[:3])-1)<.0002 and abs(tangent[3])==1
                assert abs(sum(a*b for a,b in zip(normal,tangent)))<.0002
                # Blender exports glTF V with its conventional flip.
                if normal[1]>.001:
                    max_uv_error=max(max_uv_error,abs(tex[0]-pos[0]*repeats),abs(tex[1]-(1-pos[2]*repeats)))
                min_normal_y=min(min_normal_y,normal[1])
            for start in range(0,len(indices),3):
                pts=[p[i] for i in indices[start:start+3]]
                a,b,c=pts
                ny=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2])
                if ny<=0:
                    # Non-support faces may only close the far outer shell.
                    assert all(abs(v[1]-surface.get('undersideY',-30))<.001 for v in pts) or any(
                        all(abs(v[axis]-bound)<.001 for v in pts)
                        for axis,bounds in ((0,(x[0],x[-1])),(2,(z[0],z[-1])))
                        for bound in bounds), 'Interior curtain or reversed support face'
                actual[triangle(pts)]+=1;triangles+=1
    assert actual==expected, 'Exported positions/topology/winding differ from support data'
    if 'undersideY' in surface:
        assert surface['undersideY'] < min(min(row) for row in h)-4., 'Underside crosses the low terrain'
    assert max_uv_error<.00003, 'UV0 differs from the declared trail/forest real-world scale'
    assert len(surface['holes'])==3 and len(surface['cells'])==triangles
    for i,j,_ in surface['cells']:
        if i<0:
            assert j==-1, 'Invalid shell cell'
            continue
        px,pz=(x[i]+x[i+1])*.5,(z[j]+z[j+1])*.5
        assert not any(a<px<b and c<pz<d for a,b,c,d in surface['holes']), 'Authored hole covered'
    # Explicitly establish actual land behind the shaft and around the landing,
    # rather than counting a long centreline as terrain coverage.
    cells=set((i,j) for i,j,_ in surface['cells'] if i>=0)
    if 'walkable' in surface:
        assert len(surface['walkable'])==triangles
        for index,(face,walkable) in enumerate(zip(surface['faces'],surface['walkable'])):
            a,b,c=(vertices[n] for n in face)
            ab=[b[k]-a[k] for k in range(3)];ac=[c[k]-a[k] for k in range(3)]
            normal=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
            expected_walkable=normal[1]>0 and math.hypot(normal[0],normal[2])<=surface['maxWalkableSlope']*normal[1]
            assert walkable==expected_walkable, 'Walkability disagrees with exported geometric slope'
    def has_cell(px,pz):
        import bisect
        return (bisect.bisect_right(x,px)-1,bisect.bisect_right(z,pz)-1) in cells
    for px,pz in [(-33.7,-20),(-38,-15.2),(-29,-15.2),(-33.7,-10),(-41,-21),(-27,-21)]:
        assert has_cell(px,pz), 'Missing surrounding ground'
    # Query the exported triangles, including split trail-material faces. A
    # centreline/pad receipt alone cannot prove that the capsule fits the path.
    import bisect
    triangles_by_cell={}
    for index,(i,j,_) in enumerate(surface['cells']):
        if i>=0: triangles_by_cell.setdefault((i,j),[]).append(index)
    def sample(px,pz):
        cell=(bisect.bisect_right(x,px)-1,bisect.bisect_right(z,pz)-1)
        for index in triangles_by_cell.get(cell,[]):
            a,b,c=(vertices[n] for n in surface['faces'][index])
            den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
            if abs(den)<1e-12: continue
            u=((b[2]-c[2])*(px-c[0])+(c[0]-b[0])*(pz-c[2]))/den
            v=((c[2]-a[2])*(px-c[0])+(a[0]-c[0])*(pz-c[2]))/den
            if min(u,v,1-u-v)>=-1e-5:
                return u*a[1]+v*b[1]+(1-u-v)*c[1],surface['walkable'][index]
        return None
    max_route_error=0.; route_samples=0
    for station,(px,pz,py) in enumerate(surface['routeStations']):
        for offset in range(9):
            angle=(offset-1)*math.pi/4
            dx,dz=(0.,0.) if offset==0 else (.24*math.cos(angle),.24*math.sin(angle))
            hit=sample(px+dx,pz+dz)
            assert hit is not None and hit[1], f'Unwalkable route capsule at station {station}, offset {offset}'
            if offset==0: max_route_error=max(max_route_error,abs(hit[0]-py))
            route_samples+=1
    assert max_route_error<=.18, 'Exported trail deviates above the existing maximum step from the graded route'
    # All clipped surface intersections must clear the independently authored
    # roof AABBs. Corner/vertex-only sampling can miss a diagonal intrusion.
    assert minimum_height_over_rectangle([(-2,0,0),(2,0,.01),(2,0,-.01)],(-1,1,-1,1))==0
    assert minimum_height_over_rectangle([(-2,2,0),(-3,2,.01),(-3,2,-.01)],(-1,1,-1,1)) is None
    entry_intersections=0;minimum_entry_cover=math.inf
    for envelope in surface.get('entryEnvelopes',[]):
        hits=0
        for face in surface['faces'][:surface['surfaceFaceCount']]:
            pts=[vertices[n] for n in face]
            if max(p[0] for p in pts)<envelope[0] or min(p[0] for p in pts)>envelope[1] or max(p[2] for p in pts)<envelope[2] or min(p[2] for p in pts)>envelope[3]:continue
            low=minimum_height_over_rectangle(pts,envelope)
            if low is None:continue
            hits+=1;entry_intersections+=1
            minimum_entry_cover=min(minimum_entry_cover,low-envelope[4]+.20)
            assert low>=envelope[4]-.0001, 'Terrain cuts through the enclosed stair roof'
        assert hits>0, 'Entry roof envelope has no terrain coverage'
    # Complete triangle clipping catches thin diagonal intrusions whose three
    # vertices all lie outside a room. Match the native occupied-volume inset.
    assert triangle_enters_box([(-2,0,0),(2,0,.01),(2,0,-.01)],(-1,-1,-1),(1,1,1))
    assert not triangle_enters_box([(-2,2,0),(2,2,.01),(2,2,-.01)],(-1,-1,-1),(1,1,1))
    rooms=[(-1.85,1.85,-6.4,3.4),(-1.2,1.2,-10.,-6.4),(0.,4.8,-11.2,-8.8),
           (3.6,6.,-15.2,-10.),(-2.5,4.8,-16.4,-14.),(-8.5,-2.5,-18.,-12.4),
           (-28.5,-8.5,-16.8,-13.6),(-30.5,-28.5,-16.8,-13.6),(-36.9,-30.5,-18.4,-12.)]
    for face in surface['faces']:
        points=[vertices[n] for n in face]
        if min(p[1] for p in points)>1.345 or max(p[1] for p in points)<-.945: continue
        for xmin,xmax,zmin,zmax in rooms:
            assert not triangle_enters_box(points,(xmin+.005,-.945,zmin+.005),
                                           (xmax-.005,1.345,zmax-.005)), 'Terrain enters occupied tomb volume'
    return {'status':'PASS','triangles':triangles,'supportTriangleMultisetMatchesGlb':True,
            'shaftAndTwoSkylightsOpen':True,'landBehindAndAroundRope':True,
            'maxUvError':max_uv_error,'minimumVertexNormalY':min_normal_y,
            'routeCapsuleSamples':route_samples,'routeCapsuleRadius':.24,
            'maxRouteHeightError':max_route_error,
            'occupiedTombVolumesChecked':9,'thinDiagonalClippingNegativeCases':True,
            'entryRoofTriangleIntersections':entry_intersections,
            'minimumEntryRoofCoverMetres':minimum_entry_cover if entry_intersections else None,
            'glbSha256':surface['sourceGlbSha256'],'visualAcceptance':False}


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory',type=Path)
    args=parser.parse_args();print(json.dumps(validate(args.directory),indent=2))
