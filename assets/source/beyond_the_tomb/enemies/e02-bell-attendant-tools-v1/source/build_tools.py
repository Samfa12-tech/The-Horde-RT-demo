"""Original E02 duty handbell and short maintenance staff. Blender 4.3+.
No external geometry/images or paid provider. Run with --output-dir NEW_DIRECTORY.
All distances are metres; build source is independently authored for this kit.
"""
import argparse, json, math, sys, struct, zlib
from pathlib import Path
import bpy,bmesh
import numpy as np
from mathutils import Vector
ap=argparse.ArgumentParser();ap.add_argument('--output-dir',required=True)
args=sys.argv[sys.argv.index('--')+1:];OUT=Path(ap.parse_args(args).output_dir).resolve()
for s in ['textures','previews','evidence']: (OUT/s).mkdir(parents=True,exist_ok=True)
SEED=26100602

def png(path,rgb):
 a=np.clip(np.rint(rgb*255),0,255).astype('uint8')[::-1];h,w,c=a.shape
 def chunk(k,v): return struct.pack('>I',len(v))+k+v+struct.pack('>I',zlib.crc32(k+v)&0xffffffff)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(b'\x00'+r.tobytes() for r in a),9))+chunk(b'IEND',b''))

# Original texture pixels. Shared quiet ash/iron palette, independently seeded.
n=512;y,x=np.mgrid[:n,:n]/(n-1);rng=np.random.default_rng(SEED)
fine=rng.normal(0,1,(n,n));broad=fine.copy()
for i in range(30): broad=(broad*4+np.roll(broad,1,0)+np.roll(broad,-1,0)+np.roll(broad,1,1)+np.roll(broad,-1,1))/8
broad/=broad.std()
warp=x+.002*np.sin(y*29)+.008*np.sin(y*5+x*17)
grain=np.sin(warp*870+.7*np.sin(warp*163));pores=np.maximum(0,np.sin(warp*1380-y*3))**18
wood=np.array([.227,.174,.120])+(grain*.012-pores*.019+broad*.010+fine*.002)[...,None]
wood+=np.sin(x*61+y*2)[...,None]*np.array([.014,.010,.006])
rust=np.clip((broad+.5*np.sin(x*72-y*59)-.2)*.19,0,.55)
mineral=np.clip((np.sin(x*55+y*71)+np.cos(x*47-y*103)-1.7)*.16,0,.04)
iron=np.array([.137,.147,.141])+(broad*.010+fine*.004)[...,None]
iron=iron*(1-rust[...,None])+np.array([.187,.136,.089])*rust[...,None]+mineral[...,None]*np.array([.74,.79,.67])
mask=x<.5;base=np.where(mask[...,None],wood,iron)
rough=np.where(mask,.83+.019*broad,.74+.22*rust+.018*broad)
metal=np.where(mask,0,.87*(1-rust*.85))
orm=np.stack([np.ones_like(x),rough,metal],axis=-1)
h=np.where(mask,grain*.014-pores*.016+broad*.004,broad*.006+fine*.002)
dx=(np.roll(h,-1,1)-np.roll(h,1,1))*1.2;dy=(np.roll(h,-1,0)-np.roll(h,1,0))*1.2
normal=np.stack([-dx,-dy,np.ones_like(x)],axis=-1);normal=normal/np.linalg.norm(normal,axis=-1)[...,None]*.5+.5
for name,a in [('basecolor',base),('orm',orm),('normal',normal)]:
 rgba=np.concatenate([a,np.ones((n,n,1))],axis=-1);png(OUT/'textures'/('abbey_duty_'+name+'.png'),rgba)

def clear():
 bpy.ops.wm.read_factory_settings(use_empty=True)

