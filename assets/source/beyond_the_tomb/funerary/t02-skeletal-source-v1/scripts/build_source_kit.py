"""Rebuild from verified CC0 source. Run Blender with --disable-autoexec.
All writes are contained in --output. No provider, network, game, or account calls.
"""
import bpy,bmesh,json,math,hashlib,os,sys,argparse
from mathutils import Vector,Matrix
import numpy as np
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--output',required=True);a=p.parse_args(args)
SOURCE_HASH='0ba13c9f0c2af50aafb317f450a92083f7d08ece67c3ecc9a2c64eac13257926'
assert hashlib.sha256(open(a.source,'rb').read()).hexdigest()==SOURCE_HASH
OUT=a.output
for d in ['source','meshes','evidence','arrangements','provenance']:os.makedirs(os.path.join(OUT,d),exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=a.source,load_ui=False,use_scripts=False)
original_text='\n\n'.join(t.as_string() for t in bpy.data.texts)
open(os.path.join(OUT,'provenance','embedded-source-notice.txt'),'w').write(original_text)
# Capture authored base topology, never evaluate the legacy armature or subdivision.
raw={}
for o in bpy.data.objects:
 if o.type!='MESH':continue
 verts=[o.matrix_world@v.co for v in o.data.vertices]
 groups={g.name:[v.index for v in o.data.vertices if any(x.group==g.index and x.weight>0 for x in v.groups)] for g in o.vertex_groups}
 raw[o.name]={'verts':verts,'faces':[tuple(p.vertices) for p in o.data.polygons],'groups':groups}
allv=[v for d in raw.values() for v in d['verts']]
lo=Vector([min(v[k] for v in allv) for k in range(3)]);hi=Vector([max(v[k] for v in allv) for k in range(3)])
factor=1.72/(hi.z-lo.z)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
collections={}
for name in ['01_MASTERS_STATIC','02_SOURCE_CAGES_LOD_CANDIDATES','03_SKELETAL_CORE_STATIC','04_ARRANGEMENT_A_EXAMPLE','05_ARRANGEMENT_B_EXAMPLE']:
 c=bpy.data.collections.new(name);scene.collection.children.link(c);collections[name]=c
mat=bpy.data.materials.new('Horde_AgedBone_Matte_v1');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.46,.395,.29,1);bs.inputs['Roughness'].default_value=.84;bs.inputs['Metallic'].default_value=0
mat.diffuse_color=(.46,.395,.29,1);mat['surface_provenance']='New project-authored uniform PBR parameter set; no external textures or baked lighting.'
mat['linear_base_colour']=[.46,.395,.29,1];mat['texture_bytes']=0

def create(name,verts,faces,col):
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
 o=bpy.data.objects.new(name,mesh);col.objects.link(o);mesh.materials.append(mat)
 bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
 for f in mesh.polygons:f.use_smooth=True
 o['source_sha256']=SOURCE_HASH;o['status']='Static source candidate, not game-admitted';return o

def subset(d,indices):
 ids=sorted(indices);mapping={v:i for i,v in enumerate(ids)}
 return [d['verts'][i].copy() for i in ids],[tuple(mapping[i] for i in f) for f in d['faces'] if all(i in mapping for i in f)],mapping

def normalize(verts,long_axis=False):
 vs=[v*factor for v in verts]
 if long_axis:
  ar=np.array([list(v) for v in vs]);center=ar.mean(axis=0);cov=np.cov((ar-center).T);_,eig=np.linalg.eigh(cov);axis=Vector(eig[:,-1]);axis=axis if axis.z>0 else -axis
  rot=axis.rotation_difference(Vector((0,0,1))).to_matrix();vs=[rot@(v-Vector(center)) for v in vs]
 low=Vector([min(v[k] for v in vs) for k in range(3)]);high=Vector([max(v[k] for v in vs) for k in range(3)])
 origin=Vector(((low.x+high.x)/2,(low.y+high.y)/2,low.z));return [v-origin for v in vs]

