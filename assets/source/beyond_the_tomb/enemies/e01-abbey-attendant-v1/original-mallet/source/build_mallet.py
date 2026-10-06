"""Original Abbey attendant maintenance mallet, deterministic Blender 4.3+ build.
No external models, photographs, generators, or paid providers are used.
Run: blender -b --python source/build_mallet.py -- --output-dir /absolute/new/path
"""
import argparse, json, math, os, struct, sys, zlib
from pathlib import Path
import bpy, bmesh
import numpy as np
from mathutils import Vector

SEED = 261006
args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
ap = argparse.ArgumentParser()
ap.add_argument('--output-dir', required=True)
OUT = Path(ap.parse_args(args).output_dir).resolve()
for p in ['textures', 'previews', 'evidence']:
    (OUT / p).mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for d in list(bpy.data.materials):
    bpy.data.materials.remove(d)

def png_write(path, pixels):
    arr = np.clip(np.rint(pixels * 255), 0, 255).astype('uint8')[::-1]
    h, w, _ = arr.shape
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    raw = b''.join(b'\x00' + row.tobytes() for row in arr)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w,h,8,6,0,0,0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

def make_textures():
    n = 512
    y,x = np.mgrid[0:n,0:n] / (n-1)
    rng = np.random.default_rng(SEED)
    speckle = rng.normal(0,1,(n,n))
    soft = speckle.copy()
    for _ in range(22):
        soft = (soft*4 + np.roll(soft,1,0) + np.roll(soft,-1,0) + np.roll(soft,1,1) + np.roll(soft,-1,1))/8
    soft /= max(float(np.std(soft)), 1e-8)
    warped = x + .007*np.sin(y*11 + x*12) + .0015*np.sin(y*55)
    grain = np.sin(warped*650 + np.sin(warped*130)*2.1)
    narrow = np.maximum(0,np.sin(warped*1550 + y*5))**14
    coarse = .5*np.sin(x*45 + y*3) + .22*np.sin(x*119-y*4)
    grip_wear = np.exp(-((y-.185)/.16)**2)
    wood = np.zeros((n,n,3)) + np.array([.235,.178,.117])
    wood += (grain*.015 + narrow*(-.027) + coarse*.026 + soft*.009 + speckle*.003)[...,None]
    wood -= (grip_wear*.035)[...,None] * np.array([1,.95,.82])
    wood -= (np.exp(-((y-.79)/.06)**2)*.021)[...,None]
    iron = np.zeros((n,n,3)) + np.array([.142,.150,.143])
    iron += (soft*.012 + speckle*.008)[...,None]
    rust_signal = np.sin(x*39+y*21) + .75*np.sin(x*91-y*31) + .55*np.cos(y*101+x*27) + soft*.21
    rust = np.clip((rust_signal-.35)/2.5,0,.55)
    iron = iron*(1-rust[...,None]) + np.array([.168,.122,.080])*rust[...,None]
    mineral = np.clip((np.sin(x*74-y*32)+np.sin(y*87+x*23)-1.62)*.24,0,.06)
    iron += mineral[...,None] * np.array([.80,.85,.74])
    wood_mask = x < .5
    base = np.where(wood_mask[...,None],wood,iron)
    # Red is neutral AO. No baked occlusion is claimed.
    rough = np.where(wood_mask,.84-grip_wear*.10 + soft*.018,.81+rust*.24+soft*.012)
    metal = np.where(wood_mask,0,.82*(1-rust*.6))
    orm = np.stack([np.ones_like(x),rough,metal],axis=-1)
    height = np.where(wood_mask,.015*grain-.02*narrow + .006*soft,.009*soft + .003*speckle)
    dx = (np.roll(height,-1,1)-np.roll(height,1,1))*1.8
    dy = (np.roll(height,-1,0)-np.roll(height,1,0))*1.8
    normal = np.stack([-dx,-dy,np.ones_like(x)],axis=-1)
    normal /= np.linalg.norm(normal,axis=-1)[...,None]
    normal = normal*.5+.5
    for name,data in [('basecolor',base),('orm',orm),('normal',normal)]:
        rgba = np.concatenate([data,np.ones((n,n,1))],axis=-1)
        png_write(OUT/'textures'/('abbey_mallet_'+name+'.png'),rgba)