def material():
 m=bpy.data.materials.new('AbbeyDuty_wood_iron_512');m.use_nodes=True;m.use_backface_culling=True
 nodes,links=m.node_tree.nodes,m.node_tree.links;bsdf=nodes.get('Principled BSDF');bsdf.inputs['Alpha'].default_value=1
 t={}
 for k in ['basecolor','orm','normal']:
  v=nodes.new('ShaderNodeTexImage');v.name='Atlas_'+k;v.image=bpy.data.images.load(str(OUT/'textures'/('abbey_duty_'+k+'.png')))
  if k!='basecolor':v.image.colorspace_settings.name='Non-Color'
  t[k]=v
 links.new(t['basecolor'].outputs['Color'],bsdf.inputs['Base Color']);sep=nodes.new('ShaderNodeSeparateColor');links.new(t['orm'].outputs['Color'],sep.inputs['Color'])
 links.new(sep.outputs['Green'],bsdf.inputs['Roughness']);links.new(sep.outputs['Blue'],bsdf.inputs['Metallic'])
 nm=nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.55;links.new(t['normal'].outputs['Color'],nm.inputs['Color']);links.new(nm.outputs['Normal'],bsdf.inputs['Normal'])
 group=bpy.data.node_groups.new('glTF Material Output','ShaderNodeTree');group.interface.new_socket(name='Occlusion',in_out='INPUT',socket_type='NodeSocketFloat')
 node=nodes.new('ShaderNodeGroup');node.node_tree=group;links.new(sep.outputs['Red'],node.inputs['Occlusion'])
 return m

class MeshBuilder:
 def __init__(self):self.v=[];self.f=[];self.uv=[];self.sm=[];self.parts=[]
 def rings(self,name,rings,rect,caps=True,smooth=True):
  base=len(self.v);N=len(rings[0]);K=len(rings);self.v.extend(p for r in rings for p in r);u0,v0,u1,v1=rect
  # UV v is arc-length along ring path, retaining shape without collapsed bands.
  centers=[np.array(r).mean(axis=0) for r in rings]
  lengths=[0.0]
  for a,b in zip(rings,rings[1:]): lengths.append(lengths[-1]+float(np.mean(np.linalg.norm(np.array(b)-np.array(a),axis=1))))
  lengths=[v/lengths[-1] for v in lengths]
  for k in range(K-1):
   for j in range(N):
    self.f.append((base+k*N+j,base+k*N+(j+1)%N,base+(k+1)*N+(j+1)%N,base+(k+1)*N+j))
    self.uv.append([(u0+(u1-u0)*j/N,v0+(v1-v0)*lengths[k]),(u0+(u1-u0)*(j+1)/N,v0+(v1-v0)*lengths[k]),(u0+(u1-u0)*(j+1)/N,v0+(v1-v0)*lengths[k+1]),(u0+(u1-u0)*j/N,v0+(v1-v0)*lengths[k+1])]);self.sm.append(smooth)
  if caps:
   for k,rev in [(0,True),(K-1,False)]:
    js=list(range(N))[::-1] if rev else list(range(N));self.f.append([base+k*N+j for j in js]);self.uv.append([((u0+u1)/2+.12*math.cos(j*2*math.pi/N),(v0+v1)/2+min(.09,(v1-v0)*.3)*math.sin(j*2*math.pi/N)) for j in js]);self.sm.append(False)
  self.parts.append({'name':name,'vertices':N*K,'closed_caps':caps})
 def lathe(self,name,profile,N,rect,smooth=True):
  rings=[[(r*math.cos(j*2*math.pi/N),r*math.sin(j*2*math.pi/N),z) for j in range(N)] for z,r in profile]
  self.rings(name,rings,rect,True,smooth)
 def object(self,name,mat,parent,origin=(0,0,0)):
  me=bpy.data.meshes.new(name+'_editable');me.from_pydata(self.v,[],self.f);me.update();uv=me.uv_layers.new(name='UVMap')
  for p,coords,sm in zip(me.polygons,self.uv,self.sm):
   p.use_smooth=sm
   for li,co in zip(p.loop_indices,coords):uv.data[li].uv=co
  bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(me);bm.free()
  for v in me.vertices:v.co-=Vector(origin)
  me.update();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);o.data.materials.append(mat);o.parent=parent;o.location=origin
  tr=o.modifiers.new('Export triangulation','TRIANGULATE');tr.quad_method='FIXED';tr.ngon_method='BEAUTY'
  o['component_description']=json.dumps(self.parts);return o

