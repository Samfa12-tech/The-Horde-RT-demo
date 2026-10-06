import bpy,bmesh,sys,pathlib,math,json
from mathutils import Vector,Matrix
source,out=map(pathlib.Path,sys.argv[sys.argv.index('--')+1:]);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(source))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH');rig.animation_data_clear();rig.data.pose_position='REST'
bone=rig.data.bones['RightHand'];group=mesh.vertex_groups['RightHand'];to_hand=bone.matrix_local.inverted()@rig.matrix_world.inverted()@mesh.matrix_world;from_hand=to_hand.inverted();changes=[]
# Add only one local edge subdivision in the right hand to support the bend;
# preserve the provider source and interpolate its weights/UVs.
bm=bmesh.new();bm.from_mesh(mesh.data);deform=bm.verts.layers.deform.active
selected=[e for e in bm.edges if all(v[deform].get(group.index,0)>.60 for v in e.verts) and any((to_hand@v.co).y>7 for v in e.verts)]
bmesh.ops.subdivide_edges(bm,edges=selected,cuts=1,use_grid_fill=True)
bm.to_mesh(mesh.data);bm.free();mesh.data.update()
# Reconstruct the local palm plane from the original curved hand. Using the
# raw Hand bone X as thickness folded the already raised fingertips inside out.
normal_local=Vector((1,-.48404461,-.32947066)).normalized()
finger_local=Vector((.48404461,1,0)).normalized()
width_local=normal_local.cross(finger_local).normalized()
basis=Matrix((normal_local,finger_local,width_local)).transposed()
base=Vector((.48404461*8.5-6.21240884,8.5,0))
radius=2.5
thumb_tip_ids=[]
for v in mesh.data.vertices:
 w=next((a.weight for a in v.groups if a.group==group.index),0.)
 if w<.5:continue
 p=to_hand@v.co;q=basis.transposed()@(p-base);t=q.copy()
 if q.y>0:
  theta=min(q.y*.29,3.5)
  t.x=radius-(radius-q.x)*math.cos(theta);t.y=(radius-q.x)*math.sin(theta)
  fade=min(q.y/1.4,1);fade=fade*fade*(3-2*fade);t=q.lerp(t,fade)
 # Raised thumb is isolated by distance out of the fitted palm plane.
 # Curl its original surface along a separate continuous arc, preserving volume.
 thumb_weight=min(max((q.x-1.5)/2,0),1);thumb_weight=thumb_weight*thumb_weight*(3-2*thumb_weight)
 gate_y=min(max((q.y+1)/2,0),1);gate_z=min(max((-q.z-2.5)/2,0),1);thumb_weight*=gate_y*gate_y*(3-2*gate_y)*gate_z*gate_z*(3-2*gate_z)
 if thumb_weight>0:
  theta=min(max((q.x-2)/6,0),1)*1.6;cross=q.z-(-.5*q.x-2)
  thumb=Vector((2+2.8*math.sin(theta)-cross*math.sin(theta),q.y,-3+2.8*(1-math.cos(theta))+cross*math.cos(theta)))
  t=t.lerp(thumb,thumb_weight)
 if q.x>2.0 and q.y>-2 and q.z<-2.5 and w>.9:thumb_tip_ids.append(v.index)
 target=p.lerp(base+basis@t,min(max((w-.5)/.4,0),1));distance=(from_hand@target-v.co).length*.01
 if distance>1e-7:changes.append([v.index,distance]);v.co=from_hand@target
# Two restrained, seam-coherent smoothing iterations blunt the pointed thumb.
def key(v):return tuple(round(c,4) for c in v.co)
keys={v.index:key(v) for v in mesh.data.vertices};positions={key(v):v.co.copy() for v in mesh.data.vertices};adj={k:set() for k in positions}
for e in mesh.data.edges:
 a,b=(keys[i] for i in e.vertices);adj[a].add(b);adj[b].add(a)
selected={keys[i] for i in thumb_tip_ids}
for iteration in range(4):
 updates={k:positions[k].lerp(sum((positions[n] for n in adj[k]),Vector())/len(adj[k]),.35) for k in selected if adj[k]}
 positions.update(updates)
for v in mesh.data.vertices:
 if keys[v.index] in selected:v.co=positions[keys[v.index]]