make_textures()
mat = bpy.data.materials.new('AbbeyMallet_PBR_Atlas')
mat.use_nodes=True
mat.use_backface_culling=True
nodes,links=mat.node_tree.nodes,mat.node_tree.links
bsdf=nodes.get('Principled BSDF')
bsdf.inputs['Roughness'].default_value=.75
bsdf.inputs['Alpha'].default_value=1
tex={}
for key in ['basecolor','orm','normal']:
    t=nodes.new('ShaderNodeTexImage')
    t.name='Atlas_'+key
    t.label=key.upper()+' 512 x 512'
    t.image=bpy.data.images.load(str(OUT/'textures'/('abbey_mallet_'+key+'.png')))
    if key!='basecolor': t.image.colorspace_settings.name='Non-Color'
    tex[key]=t
links.new(tex['basecolor'].outputs['Color'],bsdf.inputs['Base Color'])
sep=nodes.new('ShaderNodeSeparateColor')
links.new(tex['orm'].outputs['Color'],sep.inputs['Color'])
links.new(sep.outputs['Green'],bsdf.inputs['Roughness'])
links.new(sep.outputs['Blue'],bsdf.inputs['Metallic'])
norm=nodes.new('ShaderNodeNormalMap')
norm.inputs['Strength'].default_value=.6
links.new(tex['normal'].outputs['Color'],norm.inputs['Color'])
links.new(norm.outputs['Normal'],bsdf.inputs['Normal'])
# Recognized glTF AO node: ORM R is 1.0, leaving AO neutral.
group=bpy.data.node_groups.new('glTF Material Output','ShaderNodeTree')
group.interface.new_socket(name='Occlusion',in_out='INPUT',socket_type='NodeSocketFloat')
group_node=nodes.new('ShaderNodeGroup'); group_node.node_tree=group
links.new(sep.outputs['Red'],group_node.inputs['Occlusion'])

verts=[]; faces=[]; face_uv=[]; face_smooth=[]; sections=[]
def append_shell(name,rings,side_rect,cap_rects,smooth=False):
    """Closed rings, n consistent perimeter vertices; UVs separated at seam/caps."""
    start=len(verts); n=len(rings[0]); nr=len(rings)
    verts.extend(v for ring in rings for v in ring)
    u0,v0,u1,v1=side_rect
    for k in range(nr-1):
        for j in range(n):
            ids=[start+k*n+j,start+k*n+(j+1)%n,start+(k+1)*n+(j+1)%n,start+(k+1)*n+j]
            uv=[(u0+(u1-u0)*j/n,v0+(v1-v0)*k/(nr-1)),(u0+(u1-u0)*(j+1)/n,v0+(v1-v0)*k/(nr-1)),(u0+(u1-u0)*(j+1)/n,v0+(v1-v0)*(k+1)/(nr-1)),(u0+(u1-u0)*j/n,v0+(v1-v0)*(k+1)/(nr-1))]
            faces.append(ids);face_uv.append(uv);face_smooth.append(smooth)
    for k,reverse,rect in [(0,True,cap_rects[0]),(nr-1,False,cap_rects[1])]:
        js=list(reversed(range(n))) if reverse else list(range(n))
        faces.append([start+k*n+j for j in js])
        cu0,cv0,cu1,cv1=rect
        face_uv.append([((cu0+cu1)/2+math.cos(2*math.pi*j/n)*(cu1-cu0)/2,(cv0+cv1)/2+math.sin(2*math.pi*j/n)*(cv1-cv0)/2) for j in js])
        face_smooth.append(False)
    sections.append({'name':name,'vertex_start':start,'vertex_count':nr*n})

