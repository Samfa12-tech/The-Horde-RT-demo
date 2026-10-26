"""Owned crown massing sample. Blender -b --threads 2 --python <this> -- --output reports/<fresh>/candidate/assets

Rebuild foliage as overlapping closed three-dimensional lobes; preserve every
wood/moss/image byte and the original triangle budgets. No Blender exporter.
Unadmitted source study, not a new runtime binding or canonical package.
"""
from pathlib import Path
import argparse, hashlib, json, math, random, struct, sys
import numpy as np

ROOT=Path(__file__).resolve().parents[5]
SOURCE=ROOT/'assets/models/world/runtime/forest-tree-pair-v1'
PHI=(1+math.sqrt(5))/2
ICO=np.array([(-1,PHI,0),(1,PHI,0),(-1,-PHI,0),(1,-PHI,0),(0,-1,PHI),(0,1,PHI),
    (0,-1,-PHI),(0,1,-PHI),(PHI,0,-1),(PHI,0,1),(-PHI,0,-1),(-PHI,0,1)],float)
ICO/=np.linalg.norm(ICO,axis=1)[:,None]
FACES=np.array([(0,11,5),(0,5,1),(0,1,7),(0,7,10),(0,10,11),(1,5,9),(5,11,4),(11,10,2),(10,7,6),(7,1,8),
    (3,9,4),(3,4,2),(3,2,6),(3,6,8),(3,8,9),(4,9,5),(2,4,11),(6,2,10),(8,6,7),(9,8,1)],np.uint16)
OCTA=np.array([(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)],float)
OF=np.array([(0,2,4),(4,2,1),(1,2,5),(5,2,0),(0,4,3),(4,1,3),(1,5,3),(5,0,3)],np.uint16)

