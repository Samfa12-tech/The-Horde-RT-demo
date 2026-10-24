from pathlib import Path
import bpy,json
from mathutils import Vector
r=Path.cwd();o=r/'assets/models/world/source/chimney-smoke-study-r1';(o/'source').mkdir(exist_ok=True);(o/'previews').mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.file_preview_type='NONE'
bpy.ops.import_scene.gltf(filepath=str(r/'reports/chimney-smoke-20261024/open-flue/bellwether-open-flue-proof.glb'))
s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=24;s.cycles.use_denoising=False;s.render.resolution_x=960;s.render.resolution_y=720;s.render.resolution_percentage=100
s.world=bpy.data.worlds.new('Source inspection world');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.35
for xyz,power,size in [((97,-111,6),650,4),((93,-108,3),350,3)]:
 d=bpy.data.lights.new('QA-only area','AREA');d.energy=power;d.size=size;ob=bpy.data.objects.new(d.name,d);s.collection.objects.link(ob);ob.location=xyz;ob.rotation_euler=(Vector((95.36,-109.28,.42))-ob.location).to_track_quat('-Z','Y').to_euler()
cam=bpy.data.objects.new('QA-only camera',bpy.data.cameras.new('QA-only camera'));s.collection.objects.link(cam);s.camera=cam;cam.location=(97.2,-111.1,2.6);cam.data.lens=45;cam.rotation_euler=(Vector((95.36,-109.28,.35))-cam.location).to_track_quat('-Z','Y').to_euler()
source=o/'source/b01-open-flue-r1.blend';assert not source.exists();bpy.ops.wm.save_as_mainfile(filepath=str(source));s.render.filepath=str(o/'previews/b01-open-flue-close.png');bpy.ops.render.render(write_still=True);print('OPEN_FLUE_SOURCE_PREVIEW_COMPLETE',flush=True)
