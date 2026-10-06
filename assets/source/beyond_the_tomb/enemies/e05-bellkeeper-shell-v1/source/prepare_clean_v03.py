"""Accepted v03: one small rear-neck patch, measured 2m normalization, no emission, 1K PBR.
No surgery to clothing, face, limbs or props. Provider source and lossless maps stay unchanged.
"""
import bpy,bmesh,json,sys,argparse,hashlib
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.geometry import barycentric_transform
import math
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--source-dir',required=True);p.add_argument('--output-dir',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);src=Path(a.source_dir);out=Path(a.output_dir);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(src/'model.glb'))
o=next(o for o in bpy.context.scene.objects if o.type=='MESH');bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
co=np.array([v.co[:] for v in o.data.vertices]);lo=co.min(0);hi=co.max(0);s=2/(hi[2]-lo[2]);shift=np.array([-(hi[0]+lo[0])/2,0,-lo[2]])
for v in o.data.vertices:v.co=Vector((np.array(v.co)+shift)*s)
o.data.update();bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);uvlayer=bm.loops.layers.uv.active
boundary_before=sum(e.is_boundary for e in bm.edges)
ne=[e for e in bm.edges if e.is_boundary and all(.010<v.co.x<.080 and .03<v.co.y<.11 and 1.72<v.co.z<1.83 for v in e.verts)]
# Fill only closed small cycles in this local boundary cluster. The adjacent collar opening is retained.
filled=[]; patch_errors=[]
for seed in list(ne):
    if not seed.is_boundary:continue
    start,goal=seed.verts;queue=[(start,[start])];seen={start};path=None
    while queue:
        current,route=queue.pop(0)
        for edge in current.link_edges:
            if edge==seed or edge not in ne or not edge.is_boundary:continue
            nxt=edge.other_vert(current)
            if nxt==goal:path=route+[nxt];queue=[];break
            if nxt not in seen:seen.add(nxt);queue.append((nxt,route+[nxt]))
        if path:break
    if path and 3<=len(path)<=12:
        try:filled.append(bm.faces.new(path))
        except ValueError as e:patch_errors.append(str(e))
# Reconstruct only the tiny defective nape sector from the intact opposite nape.
# Both geometry and UVs follow the observed clean counterpart rather than crushing arbitrary neck vertices.
dco=[];duv=[];dface=[]
for face in bm.faces:
    if face in filled:continue
    cc=face.calc_center_median()
    if cc.x<-.008 and cc.y>.020 and 1.69<cc.z<1.87:
        for k in range(1,len(face.loops)-1):
            ids=[]
            for loop in [face.loops[0],face.loops[k],face.loops[k+1]]:
                ids.append(len(dco));dco.append(loop.vert.co.copy());duv.append(loop[uvlayer].uv.copy())
            dface.append(tuple(ids))
dbvh=BVHTree.FromPolygons(dco,dface,all_triangles=True) if dface else None
patch_vertex_changes=[];nape_smooth=[];mirror_plane=-.004
if dbvh:
    for v in bm.verts:
        x,y,z=v.co
        if .005<x<.09 and .025<y<.115 and 1.72<z<1.835:
            target=Vector((2*mirror_plane-x,y,z));hit,normal,index,distance=dbvh.find_nearest(target)
            if distance<.08:
                replacement=Vector((2*mirror_plane-hit.x,hit.y,hit.z))
                w=min(1,(x-.005)/.008,(.09-x)/.014,(z-1.72)/.014,(1.835-z)/.014)
                before=list(v.co);v.co=v.co.lerp(replacement,max(0,w));patch_vertex_changes.append({'before':before,'after':list(v.co)})
    for face in bm.faces:
        cc=face.calc_center_median()
        if face in filled or (.005<cc.x<.09 and .025<cc.y<.115 and 1.725<cc.z<1.83):
            face.smooth=True;face.material_index=1
            for loop in face.loops:
                target=loop.vert.co.copy();target.x=2*mirror_plane-target.x
                hit,normal,index,distance=dbvh.find_nearest(target);ids=dface[index];aa,bb,cc=[dco[i] for i in ids];ua,ub,uc=[Vector((*duv[i],0)) for i in ids];mapped=barycentric_transform(hit,aa,bb,cc,ua,ub,uc);loop[uvlayer].uv=(mapped.x,mapped.y)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));boundary_after=sum(e.is_boundary for e in bm.edges);nonman_interior=sum(len(e.link_faces)>2 for e in bm.edges)
