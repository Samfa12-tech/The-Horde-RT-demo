"""Offline same-rig authoring proof only. Never writes production/source input."""
import bpy, pathlib, hashlib, json, math, argparse, sys
from mathutils import Vector,Matrix
ROOT=next(p for p in pathlib.Path(__file__).resolve().parents if (p/'src/gameplay/dialogue/CompanionRouteFixture.h').is_file())
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True)
parser.add_argument('--sole-cleanup',action='store_true',help='Bake two bounded sole-pose corrections; not a runtime solver')
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=False)
SOURCE=ROOT/'assets/models/characters/source/kit-warden-v1/briarhold-warden-1k.glb'
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()=='47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e'
assert not (OUT/'kit-step-turn-proof.blend').exists()
FPS=60; LAST=46; ANGLE=math.pi/4
def smooth(t):
 t=max(0.,min(1.,t));return t*t*(3-2*t)
def rot(a):return Matrix.Rotation(a,4,'Z')
def activate(s):bpy.context.window.scene=s
def update():bpy.context.view_layer.update()
def mesh_points(o):
 dg=bpy.context.evaluated_depsgraph_get();ev=o.evaluated_get(dg);m=ev.to_mesh()
 points=[ev.matrix_world@v.co for v in m.vertices];ev.to_mesh_clear();return points
def geometry(o):
 return hashlib.sha256(repr(([tuple(v.co) for v in o.data.vertices],
  [tuple(p.vertices) for p in o.data.polygons],
  [[(g.group,g.weight) for g in v.groups] for v in o.data.vertices])).encode()).hexdigest()
def bind(a):return {b.name:[list(r) for r in b.matrix_local] for b in a.data.bones}
def world_matrix(a,name):return a.matrix_world@a.pose.bones[name].matrix
def target_matrix(base,rotation,point):
 m=rotation@base;m.translation=point;return m
def leg(a,base,side,target,foot_yaw,drop,lateral):
 names=[side+n for n in ('UpLeg','Leg','Foot')]
 h=(base[names[0]].translation+drop+lateral);k0=base[names[1]].translation+drop+lateral
 ankle=base[names[2]].translation+drop+lateral
 l1=(k0-h).length;l2=(ankle-k0).length;d=target-h;raw=d.length
 stretch=max(0.,raw-l1-l2)
 length=min(max(raw,abs(l1-l2)+1e-6),l1+l2-1e-6);axis=d.normalized()
 pole=k0-h-(ankle-h)*((k0-h).dot(ankle-h)/max((ankle-h).length_squared,1e-12))
 # A supporting knee follows the planted foot's heading, rather than twisting
 # with the turning pelvis. This is authored offline and baked into the take.
 pole=rot(foot_yaw).to_3x3()@pole
 pole-=axis*pole.dot(axis)
 if pole.length<1e-6:pole=Vector((0,-1,0))-axis*axis.dot(Vector((0,-1,0)))
 pole.normalize();along=(l1*l1-l2*l2+length*length)/(2*length)
 knee=h+axis*along+pole*math.sqrt(max(0.,l1*l1-along*along))
 q1=(k0-h).rotation_difference(knee-h);q2=(ankle-k0).rotation_difference(target-knee)
 inv=a.matrix_world.inverted()
 for name,m in [(names[0],target_matrix(base[names[0]],q1.to_matrix().to_4x4(),h)),
                (names[1],target_matrix(base[names[1]],q2.to_matrix().to_4x4(),knee)),
                (names[2],target_matrix(base[names[2]],rot(foot_yaw),target))]:
  a.pose.bones[name].matrix=inv@m;update()
 return stretch

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.render.fps=FPS
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
left=bpy.context.scene;left.name='LEFT - authored 45 degree proof'
arm=next(o for o in left.objects if o.type=='ARMATURE')
mesh=bpy.data.objects['briarhold-warden']
original_objects=list(left.objects)
original_bind=bind(arm);original_geometry=geometry(mesh)
for t in arm.animation_data.nla_tracks:t.mute=True
idle=bpy.data.actions['Warden|idle'];arm.animation_data.action=idle
arm.animation_data.action_slot=idle.slots[0]
left.frame_set(0);update()
base_basis={p.name:p.matrix_basis.copy() for p in arm.pose.bones}
base_world={p.name:world_matrix(arm,p.name).copy() for p in arm.pose.bones}
base_mesh=mesh_points(mesh)
boot={}
for side in ('Left','Right'):
 allowed={mesh.vertex_groups[n].index for n in [side+x for x in ('UpLeg','Leg','Foot','ToeBase')]}
 candidates=[v.index for v in mesh.data.vertices if v.groups and all(g.group in allowed for g in v.groups if g.weight>1e-5) and base_mesh[v.index].z<.25]
 floor=min(base_mesh[i].z for i in candidates)
 sole=[i for i in candidates if base_mesh[i].z<=floor+.006]
 assert len(sole)>=3
 boot[side]={'vertices':sole,'baseFloor':floor,'candidateCount':len(candidates)}
