import bpy,sys,pathlib,json,hashlib
from mathutils import Matrix, Vector
source,out=map(pathlib.Path,sys.argv[sys.argv.index('--')+1:]);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
assert len(meshes)==1
obj=meshes[0]; obj.name='E01_AbbeyAttendant_H1'
obj.data.transform(obj.matrix_world);obj.matrix_world=Matrix.Identity(4)
z=[v.co.z for v in obj.data.vertices];height=max(z)-min(z);factor=1.7/height
obj.data.transform(Matrix.Translation((0,0,-min(z)*factor))@Matrix.Scale(factor,4))
mat=bpy.data.materials.new('E01_ClothBoneIron_PBR');mat.use_nodes=True;mat.use_backface_culling=False
nt=mat.node_tree;bs=nt.nodes.get('Principled BSDF')
def tex(name,colour):
    t=nt.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(out/'maps'/name));t.image.colorspace_settings.name=colour;return t
base=tex('E01_base_color_1k.png','sRGB');normal=tex('E01_normal_1k.png','Non-Color');orm=tex('E01_orm_1k.png','Non-Color')
nt.links.new(base.outputs['Color'],bs.inputs['Base Color'])
split=nt.nodes.new('ShaderNodeSeparateColor');nt.links.new(orm.outputs['Color'],split.inputs['Color']);nt.links.new(split.outputs['Green'],bs.inputs['Roughness']);nt.links.new(split.outputs['Blue'],bs.inputs['Metallic'])
nm=nt.nodes.new('ShaderNodeNormalMap');nt.links.new(normal.outputs['Color'],nm.inputs['Color']);nt.links.new(nm.outputs['Normal'],bs.inputs['Normal'])
obj.data.materials.clear();obj.data.materials.append(mat)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
obj['asset_id']='E01';obj['source_sha256']=hashlib.sha256(source.read_bytes()).hexdigest();obj['rest_pose']='original provider pose, unmodified';obj['horde_up']='+Y';obj['horde_forward']='+Z'
for image in bpy.data.images:
    if image.source=='FILE' and image.filepath: image.pack()
glb=out/'E01_AbbeyAttendant_12k_1k_prerig.glb'
bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',export_tangents=True,export_animations=False,export_skins=False,export_morph=False,export_cameras=False,export_lights=False,export_extras=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'E01_AbbeyAttendant_prerig.blend'))
obj.data.calc_loop_triangles()
report={'source':str(source),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'output':str(glb),'output_sha256':hashlib.sha256(glb.read_bytes()).hexdigest(),'bytes':glb.stat().st_size,'triangles':len(obj.data.loop_triangles),'blender_vertices':len(obj.data.vertices),'source_height':height,'scale_factor':factor,'height_m':1.7,'up':'+Y','forward':'+Z','origin':'ground-centred','pose_changed':False,'topology_changed':False,'hand_grip':'pending post-rig surface curl; no geometry replacement','blender':bpy.app.version_string}
(out/'preparation-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
