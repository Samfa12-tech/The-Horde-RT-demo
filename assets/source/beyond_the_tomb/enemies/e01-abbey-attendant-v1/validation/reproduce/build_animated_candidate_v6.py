import bpy,bmesh,sys,pathlib,json,math,hashlib
from mathutils import Matrix,Vector,Quaternion
blend,source,out=map(pathlib.Path,sys.argv[sys.argv.index('--')+1:]);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(blend));scene=bpy.context.scene;scene.render.fps=60
rig=next(o for o in scene.objects if o.type=='ARMATURE');mesh=next(o for o in scene.objects if o.type=='MESH');rig.data.pose_position='POSE';rig.animation_data_create();rig.animation_data.action=None
bm=bmesh.new();bm.from_mesh(mesh.data);bmesh.ops.triangulate(bm,faces=list(bm.faces),quad_method='BEAUTY',ngon_method='BEAUTY');bmesh.ops.dissolve_degenerate(bm,dist=0.00001,edges=list(bm.edges));bm.normal_update();bm.to_mesh(mesh.data);bm.free();mesh.data.update();mesh.data.normals_split_custom_set([(0,0,0)]*len(mesh.data.loops))
# Imported glTF bone display tails are oversized; shortening them along their
# existing axes preserves rest matrices and improves the editable source.
bpy.context.view_layer.objects.active=rig;rig.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
length_changes=[]
for b in rig.data.edit_bones:b.use_connect=False
for b in rig.data.edit_bones:
 target=min(((c.head-b.head).length for c in b.children if (c.head-b.head).length>.01),default=7 if 'Hand' in b.name else 3)
 if abs(b.length-target)>1e-4:length_changes.append([b.name,b.length,target]);b.length=target
bpy.ops.object.mode_set(mode='OBJECT')
# Explicit positive top-four normalized influences after the local hand repair.
for v in mesh.data.vertices:
 weights=sorted([(g.group,g.weight) for g in v.groups if g.weight>1e-5],key=lambda p:p[1],reverse=True)[:4]
 total=sum(w for _,w in weights);assert total>0
 for group in mesh.vertex_groups:group.remove([v.index])
 for i,w in weights:mesh.vertex_groups[i].add([v.index],w/total,'REPLACE')
