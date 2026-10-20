from pathlib import Path
import math,json,hashlib,struct
root=Path.cwd();out=root/'reports/arm-admission-20261020-r1'
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def norm(a):
    n=math.sqrt(dot(a,a));return tuple(v/n for v in a)
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def loadobj(p):
    points=[];tris=[];matrix=None
    for s in p.read_text().splitlines():
        if s.startswith('# model_to_world_row_major_3x4 '):matrix=list(map(float,s.split()[2:]))
        elif s.startswith('v '):points.append(tuple(map(float,s.split()[1:])))
        elif s.startswith('f '):tris.append(tuple(int(x.split('/')[0])-1 for x in s.split()[1:]))
    assert len(matrix)==12 and all(len(t)==3 for t in tris)
    world=[tuple(sum(matrix[r*4+k]*v[k] for k in range(3))+matrix[r*4+3] for r in range(3)) for v in points]
    return points,world,tris,matrix

def analyze(p,pose):
    points,world,tris,matrix=loadobj(p)
    x,support,z,yaw,pitch=pose
    # rt_frame.glsl: stationary frozen walkAmount/time=0. The pose Y is support,
    # while PlayerEyeWorldY = .70 + (support - (-.95)); no crouch/rope offset here.
    eye=(x,.70+support+.95,z)
    f=norm((math.sin(yaw),max(-.32,min(.28,pitch))-.05,-math.cos(yaw)))
    r=norm(cross(f,(0,1,0)));u=norm(cross(r,f));aspect=960/540
    camera=[(dot(tuple(v[i]-eye[i] for i in range(3)),r),dot(tuple(v[i]-eye[i] for i in range(3)),u),dot(tuple(v[i]-eye[i] for i in range(3)),f)) for v in world]
    # Five homogeneous frustum planes derived from ray = f*1.22 + r*aspect*ndcX - u*.74*ndcY.
    planes=[lambda v:v[2]-1e-8,lambda v:aspect*v[2]/1.22+v[0],lambda v:aspect*v[2]/1.22-v[0],lambda v:.74*v[2]/1.22+v[1],lambda v:.74*v[2]/1.22-v[1]]
    def clip(poly,plane):
        result=[]
        for a,b in zip(poly,poly[1:]+poly[:1]):
            da,db=plane(a),plane(b)
            if da>=0:result.append(a)
            if (da>=0)!=(db>=0):
                t=da/(da-db);result.append(tuple(a[i]+t*(b[i]-a[i]) for i in range(3)))
        return result
    visible=[];screen=[];infront=0;horizontalScreen=[]
    for n,t in enumerate(tris):
        poly=[camera[i] for i in t]
        if max(v[2] for v in poly)>0:infront+=1
        horizontal=poly
        for plane in planes[:3]:
            horizontal=clip(horizontal,plane)
            if not horizontal:break
        horizontalScreen.extend(((.5+.5*v[0]*1.22/v[2]/aspect)*960,(.5-.5*v[1]*1.22/v[2]/.74)*540) for v in horizontal)
        for plane in planes:
            poly=clip(poly,plane)
            if not poly:break
        if poly:
            visible.append(n)
            screen.extend(((.5+.5*v[0]*1.22/v[2]/aspect)*960,(.5-.5*v[1]*1.22/v[2]/.74)*540) for v in poly)
    return {'file':str(p.relative_to(root)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'vertices':len(points),'triangles':len(tris),'transformRowMajor3x4':matrix,'poseSupportYawPitch':pose,'shaderEye':eye,'forward':f,'right':r,'up':u,'localBounds':[[min(v[i] for v in points) for i in range(3)],[max(v[i] for v in points) for i in range(3)]],'cameraBounds':[[min(v[i] for v in camera) for i in range(3)],[max(v[i] for v in camera) for i in range(3)]],'trianglesWithAnyPositiveForwardDepth':infront,'frustumIntersectingTriangles':len(visible),'horizontalClippedScreenYRange':[min(v[1] for v in horizontalScreen),max(v[1] for v in horizontalScreen)] if horizontalScreen else None,'clippedScreenBounds':[[min(v[i] for v in screen) for i in range(2)],[max(v[i] for v in screen) for i in range(2)]] if screen else None,'scope':'CPU exact uploaded triangles + captured float32 model transform, shader camera equations; no visibility/occlusion or GPU-hit claim'}
rows=[]
for label,stem in [('blocked-retreat','192-rescue-journey-start'),('rect-niche','218-chapter-niche-rect')]:
    base=root/'reports/tomb-composition-20261020-r1'/label
    recorded=json.loads((base/'failed-primary-pose.json').read_text())['camera']
    f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
    authored=[0,-.95,1.85,math.pi,-.05] if label=='blocked-retreat' else [-.4,-.95,-.7,-math.pi/2,-.32]
    pose=list(map(f32,authored))
    assert max(abs(a-b) for a,b in zip(recorded,pose))<.00001
    rows.append(analyze(base/(stem+'.obj'),pose))
    rows.append(analyze(base/(stem+'.player-world-body.obj'),pose))
control=root/'reports/arm-admission-20261020-r1/positive-forest'
manifest=json.loads((control/'capture-manifest.json').read_text(encoding='utf-8-sig'))
c=manifest['captures'][0];camera=c['camera'];support=c['visibility']['verticalProofPoseWitness']['supportWorldY']
positive=analyze(control/c['viewmodelGeometry']['file'],[camera['x'],support,camera['z'],camera['yaw'],camera['pitch']])
assert positive['frustumIntersectingTriangles']>0 and c['visibility']['primaryPixels']['player']>0
positive['nativePrimaryPlayerPixels']=c['visibility']['primaryPixels']['player']
positive['scope']+='; fresh current positive control; same executable as both predicted-negative captures'
rows.append(positive)
(out/'projection.json').write_text(json.dumps({'shader':'shaders/raytracing/include/rt_frame.glsl:29-43','assumptions':'960x540 unrotated, walkAmount=0, no crouch/rope offset; support Y converted to eye Y','numericMethod':'CPU double evaluation of shader equations using exact captured float32 model matrix and authored float32 camera inputs; diagnostic pose text independently agrees within 1e-5. Not GPU floating-point/occlusion simulation.','rows':rows},indent=2)+'\n')
for row in rows:print(Path(row['file']).name, 'triangles',row['triangles'],'positive-depth',row['trianglesWithAnyPositiveForwardDepth'],'frustum',row['frustumIntersectingTriangles'],'horizontalY',row['horizontalClippedScreenYRange'])
