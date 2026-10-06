import bpy,sys,pathlib,json
source,out=map(pathlib.Path,sys.argv[sys.argv.index('--')+1:]);out.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.scene.render.fps=60;bpy.ops.import_scene.gltf(filepath=str(source),bone_heuristic='BLENDER')
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');rig.animation_data.action=None
for t in rig.animation_data.nla_tracks:
 t.mute=False
 for strip in t.strips:
  start,end=strip.action.frame_range;strip.action_frame_start=start;strip.action_frame_end=end;strip.frame_start=0;strip.frame_end=end-start;strip.scale=1
bpy.ops.export_scene.gltf(filepath=str(out),export_format='GLB',export_tangents=True,export_animations=True,export_animation_mode='NLA_TRACKS',export_force_sampling=True,export_frame_range=False,export_skins=True,export_morph=False,export_cameras=False,export_lights=False,export_extras=True)
print('ROUNDTRIP_WRITTEN',out)