bm.to_mesh(o.data);bm.free();o.data.update()
repair_polygon_ids=[poly.index for poly in o.data.polygons if poly.material_index==1]
for poly in o.data.polygons:poly.use_smooth=True
# Preserve provider UVs; new patch UV corners are interpolated from the immediate neck boundary.
N=1024;mask=np.zeros((N,N),bool);o.data.calc_loop_triangles();uv=o.data.uv_layers.active
for tri in o.data.loop_triangles:
    cc=sum((o.data.vertices[i].co for i in tri.vertices),Vector())/3
    if not (cc.z<.24 or (abs(cc.x)>.40 and .90<cc.z<1.28)):continue
    q=np.array([uv.data[i].uv[:] for i in tri.loops])*(N-1);x0=max(0,int(np.floor(q[:,0].min())));x1=min(N-1,int(np.ceil(q[:,0].max())));y0=max(0,int(np.floor(q[:,1].min())));y1=min(N-1,int(np.ceil(q[:,1].max())))
    if x0>x1 or y0>y1:continue
    xx,yy=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5);v0,v1,v2=q;d=(v1[1]-v2[1])*(v0[0]-v2[0])+(v2[0]-v1[0])*(v0[1]-v2[1])
    if abs(d)<1e-8:continue
    aa=((v1[1]-v2[1])*(xx-v2[0])+(v2[0]-v1[0])*(yy-v2[1]))/d;bb=((v2[1]-v0[1])*(xx-v2[0])+(v0[0]-v2[0])*(yy-v2[1]))/d;mask[y0:y1+1,x0:x1+1]|=(aa>=-.002)&(bb>=-.002)&(aa+bb<=1.002)
images=[]
def load(name,filename,noncolor=False):
    im=bpy.data.images.load(str(src/filename),check_existing=False);original=list(im.size);im.name=name
    if noncolor:im.colorspace_settings.name='Non-Color'
    im.scale(N,N);images.append({'name':name,'source_file':filename,'source_size':original,'size':[N,N],'source_sha256':hashlib.sha256((src/filename).read_bytes()).hexdigest()});return im
base=load('E05Body_BaseColor_1K','texture_0_base_color.png')
normal=load('E05Body_Normal_1K','texture_0_normal.png',True)
rough=load('E05Body_Roughness_Input','texture_0_roughness.png',True);rp=np.array(rough.pixels[:],dtype=np.float32).reshape(N,N,4)[:,:,0];rp[mask]=np.maximum(rp[mask],.65)
orm=bpy.data.images.new('E05Body_ORM_1K',width=N,height=N,alpha=False);orm.colorspace_settings.name='Non-Color';pix=np.stack([np.ones_like(rp),rp,np.zeros_like(rp),np.ones_like(rp)],axis=-1).astype(np.float32);orm.pixels.foreach_set(pix.ravel());orm.update()
# A small UV tile in verified-unused atlas space avoids stretched skin/collar seam samples on repaired faces.
occupancy=np.zeros((N,N),bool)
for tri in o.data.loop_triangles:
    q=np.array([uv.data[i].uv[:] for i in tri.loops])*(N-1);x0=max(0,int(np.floor(q[:,0].min())));x1=min(N-1,int(np.ceil(q[:,0].max())));y0=max(0,int(np.floor(q[:,1].min())));y1=min(N-1,int(np.ceil(q[:,1].max())))
    if x0>x1 or y0>y1:continue
    xx,yy=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5);v0,v1,v2=q;d=(v1[1]-v2[1])*(v0[0]-v2[0])+(v2[0]-v1[0])*(v0[1]-v2[1])
    if abs(d)<1e-8:continue
    aa=((v1[1]-v2[1])*(xx-v2[0])+(v2[0]-v1[0])*(yy-v2[1]))/d;bb=((v2[1]-v0[1])*(xx-v2[0])+(v0[0]-v2[0])*(yy-v2[1]))/d;occupancy[y0:y1+1,x0:x1+1]|=(aa>=-.01)&(bb>=-.01)&(aa+bb<=1.01)
tile=None;S=40
for yy in range(4,N-S-4,4):
    for xx in range(4,N-S-4,4):
        if not occupancy[yy-3:yy+S+3,xx-3:xx+S+3].any():tile=(xx,yy);break
    if tile:break
assert tile,'No unused atlas tile for the small neck repair'
bp=np.array(base.pixels[:],dtype=np.float32).reshape(N,N,4);npix=np.array(normal.pixels[:],dtype=np.float32).reshape(N,N,4);opix=np.array(orm.pixels[:],dtype=np.float32).reshape(N,N,4)
samples={'skin':[],'cloth':[]}
for poly in o.data.polygons:
    cc=poly.center
    key='skin' if 1.785<cc.z<1.835 else ('cloth' if 1.68<cc.z<1.735 else None)
    if key and cc.x<-.008 and cc.y>.02:
        uu=np.mean([uv.data[i].uv[:] for i in poly.loop_indices],axis=0);samples[key].append(bp[min(N-1,max(0,int(uu[1]*N))),min(N-1,max(0,int(uu[0]*N))),:3])
skin=np.median(samples['skin'],axis=0);cloth=np.median(samples['cloth'],axis=0)
for ytile in range(S):
    z=1.70+ytile/(S-1)*.17;alpha=np.clip((z-1.750)/.012,0,1);colour=cloth*(1-alpha)+skin*alpha
    bp[tile[1]+ytile,tile[0]:tile[0]+S,:3]=colour
    npix[tile[1]+ytile,tile[0]:tile[0]+S,:3]=(.5,.5,1)
    opix[tile[1]+ytile,tile[0]:tile[0]+S,:3]=(1,.78-.18*alpha,0)