# Slightly bent, worn oval ash handle, 12-sided and 12 stations: 284 triangles.
handle_specs=[(-.055,.0122,.0105),(-.050,.0131,.0111),(-.035,.0135,.0115),(-.010,.0135,.0115),(.025,.0135,.0115),(.060,.0126,.0109),(.100,.0118,.0103),(.152,.0107,.0095),(.202,.0100,.0090),(.236,.0108,.0101),(.279,.0105,.0098),(.285,.0098,.0091)]
rings=[]
for k,(z,rx,ry) in enumerate(handle_specs):
    cx=.0014*math.sin((z+.055)/.34*math.pi)
    cy=.0012*math.sin((z+.055)/.34*math.pi*1.4)
    ring=[]
    for j in range(12):
        a=2*math.pi*j/12
        wear=1+.009*math.sin(j*3+k*.72)
        ring.append((cx+rx*math.cos(a)*wear,cy+ry*math.sin(a)*wear,z))
    rings.append(ring)
append_shell('Worn ash handle',rings,(.025,.025,.475,.830),[(.035,.862,.195,.982),(.270,.862,.430,.982)],True)

# Octagonal cross section is a clipped rectangle; broad central faces and worn end bevels.
# Axis +X, head centered at Z=.254m; two square striking faces, no spike or blade.
head_specs=[(-.060,.0180,.0208),(-.057,.0210,.0242),(-.049,.0210,.0244),(-.030,.0188,.0228),(0,.0184,.0240),(.030,.0190,.0230),(.049,.0210,.0244),(.057,.0210,.0242),(.060,.0181,.0208)]
rings=[]
for k,(x,hy,hz) in enumerate(head_specs):
    bevel=.0060
    perimeter=[(-hy,-hz+bevel),(-hy+bevel,-hz),(hy-bevel,-hz),(hy,-hz+bevel),(hy,hz-bevel),(hy-bevel,hz),(-hy+bevel,hz),(-hy,hz-bevel)]
    ring=[]
    for j,(y,z) in enumerate(perimeter):
        ding=(.0004*math.sin(j*4.1+k*2.1)) if k not in [0,len(head_specs)-1] else 0
        ring.append((x,y+ding,.254+z+ding*.6))
    rings.append(ring)
append_shell('Small forged iron head',rings,(.525,.025,.975,.660),[(.550,.705,.720,.875),(.780,.705,.950,.875)],False)

# Thin forged reinforcement collar below the head, not decoration.
collar_specs=[(.209,.0104,.0095),(.211,.0113,.0104),(.225,.0114,.0106),(.227,.0108,.0100)]
rings=[]
for z,rx,ry in collar_specs:
    rings.append([(.0004+rx*math.cos(2*math.pi*j/12),.0002+ry*math.sin(2*math.pi*j/12),z) for j in range(12)])
append_shell('Iron neck collar',rings,(.525,.902,.975,.946),[(.545,.958,.595,.990),(.650,.958,.700,.990)],False)

mesh=bpy.data.meshes.new('AbbeyMallet_OriginalMesh')
mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('AbbeyMaintenanceMallet',mesh)
bpy.context.collection.objects.link(obj)
obj.data.materials.append(mat)
uv=mesh.uv_layers.new(name='UVMap')
for poly,uvs,smooth in zip(mesh.polygons,face_uv,face_smooth):
    poly.use_smooth=smooth
    for li,co in zip(poly.loop_indices,uvs): uv.data[li].uv=co
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(mesh);bm.free();mesh.update()
grip=bpy.data.objects.new('Grip',None);bpy.context.collection.objects.link(grip)
grip.empty_display_type='ARROWS';grip.empty_display_size=.04
obj.parent=grip
grip['reference']='Provisional tool grip origin; not a measured E01 RightGrip hand transform.'
grip['handle_axis_blender']='+Z';grip['head_axis_blender']='+X'
obj['asset_id']='horde.e01.abbey-maintenance-mallet.v1'
obj['provenance']='Original deterministic local Blender geometry and procedural texture authoring.'
obj['grip_diameter_m']='0.027 x 0.023'
obj['grip_region_blender_z_m']='-0.040 to +0.060'
obj['authoring_seed']=SEED

scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1.0
scene.render.engine='CYCLES';scene.cycles.samples=64;scene.cycles.use_denoising=False
scene.render.resolution_x=1100;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.065,.074,.082,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.45
scene.view_settings.view_transform='AgX'
scene.view_settings.look='AgX - Medium High Contrast'
scene.view_settings.exposure=.0

