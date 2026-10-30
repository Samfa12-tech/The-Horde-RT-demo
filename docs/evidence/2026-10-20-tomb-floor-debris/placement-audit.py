import struct,json,hashlib,pathlib,math
root=pathlib.Path.cwd()
relative='assets/models/world/runtime/chapter-entry-return-v1/collapsed-entry-lod0.runtime.glb'
paths=[root/relative,root/'assets/models/world/source/chapter-entry-return-r03/runtime-candidate/collapsed-entry-lod0.runtime.glb']
b=paths[0].read_bytes();jl=struct.unpack_from('<I',b,12)[0];doc=json.loads(b[20:20+jl]);off=20+jl;bn=struct.unpack_from('<I',b,off)[0];binary=b[off+8:off+8+bn]
def accessor(i):
 a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];n={'SCALAR':1,'VEC3':3}[a['type']];fmt='<'+{5126:'f',5125:'I',5123:'H'}[a['componentType']]*n;size=struct.calcsize(fmt);o=v.get('byteOffset',0)+a.get('byteOffset',0);return [struct.unpack_from(fmt,binary,o+k*v.get('byteStride',size)) for k in range(a['count'])]
triangles=[]
for p in doc['meshes'][0]['primitives']:
 positions=accessor(p['attributes']['POSITION']);indices=[x[0] for x in accessor(p['indices'])]
 triangles.extend((positions[indices[k]],positions[indices[k+1]],positions[indices[k+2]],p['material']) for k in range(0,len(indices),3))
def sub(a,b):return tuple(a[i]-b[i] for i in range(3))
def dot(a,b):return sum(a[i]*b[i] for i in range(3))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def hit(origin,direction,t):
 a,b,c,_=t;e1=sub(b,a);e2=sub(c,a);p=cross(direction,e2);det=dot(e1,p)
 if abs(det)<1e-12:return None
 s=sub(origin,a);u=dot(s,p)/det
 if u< -1e-9 or u>1+1e-9:return None
 q=cross(s,e1);v=dot(direction,q)/det
 if v< -1e-9 or u+v>1+1e-9:return None
 distance=dot(e2,q)/det
 return distance if distance>1e-6 else None
def nearest(origin,target,geometry=triangles):
 direction=sub(target,origin);found=[]
 for i,t in enumerate(geometry):
  fraction=hit(origin,direction,t)
  if fraction is not None and fraction<1-1e-5:found.append((fraction,i))
 if not found:return None
 f,i=min(found);return {'fraction':f,'triangle':i,'point':[origin[k]+f*direction[k] for k in range(3)],'material':geometry[i][3]}
audit=json.loads((root/'assets/models/world/source/chapter-entry-return-r03/authoring-audit.json').read_text())
parts=[]
for part in audit['geometry']:
 if part['name'] not in ('Supported fractured roof shoulder right','fallen-right'):continue
 lo,hi=part['boundsHorde'];contained=[t for t in triangles if all(lo[i]-1e-5<=v[i]<=hi[i]+1e-5 for v in t[:3] for i in range(3))];vertices=[v for t in contained for v in t[:3]]
 parts.append({'name':part['name'],'sourceObjectTriangleCount':part['triangles'],'runtimeBoundsContainedTriangleCount':len(contained),'runtimeActualBounds':[[min(v[i] for v in vertices) for i in range(3)],[max(v[i] for v in vertices) for i in range(3)]],'limit':'Runtime is merged by material; contained triangles include other overlapping source pieces. This is a geometric bounds check, not a named-object triangle inventory.'})
