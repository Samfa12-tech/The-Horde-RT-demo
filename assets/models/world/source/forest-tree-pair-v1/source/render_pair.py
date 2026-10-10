"""Render actual exported GLBs at comparable meter scale. QA only."""
import bpy,sys
from pathlib import Path
from mathutils import Vector
root=Path(sys.argv[sys.argv.index('--')+1]) if '--' in sys.argv else Path(__file__).resolve().parent.parent
bpy.ops.wm.open_mainfile(filepath=str(root/'source'/'horde-upright-alder-v1.blend'))
for ob in list(bpy.data.objects):
 if ob.type=='MESH' and 'QA-only' not in ob.name: bpy.data.objects.remove(ob,do_unlink=True)
for slug,x in [('horde-upright-alder-v1',-2.5),('horde-irregular-pine-v1',2.5)]:
 bpy.ops.import_scene.gltf(filepath=str(root/'runtime'/f'{slug}.glb'))
 obs=list(bpy.context.selected_objects)
 for ob in obs:
  if ob.parent is None: ob.location.x+=x
sc=bpy.context.scene; sc.render.resolution_x=1500; sc.render.resolution_y=1200; sc.cycles.samples=48; sc.cycles.use_denoising=False
cam=sc.camera; cam.location=(9,-27,11.8); cam.data.ortho_scale=12.8; center=Vector((0,0,4.3)); cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler(); sc.render.filepath=str(root/'previews'/'horde-tree-pair-runtime-comparison.png'); bpy.ops.render.render(write_still=True)
