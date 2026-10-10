"""Blender source topology checks and runtime GLB reimport/render.
Run with Blender: --python verify_blender_roundtrip.py -- /bundle/root
"""
import bpy,bmesh,json,sys,math
from pathlib import Path
from mathutils import Vector
root=Path(sys.argv[sys.argv.index('--')+1]) if '--' in sys.argv else Path(__file__).resolve().parent.parent
reports=[]
lod='--lod1' in sys.argv
slugs=['horde-upright-alder-v1','horde-irregular-pine-v1']
if lod: slugs=[x+'-lod1' for x in slugs]
for slug in slugs:
 bpy.ops.wm.open_mainfile(filepath=str(root/'source'/f'{slug}.blend'))
 source=[]
 for ob in list(bpy.data.objects):
  if ob.type!='MESH' or 'QA-only' in ob.name: continue
  bm=bmesh.new(); bm.from_mesh(ob.data); boundary=sum(e.is_boundary for e in bm.edges); nonmanifold=sum(not e.is_manifold for e in bm.edges); volume=bm.calc_volume(signed=True)
  source.append({'name':ob.name,'boundary_edges':boundary,'nonmanifold_edges':nonmanifold,'signed_volume_m3':volume}); bm.free(); bpy.data.objects.remove(ob,do_unlink=True)
 bpy.ops.import_scene.gltf(filepath=str(root/'runtime'/f'{slug}.glb'))
 imported=[o for o in bpy.context.selected_objects if o.type=='MESH']; post=[]; allverts=[]; total=0
 for ob in imported:
  ob.data.calc_loop_triangles(); total+=len(ob.data.loop_triangles); allverts.extend([ob.matrix_world@v.co for v in ob.data.vertices])
  bm=bmesh.new(); bm.from_mesh(ob.data); bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6)
  post.append({'name':ob.name,'boundary_edges_after_seam_weld':sum(e.is_boundary for e in bm.edges),'nonmanifold_edges_after_seam_weld':sum(not e.is_manifold for e in bm.edges),'signed_volume_m3':bm.calc_volume(signed=True)}); bm.free()
 assert all(x['boundary_edges']==0 and x['nonmanifold_edges']==0 and x['signed_volume_m3']>0 for x in source)
 assert all(x['boundary_edges_after_seam_weld']==0 and x['nonmanifold_edges_after_seam_weld']==0 and x['signed_volume_m3']>0 for x in post)
 bounds={'min':[min(v[i] for v in allverts) for i in range(3)],'max':[max(v[i] for v in allverts) for i in range(3)]}
 sc=bpy.context.scene; sc.cycles.samples=24; sc.cycles.use_denoising=False; sc.render.resolution_x=900; sc.render.resolution_y=1080
 cam=sc.camera; cam.location=(11,-20,10.7); cam.data.ortho_scale=10.3; center=Vector((0,0,4.15 if 'alder' in slug else 4.4)); cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
 sc.render.filepath=str(root/'previews'/f'{slug}-runtime-reimport.png'); bpy.ops.render.render(write_still=True)
 cam.location=(-14,17,10.2); cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler(); sc.render.filepath=str(root/'previews'/f'{slug}-back.png'); bpy.ops.render.render(write_still=True)
 reports.append({'asset':slug,'source_topology':source,'runtime_reimport_topology':post,'runtime_triangles':total,'runtime_bounds_blender_world_xyz':bounds,'blender_roundtrip_import_pass':True,'source_closed_volume_meshes_pass':True,'runtime_closed_volume_after_uv_seam_weld_pass':True,'runtime_render':f'previews/{slug}-runtime-reimport.png','engine_gpu_import':'not performed'})
(root/'qa'/('blender-roundtrip-lod1.json' if lod else 'blender-roundtrip.json')).write_text(json.dumps(reports,indent=2)); print(json.dumps(reports,indent=2))