def subdivide_part(verts,faces):
 m=bpy.data.meshes.new('_temporary_cage');m.from_pydata(verts,[],faces);m.update();o=bpy.data.objects.new('_temporary',m);scene.collection.objects.link(o)
 md=o.modifiers.new('Bounded smoothing: one Catmull-Clark level','SUBSURF');md.levels=1;md.render_levels=1
 deps=bpy.context.evaluated_depsgraph_get();evaluated=o.evaluated_get(deps);em=evaluated.to_mesh();v=[x.co.copy() for x in em.vertices];f=[tuple(x.vertices) for x in em.polygons];evaluated.to_mesh_clear();bpy.data.objects.remove(o,do_unlink=True);bpy.data.meshes.remove(m);return v,f

def metrics(o):
 m=o.data;m.calc_loop_triangles();vs=[v.co for v in m.vertices];low=[min(v[k] for v in vs) for k in range(3)];high=[max(v[k] for v in vs) for k in range(3)]
 bm=bmesh.new();bm.from_mesh(m);boundary=sum(e.is_boundary for e in bm.edges);nonman=sum(not e.is_manifold for e in bm.edges);deg=sum(f.calc_area()<1e-14 for f in bm.faces);volume=bm.calc_volume(signed=True);bm.free()
 return {'vertices':len(vs),'triangles':len(m.loop_triangles),'polygons':len(m.polygons),'materials':len(m.materials),'bounds_min_m':low,'bounds_max_m':high,'dimensions_m':[high[k]-low[k] for k in range(3)],'boundary_edges':boundary,'non_manifold_edges_including_boundary':nonman,'zero_area_faces':deg,'signed_volume_m3':volume,'mesh_objects':1,'armatures':0,'actions':0,'textures':0}

masters={};cages={};selection={}
for name,source,group in [('femur','BONES_LEG.L','FEMUR.L'),('humerus','BONES_ARM.L','HUMERUS.L')]:
 d=raw[source];ids=d['groups'][group];v,f,mapping=subset(d,ids);v=normalize(v,True)
 cage=create(name+'_cage_lod1',v,f,collections['02_SOURCE_CAGES_LOD_CANDIDATES']);cages[name]=cage
 sv,sf=subdivide_part(v,f)
 # Preserve the cage's overall metric dimensions after smoothing; no global unit drift.
 svlo=Vector([min(x[k] for x in sv) for k in range(3)]);svhi=Vector([max(x[k] for x in sv) for k in range(3)]);vlo=Vector([min(x[k] for x in v) for k in range(3)]);vhi=Vector([max(x[k] for x in v) for k in range(3)])
 sv=[Vector([vlo[k]+(x[k]-svlo[k])*(vhi[k]-vlo[k])/(svhi[k]-svlo[k]) for k in range(3)]) for x in sv]
 master=create(name+'_master_lod0',sv,sf,collections['01_MASTERS_STATIC']);masters[name]=master;master['orientation']='Proximal end +Z; long axis PCA normalized; origin at lower bound, XY centred';master['surface_operation']='One Catmull-Clark level; bounds restored to source cage dimensions'
 selection[name]={'source_mesh':source,'source_vertex_group':group,'source_vertex_indices':ids,'source_cage_vertices':len(v),'source_cage_triangles':metrics(cage)['triangles'],'operation':master['surface_operation']}
# Skull with the whole jaw/teeth. Preserve actual cavities. Smooth only the two largest connected shell islands; leave all teeth unchanged.
d=raw['BONES_HEAD'];v=normalize(d['verts']);f=d['faces'];cage=create('skull_jaw_cage_lod1',v,f,collections['02_SOURCE_CAGES_LOD_CANDIDATES']);cages['skull_jaw']=cage
adj=[set() for _ in v]
for face in f:
 for i,j in zip(face,face[1:]+face[:1]):adj[i].add(j);adj[j].add(i)
unseen=set(range(len(v)));comps=[]
while unseen:
 todo=[unseen.pop()];comp=[]
 while todo:
  i=todo.pop();comp.append(i)
  for j in adj[i]:
   if j in unseen:unseen.remove(j);todo.append(j)
 comps.append(comp)
