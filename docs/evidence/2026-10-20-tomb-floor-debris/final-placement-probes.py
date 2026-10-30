import struct,json,pathlib,hashlib,re,math,collections
root=pathlib.Path.cwd()
files={k:root/v for k,v in {
 'recipe':'src/scene/TombFloorDebris.h','runtime':'assets/models/world/runtime/chapter-entry-return-v1/collapsed-entry-lod0.runtime.glb',
 'camera':'src/platform/windows/WindowsEnvironmentSurveyLaunch.h','floor':'src/vulkan/raytracing/PresentableTinyRtScene.cpp','route':'src/gameplay/ShowcaseRoute.h','projection':'shaders/raytracing/include/rt_frame.glsl'}.items()}
raw={k:p.read_bytes() for k,p in files.items()};source=raw['recipe'].decode('utf-8-sig')
number=r'[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?'
def extract(block,width):
 rows=[]
 for row in re.findall(r'\{([^{}]+)\}',block):
  values=[float(v) for v in re.findall(number,row)]
  if len(values)==width:rows.append(values)
 return rows
specs=extract(source.split('kTombFloorFragments{{',1)[1].split('}};',1)[0],6)
shape=extract(source.split('shape{{',1)[1].split('}};',1)[0],3)
assert len(specs)==6 and len(shape)==8
assert 'floor=gameplay::kRouteFloorWorldY-.001f' in source
assert 'Quad(vertices,indices,p[4],p[5],p[6],p[7],inside);' in source
assert 'floorDebris' not in raw['route'].decode('utf-8-sig')
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
add=lambda x,y:f32(f32(x)+f32(y));mul=lambda x,y:f32(f32(x)*f32(y))
floorY=f32(-.95);baseY=add(floorY,-.001)
positions=[];chipTriangles=[]
quads=[(0,1,2,3),(4,5,6,7)]+[(i,(i+1)%4,4+(i+1)%4,4+i) for i in range(4)]
for stone,spec in enumerate(specs):
 x,z,w,l,h,yaw=map(f32,spec);c=f32(math.cos(yaw));s=f32(math.sin(yaw));p=[]
 for a,b,d in shape:
  xx=mul(a,w);zz=mul(d,l)
  p.append((add(add(x,mul(c,xx)),mul(s,zz)),add(baseY,mul(b,h)),add(add(z,-mul(s,xx)),mul(c,zz))))
 positions.append(p)
 for face,quad in enumerate(quads):
  for triangle,ind in enumerate(((quad[0],quad[1],quad[2]),(quad[0],quad[2],quad[3]))):
   chipTriangles.append({'v':tuple(p[i] for i in ind),'kind':'chip','stone':stone,'face':'bottom' if face==0 else 'top' if face==1 else 'side','faceIndex':face,'triangleInFace':triangle})
b=raw['runtime'];n=struct.unpack_from('<I',b,12)[0];gl=json.loads(b[20:20+n]);o=20+n;length=struct.unpack_from('<I',b,o)[0];binary=b[o+8:o+8+length]
assert len(gl['nodes'])==1 and all(key not in gl['nodes'][0] for key in ('matrix','rotation','translation','scale'))
def accessor(i):
 a=gl['accessors'][i];v=gl['bufferViews'][a['bufferView']];count={'SCALAR':1,'VEC3':3}[a['type']];fmt='<'+{5126:'f',5125:'I',5123:'H'}[a['componentType']]*count;size=struct.calcsize(fmt);off=v.get('byteOffset',0)+a.get('byteOffset',0);return [struct.unpack_from(fmt,binary,off+k*v.get('byteStride',size)) for k in range(a['count'])]
