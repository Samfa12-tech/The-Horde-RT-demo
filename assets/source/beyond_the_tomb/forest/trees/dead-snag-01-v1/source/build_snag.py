import bpy,math,json,os
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[1]
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.save_version=0
bpy.ops.import_scene.gltf(filepath=str(R/'source/DeadTree_1.gltf'))
obs=[o for o in bpy.context.scene.objects if o.type=='MESH']
bpy.ops.object.select_all(action='DESELECT')
for o in obs:o.select_set(True)
bpy.context.view_layer.objects.active=obs[0]
if len(obs)>1:bpy.ops.object.join()
o=bpy.context.object;o.name='Horde_Dead_Snag_01_LOD0'
# Weld coincident glTF seam vertices before decimation; UVs remain per-loop.
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.remove_doubles(threshold=0.0001);bpy.ops.object.mode_set(mode='OBJECT')
# Preserve source's metre-scale shape; put lowest trunk vertex on ground.
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
low=min((o.matrix_world@v.co).z for v in o.data.vertices);o.location.z-=low
bpy.ops.object.transform_apply(location=True,rotation=False,scale=False)
mat=bpy.data.materials.new('Original_Weathered_Bark');mat.use_nodes=True
nt=mat.node_tree;bs=nt.nodes.get('Principled BSDF');bs.inputs['Metallic'].default_value=0;bs.inputs['Roughness'].default_value=.82
for name,socket,color in [('snag_bark_basecolor.png','Base Color',True),('snag_bark_roughness.png','Roughness',False)]:
 im=bpy.data.images.load(str(R/'textures'/name));im.colorspace_settings.name='sRGB' if color else 'Non-Color';im.pack();node=nt.nodes.new('ShaderNodeTexImage');node.image=im;node.extension='REPEAT';nt.links.new(node.outputs['Color'],bs.inputs[socket])
im=bpy.data.images.load(str(R/'textures/snag_bark_normal.png'));im.colorspace_settings.name='Non-Color';im.pack();node=nt.nodes.new('ShaderNodeTexImage');node.image=im;node.extension='REPEAT';normal=nt.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=.6;nt.links.new(node.outputs['Color'],normal.inputs['Color']);nt.links.new(normal.outputs['Normal'],bs.inputs['Normal'])
o.data.materials.clear();o.data.materials.append(mat)
for p in o.data.polygons:p.use_smooth=True
lods=[o]
for i,r in [(1,.5),(2,.25)]:
 d=o.copy();d.data=o.data.copy();bpy.context.collection.objects.link(d);d.name=f'Horde_Dead_Snag_01_LOD{i}';bpy.context.view_layer.objects.active=d;mod=d.modifiers.new('Offline conservative decimation','DECIMATE');mod.ratio=r;bpy.ops.object.modifier_apply(modifier=mod.name);lods.append(d)
report=[]
for d in lods:
 bpy.ops.object.select_all(action='DESELECT');d.select_set(True);bpy.context.view_layer.objects.active=d;d.data.calc_loop_triangles()
 report.append({'name':d.name,'vertices':len(d.data.vertices),'triangles':len(d.data.loop_triangles),'dimensions':list(d.dimensions)})
 bpy.ops.export_scene.gltf(filepath=str(R/'models'/f'{d.name}.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_image_format='AUTO',export_animations=False,export_extras=True)
 d.hide_render=d!=o;d.hide_set(d!=o)
# Store all three overlapping source LODs; only LOD0 visible.
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.wm.save_as_mainfile(filepath=str(R/'source/Horde_Dead_Snag_01.blend'))
(R/'evidence/geometry_report.json').write_text(json.dumps(report,indent=2)+'\n')
# Separate inspection renders; lighting/ground are not exported into models/source.
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=False;scene.render.resolution_x=720;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Inspection only');scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.2,.23,.27,1);scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.45
for pos,power,size in [((4,-5,8),1300,5),((-5,1,6),1000,4)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,3))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=7.5
for i,ang in enumerate([0,90,180]):
 a=math.radians(ang);camera.location=(math.sin(a)*12,-math.cos(a)*12,4.4);camera.rotation_euler=(Vector((0,0,3.1))-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(R/'evidence'/f'view_{ang:03}.png');bpy.ops.render.render(write_still=True)
# LOD side-by-side captures from same camera, avoiding naming them runtime validation.
for i,d in enumerate(lods):
 for ob in lods:ob.hide_render=ob!=d
 camera.location=(8,-10,4.4);camera.rotation_euler=(Vector((0,0,3.1))-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(R/'evidence'/f'lod_{i}.png');bpy.ops.render.render(write_still=True)
print('FINISHED',json.dumps(report))
