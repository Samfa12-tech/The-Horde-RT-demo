"""Offline root-footprint placement proposal; native import/tests remain admission.

Preserves v2 and every GLB/master. Writes a separate v3 JSON only with --write.
Uses exact baked runtime triangles; no planning-heightfield substitution.
"""
import argparse
import hashlib
import json
import math
import struct
from collections import defaultdict
from pathlib import Path
import importlib.util

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('forest_contract', Path(__file__).with_name('build-forest-placement-contract.py'))
CONTRACT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CONTRACT)
SOURCE = ROOT / 'assets/models/world/source/forest-placement-v1/woodland-placements-v2.json'
OUTPUT = SOURCE.with_name('woodland-placements-v3.json')
TERRAIN = ROOT / 'assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb'
ROUTE = [(-33.7,-12.8),(-28.7,-5.8),(-15.7,.2),(-17.7,15.2),(-3.7,20.2),(13.3,36.2)]


def terrain_queries():
    data = TERRAIN.read_bytes()
    length = struct.unpack_from('<I', data, 12)[0]
    doc = json.loads(data[20:20+length]); binary = data[28+length:]
    assert all(not any(k in n for k in ('matrix','scale','rotation','translation')) for n in doc['nodes'])
    def accessor(index):
        a=doc['accessors'][index]; v=doc['bufferViews'][a['bufferView']]
        fmt={5126:'f',5123:'H',5125:'I'}[a['componentType']]
        count={'VEC3':3,'SCALAR':1}[a['type']]; fmt='<'+str(count)+fmt
        stride=v.get('byteStride',struct.calcsize(fmt)); offset=v.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from(fmt,binary,offset+i*stride) for i in range(a['count'])]
    cells=defaultdict(list)
    for mesh in doc['meshes']:
        for primitive in mesh['primitives']:
            points=accessor(primitive['attributes']['POSITION']); indices=accessor(primitive['indices'])
            for i in range(0,len(indices),3):
                a,b,c=[points[indices[j][0]] for j in range(i,i+3)]
                den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
                if den>=-1e-10: continue  # highest upward-facing surface, native convention
                lo=[max(-100,math.floor(min(p[k] for p in (a,b,c))/2)) for k in (0,2)]
                hi=[min(100,math.floor(max(p[k] for p in (a,b,c))/2)) for k in (0,2)]
                for x in range(lo[0],hi[0]+1):
                    for z in range(lo[1],hi[1]+1): cells[x,z].append((a,b,c,den))
    def query(x,z):
        result=None
        for a,b,c,den in cells[math.floor(x/2),math.floor(z/2)]:
            u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/den
            v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/den
            if min(u,v,1-u-v)<-1e-7: continue
            y=u*a[1]+v*b[1]+(1-u-v)*c[1]
            result=y if result is None else max(result,y)
        return result
    return query


def segment_box(a,b,box,pad):
    enter,leave=0.,1.
    for axis in (0,1):
        lo,hi=box[axis]-pad,box[axis+2]+pad; d=b[axis]-a[axis]
        if abs(d)<1e-8:
            if not lo<=a[axis]<=hi: return False
        else:
            t0,t1=sorted(((lo-a[axis])/d,(hi-a[axis])/d))
            enter=max(enter,t0); leave=min(leave,t1)
            if enter>leave: return False
    return True


def generate():
    plan=json.loads(SOURCE.read_text()); ground=terrain_queries()
    sources={name:CONTRACT.bark_points(spec) for name,spec in plan['sources'].items()}
    shapes=[]
    for p in plan['placements']:
        c,s=math.cos(p['yawRadians']),math.sin(p['yawRadians']); points=sources[p['species']]
        rotated=[(c*x+s*z,y,-s*x+c*z) for x,y,z in points]; bottom=min(p[1] for p in rotated)
        tips=sorted(set(p for p in rotated if p[1]<=bottom+.02))
        box=[min(p[k] for p in rotated) for k in (0,2)]+[max(p[k] for p in rotated) for k in (0,2)]
        shapes.append((bottom,tips,box))
    boxes=[[p['x']+b[0],p['z']+b[1],p['x']+b[2],p['z']+b[3]] for p,(_,_,b) in zip(plan['placements'],shapes)]
    offsets=sorted(((i*.5,j*.5) for i in range(-10,11) for j in range(-10,11)),key=lambda p:(p[0]**2+p[1]**2,p))
    measurements=[]
    for index,(p,(bottom,tips,b)) in enumerate(zip(plan['placements'],shapes)):
        def burial(x,z):
            centre=ground(x,z)
            if centre is None: return None
            values=[ground(x+a,z+c) for a,_,c in tips]
            if any(v is None for v in values): return None
            return max(0.,max(centre-bottom+y-g+.02 for (_,y,_),g in zip(tips,values)))
        before=burial(p['x'],p['z']); chosen=None
        for dx,dz in offsets:
            x,z=p['x']+dx,p['z']+dz; box=[x+b[0],z+b[1],x+b[2],z+b[3]]
            if any(segment_box(a,c,box,1.44) for a,c in zip(ROUTE,ROUTE[1:])): continue
            if any(j!=index and box[0]<=other[2] and box[2]>=other[0] and box[1]<=other[3] and box[3]>=other[1] for j,other in enumerate(boxes)): continue
            depth=burial(x,z)
            if depth is None or depth>.60: continue  # 5cm reserve below native admission limit
            chosen=(x,z,box,depth); break
        if chosen is None: raise RuntimeError(f'No bounded placement for tree {index}; preserve v2 and review')
        x,z,boxes[index],depth=chosen
        measurements.append(dict(index=index,moveMetres=math.hypot(x-p['x'],z-p['z']),beforeBurial=before,afterBurial=depth))
        p['x'],p['z']=round(x,9),round(z,9)
    plan['description']='64 upright woodland placements fitted to native root footprints; 2.4m trail plus capsule margin, original models and four material families retained. v2 preserved. Runtime validates <=0.65m burial and actual imported tips.'
    plan['grounding']={'terrainSha256':hashlib.sha256(TERRAIN.read_bytes()).hexdigest(),'rootBaseBandMetres':.02,'minimumTipEmbedMetres':.02,'runtimeMaximumBurialMetres':.65,'measurements':measurements}
    return plan


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--write',action='store_true'); args=parser.parse_args()
    result=generate(); encoded=json.dumps(result,indent=2)+'\n'
    if args.write:
        with OUTPUT.open('x',encoding='utf-8',newline='\n') as f: f.write(encoded)
    else: assert OUTPUT.read_text()==encoded,'root placement receipt is stale'
    rows=result['grounding']['measurements']
    print(json.dumps({'trees':len(rows),'moved':sum(r['moveMetres']>0 for r in rows),'maximumMoveMetres':max(r['moveMetres'] for r in rows),'maximumBurialMetres':max(r['afterBurial'] for r in rows)}))
