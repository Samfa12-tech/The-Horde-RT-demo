"""Bounded CPU geometry oracle, not GPU/render acceptance or a production quality change."""
import json,math,struct
from pathlib import Path
from PIL import Image
root=Path.cwd();private=root/'reports/frozen-mist';public=root/'docs/evidence/2026-10-10-frozen-mist-attribution'
state=json.loads((private/'owner-view/captures/current-midpoint-all/frozen-state.json').read_text())
vertices=state['triangles'];triangles=[vertices[i:i+3] for i in range(0,len(vertices),3)]
W,H=1232,803;origin=state['camera'][:3];yaw,pitch=state['camera'][3:]
def sub(a,b):return [x-y for x,y in zip(a,b)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def mul(a,b):return [x*b for x in a]
def add(a,b):return [x+y for x,y in zip(a,b)]
def norm(a):return mul(a,1/math.sqrt(dot(a,a)))
f=norm([math.sin(yaw),-.05+pitch,-math.cos(yaw)]);r=norm(cross(f,[0,1,0]));u=norm(cross(r,f))
def ray(x,y):return norm(add(add(mul(f,1.22),mul(r,((x+.5)/W*2-1)*W/H)),mul(u,((y+.5)/H*2-1)*-.74)))
def medium(d):
 lo,hi=0,float('inf')
 for o,v,a,b in zip(origin,d,[-36.65,-.95,-18.15],[-30.65,.20,-12.25]):
  if abs(v)<1e-12:
   if not a<=o<=b:return None
  else:
   p,q=(a-o)/v,(b-o)/v;lo=max(lo,min(p,q));hi=min(hi,max(p,q))
 # Explicitly limited to selected clear-floor rays, checked against actual GPU sample masks below.
 if d[1]>=0:return None
 hi=min(hi,(-.95-origin[1])/d[1])
 return (lo,hi) if hi>lo+.01 else None
def planes(light):
 result=[]
 for a,b,c in triangles:
  n=cross(sub(b,a),sub(c,a))
  if dot(n,sub(light,a))<0:n=mul(n,-1)
  pp=[(mul(n,-1),a)]
  for e1,e2,other in [(a,b,c),(b,c,a),(c,a,b)]:
   n=cross(sub(e1,light),sub(e2,light))
   if dot(n,sub(other,light))<0:n=mul(n,-1)
   pp.append((n,light))
  result.append(pp)
 return result
def intervals(d,bounds,cones):
 result=[]
 for pp in cones:
  lo,hi=bounds
  for n,base in pp:
   intercept=dot(n,sub(origin,base));slope=dot(n,d)
   if abs(slope)<1e-15:
    if intercept<0:hi=lo-1;break
   elif slope>0:lo=max(lo,-intercept/slope)
   else:hi=min(hi,-intercept/slope)
   if hi<lo:break
  if hi>lo:result.append([lo,hi])
 merged=[]
 for a,b in sorted(result):
  if merged and a<=merged[-1][1]:merged[-1][1]=max(b,merged[-1][1])
  else:merged.append([a,b])
 return merged
def blocked(t,intervals):return any(a<=t<=b for a,b in intervals)
def smooth(a,b,x):v=max(0,min(1,(x-a)/(b-a)));return v*v*(3-2*v)
def density(p):
 x,y,z=p;time=state['walkTime'];floor=max(0,min(1.15,y+.95))
 ca=math.sin(x*1.32+z*.86+time*.19+math.sin(z*.71-time*.11)*.72)
 cb=math.sin(x*-.63+z*1.57-time*.14+math.sin(x*.84+time*.09)*.58)
 noise=max(.12,min(1,.54+ca*.23+cb*.19))
 focus=1-smooth(1.25,3.05,math.hypot(x+32.2,z+13.1))
 return (.080+focus*.27)*math.exp(-floor*3.25)*(1-smooth(.82,1.12,floor))*noise
def integrate(d,bounds,ints,n):
 lo,hi=bounds;ds=(hi-lo)/n;T=1;total=loss=0
 for i in range(n):
  t=lo+(i+.5)*ds;rho=density(add(origin,mul(d,t)));weight=T*rho*.82*ds
  total+=weight;loss+=weight*blocked(t,ints);T*=math.exp(-rho*ds)
 return loss/total if total>0 else -1
def floats(name):
 im=Image.open(private/'owner-midpoint-rope/captures'/name/'192-rescue-journey-start.png').convert('RGBA')
 return [v[0] for v in struct.iter_unpack('<f',im.tobytes('raw','BGRA'))]
rows=[]
for quality in ['current','higher']:
 mid=floats(quality+'-midpoint-rope-loss')
 mask=list(Image.open(private/'owner-weights/captures'/(quality+'-sample-local-masks')/'192-rescue-journey-start.png').convert('RGBA').getdata())
 # Four/eight representative pixels from each footprint, all away from hands and body.
 selections=[]
 for label,predicate in [('midpoint-band',lambda i:mid[i]>1e-6),('sample-local-continuation',lambda i:mask[i][2]>0)]:
  choices=[i for i in range(W*H) if 780<=i%W<=1050 and 450<=i//W<=730 and predicate(i)]
  assert choices
  selections.extend((label,choices[round((len(choices)-1)*j/7)]) for j in range(8))
 for label,i in selections:
  x,y=i%W,i//W;d=ray(x,y);bounds=medium(d);assert bounds
  index=0 if quality=='higher' else ((x+y)&1)
  light=[[-34.35,2.76,-16.02],[-33.05,2.76,-14.38]][index]
  ints=intervals(d,bounds,planes(light));lo,hi=bounds
  cpuMask=sum(1<<j for j in range(6) if blocked(lo+(j+.5)*(hi-lo)/6,ints))
  midBlocked=blocked((lo+hi)*.5,ints)
  verified=cpuMask==mask[i][2] and midBlocked==(mid[i]>1e-6)
  rows.append(dict(quality=quality,category=label,pixel=[x,y],aperture_index=index,medium_t=bounds,rope_shadow_intervals_t=ints,shadowed_length=sum(b-a for a,b in ints),midpoint_rope_gpu=mid[i],gpu_six_sample_rope_mask=mask[i][2],cpu_six_sample_rope_mask=cpuMask,discrete_hardware_agreement=verified,weighted_rope_fraction={str(n):integrate(d,bounds,ints,n) for n in [6,64,256,1024,4096]}))
result=dict(scope='CPU analytic shadow cones of the actual 176 frozen rope triangles, point aperture light, selected unobstructed-floor camera rays. Independent geometric explanation, not Vulkan presentation or a production sampling change. Per-ray agreement with GPU midpoint/six-sample masks explicitly controls validity.',density_time=state['walkTime'],accepted_rays=sum(r['discrete_hardware_agreement'] for r in rows),rejected_rays=sum(not r['discrete_hardware_agreement'] for r in rows),rows=rows)
(public/'continuous-rope-oracle.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(accepted=result['accepted_rays'],rejected=result['rejected_rays'],examples=[r for r in rows if r['discrete_hardware_agreement']][:4]),indent=2))
