import bpy,sys,pathlib,json,math,hashlib
from mathutils import Vector,Matrix,Quaternion,Euler
root=pathlib.Path('/source-workspace/horde-meshy/bellkeeper/motion-trial-v01');out=root/'authored-v02';out.mkdir(exist_ok=True);(out/'poses').mkdir(exist_ok=True)
source=pathlib.Path('/source-workspace/horde-meshy/bellkeeper/rigged-shell-v01/bellkeeper_fitted_rigged_shell.blend');bpy.ops.wm.open_mainfile(filepath=str(source));scene=bpy.context.scene;scene.render.fps=60;rig=bpy.data.objects['Armature'];body=bpy.data.objects['char1'];rig.animation_data_clear();rig.data.pose_position='POSE'
for o in list(scene.objects):
 if o.type in ['CAMERA','LIGHT'] or o.name=='Icosphere':bpy.data.objects.remove(o,do_unlink=True)
for pb in rig.pose.bones:pb.rotation_mode='QUATERNION';pb.rotation_quaternion=(1,0,0,0);pb.location=(0,0,0);pb.scale=(1,1,1)
for o in scene.objects:
 if o!=rig:o.animation_data_clear()
# Optional local hand-curl draft, topology/UV and per-vertex weights unchanged.
with bpy.data.libraries.load(str(root/'grip-draft/grip-body-draft.blend'),link=False) as (src,dst):dst.objects=['char1']
g=dst.objects[0];assert len(g.data.vertices)==len(body.data.vertices)
for a,b in zip(body.data.vertices,g.data.vertices):a.co=b.co
bpy.data.objects.remove(g,do_unlink=True);body.data.normals_split_custom_set([(0,0,0)]*len(body.data.loops));body.data.update()
for poly in body.data.polygons:poly.use_smooth=True
# Bind exact source PNG bytes explicitly after verifying provider UV preservation.
for mat in body.data.materials:
 for node in mat.node_tree.nodes:
  if node.type=='TEX_IMAGE' and node.image:
   name=node.image.name.split('.')[0];p=pathlib.Path('/source-workspace/horde-meshy/bellkeeper/accepted-core-v03f')/(name+'.png')
   if p.exists():
    im=bpy.data.images.load(str(p),check_existing=False);im.colorspace_settings.name='sRGB' if 'basecolor' in name else 'Non-Color';im.pack();node.image=im
# Actual joint socket, never a bone-parented Empty.
gdef=json.loads((root/'grip-draft/grip-definition.json').read_text());gm=Matrix(gdef['grip_world_matrix_blender']);bpy.context.view_layer.objects.active=rig;rig.select_set(True);bpy.ops.object.mode_set(mode='EDIT');b=rig.data.edit_bones.new('RightGrip');b.parent=rig.data.edit_bones['RightHand'];b.use_connect=False;b.matrix=rig.matrix_world.inverted()@gm;b.length=5;b.use_deform=True;bpy.ops.object.mode_set(mode='OBJECT')
rig.pose.bones['RightGrip'].rotation_mode='QUATERNION';rig['RightGrip_status']='Measured first local hand-curl proposal; source-only until contact QA';rig['animation_scope']='Authored source trial, not gameplay event authority'
baseobjects=set(scene.objects);bpy.ops.import_scene.gltf(filepath=str(root/'bell-striker-v01/bellkeeper_short_maul.glb'));new=set(scene.objects)-baseobjects;tool=next(o for o in new if o.type=='MESH');tool.name='E05_ProposedShortMaul';tool.parent=rig;tool.matrix_world=gm;vg=tool.vertex_groups.new(name='RightGrip');vg.add(list(range(len(tool.data.vertices))),1,'REPLACE');mod=tool.modifiers.new('RightGrip rigid source binding','ARMATURE');mod.object=rig
for o in new:
 if o!=tool:bpy.data.objects.remove(o,do_unlink=True)
