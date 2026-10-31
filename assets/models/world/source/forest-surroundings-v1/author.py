"""Deterministic, source-only whole-route woodland authoring. No runtime admission.

Uses standard Python, original project-owned centreline data/materials and the
actual imported terrain. --write creates/updates this draft's fixed output roster;
--check regenerates in memory and rejects changed/missing bytes. Original files,
shipping rosters, renderer capacities and gameplay collision are never written.
"""
import argparse
from collections import Counter, defaultdict
import copy
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import re
import struct

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
TREE = ROOT / 'assets/models/world/runtime/forest-tree-pair-v1'
QA = ROOT / 'assets/models/world/source/forest-tree-pair-v1/qa'
MAP = ROOT / 'docs/references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json'
PLACEMENTS = ROOT / 'assets/models/world/source/forest-placement-v1/woodland-placements-v3.json'
TERRAIN = ROOT / 'assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb'
SEED = 20261031
RIGHTS = ('Original project-authored geometry, derived from the owned Horde tree '
          'centreline records and original PBR maps. No provider, purchase or '
          'third-party geometry. Existing rights remain; no blanket MIT/CC0 grant.')
SOURCES = {'pine': ('horde-irregular-pine-v1-lod1', 'db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f'),
           'alder': ('horde-upright-alder-v1-lod1', 'f545d7e08370b5e451597c3fd903d8d93b6703d11c75039181a04f7f5312e0bb')}
TRACE_SHA={'pine':'f18a52c778aa752fc32bc369cb78847b77aa5cbb7c75ec600f457986564ad6a0',
           'alder':'02d6c82dd13e34f120a760a8070be03e649aeb327496351e76bfe522e7ea4de3'}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data): return hashlib.sha256(data).hexdigest()
def source_text_sha(data):
    """Git's editable text identity, independent of checkout LF/CRLF policy.

    Only explicitly named editable JSON/C++ inputs use this. Frozen archives,
    binary geometry/images and all generated outputs keep exact-byte identity.
    """
    data.decode('utf-8')
    canonical=data.replace(b'\r\n',b'\n')
    require(b'\r' not in canonical,'Unsupported bare CR in editable source text')
    return sha(canonical)
def encoded(d): return (json.dumps(d, indent=2, allow_nan=False) + '\n').encode()
def encoded_layout(layout):
    # Keep each generated placement/panel as one reviewable stable-ID row.
    blocks=[]
    for key,value in layout.items():
        if key in ('boundary','placements'):
            body='[\n'+',\n'.join('    '+json.dumps(row,separators=(',',':'),allow_nan=False) for row in value)+'\n  ]'
        else:body=json.dumps(value,indent=2,allow_nan=False)
        blocks.append('  '+json.dumps(key)+': '+body)
    return ('{\n'+',\n'.join(blocks)+'\n}\n').encode()
