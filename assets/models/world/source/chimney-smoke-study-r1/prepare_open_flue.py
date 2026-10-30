"""Owned B01-only open flue proof from admitted v4; no DCC export/temp path used.
Run from worktree. Outputs stay in ignored reports/chimney-smoke-20261024/open-flue.
"""
from pathlib import Path
import json,struct,hashlib,math,copy
R=Path.cwd();OUT=R/'reports/chimney-smoke-20261024/open-flue'
SOURCE=R/'assets/models/world/runtime/bellwether-shell-v4/bellwether-shell-v4.glb'
data=SOURCE.read_bytes();assert hashlib.sha256(data).hexdigest()=='66058108eee9e014acfa10086b9da81e577d40d9f88a3a6d7c2131388eea06ba'
n=struct.unpack_from('<I',data,12)[0];doc=json.loads(data[20:20+n]);binary=data[28+n:]
def access(i):
 a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];w={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];fmt='<'+{5126:'f',5123:'H'}[a['componentType']]*w;size=struct.calcsize(fmt)
 return [list(struct.unpack_from(fmt,binary,v.get('byteOffset',0)+a.get('byteOffset',0)+j*v.get('byteStride',size))) for j in range(a['count'])]
def sub(a,b):return [x-y for x,y in zip(a,b)]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def unit(a):
 s=math.sqrt(dot(a,a));assert s>1e-8;return [x/s for x in a]
parts=[]
for p in doc['meshes'][0]['primitives']:
 parts.append({**{k:access(i) for k,i in p['attributes'].items()},'indices':[x[0] for x in access(p['indices'])]})
