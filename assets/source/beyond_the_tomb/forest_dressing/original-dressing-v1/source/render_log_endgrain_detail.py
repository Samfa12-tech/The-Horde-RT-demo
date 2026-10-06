import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(ROOT/'assets'/'bh_log_fallen_trunk.glb'))
s=bpy.context.scene;s.view_layers[0].material_override=None;s.render.engine='CYCLES';s.cycles.samples=32;s.cycles.use_denoising=False;s.render.resolution_x=700;s.render.resolution_y=700;s.render.resolution_percentage=100;s.view_settings.view_transform='AgX';s.world.use_nodes=True;bg=s.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.15,.18,.22,1);bg.inputs['Strength'].default_value=.55
p=Vector((1.60,-.06,.40))
def aim(o):o.rotation_euler=(p-o.location).to_track_quat('-Z','Y').to_euler()
for n,loc,pow,size in [('Key',(4,-3,5),700,3),('Fill',(4,2,3),350,3),('Rim',(-1,-2,3),350,3)]:
 d=bpy.data.lights.new(n,'AREA');d.energy=pow;d.size=size;o=bpy.data.objects.new(n,d);bpy.context.collection.objects.link(o);o.location=loc;aim(o)
d=bpy.data.cameras.new('Endgrain detail camera');c=bpy.data.objects.new('Endgrain detail camera',d);bpy.context.collection.objects.link(c);c.location=(3.5,-1.7,1.25);aim(c);d.type='ORTHO';d.ortho_scale=.95;s.camera=c;s.render.filepath=str(ROOT/'previews'/'bh_log_fallen_trunk-endgrain-detail.png');bpy.ops.render.render(write_still=True)
