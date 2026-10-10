"""Render native geometry alpha masks for reproducible LOD silhouette comparisons."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector
root=Path(sys.argv[sys.argv.index('--')+1]) if '--' in sys.argv else Path(__file__).resolve().parent.parent
bpy.ops.wm.read_factory_settings(use_empty=True); sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.samples=16; sc.cycles.use_denoising=False; sc.render.resolution_x=512; sc.render.resolution_y=600; sc.render.resolution_percentage=100; sc.render.film_transparent=True; sc.render.image_settings.file_format='PNG'; sc.render.image_settings.color_mode='RGBA'
bpy.ops.object.camera_add(location=(11,-20,10.7)); cam=bpy.context.object; cam.data.type='ORTHO'; cam.data.ortho_scale=10.3; sc.camera=cam
mat=bpy.data.materials.new('QA white silhouette'); mat.use_nodes=True; ns=mat.node_tree.nodes; ns.clear(); em=ns.new('ShaderNodeEmission'); em.inputs['Color'].default_value=(1,1,1,1); out=ns.new('ShaderNodeOutputMaterial'); mat.node_tree.links.new(em.outputs[0],out.inputs[0]); sc.view_layers[0].material_override=mat
for tree in ['horde-upright-alder-v1','horde-irregular-pine-v1']:
 for lod in ['', '-lod1']:
  slug=tree+lod; bpy.ops.import_scene.gltf(filepath=str(root/'runtime'/f'{slug}.glb')); obs=list(bpy.context.selected_objects)
  for angle,loc in [('front',(11,-20,10.7)),('back',(-14,17,10.2))]:
   cam.location=loc; center=Vector((0,0,4.15 if 'alder' in tree else 4.4)); cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler(); sc.render.filepath=str(root/'qa'/f'{slug}-{angle}-silhouette.png'); bpy.ops.render.render(write_still=True)
  for ob in obs: bpy.data.objects.remove(ob,do_unlink=True)