a=parts[0];old=copy.deepcopy(a);cx,cz=95.36,109.28;oldtop=max(a['POSITION'][i][1] for i in range(48,72));top=oldtop+.7
assert abs(oldtop+.27955078125)<1e-6
selected=set(range(48,72));cap=[]
for t in range(len(a['indices'])//3):
 ids=a['indices'][3*t:3*t+3]
 if any(i in selected for i in ids):
  assert all(i in selected for i in ids)
  if all(a['NORMAL'][i][1]>.99 for i in ids):cap.append(t)
assert len(cap)==2
for i in selected:
 if abs(a['POSITION'][i][1]-oldtop)<1e-7:
  a['POSITION'][i][1]=top
  if abs(a['NORMAL'][i][1])<1e-6:a['TEXCOORD_0'][i][1]-=.35
assert sum(old['POSITION'][i]!=a['POSITION'][i] for i in selected)==12
a['indices']=[i for t in range(len(a['indices'])//3) if t not in cap for i in a['indices'][3*t:3*t+3]]
def quad(points,normal):
 if dot(cross(sub(points[1],points[0]),sub(points[2],points[0])),normal)<0:points=list(reversed(points))
 u=unit(sub(points[1],points[0]));v=cross(normal,u);base=len(a['POSITION'])
 for point in points:
  d=sub(point,points[0]);a['POSITION'].append(point);a['NORMAL'].append(normal[:]);a['TEXCOORD_0'].append([dot(d,u)*.5,dot(d,v)*.5]);a['TANGENT'].append(u+[1.])
 a['indices'].extend([base,base+1,base+2,base,base+2,base+3])
# Four top rim strips, four inward walls and the well floor; no sealed mouth.
x0,x1=cx-.55,cx+.55;z0,z1=cz-.5,cz+.5;ix0,ix1=cx-.33,cx+.33;iz0,iz1=cz-.28,cz+.28;bottom=top-.6
for points in [ [[x0,top,z0],[x1,top,z0],[ix1,top,iz0],[ix0,top,iz0]], [[x1,top,z0],[x1,top,z1],[ix1,top,iz1],[ix1,top,iz0]], [[x1,top,z1],[x0,top,z1],[ix0,top,iz1],[ix1,top,iz1]], [[x0,top,z1],[x0,top,z0],[ix0,top,iz0],[ix0,top,iz1]] ]:quad(points,[0.,1.,0.])
for points,normal in [ ([[ix0,top,iz0],[ix1,top,iz0],[ix1,bottom,iz0],[ix0,bottom,iz0]],[0.,0.,1.]), ([[ix1,top,iz0],[ix1,top,iz1],[ix1,bottom,iz1],[ix1,bottom,iz0]],[-1.,0.,0.]), ([[ix1,top,iz1],[ix0,top,iz1],[ix0,bottom,iz1],[ix1,bottom,iz1]],[0.,0.,-1.]), ([[ix0,top,iz1],[ix0,top,iz0],[ix0,bottom,iz0],[ix0,bottom,iz1]],[1.,0.,0.]), ([[ix0,bottom,iz0],[ix1,bottom,iz0],[ix1,bottom,iz1],[ix0,bottom,iz1]],[0.,1.,0.]) ]:quad(points,normal)
assert len(a['POSITION'])==1716 and sum(len(x['indices'])//3 for x in parts)==982
# Existing source rows outside B01 remain exact; no roof/other town change.
for key in ['POSITION','NORMAL','TEXCOORD_0','TANGENT']:
 assert all(a[key][i]==old[key][i] for i in range(len(old[key])) if i not in selected)
for part in parts:
 for t in range(0,len(part['indices']),3):
  ids=part['indices'][t:t+3];points=[part['POSITION'][i] for i in ids];face=unit(cross(sub(points[1],points[0]),sub(points[2],points[0])))
  assert min(dot(face,part['NORMAL'][i]) for i in ids)>.9999
outdoc=copy.deepcopy(doc);outdoc['accessors']=[];outdoc['bufferViews']=[];outbin=bytearray()
def put(rows,kind,component):
 while len(outbin)%4:outbin.append(0)
 start=len(outbin);fmt='<'+('f' if component==5126 else 'H')*len(rows[0])
 for row in rows:outbin.extend(struct.pack(fmt,*row))
 vi=len(outdoc['bufferViews']);outdoc['bufferViews'].append({'buffer':0,'byteOffset':start,'byteLength':len(outbin)-start});ai=len(outdoc['accessors']);a={'bufferView':vi,'componentType':component,'count':len(rows),'type':kind}
 if kind=='VEC3':a.update(min=[min(row[k] for row in rows) for k in range(3)],max=[max(row[k] for row in rows) for k in range(3)])
 outdoc['accessors'].append(a);return ai
for pi,part in enumerate(parts):
 p=outdoc['meshes'][0]['primitives'][pi]
 p['attributes']={k:put(part[k],{'POSITION':'VEC3','NORMAL':'VEC3','TEXCOORD_0':'VEC2','TANGENT':'VEC4'}[k],5126) for k in ['POSITION','NORMAL','TEXCOORD_0','TANGENT']};p['indices']=put([[x] for x in part['indices']],'SCALAR',5123)
while len(outbin)%4:outbin.append(0)
outdoc['buffers']=[{'byteLength':len(outbin)}];j=json.dumps(outdoc,separators=(',',':')).encode();j+=b' '*((-len(j))%4)
result=struct.pack('<III',0x46546c67,2,28+len(j)+len(outbin))+struct.pack('<II',len(j),0x4e4f534a)+j+struct.pack('<II',len(outbin),0x004e4942)+outbin
assert not OUT.exists();OUT.mkdir(parents=True)
(OUT/'bellwether-open-flue-proof.glb').write_bytes(result);(OUT/'asset.manifest.json').write_bytes(SOURCE.with_name('asset.manifest.json').read_bytes())
receipt={'status':'isolated owned unadmitted B01 proof','sourceSha256':hashlib.sha256(data).hexdigest(),'candidateSha256':hashlib.sha256(result).hexdigest(),'triangles':982,'vertices':1990,'materials':2,'removedOriginalCapTriangles':cap,'newRimWellFloorTriangles':18,'outlet':[cx,top-.04,cz],'rimY':top,'holeHalfExtent':[.33,.28],'wellDepth':.6,'unchangedOtherSourceAttributeRows':True,'originalRuntimeUntouched':True,'nativeImportAndRT':'not yet run','limitations':['Inherited v4 UV bitangent mismatches outside B01 remain; no unrelated repair.','Source geometry derivative only; no smoke or town gameplay admission.']}
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
