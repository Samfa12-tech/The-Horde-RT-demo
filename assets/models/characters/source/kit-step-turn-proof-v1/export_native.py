"""Append proof joint clips to the pinned GLB; never round-trip its static data."""
import bpy,json,struct,pathlib,hashlib,math,argparse,sys,copy
from mathutils import Matrix,Vector,Quaternion
ROOT=next(p for p in pathlib.Path(__file__).resolve().parents if (p/'src/gameplay/dialogue/CompanionRouteFixture.h').is_file())
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=False)
base=ROOT/'assets/models/characters/runtime/kit-warden-v1/kit-warden-lod0.runtime.glb'
data=base.read_bytes();assert hashlib.sha256(data).hexdigest()=='1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713'
size=struct.unpack_from('<I',data,12)[0];doc=json.loads(data[20:20+size]);original=copy.deepcopy(doc)
binary=bytearray(data[28+size:]);old_binary=bytes(binary)
def accessor(i):
 a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
 f={5121:'B',5123:'H',5125:'I',5126:'f'}[a['componentType']];fmt='<'+f*n;stride=v.get('byteStride',struct.calcsize(fmt))
 return [struct.unpack_from(fmt,binary,v.get('byteOffset',0)+a.get('byteOffset',0)+j*stride) for j in range(a['count'])]
def local(t,q,s):return Matrix.Translation(Vector(t))@Quaternion((q[3],q[0],q[1],q[2])).to_matrix().to_4x4()@Matrix.Diagonal((*s,1))
def from_column(values):return Matrix(tuple(tuple(values[c*4+r] for c in range(4)) for r in range(4)))
parents={c:i for i,n in enumerate(doc['nodes']) for c in n.get('children',[])}
trs={i:[n.get('translation',[0,0,0]),n.get('rotation',[0,0,0,1]),n.get('scale',[1,1,1])] for i,n in enumerate(doc['nodes'])}
idle=next(a for a in doc['animations'] if a['name']=='Warden|idle')
for ch in idle['channels']:
 s=idle['samplers'][ch['sampler']];assert accessor(s['input'])[0][0]==0
 trs[ch['target']['node']][{'translation':0,'rotation':1,'scale':2}[ch['target']['path']]]=accessor(s['output'])[0]
g0={}
def original_global(i):
 if i not in g0:
  n=doc['nodes'][i];m=from_column(n['matrix']) if 'matrix' in n else local(*trs[i])
  g0[i]=original_global(parents[i])@m if i in parents else m
 return g0[i]
for i in range(len(doc['nodes'])):original_global(i)
joints=doc['skins'][0]['joints'];names={i:doc['nodes'][i]['name'] for i in joints}
primitive=doc['meshes'][0]['primitives'][0];positions=accessor(primitive['attributes']['POSITION'])
influences=accessor(primitive['attributes']['JOINTS_0']);weights=accessor(primitive['attributes']['WEIGHTS_0'])
ibm=[from_column(v) for v in accessor(doc['skins'][0]['inverseBindMatrices'])]
native_idle=[]
for p,js,ws in zip(positions,influences,weights):
 q=Vector((0,0,0))
 for j,w in zip(js,ws):
  if w:q+=w*(g0[joints[j]]@ibm[j]@Vector(p))
 native_idle.append(q)
C=Matrix.Rotation(math.pi/2,4,'X');Ci=C.inverted()
def mesh_points(mesh):
 ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());m=ev.to_mesh()
 p=[ev.matrix_world@v.co for v in m.vertices];ev.to_mesh_clear();return p
def append(values,kind):
 n={'SCALAR':1,'VEC3':3,'VEC4':4}[kind];start=len(binary);assert start%4==0
 for v in values:binary.extend(struct.pack('<'+'f'*n,*v))
 view=len(doc['bufferViews']);doc['bufferViews'].append({'buffer':0,'byteOffset':start,'byteLength':len(binary)-start})
 item={'bufferView':view,'componentType':5126,'count':len(values),'type':kind}
 if kind=='SCALAR':item.update(min=[min(v[0] for v in values)],max=[max(v[0] for v in values)])
 index=len(doc['accessors']);doc['accessors'].append(item);return index
