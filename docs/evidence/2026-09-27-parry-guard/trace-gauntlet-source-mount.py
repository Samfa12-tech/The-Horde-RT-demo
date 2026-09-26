"""Read-only UV-correspondence trace from source gauntlet to exact native upload.
No bone-axis assumption defines the cuff: it follows the labelled source +X.
Run in Blender Python for NumPy; prints evidence only, never writes an asset.
"""
import json
import struct
import sys
from pathlib import Path
import numpy as np

def local(node):
    if 'matrix' in node:
        return np.array(node['matrix']).reshape(4,4,order='F')
    x,y,z,w = node.get('rotation',[0,0,0,1])
    m = np.eye(4)
    m[:3,:3] = np.array([[1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w],
        [2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w],
        [2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y]]) @ np.diag(node.get('scale',[1,1,1]))
    m[:3,3] = node.get('translation',[0,0,0])
    return m

class Glb:
    def __init__(self,path):
        raw=Path(path).read_bytes(); n=struct.unpack_from('<I',raw,12)[0]
        self.doc=json.loads(raw[20:20+n]); self.binary=raw[28+n:]
        self.parents={c:i for i,n in enumerate(self.doc['nodes']) for c in n.get('children',[])}
    def acc(self,i):
        a=self.doc['accessors'][i]; b=self.doc['bufferViews'][a['bufferView']]
        count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
        dt=np.dtype({5121:'u1',5123:'<u2',5125:'<u4',5126:'<f4'}[a['componentType']])
        return np.ndarray((a['count'],count),dtype=dt,buffer=self.binary,
            offset=b.get('byteOffset',0)+a.get('byteOffset',0),
            strides=(b.get('byteStride',dt.itemsize*count),dt.itemsize)).copy()
    def global_node(self,i):
        return (self.global_node(self.parents[i]) if i in self.parents else np.eye(4)) @ local(self.doc['nodes'][i])

source_path,bind_path,obj_path=sys.argv[sys.argv.index('--')+1:]
source,bind=Glb(source_path),Glb(bind_path)
source_node=next(i for i,n in enumerate(source.doc['nodes']) if n.get('extras',{}).get('hordeGauntletSchema')==1)
extras=source.doc['nodes'][source_node]['extras']
source_global=source.global_node(source_node)
source_primitive=source.doc['meshes'][source.doc['nodes'][source_node]['mesh']]['primitives'][0]
source_positions=source.acc(source_primitive['attributes']['POSITION'])
source_positions=(source_global @ np.column_stack((source_positions,np.ones(len(source_positions)))).T).T[:,:3]
source_uvs=source.acc(source_primitive['attributes']['TEXCOORD_0'])
lookup={}
for uv,p in zip(source_uvs,source_positions):
    lookup.setdefault(tuple(np.round(uv,6)),set()).add(tuple(np.round(p,6)))
lookup={k:next(iter(v)) for k,v in lookup.items() if len(v)==1}
bind_positions=[]; bind_uvs=[]; joints=[]; weights=[]; is_gauntlet=[]
for primitive in bind.doc['meshes'][0]['primitives']:
    attrs=primitive['attributes']; p=bind.acc(attrs['POSITION'])
    bind_positions.extend(p); bind_uvs.extend(bind.acc(attrs['TEXCOORD_0']))
    joints.extend(bind.acc(attrs['JOINTS_0'])); weights.extend(bind.acc(attrs['WEIGHTS_0']))
    name=bind.doc['materials'][primitive['material']]['name']
    is_gauntlet.extend(['Gauntlet' in name]*len(p))
bind_positions,bind_uvs,joints,weights,is_gauntlet=map(np.array,(bind_positions,bind_uvs,joints,weights,is_gauntlet))
posed=np.array([list(map(float,line.split()[1:4])) for line in Path(obj_path).read_text().splitlines() if line.startswith('v ')])
assert len(posed)==len(bind_positions)
names={n.get('name'):i for i,n in enumerate(bind.doc['nodes'])}
bone_ids={bind.doc['nodes'][n]['name']:j for j,n in enumerate(bind.doc['skins'][0]['joints'])}
unit=lambda v:v/np.linalg.norm(v)
point=lambda m,p:(m@np.append(p,1))[:3]