existing=[]
for primitive in gl['meshes'][0]['primitives']:
 P=accessor(primitive['attributes']['POSITION']);I=[x[0] for x in accessor(primitive['indices'])]
 existing += [{'v':tuple(P[I[k+i]] for i in range(3)),'kind':'existing-runtime','runtimeTriangle':len(existing)+k//3,'material':gl['materials'][primitive['material']].get('name',str(primitive['material']))} for k in range(0,len(I),3)]
def sub(a,b):return tuple(a[i]-b[i] for i in range(3))
def dot(a,b):return sum(a[i]*b[i] for i in range(3))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a):
 n=math.sqrt(dot(a,a));return tuple(x/n for x in a)
def hit(origin,direction,t):
 a,b,c=t['v'];e1=sub(b,a);e2=sub(c,a);p=cross(direction,e2);det=dot(e1,p)
 if abs(det)<1e-12:return None
 q=sub(origin,a);u=dot(q,p)/det
 if u< -1e-9 or u>1+1e-9:return None
 q2=cross(q,e1);v=dot(direction,q2)/det
 if v< -1e-9 or u+v>1+1e-9:return None
 fraction=dot(e2,q2)/det
 return fraction if fraction>1e-6 else None
def nearest(origin,target,triangles):
 delta=sub(target,origin);matches=[]
 for i,t in enumerate(triangles):
  fraction=hit(origin,delta,t)
  if fraction is not None and fraction<1-1e-5:matches.append((fraction,i))
 if not matches:return None
 fraction,i=min(matches);t=triangles[i]
 return {**{k:v for k,v in t.items() if k!='v'},'fraction':fraction,'point':[origin[k]+fraction*delta[k] for k in range(3)]}
assert 'gameplay::kRouteFloorWorldY,0.f,-.15f}' in raw['camera'].decode('utf-8-sig')
assert 'EnvironmentSurveyCamera{.9f,1.3f,' in raw['camera'].decode('utf-8-sig')
assert 'gameplay::kRouteFloorWorldY,2.74571f,-.32f}' in raw['camera'].decode('utf-8-sig')
cameras=[{'name':'ordinary-spawn','origin':(0,.70,1.85),'yaw':0.,'pitch':-.15}, {'name':'actual-debris-diagnostic','origin':(.9,.70,1.3),'yaw':f32(2.74571),'pitch':f32(-.32)}]
projection=raw['projection'].decode('utf-8-sig')
for literal in ('forward * 1.22 + right * screen.x + up * screen.y','screen.y *= -0.74','-0.05 + pitch'):assert literal in projection
aspect=960/540
# Straight native shader projection with no FOV changes. Visibility excludes rig/props outside this GLB.
def project(point,camera):
 forward=unit((math.sin(camera['yaw']),-.05+camera['pitch'],-math.cos(camera['yaw'])));right=unit(cross(forward,(0,1,0)));up=unit(cross(right,forward));d=sub(point,camera['origin']);depth=dot(d,forward)
 if depth<=0:return {'inFront':False,'inFrame':False,'cameraForwardDepth':depth,'uv':None}
 uv=(.5*(1+1.22*dot(d,right)/depth/aspect),.5*(1-1.22*dot(d,up)/depth/.74))
 return {'inFront':True,'inFrame':all(0<=v<=1 for v in uv),'cameraForwardDepth':depth,'uv':uv}
samples=[]
for stone,p in enumerate(positions):
 points=[('top-vertex-'+str(i),p[i]) for i in range(4,8)]
 for index,ids in enumerate(((4,5,6),(4,6,7))):points.append(('top-triangle-centroid-'+str(index),tuple(sum(p[i][axis] for i in ids)/3 for axis in range(3))))
 for name,point in points:
  views=[]
  for camera in cameras:
   existingHit=nearest(camera['origin'],point,existing);chipsHit=nearest(camera['origin'],point,chipTriangles)
   hits=[x for x in (existingHit,chipsHit) if x is not None];first=min(hits,key=lambda x:x['fraction']) if hits else None
   classification='clear' if first is None else 'existing-masonry' if first['kind']=='existing-runtime' else 'self-chip' if first['stone']==stone else 'other-chip'
   views.append({'camera':camera['name'],'projection':project(point,camera),'existingRuntimeBlocker':existingHit,'chipBlocker':chipsHit,'firstBlockerClass':classification,'firstBlocker':first})
  samples.append({'stone':stone,'sample':name,'point':point,'views':views})
# Verify the production floor literal rather than inventing support from the GLB, whose inspection floor is excluded.
floorSource=raw['floor'].decode('utf-8-sig');floorLiteral='addWorldQuad({{-1.85f, kRouteFloorWorldY, 3.4f}}, {{1.85f, kRouteFloorWorldY, 3.4f}}, {{1.85f, kRouteFloorWorldY, -6.4f}}, {{-1.85f, kRouteFloorWorldY, -6.4f}}, SurfaceWetCobble, SurfaceUp);';assert floorLiteral in floorSource
vertical=[]
for stone,p in enumerate(positions):
 queries=[('base-corner-'+str(i),(p[i][0],p[i][2])) for i in range(4)]+[('centre',(specs[stone][0],specs[stone][1]))]
 for name,(x,z) in queries:
  origin=(x,floorY+.0001,z);target=(x,floorY+.12,z)
  others=[t for t in chipTriangles if t['stone']!=stone]
  vertical.append({'stone':stone,'sample':name,'xz':[x,z],'withinActualOrdinaryFloorQuad':-1.85<=x<=1.85 and -6.4<=z<=3.4,'floorY':floorY,'existingRuntimeLowVerticalBlocker':nearest(origin,target,existing),'otherChipLowVerticalBlocker':nearest(origin,target,others)})
stoneRows=[]
for stone,p in enumerate(positions):
 bounds=[[min(v[i] for v in p) for i in range(3)],[max(v[i] for v in p) for i in range(3)]]
 stoneRows.append({'stone':stone,'spec':specs[stone],'actualVertices':p,'bounds':bounds,'bottomOverlapMetres':floorY-bounds[0][1],'topHeightAboveFloorMetres':bounds[1][1]-floorY})
summary={}
for camera in cameras:
 v=[view for s in samples for view in s['views'] if view['camera']==camera['name']]
 summary[camera['name']]={'sampleCount':len(v),'firstBlockerClasses':dict(collections.Counter(x['firstBlockerClass'] for x in v)),'existingRuntimeClear':sum(x['existingRuntimeBlocker'] is None for x in v),'inFrame':sum(x['projection']['inFrame'] for x in v),'clearAndInFrame':sum(x['firstBlockerClass']=='clear' and x['projection']['inFrame'] for x in v)}
allP=[p for group in positions for p in group]
report={'schema':1,'evidenceClass':'One final offline CPU vertex/centroid probe against finished recipe and current native runtime GLB; not a game run, GPU lighting/readability, contact, device or owner acceptance','files':{k:{'path':str(files[k].relative_to(root)),'sha256':hashlib.sha256(v).hexdigest()} for k,v in raw.items()},'runtimeTriangleCount':len(existing),'generatedChipTriangles':len(chipTriangles),'recipeEvaluation':'Float32 emulation of stored specs, arithmetic and trig results; intersection uses doubles and ignores last 1e-5 segment fraction to avoid endpoint self hits. This is not a native C++ execution.','cameras':cameras,'projectionExtent':[960,540],'actualCombinedBounds':[[min(p[i] for p in allP) for i in range(3)],[max(p[i] for p in allP) for i in range(3)]],'stones':stoneRows,'topSamples':samples,'topSummary':summary,'lowVerticalQueries':vertical,'floorSourceLiteral':floorLiteral,'lowVerticalSummary':{'count':len(vertical),'allWithinActualFloorQuad':all(x['withinActualOrdinaryFloorQuad'] for x in vertical),'existingRuntimeBlockers':sum(x['existingRuntimeLowVerticalBlocker'] is not None for x in vertical),'otherChipBlockers':sum(x['otherChipLowVerticalBlocker'] is not None for x in vertical)},'limits':['Ordinary spawn has yaw zero and faces away from the chips: clear geometric segments do not mean chips appear in its unchanged view.','Loaded player/held-item meshes and procedural scene geometry other than the verified floor are not included in this GLB probe. Native RT must inspect their visibility, light, shadows and material readability.','Cosmetic only; unchanged collision and no stone contact, kicking, simulation or save state.','Rejected pocket and earlier reports remain unchanged.']}
assert all(files[k].read_bytes()==v for k,v in raw.items()),'Input changed during frozen offline audit.'
output=root/'reports/floor-chip-final-vertex-probes-2026-10-20.json'
with output.open('x',encoding='utf-8') as f:json.dump(report,f,indent=2);f.write('\n')
print(json.dumps({'report':str(output),'bounds':report['actualCombinedBounds'],'topSummary':summary,'vertical':report['lowVerticalSummary']},indent=2))
