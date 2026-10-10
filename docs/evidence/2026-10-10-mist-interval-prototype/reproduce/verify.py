"""Independent CPU segment/triangle checks and actual hardware interval readback.
Predeclared tolerances: discovered union length error <=0.3mm + 0.01% of
union length; surface-only RGBA equality exact. These are diagnostic geometric
criteria, not owner visual acceptance or a new production test tolerance.
"""
import json,math,struct,hashlib,time,re
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
root=Path.cwd();out=root/'reports/mist-interval-prototype';public=root/'docs/evidence/2026-10-10-mist-interval-prototype'
shader_rows={r['name']:r for r in json.loads((out/'shader-metrics.json').read_text())}
exe_hash=hashlib.sha256((out/'build/Debug/frozen_mist.exe').read_bytes()).hexdigest()
def planes(triangle,light):
    a,b,c=map(np.array,triangle);light=np.array(light);n=np.cross(b-a,c-a)
    if np.dot(n,light-a)<0:n=-n
    p=[(-n,a)]
    for x,y,z in [(a,b,c),(b,c,a),(c,a,b)]:
        n=np.cross(x-light,y-light)
        if np.dot(n,z-light)<0:n=-n
        p.append((n,light))
    return p
def clip(origin,direction,first,last,pp):
    for n,b in pp:
        a=np.dot(n,origin-b);v=np.dot(n,direction)
        if abs(v)<1e-12:
            if a<0:return None
        elif v>0:first=max(first,-a/v)
        else:last=min(last,-a/v)
        if last<=first:return None
    return [first,last]
def union(intervals):
    result=[]
    for first,last in sorted(intervals):
        if result and first<=result[-1][1]:result[-1][1]=max(result[-1][1],last)
        else:result.append([first,last])
    return result
def length(intervals):return sum(b-a for a,b in intervals)
def hit(origin,point,triangle):
    # Independent finite Moller-Trumbore segment intersection, no cone planes.
    a,b,c=map(np.array,triangle);d=point-origin;e1=b-a;e2=c-a;p=np.cross(d,e2);det=np.dot(e1,p)
    if abs(det)<1e-12:return False
    v=origin-a;u=np.dot(v,p)/det;q=np.cross(v,e1);w=np.dot(d,q)/det;t=np.dot(e2,q)/det
    return 0<=u<=1 and 0<=w and u+w<=1 and 0<t<1
def case(name,triangles,o,d,light,end=10):
    o=np.array(o);d=np.array(d);light=np.array(light);intervals=union([v for t in triangles if (v:=clip(o,d,0,end,planes(t,light)))])
    for x in np.linspace(.0001,end-.0001,1024):
        if any(abs(x-a)<1e-8 or abs(x-b)<1e-8 for a,b in intervals):continue
        analytic=any(a<x<b for a,b in intervals)
        exact=any(hit(light,o+d*x,t) for t in triangles)
        assert analytic==exact,(name,x,intervals)
    return dict(name=name,intervals=intervals,blocked_metres=length(intervals),independent_samples=1024)
