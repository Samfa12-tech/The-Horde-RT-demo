import bpy, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
m=bpy.data.materials.new('BH_Original_Stone');m.use_nodes=True;n=m.node_tree.nodes;p=n.get('Principled BSDF')
p.inputs['Roughness'].default_value=.9
for suffix,socket in [('basecolor','Base Color'),('roughness','Roughness'),('normal',None)]:
    t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(ROOT/'textures'/f'horde-stone-original-{suffix}-512.png'))
    if suffix!='basecolor':t.image.colorspace_settings.name='Non-Color'
    if suffix=='normal':
        a=n.new('ShaderNodeNormalMap');a.inputs['Strength'].default_value=.5;m.node_tree.links.new(t.outputs['Color'],a.inputs['Color']);m.node_tree.links.new(a.outputs['Normal'],p.inputs['Normal'])
    else:m.node_tree.links.new(t.outputs['Color'],p.inputs[socket])
bpy.ops.mesh.primitive_uv_sphere_add(segments=64,ring_count=32,location=(0,0,1))
o=bpy.context.object;o.data.materials.append(m)
for f in o.data.polygons:f.use_smooth=True
bpy.ops.mesh.primitive_plane_add(size=200);g=bpy.context.object;gm=bpy.data.materials.new('Preview floor only');gm.diffuse_color=(.095,.105,.115,1);g.data.materials.append(gm)
def light(name,loc,power,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
light('Key',(3,-4,6),550,5);light('Fill',(-4,-1,4),240,4);light('Rim',(1,3,5),380,3)
d=bpy.data.cameras.new('Camera');c=bpy.data.objects.new('Camera',d);bpy.context.collection.objects.link(c);c.location=(3,-5,3);c.rotation_euler=(Vector((0,0,1))-c.location).to_track_quat('-Z','Y').to_euler();d.type='ORTHO';d.ortho_scale=3
s=bpy.context.scene;s.camera=c;s.render.engine='CYCLES';s.cycles.samples=32;s.cycles.use_denoising=False;s.render.resolution_x=512;s.render.resolution_y=512;s.render.resolution_percentage=100;s.world.color=(.08,.08,.08);s.view_settings.view_transform='AgX';s.render.filepath=str(ROOT/'previews'/'stone-material-test.png');bpy.ops.render.render(write_still=True)
