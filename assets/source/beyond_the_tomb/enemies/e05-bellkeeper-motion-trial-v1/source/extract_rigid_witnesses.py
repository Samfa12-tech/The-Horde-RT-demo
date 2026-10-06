"""Select exact one-joint-weight witness vertices from meshes[0]/skins[0]."""
import json,struct,sys,hashlib
from pathlib import Path
import numpy as np
source,out,*names=sys.argv[1:];raw=Path(source).read_bytes();jl,jt=struct.unpack_from('<II',raw,12);doc=json.loads(raw[20:20+jl]);bp=20+jl;bl,bt=struct.unpack_from('<II',raw,bp);binary=raw[bp+8:bp+8+bl]
types={5120:np.int8,5121:np.uint8,5122:np.int16,5123:np.uint16,5125:np.uint32,5126:np.float32};widths={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
def get(i):
 a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];dt=np.dtype(types[a['componentType']]);w=widths[a['type']];return np.ndarray((a['count'],w),dtype=dt,buffer=binary,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',w*dt.itemsize),dt.itemsize)).copy()
skin=doc['skins'][0];inv=get(skin['inverseBindMatrices']).reshape((-1,4,4)).transpose(0,2,1);name_by_joint={i:doc['nodes'][n].get('name',str(n)) for i,n in enumerate(skin['joints'])}
rows={};base=0;prim_report=[]
for pi,p in enumerate(doc['meshes'][0]['primitives']):
 a=p['attributes'];positions=get(a['POSITION']);weights=get(a['WEIGHTS_0']);joints=get(a['JOINTS_0']);assert weights.shape[1]==4 and joints.shape[1]==4
 for vi in range(len(positions)):
  active=np.flatnonzero(weights[vi]>1e-6)
  if len(active)!=1 or abs(weights[vi,active[0]]-1)>1e-6:continue
  ji=int(joints[vi,active[0]]);name=name_by_joint[ji]
  if names and name not in names:continue
  local=(inv[ji]@np.r_[positions[vi],1])[:3];rows.setdefault(name,[]).append((base+vi,positions[vi],local))
 prim_report.append({'index':pi,'first_unique_vertex':base,'vertex_count':len(positions),'material':p.get('material')});base+=len(positions)
lines=[];counts={}
for name,vertices in rows.items():
 coords=np.array([v[1] for v in vertices]);selected=set()
 for axis in range(3):selected|={int(coords[:,axis].argmin()),int(coords[:,axis].argmax())}
 for i in np.linspace(0,len(vertices)-1,min(8,len(vertices))).astype(int):selected.add(int(i))
 counts[name]={'rigid_vertex_count':len(vertices),'witness_count':len(selected)}
 for i in sorted(selected):
  vi,p,local=vertices[i];lines.append(f'{name} {vi} '+ ' '.join(format(float(c),'.10g') for c in local))
missing=set(names)-set(rows)
if missing:raise ValueError('No exact-rigid geometry for: '+str(sorted(missing)))
Path(out).write_text('\n'.join(lines)+'\n');Path(out+'.json').write_text(json.dumps({'source':source,'source_sha256':hashlib.sha256(raw).hexdigest(),'primitives':prim_report,'groups':counts,'witness_count':len(lines),'tolerance':'weight > 1e-6 and sum one ±1e-6'},indent=2)+'\n');print(json.dumps(counts))
