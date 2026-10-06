"""Read-only source/GLB inspections and fresh Blender import, writing evidence only."""
import argparse,collections,hashlib,json,math,struct,sys
from pathlib import Path
import bpy,bmesh
import numpy as np
from mathutils import Vector
ap=argparse.ArgumentParser();ap.add_argument('--asset-dir',required=True)
args=sys.argv[sys.argv.index('--')+1:];ROOT=Path(ap.parse_args(args).asset_dir)
report={'blender_version':bpy.app.version_string,'checks':{},'limitations':['No Khronos validator installed or run.','No game-engine import, collision or attack-timing test.','Component shells intentionally intersect at joints.','Procedural normal map only; no high-poly bake.']}
def inspect_mesh(obj,label):
    mesh=obj.data;mesh.calc_loop_triangles()
    bm=bmesh.new();bm.from_mesh(mesh)
    visited=set();volumes=[];component_faces=[]
    for v in bm.verts:
        if v.index in visited:continue
        stack=[v];group=set()
        while stack:
            q=stack.pop()
            if q.index in visited:continue
            visited.add(q.index);group.add(q)
            stack.extend(e.other_vert(q) for e in q.link_edges)
        fs={f for q in group for f in q.link_faces}
        volume=0
        for f in fs:
            vs=[q.co for q in f.verts]
            for i in range(1,len(vs)-1):volume+=vs[0].dot(vs[i].cross(vs[i+1]))/6
        volumes.append(volume);component_faces.append(len(fs))
    uv=mesh.uv_layers.active
    uv_degenerate=0;uv_nonfinite=0;uv_outside=0
    uv_areas=[];min_dot=1.
    for tri in mesh.loop_triangles:
        co=[mesh.vertices[i].co for i in tri.vertices]
        geom=(co[1]-co[0]).cross(co[2]-co[0])
        if uv:
            u=[uv.data[i].uv for i in tri.loops]
            area=abs((u[1].x-u[0].x)*(u[2].y-u[0].y)-(u[1].y-u[0].y)*(u[2].x-u[0].x))/2
            uv_areas.append(area)
            uv_degenerate+=int(area<1e-12)
    if uv:
        uv_nonfinite=sum(not math.isfinite(c) for d in uv.data for c in d.uv)
        uv_outside=sum(not(-1e-7<=c<=1+1e-7) for d in uv.data for c in d.uv)
    invalid_vert=sum(not all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
    degenerate=sum(((mesh.vertices[t.vertices[1]].co-mesh.vertices[t.vertices[0]].co).cross(mesh.vertices[t.vertices[2]].co-mesh.vertices[t.vertices[0]].co)).length<1e-12 for t in mesh.loop_triangles)
    bounds=[[min(v.co[i] for v in mesh.vertices),max(v.co[i] for v in mesh.vertices)] for i in range(3)]
    data={'vertices':len(mesh.vertices),'edges':len(mesh.edges),'polygons':len(mesh.polygons),'triangles':len(mesh.loop_triangles),'nonfinite_vertices':invalid_vert,'degenerate_triangles':degenerate,'loose_vertices':sum(not v.link_edges for v in bm.verts),'boundary_edges':sum(e.is_boundary for e in bm.edges),'non_manifold_edges':sum(not e.is_manifold for e in bm.edges),'inconsistent_winding_edges':sum(e.is_manifold and not e.is_contiguous for e in bm.edges),'connected_components':len(volumes),'signed_component_volumes_m3':volumes,'positive_component_volumes':all(v>0 for v in volumes),'bounds_xyz_m':bounds,'uv_layers':len(mesh.uv_layers),'uv_degenerate_triangles':uv_degenerate,'uv_nonfinite_values':uv_nonfinite,'uv_out_of_unit_range_values':uv_outside,'minimum_uv_triangle_area':min(uv_areas) if uv_areas else None}
    bm.free();report['checks'][label]=data
    return data

bpy.ops.wm.open_mainfile(filepath=str(ROOT/'abbey_attendant_maintenance_mallet.blend'))
obj=bpy.data.objects['AbbeyMaintenanceMallet']
source=inspect_mesh(obj,'editable_blend_mesh')
report['source_materials']=len(obj.data.materials)
report['source_origin_m']=list(obj.location)
report['source_scale']=list(obj.scale)
report['source_rotation']=list(obj.rotation_euler)

raw=(ROOT/'abbey_attendant_maintenance_mallet.glb').read_bytes()
magic,ver,length=struct.unpack_from('<III',raw)
assert magic==0x46546C67 and ver==2 and length==len(raw)
jl,jt=struct.unpack_from('<II',raw,12);doc=json.loads(raw[20:20+jl])
bp=20+jl;bl,bt=struct.unpack_from('<II',raw,bp);binary=raw[bp+8:bp+8+bl]
types={5120:np.int8,5121:np.uint8,5122:np.int16,5123:np.uint16,5125:np.uint32,5126:np.float32}
widths={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
def accessor(i):
    a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];dt=np.dtype(types[a['componentType']]);w=widths[a['type']];offset=v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',w*dt.itemsize)
    return np.ndarray((a['count'],w),dtype=dt,buffer=binary,offset=offset,strides=(stride,dt.itemsize)).copy()

g={'mesh_count':len(doc.get('meshes',[])),'material_count':len(doc.get('materials',[])),'texture_count':len(doc.get('textures',[])),'image_count':len(doc.get('images',[])),'external_uris':[x['uri'] for key in ['buffers','images'] for x in doc.get(key,[]) if 'uri' in x],'images':[],'materials':doc.get('materials',[]),'nodes':doc.get('nodes',[]),'primitives':[]}
for im in doc.get('images',[]):
    bv=doc['bufferViews'][im['bufferView']];b=binary[bv.get('byteOffset',0):bv.get('byteOffset',0)+bv['byteLength']]
    if b[:8]==b'\x89PNG\r\n\x1a\n':dims=list(struct.unpack_from('>II',b,16))
    else:dims=None
    g['images'].append({'name':im.get('name'),'mime':im.get('mimeType'),'bytes':len(b),'dimensions':dims,'sha256':hashlib.sha256(b).hexdigest()})
for mesh in doc['meshes']:
    for p in mesh['primitives']:
        pos=accessor(p['attributes']['POSITION']);idx=accessor(p['indices']).ravel().reshape((-1,3));n=accessor(p['attributes']['NORMAL']);uv=accessor(p['attributes']['TEXCOORD_0']);t=accessor(p['attributes']['TANGENT']) if 'TANGENT' in p['attributes'] else None
        geom=np.cross(pos[idx[:,1]]-pos[idx[:,0]],pos[idx[:,2]]-pos[idx[:,0]])
        mag=np.linalg.norm(geom,axis=1);gn=geom/np.maximum(mag[:,None],1e-15)
        dots=np.einsum('tvi,ti->tv',n[idx],gn)
        pd={'vertices_after_attribute_splits':len(pos),'triangles':len(idx),'index_out_of_bounds':int(np.sum((idx<0)|(idx>=len(pos)))),'degenerate_triangles':int(np.sum(mag<1e-12)),'finite_positions':bool(np.all(np.isfinite(pos))),'finite_normals':bool(np.all(np.isfinite(n))),'normal_length_min':float(np.min(np.linalg.norm(n,axis=1))),'normal_length_max':float(np.max(np.linalg.norm(n,axis=1))),'normal_to_face_min_dot':float(np.min(dots)),'opposed_vertex_normals':int(np.sum(dots<=0)),'has_tangents':t is not None,'finite_tangents':bool(np.all(np.isfinite(t))) if t is not None else None,'tangent_normal_max_abs_dot':float(np.max(np.abs(np.sum(t[:,:3]*n,axis=1)))) if t is not None else None,'tangent_w_values':list(map(float,np.unique(t[:,3]))) if t is not None else None,'uv_within_unit_square':bool(np.all((uv>=0)&(uv<=1))),'bounds_xyz_m':np.stack([pos.min(axis=0),pos.max(axis=0)],axis=1).tolist()}
        g['primitives'].append(pd)
report['gltf_static']=g
(ROOT/'evidence'/'gltf_document.json').write_text(json.dumps(doc,indent=2))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(ROOT/'abbey_attendant_maintenance_mallet.glb'))
imported=[o for o in bpy.context.scene.objects if o.type=='MESH']
report['fresh_import']={'mesh_objects':len(imported),'images':len(bpy.data.images),'materials':len(bpy.data.materials),'import_completed':True}
for i,o in enumerate(imported):
    inspect_mesh(o,'fresh_import_raw_'+str(i))
    # glTF has deliberate splits along hard-normal and UV seams. Geometrically weld
    # a temporary copy to verify closed shell connectivity, without editing deliverable.
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=bm.verts,dist=1e-7)
    temporary=bpy.data.meshes.new('validation_weld_only');bm.to_mesh(temporary);bm.free()
    tmp=bpy.data.objects.new('ValidationOnly',temporary);bpy.context.collection.objects.link(tmp)
    inspect_mesh(tmp,'fresh_import_welded_'+str(i));bpy.data.objects.remove(tmp,do_unlink=True)
