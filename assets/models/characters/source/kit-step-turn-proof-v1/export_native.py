"""Append proof joint clips to the pinned GLB; never round-trip its static data."""
import bpy,json,struct,pathlib,hashlib,math,argparse,sys,copy
from mathutils import Matrix,Vector,Quaternion
ROOT=next(p for p in pathlib.Path(__file__).resolve().parents if (p/'src/gameplay/dialogue/CompanionRouteFixture.h').is_file())
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True)
mode=parser.add_mutually_exclusive_group()
mode.add_argument('--hearth-sit',action='store_true',help='Append the chair-fitted take to the preserved turn payload')
mode.add_argument('--ankle-approach',type=pathlib.Path,metavar='HEARTH_GLB',
 help='Append the two road takes to a rebuilt, pinned hearth-sit GLB')
parser.add_argument('--road-fit',type=pathlib.Path,metavar='ROAD_BLEND',
 help='Use the placed road-contact take for ankle assessment while preserving the original gait')
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
if args.road_fit and not args.ankle_approach:
 parser.error('--road-fit requires --ankle-approach')
OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=False)
runtime=ROOT/'assets/models/characters/runtime/kit-warden-v1'
base=args.ankle_approach or (pathlib.Path(__file__).parent/'native/kit-turn-proof.glb' if args.hearth_sit else runtime/'kit-warden-lod0.runtime.glb')
data=base.read_bytes()
base_hash=('799c699e0a7f454d9b4131c6fb9bf133866045f7c8f9ef6fe05d1fbed7370d9d' if args.ankle_approach else
 'a72091cac1768f66353178247bde804af7e4b41dd482ccaad813215131583c11' if args.hearth_sit else
 '1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713')
assert hashlib.sha256(data).hexdigest()==base_hash
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

def append_joint_clip(name,frames,fps,native_world_at):
 tracks={i:{k:[] for k in ('translation','rotation','scale')} for i in joints}
 for frame in frames:
  native_world=native_world_at(frame)
  for i in joints:
   parent=native_world.get(parents.get(i),g0.get(parents.get(i),Matrix.Identity(4)))
   matrix=parent.inverted()@native_world[i];t,q,s=matrix.decompose()
   values=[q.x,q.y,q.z,q.w]
   if tracks[i]['rotation'] and sum(x*y for x,y in zip(values,tracks[i]['rotation'][-1]))<0:values=[-x for x in values]
   tracks[i]['translation'].append(list(t));tracks[i]['rotation'].append(values);tracks[i]['scale'].append(list(s))
 time_accessor=append([[(frame-frames.start)/fps] for frame in frames],'SCALAR')
 animation={'name':name,'samplers':[],'channels':[]}
 for i in joints:
  for path in ('translation','rotation','scale'):
   output=append(tracks[i][path],'VEC4' if path=='rotation' else 'VEC3')
   animation['channels'].append({'sampler':len(animation['samplers']),'target':{'node':i,'path':path}})
   animation['samplers'].append({'input':time_accessor,'output':output,'interpolation':'LINEAR'})
 doc['animations'].append(animation)

def finish_payload():
 assert bytes(binary[:len(old_binary)])==old_binary
 for key in ('nodes','skins','meshes','materials','images','textures','samplers','scenes','scene'):
  assert doc.get(key)==original.get(key),key
 for key in ('animations','bufferViews','accessors'):
  assert doc[key][:len(original[key])]==original[key],key
 doc['buffers'][0]['byteLength']=len(binary)
 encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);binary.extend(bytes((-len(binary))%4))
 return struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary

def calibrate(scene,arm,mesh,inverse_root):
 # Only the original, undisplaced idle can calibrate Blender's local bone axes.
 neutral=bpy.data.actions['Warden|idle']
 arm.animation_data.action=neutral;arm.animation_data.action_slot=neutral.slots[0]
 scene.frame_set(0)
 initial=[Ci@inverse_root@p for p in mesh_points(mesh)]
 assert len(initial)==len(native_idle)
 initial_error=max((p-q).length for p,q in zip(initial,native_idle));assert initial_error<.0001,initial_error
 bones={i:inverse_root@arm.matrix_world@arm.pose.bones[names[i]].matrix for i in joints}
 calibration_error=max((m.translation-(C@g0[i]).translation).length for i,m in bones.items())
 assert calibration_error<.0001,calibration_error
 correction={i:m.inverted()@C@g0[i] for i,m in bones.items()}

 def at_frame(frame):
  scene.frame_set(frame)
  return {i:Ci@inverse_root@arm.matrix_world@arm.pose.bones[names[i]].matrix@correction[i] for i in joints}
 return initial,initial_error,calibration_error,at_frame

def reference_samples(filename,samples,fps,scene,mesh,initial,inverse_root):
 indices=set(range(0,len(initial),211))|{len(initial)-1}
 floor=min(p.y for p in initial)
 indices.update(i for i in range(0,len(initial),29) if initial[i].y<floor+.06)
 indices=sorted(indices)
 assert len(indices)<512
 with (OUT/filename).open('xb') as stream:
  stream.write(struct.pack('<4sI',b'KSP1',len(samples)))
  for frame in samples:
   scene.frame_set(frame)
   points=mesh_points(mesh)
   stream.write(struct.pack('<fI',(frame-1)/fps,len(indices)))
   for index in indices:stream.write(struct.pack('<Ifff',index,*(Ci@inverse_root@points[index])))