def add(a, b): return tuple(x+y for x, y in zip(a, b))
def sub(a, b): return tuple(x-y for x, y in zip(a, b))
def mul(a, s): return tuple(x*s for x in a)
def dot(a, b): return sum(x*y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def unit(a):
    n=math.sqrt(dot(a, a)); require(n>1e-14, 'Zero vector'); return mul(a, 1/n)
def yup(p): return (p[0], p[2], -p[1])
def frame(v):
    v=unit(v); a=unit(cross(v, (0, 1, 0) if abs(v[1])<.94 else (1, 0, 0)))
    return a, cross(v, a)


class Mesh:
    def __init__(self): self.positions=[]; self.faces=[]; self.components=0

    def solid(self, points, faces):
        vol=sum(dot(points[a], cross(points[b], points[c])) for a,b,c in faces)/6
        require(abs(vol)>1e-10, 'Zero-volume component')
        if vol<0: faces=[(a,c,b) for a,b,c in faces]
        base=len(self.positions); self.positions.extend(points)
        self.faces.extend(tuple(base+i for i in f) for f in faces); self.components+=1

    def tube(self, points, radii, sides):
        p=[]; f=[]
        for i,(point,radius) in enumerate(zip(points,radii)):
            a,b=frame(sub(points[min(i+1,len(points)-1)],points[max(0,i-1)]))
            p.extend(add(point,mul(add(mul(a,math.cos(j*math.tau/sides)),mul(b,math.sin(j*math.tau/sides))),radius)) for j in range(sides))
        for r in range(len(points)-1):
            for j in range(sides):
                a=r*sides+j; b=r*sides+(j+1)%sides; c=b+sides; d=a+sides
                f.extend([(a,b,c),(a,c,d)])
        for j in range(1,sides-1): f.extend([(0,j+1,j),((len(points)-1)*sides,(len(points)-1)*sides+j,(len(points)-1)*sides+j+1)])
        self.solid(p,f)

    def lens(self, center, axis, length, width, thickness, roll=0):
        axis=unit(axis); a,b=frame(axis)
        a,b=add(mul(a,math.cos(roll)),mul(b,math.sin(roll))),add(mul(a,-math.sin(roll)),mul(b,math.cos(roll)))
        p=[add(center,mul(axis,-length*.5)),add(center,mul(a,-width)),add(center,mul(axis,length*.5)),add(center,mul(a,width)),add(center,mul(b,thickness)),add(center,mul(b,-thickness))]
        f=[]
        for j in range(4): f.extend([(j,(j+1)%4,4),((j+1)%4,j,5)])
        self.solid(p,f)

    def crown(self, center, width, height, phase, sides=8):
        # Irregular closed, non-planar far silhouette; never a near-tree swap.
        p=[add(center,(width*(1+.16*math.sin(j*2.37+phase))*math.cos(j*math.tau/sides), .12*height*math.sin(j*2.1+phase), width*.8*(1+.12*math.cos(j*1.7+phase))*math.sin(j*math.tau/sides))) for j in range(sides)]
        p.extend([add(center,(width*.12,height*.65,0)),add(center,(-width*.08,-height*.35,width*.07))])
        self.solid(p,[(j,(j+1)%sides,sides) for j in range(sides)]+[((j+1)%sides,j,sides+1) for j in range(sides)])

    def box(self, lo, hi):
        p=[(x,y,z) for x in (lo[0],hi[0]) for y in (lo[1],hi[1]) for z in (lo[2],hi[2])]
        f=[]
        for a,b,c,d in [(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]: f.extend([(a,b,c),(a,c,d)])
        self.solid(p,f)

    def arrays(self):
        n=[(0,0,0) for _ in self.positions]
        for a,b,c in self.faces:
            v=cross(sub(self.positions[b],self.positions[a]),sub(self.positions[c],self.positions[a]))
            require(dot(v,v)>1e-18,'Degenerate triangle')
            for i in (a,b,c): n[i]=add(n[i],v)
        n=[unit(v) for v in n]
        # One deterministic non-degenerate projection; actual mapped tangents.
        best=None
        for projection in [(0.29,.61,.74),(.71,-.37,.59),(-.57,.72,.31)]:
            u,v=frame(projection); uv=[(dot(p,u)/1.7,dot(p,v)/1.7) for p in self.positions]
            quality=min(abs((uv[b][0]-uv[a][0])*(uv[c][1]-uv[a][1])-(uv[c][0]-uv[a][0])*(uv[b][1]-uv[a][1])) for a,b,c in self.faces)
            if best is None or quality>best[0]: best=(quality,uv)
        quality,uv=best; require(quality>1e-12,'Degenerate UV0')
        ts=[(0,0,0) for _ in n]; bs=ts[:]
        for a,b,c in self.faces:
            u1,v1=sub(uv[b],uv[a]); u2,v2=sub(uv[c],uv[a]); den=u1*v2-u2*v1
            e1,e2=sub(self.positions[b],self.positions[a]),sub(self.positions[c],self.positions[a])
            t=mul(sub(mul(e1,v2),mul(e2,v1)),1/den); bt=mul(sub(mul(e2,u1),mul(e1,u2)),1/den)
            for i in (a,b,c): ts[i]=add(ts[i],t); bs[i]=add(bs[i],bt)
        tangents=[]
        for normal,t,b in zip(n,ts,bs):
            t=unit(sub(t,mul(normal,dot(t,normal)))); tangents.append((*t,-1. if dot(cross(normal,t),b)<0 else 1.))
        return {'POSITION':self.positions,'NORMAL':n,'TEXCOORD_0':uv,'TANGENT':tangents}


def unpack(data):
    require(struct.unpack_from('<III',data)==(0x46546c67,2,len(data)),'Bad GLB')
    n,kind=struct.unpack_from('<II',data,12); require(kind==0x4e4f534a,'Missing JSON')
    j=json.loads(data[20:20+n]); size,kind=struct.unpack_from('<II',data,20+n)
    require(kind==0x004e4942,'Missing BIN'); return j,data[28+n:28+n+size]


def asset(species, level):
    name,identity=SOURCES[species]; source=(TREE/(name+'.glb')).read_bytes()
    require(sha(source)==identity,'Original source changed')
    j,source_binary=unpack(source)
    trace_data=(QA/(name+'-centerlines.json')).read_bytes()
    require(sha(trace_data)==TRACE_SHA[species],'Original authoring centreline changed')
    trace=json.loads(trace_data)
    tubes=[a for a in trace if a['kind']=='tube' and 'trunk roots branches' in a['mesh']]
    wood,foliage=Mesh(),Mesh()
    if level=='middle':
        selected=[0]+list(range(1,7))
        for i in selected:
            t=tubes[i]; ids=([0,len(t['points'])//2,len(t['points'])-1] if i==0 else [0,len(t['points'])-1])
            wood.tube([yup(t['points'][k]) for k in ids],[t['radii'][k] for k in ids],5 if i==0 else 3)
        # Original trunk/root centreline, new branch-supported irregular crown.
        # Dense distributed small sprays, not enlarged original leaves or lobes.
        count=20 if species=='pine' else 14
        for i in range(count):
            angle=i*2.399963+.2*math.sin(i*1.7)
            height=(2.8+i*.26) if species=='pine' else (2.95+i*.26)
            length=(2.1-i*.06) if species=='pine' else (1.25+.5*math.sin(i*.8)**2)
            trunk=tubes[0]['points'];trunk=[yup(p) for p in trunk]
            a,b=next((a,b) for a,b in zip(trunk,trunk[1:]) if a[1]<=height<=b[1])
            base=add(a,mul(sub(b,a),(height-a[1])/(b[1]-a[1])))
            end=add(base,(math.cos(angle)*length,.48 if species=='pine' else 1.55,math.sin(angle)*length))
            middle=add(mul(add(base,end),.5),(.03,.10,-.02))
            wood.tube([base,middle,end],[.075 if species=='pine' else .085,.038,.008],3)
            axis=unit(sub(end,base));side=unit(cross(axis,(0,1,0)))
            for k in range(7 if species=='pine' else 10):
                t=.22+k*(.105 if species=='pine' else .074)
                anchor=add(base,mul(sub(end,base),t))
                sign=-1 if k%2 else 1;center=add(anchor,mul(side,sign*.16))
                direction=unit(add(axis,mul(side,sign*.7)))
                foliage.lens(center,direction,.78 if species=='pine' else .62,.24 if species=='pine' else .22,.075,i*.47+k*.31)
    elif level=='far':
        t=tubes[0]; ids=[0,5,len(t['points'])-1]
        wood.tube([yup(t['points'][k]) for k in ids],[t['radii'][k] for k in ids],4)
        if species=='pine':
            for i in range(9):
                y=3.35+i*.57; width=1.65*(1-i/11)
                foliage.crown((.10+.045*i,y,-.05),width,1.18,i*.73,8)
        else:
            for i in range(8):
                angle=i*2.4; y=4.6+(i%3)*.92
                c=(math.cos(angle)*.72,y,math.sin(angle)*.65)
                wood.tube([(.2,3.6,0),c],[.08,.015],3)
                foliage.crown(c,.82,1.8,i*.91,6)
    else: raise ValueError('Unknown distance band')
    return make_glb(j,source_binary,[(wood,next(i for i,m in enumerate(j['materials']) if 'bark' in m['name'])),(foliage,next(i for i,m in enumerate(j['materials']) if ('needle' if species=='pine' else 'alder leaves') in m['name']))],f'{species}-{level}'), trace


def make_glb(source, binary, meshes, name):
    doc={'asset':{'version':'2.0','generator':'Horde original offline woodland recipe v1'},'scene':0,'scenes':[{'nodes':[0]}],'nodes':[{'mesh':0,'name':name}], 'meshes':[{'name':name,'primitives':[]}], 'buffers':[{}], 'bufferViews':[], 'accessors':[]}
    for key in ('materials','textures','samplers','images','extensionsUsed','extensionsRequired'):
        if key in source: doc[key]=copy.deepcopy(source[key])
    payload=bytearray()
    def view(data,target=None):
        require(len(data)>0,'Empty buffer view/image')
        payload.extend(b'\0'*(-len(payload)%4)); v={'buffer':0,'byteOffset':len(payload),'byteLength':len(data)}
        if target: v['target']=target
        doc['bufferViews'].append(v); payload.extend(data); return len(doc['bufferViews'])-1
    for image in doc.get('images',[]):
        old=source['bufferViews'][image['bufferView']]; off=old.get('byteOffset',0)
        image['bufferView']=view(binary[off:off+old['byteLength']])
    def accessor(rows,kind,component=5126):
        width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[kind]
        if component==5126:
            # Canonical authored precision before float storage/min-max JSON;
            # do not make cross-host receipts depend on libm's last double bit.
            rows=[tuple(struct.unpack('<f',struct.pack('<f',0. if abs(v)<5e-10 else round(v,9)))[0] for v in row) for row in rows]
        data=b''.join(struct.pack('<'+('f' if component==5126 else 'I')*width,*r) for r in rows)
        a={'bufferView':view(data,34962 if component==5126 else 34963),'componentType':component,'count':len(rows),'type':kind}
        if kind=='VEC3': a.update(min=[min(r[k] for r in rows) for k in range(3)],max=[max(r[k] for r in rows) for k in range(3)])
        doc['accessors'].append(a); return len(doc['accessors'])-1
    for mesh,material in meshes:
        attrs=mesh.arrays(); doc['meshes'][0]['primitives'].append({'attributes':{k:accessor(v,{'POSITION':'VEC3','NORMAL':'VEC3','TEXCOORD_0':'VEC2','TANGENT':'VEC4'}[k]) for k,v in attrs.items()},'indices':accessor([(i,) for f in mesh.faces for i in f],'SCALAR',5125),'material':material,'mode':4})
    doc['buffers'][0]['byteLength']=len(payload)
    js=json.dumps(doc,separators=(',',':'),allow_nan=False).encode();js+=b' '*(-len(js)%4);payload+=b'\0'*(-len(payload)%4)
    return struct.pack('<III',0x46546c67,2,28+len(js)+len(payload))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(payload),0x004e4942)+payload


def fence():
    j,b=unpack((TREE/(SOURCES['pine'][0]+'.glb')).read_bytes());m=Mesh()
    for x in (0,2.4): m.box((x-.065,-.08,-.065),(x+.065,1.42,.065))
    for y in (.27,.68,1.09): m.box((0,y-.055,-.05),(2.4,y+.055,.05))
    return make_glb(j,b,[(m,next(i for i,v in enumerate(j['materials']) if 'bark' in v['name']))],'three-rail-boundary-panel')


def project(p,route):
    best=None
    for i,(a,b) in enumerate(zip(route,route[1:])):
        d=sub(b,a); t=max(0,min(1,dot(sub(p,a),d)/dot(d,d))); q=add(a,mul(d,t)); dist=math.dist(p,q)
        if best is None or dist<best[0]: best=(dist,i,t,q)
    return best


def root_tips(data):
    j,b=unpack(data);a=j['accessors'][j['meshes'][0]['primitives'][0]['attributes']['POSITION']];v=j['bufferViews'][a['bufferView']]
    points=list(struct.iter_unpack('<fff',b[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']]))
    low=min(p[1] for p in points)
    return low,sorted(set(p for p in points if p[1]<=low+.03))


def simplify(points,tolerance):
    if len(points)<=2:return points
    d,i=max((project(p,[points[0],points[-1]])[0],i) for i,p in enumerate(points[1:-1],1))
    if d<=tolerance:return [points[0],points[-1]]
    return simplify(points[:i+1],tolerance)[:-1]+simplify(points[i:],tolerance)


def composition(assets,tips):
    mapping=json.loads(MAP.read_text()); route=[(round(x+78.3,6),round(126.2-y,6)) for x,y,z in mapping['forest']]
    require(route==[(-33.7,-12.8),(-28.7,-5.8),(-15.7,.2),(-17.7,15.2),(-3.7,20.2),(13.3,36.2)],'Map transform/route changed')
    spec=importlib.util.spec_from_file_location('forest_terrain_queries',ROOT/'tools/fit-forest-root-placements.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);ground=mod.terrain_queries()
    # A source-only union of corridor and explicit pockets. F01 includes graves,
    # slab/shaft and rope approaches, not merely the trail's landing centre.
    pockets=[{'id':'F01-burial-shaft','center':[-33.7,-17.0],'radius':10.0},{'id':'F02-reunion','center':list(route[1]),'radius':3.2},{'id':'F04-lookout','center':list(route[-1]),'radius':3.5}]
    original=json.loads(PLACEMENTS.read_text())
    require(original['grounding']['terrainSha256']==sha(TERRAIN.read_bytes()),'Original terrain identity changed')
    # Bend the candidate boundary around an original reachable trunk instead of
    # driving rails through its bark. These retain their existing colliders.
    trunkPockets=[]
    header=(ROOT/'src/gameplay/simulation/ForestTreePlacementContract.h').read_text()
    for line in header.splitlines():
        if '{ForestTreeSpecies::' not in line:continue
        vals=[float(x.rstrip('f')) for x in re.findall(r'-?\d+\.\d+f',line)]
        x,z,yaw,minx,minz,maxx,maxz,burial=vals
        radius=max(math.hypot(a-x,b-z) for a in (minx,maxx) for b in (minz,maxz))+.80
        base=min([project((x,z),route)[0]-2.4]+[math.dist((x,z),k['center'])-k['radius'] for k in pockets])
        if base<radius+.60:trunkPockets.append({'center':[x,z],'radius':radius,'source':'original-trunk-boundary-clearance'})
    def clearance(p): return min([project(p,route)[0]-2.4]+[math.dist(p,k['center'])-k['radius'] for k in pockets+trunkPockets])
    buildings=[(v['x']+78.3,126.2-v['y'],v['width']/2+10,v['depth']/2+10) for v in mapping['buildings']]
    river=[(x+78.3,126.2-y) for x,y,z in mapping['river_surface']]
    church=next(v for v in mapping['buildings'] if v['id']=='B02'); church=(church['x']+78.3,126.2-church['y'])
    tavern=next(v for v in mapping['buildings'] if v['id']=='B01'); tavern=(tavern['x']+78.3,126.2-tavern['y'])
    windows=[{'from':list(route[-1]),'to':list(church),'halfWidth':7.5},{'from':list(route[-1]),'to':list(tavern),'halfWidth':10.0}]
    townKeepout={'min':[15,10],'max':[150,195],'reason':'Conservative R2 town/garden/arrival reserve, not a new footprint approval'}
    def excludes(p):
        if all(townKeepout['min'][k]<p[k]<townKeepout['max'][k] for k in (0,1)):return True
        if any(abs(p[0]-x)<w and abs(p[1]-z)<d for x,z,w,d in buildings): return True
        if project(p,river)[0]<14: return True
        for w in windows:
            dist,_,t,_=project(p,[tuple(w['from']),tuple(w['to'])])
            if dist<w['halfWidth'] and .02<t<1: return True
        return False
    rows=[]
    for i,p in enumerate(original['placements']): rows.append({'id':f'original-{i:03}','band':'near-original','sourceIndex':i,**p,'collision':'existing-original-contract'})
    def station(t):
        lengths=[math.dist(a,b) for a,b in zip(route,route[1:])]; d=t*sum(lengths)
        for i,length in enumerate(lengths):
            if d<=length or i==len(lengths)-1:
                direction=unit(sub(route[i+1],route[i]));return add(route[i],mul(direction,d)),direction
            d-=length
    stands={}
    for band,n,offset in [('middle',4,24),('far',6,63)]:
        stands[band]=[]
        for i in range(n):
            point,direction=station(i/(n-1));normal=(-direction[1],direction[0])
            for side in (-1,1):stands[band].append(add(point,mul(normal,offset*side)))
        if band=='far':
            point,direction=station(0);back=mul(direction,-1);angle=math.atan2(back[1],back[0])
            stands[band].extend(add(point,(math.cos(angle+a)*65,math.sin(angle+a)*65)) for a in (-.7,0,.7))
        # Bounded composition adjustment, not a changed keepout: move a stand
        # centre to the nearest valid map/clearance cell before filling it.
        target=(20,28) if band=='middle' else (53,90)
        for i,anchor in enumerate(stands[band]):
            choices=[add(anchor,(dx,dz)) for dx in range(-21,22,3) for dz in range(-21,22,3)]
            choices=[q for q in choices if target[0]<=clearance(q)<=target[1] and not excludes(q)]
            require(bool(choices),f'No valid authored {band} stand {i}')
            stands[band][i]=min(choices,key=lambda q:(math.dist(anchor,q),q))
    rng=random.Random(SEED)
    for band,count,lo,hi in [('middle',96,12,36),('far',320,38,115)]:
        accepted=0
        for attempt in range(150000):
            stand=accepted%len(stands[band]);anchor=stands[band][stand];radius=10 if band=='middle' else 20
            angle=rng.random()*math.tau;r=radius*math.sqrt(rng.random());p=add(anchor,(math.cos(angle)*r,math.sin(angle)*r));c=clearance(p)
            if not lo<=c<=hi or excludes(p): continue
            if any(math.hypot(p[0]-v['x'],p[1]-v['z'])<(3.1 if band=='middle' else 3.7) for v in rows): continue
            y=ground(*p)
            if y is None: continue
            # Reject unsupported/steep patches rather than trusting a centre.
            samples=[ground(p[0]+dx,p[1]+dz) for dx,dz in [(0,0),(.55,0),(-.55,0),(0,.55),(0,-.55)]]
            if any(v is None for v in samples) or max(samples)-min(samples)>.7: continue
            species='pine' if rng.random()<(.68 if band=='far' else .55) else 'alder';name=f'{species}-{band}.glb'
            bounds=assets[name]['bounds']
            radius=max(math.hypot(x,z) for x in (bounds[0][0],bounds[1][0]) for z in (bounds[0][2],bounds[1][2]))
            if c-radius-1.5<lo:continue  # geometry extent + conservative fence-contour expansion
            yaw=round(rng.random()*math.tau,6);co,si=math.cos(yaw),math.sin(yaw);low,local=tips[name]
            support=[ground(p[0]+co*x+si*z,p[1]-si*x+co*z) for x,ly,z in local]
            if any(v is None for v in support):continue
            fitted=min(g-ly-.02 for (_,ly,_),g in zip(local,support));burial=y-low-fitted
            if not 0<=burial<=.65:continue
            rows.append({'id':f'{band}-{accepted:03}','band':band,'stand':stand,'asset':name,'species':species.title(),'x':round(p[0],6),'z':round(p[1],6),'originY':round(fitted,6),'yawRadians':yaw,'scale':1.0,'rootPlayableClearance':round(c,6),'nearestPlayableClearance':round(c-radius-1.5,6),'collision':'none-ONLY-after-boundary-admission','supportSamples':len(local),'fittedBurial':round(burial,6)})
            accepted+=1
            if accepted==count: break
        require(accepted==count,f'Unable to fit {band} population: {accepted}/{count}, next stand {accepted%len(stands[band])}; constraints unchanged')
    # Boundary source plan: a continuous visible fallback fence around the union.
    # Replacement bank/rock runs are proposals only, never assumed impassable.
    step=.4; occupied=set(); disks=pockets+trunkPockets
    limits=[(math.floor(min([p[k]-2.4 for p in route]+[v['center'][k]-v['radius'] for v in disks])/step)-5,
             math.ceil(max([p[k]+2.4 for p in route]+[v['center'][k]+v['radius'] for v in disks])/step)+5) for k in (0,1)]
    for ix in range(*limits[0]):
        for iz in range(*limits[1]):
            if clearance(((ix+.5)*step,(iz+.5)*step))<.7: occupied.add((ix,iz))
    # Conservative cell bridges eliminate diagonal corner kisses. Fill tiny
    # enclosed raster holes, never subtract required walking space.
    for x,z in sorted(occupied):
        for dz in (-1,1):
            if (x+1,z+dz) in occupied and (x+1,z) not in occupied and (x,z+dz) not in occupied:occupied.add((x+1,z))
    outside={(limits[0][0],limits[1][0])};queue=list(outside)
    for x,z in queue:
        for q in [(x+1,z),(x-1,z),(x,z+1),(x,z-1)]:
            if limits[0][0]<=q[0]<limits[0][1] and limits[1][0]<=q[1]<limits[1][1] and q not in occupied and q not in outside:outside.add(q);queue.append(q)
    occupied.update((x,z) for x in range(*limits[0]) for z in range(*limits[1]) if (x,z) not in outside)
    edges={}
    for x,z in occupied:
        for neighbor,a,b in [((x,z-1),(x,z),(x+1,z)),((x+1,z),(x+1,z),(x+1,z+1)),((x,z+1),(x+1,z+1),(x,z+1)),((x-1,z),(x,z+1),(x,z))]:
            if neighbor not in occupied: require(a not in edges,'Ambiguous contour');edges[a]=b
    start=min(edges);chain=[start];v=start
    while True:
        v=edges[v]
        if v==start:break
        chain.append(v); require(len(chain)<=len(edges),'Unclosed contour')
    require(len(chain)==len(edges),'Disconnected corridor boundary')
    corners=[]
    for i,p in enumerate(chain):
        a,b=chain[i-1],chain[(i+1)%len(chain)]
        if cross((p[0]-a[0],0,p[1]-a[1]),(b[0]-p[0],0,b[1]-p[1]))!=(0,0,0):corners.append((p[0]*step,p[1]*step))
    split=max(range(len(corners)),key=lambda i:math.dist(corners[0],corners[i]))
    corners=simplify(corners[:split+1],.32)[:-1]+simplify(corners[split:]+[corners[0]],.32)[:-1]
    # Smooth contour, split long runs into ordinary <=2.4m terrain-conformed
    # modules. Rails require endpoint-height shear; posts remain upright.
    panels=[]
    for p,q in zip(corners,corners[1:]+corners[:1]):
        n=max(1,math.ceil(math.dist(p,q)/2.4))
        panels.extend(add(p,mul(sub(q,p),i/n)) for i in range(n))
    corners=panels
    boundary=[]
    for i,p in enumerate(corners):
        q=corners[(i+1)%len(corners)];a,b=ground(*p),ground(*q)
        require(a is not None and b is not None,'Boundary requires actual terrain support')
        require(all(0<=clearance(add(p,mul(sub(q,p),s/10)))<=1.5 for s in range(11)),'Boundary cuts protected pocket or exceeds detail-band allowance')
        direction=unit(sub(q,p));outward=(direction[1],-direction[0]);center=mul(add(p,q),.5)
        outer,inner=ground(*add(center,mul(outward,1.2))),ground(*add(center,mul(outward,-1.2)))
        rise=None if outer is None or inner is None else round(outer-inner,6)
        boundary.append({'id':f'edge-{i:03}','a':[round(p[0],6),round(a,6),round(p[1],6)],'b':[round(q[0],6),round(b,6),round(q[1],6)],'default':'visible-fence','asset':'boundary-panel.glb','widthScale':round(math.dist(p,q)/2.4,6),'endpointHeightDelta':round(b-a,6),'existingTerrainOutwardRise':rise,'preferredTreatment':'inspect-existing-bank' if rise is not None and rise>.65 else 'short-fence-or-grounded-rock-run','naturalAlternative':'bank-or-rock-ONLY-after-solid-mesh-and-capsule-proof','collisionAdmitted':False})
    counts=Counter(v['band'] for v in rows)
    trees=sum(assets[v['asset']]['triangles'] for v in rows if 'asset' in v)
    # Preserve and locate the existing twelve rock authoring records. These are
    # references for fence/bank joins, not duplicate/new placements or proof that
    # a boulder closes a boundary gap. Native grounded mesh/collision stays owned.
    rock_source=(ROOT/'src/scene/ForestRockDressing.cpp').read_bytes()
    block=rock_source.decode().split('kPlacements{{',1)[1].split('}};',1)[0]
    rocks=[]
    for i,row in enumerate(re.findall(r'\{([^{}]+)\}',block)):
        source,segment,fraction,side,yaw,scale=[float(v.strip().rstrip('f')) for v in row.split(',')]
        a,b=route[int(segment)-2:int(segment)];direction=unit(sub(b,a));point=add(add(a,mul(sub(b,a),fraction)),mul((-direction[1],direction[0]),side))
        nearest=min(boundary,key=lambda e:project(point,[(e['a'][0],e['a'][2]),(e['b'][0],e['b'][2])])[0])
        rocks.append({'nativePlacementIndex':i,'sourceIndex':int(source),'x':round(point[0],6),'z':round(point[1],6),'terrainY':round(ground(*point),6),'yaw':yaw,'scale':scale,'nearestBoundaryRun':nearest['id'],'referenceOnly':True})
    require(len(rocks)==12,'Existing rock roster changed')
    return {'schema':1,'status':'SOURCE-ONLY, NOT RUNTIME BOUNDARY/COLLISION/FOREST ADMISSION','seed':SEED,'coordinates':'native X east, Y up, Z south; R2 X=E+78.3, Z=126.2-N','mapSha256':sha(MAP.read_bytes()),'terrainSha256':sha(TERRAIN.read_bytes()),'originalPlacementsSha256':source_text_sha(PLACEMENTS.read_bytes()),'editableTextIdentity':'originalPlacementsSha256 and existingRockSourceSha256 hash UTF-8 LF-normalized text; all binary/frozen-archive inputs and generated outputs retain exact-byte checks','route':route,'clearTrailWidth':2.4,'corridorHalfWidth':2.4,'pockets':pockets,'originalTrunkClearancePockets':trunkPockets,'windows':windows,'townKeepout':townKeepout,'riverClearance':14,'standAnchors':stands,'boundary':boundary,'existingRockReferences':rocks,'existingRockSourceSha256':source_text_sha(rock_source),'placements':rows,'counts':dict(counts),'candidateTreeTriangles':323264+trees,'addedTreeRawUpperBytes':trees*140,'boundaryGeometryPolicy':'Closed conservative source contour; fences close every unproven bank/rock gap. Endpoint-height conformance keeps posts vertical. Replace fallback runs with grounded rocks/existing banks only after actual-mesh/capsule validation. No invisible barrier or old collision removal.','retention':'No old ordinary/shared certificate applies automatically; candidate full-source retention/recreation proof required at integration.','admission':{'normalDefault':False,'boundary':False,'collisionExemption':False,'nativeImporter':False,'rtVisuals':False,'gpuCost':False}}


def inspect(data):
    j,b=unpack(data);tris=vertices=0;lo=[math.inf]*3;hi=[-math.inf]*3
    for mesh in j['meshes']:
        for pr in mesh['primitives']:
            def read(ai):
                a=j['accessors'][ai];v=j['bufferViews'][a['bufferView']];fmt={5126:'f',5125:'I'}[a['componentType']];n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];return list(struct.iter_unpack('<'+fmt*n,b[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']]))
            p=read(pr['attributes']['POSITION']);n=read(pr['attributes']['NORMAL']);t=read(pr['attributes']['TANGENT']);uv=read(pr['attributes']['TEXCOORD_0']);idx=[v[0] for v in read(pr['indices'])]
            require(all(math.isfinite(x) for rows in (p,n,t,uv) for row in rows for x in row),'Nonfinite attribute')
            require(all(abs(dot(v,v)-1)<1e-4 for v in n),'Invalid normals')
            require(all(abs(dot(a,v[:3]))<1e-4 and abs(dot(v[:3],v[:3])-1)<1e-4 and v[3] in (-1,1) for a,v in zip(n,t)),'Invalid tangent')
            edges=Counter();volume=0
            for i in range(0,len(idx),3):
                a,c,d=idx[i:i+3]; require(max(a,c,d)<len(p),'Bad index');v=cross(sub(p[c],p[a]),sub(p[d],p[a]));require(dot(v,v)>1e-16,'Degenerate stored triangle');volume+=dot(p[a],cross(p[c],p[d]))/6
                for x,y in [(a,c),(c,d),(d,a)]: edges[x,y]+=1
            require(volume>0 and all(count==1 and edges[y,x]==1 for (x,y),count in edges.items()),'Unclosed/opposite topology')
            for v in p:
                for k in range(3):lo[k]=min(lo[k],v[k]);hi[k]=max(hi[k],v[k])
            tris+=len(idx)//3;vertices+=len(p)
    require(all(m.get('alphaMode','OPAQUE')=='OPAQUE' for m in j['materials']),'Unadmitted alpha')
    for image in j['images']:
        view=j['bufferViews'][image['bufferView']];offset=view.get('byteOffset',0)
        require(view['byteLength']>8 and b[offset:offset+8]==b'\x89PNG\r\n\x1a\n','Missing embedded PNG payload')
    return {'triangles':tris,'vertices':vertices,'bounds':[lo,hi],'bytes':len(data),'sha256':sha(data),'opaque':True,'uv0NormalsTangents':True,'closedOrientedComponents':True,'rawNativeBytes':vertices*64+tris*12,'embeddedImageSha256':[sha(b[j['bufferViews'][i['bufferView']].get('byteOffset',0):j['bufferViews'][i['bufferView']].get('byteOffset',0)+j['bufferViews'][i['bufferView']]['byteLength']]) for i in j['images']]}


def produce():
    files={};stats={};tips={}
    for species in SOURCES:
        for level in ('middle','far'):
            data,_=asset(species,level);name=f'{species}-{level}.glb';stats[name]=inspect(data)
            require(stats[name]['triangles']<=(1500 if level=='middle' else 250),'Authored tree budget exceeded')
            files['candidates/'+name]=data
            tips[name]=root_tips(data)
    data=fence();stats['boundary-panel.glb']=inspect(data);files['candidates/boundary-panel.glb']=data
    require(stats['boundary-panel.glb']['triangles']<=100,'Fence budget exceeded')
    layout=composition(stats,tips);require(layout['candidateTreeTriangles']<=547264,'Whole tree budget exceeded')
    files['composition.json']=encoded_layout(layout)
    manifest=json.loads((TREE/'asset.manifest.json').read_text());manifest.update(asset='forest-surroundings-v1-source-candidates',distribution='source-only-unadmitted',licenceStatus=RIGHTS)
    manifest['budgets'].update(maxVertices=1500,maxIndices=4500,maxPrimitives=2)
    manifest['lods']=[{'name':'middle','maxTriangles':1500},{'name':'far','maxTriangles':250},{'name':'boundary-panel','maxTriangles':100}]
    files['candidates/asset.manifest.json']=encoded(manifest)
    files['asset-receipt.json']=encoded({'schema':1,'status':'UNADMITTED SOURCE CANDIDATES','rights':RIGHTS,'provider':'none','originalModels':{s:{'path':str((TREE/(v[0]+'.glb')).relative_to(ROOT)).replace('\\','/'),'sha256':v[1],'centrelineSha256':TRACE_SHA[s]} for s,v in SOURCES.items()},'assets':stats,'nativeImporter':'external exact-file receipt required; not implied by authoring','gpuPresentation':'not run','normalActivation':False})
    return files


def main():
    p=argparse.ArgumentParser(description=__doc__);g=p.add_mutually_exclusive_group(required=True);g.add_argument('--write',action='store_true');g.add_argument('--check',action='store_true');a=p.parse_args()
    files=produce()
    for name,data in files.items():
        path=HERE/name
        if a.check:require(path.is_file() and path.read_bytes()==data,f'Stale candidate: {name}')
        else:
            require(not path.is_symlink(),'Candidate output is linked')
            path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    print(json.dumps({'files':len(files),'mode':'check' if a.check else 'write','status':'pass'}))


if __name__=='__main__': main()