print('BOOT_SUBSETS',json.dumps(boot),flush=True)
arm.animation_data.action=None
root=bpy.data.objects.new('PROOF ONLY root yaw - not exported motion',None);left.collection.objects.link(root)
for o in original_objects:
 if o.parent is None:o.parent=root
 if o.name=='Icosphere':o.hide_render=True;o.hide_viewport=True
mesh.color=(.25,.31,.35,1)
# Independent right take shares unchanged mesh and bind data; only object poses/actions differ.
right=bpy.data.scenes.new('RIGHT - authored 45 degree proof');right.render.fps=FPS
mapping={}
for o in list(left.objects):
 clone=o.copy();clone.name=o.name+' RIGHT';right.collection.objects.link(clone);mapping[o]=clone
for o,clone in mapping.items():
 clone.parent=mapping.get(o.parent,o.parent)
 if clone.animation_data:clone.animation_data_clear()
 for modifier in clone.modifiers:
  if modifier.type=='ARMATURE':modifier.object=mapping.get(modifier.object,modifier.object)
records=[]
for scene,a,m,r,sign in [(left,arm,mesh,root,1),(right,mapping[arm],mapping[mesh],mapping[root],-1)]:
 activate(scene);scene.frame_start=1;scene.frame_end=LAST
 a.animation_data_create();a.animation_data.action=bpy.data.actions.new('AUTHORED '+('LEFT' if sign==1 else 'RIGHT')+' original 24 joint keys')
 r.animation_data_create();r.animation_data.action=bpy.data.actions.new('PROOF '+('LEFT' if sign==1 else 'RIGHT')+' applied yaw curve')
 rows=[];support_positions={};max_stretch=0;max_endpoint=0
 for frame in range(1,LAST+1):
  scene.frame_set(frame);u=(frame-1)/(LAST-1);yaw=sign*ANGLE*smooth((u-.04)/.92)
  r.rotation_euler=(0,0,yaw)
  for p in a.pose.bones:p.matrix_basis=base_basis[p.name].copy();p.rotation_mode='QUATERNION'
  update();R=rot(yaw);base={n:R@w for n,w in base_world.items()}
  drop=Vector((0,0,-.025*math.sin(math.pi*u)**2))
  lateral=R.to_3x3()@Vector(((-1 if u<.5 else 1)*sign*.012*math.sin(2*math.pi*u)**2,0,0))
  hips=base['Hips'].copy();hips.translation+=drop+lateral
  a.pose.bones['Hips'].matrix=a.matrix_world.inverted()@hips;update()
  foot_goals={}
  for side in ('Left','Right'):
   first=(side=='Left')==(sign==1)
   begin,end=(.08,.47) if first else (.53,.92)
   phase=max(0.,min(1.,(u-begin)/(end-begin)));travel=smooth(phase)
   initial=base_world[side+'Foot'].translation
   final=rot(sign*ANGLE)@initial
   target=initial.lerp(final,travel);target.z+=.055*math.sin(math.pi*phase)**2
   foot_goals[side]=(target.copy(),sign*ANGLE*travel,initial.copy())
   max_stretch=max(max_stretch,leg(a,base,side,target,sign*ANGLE*travel-yaw,drop,lateral))
  if args.sole_cleanup:
   # One authoring refinement, exactly two rigid sole-pose fits per foot.
   # Apply continuously through swing/stance; never jump at a contact boundary.
   for side,(goal,heading,initial) in foot_goals.items():
    target=goal.copy();adjusted_heading=heading
    desired=[rot(heading)@(base_mesh[i]-initial)+goal for i in boot[side]['vertices']]
    for iteration in range(2):
     actual=mesh_points(m);positions=[actual[i] for i in boot[side]['vertices']]
     centre=sum((Vector((p.x,p.y)) for p in positions),Vector((0.,0.)))/len(positions)
     wanted=sum((Vector((p.x,p.y)) for p in desired),Vector((0.,0.)))/len(desired)
     cross=dot=0.
     for p,q in zip(positions,desired):
      v=Vector((p.x,p.y))-centre;w=Vector((q.x,q.y))-wanted
      cross+=v.x*w.y-v.y*w.x;dot+=v.dot(w)
     angle=max(-math.radians(5),min(math.radians(5),math.atan2(cross,dot)))
     turn=Matrix.Rotation(angle,2);moved=turn@(Vector((target.x,target.y))-centre)+wanted
     delta=moved-Vector((target.x,target.y))
     if delta.length>.015:delta*=.015/delta.length
     target.x+=delta.x;target.y+=delta.y;adjusted_heading+=angle
     max_stretch=max(max_stretch,leg(a,base,side,target,adjusted_heading-yaw,drop,lateral))
  update()
  for p in a.pose.bones:
   p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
  r.keyframe_insert('rotation_euler',frame=frame)
  points=mesh_points(m);row={'frame':frame,'seconds':(frame-1)/FPS,'yawDegrees':math.degrees(yaw),'feet':{}}
  for side in ('Left','Right'):
   first=(side=='Left')==(sign==1);begin,end=(.08,.47) if first else (.53,.92)
   stance='initial' if u<=begin else 'final' if u>=end else 'swing'
   indices=boot[side]['vertices'];positions=[points[i] for i in indices]
   ideal=[base_mesh[i] if stance=='initial' else rot(sign*ANGLE)@base_mesh[i] for i in indices]
   error=max((Vector((p.x,p.y))-Vector((q.x,q.y))).length for p,q in zip(positions,ideal)) if stance!='swing' else None
   row['feet'][side]={'stance':stance,'supportSoleMaxHorizontalDriftM':error,'soleMinimumZ':min(p.z for p in positions),'soleMaximumZ':max(p.z for p in positions)}
  if frame in (1,LAST):
   endpoint=max((R.inverted()@p-q).length for p,q in zip(points,base_mesh));max_endpoint=max(max_endpoint,endpoint)
   row['idleZeroEndpointMaxVertexErrorM']=endpoint
  rows.append(row)
 # Preserve sampled keys with linear interpolation; no hidden overshoot between keys.
 for action in [a.animation_data.action,r.animation_data.action]:
  action.use_fake_user=True
  for layer in action.layers:
   for strip in layer.strips:
    for slot in action.slots:
     bag=strip.channelbag(slot)
     if bag:
      for curve in bag.fcurves:
       for point in curve.keyframe_points:point.interpolation='LINEAR'
 metrics={'direction':'left' if sign==1 else 'right','durationSeconds':(LAST-1)/FPS,'angleDegrees':sign*45,
  'maxRequiredLegStretchM':max_stretch,'maxIdleZeroEndpointVertexErrorM':max_endpoint,
  'maxSupportSoleHorizontalDriftM':max(f['supportSoleMaxHorizontalDriftM'] for row in rows for f in row['feet'].values() if f['supportSoleMaxHorizontalDriftM'] is not None),
  'minimumSoleZ':min(f['soleMinimumZ'] for row in rows for f in row['feet'].values()),
  'maximumSoleZ':max(f['soleMinimumZ'] for row in rows for f in row['feet'].values()),
  'bindUnchanged':bind(a)==original_bind,'geometryWeightsUnchanged':geometry(m)==original_geometry,'frames':rows}
 records.append(metrics);print('TURN_METRICS',json.dumps({k:v for k,v in metrics.items() if k!='frames'}),flush=True)
 scene.frame_set(1)
 # Offline authoring studio only. This is not an RT game capture or acceptance.
 scene.render.engine='BLENDER_WORKBENCH';scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
 scene.display.shading.light='STUDIO';scene.display.shading.color_type='OBJECT';scene.display.shading.show_shadows=True
 scene.display.shading.show_cavity=True;scene.display.shading.cavity_type='BOTH';scene.display.shading.show_object_outline=True
 scene.display.shading.background_type='WORLD';scene.world=bpy.data.worlds.new(scene.name+' studio');scene.world.color=(.12,.12,.12)
 scene.display.render_aa='8';scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
 bpy.ops.mesh.primitive_plane_add(size=4,location=(0,0,min(x['baseFloor'] for x in boot.values())-.001))
 floor=bpy.context.object;floor.name='PREVIEW ONLY contact ground';floor.color=(.7,.7,.7,1)
 for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]:
  bpy.ops.mesh.primitive_cube_add(size=1,location=(x*.5,y*.5,-.005));o=bpy.context.object;o.name='PREVIEW ONLY floor orientation';o.scale=(.002,1,.001);o.color=(.2,.2,.2,1)
 camera_data=bpy.data.cameras.new(scene.name+' camera');camera=bpy.data.objects.new(scene.name+' camera',camera_data);scene.collection.objects.link(camera)
 camera.location=(2.6,-3.4,2.1);camera.rotation_euler=(Vector((0,0,.87))-camera.location).to_track_quat('-Z','Y').to_euler()
 camera_data.type='ORTHO';camera_data.ortho_scale=2.15;scene.camera=camera
 scene['evidence']='OFFLINE AUTHORING PREVIEW ONLY. Not native RT, phone or admitted runtime motion.'
 scene['contact_policy']='Original geometry/bind data. Two completed foot exchanges, fixed root translation, declared preview root yaw. Original Idle phase zero endpoints.'
 scene['source_sha256']='47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e'
activate(left);left.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'kit-step-turn-proof.blend'),compress=True)
result={'schema':1,'evidenceClass':'Offline Blender authored same-rig mesh/contact preview; not native RT or runtime acceptance','soleCleanupEnabled':args.sole_cleanup,
 'sourceSha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'originalGeometryWeightsDigest':original_geometry,
 'sourceRestBindUnchanged':all(x['bindUnchanged'] for x in records),'directions':records}
(OUT/'authoring-contact.json').write_text(json.dumps(result,indent=2)+'\n')
print('AUTHORING_SAVED',flush=True)