def sha(b):return hashlib.sha256(b).hexdigest()
def unpack(p):
    raw=p.read_bytes();magic,ver,size=struct.unpack_from('<III',raw);assert (magic,ver,size)==(0x46546c67,2,len(raw))
    n,kind=struct.unpack_from('<II',raw,12);assert kind==0x4e4f534a
    j=json.loads(raw[20:20+n]);bn,bk=struct.unpack_from('<II',raw,20+n);assert bk==0x004e4942
    data=raw[28+n:28+n+bn]
    return raw,j,[data[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']] for v in j['bufferViews']]

def crowns(species):
    rng=random.Random(97261 if species=='pine' else 97262);lobes=[]
    if species=='pine':
        tiers=[(3.70,1.55,16),(4.35,1.70,18),(4.80,1.75,18),(5.35,1.50,16),(5.90,1.45,16),
            (6.35,1.25,14),(6.80,1.12,14),(7.25,.90,12),(7.70,.65,10),(8.10,.40,8),(8.45,.16,4)]
    else:tiers=[(5.0,.65,22),(5.65,1.0,22),(6.3,1.25,22),(6.95,1.40,22),(7.6,1.15,22),(8.2,.60,22)]
    for level,(y,radius,count) in enumerate(tiers):
        for n in range(count):
            angle=n*2*math.pi/count+level*2.399+rng.uniform(-.18,.18)
            ring=(.22 if n%4==0 else rng.uniform(.52,.78))*radius
            # Slightly drifting core and uneven whorls retain the bent trunk,
            # open lower stubs and species-specific cone/fork silhouette.
            corex=.04+(y-3.6)*.085 if species=='pine' else .25+.17*math.sin(level*.9)
            corez=-.12 if species=='pine' else -.05
            center=np.array([corex+math.cos(angle)*ring,y+rng.uniform(-.17,.17),corez+math.sin(angle)*ring])
            scales=np.array([.25+.20*radius,.15+.12*radius,.27+.19*radius]) if species=='pine' else np.array([.41,.37,.44])
            scales*=rng.uniform(.88,1.12)
            lobes.append((center,scales,angle))
    assert len(lobes)==(146 if species=='pine' else 132)
    if species=='pine':lobes.append((np.array([.42,8.60,-.08]),np.array([.16,.18,.16]),0.))
    return lobes

def foliage(species):
    positions=[];normals=[];uv=[];tangents=[];indices=[];components=[]
    lobes=crowns(species)
    for n,(center,scales,angle) in enumerate(lobes):
        sphere,faces=(OCTA,OF) if species=='pine' and n==len(lobes)-1 else (ICO,FACES)
        c,s=math.cos(angle),math.sin(angle);rotation=np.array([[c,0,s],[0,1,0],[-s,0,c]])
        p=(sphere*scales)@rotation.T+center
        normal=(sphere/scales)@rotation.T;normal/=np.linalg.norm(normal,axis=1)[:,None]
        tangent=np.column_stack((-sphere[:,2]*scales[0],np.zeros(len(p)),sphere[:,0]*scales[2]))@rotation.T
        pole=np.linalg.norm(tangent,axis=1)<1e-8;tangent[pole]=[1.,0.,0.]
        tangent-=normal*np.sum(normal*tangent,axis=1)[:,None];tangent/=np.linalg.norm(tangent,axis=1)[:,None]
        # Closed components: two oppositely directed uses of every edge.
        edges={}
        for f in faces:
            for a,b in zip(f,np.roll(f,-1)):edges[(int(a),int(b))]=edges.get((int(a),int(b)),0)+1
        assert all(v==1 and edges.get((b,a))==1 for (a,b),v in edges.items())
        triangles=p[faces];area=np.linalg.norm(np.cross(triangles[:,1]-triangles[:,0],triangles[:,2]-triangles[:,0]),axis=1)*.5
        volume=np.einsum('ij,ij->i',triangles[:,0]-center,np.cross(triangles[:,1]-center,triangles[:,2]-center)).sum()/6
        assert area.min()>1e-9 and volume>0
        indices.extend((faces+len(positions)).ravel().tolist());positions.extend(p.tolist());normals.extend(normal.tolist())
        uv.extend(np.column_stack((np.arctan2(sphere[:,2],sphere[:,0])/(2*math.pi)+.5,np.arccos(sphere[:,1])/math.pi)).tolist())
        tangents.extend(np.column_stack((tangent,np.ones(len(p)))).tolist())
        components.append({'center':center.tolist(),'radii':scales.tolist(),'minimumTriangleArea':float(area.min()),'positiveVolume':float(volume)})
    return {'POSITION':np.array(positions,dtype='<f4'),'NORMAL':np.array(normals,dtype='<f4'),
        'TEXCOORD_0':np.array(uv,dtype='<f4'),'TANGENT':np.array(tangents,dtype='<f4')},np.array(indices,dtype='<u2'),components

def derive(species,stem,out):
    source=SOURCE/(stem+'-lod1.glb');original,j,views=unpack(source)
    oldviews=views[:];oldj=json.loads(json.dumps(j))
    mesh=next(m for m in j['meshes'] if 'needle-spray' in m['name'] or 'opaque leaves' in m['name']);p=mesh['primitives'][0]
    attributes,indices,components=foliage(species);changed=set()
    assert len(indices)==j['accessors'][p['indices']]['count']
    for name,values in [*attributes.items(),('indices',indices)]:
        ai=p['indices'] if name=='indices' else p['attributes'][name];a=j['accessors'][ai];v=a['bufferView']
        assert not a.get('byteOffset',0) and 'byteStride' not in j['bufferViews'][v]
        a['count']=len(values);views[v]=values.tobytes();changed.add(v)
        if name=='POSITION':a['min']=values.min(axis=0).tolist();a['max']=values.max(axis=0).tolist()
        assert np.isfinite(values).all()
    data=bytearray()
    for v,payload in zip(j['bufferViews'],views):
        data.extend(b'\0'*((-len(data))%4));v['byteOffset']=len(data);v['byteLength']=len(payload);data.extend(payload)
    data.extend(b'\0'*((-len(data))%4));j['buffers'][0]['byteLength']=len(data)
    assert j['materials']==oldj['materials'] and j['images']==oldj['images']
    unchanged=[{'view':i,'sha256':sha(v)} for i,v in enumerate(views) if i not in changed]
    assert all(v==oldviews[i] for i,v in enumerate(views) if i not in changed)
    js=json.dumps(j,separators=(',',':')).encode();js+=b' '*((-len(js))%4)
    payload=struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(data),0x004e4942)+data
    result=struct.pack('<III',0x46546c67,2,12+len(payload))+payload;(out/source.name).write_bytes(result)
    triangles=sum(j['accessors'][p['indices']]['count']//3 for m in j['meshes'] for p in m['primitives'])
    vertices=sum(j['accessors'][p['attributes']['POSITION']]['count'] for m in j['meshes'] for p in m['primitives'])
    assert triangles==(5000 if species=='pine' else 5102) and vertices<=5632
    return {'species':species,'source':source.relative_to(ROOT).as_posix(),'sourceSha256':sha(original),'candidateSha256':sha(result),
        'sourceBytes':len(original),'candidateBytes':len(result),'triangles':triangles,'vertices':vertices,'materials':len(j['materials']),
        'closedCrownLobes':len(components),'crownBounds':[attributes['POSITION'].min(axis=0).tolist(),attributes['POSITION'].max(axis=0).tolist()],
        'woodMossImagesMaterialsUnchanged':True,'unchangedViews':unchanged,'components':components,'runtimeAdmitted':False}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    assert args.output.resolve().is_relative_to(ROOT)
    out=args.output/'models/world/runtime/forest-tree-pair-v1';out.mkdir(parents=True,exist_ok=False)
    records=[derive(sp,stem,out) for sp,stem in [('pine','horde-irregular-pine-v1'),('alder','horde-upright-alder-v1')]]
    (out/'asset.manifest.json').write_bytes((SOURCE/'asset.manifest.json').read_bytes())
    (args.output.parent/'geometry-receipt.json').write_text(json.dumps({'sourceHead':'a66d359f','recipeSha256':sha(Path(__file__).read_bytes()),
        'scope':'closed clustered crown topology within original budgets; original wood/moss/images retained; production unchanged','records':records},indent=2)+'\n')
    print(json.dumps([{k:v for k,v in r.items() if k not in ['components','unchangedViews']} for r in records],indent=2))

if __name__=='__main__':main()
