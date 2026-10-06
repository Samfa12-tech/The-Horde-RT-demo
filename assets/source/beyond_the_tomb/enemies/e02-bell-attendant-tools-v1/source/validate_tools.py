"""Direct source/static glTF/fresh Blender import evidence. Not engine certification.
Run: blender -b --python validate_tools.py -- --asset-dir /absolute/path
"""
import argparse,hashlib,json,math,struct,sys
from pathlib import Path
import bpy,bmesh
import numpy as np
from mathutils import Vector
ap=argparse.ArgumentParser();ap.add_argument('--asset-dir',required=True)
ROOT=Path(ap.parse_args(sys.argv[sys.argv.index('--')+1:]).asset_dir)
report={'blender_version':bpy.app.version_string,'validation_scope':'Original source, embedded GLB payload, and fresh Blender import only','assets':{},'limitations':['game-dev CLI is unavailable; no canonical game-dev package receipt.','No Khronos validator run.','No engine import, GPU/device measurement, H1 socket fit, collision, audio or combat timing test.','Clapper rest transform and editable pivot only; no animation or physics.','Intentional intersecting closed components at mechanical joints; hidden bores not modeled.','Opaque material uses procedural normal texture, not high-poly baking; ORM AO channel is neutral.']}

def source_check(o):
 m=o.data;m.calc_loop_triangles();bm=bmesh.new();bm.from_mesh(m);bm.verts.ensure_lookup_table();bm.verts.index_update()
 seen=set();vol=[]
 for v in bm.verts:
  if v.index in seen:continue
  stack=[v];component=set()
  while stack:
   w=stack.pop()
   if w.index in seen:continue
   seen.add(w.index);component.add(w);stack.extend(e.other_vert(w) for e in w.link_edges)
  faces={f for q in component for f in q.link_faces};sv=0
  for f in faces:
   pts=[q.co for q in f.verts]
   for i in range(1,len(pts)-1):sv+=pts[0].dot(pts[i].cross(pts[i+1]))/6
  vol.append(sv)
 uv=m.uv_layers.active;degen_uv=0;degen_geom=0;dots=[]
 for t in m.loop_triangles:
  q=[m.vertices[i].co for i in t.vertices];cross=(q[1]-q[0]).cross(q[2]-q[0]);degen_geom+=int(cross.length<1e-12)
  if uv:
   a,b,c=[uv.data[i].uv for i in t.loops];degen_uv+=int(abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<1e-12)
 out={'vertices':len(m.vertices),'polygons':len(m.polygons),'triangles':len(m.loop_triangles),'connected_shells':len(vol),'signed_shell_volumes_m3':vol,'all_shell_volumes_positive':all(v>1e-12 for v in vol),'boundary_edges':sum(e.is_boundary for e in bm.edges),'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),'inconsistent_winding_edges':sum(e.is_manifold and not e.is_contiguous for e in bm.edges),'loose_vertices':sum(not v.link_edges for v in bm.verts),'degenerate_triangles':degen_geom,'nonfinite_positions':sum(not all(math.isfinite(c) for c in v.co) for v in m.vertices),'has_uv':bool(uv),'degenerate_uv_triangles':degen_uv,'uv_outside_unit':sum(not(-1e-7<=c<=1+1e-7) for d in uv.data for c in d.uv) if uv else -1,'uv_nonfinite':sum(not math.isfinite(c) for d in uv.data for c in d.uv) if uv else -1,'material_slots':len(m.materials),'local_origin_m':list(o.location),'rotation_xyz_rad':list(o.rotation_euler),'scale':list(o.scale)}
 bm.free();return out

def good_source(s):
 return s['nonmanifold_edges']==0 and s['inconsistent_winding_edges']==0 and s['all_shell_volumes_positive'] and s['degenerate_triangles']==0 and s['nonfinite_positions']==0 and s['degenerate_uv_triangles']==0 and s['uv_outside_unit']==0 and s['uv_nonfinite']==0 and s['has_uv']

def world_bounds(objects):
 p=np.array([list(o.matrix_world@v.co) for o in objects for v in o.data.vertices]);return np.stack([p.min(axis=0),p.max(axis=0)],axis=1).tolist()

def glb_check(path):
 raw=path.read_bytes();magic,version,length=struct.unpack_from('<III',raw);assert magic==0x46546c67 and version==2 and length==len(raw)
 jl,jt=struct.unpack_from('<II',raw,12);assert jt==0x4e4f534a;doc=json.loads(raw[20:20+jl]);bp=20+jl;bl,bt=struct.unpack_from('<II',raw,bp);assert bt==0x004e4942;binary=raw[bp+8:bp+8+bl]
 type_map={5121:np.uint8,5123:np.uint16,5125:np.uint32,5126:np.float32};widths={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
 def acc(i):
  a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];dt=np.dtype(type_map[a['componentType']]);w=widths[a['type']]
  return np.ndarray((a['count'],w),dtype=dt,buffer=binary,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',w*dt.itemsize),dt.itemsize)).copy()
 out={'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'mesh_count':len(doc['meshes']),'material_count':len(doc['materials']),'image_count':len(doc['images']),'external_uris':[v['uri'] for key in ['buffers','images'] for v in doc.get(key,[]) if 'uri' in v],'extensions_used':doc.get('extensionsUsed',[]),'materials':doc['materials'],'images':[],'primitives':[],'nodes':doc['nodes']}
 for im in doc['images']:
  bv=doc['bufferViews'][im['bufferView']];data=binary[bv.get('byteOffset',0):bv.get('byteOffset',0)+bv['byteLength']]
  dims=list(struct.unpack_from('>II',data,16)) if data[:8]==b'\x89PNG\r\n\x1a\n' else None
  out['images'].append({'name':im.get('name'),'bytes':len(data),'dimensions':dims,'mime':im.get('mimeType'),'sha256':hashlib.sha256(data).hexdigest()})
 for mesh in doc['meshes']:
  for p in mesh['primitives']:
   a=p['attributes'];pos=acc(a['POSITION']);norm=acc(a['NORMAL']);uv=acc(a['TEXCOORD_0']);t=acc(a['TANGENT']) if 'TANGENT'in a else None;idx=acc(p['indices']).ravel().reshape(-1,3)
   geo=np.cross(pos[idx[:,1]]-pos[idx[:,0]],pos[idx[:,2]]-pos[idx[:,0]]);size=np.linalg.norm(geo,axis=1);unit=geo/np.maximum(size[:,None],1e-20);dot=np.einsum('tvi,ti->tv',norm[idx],unit)
   u=uv[idx];area=np.abs((u[:,1,0]-u[:,0,0])*(u[:,2,1]-u[:,0,1])-(u[:,1,1]-u[:,0,1])*(u[:,2,0]-u[:,0,0]))
   out['primitives'].append({'mesh_name':mesh.get('name'),'vertices_after_attribute_splits':len(pos),'triangles':len(idx),'indices_valid':bool(np.all((idx>=0)&(idx<len(pos)))),'finite_positions':bool(np.isfinite(pos).all()),'finite_normals':bool(np.isfinite(norm).all()),'normal_length_minmax':[float(np.linalg.norm(norm,axis=1).min()),float(np.linalg.norm(norm,axis=1).max())],'normal_face_dot_min':float(dot.min()),'opposed_vertex_normals':int((dot<=0).sum()),'degenerate_triangles':int((size<1e-12).sum()),'has_tangents':t is not None,'tangents_finite':bool(np.isfinite(t).all()) if t is not None else False,'normal_tangent_abs_dot_max':float(np.abs((norm*t[:,:3]).sum(axis=1)).max()) if t is not None else None,'tangent_w_values':[float(v) for v in np.unique(t[:,3])] if t is not None else [],'uv_unit_square':bool(((uv>=0)&(uv<=1)).all()),'uv_finite':bool(np.isfinite(uv).all()),'degenerate_uv_triangles':int((area<1e-12).sum())})
 out['total_triangles']=sum(p['triangles'] for p in out['primitives']);out['total_split_vertices']=sum(p['vertices_after_attribute_splits'] for p in out['primitives'])
 out['policy_pass']=out['total_triangles']<=2000 and out['material_count']==1 and out['image_count']==3 and not out['external_uris'] and all(i['dimensions']==[512,512] for i in out['images']) and all(m.get('alphaMode','OPAQUE')=='OPAQUE' and not m.get('doubleSided',False) for m in doc['materials']) and all(p['indices_valid'] and p['finite_positions'] and p['finite_normals'] and p['opposed_vertex_normals']==0 and p['degenerate_triangles']==0 and p['has_tangents'] and p['tangents_finite'] and p['uv_unit_square'] and p['uv_finite'] and p['degenerate_uv_triangles']==0 for p in out['primitives'])
 (ROOT/'evidence'/(path.stem+'_gltf_document.json')).write_text(json.dumps(doc,indent=2))
 return out

def import_render(name,target,loc,scale):
 s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=48;s.cycles.use_denoising=False;s.render.resolution_x=900;s.render.resolution_y=900;s.render.resolution_percentage=100;s.render.image_settings.file_format='PNG'
 s.world=bpy.data.worlds.new('FreshImportWorld');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.065,.074,.082,1);s.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.55;s.view_settings.view_transform='AgX';s.view_settings.look='AgX - Medium High Contrast'
 for nm,xyz,power,size,col in [('Key',(-.45,-.55,.65),20,.65,(1,.91,.81)),('Fill',(.5,-.25,.1),11,.5,(.8,.88,1)),('Rim',(0,.5,.45),19,.5,(.93,1,.96))]:
  ld=bpy.data.lights.new(nm,'AREA');ld.energy=power;ld.shape='DISK';ld.size=size;ld.color=col;o=bpy.data.objects.new(nm,ld);bpy.context.collection.objects.link(o);o.location=xyz;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
 cd=bpy.data.cameras.new('FreshImportCamera');cam=bpy.data.objects.new('FreshImportCamera',cd);bpy.context.collection.objects.link(cam);s.camera=cam;cd.type='ORTHO';cd.ortho_scale=scale;cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
 if 'handbell' in name:
  ld=bpy.data.lights.new('Inspection_underfill','AREA');ld.energy=4;ld.shape='DISK';ld.size=.25;o=bpy.data.objects.new('Inspection_underfill',ld);bpy.context.collection.objects.link(o);o.location=(.1,-.28,-.40);o.rotation_euler=(Vector((0,0,-.1))-o.location).to_track_quat('-Z','Y').to_euler()
 s.render.filepath=str(ROOT/'previews'/name);bpy.ops.render.render(write_still=True)

for stem,kind in [('abbey_bell_attendant_handbell','handbell'),('abbey_bell_attendant_short_staff','short_staff')]:
 bpy.ops.wm.open_mainfile(filepath=str(ROOT/(stem+'.blend')))
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];bpy.context.view_layer.update();src={o.name:source_check(o) for o in meshes};bounds=world_bounds(meshes)
 packed=[{'name':im.name,'packed':bool(im.packed_file),'size':list(im.size),'colorspace':im.colorspace_settings.name} for im in bpy.data.images if im.source=='FILE']
 roots=[{'name':o.name,'location':list(o.location),'rotation':list(o.rotation_euler),'scale':list(o.scale)} for o in bpy.context.scene.objects if o.type=='EMPTY']
 g=glb_check(ROOT/(stem+'.glb'))
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(ROOT/(stem+'.glb')))
 imported=[o for o in bpy.context.scene.objects if o.type=='MESH'];bpy.context.view_layer.update();import_bounds=world_bounds(imported);raws={o.name:source_check(o) for o in imported};welds={}
 for o in imported:
  bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=bm.verts,dist=1e-7);me=bpy.data.meshes.new('ValidationTemporary');bm.to_mesh(me);bm.free();v=bpy.data.objects.new('ValidationTemporary',me);bpy.context.collection.objects.link(v);welds[o.name]=source_check(v);bpy.data.objects.remove(v,do_unlink=True);bpy.data.meshes.remove(me)
 fresh={'succeeded':True,'mesh_count':len(imported),'image_count':len(bpy.data.images),'bounds_blender_xyz_m':import_bounds,'bounds_delta_max_m':float(np.max(np.abs(np.array(bounds)-np.array(import_bounds)))),'raw_meshes':raws,'temporary_position_weld_meshes':welds,'attribute_split_note':'glTF vertex normals and UV seams deliberately split positions. Weld is validation-only, not applied to deliverable.'}
 passed=all(good_source(v) for v in src.values()) and all(good_source(v) for v in welds.values()) and g['policy_pass'] and fresh['bounds_delta_max_m']<1e-6 and all(i['packed'] for i in packed) and len(packed)==3
 a={'result':'PASS' if passed else 'FAIL','source_meshes':src,'source_bounds_blender_xyz_m':bounds,'source_dimensions_m':[b[1]-b[0] for b in bounds],'packed_source_images':packed,'root_transforms':roots,'static_glb':g,'fresh_import':fresh}
 if kind=='handbell':name='09_handbell_fresh_glb_import.png';import_render(name,(0,0,-.059),(.31,-.55,-.34),.365)
 else:name='10_staff_fresh_glb_import.png';import_render(name,(0,0,.11),(.55,-1.3,.37),1.01)
 fresh['render']='previews/'+name;report['assets'][kind]=a
 (ROOT/'evidence'/'validation.json').write_text(json.dumps(report,indent=2))
report['result']='PASS' if all(v['result']=='PASS' for v in report['assets'].values()) else 'FAIL'
(ROOT/'evidence'/'validation.json').write_text(json.dumps(report,indent=2))
print('VALIDATION_RESULT',report['result'])
for k,v in report['assets'].items():print(k,v['result'],v['static_glb']['total_triangles'],'triangles',v['static_glb']['total_split_vertices'],'split vertices')
if report['result']!='PASS':sys.exit(2)
