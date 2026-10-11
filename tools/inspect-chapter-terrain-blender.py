"""Non-destructive source inspection renders; never saves the opened master."""
import argparse
from pathlib import Path
import sys
import bpy
from mathutils import Vector

p=argparse.ArgumentParser();p.add_argument('--master',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
if a.output.exists():raise RuntimeError('Inspection output already exists')
bpy.ops.wm.open_mainfile(filepath=str(a.master.resolve()))
a.output.mkdir(parents=True)
scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH'
scene.render.resolution_x=1200;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.display.shading.light='STUDIO';scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.display.shading.cavity_type='BOTH';scene.display.shading.background_type='WORLD'
scene.world.color=(.075,.085,.10)
for mat in bpy.data.materials:
    mat.diffuse_color=(.42,.31,.17,1) if 'Trail' in mat.name else (.20,.28,.17,1)
camera=bpy.data.cameras.new('InspectionOnly');obj=bpy.data.objects.new('InspectionOnly',camera)
scene.collection.objects.link(obj);scene.camera=obj;camera.type='ORTHO'
for name,position,target,scale in [
    ('near-overhead',(-12,-7,100),(-12,-7,0),100),
    ('mound-section',(-63,35,30),(-20,0,2),100),
    ('landing-surround',(-44,29,14),(-33.7,15.2,2.05),29)]:
    obj.location=position;obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    camera.ortho_scale=scale;scene.render.filepath=str((a.output/(name+'.png')).resolve())
    bpy.ops.render.render(write_still=True)
