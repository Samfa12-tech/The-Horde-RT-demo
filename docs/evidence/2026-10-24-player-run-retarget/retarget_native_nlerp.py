"""Offline owned-run retarget; original static GLB bytes and Idle remain exact."""
from pathlib import Path
import json,struct,hashlib,copy,bisect,math
from mathutils import Matrix,Vector,Quaternion
ROOT=Path.cwd();OUT=ROOT/'reports/player-run-retarget-20261024'
PLAYER=ROOT/'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
KIT=ROOT/'assets/models/characters/runtime/kit-warden-v1/kit-warden-lod0.runtime.glb'

class Data:
 def __init__(self,p):
  self.path=p;self.data=p.read_bytes();n=struct.unpack_from('<I',self.data,12)[0]
  self.doc=json.loads(self.data[20:20+n]);self.bin=self.data[28+n:]
  self.nodes=self.doc['nodes'];self.parents={c:i for i,n in enumerate(self.nodes) for c in n.get('children',[])}
  self.names={n['name']:i for i,n in enumerate(self.nodes)};self.joints=self.doc['skins'][0]['joints']
 def access(self,i):
  a=self.doc['accessors'][i];v=self.doc['bufferViews'][a['bufferView']]
  num={'SCALAR':1,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']];fmt='<'+{5126:'f',5123:'H',5125:'I'}[a['componentType']]*num
  return [struct.unpack_from(fmt,self.bin,v.get('byteOffset',0)+a.get('byteOffset',0)+j*v.get('byteStride',struct.calcsize(fmt))) for j in range(a['count'])]
 def local(self,i,trs=None):
  n=self.nodes[i]
  if 'matrix' in n:return Matrix(tuple(tuple(n['matrix'][c*4+r] for c in range(4)) for r in range(4)))
  t,q,s=trs if trs is not None else (n.get('translation',[0,0,0]),n.get('rotation',[0,0,0,1]),n.get('scale',[1,1,1]))
  return Matrix.Translation(Vector(t))@Quaternion((q[3],q[0],q[1],q[2])).to_matrix().to_4x4()@Matrix.Diagonal((*s,1))
 def worlds(self,trs=None):
  out={}
  def one(i):
   if i not in out:out[i]=one(self.parents[i])@self.local(i,trs.get(i) if trs else None) if i in self.parents else self.local(i,trs.get(i) if trs else None)
   return out[i]
  for i in range(len(self.nodes)):one(i)
  return out
 def bind(self):
  values=self.access(self.doc['skins'][0]['inverseBindMatrices'])
  return {i:Matrix(tuple(tuple(v[c*4+r] for c in range(4)) for r in range(4))).inverted() for i,v in zip(self.joints,values)}
 def tracks(self,name):
  a=next(x for x in self.doc['animations'] if x['name']==name);out=[]
  for c in a['channels']:
   # Match the existing native reader's linear/Nlerp sampling of these dense
   # LINEAR/STEP-labelled keys; do not claim preservation of glTF STEP semantics.
   s=a['samplers'][c['sampler']];assert s.get('interpolation','LINEAR') in ('LINEAR','STEP')
   out.append((c['target']['node'],c['target']['path'],[t[0] for t in self.access(s['input'])],self.access(s['output'])))
  return out
 def pose(self,tracks,t):
  trs={i:[n.get('translation',[0,0,0]),n.get('rotation',[0,0,0,1]),n.get('scale',[1,1,1])] for i,n in enumerate(self.nodes)}
  for i,path,times,values in tracks:
   k=max(0,min(len(times)-2,bisect.bisect_right(times,t)-1));u=max(0,min(1,(t-times[k])/(times[k+1]-times[k])))
   a,b=values[k],values[k+1]
   if path=='rotation':
    if sum(x*y for x,y in zip(a,b))<0:b=tuple(-v for v in b)
    q=[x+(y-x)*u for x,y in zip(a,b)];norm=math.sqrt(sum(x*x for x in q));assert norm>1e-8;v=[x/norm for x in q]
    assert abs(sum(x*x for x in v)-1)<1e-6
   else:v=[x+(y-x)*u for x,y in zip(a,b)]
   trs[i][{'translation':0,'rotation':1,'scale':2}[path]]=v
  return self.worlds(trs)

p=Data(PLAYER);s=Data(KIT)
assert hashlib.sha256(p.data).hexdigest()=='46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f'
assert hashlib.sha256(s.data).hexdigest()=='1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713'
pb,sb=p.bind(),s.bind();pw=p.worlds();sw=s.worlds()
names=[s.nodes[i]['name'] for i in s.joints]
assert len(names)==24 and all(n in p.names for n in names)
assert all(p.nodes[p.parents[p.names[n]]]['name']==s.nodes[s.parents[s.names[n]]]['name'] for n in names)
def length(d,b,side):return sum((b[d.names[side+a]].translation-b[d.names[side+c]].translation).length for a,c in [('UpLeg','Leg'),('Leg','Foot')])
ratios=[length(p,pb,z)/length(s,sb,z) for z in ('Left','Right')];ratio=sum(ratios)/2
run=s.tracks('Warden|run');duration=max(max(t) for _,_,t,_ in run);assert abs(duration-0.5333333333)<.001
rest_delta={n:(pb[p.names[n]].to_quaternion().rotation_difference(sb[s.names[n]].to_quaternion()).angle*180/math.pi) for n in names}
tracks={i:{x:[] for x in ('translation','rotation','scale')} for i in p.joints}
first_hip=None;globals_saved=[]
for frame in range(65):
 t=frame/64*duration;pose=s.pose(run,t);desired={}
 for n in names:
  i=p.names[n];j=s.names[n]
  desired[i]=(pose[j].to_quaternion()@sb[j].to_quaternion().inverted()@pb[i].to_quaternion()).normalized()
 globals_={}
 def target(i):
  if i in globals_:return globals_[i]
  parent=target(p.parents[i]) if i in p.parents else Matrix.Identity(4)
  node=p.nodes[i];trs=[node.get('translation',[0,0,0]),node.get('rotation',[0,0,0,1]),node.get('scale',[1,1,1])]
  if i in desired:
   q=parent.to_quaternion().inverted()@desired[i];trs[1]=[q.x,q.y,q.z,q.w]
   if node['name']=='Hips':
    worldpos=pb[i].translation+(pose[s.names['Hips']].translation-sb[s.names['Hips']].translation)*ratio
    trs[0]=list(parent.inverted()@worldpos)
  local=p.local(i,trs);world=parent@local;globals_[i]=world
  if i in tracks:
   tr,q,sc=local.decompose();quat=[q.x,q.y,q.z,q.w]
   if tracks[i]['rotation'] and sum(a*b for a,b in zip(quat,tracks[i]['rotation'][-1]))<0:quat=[-v for v in quat]
   tracks[i]['translation'].append(list(tr));tracks[i]['rotation'].append(quat);tracks[i]['scale'].append(list(sc))
  return world
 for i in range(len(p.nodes)):target(i)
 globals_saved.append({n:list(globals_[p.names[n]].translation) for n in ('Hips','LeftFoot','RightFoot')})

doc=copy.deepcopy(p.doc);binary=bytearray(p.bin)
def append(values,kind):
 count={'SCALAR':1,'VEC3':3,'VEC4':4}[kind];start=len(binary);assert start%4==0
 for v in values:binary.extend(struct.pack('<'+'f'*count,*v))
 vi=len(doc['bufferViews']);doc['bufferViews'].append(dict(buffer=0,byteOffset=start,byteLength=len(binary)-start))
 a=dict(bufferView=vi,componentType=5126,count=len(values),type=kind)
 if kind=='SCALAR':a.update(min=[values[0][0]],max=[values[-1][0]])
 ai=len(doc['accessors']);doc['accessors'].append(a);return ai
time=append([[i/64*duration] for i in range(65)],'SCALAR')
clip=dict(name='Walking',channels=[],samplers=[])
for i in p.joints:
 for path in ('translation','rotation','scale'):
  v=append(tracks[i][path],'VEC4' if path=='rotation' else 'VEC3');si=len(clip['samplers'])
  clip['samplers'].append(dict(input=time,output=v,interpolation='LINEAR'));clip['channels'].append(dict(sampler=si,target=dict(node=i,path=path)))
doc['animations'][next(i for i,a in enumerate(doc['animations']) if a['name']=='Walking')]=clip
doc['buffers'][0]['byteLength']=len(binary)
js=json.dumps(doc,separators=(',',':')).encode();js+=b' '*((-len(js))%4)
assert binary[:len(p.bin)]==p.bin
for k in ('nodes','skins','meshes','materials','scenes','scene'):assert doc.get(k)==p.doc.get(k),k
assert doc['animations'][0]==p.doc['animations'][0]
out=OUT/'player-run-native-nlerp-proof.glb';assert not out.exists()
out.write_bytes(struct.pack('<III',0x46546c67,2,28+len(js)+len(binary))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(binary),0x004e4942)+binary)
report=dict(sourcePlayerSha256=hashlib.sha256(p.data).hexdigest(),sourceKitSha256=hashlib.sha256(s.data).hexdigest(),sourceRunDuration=duration,playerLegLengths={z:length(p,pb,z) for z in ('Left','Right')},kitLegLengths={z:length(s,sb,z) for z in ('Left','Right')},legScaleRatios=ratios,rootTranslationScale=ratio,restGlobalRotationDifferenceDegrees=rest_delta,staticPrefixBytes=len(p.bin),staticPrefixUnchanged=True,staticMetadataUnchanged=True,idleUnchanged=True,testOnlyBinding='retargeted Run replaces Walking only in this separate diagnostic GLB; no production binding',frames=globals_saved,unscaledProofSha256=hashlib.sha256(out.read_bytes()).hexdigest())
(OUT/'retarget-native-nlerp.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ('frames','restGlobalRotationDifferenceDegrees')}),flush=True)