def export_hearth_sit():
 scene=bpy.data.scenes['Bellwether B01 - Kit protected sit']
 bpy.context.window.scene=scene
 arm=next(obj for obj in scene.objects if obj.type=='ARMATURE')
 mesh=scene.objects['briarhold-warden']
 root=scene.objects['STUDY ONLY - seated placement']
 take,slot=arm.animation_data.action,arm.animation_data.action_slot
 frames=range(scene.frame_start,scene.frame_end+1);fps=scene.render.fps
 assert frames==range(1,84) and fps==60 and len(joints)==24
 inverse_root=root.matrix_world.inverted()
 initial,initial_error,calibration_error,at_frame=calibrate(scene,arm,mesh,inverse_root)
 arm.animation_data.action=take;arm.animation_data.action_slot=slot

 name='Kit|hearth_sit'
 append_joint_clip(name,frames,fps,at_frame)
 result=finish_payload()
 output=OUT/'kit-warden-turn.runtime.glb'
 output.write_bytes(result)
 reference_samples('native-sit-reference.bin',(1,9,21,35,42,56,70,83),fps,scene,mesh,initial,inverse_root)
 receipt={'schema':1,'sourceBlendSha256':hashlib.sha256(pathlib.Path(bpy.data.filepath).read_bytes()).hexdigest(),
  'baseGlbSha256':hashlib.sha256(data).hexdigest(),'outputGlbSha256':hashlib.sha256(result).hexdigest(),
  'outputBytes':len(result),'originalBinaryPrefixBytes':len(old_binary),'clip':name,
  'frames':len(frames),'fps':fps,'jointCount':len(joints),'modelRoot':[root.location.x,root.location.z,-root.location.y],
  'rootYawRadians':root.rotation_euler.z,'calibrationJointOriginMaxErrorM':calibration_error,'idleGeometryMaxErrorM':initial_error}
 (OUT/'export-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
 print('HEARTH_SIT_EXPORT',json.dumps(receipt),flush=True)

def export_ankle_approach():
 original_source=pathlib.Path(bpy.data.filepath)
 clips=[]
 initial_errors=[]
 calibration_errors=[]
 for name,last,samples in (('ankle_assess',97,(1,9,15,21,33,61,73,97)),
                           ('protective_walk',53,(1,8,15,21,27,34,42,53))):
  fitted=args.road_fit is not None and name=='ankle_assess'
  source=args.road_fit.resolve() if fitted else original_source
  if pathlib.Path(bpy.data.filepath)!=source:
   bpy.ops.wm.open_mainfile(filepath=str(source))
  scene=bpy.data.scenes['Bellwether lower road - loose stone contact' if fitted else
                        'Bellwether road - Kit ankle and protective gait']
  bpy.context.window.scene=scene
  arm=next(obj for obj in scene.objects if obj.type=='ARMATURE')
  mesh=scene.objects['briarhold-warden']
  fps=scene.render.fps
  assert fps==60 and len(joints)==24
  inverse_root=(scene.objects['Kit - witnessed native road placement'].matrix_world.inverted()
                if fitted else Matrix.Identity(4))
  initial,initial_error,calibration_error,at_frame=calibrate(scene,arm,mesh,inverse_root)
  initial_errors.append(initial_error)
  calibration_errors.append(calibration_error)
  clip_name=f'Kit|{name}'
  take=bpy.data.actions[f'{clip_name}_road_fit' if fitted else clip_name]
  assert tuple(take.frame_range)==(1,last)
  arm.animation_data.action=take;arm.animation_data.action_slot=take.slots[0]
  append_joint_clip(clip_name,range(1,last+1),fps,at_frame)
  reference_samples(f'{name}-reference.bin',samples,fps,scene,mesh,initial,inverse_root)
  clips.append({'name':clip_name,'frames':last})
 result=finish_payload()
 (OUT/'kit-warden-turn.runtime.glb').write_bytes(result)
 receipt={'schema':1,'sourceBlendSha256':hashlib.sha256(original_source.read_bytes()).hexdigest(),
  'baseGlbSha256':base_hash,'outputGlbSha256':hashlib.sha256(result).hexdigest(),
  'outputBytes':len(result),'originalBinaryPrefixBytes':len(old_binary),'clips':clips,
  'fps':fps,'jointCount':len(joints),'calibrationJointOriginMaxErrorM':max(calibration_errors),
  'idleGeometryMaxErrorM':max(initial_errors)}
 if args.road_fit:
  receipt['roadFitBlendSha256']=hashlib.sha256(args.road_fit.read_bytes()).hexdigest()
 (OUT/'export-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
 print('ANKLE_APPROACH_EXPORT',json.dumps(receipt),flush=True)

if args.hearth_sit:
 export_hearth_sit()
 sys.exit(0)
if args.ankle_approach:
 export_ankle_approach()
 sys.exit(0)

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
 curve=[]
 def at_frame(frame):
  scene.frame_set(frame);remove=root.matrix_world.inverted()
  native_world={i:Ci@remove@arm.matrix_world@arm.pose.bones[names[i]].matrix@correction[i] for i in joints}
  curve.append([(frame-1)/60,root.rotation_euler.z])
  return native_world
 name='KitProof|turn_'+direction
 append_joint_clip(name,range(1,47),60,at_frame)
 motions.append({'clip':name,'axis':'asset-space +Y model yaw; logical yaw delta is its negative under CompanionModelYaw','rootYawEmbeddedInJointClip':False,'curveSecondsModelYawRadians':curve})
 for sample in range(19):
  seconds=sample/24;f=1+seconds*60;scene.frame_set(int(f),subframe=f-int(f));points=mesh_points(mesh)
  references.append((slot,seconds,root.rotation_euler.z,[(i,list(Ci@points[i])) for i in indices]))
 summaries.append({'direction':direction,'calibrationJointOriginMaxErrorM':calibration_error,'idleGeometryMaxErrorM':initial_error,'referenceVertices':len(indices)})
result=finish_payload()
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
