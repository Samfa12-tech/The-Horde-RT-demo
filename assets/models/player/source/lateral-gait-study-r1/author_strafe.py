"""One offline lateral gait proof on the unchanged owned player rig."""
import bpy, pathlib, hashlib, json, math, argparse, sys
from mathutils import Matrix,Vector
ROOT=next(p for p in pathlib.Path(__file__).resolve().parents if (p/'src/gameplay/animation/PlayerAnimationState.cpp').is_file())
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True);parser.add_argument('--knee-cleanup',action='store_true')
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=args.output.resolve()
OUT.mkdir(exist_ok=False)
SOURCE=ROOT/'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
SOURCE_SHA='46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f'
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SOURCE_SHA
FPS=60; PERIOD=40/60; SPEED=1.9; STRIDE=SPEED*PERIOD; LAST=41

def update():bpy.context.view_layer.update()
def points(o):
 ev=o.evaluated_get(bpy.context.evaluated_depsgraph_get());m=ev.to_mesh()
 p=[ev.matrix_world@v.co for v in m.vertices];ev.to_mesh_clear();return p
def bind(a):return {b.name:[list(r) for r in b.matrix_local] for b in a.data.bones}
def geom(o):return hashlib.sha256(repr(([tuple(v.co) for v in o.data.vertices],[tuple(p.vertices) for p in o.data.polygons], [[(g.group,g.weight) for g in v.groups] for v in o.data.vertices])).encode()).hexdigest()
def wm(a,n):return a.matrix_world@a.pose.bones[n].matrix
def set_world(a,n,m):a.pose.bones[n].matrix=a.matrix_world.inverted()@m;update()
def leg(a,base,side,target,drop):
 names=[side+n for n in ('UpLeg','Leg','Foot')]
 h=base[names[0]].translation+drop;k0=base[names[1]].translation+drop;ankle=base[names[2]].translation+drop
 l1=(k0-h).length;l2=(ankle-k0).length;d=target-h;raw=d.length
 length=min(max(raw,abs(l1-l2)+1e-6),l1+l2-1e-6);axis=d.normalized()
 pole=k0-h-(ankle-h)*(k0-h).dot(ankle-h)/max((ankle-h).length_squared,1e-12)
 pole-=axis*pole.dot(axis)
 if pole.length<1e-6:pole=Vector((0,-1,0))-axis*axis.dot(Vector((0,-1,0)))
 pole.normalize();along=(l1*l1-l2*l2+length*length)/(2*length)
 knee=h+axis*along+pole*math.sqrt(max(0,l1*l1-along*along))
 for n,q,position in [(names[0],(k0-h).rotation_difference(knee-h),h),(names[1],(ankle-k0).rotation_difference(target-knee),knee)]:
  m=q.to_matrix().to_4x4()@base[n];m.translation=position;set_world(a,n,m)
 m=base[names[2]].copy();m.translation=target;set_world(a,names[2],m)
 return max(0,raw-l1-l2)

def travel(p):
 # Stance local velocity exactly cancels the declared proof root speed.
 # Swing uses endpoint velocities from stance, rather than a snapping reset.
 if p<=.6:return .3-p,0,True
 u=(p-.6)/.4;h00=2*u**3-3*u*u+1;h10=u**3-2*u*u+u;h01=-2*u**3+3*u*u;h11=u**3-u*u
 return -.3*h00-.4*h10+.3*h01-.4*h11,.075*math.sin(math.pi*u)**2,False

bpy.ops.wm.read_factory_settings(use_empty=True)
assert 'NONE' in [e.identifier for e in bpy.context.preferences.filepaths.bl_rna.properties['file_preview_type'].enum_items]
bpy.context.preferences.filepaths.file_preview_type='NONE' # This process only; do not write a global thumbnail or save preferences.
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
left=bpy.context.scene;left.name='RIGHT lateral crossover proof - NOT RUNTIME'
arm=next(o for o in left.objects if o.type=='ARMATURE');mesh=next(o for o in left.objects if o.type=='MESH')
original_bind=bind(arm);original_geometry=geom(mesh)
for t in arm.animation_data.nla_tracks:t.mute=True
idle=bpy.data.actions['Idle'];arm.animation_data.action=idle;arm.animation_data.action_slot=idle.slots[0]
left.frame_set(0);update();basis={p.name:p.matrix_basis.copy() for p in arm.pose.bones};base={p.name:wm(arm,p.name).copy() for p in arm.pose.bones};base_points=points(mesh)
boot={}
for side in ('Left','Right'):
 allowed={mesh.vertex_groups[side+n].index for n in ('UpLeg','Leg','Foot','ToeBase')}
 candidates=[v.index for v in mesh.data.vertices if v.groups and all(g.group in allowed for g in v.groups if g.weight>1e-5) and base_points[v.index].z<.25]
 low=min(base_points[i].z for i in candidates)
 sole=[i for i in candidates if base_points[i].z<=low+.006];assert len(sole)>=3
 boot[side]=dict(vertices=sole,minimumZ=low)
original_objects=list(left.objects);arm.animation_data.action=None
right=bpy.data.scenes.new('LEFT lateral crossover proof - NOT RUNTIME');mapping={}
for o in original_objects:
 clone=o.copy();clone.name=o.name+' LEFT';right.collection.objects.link(clone);mapping[o]=clone
for o,clone in mapping.items():
 clone.parent=mapping.get(o.parent,o.parent)
 if clone.animation_data:clone.animation_data_clear()
 for mod in clone.modifiers:
  if mod.type=='ARMATURE':mod.object=mapping.get(mod.object,mod.object)