rest={b.name:b.matrix_local.copy() for b in rig.data.bones};rworld={n:(rig.matrix_world@m).to_quaternion() for n,m in rest.items()};rig.animation_data_create();rig.animation_data.use_nla=False
G={'RightArm':[-5,6,0],'RightForeArm':[-18,0,0],'LeftArm':[0,-4,0],'LeftForeArm':[-5,0,0]}
O={**G,'RightArm':[0,14,0],'RightForeArm':[0,0,0],'LeftArm':[0,-14,0],'LeftForeArm':[0,0,0],'open':1}
W={**G,'RightArm':[-32,18,-4],'RightForeArm':[-52,0,0],'RightHand':[8,0,0],'Spine':[0,0,-8],'LeftArm':[0,-9,0],'LeftForeArm':[-12,0,0],'LeftUpLeg':[-3,0,0],'RightUpLeg':[-3,0,0],'LeftLeg':[6,0,0],'RightLeg':[6,0,0]}
I={**G,'RightArm':[-30,8,18],'RightForeArm':[7,0,0],'RightHand':[0,0,-8],'Spine':[3,0,8],'LeftArm':[5,-9,0],'LeftForeArm':[-10,0,0]}
F={**G,'RightArm':[-18,4,14],'RightForeArm':[10,0,0],'RightHand':[4,0,-6],'Spine':[2,0,7]}
D={**O,'LeftUpLeg':[-10,0,0],'RightUpLeg':[-10,0,0],'LeftLeg':[104,0,0],'RightLeg':[104,0,0],'LeftFoot':[-92,0,0],'RightFoot':[-92,0,0],'RightArm':[4,17,0],'RightForeArm':[13,0,0],'RightHand':[10,0,0],'LeftArm':[4,-20,0],'LeftForeArm':[8,0,0],'Spine02':[3,0,0],'Spine01':[3,0,0],'Spine':[3,0,0],'Head':[10,0,0],'hip_z':-.52}
Dmid={**O,'LeftUpLeg':[-5,0,0],'RightUpLeg':[-5,0,0],'LeftLeg':[35,0,0],'RightLeg':[35,0,0],'LeftFoot':[-28,0,0],'RightFoot':[-28,0,0],'hip_z':-.09,'Head':[5,0,0]}
sequences={'Idle':[(0,G),(1.5,{**G,'Spine':[.6,0,0],'Head':[-.5,0,0]}),(3,G)],'Attack':[(0,G),(.65,W),(.85,I),(1.15,F),(2.3,G)],'ExposureOpen':[(0,G),(.65,O)],'ExposureHold':[(0,O),(.75,{**O,'Head':[1,0,0]}),(1.5,O)],'RecoveryClose':[(0,O),(.2,O),(.9,G)],'Dead':[(0,O),(.65,Dmid),(1.8,D),(2.6,{**D,'Head':[15,0,0]}),(3.3,{**D,'Head':[15,0,0]})]}
def interp(a,b,f):
 keys=set(a)|set(b);r={}
 for k in keys:
  if k in ['open','hip_z']:r[k]=a.get(k,0)+(b.get(k,0)-a.get(k,0))*f
  else:r[k]=[a.get(k,[0,0,0])[i]+(b.get(k,[0,0,0])[i]-a.get(k,[0,0,0])[i])*f for i in range(3)]
 return r
def apply(p):
 for pb in rig.pose.bones:pb.rotation_mode='QUATERNION';pb.rotation_quaternion=(1,0,0,0);pb.location=(0,0,0);pb.scale=(1,1,1)
 for n,v in p.items():
  if n in ['open','hip_z']:continue
  q=Euler([math.radians(a) for a in v],'XYZ').to_quaternion();rq=rworld[n];rig.pose.bones[n].rotation_quaternion=rq.inverted()@q@rq
 for n,sign in [('E05_Shutter.L',1),('E05_Shutter.R',-1)]:rig.pose.bones[n].rotation_quaternion=Quaternion((0,0,1),math.radians(95*sign*p.get('open',0)))
 delta=rig.matrix_world.inverted().to_3x3()@Vector((0,0,p.get('hip_z',0)));rig.pose.bones['Hips'].location=rest['Hips'].inverted().to_3x3()@delta;bpy.context.view_layer.update()
report={'source_shell_blend':str(source),'source_shell_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'hand_grip':gdef,'status':'First bounded authored trial; clips remain proposals until individual QA','fps':60,'clips':{},'notes':['No provider motion speed scaling used in authored clips.','Pose interpolation smoothstep is explicitly sampled LINEAR at 60 Hz for native reader.','Vertical root is grounded against minimum body surface each sample; feet/contact quality must still be reviewed.','Original Walking/Running remain source references in original-rig-qa, not silently accepted as H2 motion.','Separate tool is included only in editable/render assembly; joined host derivative excludes its geometry.']}
actions=[]
for name,seq in sequences.items():
 action=bpy.data.actions.new(name);action.use_fake_user=True;rig.animation_data.action=action;last=round(seq[-1][0]*60);ground=[];foot=[];toolminimum=[]
 for frame in range(last+1):
  scene.frame_set(frame);t=frame/60;i=next((i for i in range(len(seq)-1) if t<=seq[i+1][0]+1e-8),len(seq)-2);ta,a=seq[i];tb,b=seq[i+1];f=max(0,min(1,(t-ta)/(tb-ta)));f=f*f*(3-2*f);apply(interp(a,b,f))
  dep=bpy.context.evaluated_depsgraph_get();ev=body.evaluated_get(dep);low=min((ev.matrix_world@v.co).z for v in ev.data.vertices);shift=.002-low;ground.append(shift);delta=rig.matrix_world.inverted().to_3x3()@Vector((0,0,shift));rig.pose.bones['Hips'].location+=rest['Hips'].inverted().to_3x3()@delta;bpy.context.view_layer.update()
  for pb in rig.pose.bones:
   for prop in ['location','rotation_quaternion','scale']:pb.keyframe_insert(prop,frame=frame,group=pb.name)
  foot.append({'t':t,'left':list(rig.matrix_world@rig.pose.bones['LeftFoot'].matrix.translation),'right':list(rig.matrix_world@rig.pose.bones['RightFoot'].matrix.translation)})
  ev=tool.evaluated_get(bpy.context.evaluated_depsgraph_get());toolminimum.append(min((ev.matrix_world@v.co).z for v in ev.data.vertices))
 for fc in action.fcurves:
  for k in fc.keyframe_points:k.interpolation='LINEAR'
 report['clips'][name]={'duration_s':last/60,'frames':last+1,'ground_shift_range_m':[min(ground),max(ground)],'minimum_tool_z_m':min(toolminimum),'proposed_only':True};(out/(name+'-foot-samples.json')).write_text(json.dumps(foot));actions.append(action)
# Keep source modular and keyable. Only one action active for human inspection.
rig.animation_data.action=next(a for a in actions if a.name=='Idle');scene.frame_set(30)
for img in bpy.data.images:
 if img.source=='FILE' and img.filepath and not img.packed_file:img.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(out/'E05_Bellkeeper_modular_motion_trial.blend'),compress=True)