references=[];motions=[];summaries=[];sole_indices=None
for scene in bpy.data.scenes:
 if 'authored 45 degree proof' not in scene.name:continue
 bpy.context.window.scene=scene
 arm=next(o for o in scene.objects if o.type=='ARMATURE')
 mesh=next(o for o in scene.objects if o.type=='MESH' and o.name.startswith('briarhold-warden'))
 root=next(o for o in scene.objects if o.name.startswith('PROOF ONLY root yaw'))
 direction='left' if scene.name.startswith('LEFT') else 'right';slot=0 if direction=='left' else 1
 scene.frame_set(1);initial=mesh_points(mesh)
 initial_error=max((Ci@p-q).length for p,q in zip(initial,native_idle));assert len(initial)==len(native_idle) and initial_error<.0001,(direction,initial_error)
 correction={i:(arm.matrix_world@arm.pose.bones[names[i]].matrix).inverted()@C@g0[i] for i in joints}
 calibration_error=max((arm.matrix_world@arm.pose.bones[names[i]].matrix).translation.__sub__((C@g0[i]).translation).length for i in joints)
 assert calibration_error<.0001,calibration_error
 if sole_indices is None:
  sole_indices=set()
  for side in ('Left','Right'):
   groups={mesh.vertex_groups[side+n].index for n in ('UpLeg','Leg','Foot','ToeBase')}
   boot=[v.index for v in mesh.data.vertices if v.groups and all(g.group in groups for g in v.groups if g.weight>1e-5) and initial[v.index].z<.25]
   low=min(initial[i].z for i in boot);sole_indices.update(i for i in boot if initial[i].z<=low+.006)
 indices=sorted(sole_indices|set(range(0,len(initial),421))|{len(initial)-1})
 tracks={i:{k:[] for k in ('translation','rotation','scale')} for i in joints};curve=[]
 for frame in range(1,47):
  scene.frame_set(frame);remove=root.matrix_world.inverted()
  native_world={i:Ci@remove@arm.matrix_world@arm.pose.bones[names[i]].matrix@correction[i] for i in joints}
  for i in joints:
   parent=native_world.get(parents.get(i),g0.get(parents.get(i),Matrix.Identity(4)))
   matrix=parent.inverted()@native_world[i];t,q,s=matrix.decompose()
   values=[q.x,q.y,q.z,q.w]
   if tracks[i]['rotation'] and sum(x*y for x,y in zip(values,tracks[i]['rotation'][-1]))<0:values=[-x for x in values]
   tracks[i]['translation'].append(list(t));tracks[i]['rotation'].append(values);tracks[i]['scale'].append(list(s))
  curve.append([(frame-1)/60,root.rotation_euler.z])
 time_accessor=append([[(f-1)/60] for f in range(1,47)],'SCALAR')
 animation={'name':'KitProof|turn_'+direction,'samplers':[],'channels':[]}
 for i in joints:
  for path in ('translation','rotation','scale'):
   output=append(tracks[i][path],'VEC4' if path=='rotation' else 'VEC3')
   animation['channels'].append({'sampler':len(animation['samplers']),'target':{'node':i,'path':path}})
   animation['samplers'].append({'input':time_accessor,'output':output,'interpolation':'LINEAR'})
 doc['animations'].append(animation)
 motions.append({'clip':animation['name'],'axis':'asset-space +Y model yaw; logical yaw delta is its negative under CompanionModelYaw','rootYawEmbeddedInJointClip':False,'curveSecondsModelYawRadians':curve})
 for sample in range(19):
  seconds=sample/24;f=1+seconds*60;scene.frame_set(int(f),subframe=f-int(f));points=mesh_points(mesh)
  references.append((slot,seconds,root.rotation_euler.z,[(i,list(Ci@points[i])) for i in indices]))
 summaries.append({'direction':direction,'calibrationJointOriginMaxErrorM':calibration_error,'idleGeometryMaxErrorM':initial_error,'referenceVertices':len(indices)})
assert bytes(binary[:len(old_binary)])==old_binary
for key in ('nodes','skins','meshes','materials','images','textures','samplers','scenes','scene'):
 assert doc.get(key)==original.get(key),key
assert doc['animations'][:len(original['animations'])]==original['animations']
assert doc['bufferViews'][:len(original['bufferViews'])]==original['bufferViews']
assert doc['accessors'][:len(original['accessors'])]==original['accessors']
doc['buffers'][0]['byteLength']=len(binary)
encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);binary+=bytes((-len(binary))%4)
result=struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary
(OUT/'kit-turn-proof.glb').write_bytes(result)
(OUT/'asset.manifest.json').write_bytes((base.parent/'asset.manifest.json').read_bytes())
with (OUT/'native-reference.bin').open('xb') as stream:
 stream.write(struct.pack('<4sI',b'KTP1',len(references)))
 for slot,seconds,yaw,points in references:
  stream.write(struct.pack('<IffI',slot,seconds,yaw,len(points)))
  for index,p in points:stream.write(struct.pack('<Ifff',index,*p))
(OUT/'root-motion.json').write_text(json.dumps({'schema':1,'clips':motions},indent=2)+'\n')
(OUT/'export-receipt.json').write_text(json.dumps({'schema':1,'sourceBlendSha256':hashlib.sha256(pathlib.Path(bpy.data.filepath).read_bytes()).hexdigest(),'baseGlbSha256':hashlib.sha256(data).hexdigest(),'outputGlbSha256':hashlib.sha256(result).hexdigest(),'outputBytes':len(result),'originalBinaryPrefixBytes':len(old_binary),'staticDataAndOriginalClipsUnchanged':True,'rootMotionNotBakedTwice':True,'directions':summaries,'nativeAcceptance':'Pending actual existing-loader validation, not inferred from export.'},indent=2)+'\n')
print('NATIVE_EXPORT',json.dumps(summaries),flush=True)
