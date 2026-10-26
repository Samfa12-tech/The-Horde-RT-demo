"""Owned branch-supported pine study; originals and production stay unchanged.
Run with installed Blender/numpy: -b --threads 2 --python <this> -- --output reports/<fresh>/candidate/assets
Retains original trunk, roots, lower stubs, ten primary boughs and moss. Replaces
forty sparse twigs with twenty additional boughs/eighty twigs and attached sprays.
No glTF exporter, external assets, shader path or intake-limit change.
"""
from pathlib import Path
import argparse, hashlib, json, math, random, struct, sys
import numpy as np
ROOT=Path(__file__).resolve().parents[5]
SOURCE=ROOT/'assets/models/world/runtime/forest-tree-pair-v1'

def sha(b):return hashlib.sha256(b).hexdigest()
def unit(v):return v/np.linalg.norm(v)
def unpack(path):
    b=path.read_bytes();assert struct.unpack_from('<III',b)==(0x46546c67,2,len(b))
    n,t=struct.unpack_from('<II',b,12);assert t==0x4e4f534a
    j=json.loads(b[20:20+n]);size,kind=struct.unpack_from('<II',b,20+n);assert kind==0x004e4942
    data=b[28+n:28+n+size]
    return b,j,[data[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']] for v in j['bufferViews']]
def array(j,views,ai):
    a=j['accessors'][ai];v=j['bufferViews'][a['bufferView']]
    assert not a.get('byteOffset',0) and 'byteStride' not in v and 'sparse' not in a
    width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
    return np.frombuffer(views[a['bufferView']],dtype={5126:'<f4',5123:'<u2'}[a['componentType']]).reshape(a['count'],width).copy()
def mesh_arrays(j,views,p):
    return {k:array(j,views,ai) for k,ai in p['attributes'].items()},array(j,views,p['indices']).reshape(-1,3)
def components(pos,indices):
    parent=list(range(len(pos)))
    def find(x):
        while parent[x]!=x:parent[x]=parent[parent[x]];x=parent[x]
        return x
    def join(a,b):parent[find(a)]=find(b)
    welded={}
    for i,p in enumerate(pos):
        key=tuple(p)
        if key in welded:join(i,welded[key])
        else:welded[key]=i
    for a,b,c in indices:join(int(a),int(b));join(int(b),int(c))
    groups={}
    for ti,face in enumerate(indices):groups.setdefault(find(int(face[0])),[]).append(ti)
    return sorted(groups.values(),key=min)
def frame(axis):
    axis=unit(axis);ref=np.array([0.,1.,0.]) if abs(axis[1])<.94 else np.array([1.,0.,0.])
    a=unit(np.cross(axis,ref));return a,np.cross(axis,a)
def path_point(points,t):
    lengths=np.linalg.norm(np.diff(points,axis=0),axis=1);d=t*lengths.sum()
    for a,b,length in zip(points[:-1],points[1:],lengths):
        if d<=length:return a+(b-a)*d/length
        d-=length
    return points[-1].copy()
def closed_tube(points,radii,sides):
    positions=[];faces=[]
    for i,(point,radius) in enumerate(zip(points,radii)):
        direction=unit(points[min(i+1,len(points)-1)]-points[max(0,i-1)]);a,b=frame(direction)
        positions.extend([point+radius*(a*math.cos(j*2*math.pi/sides)+b*math.sin(j*2*math.pi/sides)) for j in range(sides)])
    for ring in range(len(points)-1):
        for a in range(sides):
            b=(a+1)%sides;i=ring*sides;j=(ring+1)*sides
            faces.extend([(i+a,i+b,j+b),(i+a,j+b,j+a)])
    for i in range(1,sides-1):faces.append((0,i+1,i))
    top=(len(points)-1)*sides
    for i in range(1,sides-1):faces.append((top,top+i,top+i+1))
    p=np.asarray(positions,float);f=np.asarray(faces,np.uint16);tri=p[f]
    face_n=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);area=np.linalg.norm(face_n,axis=1)
    assert area.min()>1e-10
    # Real-world planar bark mapping, no degenerate cap UVs or seam duplication.
    candidates=[]
    for vector in [np.array([.29,.61,.74]),np.array([.71,-.37,.59]),np.array([-.57,.72,.31])]:
        u,v=frame(unit(vector));uv=np.column_stack((p@u,p@v))/1.7
        d1=uv[f[:,1]]-uv[f[:,0]];d2=uv[f[:,2]]-uv[f[:,0]];det=d1[:,0]*d2[:,1]-d1[:,1]*d2[:,0]
        candidates.append((float(np.abs(det).min()),uv,det,d1,d2))
    quality,uv,det,d1,d2=max(candidates,key=lambda x:x[0]);assert quality>1e-12
    dp1=tri[:,1]-tri[:,0];dp2=tri[:,2]-tri[:,0]
    face_t=(dp1*d2[:,1,None]-dp2*d1[:,1,None])/det[:,None]
    face_b=(-dp1*d2[:,0,None]+dp2*d1[:,0,None])/det[:,None]
    normal=np.zeros_like(p);tangent=np.zeros_like(p);bitangent=np.zeros_like(p)
    for face,n,t,b,w in zip(f,face_n,face_t,face_b,area):
        for vertex in face:normal[vertex]+=n;tangent[vertex]+=t*w;bitangent[vertex]+=b*w
    normal/=np.linalg.norm(normal,axis=1)[:,None];tangent-=normal*np.sum(normal*tangent,axis=1)[:,None];tangent/=np.linalg.norm(tangent,axis=1)[:,None]
    handed=np.where(np.sum(np.cross(normal,tangent)*bitangent,axis=1)<0,-1.,1.)
    attrs={'POSITION':p,'NORMAL':normal,'TEXCOORD_0':uv,'TANGENT':np.column_stack((tangent,handed))}
    edges={}
    for face in f:
        for a,b in zip(face,np.roll(face,-1)):edges[(int(a),int(b))]=edges.get((int(a),int(b)),0)+1
    assert all(n==1 and edges.get((b,a))==1 for (a,b),n in edges.items())
    volume=np.einsum('ij,ij->i',tri[:,0],np.cross(tri[:,1],tri[:,2])).sum()/6
    assert volume>0
    return attrs,f
def append_mesh(destination,faces,attrs,indices):
    base=len(destination['POSITION'])
    for name in destination:destination[name].extend(attrs[name].tolist())
    faces.extend((indices+base).tolist())
def digest(attrs,faces):
    h=hashlib.sha256()
    for name in ['POSITION','NORMAL','TEXCOORD_0','TANGENT']:h.update(np.asarray(attrs[name],dtype='<f4')[faces].tobytes())
    return h.hexdigest()
def build_pine(out):
    source=SOURCE/'horde-irregular-pine-v1-lod1.glb';original,j,views=unpack(source);unchanged=views[:]
    wood=next(m for m in j['meshes'] if 'closed trunk roots branches' in m['name'])['primitives'][0]
    old,oldfaces=mesh_arrays(j,views,wood);groups=components(old['POSITION'],oldfaces);assert len(groups)==59
    # Original ordered source: trunk, six roots, ten (bough+four twig) groups,
    # then two bare lower stubs. Preserve original primary moss attachments.
    kept=list(range(7))+[7+i*5 for i in range(10)]+[57,58]
    face_ids=sorted(t for i in kept for t in groups[i]);faces=oldfaces[face_ids]
    used=np.unique(faces);remap=np.zeros(len(old['POSITION']),dtype=np.uint16);remap[used]=np.arange(len(used))
    before_digest=digest(old,faces);attrs={k:v[used].tolist() for k,v in old.items()};newfaces=(remap[faces]).tolist()
    assert len(faces)==876 and digest(attrs,np.asarray(newfaces))==before_digest
    trace=json.loads((ROOT/'assets/models/world/source/forest-tree-pair-v1/qa/horde-irregular-pine-v1-lod1-centerlines.json').read_text())
    tubes=[x for x in trace if x['kind']=='tube' and x['mesh']=='Pine | closed trunk roots branches'];assert len(tubes)==59
    rng=random.Random(20261026);branches=[]
    for level in range(10):
        tube=tubes[7+level*5];path=np.array([[x,z,-y] for x,y,z in tube['points']],float);base=path[0]
        assert path[:,1].min()>3.2
        branches.append(path)
        for side in [1,2]:
            angle=side*2*math.pi/3+rng.uniform(-.20,.20);c,s=math.cos(angle),math.sin(angle);rot=np.array([[c,0,s],[0,1,0],[-s,0,c]])
            shifted=(path-base)@rot.T*rng.uniform(.88,1.16)+base
            branches.append(shifted)
            points=np.array([shifted[0],path_point(shifted,.55),shifted[-1]])
            radius=tube['radii'][0]*.86;v,f=closed_tube(points,[radius,radius*.47,.004],4);assert len(f)==20
            append_mesh(attrs,newfaces,v,f)
    foliage=next(m for m in j['meshes'] if 'needle-spray' in m['name'])['primitives'][0];leaf,leaffaces=mesh_arrays(j,views,foliage)
    newleaf={k:v.copy() for k,v in leaf.items()};attached=[];component=0;twig_count=0
    def spray(anchor,axis,width,length):
        nonlocal component
        k=component*10;points=leaf['POSITION'][k:k+10].astype(float);a=unit(points[4]-points[0]);side=unit(points[6]-points[2]);side=unit(side-a*np.dot(side,a));normal=np.cross(a,side)
        side2,normal2=frame(axis);roll=rng.uniform(-math.pi,math.pi);side2,normal2=side2*math.cos(roll)+normal2*math.sin(roll),-side2*math.sin(roll)+normal2*math.cos(roll)
        oldlen=np.linalg.norm(points[4]-points[0]);oldwidth=np.linalg.norm(points[6]-points[2])/2.3
        transform=np.column_stack((axis,side2,normal2))@np.diag([length/oldlen,width/oldwidth,width/oldwidth])@np.column_stack((a,side,normal)).T
        assert np.linalg.det(transform)>0
        newleaf['POSITION'][k:k+10]=anchor+(points-points[0])@transform.T
        n=leaf['NORMAL'][k:k+10].astype(float)@np.linalg.inv(transform);n/=np.linalg.norm(n,axis=1)[:,None]
        t=leaf['TANGENT'][k:k+10,:3].astype(float)@transform.T;t-=n*np.sum(n*t,axis=1)[:,None];t/=np.linalg.norm(t,axis=1)[:,None]
        newleaf['NORMAL'][k:k+10]=n;newleaf['TANGENT'][k:k+10,:3]=t
        assert np.linalg.norm(newleaf['POSITION'][k]-anchor)<1e-6
        attached.append(anchor.tolist());component+=1
    for number,path in enumerate(branches):
        count=3 if number<20 else 2;direction=unit(path[-1]-path[0]);across=np.array([direction[2],0.,-direction[0]]);across=unit(across)
        for index in range(count):
            anchor=path_point(path,.43+(index+1)*.45/count);sign=1 if index%2 else -1
            axis=unit(direction*.40+across*sign*.77+np.array([0.,.32,0.]));tip=anchor+axis*rng.uniform(.32,.52)
            v,f=closed_tube(np.array([anchor,tip]),[.014,.0025],3);assert len(f)==8;append_mesh(attrs,newfaces,v,f);twig_count+=1
            leaves=12//count
            for q in range(leaves):
                a=anchor+(tip-anchor)*(.15+q*.75/max(1,leaves-1));spread=across*(1 if q%2 else -1)
                leafaxis=unit(axis*.55+spread*.45+np.array([0.,rng.uniform(.12,.38),0.]))
                taper=.85 if number>=24 else 1.
                spray(a,leafaxis,rng.uniform(.085,.12)*taper,rng.uniform(.37,.53)*taper)
    for n in range(6):
        angle=n*2.399;anchor=np.array([.43+math.cos(angle)*.06,8.38+n*.022,-.04+math.sin(angle)*.06]);axis=unit(np.array([math.cos(angle)*.38,.65,math.sin(angle)*.38]));spray(anchor,axis,.07,.31)
    assert component==366 and twig_count==80 and len(newfaces)==1916
    attrs={k:np.asarray(v,dtype='<f4') for k,v in attrs.items()};newfaces=np.asarray(newfaces,dtype='<u2')
    assert digest(attrs,newfaces[:len(faces)])==before_digest
    changed=set()
    for primitive,values,indices in [(wood,attrs,newfaces),(foliage,newleaf,leaffaces)]:
        for name,data in [*values.items(),('indices',indices)]:
            ai=primitive['indices'] if name=='indices' else primitive['attributes'][name];access=j['accessors'][ai];view=access['bufferView'];changed.add(view)
            a=np.asarray(data,dtype='<u2' if name=='indices' else '<f4');assert np.isfinite(a).all();views[view]=a.tobytes();access['count']=a.size if name=='indices' else len(a)
            if name=='POSITION':access['min']=a.min(axis=0).tolist();access['max']=a.max(axis=0).tolist()
    data=bytearray()
    for view,payload in zip(j['bufferViews'],views):data.extend(b'\0'*((-len(data))%4));view['byteOffset']=len(data);view['byteLength']=len(payload);data.extend(payload)
    data.extend(b'\0'*((-len(data))%4));j['buffers'][0]['byteLength']=len(data)
    assert all(v==unchanged[i] for i,v in enumerate(views) if i not in changed)
    js=json.dumps(j,separators=(',',':')).encode();js+=b' '*((-len(js))%4);payload=struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(data),0x004e4942)+data;result=struct.pack('<III',0x46546c67,2,12+len(payload))+payload
    total_vertices=sum(j['accessors'][p['attributes']['POSITION']]['count'] for m in j['meshes'] for p in m['primitives']);total_tris=sum(j['accessors'][p['indices']]['count']//3 for m in j['meshes'] for p in m['primitives']);assert total_vertices<=5632 and total_tris==5000
    (out/source.name).write_bytes(result)
    return {'sourceSha256':sha(original),'candidateSha256':sha(result),'triangles':total_tris,'vertices':total_vertices,'materials':len(j['materials']),'preservedWoodTriangles':len(faces),'preservedWoodTriangleAttributeSha256':before_digest,'retainedPrimaryBoughs':10,'addedPrimaryBoughs':20,'newSupportedTwigs':twig_count,'attachedNeedleSprays':len(attached),'mossImagesMaterialsUnchanged':True,'sourceTrunkRootsLowerStubsPreserved':True,'runtimeAdmitted':False}
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);args=p.parse_args(sys.argv[sys.argv.index('--')+1:]);assert args.output.resolve().is_relative_to(ROOT)
    out=args.output/'models/world/runtime/forest-tree-pair-v1';out.mkdir(parents=True,exist_ok=False);record=build_pine(out)
    for name in ['horde-upright-alder-v1-lod1.glb','asset.manifest.json']:(out/name).write_bytes((SOURCE/name).read_bytes())
    (args.output.parent/'geometry-receipt.json').write_text(json.dumps({'sourceHead':'e0390055','pine':record,'alder':'original byte-identical control','productionChanged':False},indent=2)+'\n');print(json.dumps(record,indent=2))
if __name__=='__main__':main()