# A helper bone avoids the importer-specific Empty/bone-tail offset.
grip_data=json.loads((blend.parent/'grip-draft-metrics.json').read_text());grip_world=Matrix(grip_data['grip_world_matrix_blender'])@Matrix.Rotation(math.pi/2,4,'X');local=rig.matrix_world.inverted()@grip_world
bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='EDIT')
e=rig.data.edit_bones.new('RightGrip');e.parent=rig.data.edit_bones['RightHand'];e.use_connect=False
rotation=local.to_3x3().normalized();e.head=local.translation;e.tail=e.head+rotation@Vector((0,2,0));e.align_roll(rotation@Vector((0,0,1)))
bpy.ops.object.mode_set(mode='OBJECT');rig.data.bones['RightGrip'].hide=True
rig.pose.bones['RightGrip'].scale=(100,100,100)
base_set=set(scene.objects);rest={b.name:b.matrix_local.copy() for b in rig.data.bones};rest_inv={}
for b in rig.data.bones:rest_inv[b.name]=(rest[b.parent.name].inverted()@rest[b.name] if b.parent else rest[b.name]).inverted()
report={'source_blend':str(blend),'sources':{},'clips':{},'display_bone_lengths_repaired':length_changes,'wrist_roll_degrees':{'RightForeArm':30,'RightHand':30,'LeftForeArm':-30,'LeftHand':-30},'ground_policy':'all non-running clips: lowest body surface at +1 mm each sampled frame; Running retained as source-only with no grounding correction','dead_resting_wrist_correction':'RightHand additional -45 degrees about local Y, smooth ramp from 20% to 55% clip; independently clearance-tested','fps':60,'mesh_topology':'local right-hand subdivision and grip repair; preserved source separately'}
config=[(source/'animation-six-v02/merged_animations.glb',{'Idle':'Idle_5','Right_Hand_Sword_Slash':'Attack','Dead':'Dead','Hit_Reaction':'Hit_Reaction','Idle_Turn_Left':'Idle_Turn_Left','Idle_Turn_Right':'Idle_Turn_Right'}),(source/'rig-v02/walking_glb.glb',{'Armature|walking_man|baselayer':'Walking'}),(source/'rig-v02/running_glb.glb',{'Armature|running|baselayer':'Running'})]
actions=[]
for file,names in config:
 before=set(bpy.data.actions);bpy.ops.import_scene.gltf(filepath=str(file));newobjects=set(scene.objects)-base_set;sr=next(o for o in newobjects if o.type=='ARMATURE');newactions=set(bpy.data.actions)-before;sr.animation_data.use_nla=False;sr.data.pose_position='POSE'
 report['sources'][file.name]=hashlib.sha256(file.read_bytes()).hexdigest()
 for srcname,dstname in names.items():
  action=next(a for a in newactions if a.name.startswith(srcname+'_Armature') or a.name==srcname)
  sr.animation_data.action=action;duration=round(action.frame_range[1])/60;last=round(duration*60)
  target=bpy.data.actions.new(dstname);target.use_fake_user=True;rig.animation_data.action=target;rig.animation_data.use_nla=False
  max_error=0.;ground=[];foot_samples=[]
  for frame in range(last+1):
   scene.frame_set(frame);bpy.context.view_layer.update();convert=rig.matrix_world.inverted()@sr.matrix_world;goal={b.name:convert@b.matrix.copy() for b in sr.pose.bones}
   for bone in rig.pose.bones:
    name=bone.name;parent=bone.parent
    if name=='RightGrip':
     bone.location=(0,0,0);bone.rotation_mode='QUATERNION';bone.rotation_quaternion=(1,0,0,0);bone.scale=(100,100,100);continue
    local=goal[parent.name].inverted()@goal[name] if parent else goal[name]
    loc,quat,scale=(rest_inv[name]@local).decompose();bone.rotation_mode='QUATERNION';bone.location=loc;bone.rotation_quaternion=quat;bone.scale=scale
   bpy.context.view_layer.update()
   max_error=max(max_error,max((rig.matrix_world@b.matrix.translation-rig.matrix_world@goal[b.name].translation).length for b in rig.pose.bones if b.name in goal))
   for name,degrees in report['wrist_roll_degrees'].items():
    pb=rig.pose.bones[name];pb.rotation_quaternion=pb.rotation_quaternion@Quaternion(Vector((0,1,0)),math.radians(degrees))
   if dstname=='Dead':
    f=frame/last;ramp=min(max((f-.2)/.35,0),1);ramp=ramp*ramp*(3-2*ramp);pb=rig.pose.bones['RightHand'];pb.rotation_quaternion=pb.rotation_quaternion@Quaternion(Vector((0,1,0)),math.radians(-45*ramp))
   bpy.context.view_layer.update();de=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());minimum=min((de.matrix_world@v.co).z for v in de.data.vertices)
   shift=.001-minimum if dstname!='Running' else 0.;ground.append(shift)
   delta=rig.matrix_world.inverted().to_3x3()@Vector((0,0,shift));rig.pose.bones['Hips'].location+=rest['Hips'].inverted().to_3x3()@delta
   for pb in rig.pose.bones:
    for prop in ['location','rotation_quaternion','scale']:pb.keyframe_insert(prop,frame=frame,group=pb.name)
   bpy.context.view_layer.update();foot_samples.append({'time':frame/60,'left':list(rig.matrix_world@rig.pose.bones['LeftFoot'].matrix.translation),'right':list(rig.matrix_world@rig.pose.bones['RightFoot'].matrix.translation),'ground_shift_m':shift})
  for fc in target.fcurves:
   for key in fc.keyframe_points:key.interpolation='LINEAR'
  report['clips'][dstname]={'source_name':srcname,'duration':duration,'frames':last+1,'pre_correction_retarget_max_joint_position_error_m':max_error,'ground_correction_range_m':[min(ground),max(ground)],'source_only':dstname not in ['Idle_5','Walking','Attack','Dead']}
  (out/(dstname+'-foot-samples.json')).write_text(json.dumps(foot_samples))
  actions.append(target)
 for obj in newobjects:bpy.data.objects.remove(obj,do_unlink=True)
 for act in newactions:bpy.data.actions.remove(act)
rig.animation_data.action=None;rig.animation_data.use_nla=True
for track in list(rig.animation_data.nla_tracks):rig.animation_data.nla_tracks.remove(track)
for action in actions:
 track=rig.animation_data.nla_tracks.new();track.name=action.name;strip=track.strips.new(action.name,0,action);strip.blend_type='REPLACE';track.mute=False
scene.frame_start=0;scene.frame_end=max(round(a.frame_range[1]) for a in actions);scene.frame_set(0)
for collection in [bpy.data.images,bpy.data.materials,bpy.data.meshes,bpy.data.armatures]:
 for block in list(collection):
  if block.users==0:collection.remove(block)
for img in bpy.data.images:
 if img.source=='FILE' and img.filepath:img.pack()
mesh.data.calc_loop_triangles();report['triangles']=len(mesh.data.loop_triangles);report['vertices_blender']=len(mesh.data.vertices)
for track in rig.animation_data.nla_tracks:track.mute=track.name!='Idle_5'
scene.frame_set(60)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'E01_AbbeyAttendant_editable.blend'),compress=True)
for track in rig.animation_data.nla_tracks:track.mute=False
options=dict(export_format='GLB',export_tangents=True,export_animations=True,export_animation_mode='NLA_TRACKS',export_force_sampling=True,export_frame_range=False,export_skins=True,export_morph=False,export_cameras=False,export_lights=False,export_extras=True)
bpy.ops.export_scene.gltf(filepath=str(out/'E01_AbbeyAttendant_all_clips.glb'),**options)
for track in rig.animation_data.nla_tracks:track.mute=track.name not in ['Idle_5','Walking','Attack','Dead']
bpy.ops.export_scene.gltf(filepath=str(out/'E01_AbbeyAttendant_core.glb'),**options)
for track in rig.animation_data.nla_tracks:track.mute=False
(out/'processing-report.json').write_text(json.dumps(report,indent=2)+'\n')