source=root/'src/vulkan/raytracing/PresentableTinyRtScene.cpp';text=source.read_text();needle='addWorldQuad({{-1.92f, kRouteFloorWorldY, 17.48f}}, {{1.92f, kRouteFloorWorldY, 17.48f}},\n                 {{1.92f, kRouteFloorWorldY, 3.4f}}, {{-1.92f, kRouteFloorWorldY, 3.4f}}, SurfaceWetCobble, SurfaceUp);'
assert needle in text.replace('\r\n','\n')
# Existing ordinary opening floor is constructed with the same floor authority.
assert 'addWorldQuad' in text
floorTriangles=[((-1.85,-.95,-6.4),(1.85,-.95,-6.4),(1.85,-.95,3.4),-1),((-1.85,-.95,-6.4),(1.85,-.95,3.4),(-1.85,-.95,3.4),-1),((-1.92,-.95,3.4),(1.92,-.95,3.4),(1.92,-.95,17.48),-1),((-1.92,-.95,3.4),(1.92,-.95,17.48),(-1.92,-.95,17.48),-1)]
spawn=(0,.70,1.85);camera=(.9,.70,1.30)
rejected=[{'target':[x,-.92,z],'spawnBlocker':nearest(spawn,(x,-.92,z)),'rightFrontBlocker':nearest((1.3,.70,2.8),(x,-.92,z))} for x in (1.63,1.715,1.80) for z in (3.45,3.65,3.85)]
revised=[]
for x in (1.63,1.715,1.80):
 for z in (3.15,3.25,3.35):
  for y in (-.93,-.89):
   p=(x,y,z);floorHit=nearest((x,-.88,z),(x,-1.1,z),floorTriangles)
   revised.append({'target':p,'spawnBlocker':nearest(spawn,p),'diagnosticCameraBlocker':nearest(camera,p),'runtimeFloorToTopBlocker':nearest((x,-.949,z),(x,-.88,z)),'proceduralFloorHit':floorHit})
centre=(1.715,-.92,3.25);delta=sub(centre,camera);yaw=math.atan2(delta[0],-delta[2]);horizontal=math.hypot(delta[0],delta[2]);depression=math.atan2(-delta[1],horizontal);pitch=-.32;cameraDown=math.atan(.05-pitch);verticalScreen=-1.22*math.tan(depression-cameraDown);uvY=.5*(1-verticalScreen/.74)
report={'schema':1,'evidenceClass':'Read-only CPU parsing/ray probes of current runtime GLB and exact production floor construction; not GPU capture, lighting, gameplay contact or device acceptance','files':[{'path':str(p.relative_to(root)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size} for p in paths],'runtimeTriangles':len(triangles),'runtimeNodes':doc['nodes'],'boundsChecks':parts,'rejectedEnvelope':{'min':[1.63,-.95,3.45],'max':[1.80,-.89,3.85],'samples':rejected,'result':'REJECTED: all sampled low points occluded by existing opaque masonry'},'blockingTriangle6892':triangles[6892],'revisedEnvelope':{'min':[1.63,-.95,3.15],'max':[1.80,-.89,3.35],'samples':revised,'all36ViewSegmentsClear':all(x['spawnBlocker'] is None and x['diagnosticCameraBlocker'] is None for x in revised),'floorConstructionSource':str(source.relative_to(root)),'floorWorldY':-.95,'floorSourceHash':hashlib.sha256(source.read_bytes()).hexdigest(),'collisionPolicy':'Cosmetic only; no collision, stepping, kicking, save or physics state. Clears spawn/central route, not every possible perimeter foot position.'},'diagnosticCamera':{'x':camera[0],'z':camera[2],'supportY':-.95,'yaw':yaw,'pitch':pitch,'timeSeconds':0,'eyeY':camera[1],'centreTarget':centre,'predictedCentreUvYAtOrdinaryShaderProjection':uvY,'contract':'Render-only coherent rebase; loaded simulation/spawn unchanged. Existing lights/items move coherently with diagnostic camera; no added lights, fog/quality/FOV change. CPU GLB occlusion only, actual readability still requires paired native RT.'}}
out=root/'reports/floor-chip-runtime-audit-2026-10-20.json'
with out.open('x',encoding='utf-8') as f:json.dump(report,f,indent=2);f.write('\n')
print(json.dumps({'report':str(out),'runtimeHash':report['files'][0]['sha256'],'allRejectedBlocked':all(x['spawnBlocker'] and x['rightFrontBlocker'] for x in rejected),'revised18TargetsBothViewsClear':report['revisedEnvelope']['all36ViewSegmentsClear'],'revisedFloorToTopClear':all(x['runtimeFloorToTopBlocker'] is None for x in revised),'camera':report['diagnosticCamera']},indent=2))