def fit(a,b,label):
    assert len(a)>100,(label,'too few correspondences',len(a))
    matrix,_,rank,_=np.linalg.lstsq(np.column_stack((a,np.ones(len(a)))),b,rcond=None)
    error=float(np.max(np.abs(np.column_stack((a,np.ones(len(a))))@matrix-b)))
    assert rank==4 and error<2e-5,(label,rank,error)
    affine=np.eye(4); affine[:3,:]=matrix.T
    return affine,dict(samples=len(a),maximumCoordinateError=error,determinant=float(np.linalg.det(affine[:3,:3])))

result={}
for side in ('Left','Right'):
    selected=is_gauntlet & np.any((joints==bone_ids[side+'Hand'])&(weights>.99999),axis=1)
    pairs=[(lookup[key],p) for uv,p in zip(bind_uvs[selected],bind_positions[selected]) if (key:=tuple(np.round(uv,6))) in lookup]
    src_to_bind,mount_stats=fit(np.array([x[0] for x in pairs]),np.array([x[1] for x in pairs]),side+' source-to-bind')
    bind_to_pose,skin_stats=fit(bind_positions[selected],posed[selected],side+' Hand skin')
    forearm=np.any((joints==bone_ids[side+'ForeArm'])&(weights>.99999),axis=1)
    forearm_to_pose,forearm_stats=fit(bind_positions[forearm],posed[forearm],side+' ForeArm skin')
    upper=np.any((joints==bone_ids[side+'Arm'])&(weights>.99999),axis=1)
    upper_to_pose,upper_stats=fit(bind_positions[upper],posed[upper],side+' Arm skin')
    hand=bind.global_node(names[side+'Hand']); elbow=bind.global_node(names[side+'ForeArm'])
    shoulder=bind.global_node(names[side+'Arm'])
    posed_wrist=point(bind_to_pose,hand[:3,3]); posed_elbow=point(forearm_to_pose,elbow[:3,3])
    posed_shoulder=point(upper_to_pose,shoulder[:3,3])
    upper_axis=posed_elbow-posed_shoulder; lower_axis=posed_wrist-posed_elbow
    source_to_pose=bind_to_pose@src_to_bind
    cuff_source=source_global[:3,:3]@np.array(extras['hordeCuffAxis'])
    cuff_bind=unit(src_to_bind[:3,:3]@cuff_source)
    cuff_posed=unit(source_to_pose[:3,:3]@cuff_source)
    result[side]=dict(sourceToBind=mount_stats,handSkin=skin_stats,forearmSkin=forearm_stats,
        sourceToBindMatrix=src_to_bind.tolist(),sourceToPoseMatrix=source_to_pose.tolist(),
        cuffBind=cuff_bind.tolist(),cuffPosed=cuff_posed.tolist(),
        cuffVsBindProximal=float(cuff_bind@unit(elbow[:3,3]-hand[:3,3])),
        cuffVsPosedProximal=float(cuff_posed@unit(posed_elbow-posed_wrist)),
        upperSkin=upper_stats,
        elbowFlexionDegrees=float(np.degrees(np.arccos(np.clip(unit(upper_axis)@unit(lower_axis),-1,1)))),
        reachFraction=float(np.linalg.norm(posed_wrist-posed_shoulder)/(np.linalg.norm(upper_axis)+np.linalg.norm(lower_axis))),
        wristContinuityMetres=float(np.linalg.norm(posed_wrist-point(forearm_to_pose,hand[:3,3]))))
print('SOURCE_MOUNT_TRACE '+json.dumps(result,sort_keys=True))