# Fit only the inner contact surface to the measured oval wooden grip.
# Adaptive hand-only splits prevent long surface chords crossing the handle.
hand_world=rig.matrix_world@bone.matrix_local
fx=(hand_world.to_3x3()@finger_local).normalized();fz=(hand_world.to_3x3()@(-width_local)).normalized();fy=fz.cross(fx).normalized();fc=hand_world@(base+normal_local*radius)
fit_frame=Matrix(((fx.x,fy.x,fz.x,fc.x),(fx.y,fy.y,fz.y,fc.y),(fx.z,fy.z,fz.z,fc.z),(0,0,0,1)));tool_from_mesh=fit_frame.inverted()@mesh.matrix_world;mesh_from_tool=tool_from_mesh.inverted()
fit_report=[]
for iteration in range(2):
 bm=bmesh.new();bm.from_mesh(mesh.data);deform=bm.verts.layers.deform.active;split_edges=set()
 for face in bm.faces:
  if not all(v[deform].get(group.index,0)>.60 for v in face.verts):continue
  samples=[tool_from_mesh@face.calc_center_median()]+[tool_from_mesh@((e.verts[0].co+e.verts[1].co)*.5) for e in face.edges]
  if any(-.050<p.z<.110 and (p.x/.0135)**2+(p.y/.0115)**2<.98**2 for p in samples):split_edges.update(face.edges)
 if split_edges:bmesh.ops.subdivide_edges(bm,edges=list(split_edges),cuts=1,use_grid_fill=True)
 count=0
 for v in bm.verts:
  if v[deform].get(group.index,0)<.60:continue
  p=tool_from_mesh@v.co;rho=math.sqrt((p.x/.0135)**2+(p.y/.0115)**2)
  if -.050<p.z<.110 and rho<1.08:
   if rho<1e-7:p.x=.0135*1.08;p.y=0
   else:p.x*=1.08/rho;p.y*=1.08/rho
   v.co=mesh_from_tool@p;v[deform].clear();v[deform][group.index]=1.0;count+=1
 bm.normal_update();bm.to_mesh(mesh.data);bm.free();mesh.data.update();fit_report.append({'iteration':iteration,'split_edges':len(split_edges),'projected_vertices':count})
mesh.data.normals_split_custom_set([(0,0,0)]*len(mesh.data.loops))
# Do not replace source geometry or change the skeleton contract at this draft stage.
for img in bpy.data.images:
 if img.source=='FILE' and img.filepath:img.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(out/'grip-candidate.blend'))
# Review tool: exact final separately-authored GLB, rigid pose in measured hand frame.
pre=set(bpy.context.scene.objects);bpy.ops.import_scene.gltf(filepath='/source-workspace/horde-attendant-mallet-v1/abbey_attendant_maintenance_mallet.glb')
new=set(bpy.context.scene.objects)-pre;tool=next(o for o in new if o.type=='MESH');tool.matrix_world=Matrix.Identity(4);tool.parent=None
# Tool Blender +Z shaft maps along hand-local Z. Tool +X head maps along hand Y.
# centre x=2.5cm,y=8.5cm is the centre of the curled finger arc.
hand_world=rig.matrix_world@bone.matrix_local
x=(hand_world.to_3x3()@finger_local).normalized();z=(hand_world.to_3x3()@(-width_local)).normalized();y=z.cross(x).normalized();centre=hand_world@(base+normal_local*radius)
mat=Matrix(((x.x,y.x,z.x,centre.x),(x.y,y.y,z.y,centre.y),(x.z,y.z,z.z,centre.z),(0,0,0,1)));tool.matrix_world=mat
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=False;scene.render.resolution_x=1000;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('QAWorld');scene.world.color=(.15,.15,.15)
for loc,en in [((2,-3,4),650),((-3,-1,2),400),((1,3,3),500)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=en;o.data.size=3;o.rotation_euler=(centre-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=.52;scene.camera=cam
normal=(hand_world.to_3x3()@normal_local).normalized();finger=x
for label,direction in [('palm',normal),('side',finger),('threequarter',(normal+finger).normalized())]:
 target=centre;cam.location=target+direction*.8;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(out/(label+'.png'));bpy.ops.render.render(write_still=True)
(out/'grip-draft-metrics.json').write_text(json.dumps({'changed_vertices':len(changes),'max_displacement_m':max(x[1] for x in changes),'topology_changed':'local right-hand edge subdivision, one cut; UV/weight interpolation','uv_changed':False,'grip_world_matrix_blender':list(map(list,mat)),'contact_fit':fit_report,'status':'candidate only; visually review before final','source':str(source)},indent=2))
