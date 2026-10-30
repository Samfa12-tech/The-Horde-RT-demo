import bpy,bmesh,os,json,math
from mathutils import Vector
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
manifest=json.load(open(ROOT+'/asset-manifest.json'));reports=[]
for asset in manifest['assets']:
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.gltf(filepath=ROOT+'/'+asset['model'])
 obs=[o for o in bpy.context.scene.objects if o.type=='MESH']
 report={'id':asset['id'],'import':'Blender GLB reimport succeeded','triangles':0,'vertices':0,'materials':[],'objects':[]}
 coords=[]
 for o in obs:
  me=o.data;me.calc_loop_triangles();report['triangles']+=len(me.loop_triangles);report['vertices']+=len(me.vertices)
  coords.extend([o.matrix_world@v.co for v in me.vertices]);report['materials'].extend([m.name for m in me.materials])
  bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6)
  nonmanifold=sum(not e.is_manifold for e in bm.edges);deg=sum(f.calc_area()<1e-12 for f in bm.faces);bm.free()
  report['objects'].append({'name':o.name,'uv_layers':len(me.uv_layers),'nonmanifold_edges_after_seam_weld':nonmanifold,'degenerate_faces':deg})
 mins=Vector([min(c[i] for c in coords) for i in range(3)]);maxs=Vector([max(c[i] for c in coords) for i in range(3)]);center=(mins+maxs)/2;size=maxs-mins
 report['bounds_blender_m']={'min':list(mins),'max':list(maxs),'size':list(size)};report['textures']=0;report['alpha_modes']='OPAQUE';report['materials']=sorted(set(report['materials']));report['file_bytes']=os.path.getsize(ROOT+'/'+asset['model'])
 reports.append(report)
 # Studio elements are preview only; never exported in runtime GLB.
 scale=max(size);bpy.ops.mesh.primitive_plane_add(size=200*scale,location=(0,0,mins.z-.003*scale));floor=bpy.context.object
 m=bpy.data.materials.new('preview_only_ground');m.diffuse_color=(.065,.075,.08,1);floor.data.materials.append(m)
 bpy.ops.object.camera_add(location=center+Vector((1.25,-1.65,1.32))*scale);cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=scale*1.62;bpy.context.scene.camera=cam
 for nm,rel,power,col,sz in [('Key',(-2,-3,4),450,(1,.89,.76),3),('Fill',(3,-1,2),240,(.76,.85,1),3),('Rim',(0,3,3),550,(1,.96,.87),2)]:
  bpy.ops.object.light_add(type='AREA',location=center+Vector(rel)*scale);l=bpy.context.object;l.name=nm;l.data.energy=power*scale*scale;l.data.color=col;l.data.shape='DISK';l.data.size=sz*scale;l.rotation_euler=(center-l.location).to_track_quat('-Z','Y').to_euler()
 sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=96;sc.cycles.use_denoising=False;sc.render.resolution_x=640;sc.render.resolution_y=540;sc.render.resolution_percentage=100
 sc.world.color=(.18,.18,.18);sc.view_settings.view_transform='AgX';sc.render.image_settings.file_format='PNG';sc.render.filepath=ROOT+'/previews/'+asset['id']+'.png'
 bpy.ops.render.render(write_still=True)
open(ROOT+'/validation/blender-reimport-report.json','w').write(json.dumps(reports,indent=2))
print('QA_REPORT',json.dumps(reports))
