import bpy,pathlib,json,math
from mathutils import Vector,Matrix
root=pathlib.Path('/source-workspace/horde-meshy/bellkeeper/motion-trial-v01');out=root/'sealed-trial';out.mkdir(exist_ok=True);bpy.ops.wm.open_mainfile(filepath=str(root/'authored-v02/E05_Bellkeeper_modular_motion_trial.blend'));scene=bpy.context.scene;rig=bpy.data.objects['Armature'];tool=bpy.data.objects['E05_ProposedShortMaul'];body=bpy.data.objects['char1'];sample_times={'Idle':[0,1.5,3],'Attack':[0,.65,.85,1.15,2.3],'ExposureOpen':[0,.325,.65],'ExposureHold':[0,.75,1.5],'RecoveryClose':[0,.2,.55,.9],'Dead':[0,.65,1.8,2.6,3.3]}
def evaluated(o):
 e=o.evaluated_get(bpy.context.evaluated_depsgraph_get());return [e.matrix_world@v.co for v in e.data.vertices]
old={}
for name,ts in sample_times.items():
 rig.animation_data.action=bpy.data.actions[name]
 for t in ts:scene.frame_set(round(t*60));bpy.context.view_layer.update();old[(name,t)]=evaluated(tool)
# Explicit head/tail/roll reconstruction preserves the measured tool basis.
gm=Matrix(json.loads((root/'grip-draft/grip-definition.json').read_text())['grip_world_matrix_blender']);local=rig.matrix_world.inverted()@gm;bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='EDIT');bone=rig.data.edit_bones['RightGrip'];rotation=local.to_3x3().normalized();bone.head=local.translation;bone.tail=bone.head+rotation@Vector((0,5,0));bone.align_roll(rotation@Vector((0,0,1)));bpy.ops.object.mode_set(mode='OBJECT')
# Unit-world socket under centimetre armature. Body carries zero weights to it.
for action in bpy.data.actions:
 for fc in action.fcurves:
  if fc.data_path=='pose.bones["RightGrip"].scale':
   for key in fc.keyframe_points:key.co.y=100;key.handle_left.y=100;key.handle_right.y=100
rig.data.bones['RightGrip'].use_deform=False
for mod in list(tool.modifiers):tool.modifiers.remove(mod)
for con in list(tool.constraints):tool.constraints.remove(con)
tool.parent=None;tool.vertex_groups.clear();con=tool.constraints.new('COPY_TRANSFORMS');con.name='Exact unit RightGrip source preview';con.target=rig;con.subtarget='RightGrip';con.target_space='WORLD';con.owner_space='WORLD';con.mix_mode='REPLACE'
report={'method':'RightGrip helper has constant100 local scale under 0.01 armature, making unit-world socket. Preview prop uses direct world Copy Transforms at bone head, no parent-bone Empty or tail offset. Body/shell contain zero positive RightGrip weights.','samples':[],'max_tool_vertex_difference_m':0,'max_socket_axis_scale_error':0}
for name,ts in sample_times.items():
 rig.animation_data.action=bpy.data.actions[name]
 for t in ts:
  scene.frame_set(round(t*60));bpy.context.view_layer.update();now=evaluated(tool);err=max((a-b).length for a,b in zip(old[(name,t)],now));m=rig.matrix_world@rig.pose.bones['RightGrip'].matrix;sc=max(abs(m.to_3x3().col[i].length-1)for i in range(3));report['samples'].append({'clip':name,'t':t,'tool_vertex_max_delta_m':err,'RightGrip_world_matrix_blender':list(map(list,m))});report['max_tool_vertex_difference_m']=max(report['max_tool_vertex_difference_m'],err);report['max_socket_axis_scale_error']=max(report['max_socket_axis_scale_error'],sc)
assert report['max_tool_vertex_difference_m']<.00002,report
assert report['max_socket_axis_scale_error']<.00001,report
rig['RightGrip_status']='Unit-world helper; hand/weapon contact checked in source poses; native transform and fresh-import proof separate'
rig.animation_data.action=bpy.data.actions['Idle'];scene.frame_set(30);bpy.ops.wm.save_as_mainfile(filepath=str(out/'E05_Bellkeeper_modular_motion_trial.blend'),compress=True)
# All six source motions, including explicit Dead draft. No Walking admission.
for o in scene.objects:o.select_set(False)
mesh=[o for o in scene.objects if o.type=='MESH' and o!=tool and any(m.type=='ARMATURE' and m.object==rig for m in o.modifiers)]
for o in mesh:
 o.select_set(True)
 if o.data.uv_layers.active:o.data.uv_layers.active.name='UVMap'
bpy.context.view_layer.objects.active=body;bpy.ops.object.join();joined=bpy.context.object;joined.name='E05_BodyAndRigidShell_HostTrial';tri=joined.modifiers.new('Host triangles','TRIANGULATE');tri.quad_method='FIXED';bpy.ops.object.modifier_apply(modifier=tri.name)
rig.animation_data.action=None;rig.animation_data.use_nla=True
for tr in list(rig.animation_data.nla_tracks):rig.animation_data.nla_tracks.remove(tr)
for name in sample_times:
 action=bpy.data.actions[name];tr=rig.animation_data.nla_tracks.new();tr.name=name;st=tr.strips.new(name,0,action);st.blend_type='REPLACE'
for o in scene.objects:o.select_set(False)
joined.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=joined
bpy.ops.export_scene.gltf(filepath=str(out/'E05_Bellkeeper_host_motion_trial_raw.glb'),export_format='GLB',use_selection=True,export_tangents=True,export_skins=True,export_animations=True,export_animation_mode='NLA_TRACKS',export_force_sampling=True,export_frame_range=False,export_morph=False,export_cameras=False,export_lights=False,export_extras=True)
(out/'socket-scale-equivalence.json').write_text(json.dumps(report,indent=2)+'\n');print('SEALED',report['max_tool_vertex_difference_m'],report['max_socket_axis_scale_error'])