# Thin interior shadow with clear endpoints and midpoint. Double and disjoint
# blockers exercise union, while transformed geometry prevents rope-only math.
triangle=[[2.09,2,-.03],[2.11,2,-.03],[2.10,2,.03]]
o=[0,0,0];d=[1,0,0];light=[0,5,0];tests=[]
tests.append(case('thin interior clear endpoints and midpoint',[triangle],o,d,light))
assert tests[-1]['blocked_metres']>0
assert not any(hit(np.array(light),np.array(o)+np.array(d)*x,triangle) for x in [0,5,10])
tests.append(case('overlap is not double darkening',[triangle,triangle],o,d,light))
assert tests[-1]['blocked_metres']==tests[0]['blocked_metres']
other=(np.array(triangle)+[2,0,0]).tolist()
tests.append(case('disjoint intervals',[triangle,other],o,d,light));assert len(tests[-1]['intervals'])==2
rotation=np.array([[0,0,1],[0,1,0],[-1,0,0]])
transform=lambda p:(rotation@np.array(p)+[7,-2,3]).tolist()
tests.append(case('transformed non-rope triangle',[list(map(transform,triangle))],transform(o),(rotation@d).tolist(),transform(light)))
tests.append(case('blocker behind light does not shadow',[triangle],o,d,[0,1,0]))
assert tests[-1]['blocked_metres']==0
# Algorithm continuity subset. This is NOT a native moving-rope or dynamic-AS
# test. A thin triangle moves across the boundary between fixed density cells;
# independent segment hits still validate each geometry, and interval overlap
# must transfer continuously rather than assigning a whole cell at once.
movement=[]
for shift in np.linspace(.5,1.5,101):
    moving=np.array(triangle)+[shift,0,0]
    intervals=union([clip(np.array(o),np.array(d),0,10,planes(moving,light))])
    for t in np.linspace(.001,9.999,129):
        assert any(a<t<b for a,b in intervals)==hit(np.array(light),np.array(o)+np.array(d)*t,moving)
    cells=[sum(max(0,min((i+1)*10/6,b)-max(i*10/6,a)) for a,b in intervals)/(10/6) for i in range(6)]
    movement.append(dict(translation=float(shift),intervals=intervals,cell_coverage=cells))
maximum_cell_change=max(abs(b-a) for p,q in zip(movement,movement[1:]) for a,b in zip(p['cell_coverage'],q['cell_coverage']))
assert maximum_cell_change<.0101,(maximum_cell_change,'discontinuous cell transfer')
write=lambda p,s:Path(p).write_text(s,encoding='utf-8',newline='\n')
write(out/'analytic-tests.json',json.dumps(tests,indent=2)+'\n')

def captured(quality,name,yaw=None):
    prefix=quality+'-'+name
    if yaw is not None:prefix+='-yaw'+str(yaw)
    paths=[p for p in (out/'captures').glob(prefix+'*') if re.fullmatch(re.escape(prefix)+r'(?:-\d+)?',p.name) and (p/'completed-frame.json').exists()]
    path=max(paths,key=lambda p:(p/'completed-frame.json').stat().st_mtime)
    assert (path/'completed-frame.json').exists(),path
    frame=json.loads((path/'completed-frame.json').read_text());assert frame['presentation']['presented']
    run=json.loads((path/'run.json').read_text())
    assert run['exe_sha256']==exe_hash,(path,'stale executable')
    for key in ['source_sha256','spirv_sha256','include_sha256']:
        assert run['shader'][key]==shader_rows[name][key],(path,'stale shader',key)
    assert frame['pipeline']['activeSha256']==shader_rows[name]['spirv_sha256'],path
    assert frame['pipeline']['executionBackend']=='RayTracingPipeline',path
    assert frame['shadowQuality']['mode']==quality,path
    return path,Image.open(path/'192-rescue-journey-start.png').convert('RGBA'),frame
def unpack(image):return np.array([v[0] for v in struct.iter_unpack('<f',image.tobytes('raw','BGRA'))]).reshape(image.height,image.width)
def smooth(a,b,x):v=max(0,min(1,(x-a)/(b-a)));return v*v*(3-2*v)
def density(p,time):
    x,y,z=p;floor=max(0,min(1.15,y+.95))
    ca=math.sin(x*1.32+z*.86+time*.19+math.sin(z*.71-time*.11)*.72)
    cb=math.sin(x*-.63+z*1.57-time*.14+math.sin(x*.84+time*.09)*.58)
    flow=max(.12,min(1,.54+ca*.23+cb*.19));focus=1-smooth(1.25,3.05,math.hypot(x+32.2,z+13.1))
    return (.080+focus*.27)*math.exp(-floor*3.25)*(1-smooth(.82,1.12,floor))*flow