def root(name,kind):
 o=bpy.data.objects.new(name,None);bpy.context.collection.objects.link(o);o.empty_display_type='ARROWS';o.empty_display_size=.065
 o['unit']='metres';o['grip_reference']='Provisional authoring origin, not a measured H1 hand socket';o['asset_role']='E02 accessory candidate; no new body or rig'
 return o

def stage(target,extent):
 s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1;s.render.engine='CYCLES';s.cycles.samples=48;s.cycles.use_denoising=False
 s.render.resolution_x=900;s.render.resolution_y=900;s.render.resolution_percentage=100;s.render.image_settings.file_format='PNG';s.render.film_transparent=False
 w=bpy.data.worlds.new('ReviewWorld');s.world=w;w.use_nodes=True;w.node_tree.nodes['Background'].inputs['Color'].default_value=(.065,.074,.082,1);w.node_tree.nodes['Background'].inputs['Strength'].default_value=.55
 s.view_settings.view_transform='AgX';s.view_settings.look='AgX - Medium High Contrast'
 # Fixed broad lights; moderate neutral exposure, no beauty-stage glow.
 for name,loc,power,size,col in [('Key',(-.45,-.55,.65),20,.65,(1,.91,.81)),('Fill',(.5,-.25,.1),11,.5,(.8,.88,1)),('Rim',(0,.5,.45),19,.5,(.93,1,.96))]:
  ld=bpy.data.lights.new(name,'AREA');ld.energy=power;ld.shape='DISK';ld.size=size;ld.color=col
  ob=bpy.data.objects.new(name,ld);bpy.context.collection.objects.link(ob);ob.location=loc;ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
 c=bpy.data.cameras.new('ReviewCamera');cam=bpy.data.objects.new('ReviewCamera',c);bpy.context.collection.objects.link(cam);s.camera=cam;c.type='ORTHO';c.ortho_scale=extent
 return s,cam

def render(scene,cam,name,loc,target,scale):
 cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
 inspection=None
 if 'open_mouth' in name:
  ld=bpy.data.lights.new('Inspection_underfill','AREA');ld.energy=4;ld.shape='DISK';ld.size=.25
  inspection=bpy.data.objects.new('Inspection_underfill',ld);bpy.context.collection.objects.link(inspection);inspection.location=(.1,-.28,-.40);inspection.rotation_euler=(Vector((0,0,-.1))-inspection.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(OUT/'previews'/name);bpy.ops.render.render(write_still=True)
 if inspection:bpy.data.objects.remove(inspection,do_unlink=True)

def save_export(stem,root,objs):
 bpy.ops.object.select_all(action='DESELECT');root.select_set(True)
 for ob in objs:ob.select_set(True)
 bpy.context.view_layer.objects.active=objs[0]
 for im in bpy.data.images:
  if im.source=='FILE':im.pack()
 bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(stem+'.blend')))
 bpy.ops.export_scene.gltf(filepath=str(OUT/(stem+'.glb')),export_format='GLB',use_selection=True,export_apply=True,export_texcoords=True,export_normals=True,export_tangents=True,export_materials='EXPORT',export_yup=True,export_extras=True)

