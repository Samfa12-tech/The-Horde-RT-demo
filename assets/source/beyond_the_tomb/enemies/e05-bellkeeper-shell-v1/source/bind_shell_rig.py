"""Bind original rigid shell objects to an inspected provider rig, preserving editable parts.
The two hinge helpers have local +Z aligned to Blender world +Z in the bind pose.
A joined single-mesh/single-skin runtime-trial derivative is deliberately a separate artifact.
"""
import bpy,json,sys,argparse,hashlib,math
from pathlib import Path
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--shell-blend',required=True);p.add_argument('--rig-glb',required=True);p.add_argument('--bindings-json',required=True);p.add_argument('--output-dir',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);out=Path(a.output_dir);out.mkdir(parents=True,exist_ok=True);binding=json.load(open(a.bindings_json))
bpy.ops.wm.open_mainfile(filepath=a.shell_blend);scene=bpy.context.scene;scene.frame_set(1)
for o in list(bpy.data.collections['CORE_FIT_REFERENCE'].objects):bpy.data.objects.remove(o,do_unlink=True)
prior=set(bpy.context.scene.objects);bpy.ops.import_scene.gltf(filepath=a.rig_glb);new=[o for o in bpy.context.scene.objects if o not in prior]
rigs=[o for o in new if o.type=='ARMATURE'];assert len(rigs)==1;rig=rigs[0]
rig.animation_data_clear()
for pose_bone in rig.pose.bones:pose_bone.matrix_basis.identity()
bpy.context.view_layer.update()
body_meshes=[o for o in new if o.type=='MESH' and any(m.type=='ARMATURE' and m.object==rig for m in o.modifiers)]
assert body_meshes,'No imported skinned body mesh'
for key in ['chest','head','upper_arm_left','upper_arm_right']:assert binding[key] in rig.data.bones,(key,binding[key])
# Require already normalized final source; no silent rescale/repose here.
pts=[o.matrix_world@v.co for o in body_meshes for v in o.data.vertices];bounds_min=[min(p[i] for p in pts) for i in range(3)];bounds_max=[max(p[i] for p in pts) for i in range(3)]
assert abs(bounds_min[2])<.03 and abs(bounds_max[2]-2.0)<.04,(bounds_min,bounds_max)
SHELL=bpy.data.collections['ORIGINAL_MODULAR_SHELL'];root=bpy.data.objects['E05_ModularShell_ROOT'];hinges={s:bpy.data.objects['ShutterHinge.'+s] for s in ['L','R']}
for o in SHELL.objects:
    if o.animation_data:o.animation_data_clear()
bpy.context.view_layer.update();inverse=rig.matrix_world.inverted();hinge_world={s:h.matrix_world.translation.copy() for s,h in hinges.items()}
socket_sources={name:bpy.data.objects[name].matrix_world.translation.copy() for name in ['ChestMount','HeadMount','Shoulder.L','Shoulder.R','ReflectedPathReceiver_PROPOSAL','ExposureCentre_PROPOSAL']}
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='EDIT')
parents={'ChestMount':binding['chest'],'HeadMount':binding['head'],'Shoulder.L':binding['upper_arm_left'],'Shoulder.R':binding['upper_arm_right'],'ReflectedPathReceiver_PROPOSAL':'ChestMount','ExposureCentre_PROPOSAL':'ChestMount'}
for name,world_position in socket_sources.items():
    bn=rig.data.edit_bones.new(name);bn.head=inverse@world_position;bn.tail=inverse@(world_position+Vector((0,.045,0)));bn.align_roll(inverse.to_3x3()@Vector((0,0,1)));bn.parent=rig.data.edit_bones[parents[name]];bn.use_connect=False
for side in ['L','R']:
    bn=rig.data.edit_bones.new('E05_Shutter.'+side);bn.head=inverse@hinge_world[side];bn.tail=inverse@(hinge_world[side]+Vector((0,.065,0)));bn.align_roll(inverse.to_3x3()@Vector((0,0,1)));bn.parent=rig.data.edit_bones['ChestMount'];bn.use_connect=False
bpy.ops.object.mode_set(mode='OBJECT');bpy.context.view_layer.update()
def keep_parent(o,parent):
    bpy.context.view_layer.update();m=o.matrix_world.copy();o.parent=parent;o.parent_type='OBJECT';o.matrix_world=m;bpy.context.view_layer.update()
keep_parent(root,rig)
assignments={}
for o in list(SHELL.objects):
    if o.type!='MESH':continue
    if o.name.startswith('BreastShutter.'):bn='E05_Shutter.'+o.name.rsplit('.',1)[1]
    elif o.name.startswith('Helmet_'):bn='HeadMount'
    elif o.name.startswith('ShoulderLame.L.'):bn='Shoulder.L'
    elif o.name.startswith('ShoulderLame.R.'):bn='Shoulder.R'
    else:bn='ChestMount'
    keep_parent(o,root);o.vertex_groups.clear();vg=o.vertex_groups.new(name=bn);vg.add(list(range(len(o.data.vertices))),1.0,'REPLACE');mod=o.modifiers.new('E05_RigidBoneAttachment','ARMATURE');mod.object=rig;assignments[o.name]=bn
# Named actual bones replace Empty sockets in the rigged derivative; no bone-tail parenting offsets.
for ob in list(SHELL.objects):
    if ob.type=='EMPTY' and ob!=root:bpy.data.objects.remove(ob,do_unlink=True)
# One paired inspection animation, deliberately without gameplay durations.
for frame,angle in [(1,0),(60,95)]:
    for side,sign in [('L',1),('R',-1)]:
        pb=rig.pose.bones['E05_Shutter.'+side];pb.rotation_mode='XYZ';pb.rotation_euler=(0,0,math.radians(sign*angle));pb.keyframe_insert(data_path='rotation_euler',frame=frame)
if rig.animation_data and rig.animation_data.action:rig.animation_data.action.name='E05_ShellInspection_ClosedOpen_NOT_COMBAT_TIMING'
scene.frame_set(1)
report={'rig_source':a.rig_glb,'rig_source_sha256':hashlib.sha256(Path(a.rig_glb).read_bytes()).hexdigest(),'shell_source':a.shell_blend,'bindings':binding,'socket_helper_bones':parents,'empty_bone_parenting':'None; source Empty sockets replaced with actual helper bones','shell_mesh_bone_assignments':assignments,'new_helper_bones':{'E05_Shutter.L':{'physical_axis':'vertical world Z at bind','pose_rotation_axis':'local Z','open_degrees':95},'E05_Shutter.R':{'physical_axis':'vertical world Z at bind','pose_rotation_axis':'local Z','open_degrees':-95}},'body_meshes':[o.name for o in body_meshes],'provider_import_helpers_excluded':[o.name for o in new if o.type=='MESH' and o not in body_meshes],'body_bounds_min':bounds_min,'body_bounds_max':bounds_max,'status':'Editable source; independently selected rigid objects weighted 1.0; not a joined native engine derivative','native_boundary':'Unchanged native reader tests only meshes[0]/skins[0]; motion worker must join a separate derivative and verify it. No PBR runtime path claim.'}
(out/'rig-binding-receipt.json').write_text(json.dumps(report,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(out/'bellkeeper_fitted_rigged_shell.blend'),compress=True)
print('RIGGED_SHELL_RESULT',json.dumps(report))