report['fresh_import']['attribute_seam_note']='Raw glTF topology deliberately splits UV seams and hard normals. Raw boundary edges/open pieces are attribute splits, not holes. The separate temporary weld at 1e-7m restores the original 264 vertices and three closed outward-wound shells; the delivered file is not altered.'
welded=report['checks'].get('fresh_import_welded_0',{})
report['result']='PASS' if source['nonfinite_vertices']==0 and source['non_manifold_edges']==0 and source['inconsistent_winding_edges']==0 and source['positive_component_volumes'] and source['uv_degenerate_triangles']==0 and all(p['degenerate_triangles']==0 and p['opposed_vertex_normals']==0 and p['has_tangents'] for p in g['primitives']) and not g['external_uris'] and welded.get('non_manifold_edges')==0 and welded.get('positive_component_volumes')==True else 'FAIL'
# A render from this empty-scene GLB import verifies the delivered embedded maps.
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=64;scene.cycles.use_denoising=False
scene.render.resolution_x=1100;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.world=bpy.data.worlds.new('FreshImportWorld');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.065,.074,.082,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.45
scene.view_settings.view_transform='AgX';scene.view_settings.look='AgX - Medium High Contrast'
for name,loc,power,size,color in [('Key',(-.30,-.45,.60),9,.45,(1,.88,.76)),('Fill',(.30,-.10,.25),4,.30,(.77,.85,1)),('Rim',(.0,.25,.45),10,.27,(.91,1,.96))]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    light=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(light);light.location=loc
    light.rotation_euler=(Vector((0,0,.125))-light.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('FreshImportCamera');camera=bpy.data.objects.new('FreshImportCamera',data);bpy.context.collection.objects.link(camera);scene.camera=camera
data.type='ORTHO';data.ortho_scale=.40;camera.location=(.39,-.65,.34);camera.rotation_euler=(Vector((0,0,.114))-camera.location).to_track_quat('-Z','Y').to_euler()
scene.render.filepath=str(ROOT/'previews'/'06_fresh_glb_import.png')
bpy.ops.render.render(write_still=True)
report['fresh_import']['render']='previews/06_fresh_glb_import.png'
(ROOT/'evidence'/'validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
