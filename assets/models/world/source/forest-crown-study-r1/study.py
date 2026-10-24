"""Bounded owned crown derivative. Run from the Horde worktree with Blender -b --python <this>."""
from pathlib import Path
import bpy,json,hashlib,struct,math
from mathutils import Vector
ROOT=Path.cwd();OUT=ROOT/'assets/models/world/source/forest-crown-study-r1'
BASE=ROOT/'assets/models/world/source/forest-tree-pair-v1'
for d in ('source','runtime-candidates','previews'):(OUT/d).mkdir(exist_ok=True)
bpy.context.preferences.filepaths.file_preview_type='NONE'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def meshhash(ob):
 h=hashlib.sha256()
 for v in ob.data.vertices:h.update(struct.pack('<3f',*v.co))
 for p in ob.data.polygons:h.update(struct.pack('<I',len(p.vertices)));h.update(struct.pack('<'+'I'*len(p.vertices),*p.vertices))
 for row in ob.matrix_world:h.update(struct.pack('<4f',*row))
 return h.hexdigest()
def bounds(obs):
 v=[ob.matrix_world@v.co for ob in obs for v in ob.data.vertices]
 return [[min(p[i] for p in v) for i in range(3)],[max(p[i] for p in v) for i in range(3)]]
records=[]
for species,stem,width,length in [('pine','horde-irregular-pine-v1',1.85,1.35),('alder','horde-upright-alder-v1',1.65,1.25)]:
 source=BASE/'source'/(stem+'-lod1.blend');bpy.ops.wm.open_mainfile(filepath=str(source))
 bpy.context.preferences.filepaths.file_preview_type='NONE'
 obs=[ob for ob in bpy.context.scene.objects if ob.type=='MESH' and 'QA-only' not in ob.name]
 assert len(obs)==3,[o.name for o in obs]
 foliage=next(ob for ob in obs if 'needle-spray' in ob.name or 'opaque leaves' in ob.name)
 unchanged=[ob for ob in obs if ob!=foliage];old={ob.name:meshhash(ob) for ob in unchanged};oldbounds=bounds(obs)
 m=foliage.data;assert len(m.vertices)%6==0
 count=len(m.vertices)//6;anchors=[]
 for i in range(count):
  base=i*6;anchor=m.vertices[base].co.copy();axis=(m.vertices[base+2].co-anchor).normalized();anchors.append(list(anchor))
  for j in range(6):
   v=m.vertices[base+j];d=v.co-anchor;along=axis*d.dot(axis);v.co=anchor+along*length+(d-along)*width
  assert (m.vertices[base].co-anchor).length==0
 m.update()
 assert old=={ob.name:meshhash(ob) for ob in unchanged}
 assert all(math.isfinite(x) for ob in obs for v in ob.data.vertices for x in v.co)
 for ob in obs:ob.data.calc_loop_triangles()
 stats={'species':species,'source':str(source.relative_to(ROOT)),'sourceSha256':sha(source),'widthScale':width,'lengthScale':length,'closedFoliageComponents':count,'fixedFoliageBaseAnchors':len(anchors),'unchangedWoodMossGeometry':old,'oldBounds':oldbounds,'newBounds':bounds(obs),'meshCount':3,'triangles':sum(len(ob.data.loop_triangles) for ob in obs),'vertices':sum(len(ob.data.vertices) for ob in obs),'materials':len({m.name for ob in obs for m in ob.data.materials}),'texturesChanged':False,'runtimeAdmitted':False,'rtCostMeasured':False}
 target=OUT/'source'/(species+'-fuller-crown-r1.blend')
 if not target.exists():bpy.ops.wm.save_as_mainfile(filepath=str(target))
 stats['blendSha256']=sha(target);stats['runtimeExport']='blocked; original exporter attempted denied temporary image path; no retry'
 records.append(stats)
(OUT/'study.json').write_text(json.dumps({'scope':'single owned crown sample; no production asset/placement/collision/renderer changes','records':records},indent=2)+'\n')
# Matched editable-source preview only; no GLB export/import or game RT claim.
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.file_preview_type='NONE'
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=False
scene.render.resolution_x=1440;scene.render.resolution_y=960;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Study world');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.075,.085,.10,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.7
for species,stem,x in [('pine','horde-irregular-pine-v1',-5.7),('alder','horde-upright-alder-v1',1.9)]:
 for kind,offset in [('control',0),('study',3.8)]:
  file=BASE/'source'/(stem+'-lod1.blend') if kind=='control' else OUT/'source'/(species+'-fuller-crown-r1.blend')
  with bpy.data.libraries.load(str(file),link=False) as (source,dest):
   dest.objects=[name for name in source.objects if 'QA-only' not in name and ('closed' in name or 'opaque leaves' in name)]
  for ob in dest.objects:
   if ob is not None:
    scene.collection.objects.link(ob);ob.location.x+=x+offset
# Simple matched neutral studio, separate from shipping sources.
bpy.ops.mesh.primitive_plane_add(size=200);floor=bpy.context.object;floor.name='QA-only floor'
mat=bpy.data.materials.new('QA-only ground');mat.diffuse_color=(.14,.14,.14,1);floor.data.materials.append(mat)
for loc,power,size in [((0,-10,14),2200,10),((1,5,13),1700,9)]:
 data=bpy.data.lights.new('QA-only area','AREA');data.energy=power;data.shape='DISK';data.size=size;ob=bpy.data.objects.new(data.name,data);scene.collection.objects.link(ob);ob.location=loc;ob.rotation_euler=(Vector((0,0,4))-ob.location).to_track_quat('-Z','Y').to_euler()
camera=bpy.data.objects.new('QA-only camera',bpy.data.cameras.new('QA-only camera'));scene.collection.objects.link(camera);scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=17.5
for side,y in [('front',-25),('back',25)]:
 camera.location=(0,y,9);camera.rotation_euler=(Vector((0,0,4.4))-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(OUT/'previews'/('crown-control-study-'+side+'.png'));bpy.ops.render.render(write_still=True)
print('CROWN_STUDY_DONE',json.dumps(records),flush=True)
