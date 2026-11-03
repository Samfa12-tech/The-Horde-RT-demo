"""Optional Blender CPU authoring previews, not native gameplay evidence.
blender -b --threads 2 --python preview.py -- --output reports/<private>/previews
No glTF export, source modification, runtime launch or foreground is used.
"""
import argparse
import math
from pathlib import Path
import sys
import bpy
from mathutils import Vector

p=argparse.ArgumentParser();p.add_argument('--output',required=True)
a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);out=Path(a.output).resolve()
require_root=Path(__file__).resolve().parents[5]/'reports'
if not out.is_relative_to(require_root):raise ValueError('Previews must stay in ignored reports')
out.mkdir(parents=True,exist_ok=True)
for name in ['pine-middle','pine-far','alder-middle','alder-far','boundary-panel']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(Path(__file__).resolve().parent/'candidates'/(name+'.glb')))
    objs=[o for o in bpy.context.scene.objects if o.type=='MESH']
    points=[o.matrix_world@Vector(v) for o in objs for v in o.bound_box]
    lo=Vector([min(v[k] for v in points) for k in range(3)]);hi=Vector([max(v[k] for v in points) for k in range(3)])
    center=(lo+hi)*.5;size=max(hi-lo)
    bpy.ops.mesh.primitive_plane_add(size=30,location=(0,0,lo.z-.015))
    ground=bpy.context.object;mat=bpy.data.materials.new('Neutral studio floor');mat.diffuse_color=(.16,.17,.15,1);ground.data.materials.append(mat)
    for location,energy,scale in [((4,-5,11),1400,5),((-5,1,7),1100,5),((0,5,12),1600,4)]:
        bpy.ops.object.light_add(type='AREA',location=location);lamp=bpy.context.object;lamp.data.energy=energy;lamp.data.shape='DISK';lamp.data.size=scale;lamp.rotation_euler=(center-lamp.location).to_track_quat('-Z','Y').to_euler()
    bpy.ops.object.camera_add(location=center+Vector((size*.85,-size*1.4,size*.28)))
    camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=size*1.18
    scene=bpy.context.scene;scene.camera=camera;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=12
    scene.render.resolution_x=480;scene.render.resolution_y=640;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Neutral studio world');scene.world.color=(.18,.18,.18);scene.view_settings.view_transform='AgX'
    scene.render.image_settings.file_format='PNG';scene.render.filepath=str(out/(name+'.png'))
    bpy.ops.wm.save_as_mainfile(filepath=str(out/(name+'.blend')))
    bpy.ops.render.render(write_still=True)