metrics=[]
for scene,a,m,sign in [(left,arm,mesh,1),(right,mapping[arm],mapping[mesh],-1)]:
 bpy.context.window.scene=scene;scene.render.fps=FPS;scene.frame_start=1;scene.frame_end=LAST
 a.animation_data_create();a.animation_data.action=bpy.data.actions.new('PROOF lateral '+('right' if sign==1 else 'left'))
 rows=[];stretch=0;last_support={};stance_drift=0
 for f in range(1,LAST+1):
  scene.frame_set(f);u=(f-1)/(LAST-1);drop=Vector((0,0,-.12-.012*math.sin(2*math.pi*u)**2 if args.knee_cleanup else -.018*math.sin(2*math.pi*u)**2))
  for p in a.pose.bones:p.matrix_basis=basis[p.name].copy();p.rotation_mode='QUATERNION'
  update();hips=base['Hips'].copy();hips.translation+=drop;set_world(a,'Hips',hips)
  goals={}
  for side in ('Left','Right'):
   phase=(u+(0 if side=='Right' else .5))%1
   x,lift,stance=travel(phase);goal=base[side+'Foot'].translation.copy();goal.x+=sign*STRIDE*x
   # Stagger the cross-over in depth: no feet rotating to face lateral travel.
   goal.y+=(-.085 if side=='Left' else .085);goal.z+=lift
   stretch=max(stretch,leg(a,base,side,goal,drop));goals[side]=(phase,goal,stance)
  for p in a.pose.bones:
   p.keyframe_insert('location',frame=f);p.keyframe_insert('rotation_quaternion',frame=f);p.keyframe_insert('scale',frame=f)
  pp=points(m);row=dict(frame=f,seconds=(f-1)/FPS,rootDistance=sign*SPEED*(f-1)/FPS,feet={})
  for side,(phase,goal,stance) in goals.items():
   sole=[pp[i] for i in boot[side]['vertices']];centroid=sum(sole,Vector())/len(sole)
   world=centroid+Vector((row['rootDistance'],0,0));drift=None
   if stance and side in last_support:
    drift=(Vector((world.x,world.y))-last_support[side]).length;stance_drift=max(stance_drift,drift)
   if stance:last_support[side]=Vector((world.x,world.y))
   else:last_support.pop(side,None)
   row['feet'][side]=dict(phase=phase,stance=stance,soleMinimumZ=min(p.z for p in sole),soleMaximumZ=max(p.z for p in sole),worldSoleCentroid=list(world),adjacentStanceDrift=drift)
  rows.append(row)
 action=a.animation_data.action;action.use_fake_user=True
 for layer in action.layers:
  for strip in layer.strips:
   for slot in action.slots:
    bag=strip.channelbag(slot)
    if bag:
     for curve in bag.fcurves:
      for p in curve.keyframe_points:p.interpolation='LINEAR'
 metrics.append(dict(direction='right' if sign==1 else 'left',durationSeconds=PERIOD,declaredSpeed=SPEED,stride=STRIDE,geometryWeightsUnchanged=geom(m)==original_geometry,bindUnchanged=bind(a)==original_bind,maximumLegStretch=stretch,maximumAdjacentStanceSoleDrift=stance_drift,minimumSoleZ=min(x['soleMinimumZ'] for r in rows for x in r['feet'].values()),frames=rows))
 print('GAIT_METRICS',json.dumps({k:v for k,v in metrics[-1].items() if k!='frames'}),flush=True)
 scene.frame_set(1);scene.render.engine='BLENDER_WORKBENCH';scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
 scene.display.shading.light='STUDIO';scene.display.shading.color_type='OBJECT';m.color=(.23,.3,.34,1)
 scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True;scene.display.shading.cavity_type='BOTH';scene.display.shading.show_object_outline=True
 scene.display.shading.background_type='WORLD';scene.world=bpy.data.worlds.new(scene.name+' studio');scene.world.color=(.12,.12,.12)
 scene.display.render_aa='8';scene.render.image_settings.file_format='PNG'
 bpy.ops.mesh.primitive_plane_add(size=4,location=(0,0,min(x['minimumZ'] for x in boot.values())-.001));floor=bpy.context.object;floor.color=(.7,.7,.7,1);floor.name='PREVIEW ONLY floor'
 camera_data=bpy.data.cameras.new(scene.name+' camera');camera=bpy.data.objects.new(scene.name+' camera',camera_data);scene.collection.objects.link(camera)
 camera.location=(2.7,-3.8,1.5);camera.rotation_euler=(Vector((0,0,.8))-camera.location).to_track_quat('-Z','Y').to_euler();camera_data.type='ORTHO';camera_data.ortho_scale=2.15;scene.camera=camera
 scene['evidence']='OFFLINE AUTHORING PROOF. Not native RT or admitted locomotion. Root movement only used by contact metric; no runtime motion changed.'
 scene['source_sha256']=SOURCE_SHA
bpy.context.window.scene=left;left.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'player-lateral-proof-r1.blend'),compress=True)
(OUT/'contact.json').write_text(json.dumps(dict(schema=1,source=SOURCE_SHA,originalGeometryWeightsDigest=original_geometry,evidenceClass='offline candidate only',directions=metrics),indent=2)+'\n')
for scene in (left,right):
 bpy.context.window.scene=scene
 label='right' if scene==left else 'left'
 for frame in (1,11,21,31,41):
  scene.frame_set(frame);scene.render.filepath=str(OUT/(label+'-'+str(frame)+'.png'));bpy.ops.render.render(write_still=True)
print('AUTHORING_COMPLETE',flush=True)