for pi in repair_polygon_ids:
    poly=o.data.polygons[pi]
    for li in poly.loop_indices:
        v=o.data.vertices[o.data.loops[li].vertex_index].co;u=np.clip(v.x/.10,.04,.96);vv=np.clip((v.z-1.70)/.17,.04,.96);uv.data[li].uv=((tile[0]+u*(S-1))/(N-1),(tile[1]+vv*(S-1))/(N-1))
    poly.material_index=0
for im,arr in [(base,bp),(normal,npix),(orm,opix)]:im.pixels.foreach_set(arr.ravel());im.update()
neck_tile={'rect_pixels':[tile[0],tile[1],S,S],'source_occupancy_verified_empty_with_padding':True,'faces':len(repair_polygon_ids),'skin_rgb_sample':skin.tolist(),'cloth_rgb_sample':cloth.tolist(),'note':'Small original repair swatch sampled from adjacent existing skin/cloth; neutral normal and physically nonmetal roughness; all other texture pixels preserved before 1K resampling.'}

# Reconstruct one non-emissive opaque material. Cloth, skin, gloves and boots are nonmetal.
mat=bpy.data.materials.new('E05_PlainInnerBody_1K');mat.use_nodes=True;mat.use_backface_culling=False;n=mat.node_tree.nodes;n.clear();ln=mat.node_tree.links
bs=n.new('ShaderNodeBsdfPrincipled');output=n.new('ShaderNodeOutputMaterial');ln.new(bs.outputs['BSDF'],output.inputs['Surface']);bs.inputs['Emission Color'].default_value=(0,0,0,1);bs.inputs['Emission Strength'].default_value=0
bn=n.new('ShaderNodeTexImage');bn.image=base;ln.new(bn.outputs['Color'],bs.inputs['Base Color']);on=n.new('ShaderNodeTexImage');on.image=orm;sep=n.new('ShaderNodeSeparateColor');ln.new(on.outputs['Color'],sep.inputs[0]);ln.new(sep.outputs['Green'],bs.inputs['Roughness']);ln.new(sep.outputs['Blue'],bs.inputs['Metallic'])
nn=n.new('ShaderNodeTexImage');nn.image=normal;nm=n.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.70;ln.new(nn.outputs['Color'],nm.inputs['Color']);ln.new(nm.outputs['Normal'],bs.inputs['Normal'])
o.data.materials.clear();o.data.materials.append(mat)
for im,name in [(base,'e05_body_basecolor'),(orm,'e05_body_orm'),(normal,'e05_body_normal')]:im.filepath_raw=str(out/(name+'.png'));im.file_format='PNG';im.save();im.pack()
# Drop unused imported/generated emission from this derivative only.
for im in list(bpy.data.images):
    if im not in [base,orm,normal]:bpy.data.images.remove(im)
o.name='E05_PlainInnerBody_2m_1K';bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.export_scene.gltf(filepath=str(out/'bellkeeper_inner_2m_1k.glb'),export_format='GLB',use_selection=True,export_yup=True,export_apply=True,export_animations=False)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'bellkeeper_inner_2m_1k.blend'),compress=True);o.data.calc_loop_triangles()
report={'source':str(src/'model.glb'),'source_sha256':hashlib.sha256((src/'model.glb').read_bytes()).hexdigest(),'source_bounds_min':lo.tolist(),'source_bounds_max':hi.tolist(),'scale_to_2m':s,'offset_before_scale':shift.tolist(),'facing':'Confirmed Blender -Y / glTF +Z; no rotation','triangles':len(o.data.loop_triangles),'vertices_before_export_splits':len(o.data.vertices),'neck_patch':{'boundary_edges_selected':len(ne),'faces_added':len(filled),'patch_errors':patch_errors,'local_nape_vertex_adjustments':patch_vertex_changes,'smoothed_nape_interior_vertices':len(nape_smooth),'uv_repair':'Bounded nape shape from intact opposite side, with small verified-unused atlas repair tile; face texture unchanged','boundary_edges_before':boundary_before,'boundary_edges_after':boundary_after,'interior_edges_with_more_than_two_faces':nonman_interior,'region':'Small rear neck opening only; sleeve/hem/boot/mouth/eye joins intentionally retained'},'textures':images,'material':{'count':1,'alpha':'OPAQUE','emission':'none; original emission map and factor removed','normal_strength':.70,'ORM':'R=1 neutral AO; G=source roughness with glove/boot-only 0.65 floor; B=0 nonmetal','leather_mask_pixels':int(mask.sum()),'local_neck_texture_repair':neck_tile},'output_sha256':hashlib.sha256((out/'bellkeeper_inner_2m_1k.glb').read_bytes()).hexdigest(),'status':'Source candidate; standalone rear-neck visual check and fresh import required before rig'}
(out/'normalization-receipt.json').write_text(json.dumps(report,indent=2));print('CLEAN_V03_RESULT',json.dumps(report))