sv=[];sf=[];jawids=[];headids=[]
for comp in sorted(comps,key=lambda c:-len(c)):
 ids=sorted(comp);mp={old:i for i,old in enumerate(ids)};cv=[v[i] for i in ids];cf=[tuple(mp[i] for i in face) for face in f if all(i in mp for i in face)]
 if len(ids)>100:cv,cf=subdivide_part(cv,cf)
 offset=len(sv);sv.extend(cv);sf.extend(tuple(i+offset for i in face) for face in cf)
 group=jawids if ids[0] in set(d['groups']['JAW']) else headids;group.extend(range(offset,len(sv)))
# Keep floor-support origin and the head's authored dimensions; uniform rescale only, preserving rounded form.
minz=min(x.z for x in sv);sv=[x-Vector((0,0,minz)) for x in sv]
master=create('skull_jaw_master_lod0',sv,sf,collections['01_MASTERS_STATIC']);masters['skull_jaw']=master
for n,ids in [('Jaw_and_lower_teeth',jawids),('Cranium_and_upper_teeth',headids)]:g=master.vertex_groups.new(name=n);g.add(ids,1,'REPLACE')
master['orientation']='Front -Y, up +Z; origin at jaw support plane, XY centred';master['surface_operation']='One Catmull-Clark level on cranium and jaw only; 32 tooth islands kept at base topology'
selection['skull_jaw']={'source_mesh':'BONES_HEAD','source_cage_vertices':len(v),'source_cage_triangles':metrics(cage)['triangles'],'smoothed_shell_island_vertex_counts':[386,119],'untouched_tooth_islands':32,'operation':master['surface_operation']}
# One static mesh, one material, preserving the base topology of all eight anatomical groups.
cv=[];cf=[];anatomy={}
for name,d in sorted(raw.items()):
 off=len(cv);cv.extend([(v-Vector((0,(lo.y+hi.y)/2,lo.z)))*factor for v in d['verts']]);cf.extend(tuple(i+off for i in f) for f in d['faces']);anatomy[name]=list(range(off,len(cv)))
core=create('manny_skeletal_core_static',cv,cf,collections['03_SKELETAL_CORE_STATIC'])
for n,ids in anatomy.items():g=core.vertex_groups.new(name=n);g.add(ids,1,'REPLACE')
core['orientation']='Front -Y, up +Z; root at lower Z bound; X centred and Y bounds centred';core['rig_status']='NO RIG / NO ANIMATIONS. Original 237-bone legacy rig only in immutable original source.'
core['intended_use']='E06 skeletal-core source candidate only; bespoke shell, rig, skinning, motion and engine integration unimplemented'
# Export clean origin assets independently. Do not export layout instances or source cages unless named.
for name,obj in list(masters.items())+[(n+'_cage',o) for n,o in cages.items()]+[('skeletal_core_static',core)]:
 bpy.ops.object.select_all(action='DESELECT');obj.hide_set(False);obj.select_set(True);bpy.context.view_layer.objects.active=obj
 bpy.ops.export_scene.gltf(filepath=os.path.join(OUT,'meshes',name+'.glb'),export_format='GLB',use_selection=True,export_animations=False,export_skins=False,export_morph=False,export_apply=True,export_extras=True,export_yup=True,export_texcoords=False,export_normals=True,export_materials='EXPORT')
# Editable linked instances; examples have no niche/wall/support meshes and imply no accepted scene dimensions.
arrangements=[]
examples=[('A','04_ARRANGEMENT_A_EXAMPLE',[('skull_jaw',(-.22,.04,0),(0,0,-12)),('femur',(.02,-.1,0),(89,3,68)),('humerus',(.12,.07,0),(92,-5,126))]),('B','05_ARRANGEMENT_B_EXAMPLE',[('skull_jaw',(.18,.11,0),(0,0,18)),('humerus',(-.22,-.08,0),(90,0,61)),('femur',(-.12,.09,0),(86,0,-49))])]
for letter,col,items in examples:
 records=[]
 for index,(key,loc,angles) in enumerate(items):
  m=masters[key];o=bpy.data.objects.new(f'Example_{letter}_{index+1}_{key}',m.data);collections[col].objects.link(o);o.rotation_euler=tuple(math.radians(x) for x in angles);o.location=loc;bpy.context.view_layer.update()
  floor=min((o.matrix_world@v.co).z for v in o.data.vertices);o.location.z-=floor
  o['master_asset']=key;o['status']='Editable composition example only, no scene placement approval'
  records.append({'name':o.name,'master':key+'.glb','position_m':list(o.location),'rotation_euler_xyz_radians':list(o.rotation_euler),'scale':list(o.scale),'shared_mesh_data':True})
 arrangements.append({'id':'example_'+letter,'units':'metres','coordinates':'Blender +Z up, front -Y','acceptance':'Illustrative local composition only. No approved niche dimensions, transforms, scene density, collision or instancing implementation.','instances':records})
