import bpy,json,os,math,sys,argparse
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--package',required=True);p.add_argument('--only',nargs='*');a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);ROOT=a.package
os.makedirs(ROOT+'/evidence/renders',exist_ok=True)
results=[]
def start():
 bpy.ops.wm.read_factory_settings(use_empty=True);sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=96;sc.cycles.use_denoising=False;sc.render.resolution_x=800;sc.render.resolution_y=800;sc.render.resolution_percentage=100;sc.view_settings.view_transform='AgX';sc.render.image_settings.file_format='PNG';sc.render.film_transparent=False
 sc.world=bpy.data.worlds.new('InspectionWorld');sc.world.use_nodes=True;sc.world.node_tree.nodes['Background'].inputs[0].default_value=(.15,.17,.20,1);sc.world.node_tree.nodes['Background'].inputs[1].default_value=.35;return sc

def imp(name):
 before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=ROOT+'/meshes/'+name+'.glb');return [o for o in set(bpy.data.objects)-before if o.type=='MESH']

def render(sc,objects,title,delta=(.72,-1.6,.55),floor=True):
 bpy.context.view_layer.update();vs=[o.matrix_world@v.co for o in objects for v in o.data.vertices];lo=Vector([min(v[k] for v in vs) for k in range(3)]);hi=Vector([max(v[k] for v in vs) for k in range(3)]);center=(lo+hi)*.5;span=max(hi-lo);width=max(hi.x-lo.x,hi.y-lo.y);span=max(span,width)
 camd=bpy.data.cameras.new('InspectionCamera');cam=bpy.data.objects.new('InspectionCamera',camd);sc.collection.objects.link(cam);sc.camera=cam;camd.type='ORTHO';camd.ortho_scale=span*1.36;cam.location=center+Vector(delta)*span;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
 for name,d,power,size in [('Key',(-1.1,-1.6,2.1),150,1.8),('Fill',(1.8,-.3,1.1),75,1.5),('Rim',(.5,1.5,1.6),170,1.3)]:
  ld=bpy.data.lights.new(name,'AREA');ld.energy=power*span*span;ld.size=size*span;ob=bpy.data.objects.new(name,ld);sc.collection.objects.link(ob);ob.location=center+Vector(d)*span;ob.rotation_euler=(center-ob.location).to_track_quat('-Z','Y').to_euler()
 if floor:
  bpy.ops.mesh.primitive_plane_add(size=200*span,location=(0,0,lo.z-.0004));pl=bpy.context.object;pl.name='Preview_only_floor';mat=bpy.data.materials.new('Preview_only_floor');mat.use_nodes=True;mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.105,.12,.14,1);mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.92;pl.data.materials.append(mat)
 sc.render.filepath=ROOT+'/evidence/renders/'+title+'.png';bpy.ops.render.render(write_still=True)
 for o in objects:
  o.data.calc_loop_triangles()
 results.append({'render':title+'.png','render_source':'Fresh GLB import in factory-empty Blender scene','objects':len(objects),'materials':len(set(m.name for o in objects for m in o.data.materials)),'triangles':sum(len(o.data.loop_triangles) for o in objects),'blender_bounds_min_m':list(lo),'blender_bounds_max_m':list(hi),'preview_stage':'Cycles studio render, not Horde GPU import'})

for title,name,delta in [('01_skull_threequarter','skull_jaw',(.7,-1.6,.45)),('02_skull_side','skull_jaw',(1.7,.05,.15)),('03_skull_cage','skull_jaw_cage',(.7,-1.6,.45)),('04_femur','femur',(.8,-1.6,.45)),('05_humerus','humerus',(.8,-1.6,.45)),('08_core_front','skeletal_core_static',(.16,-2,.1)),('09_core_back','skeletal_core_static',(.8,2,.18)),('10_femur_cage','femur_cage',(.8,-1.6,.45)),('11_humerus_cage','humerus_cage',(.8,-1.6,.45))]:
 if a.only and title not in a.only:continue
 sc=start();objects=imp(name);render(sc,objects,title,delta)
for arrangement in json.load(open(ROOT+'/arrangements/editable_examples.json')):
 title='06_arrangement_a' if arrangement['id']=='example_A' else '07_arrangement_b'
 if a.only and title not in a.only:continue
 sc=start();objects=[];shared={}
 for row in arrangement['instances']:
  key=row['master'].removesuffix('.glb')
  if key not in shared:o=imp(key)[0];shared[key]=o
  else:
   base=shared[key];o=bpy.data.objects.new(row['name'],base.data);sc.collection.objects.link(o)
  o.rotation_mode='XYZ';o.location=row['position_m'];o.rotation_euler=row['rotation_euler_xyz_radians'];objects.append(o)
 render(sc,objects,title,(.25,-1.5,1.4))
if a.only and os.path.exists(ROOT+'/evidence/fresh-import-renders.json'):
 previous=json.load(open(ROOT+'/evidence/fresh-import-renders.json')); names={r['render'] for r in results};results=[r for r in previous if r['render'] not in names]+results
open(ROOT+'/evidence/fresh-import-renders.json','w').write(json.dumps(results,indent=2))