# Join only a duplicate body/shell mesh set for the host reader. Preserve modular source on disk.
for o in scene.objects:o.select_set(False)
joinmesh=[o for o in scene.objects if o.type=='MESH' and o!=tool and any(m.type=='ARMATURE' and m.object==rig for m in o.modifiers)]
for o in joinmesh:o.select_set(True)
bpy.context.view_layer.objects.active=body;bpy.ops.object.join();joined=bpy.context.object;joined.name='E05_BodyAndRigidShell_HostTrial'
tri=joined.modifiers.new('Host triangle export','TRIANGULATE');tri.quad_method='FIXED';tri.ngon_method='BEAUTY';bpy.ops.object.modifier_apply(modifier=tri.name)
# Export each authored action through one NLA track. Exactly one mesh, two materials, one skin.
rig.animation_data.action=None;rig.animation_data.use_nla=True
for tr in list(rig.animation_data.nla_tracks):rig.animation_data.nla_tracks.remove(tr)
for action in actions:
 tr=rig.animation_data.nla_tracks.new();tr.name=action.name;st=tr.strips.new(action.name,0,action);st.blend_type='REPLACE'
for o in scene.objects:o.select_set(False)
joined.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=joined
bpy.ops.export_scene.gltf(filepath=str(out/'E05_Bellkeeper_host_motion_trial.glb'),export_format='GLB',use_selection=True,export_tangents=True,export_skins=True,export_animations=True,export_animation_mode='NLA_TRACKS',export_force_sampling=True,export_frame_range=False,export_morph=False,export_cameras=False,export_lights=False,export_extras=True)
(out/'processing-report.json').write_text(json.dumps(report,indent=2)+'\n')
# Render the original modular source with exact separate prop after export.
bpy.ops.wm.open_mainfile(filepath=str(out/'E05_Bellkeeper_modular_motion_trial.blend'));scene=bpy.context.scene;rig=bpy.data.objects['Armature'];scene.render.engine='CYCLES';scene.cycles.samples=20;scene.cycles.use_denoising=False;scene.render.resolution_x=700;scene.render.resolution_y=850;scene.render.resolution_percentage=100;scene.world=bpy.data.worlds.new('MotionQA');scene.world.color=(.12,.12,.14);scene.view_settings.view_transform='AgX'
for loc,en in [((2,-3,4),650),((-3,-1,2),400),((1,3,3),500)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=en;o.data.size=3;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,0));ground=bpy.context.object;ground.name='RenderOnlyGround';mat=bpy.data.materials.new('RenderOnlyGround');mat.diffuse_color=(.11,.12,.13,1);ground.data.materials.append(mat)
bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=2.65;scene.camera=cam
poses=[('Idle',.5),('Attack',.65),('Attack',.85),('Attack',1.15),('ExposureOpen',.65),('ExposureHold',.75),('RecoveryClose',.9),('Dead',1.8),('Dead',3.3)]
for name,t in poses:
 rig.animation_data.action=bpy.data.actions[name];scene.frame_set(round(t*60));bpy.context.view_layer.update();target=Vector((0,-.03,.98))
 for view,loc in [('front',(0,-4,1.15)),('side',(4,0,1.15)),('oblique',(3,-4,1.8))]:
  cam.location=loc;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(out/'poses'/f'{name}_{round(t*100):03d}_{view}.png');bpy.ops.render.render(write_still=True)
print('MOTION_TRIAL_BUILT',out)