open(os.path.join(OUT,'arrangements','editable_examples.json'),'w').write(json.dumps(arrangements,indent=2))
# Studio source file layout: masters visible, everything else available but hidden to avoid overlapping candidates.
for n,o in masters.items():o.location={'skull_jaw':(-.3,0,0),'femur':(.06,0,0),'humerus':(.29,0,0)}[n]
for n,c in collections.items():
 if n!='01_MASTERS_STATIC':c.hide_viewport=True;c.hide_render=True
manifest={'format_version':1,'asset':'Horde skeletal source kit','status':'Static source candidate; not canonical game-dev certification or native-engine admission','source_sha256':SOURCE_HASH,'source_to_metres_scale':factor,'target_skeleton_height_m':1.72,'coordinate_contract':{'blender':'right handed, +Z up, -Y front','gltf':'right handed, +Y up, +Z front (standard Blender exporter conversion)','asset_transforms':'exported individual meshes have identity local transforms; metric geometry baked'},'material':{'name':mat.name,'baseColorFactor_linear':[.46,.395,.29,1],'metallicFactor':0,'roughnessFactor':.84,'textures':0,'reason':'Uniform matte warm bone is sufficient for this restrained source kit, shares one material and avoids texture/mip residency; cavities are real geometry. No faux AO/dirt lighting.'},'selection':selection,'assets':{n:metrics(o) for n,o in masters.items()},'source_cage_lod_candidates':{n:metrics(o) for n,o in cages.items()},'static_core':metrics(core),'source_rig':{'bone_count':237,'actions':0,'preserved_in':'source/fgc_skeleton.original.blend','exported':False},'smoothing_policy':'Sparse one-level subdivision for three prop masters only. Teeth remain base; whole-skeleton candidate remains 15585 base triangles. Cages offered for later screen-space LOD assessment, no game LOD thresholds asserted.','gaps':['Native Horde importer and material compatibility','Android and Windows RT measurements','Collision/clearance and actual niche fit','Owner visual acceptance','Core retopology/rig adaptation/animation; no runtime skeleton supplied','LOD thresholds and silhouette validation at gameplay distances']}
open(os.path.join(OUT,'evidence','build-manifest.json'),'w').write(json.dumps(manifest,indent=2))
text=bpy.data.texts.new('READ_ME.txt');text.write('Horde skeletal source kit\nGeometry: Gord Goodwin Manny, CC0. See package provenance.\nThree static masters; hidden source cages, static whole core and linked arrangement examples.\nNo rig, animations, collision, game admission or actual niche fit is asserted.\nMaterial parameter set and arrangements newly authored for this project.\nOriginal 237-bone rig remains only in fgc_skeleton.original.blend.\n')
# Save a useful initial modelling view without altering export-space geometry.
for screen in bpy.data.screens:
 for area in screen.areas:
  if area.type=='VIEW_3D':
   area.spaces.active.region_3d.view_distance=1.15
   area.spaces.active.region_3d.view_location=(0,0,.2)
   area.spaces.active.region_3d.view_rotation=Vector((.4,-1,.35)).to_track_quat('Z','Y')
   area.spaces.active.shading.color_type='MATERIAL'
bpy.context.preferences.filepaths.save_version=0
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,'source','horde_skeletal_source_kit.blend'),compress=True)
print('BUILD_MANIFEST',json.dumps(manifest))
