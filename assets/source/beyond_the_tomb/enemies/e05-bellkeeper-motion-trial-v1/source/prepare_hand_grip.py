import bpy,sys,pathlib,json,math
from mathutils import Vector,Matrix
source,out=sys.argv[sys.argv.index('--')+1:];out=pathlib.Path(out);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=source);scene=bpy.context.scene;rig=next(o for o in scene.objects if o.type=='ARMATURE');mesh=bpy.data.objects['char1'];rig.animation_data_clear();rig.data.pose_position='REST';group=mesh.vertex_groups['RightHand'];wrist=rig.matrix_world@rig.data.bones['RightHand'].head_local
finger=Vector((-.32,-.25,-.915)).normalized();normal=Vector((1,0,-.3));normal=(normal-finger*normal.dot(finger)).normalized();width=normal.cross(finger).normalized();basis=Matrix((normal,finger,width)).transposed();radius=.025;hinge=.105
changed=[]
for v in mesh.data.vertices:
 w=sum(g.weight for g in v.groups if g.group==group.index)
 if w<.5:continue
 p=mesh.matrix_world@v.co;q=basis.transposed()@(p-wrist);t=q.copy();d=q.y-hinge
 # Continuous finger curl around a across-palm cylinder, preserving thickness.
 if d>0:
  theta=min(d*29,3.0);t.x=radius-(radius-q.x)*math.cos(theta);t.y=hinge+(radius-q.x)*math.sin(theta)
  fade=min(d/.016,1);fade=fade*fade*(3-2*fade);t=q.lerp(t,fade)
 # Conservative thumb closure across the front end of the grip; retain its surface.
 thumb=min(max((q.x-.020)/.035,0),1)*min(max((-.055-q.z)/.035,0),1)*min(max((q.y-.04)/.025,0),1)
 if thumb>0:t=t.lerp(Vector((.051, .086+(q.y-.086)*.5,-.045+(q.z+.08)*.55)),thumb*.75)
 target=wrist+basis@t;weight=min(max((w-.5)/.4,0),1);target=p.lerp(target,weight)
 if (target-p).length>1e-7:changed.append([v.index,(target-p).length]);v.co=mesh.matrix_world.inverted()@target
mesh.data.normals_split_custom_set([(0,0,0)]*len(mesh.data.loops));mesh.data.update()
centre=wrist+finger*hinge+normal*radius+width*(-.02);x=finger;z=-width;y=z.cross(x).normalized();m=Matrix(((x.x,y.x,z.x,centre.x),(x.y,y.y,z.y,centre.y),(x.z,y.z,z.z,centre.z),(0,0,0,1)))
for mat in mesh.data.materials:
 for node in mat.node_tree.nodes:
  if node.type=='TEX_IMAGE' and node.image:
   name=node.image.name.split('.')[0]
   path=pathlib.Path('/source-workspace/horde-meshy/bellkeeper/accepted-core-v03f')/(name+'.png')
   if path.exists():
    im=bpy.data.images.load(str(path),check_existing=False);im.colorspace_settings.name='sRGB' if 'basecolor' in name else 'Non-Color';im.pack();node.image=im
bpy.ops.wm.save_as_mainfile(filepath=str(out/'grip-body-draft.blend'))
(out/'grip-definition.json').write_text(json.dumps({'status':'First local hand-curl/socket draft; visual and pose validation pending','source':source,'changed_vertices':len(changed),'max_displacement_m':max(d for _,d in changed),'wrist_world':list(wrist),'normal_world':list(normal),'finger_world':list(finger),'width_world':list(width),'grip_world_matrix_blender':list(map(list,m)),'topology_changed':False},indent=2)+'\n')
pre=set(scene.objects);bpy.ops.import_scene.gltf(filepath='/source-workspace/horde-meshy/bellkeeper/motion-trial-v01/bell-striker-v01/bellkeeper_short_maul.glb');tool=next(o for o in set(scene.objects)-pre if o.type=='MESH');tool.parent=None;tool.matrix_world=m
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=False;scene.render.resolution_x=700;scene.render.resolution_y=700;scene.render.resolution_percentage=100;scene.world=bpy.data.worlds.new('GripQA');scene.world.color=(.2,.2,.2)
for loc,en in [((2,-3,4),650),((-3,-1,2),400),((1,3,3),500)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=en;o.data.size=3;o.rotation_euler=(centre-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=.38
for label,direction in [('palm',normal),('outer',-normal),('front',(normal+width).normalized())]:
 cam.location=centre+direction;cam.rotation_euler=(centre-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(out/(label+'.png'));bpy.ops.render.render(write_still=True)
