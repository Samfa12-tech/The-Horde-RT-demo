"""Readable unlit topology evidence from fresh-imported actual GLB meshes."""
import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
for glb in sorted((ROOT/'assets').glob('*.glb')):
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.gltf(filepath=str(glb))
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];cs=[x.matrix_world@v.co for x in meshes for v in x.data.vertices];mn=Vector([min(v[j] for v in cs) for j in range(3)]);mx=Vector([max(v[j] for v in cs) for j in range(3)]);dim=mx-mn;center=(mn+mx)/2;scale=max(dim);target=Vector((center.x,center.y,mn.z+dim.z*.46))
 m=bpy.data.materials.new('Unlit actual triangle wire');m.use_nodes=True;n=m.node_tree.nodes;n.clear();wire=n.new('ShaderNodeWireframe');wire.use_pixel_size=True;wire.inputs['Size'].default_value=1.3;mix=n.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.45,.5,.54,1);mix.inputs[2].default_value=(.002,.003,.004,1);em=n.new('ShaderNodeEmission');out=n.new('ShaderNodeOutputMaterial');links=m.node_tree.links;links.new(wire.outputs['Fac'],mix.inputs[0]);links.new(mix.outputs['Color'],em.inputs['Color']);links.new(em.outputs[0],out.inputs['Surface'])
 s=bpy.context.scene;s.view_layers[0].material_override=m;s.render.engine='CYCLES';s.cycles.samples=8;s.cycles.use_denoising=False;s.render.resolution_x=700;s.render.resolution_y=700;s.render.resolution_percentage=100;s.world.use_nodes=True;bg=s.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.19,.22,.25,1);bg.inputs['Strength'].default_value=1;s.view_settings.view_transform='Standard'
 d=bpy.data.cameras.new('Actual topology camera');cam=bpy.data.objects.new('Actual topology camera',d);bpy.context.collection.objects.link(cam);cam.location=target+Vector((1.6,-2.8,1.8))*scale;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();d.type='ORTHO';d.ortho_scale=scale*1.37;s.camera=cam;s.render.filepath=str(ROOT/'previews'/f'{glb.stem}-wireframe.png');bpy.ops.render.render(write_still=True)
