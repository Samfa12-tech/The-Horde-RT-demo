"""Read-only GLB byte inspection; no dependency on provider receipts."""
from pathlib import Path
import struct,json,hashlib,io,math,sys
from PIL import Image
import numpy as np
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent.parent
reports=[]
for path in sorted((root/'runtime').glob('*.glb')):
 b=path.read_bytes(); magic,ver,ln=struct.unpack_from('<4sII',b); assert magic==b'glTF' and ver==2 and ln==len(b)
 jslen,kind=struct.unpack_from('<II',b,12); assert kind==0x4e4f534a; j=json.loads(b[20:20+jslen]); binlen,binkind=struct.unpack_from('<II',b,20+jslen); assert binkind==0x004e4942; binary=b[28+jslen:]
 def arr(i):
  ac=j['accessors'][i]; view=j['bufferViews'][ac['bufferView']]; dtype={5120:'i1',5121:'u1',5122:'<i2',5123:'<u2',5125:'<u4',5126:'<f4'}[ac['componentType']]; dim={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[ac['type']]; off=view.get('byteOffset',0)+ac.get('byteOffset',0)
  if 'byteStride' in view: return np.ndarray((ac['count'],dim),dtype=dtype,buffer=binary,offset=off,strides=(view['byteStride'],np.dtype(dtype).itemsize)).copy()
  return np.frombuffer(binary,dtype=dtype,count=ac['count']*dim,offset=off).reshape(-1,dim)
 ps=[]; tris=0; degenerate=0; bad=0; colors=[]; meshes=[]
 for me in j.get('meshes',[]):
  mt=0
  for pr in me['primitives']:
   assert pr.get('mode',4)==4; pos=arr(pr['attributes']['POSITION']); idx=arr(pr['indices']).reshape(-1,3); assert idx.max()<len(pos); ps.append(pos); mt+=len(idx)
   bad+=int((~np.isfinite(pos)).sum()); v=pos[idx]; degenerate+=int((np.linalg.norm(np.cross(v[:,1]-v[:,0],v[:,2]-v[:,0]),axis=1)<1e-9).sum())
   assert 'NORMAL' in pr['attributes'] and 'TEXCOORD_0' in pr['attributes']; assert np.isfinite(arr(pr['attributes']['NORMAL'])).all(); assert np.isfinite(arr(pr['attributes']['TEXCOORD_0'])).all()
  meshes.append({'name':me['name'],'triangles':mt}); tris+=mt
 ims=[]
 for im in j.get('images',[]):
  v=j['bufferViews'][im['bufferView']]; raw=binary[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']]; image=Image.open(io.BytesIO(raw)); data=np.asarray(image.convert('RGB')); ims.append({'name':im.get('name'),'dimensions':list(image.size),'byte_length':len(raw),'range':[int(data.min()),int(data.max())],'not_black':bool(data.max()>0)})
  assert image.size==(512,512); assert data.max()>0
 positions=np.concatenate(ps); opaque=all(m.get('alphaMode','OPAQUE')=='OPAQUE' for m in j['materials'])
 assert opaque and bad==0 and degenerate==0
 r={'file':str(path.relative_to(root)),'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'glb_v2_header_valid':True,'mesh_count':len(j['meshes']),'triangle_count':tris,'meshes':meshes,'material_count':len(j['materials']),'materials_opaque':opaque,'all_materials_backface_culling':all(not m.get('doubleSided',False) for m in j['materials']),'images':ims,'positions_finite':bad==0,'degenerate_triangles':degenerate,'bounds_gltf_local_xyz':{'min':positions.min(axis=0).tolist(),'max':positions.max(axis=0).tolist()},'nodes':[{'name':n.get('name'),'rotation':n.get('rotation',[0,0,0,1]),'scale':n.get('scale',[1,1,1]),'translation':n.get('translation',[0,0,0])} for n in j['nodes']],'external_uris':any('uri' in x for k in ['buffers','images'] for x in j.get(k,[])),'source_blender_axis':'Z up','exported_scene_axis':'Y up via node rotation','engine_import':'not tested','game_dev_canonical_validation':'not available; game-dev CLI absent'}
 reports.append(r)
(root/'qa'/'runtime-byte-inspection.json').write_text(json.dumps(reports,indent=2)); print(json.dumps(reports,indent=2))