# HAND BELL: complete hollow cup, capped crown, rounded lip, detached swinging clapper.
clear();mat=material();grip=root('Bell_Grip','bell');grip['axis_blender']='Handle +Z; bell mouth -Z';grip['grip_region_blender_z_m']='-0.030 to +0.065';grip['nominal_grip_diameter_m']='0.027 x 0.023'
b=MeshBuilder()
# Path descends outer wall, turns through lip, rises inner wall. Caps close crown solid.
profile=[(-.044,.009),(-.047,.023),(-.061,.028),(-.082,.030),(-.111,.037),(-.143,.049),(-.175,.063),(-.193,.075),(-.198,.077),(-.203,.076),(-.206,.072),(-.203,.069),(-.196,.070),(-.190,.069),(-.173,.059),(-.141,.045),(-.109,.033),(-.081,.026),(-.061,.024),(-.052,.018),(-.051,.009)]
b.lathe('Cast iron hollow bell with rolled modest lip',profile,24,(.535,.03,.965,.97),True)
rings=[]
for z,rx,ry in [(-.049,.009,.008),(-.031,.0125,.011),(-.020,.0135,.0115),(.035,.0135,.0115),(.065,.0127,.011),(.079,.015,.0128),(.085,.0145,.0125),(.089,.012,.010)]:
 rings.append([(rx*math.cos(j*2*math.pi/12),ry*math.sin(j*2*math.pi/12),z) for j in range(12)])
