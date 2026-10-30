"""One owned same-topology crown study, not production assets.

Run with Python/numpy from the worktree: reshape.py --output reports/<fresh>/assets.
Only the original runtime GLBs are read. This is a direct project geometry edit;
it does not invoke Blender's exporter, access its denied temporary directory,
change permissions or change temporary-path configuration. Images remain exact.
"""
from pathlib import Path
import argparse, copy, hashlib, json, math, struct, sys
import numpy as np

ROOT = Path(__file__).resolve().parents[5]
SOURCE = ROOT / 'assets/models/world/runtime/forest-tree-pair-v1'

def sha(b):
    return hashlib.sha256(b).hexdigest()

def unit(v):
    return v / np.linalg.norm(v)

def load(p):
    b = p.read_bytes()
    magic, version, size = struct.unpack_from('<III', b)
    assert (magic, version, size) == (0x46546c67, 2, len(b))
    n, kind = struct.unpack_from('<II', b, 12)
    assert kind == 0x4e4f534a
    j = json.loads(b[20:20+n])
    bn, bk = struct.unpack_from('<II', b, 20+n)
    assert bk == 0x004e4942
    return b, j, bytearray(b[28+n:28+n+bn])

def view(j, data, ai):
    a = j['accessors'][ai]; v = j['bufferViews'][a['bufferView']]
    dtype = {5126:'<f4', 5123:'<u2'}[a['componentType']]
    width = {'SCALAR':1, 'VEC2':2, 'VEC3':3, 'VEC4':4}[a['type']]
    assert 'byteStride' not in v and 'sparse' not in a
    return np.frombuffer(data, dtype=dtype, count=a['count']*width,
        offset=v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)

