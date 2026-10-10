import bpy,sys,argparse
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--package',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);path=a.package+'/source/horde_skeletal_source_kit.blend'
bpy.ops.wm.open_mainfile(filepath=path,use_scripts=False,load_ui=False)
# Save a useful initial modelling view without altering export-space geometry.
for screen in bpy.data.screens:
 for area in screen.areas:
  if area.type=='VIEW_3D':
   area.spaces.active.region_3d.view_distance=1.15
   area.spaces.active.region_3d.view_location=(0,0,.2)
   area.spaces.active.region_3d.view_rotation=Vector((.4,-1,.35)).to_track_quat('Z','Y')
   area.spaces.active.shading.color_type='MATERIAL'
bpy.context.preferences.filepaths.save_version=0
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=path,compress=True)
