"""Independent GLB 2.0 buffer/accessor inspection; standard-library only."""
import json,struct,math,pathlib,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1];out=[]
for p in sorted((ROOT/'models').glob('*.glb')):
 b=p.read_bytes();magic,ver,total=struct.unpack_from('<4sII',b);assert magic==b'glTF' and ver==2 and total==len(b)
 n,kind=struct.unpack_from('<I4s',b,12);assert kind==b'JSON';g=json.loads(b[20:20+n]);off=20+n;bn,bk=struct.unpack_from('<I4s',b,off);assert bk==b'BIN\x00';blob=b[off+8:off+8+bn]
 assert not g.get('images') and not g.get('textures') and not g.get('skins') and not g.get('animations')
 assert not g.get('extensionsRequired');assert all('uri' not in x for x in g.get('buffers',[]))
 def readacc(idx):
  a=g['accessors'][idx];v=g['bufferViews'][a['bufferView']];fmt={5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']];count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];sz=struct.calcsize(fmt)*count;stride=v.get('byteStride',sz);start=v.get('byteOffset',0)+a.get('byteOffset',0)
  assert start+(a['count']-1)*stride+sz<=len(blob)
  values=[struct.unpack_from('<'+fmt*count,blob,start+i*stride) for i in range(a['count'])];assert all(math.isfinite(x) for row in values for x in row);return values
 tri=0;checks=[]
 for m in g['meshes']:
  for q in m['primitives']:
   assert q.get('mode',4)==4;attrs=q['attributes'];assert all(k in attrs for k in ['POSITION','NORMAL','TEXCOORD_0','TANGENT'])
   pos=readacc(attrs['POSITION']);uv=readacc(attrs['TEXCOORD_0']);norm=readacc(attrs['NORMAL']);tan=readacc(attrs['TANGENT']);inds=[x[0] for x in readacc(q['indices'])];assert len(inds)%3==0 and max(inds)<len(pos);tri+=len(inds)//3
   assert all(abs(sum(x*x for x in row)-1)<.003 for row in norm)
   assert all(abs(sum(x*x for x in row[:3])-1)<.003 and abs(abs(row[3])-1)<.001 for row in tan)
   assert all(abs(sum(a*b for a,b in zip(n,t[:3])))<.003 for n,t in zip(norm,tan))
   checks.append({'vertices':len(pos),'triangles':len(inds)//3,'has_uv_normals_tangents':True})
 assert tri<=1500,(p.name,tri)
 for m in g['materials']:
  assert m.get('alphaMode','OPAQUE')=='OPAQUE';assert not m.get('doubleSided',False);assert m.get('emissiveFactor',[0,0,0])==[0,0,0]
 out.append({'file':str(p.relative_to(ROOT)),'triangles':tri,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'primitive_checks':checks,'valid_glb_2':True,'external_dependencies':False,'finite_attributes':True,'normals_tangents_normalized_orthogonal':True,'opaque_nonemissive':True,'authoring_envelope_max_triangles':1500,'native_runtime_tested':False})
(ROOT/'validation/glb-static-validation.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
