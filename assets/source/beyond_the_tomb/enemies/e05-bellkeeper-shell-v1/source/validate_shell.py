"""Static source and fresh-import validation for the original Bellkeeper shell.
Does not claim native engine import, gameplay correctness or full motion clearance.
"""
import bpy,bmesh,json,struct,sys,argparse,hashlib,math
from pathlib import Path
from mathutils import Vector
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--asset-dir',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);out=Path(a.asset_dir)
blob=(out/'bellkeeper_modular_shell.glb').read_bytes();magic,version,length=struct.unpack_from('<4sII',blob)
assert magic==b'glTF' and version==2 and length==len(blob)
chunks=[];offset=12
while offset<len(blob):
    n,t=struct.unpack_from('<I4s',blob,offset);chunks.append((t,blob[offset+8:offset+8+n]));offset+=8+n
j=json.loads(next(b for t,b in chunks if t==b'JSON'));binary=next(b for t,b in chunks if t==b'BIN\0')
(out/'evidence'/'gltf_document.json').write_text(json.dumps(j,indent=2))
components={5120:'i1',5121:'u1',5122:'<i2',5123:'<u2',5125:'<u4',5126:'<f4'}
sizes={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
def read_accessor(idx):
    a=j['accessors'][idx];v=j['bufferViews'][a['bufferView']];d=np.dtype(components[a['componentType']]);w=sizes[a['type']];stride=v.get('byteStride',w*d.itemsize);start=v.get('byteOffset',0)+a.get('byteOffset',0)
    return np.ndarray((a['count'],w),dtype=d,buffer=binary,offset=start,strides=(stride,d.itemsize)).copy()
report={'glb_sha256':hashlib.sha256(blob).hexdigest(),'file_bytes':len(blob),'glb_header_valid':True,'meshes':[],'external_dependencies':[],'animations':[],
 'materials':len(j.get('materials',[])),'images':len(j.get('images',[])),'source_meshes':[],
 'limitations':['No game-dev CLI was installed; direct Blender and independent static GLB checks only.','No native The Horde RT engine import or runtime light/attack verification.','Closed/open inspection keyframes are not proposed gameplay timing.']}
for collection in ['images','buffers']:
    for x in j.get(collection,[]):
        if x.get('uri') and not x['uri'].startswith('data:'):report['external_dependencies'].append(x['uri'])
triangles=0;finite=True;degenerate=0;uv_bad=0
for mi,m in enumerate(j.get('meshes',[])):
    entry={'name':m.get('name',str(mi)),'primitives':[]}
    for p in m['primitives']:
        pos=read_accessor(p['attributes']['POSITION']);idx=read_accessor(p['indices']).ravel().reshape(-1,3);n=len(idx);triangles+=n
        area=np.linalg.norm(np.cross(pos[idx[:,1]]-pos[idx[:,0]],pos[idx[:,2]]-pos[idx[:,0]]),axis=1)*.5
        finite=finite and bool(np.isfinite(pos).all());degenerate+=int((area<1e-12).sum())
        uv=read_accessor(p['attributes']['TEXCOORD_0']) if 'TEXCOORD_0' in p['attributes'] else None
        uv_bad+=int(((uv<-.00001)|(uv>1.00001)).any(axis=1).sum()) if uv is not None else len(pos)
        entry['primitives'].append({'triangles':n,'vertices':len(pos),'finite':bool(np.isfinite(pos).all()),'degenerate_triangles':int((area<1e-12).sum()),'uv_present':uv is not None})
    report['meshes'].append(entry)
for an in j.get('animations',[]):
    report['animations'].append({'name':an.get('name'),'channels':[{'node':j['nodes'][ch['target']['node']].get('name'),'path':ch['target']['path']} for ch in an['channels']]})
report.update(shell_triangles=triangles,finite_positions=finite,degenerate_triangles=degenerate,vertices_outside_uv_01=uv_bad)
bpy.ops.wm.open_mainfile(filepath=str(out/'bellkeeper_modular_shell.blend'))
for o in bpy.data.collections['ORIGINAL_MODULAR_SHELL'].objects:
    if o.type!='MESH':continue
    bm=bmesh.new();bm.from_mesh(o.data);nonman=sum(not e.is_manifold for e in bm.edges);boundary=sum(e.is_boundary for e in bm.edges)
    report['source_meshes'].append({'name':o.name,'vertices':len(bm.verts),'edges':len(bm.edges),'faces':len(bm.faces),'boundary_edges':boundary,'nonmanifold_edges':nonman});bm.free()
source_points=[o.matrix_world@v.co for o in bpy.data.collections['ORIGINAL_MODULAR_SHELL'].objects if o.type=='MESH' for v in o.data.vertices]
report['source_closed_bounds']={'min':[min(p[i] for p in source_points) for i in range(3)],'max':[max(p[i] for p in source_points) for i in range(3)]}
# Retain render studio but remove core and source shell; then fresh import the exact final GLB.
for name in ['CORE_FIT_REFERENCE','ORIGINAL_MODULAR_SHELL']:
    for ob in list(bpy.data.collections[name].objects):bpy.data.objects.remove(ob,do_unlink=True)
bpy.ops.import_scene.gltf(filepath=str(out/'bellkeeper_modular_shell.glb'))
imported=[o for o in bpy.context.scene.objects if o.type=='MESH']
ipt=[o.matrix_world@v.co for o in imported for v in o.data.vertices]
report['fresh_import']={'success':True,'mesh_objects':len(imported),'materials':sorted(set(m.name for o in imported for m in o.data.materials)),'bounds':{'min':[min(p[i] for p in ipt) for i in range(3)],'max':[max(p[i] for p in ipt) for i in range(3)]}}
scene=bpy.context.scene;scene.frame_set(1);scene.render.filepath=str(out/'previews'/'06_fresh_shell_import.jpg');scene.camera.location=(2.6,-5,2.5);scene.camera.rotation_euler=(Vector((0,0,1.55))-scene.camera.location).to_track_quat('-Z','Y').to_euler();scene.camera.data.ortho_scale=1.7;bpy.ops.render.render(write_still=True)
report['checks_pass']={'finite':finite,'nondegenerate':degenerate==0,'valid_uv_bounds':uv_bad==0,'one_shell_material':report['materials']==1,'embedded_resources':not report['external_dependencies'],'source_mesh_manifold':all(o['nonmanifold_edges']==0 for o in report['source_meshes']),'within_3k_shell_target':triangles<=3000,'fresh_import':True}
(out/'evidence'/'validation.json').write_text(json.dumps(report,indent=2));print('VALIDATION_RESULT',json.dumps(report['checks_pass']));print('TRIANGLES',triangles)