def derive(species, path, out):
    original, j, data = load(path); before = bytes(data); oldj = copy.deepcopy(j)
    mesh = next(m for m in j['meshes'] if 'needle-spray' in m['name'] or 'opaque leaves' in m['name'])
    p = mesh['primitives'][0]; a = p['attributes']
    pos = view(j,data,a['POSITION']); normals = view(j,data,a['NORMAL'])
    tangents = view(j,data,a['TANGENT']); oldpos = pos.copy()
    indices = view(j,data,p['indices']).ravel()
    # Original exported closed lenses: ten seam vertices / eight triangles each.
    assert len(pos)%10 == 0 and len(indices) == len(pos)//10*24
    assert all(not any(k in n for k in ('matrix','rotation','scale','translation')) for n in j['nodes'])
    fixed = []; determinants = []
    for component in range(len(pos)//10):
        k = component*10; points = oldpos[k:k+10].astype(float)
        assert np.all(indices[component*24:(component+1)*24]//10 == component)
        anchor = points[0]; axis = unit(points[4]-anchor)
        side = unit(points[6]-points[2]); side = unit(side-axis*np.dot(side,axis))
        normal = unit(np.cross(axis,side)); basis = np.column_stack((axis,side,normal))
        # Retain the branch attachment and irregular tiers, but spread the sprays
        # into outward bough volumes instead of stretching only their leaf width.
        radial = np.array([anchor[0]-.2,0.,anchor[2]-.05])
        if np.linalg.norm(radial)<.05: radial=np.array([math.cos(component*2.399),0.,math.sin(component*2.399)])
        radial = unit(radial)
        height = min(1.,max(0.,(anchor[1]-3.4)/5.2))
        taper = math.sin(height*math.pi)
        desired = unit(radial + np.array([0., .12 if species=='pine' else .35, 0.]))
        newaxis = unit(axis*.50 + desired*.50)
        newside = unit(side-newaxis*np.dot(side,newaxis)); newnormal=unit(np.cross(newaxis,newside))
        scales = np.array([1.7+.40*taper, 3.6+.90*taper, 2.8]) if species=='pine' else np.array([1.65,2.65,2.0])
        transform = np.column_stack((newaxis,newside,newnormal)) @ np.diag(scales) @ basis.T
        determinants.append(float(np.linalg.det(transform)))
        pos[k:k+10] = anchor + (transform @ (points-anchor).T).T
        n = normals[k:k+10].astype(float) @ np.linalg.inv(transform)
        n /= np.linalg.norm(n,axis=1)[:,None]
        t = tangents[k:k+10,:3].astype(float) @ transform.T
        t -= n*np.sum(n*t,axis=1)[:,None]; t /= np.linalg.norm(t,axis=1)[:,None]
        normals[k:k+10] = n; tangents[k:k+10,:3] = t
        fixed.append(float(np.linalg.norm(pos[k]-oldpos[k])))
    for attr in ['POSITION','NORMAL','TANGENT']:
        assert np.isfinite(view(j,data,a[attr])).all()
    positionAccessor = j['accessors'][a['POSITION']]
    positionAccessor['min'] = pos.min(axis=0).tolist(); positionAccessor['max'] = pos.max(axis=0).tolist()
    changedViews = {j['accessors'][a[x]]['bufferView'] for x in ['POSITION','NORMAL','TANGENT']}
    unchanged = []
    for i,v in enumerate(j['bufferViews']):
        if i not in changedViews:
            lo=v.get('byteOffset',0); hi=lo+v['byteLength']
            assert bytes(data[lo:hi]) == before[lo:hi]
            unchanged.append({'view':i,'sha256':sha(before[lo:hi])})
    faces = pos[indices.reshape(-1,3)].astype(float)
    areas = np.linalg.norm(np.cross(faces[:,1]-faces[:,0],faces[:,2]-faces[:,0]),axis=1)*.5
    assert min(determinants)>0 and max(fixed)==0 and areas.min()>1e-10
    assert j['materials']==oldj['materials'] and j['images']==oldj['images']
    js=json.dumps(j,separators=(',',':')).encode();js+=b' '*((-len(js))%4)
    payload=struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(data),0x004e4942)+data
    result=struct.pack('<III',0x46546c67,2,12+len(payload))+payload
    output=out/path.name;output.write_bytes(result)
    return {'species':species,'source':path.relative_to(ROOT).as_posix(),'sourceSha256':sha(original),
        'candidateSha256':sha(result),'sourceBytes':len(original),'candidateBytes':len(result),
        'foliageComponents':len(pos)//10,'triangles':sum(j['accessors'][q['indices']]['count']//3 for m in j['meshes'] for q in m['primitives']),
        'vertices':sum(j['accessors'][q['attributes']['POSITION']]['count'] for m in j['meshes'] for q in m['primitives']),
        'materials':len(j['materials']),'boundsBefore':[oldpos.min(axis=0).tolist(),oldpos.max(axis=0).tolist()],
        'boundsAfter':[pos.min(axis=0).tolist(),pos.max(axis=0).tolist()],
        'maximumAttachmentDisplacement':max(fixed),'minimumPositiveDeterminant':min(determinants),
        'minimumTriangleArea':float(areas.min()),'unchangedBufferViews':unchanged,
        'imagesWoodMossIndicesUvsUnchanged':True,'runtimeAdmitted':False}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    assert args.output.resolve().is_relative_to(ROOT), 'Outputs stay in the authorised worktree.'
    out=args.output/'models/world/runtime/forest-tree-pair-v1';out.mkdir(parents=True,exist_ok=False)
    records=[]
    for species,stem in [('pine','horde-irregular-pine-v1'),('alder','horde-upright-alder-v1')]:
        records.append(derive(species,SOURCE/(stem+'-lod1.glb'),out))
    (out/'asset.manifest.json').write_bytes((SOURCE/'asset.manifest.json').read_bytes())
    (args.output.parent/'geometry-receipt.json').write_text(json.dumps({'sourceCommit':'7659c6f4','records':records,
        'scope':'same topology owned crown study; no production assets, placement, limits, textures or shader changes',
        'canonicalGameDevCli':'unavailable; not a verified canonical package'},indent=2)+'\n')
    print(json.dumps(records,indent=2))

if __name__=='__main__': main()