def weighted(o,d,first,last,intervals,time,n,continuous=False):
    step=(last-first)/n;T=1;total=loss=0
    for i in range(n):
        centre=first+(i+.5)*step;rho=density(o+d*centre,time);weight=T*rho*.82*step
        if continuous:coverage=float(any(a<=centre<=b for a,b in intervals))
        else:coverage=sum(max(0,min(first+(i+1)*step,b)-max(first+i*step,a)) for a,b in intervals)/step
        total+=weight;loss+=weight*coverage;T*=math.exp(-rho*step)
    return loss/total if total>0 else -1
result=dict(analytic_tests=tests,moving_triangle_subset=dict(cases=101,independent_segment_tests=101*129,
    maximum_adjacent_cell_coverage_change=maximum_cell_change,native_dynamic_rope_verified=False,rows=movement),qualities=[])
for quality in ['current','higher']:
    basepath,base,bf=captured(quality,'baseline')
    ip,corrected,cf=captured(quality,'diagnostic_high_generic_dielectric-interval')
    dp,depth,_=captured(quality,'depth');cp,coverage,_=captured(quality,'interval-coverage')
    sp,bs,_=captured(quality,'baseline-surface');sp2,cs,_=captured(quality,'interval-surface')
    assert bs.tobytes()==cs.tobytes(),quality+' surface changed'
    state=json.loads((basepath/'frozen-state.json').read_text())
    assert (ip/'frozen-state.json').read_bytes()==(basepath/'frozen-state.json').read_bytes()
    origin=np.array(state['camera'][:3]);yaw,pitch=state['camera'][3:];W,H=base.size
    def norm(v):return v/np.linalg.norm(v)
    forward=norm(np.array([math.sin(yaw),-.05+pitch,-math.cos(yaw)]));right=norm(np.cross(forward,[0,1,0]));up=norm(np.cross(right,forward))
    def ray(x,y):return norm(forward*1.22+right*(((x+.5)/W*2-1)*W/H)+up*(((y+.5)/H*2-1)*-.74))
    dd=unpack(depth);cc=unpack(coverage)
    wp,weights,_=captured(quality,'weighted-coverage');ww=unpack(weights)
    np_,negative,_=captured(quality,'overflow-negative');nn=unpack(negative)
    for p in [ip,dp,cp,sp,sp2,wp,np_]:
        assert (p/'frozen-state.json').read_bytes()==(basepath/'frozen-state.json').read_bytes(),(p,'unmatched frozen state')
    assert np.count_nonzero(nn==-2)>0,quality+' overflow negative did not reject'
    assert np.all(np.isfinite(dd)) and np.all(np.isfinite(cc)),quality
    assert np.all(cc>=0),quality+' overflow/invalid interval'
    mesh=np.array(state['triangles']).reshape(-1,3,3)
    pp=[list(map(lambda t:planes(t,l),mesh)) for l in [[-34.35,2.76,-16.02],[-33.05,2.76,-14.38]]]
    points=[(838,574),(915,513),(850,556)]
    candidates=np.argwhere(cc>0)
    points+= [(int(v[1]),int(v[0])) for v in candidates[::max(1,len(candidates)//256)]]
    points+= [(x,y) for x in range(640,1180,35) for y in range(410,770,30)]
    rows=[];start=time.perf_counter()
    for x,y in points:
        d=ray(x,y);first,last=0,float(dd[y,x])
        for axis,(a,b) in enumerate(zip([-36.65,-.95,-18.15],[-30.65,.20,-12.25])):
            if abs(d[axis])<1e-12:
                if not a<=origin[axis]<=b:last=-1
            else:
                p,q=(a-origin[axis])/d[axis],(b-origin[axis])/d[axis];first=max(first,min(p,q));last=min(last,max(p,q))
        source=0 if quality=='higher' else ((x+y)&1)
        intervals=union([v for p in pp[source] if (v:=clip(origin,d,first,last,p))]) if last>first else []
        expected=length(intervals);actual=float(cc[y,x]);error=abs(actual-expected)
        tolerance=.0003+.0001*expected
        rows.append(dict(pixel=[x,y],aperture=source,actual_primary_depth=float(dd[y,x]),mist_t=[first,last],intervals=intervals,expected_length=expected,gpu_length=actual,error=error,tolerance=tolerance,pass_=bool(error<=tolerance)))
    assert all(r['pass_'] for r in rows),(quality,[r for r in rows if not r['pass_']][:3])
    witnesses=[]
    for r in rows[:3]:
        x,y=r['pixel'];d=ray(x,y);first,last=r['mist_t'];intervals=r['intervals']
        expected=weighted(origin,d,first,last,intervals,state['walkTime'],6)
        actual=float(ww[y,x]);dense=weighted(origin,d,first,last,intervals,state['walkTime'],4096,True)
        assert math.isfinite(actual) and abs(actual-expected)<.00002,(quality,x,y,actual,expected)
        assert expected>.00001 and actual>0,(quality,x,y,'nonzero shadow witness disappeared')
        witnesses.append(dict(pixel=[x,y],gpu_weighted_fraction=actual,cpu_cell_weighted_fraction=expected,dense4096_fraction=dense,cell_approximation_error_pp=100*(actual-dense)))
    diff=np.abs(np.array(base,dtype=int)-np.array(corrected,dtype=int))[:,:,:3]
    row=dict(quality=quality,exact_surface_rgba_equal=True,full_frame_changed_pixels=int(np.count_nonzero(np.any(diff,axis=2))),maximum_channel_delta=int(diff.max()),interval_pixels=int(np.count_nonzero(cc>0)),overflow_pixels=int(np.count_nonzero(cc<0)),negative_overflow_pixels=int(np.count_nonzero(nn==-2)),comparison_rays=len(rows),cpu_verification_seconds=time.perf_counter()-start,maximum_length_error=max(r['error'] for r in rows),gpu_ms=dict(baseline=bf['gpu']['wholeRtGpuMs'],prototype=cf['gpu']['wholeRtGpuMs']),weighted_witnesses=witnesses,rows=rows)
    result['qualities'].append(row)
    # Game-only evidence, never a desktop screenshot. Labels distinguish modes.
    board=Image.new('RGB',(W*2,H+40),(15,15,15));board.paste(base.convert('RGB'),(0,40));board.paste(corrected.convert('RGB'),(W,40))
    draw=ImageDraw.Draw(board);draw.text((12,12),quality.upper()+' | CURRENT TOMB BASELINE',fill='white');draw.text((W+12,12),'OPAQUE ROPE INTERVAL DIAGNOSTIC — NOT ADMITTED',fill='white')
    board.save(out/(quality+'-comparison.png'))
nearby=[]
for yaw in [-.025,.025]:
    for quality in ['current','higher']:
        bp,b,bf=captured(quality,'baseline',yaw)
        ip,i,cf=captured(quality,'diagnostic_high_generic_dielectric-interval',yaw)
        assert (bp/'frozen-state.json').read_bytes()==(ip/'frozen-state.json').read_bytes(),(quality,yaw)
        state=json.loads((bp/'frozen-state.json').read_text())
        assert abs(state['camera'][3]-(.1+yaw))<1e-6,state['camera']
        assert state['triangles']==json.loads((basepath/'frozen-state.json').read_text())['triangles']
        delta=np.abs(np.array(b,dtype=int)-np.array(i,dtype=int))[:,:,:3]
        nearby.append(dict(quality=quality,yaw_delta=yaw,exact_frozen_pair=True,
            unchanged_rope_triangles=True,presented=True,changed_pixels=int(np.count_nonzero(np.any(delta,axis=2))),
            gpu_ms=dict(baseline=bf['gpu']['wholeRtGpuMs'],prototype=cf['gpu']['wholeRtGpuMs'])))
result['nearby_view_subset']=nearby
write(out/'correctness.json',json.dumps(result,indent=2)+'\n')
print(json.dumps({q['quality']:{k:v for k,v in q.items() if k!='rows'} for q in result['qualities']},indent=2))