def area(name,loc,power,size,color):
    data=bpy.data.lights.new(name,'AREA'); data.energy=power;data.shape='DISK';data.size=size;data.color=color
    light=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(light);light.location=loc
    light.rotation_euler=(Vector((0,0,.125))-light.location).to_track_quat('-Z','Y').to_euler()
area('Key_softbox',(-.30,-.45,.60),9,.45,(1,.88,.76))
area('Fill_softbox',(.30,-.10,.25),4,.30,(.77,.85,1))
area('Rim_softbox',(.0,.25,.45),10,.27,(.91,1,.96))
camera_data=bpy.data.cameras.new('EvidenceCamera');camera=bpy.data.objects.new('EvidenceCamera',camera_data);bpy.context.collection.objects.link(camera);scene.camera=camera
camera_data.type='ORTHO';camera_data.ortho_scale=.40
def render(name,loc,target,scale):
    camera.location=loc;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera_data.ortho_scale=scale
    scene.render.filepath=str(OUT/'previews'/name)
    bpy.ops.render.render(write_still=True)

bpy.context.view_layer.objects.active=obj
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);grip.select_set(True)
tri=obj.modifiers.new('Deterministic export triangulation','TRIANGULATE')
tri.quad_method='FIXED';tri.ngon_method='BEAUTY'
for im in bpy.data.images:
    if im.source=='FILE': im.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'abbey_attendant_maintenance_mallet.blend'))
bpy.ops.export_scene.gltf(filepath=str(OUT/'abbey_attendant_maintenance_mallet.glb'),export_format='GLB',use_selection=True,export_apply=True,export_texcoords=True,export_normals=True,export_tangents=True,export_materials='EXPORT',export_yup=True,export_extras=True)
render('01_front.png',(0,-.7,.115),(0,0,.115),.40)
render('02_side.png',(.7,0,.115),(0,0,.115),.40)
render('03_head_closeup.png',(.31,-.42,.40),(0,0,.237),.177)
render('04_three_quarter.png',(.39,-.65,.34),(0,0,.114),.40)
render('05_grip_closeup.png',(.30,-.60,.12),(0,0,.035),.190)
camera.location=(.39,-.65,.34)
camera.rotation_euler=(Vector((0,0,.114))-camera.location).to_track_quat('-Z','Y').to_euler()
camera_data.ortho_scale=.40
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'abbey_attendant_maintenance_mallet.blend'))

spec={'asset_id':'horde.e01.abbey-maintenance-mallet.v1','seed':SEED,'source':'Original local Blender construction, no external assets or provider generation.','source_axes':{'handle':'+Z','head':'+X','grip_origin':[0,0,0]},'gltf_axes':{'handle':'+Y','head':'+X','cross_section_minor_axis':'-Z'},'unit':'meter','handle_length_m':.340,'head_width_m':.120,'grip_nominal_diameter_m':[.027,.023],'grip_usable_z_range_m':[-.040,.060],'material_count':1,'source_mesh_count':1,'closed_shells':3,'textures':{'basecolor':'512x512 sRGB RGBA PNG, opaque alpha','orm':'512x512 linear RGBA PNG: R neutral AO=1, G roughness, B metallic','normal':'512x512 tangent +Y (OpenGL), strength .6'},'assembly_notes':'Three closed component shells overlap at mechanical joints: wooden shaft seats into solid head; neck collar reinforces shaft. No body fused to tool. No modeled eye/cavity or collision hull.','binding_status':'Provisional grip dimensions only. Final hand/socket transform requires measured repaired E01 hand; no engine binding supplied.','license_status':'Original commissioned authoring; no third-party asset license and no new public license applied.','sections':sections,'not_claimed':['Collision setup','Attack timing','Engine import','Game readiness','Measured RightGrip transform','Baked ambient occlusion']}
(OUT/'evidence'/'asset_specification.json').write_text(json.dumps(spec,indent=2))
print('BUILD_COMPLETE',OUT)
