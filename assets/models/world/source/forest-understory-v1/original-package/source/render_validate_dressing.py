"""Fresh-import checks and actual mesh studio/close-up/wire/silhouette proofs."""
import bpy, json, math, os
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
selected=set(filter(None,os.environ.get('DRESSING_ONLY','').split(',')))
rows=json.loads((ROOT/'validation'/'fresh-import-checks.json').read_text()) if selected else []
rows=[r for r in rows if Path(r['asset']).stem not in selected]
def clean():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
def area(name,loc,power,size,target):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);o.location=loc;aim(o,target)
def flat_mat(name,c,emission=False):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=1
    if emission:
        p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=1
    return m
for glb in sorted((ROOT/'assets').glob('*.glb'),key=lambda p:(0 if 'log_fallen' in p.stem else 1 if 'rock_low' in p.stem else 2,p.name)):
    if selected and glb.stem not in selected:continue
    clean();bpy.ops.import_scene.gltf(filepath=str(glb));meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];o=meshes[0]
    cs=[x.matrix_world@v.co for x in meshes for v in x.data.vertices];mn=Vector([min(v[j] for v in cs) for j in range(3)]);mx=Vector([max(v[j] for v in cs) for j in range(3)]);dim=mx-mn;center=(mn+mx)/2
    finite=all(math.isfinite(c) for x in meshes for v in x.data.vertices for c in v.co) and all(math.isfinite(c) for x in meshes for v in x.data.vertices for c in v.normal) and all(math.isfinite(c) for x in meshes for uv in x.data.uv_layers for d in uv.data for c in d.uv)
    badnorm=sum(1 for x in meshes for v in x.data.vertices if v.normal.length<.9 or v.normal.length>1.1)
    tris=sum(sum(len(p.vertices)-2 for p in x.data.polygons) for x in meshes)
    degenerate=sum(1 for x in meshes for p in x.data.polygons if p.area<1e-12)
    rows.append({'asset':glb.name,'fresh_glb_import':True,'mesh_objects':len(meshes),'triangles':tris,'finite_positions_normals_uvs':finite,'non_unit_vertex_normals':badnorm,'degenerate_triangles':degenerate,'blender_xyz_dimensions_m':[round(c,5) for c in dim],'ground_min_z':round(mn.z,8)})
    s=bpy.context.scene;s.view_layers[0].material_override=None;s.render.engine='CYCLES';s.cycles.samples=32;s.cycles.use_denoising=False;s.render.resolution_x=700;s.render.resolution_y=700;s.render.resolution_percentage=100
    s.world.use_nodes=True;bg=s.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.16,.185,.215,1);bg.inputs['Strength'].default_value=.45
    s.view_settings.view_transform='AgX';s.render.image_settings.file_format='PNG';s.render.film_transparent=False
    floor=flat_mat('Preview floor only',(.065,.075,.085));bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006));ground=bpy.context.object;ground.data.materials.append(floor)
    scale=max(dim.x,dim.y,dim.z);target=Vector((center.x,center.y,mn.z+dim.z*.46))
    area('Key',(-3*scale,-4*scale,6*scale),550*scale*scale,3.5*scale,target);area('Fill',(4*scale,-scale,3*scale),220*scale*scale,4*scale,target);area('Rim',(scale,3*scale,5*scale),350*scale*scale,3*scale,target)
    d=bpy.data.cameras.new('Evidence camera');cam=bpy.data.objects.new('Evidence camera',d);bpy.context.collection.objects.link(cam);cam.location=target+Vector((1.6,-2.8,1.8))*scale;aim(cam,target);d.type='ORTHO';d.ortho_scale=scale*1.37;s.camera=cam
    s.render.filepath=str(ROOT/'previews'/f'{glb.stem}-closeup.png');bpy.ops.render.render(write_still=True)
    # Actual topology overlay; camera is unchanged so proof corresponds to closeup.
    w=flat_mat('Evidence wire overlay',(.5,.56,.60));n=w.node_tree.nodes;links=w.node_tree.links;p=n.get('Principled BSDF');wire=n.new('ShaderNodeWireframe');wire.use_pixel_size=True;wire.inputs['Size'].default_value=.70;mix=n.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.5,.56,.60,1);mix.inputs[2].default_value=(.012,.016,.02,1);links.new(wire.outputs['Fac'],mix.inputs[0]);links.new(mix.outputs['Color'],p.inputs['Base Color'])
    s.view_layers[0].material_override=w;s.cycles.samples=16;s.render.filepath=str(ROOT/'previews'/f'{glb.stem}-wireframe.png');bpy.ops.render.render(write_still=True)
    # Silhouette is real mesh, opaque black against light gray with no floor.
    black=flat_mat('Evidence black silhouette',(0,0,0),True);s.view_layers[0].material_override=black;ground.hide_render=True
    bg.inputs['Color'].default_value=(.8,.8,.8,1);bg.inputs['Strength'].default_value=1;s.view_settings.view_transform='Standard';s.cycles.samples=8;s.render.filepath=str(ROOT/'previews'/f'{glb.stem}-silhouette.png');bpy.ops.render.render(write_still=True)
    (ROOT/'validation'/'fresh-import-checks.json').write_text(json.dumps(rows,indent=2))
print('FRESH_IMPORT_VALIDATION_COMPLETE',json.dumps(rows))