b.rings('Worn oval ash hand grip',rings,(.025,.045,.465,.965),True,True)
b.lathe('Plain iron handle ferrule',[(-.050,.010),(-.047,.014),(-.032,.014),(-.030,.013)],12,(.56,.76,.94,.92),False)
b.lathe('Internal clapper suspension boss',[(-.065,.005),(-.063,.006),(-.051,.0065),(-.049,.006)],12,(.55,.12,.95,.23),False)
body=b.object('Bell_BodyAndHandle',mat,grip)
c=MeshBuilder();pivot=(0,0,-.059)
c.lathe('Clapper short iron suspension stem',[(-.175,.0035),(-.160,.004),(-.074,.0035),(-.057,.004)],10,(.545,.11,.955,.57),True)
c.lathe('Clapper weighted striking end',[(-.195,.003),(-.192,.011),(-.184,.0165),(-.176,.017),(-.168,.012),(-.164,.004)],16,(.545,.60,.955,.94),True)
clapper=c.object('Bell_Clapper_Pivot',mat,grip,pivot);clapper['pivot_blender_m']=list(pivot);clapper['swing_reference']='Local X/Y swing pivot; rest pose only, no animation or physical constraint';clapper['range_note']='Clearance/timing for contact must be solved during measured hand and alarm animation authoring'
sc,cam=stage((0,0,-.06),.36)
render(sc,cam,'01_bell_front.png',(0,-.8,-.059),(0,0,-.059),.355)
render(sc,cam,'02_bell_three_quarter.png',(.42,-.7,.16),(0,0,-.059),.36)
render(sc,cam,'03_bell_open_mouth_clapper.png',(.23,-.38,-.45),(0,0,-.154),.215)
render(sc,cam,'04_bell_grip.png',(.23,-.50,.18),(0,0,.015),.165)
cam.location=(.42,-.7,.16);cam.rotation_euler=(Vector((0,0,-.059))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=.36
save_export('abbey_bell_attendant_handbell',grip,[body,clapper])

# SHORT STAFF: plain oval ash stick, two work-worn iron end shoes, no weapon ornament.
clear();mat=material();grip=root('Staff_Grip','staff');grip['axis_blender']='Shaft +Z toward longer working end';grip['grip_region_blender_z_m']='-0.050 to +0.060';grip['nominal_grip_diameter_m']='0.029 x 0.026'
s=MeshBuilder();rings=[]
for k,(z,rx,ry) in enumerate([(-.315,.0115,.0108),(-.305,.013,.0118),(-.265,.0132,.012),(-.190,.0137,.0123),(-.090,.0144,.0129),(-.050,.0145,.013),(.010,.0145,.013),(.060,.0145,.013),(.15,.0140,.0125),(.265,.0135,.012),(.385,.0128,.0117),(.485,.0123,.0114),(.535,.012,.0111),(.545,.0115,.0106)]):
 cx=.0015*math.sin((z+.315)/.86*math.pi);cy=.0009*math.sin((z+.315)/.86*math.pi*1.5)
 rings.append([(cx+rx*math.cos(j*2*math.pi/12)*(1+.006*math.sin(j*3+k*.5)),cy+ry*math.sin(j*2*math.pi/12),z) for j in range(12)])
s.rings('Short oval ash maintenance shaft',rings,(.025,.025,.465,.975),True,True)
s.lathe('Bottom iron end shoe',[(-.330,.0105),(-.328,.014),(-.322,.0147),(-.281,.0143),(-.278,.0136)],12,(.54,.065,.96,.39),False)
s.lathe('Working end iron cap',[ (.510,.0132),(.513,.0144),(.544,.0143),(.549,.0125),(.550,.009)],12,(.54,.55,.96,.94),False)
shaft=s.object('Staff_Complete',mat,grip);shaft['total_length_m']=.88;shaft['end_cap_note']='Modest blunt reinforcing shoes, no blade or hook';shaft['grip_note']='Provisional 110 mm zone; longer end points +Z; no measured H1 socket binding'
sc,cam=stage((0,0,.11),1.0)
render(sc,cam,'05_staff_front.png',(0,-1.7,.11),(0,0,.11),1.01)
render(sc,cam,'06_staff_three_quarter.png',(.55,-1.3,.37),(0,0,.11),1.01)
render(sc,cam,'07_staff_grip.png',(.22,-.55,.115),(0,0,.010),.21)
render(sc,cam,'08_staff_working_end.png',(.30,-.60,.62),(0,0,.518),.13)
cam.location=(.55,-1.3,.37);cam.rotation_euler=(Vector((0,0,.11))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.01
save_export('abbey_bell_attendant_short_staff',grip,[shaft])
spec={'role':'E02 proposed bell attendant accessory pair within H1 shared body/rig','new_bodies':0,'character_rigs':0,'seed':SEED,'unit':'metre','handbell':{'source_stem':'abbey_bell_attendant_handbell','overall_height_m':.295,'bell_max_diameter_m':.154,'nominal_grip_cross_section_m':[.027,.023],'grip_region_blender_z_m':[-.030,.065],'axis_blender':'Handle +Z away from bell, bell mouth -Z','axis_gltf':'Handle +Y away from bell, bell mouth -Y','root_origin_m':[0,0,0],'clapper_pivot_blender_m':[0,0,-.059],'clapper_pivot_gltf_m':[0,-.059,0],'meshes':2,'clapper':'Separate pivoted mesh; rest pose only'},'short_staff':{'source_stem':'abbey_bell_attendant_short_staff','overall_length_m':.880,'nominal_grip_cross_section_m':[.029,.026],'grip_region_blender_z_m':[-.050,.060],'axis_blender':'Longer working end +Z','axis_gltf':'Longer working end +Y','root_origin_m':[0,0,0],'meshes':1},'gltf_axis_mapping':'Blender XYZ -> glTF X,Z,-Y; metre units preserved','materials':{'per_export':1,'shared_atlas':True,'resolution':[512,512],'basecolor':'sRGB opaque PNG','orm':'linear PNG: neutral R=1 AO; G roughness; B metallic','normal':'linear PNG, OpenGL tangent +Y, strength .55','ao_note':'No baked AO claimed'},'budget_proposal':{'triangles_per_master_max':2000,'material_slots_per_mesh_max':1,'texture_side_max':512,'alpha':'OPAQUE','purpose':'Phone-conscious starting proposal, not runtime certification'},'excluded':['No body or new rig','No shoulder yoke authored','No lore marks or magical effects','No alarm audio or attack animation','No paid provider or copied character/torch','No engine integration/Git'],'attachment_status':'Grip reference only. Measure against corrected H1 hand and selected handedness; do not treat this source root as a calibrated socket.','rights':'Original local authored geometry and texture pixels. No third-party asset inputs or new public licence.'}
(OUT/'evidence'/'asset_specification.json').write_text(json.dumps(spec,indent=2))
print('BUILD_COMPLETE',OUT)
