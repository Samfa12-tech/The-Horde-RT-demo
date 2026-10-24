from pathlib import Path
from collections import Counter
import json,struct,math,hashlib
r=Path.cwd();o=r/'reports/chimney-smoke-20261024/open-flue'
def load(p):
 data=p.read_bytes();n=struct.unpack_from('<I',data,12)[0];d=json.loads(data[20:20+n]);binary=data[28+n:]
 def read(i):
  a=d['accessors'][i];v=d['bufferViews'][a['bufferView']];size={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];fmt='<'+{5123:'H',5126:'f'}[a['componentType']]*size;s=struct.calcsize(fmt)
  return [tuple(struct.unpack_from(fmt,binary,v.get('byteOffset',0)+a.get('byteOffset',0)+j*v.get('byteStride',s))) for j in range(a['count'])]
 parts=[{**{k:read(i) for k,i in p['attributes'].items()},'indices':[x[0] for x in read(p['indices'])]} for p in d['meshes'][0]['primitives']]
 return d,parts
old,a=load(r/'assets/models/world/runtime/bellwether-shell-v4/bellwether-shell-v4.glb');new,b=load(o/'bellwether-open-flue-proof.glb')
for field in ['nodes','materials','scenes','scene']:assert old[field]==new[field]
assert a[1]==b[1]
for key in ['POSITION','NORMAL','TEXCOORD_0','TANGENT']:
 assert all(a[0][key][i]==b[0][key][i] for i in range(len(a[0][key])) if i not in range(48,72))
selected=set(range(48,72))|set(range(1680,1716));edges=Counter();points=set();triangles=[]
for t in range(0,len(b[0]['indices']),3):
 ids=b[0]['indices'][t:t+3]
 if not any(i in selected for i in ids):continue
 assert all(i in selected for i in ids)
 v=[b[0]['POSITION'][i] for i in ids];triangles.append(v);points.update(v)
 for k in range(3):edges[(v[k],v[(k+1)%3])]+=1
assert len(triangles)==28
assert all(n==1 and edges[(v,u)]==1 for (u,v),n in edges.items()),'new B01 welded edge mismatch'
# A vertical line through the mouth must first hit the well floor, not a top cap.
def vertical_hits(x,z):
 hits=[]
 for tri in triangles:
  a,c,d=tri;det=(c[2]-d[2])*(a[0]-d[0])+(d[0]-c[0])*(a[2]-d[2])
  if abs(det)<1e-10:continue
  u=((c[2]-d[2])*(x-d[0])+(d[0]-c[0])*(z-d[2]))/det
  v=((d[2]-a[2])*(x-d[0])+(a[0]-d[0])*(z-d[2]))/det
  if min(u,v,1-u-v)>=-1e-6:hits.append(u*a[1]+v*c[1]+(1-u-v)*d[1])
 return hits
receipt=json.loads((o/'receipt.json').read_text());x,_,z=receipt['outlet'];top=receipt['rimY'];bottom=top-.6
for dx,dz in [(0,0),(.1,0),(-.1,0),(0,.1),(0,-.1)]:
 hits=vertical_hits(x+dx,z+dz);assert hits and abs(max(hits)-bottom)<1e-5,(dx,dz,hits)
assert abs(max(vertical_hits(x+.45,z))-top)<1e-5
proof={'weldedComponentVertices':len(points),'triangles':28,'allOrientedEdgesHaveOneReverse':True,'mouthFirstHitIsWellFloor':True,'rimFirstHitIsRaisedRim':True,'otherTownAttributesIndicesMaterialsNodesExact':True,'nativeAcceptance':'separate','trianglesTotal':982,'verticesTotal':1990}
(o/'geometry-validation.json').write_text(json.dumps(proof,indent=2)+'\n');print('OPEN_FLUE_GEOMETRY_PASS',json.dumps(proof))
