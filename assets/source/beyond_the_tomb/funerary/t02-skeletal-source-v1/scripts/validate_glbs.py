"""Local structural GLB verifier; intentionally not a native-engine admission gate."""
import sys,os,json,struct,hashlib,math,argparse
p=argparse.ArgumentParser();p.add_argument('--package',required=True);a=p.parse_args();root=a.package
COMP={5120:('b',1),5121:('B',1),5122:('h',2),5123:('H',2),5125:('I',4),5126:('f',4)};ARITY={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
reports=[]
for f in sorted(os.listdir(root+'/meshes')):
 if not f.endswith('.glb'):continue
 b=open(root+'/meshes/'+f,'rb').read();magic,ver,size=struct.unpack_from('<4sII',b);assert magic==b'glTF' and ver==2 and size==len(b)
 offset=12;js=None;binary=None
 while offset<len(b):
  length,typ=struct.unpack_from('<II',b,offset);offset+=8;data=b[offset:offset+length];offset+=length
  if typ==0x4e4f534a:js=json.loads(data)
  elif typ==0x004e4942:binary=data
 assert js and binary is not None
 assert len(js.get('meshes',[]))==1 and len(js.get('materials',[]))==1
 assert not js.get('skins') and not js.get('animations') and not js.get('textures') and not js.get('images')
 def accessor(idx):
  acc=js['accessors'][idx];view=js['bufferViews'][acc['bufferView']];fmt,n=COMP[acc['componentType']];arity=ARITY[acc['type']];stride=view.get('byteStride',n*arity);start=view.get('byteOffset',0)+acc.get('byteOffset',0)
  assert start+(acc['count']-1)*stride+n*arity<=len(binary)
  return [struct.unpack_from('<'+fmt*arity,binary,start+i*stride) for i in range(acc['count'])]
 tris=verts=degenerate=0;mins=[float('inf')]*3;maxs=[float('-inf')]*3;normal_error=0
 for mesh in js['meshes']:
  for prim in mesh['primitives']:
   assert prim.get('mode',4)==4
   assert 'JOINTS_0' not in prim['attributes'] and 'WEIGHTS_0' not in prim['attributes']
   pos=accessor(prim['attributes']['POSITION']);norm=accessor(prim['attributes']['NORMAL']);idx=[i[0] for i in accessor(prim['indices'])];assert len(idx)%3==0 and min(idx)>=0 and max(idx)<len(pos)
   assert len(norm)==len(pos);tris+=len(idx)//3;verts+=len(pos)
   for v in pos:
    assert all(math.isfinite(x) for x in v)
    for i,x in enumerate(v):mins[i]=min(mins[i],x);maxs[i]=max(maxs[i],x)
   for n in norm:
    assert all(math.isfinite(x) for x in n);normal_error=max(normal_error,abs(sum(x*x for x in n)-1))
   for i in range(0,len(idx),3):
    x,y,z=[pos[k] for k in idx[i:i+3]];v=[y[k]-x[k] for k in range(3)];w=[z[k]-x[k] for k in range(3)];cross=[v[1]*w[2]-v[2]*w[1],v[2]*w[0]-v[0]*w[2],v[0]*w[1]-v[1]*w[0]]
    if sum(c*c for c in cross)<1e-24:degenerate+=1
 assert normal_error<1e-4
 for node in js['nodes']:
  assert 'skin' not in node
  assert node.get('translation',[0,0,0])==[0,0,0]
  assert node.get('rotation',[0,0,0,1])==[0,0,0,1]
  assert node.get('scale',[1,1,1])==[1,1,1]
 mat=js['materials'][0];pbr=mat['pbrMetallicRoughness'];assert pbr['metallicFactor']==0 and abs(pbr['roughnessFactor']-.84)<1e-5
 reports.append({'file':'meshes/'+f,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'meshes':len(js['meshes']),'primitives':sum(len(m['primitives']) for m in js['meshes']),'materials':len(js['materials']),'vertices':verts,'triangles':tris,'degenerate_triangles':degenerate,'max_normal_squared_length_error':normal_error,'bounds_min_gltf_m':mins,'bounds_max_gltf_m':maxs,'dimensions_gltf_m':[maxs[i]-mins[i] for i in range(3)],'skins':0,'animations':0,'textures':0,'extensions_used':js.get('extensionsUsed',[]),'structural_checks':'PASS','topology_caveat':'See build-manifest and source-topology diagnostics. Structural checks do not imply manifold topology or engine admission.'})
print(json.dumps(reports,indent=2));open(root+'/evidence/glb-validation.json','w').write(json.dumps(reports,indent=2))
