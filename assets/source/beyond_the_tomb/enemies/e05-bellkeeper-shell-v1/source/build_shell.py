"""Original E05 authoring scaffold. Runs only against an inspected normalized core and explicit fit JSON.
No provider calls, external assets, runtime/gameplay code, or scene integration.
"""
import bpy, math, json, argparse, sys, os, hashlib
from pathlib import Path
from mathutils import Vector
import numpy as np

p=argparse.ArgumentParser()
p.add_argument('--core',required=True)
p.add_argument('--fit-json',required=True)
p.add_argument('--output-dir',required=True)
a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
OUT=Path(a.output_dir); FIT=json.load(open(a.fit_json)); CORE=Path(a.core)
assert FIT.get('inspected') is True, 'Inspected core fit required; do not guess final geometry.'
assert CORE.exists()
for d in ['source','textures','previews','evidence']: (OUT/d).mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'; bpy.context.scene.unit_settings.scale_length=1
bpy.ops.import_scene.gltf(filepath=str(CORE))
core_objects=list(bpy.context.scene.objects)
for o in core_objects:
    o['asset_role']='Meshy inner core; fit reference only'
    if o.type=='MESH':
        for poly in o.data.polygons:poly.use_smooth=True
        if not o.data.materials:
            cm=bpy.data.materials.get('CORE_Untextured_ReviewOnly') or bpy.data.materials.new('CORE_Untextured_ReviewOnly');cm.use_nodes=True;bs=cm.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.085,.085,.082,1);bs.inputs['Roughness'].default_value=.9;o.data.materials.append(cm)
# The input itself remains immutable. Transform fit references only if recorded in fit JSON.
for o in core_objects:
    if o.parent is None:
        o.rotation_euler.rotate_axis('Z',math.radians(FIT.get('core_rotation_z_degrees',0)))
        o.scale*=FIT.get('core_uniform_scale',1.0)
        o.location+=Vector(FIT.get('core_offset',[0,0,0]))
bpy.context.view_layer.update()
CORE_COLLECTION=bpy.data.collections.new('CORE_FIT_REFERENCE'); bpy.context.scene.collection.children.link(CORE_COLLECTION)
for o in core_objects:
    for c in list(o.users_collection): c.objects.unlink(o)
    CORE_COLLECTION.objects.link(o)
SHELL_COLLECTION=bpy.data.collections.new('ORIGINAL_MODULAR_SHELL'); bpy.context.scene.collection.children.link(SHELL_COLLECTION)

def relink(o):
    for c in list(o.users_collection): c.objects.unlink(o)
    SHELL_COLLECTION.objects.link(o)
    return o

def empty(name,loc=(0,0,0),parent=None):
    o=bpy.data.objects.new(name,None); SHELL_COLLECTION.objects.link(o)
    o.empty_display_type='ARROWS'; o.empty_display_size=.06
    o.location=loc
    bpy.context.view_layer.update()
    if parent:
        m=o.matrix_world.copy(); o.parent=parent; o.matrix_world=m
        bpy.context.view_layer.update()
    return o

root=empty('E05_ModularShell_ROOT')
root['asset_id']='E05'; root['status']='PROPOSAL: source-only; no engine validation'
root['axis_convention']='Blender +Z up, -Y forward, +X character left; GLB Y-up export'
# Original seeded weathered iron texture pixels. No photographic or provider inputs.
N=1024; rng=np.random.default_rng(26100605)
x,y=np.meshgrid(np.arange(N)/N,np.arange(N)/N)
coarse=(np.sin(2*math.pi*(x*3+y*2))*.5+np.cos(2*math.pi*(x*7-y*5))*.25+np.sin(2*math.pi*(x*19+y*11))*.13)
fine=rng.normal(0,1,(N,N)); marks=np.maximum(0,np.sin(2*math.pi*(x*29+y*3))-.91)
corrosion=np.clip((coarse-.28)*.48,0,.18)
base=np.stack([.125+.015*coarse,.139+.019*coarse,.145+.022*coarse],axis=-1)
base+=fine[:,:,None]*.003
base=base*(1-corrosion[:,:,None])+np.array([.135,.111,.080])*corrosion[:,:,None]
base-=marks[:,:,None]*.085
rough=np.clip(.58+.13*coarse+.045*fine+corrosion*.2,.4,.92)
metal=np.clip(.84-corrosion*.57,.38,.88)
height=coarse*.1+fine*.014-marks*.14
ny,nx=np.gradient(height)
normal=np.stack([-nx*.6,-ny*.6,np.ones_like(x)],axis=-1); normal/=np.linalg.norm(normal,axis=-1)[:,:,None]; normal=normal*.5+.5

def image(name,rgb,noncolor=False):
    im=bpy.data.images.new(name,width=N,height=N,alpha=False)
    if noncolor: im.colorspace_settings.name='Non-Color'
    rgba=np.concatenate([np.clip(rgb,0,1),np.ones((N,N,1))],axis=-1).astype(np.float32)
    im.pixels.foreach_set(rgba.ravel()); im.filepath_raw=str(OUT/'textures'/f'{name}.png'); im.file_format='PNG'; im.save(); im.pack()
    return im
baseim=image('e05_shell_basecolor',base)
ormim=image('e05_shell_orm',np.stack([np.ones_like(x),rough,metal],axis=-1),True)
normim=image('e05_shell_normal',normal,True)
mat=bpy.data.materials.new('E05_WeatheredIron_1K'); mat.use_nodes=True
n=mat.node_tree.nodes; n.clear(); l=mat.node_tree.links
out=n.new('ShaderNodeOutputMaterial'); bs=n.new('ShaderNodeBsdfPrincipled'); l.new(bs.outputs['BSDF'],out.inputs['Surface'])
bt=n.new('ShaderNodeTexImage'); bt.image=baseim; bt.label='Original procedural iron base colour'; l.new(bt.outputs['Color'],bs.inputs['Base Color'])
ot=n.new('ShaderNodeTexImage'); ot.image=ormim
sep=n.new('ShaderNodeSeparateColor'); l.new(ot.outputs['Color'],sep.inputs[0]); l.new(sep.outputs['Green'],bs.inputs['Roughness']); l.new(sep.outputs['Blue'],bs.inputs['Metallic'])
nt=n.new('ShaderNodeTexImage'); nt.image=normim
nm=n.new('ShaderNodeNormalMap'); nm.inputs['Strength'].default_value=.4; l.new(nt.outputs['Color'],nm.inputs['Color']); l.new(nm.outputs['Normal'],bs.inputs['Normal'])
mat.diffuse_color=(.13,.14,.15,1)
meshes=[]

def mesh(name,verts,faces,parent=root,thickness=0,bevel=0):
    me=bpy.data.meshes.new(name+'_mesh'); me.from_pydata(verts,[],faces); me.update()
    o=bpy.data.objects.new(name,me); SHELL_COLLECTION.objects.link(o); o.data.materials.append(mat)
    if parent: o.parent=parent
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active=o; o.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    if thickness:
        m=o.modifiers.new('OriginalPlateThickness','SOLIDIFY'); m.thickness=thickness; m.offset=0; bpy.ops.object.modifier_apply(modifier=m.name)
    if bevel:
        m=o.modifiers.new('SmallForgedEdge','BEVEL'); m.width=bevel; m.segments=1; m.affect='EDGES'; m.angle_limit=.5
        bpy.ops.object.modifier_apply(modifier=m.name)
    for poly in o.data.polygons: poly.use_smooth=True
    if hasattr(o.data,'set_sharp_from_angle'): o.data.set_sharp_from_angle(angle=math.radians(35))
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(angle_limit=1.1519,island_margin=.012)
    bpy.ops.object.mode_set(mode='OBJECT'); o.select_set(False)
    o['authorship']='Original locally authored procedural geometry; no external mesh input'
    o['source_role']='Articulated rigid armour proposal'
    meshes.append(o); return o

def ringpatch(name,rows,angles,parent=root,thickness=.007):
    # rows = (z, radius_x, radius_y, centre_y); angles measured from rear +Y.
    angles=list(angles); closed=abs((angles[-1]-angles[0])-2*math.pi)<1e-5
    if closed: angles=angles[:-1]
    vs=[(rx*math.sin(t),cy+ry*math.cos(t),z) for z,rx,ry,cy in rows for t in angles]
    k=len(angles); fs=[]
    for j in range(len(rows)-1):
        for i in range(k if closed else k-1):
            ni=(i+1)%k; fs.append((j*k+i,j*k+ni,(j+1)*k+ni,(j+1)*k+i))
    return mesh(name,vs,fs,parent,thickness)

# Fit dimensions are explicitly recorded from the inspected body, in metres.
c=FIT['chest']; h=FIT['head']; sh=FIT['shoulders']
chest=empty('ChestMount',c['mount'],root); head=empty('HeadMount',h['mount'],root)
# Reset mount-parent world transforms explicitly; all mesh coordinates below are world-authoring coordinates.
for m in [chest,head]:
    m['attachment_status']='Measured fit reference; runtime bone assignment not implemented'
    m['fit_source_sha256']=hashlib.sha256(CORE.read_bytes()).hexdigest()
# Preserve world-coordinate mesh authoring beneath chest/head sockets.
def parent_world(o,parent):
    bpy.context.view_layer.update()
    world=o.matrix_world.copy(); o.parent=parent; o.matrix_world=world
    bpy.context.view_layer.update()
    return o

back=ringpatch('Cuirass_BackAndSides',c['back_rows'],np.linspace(-c['back_arc_radians'],c['back_arc_radians'],17),root,.008)
parent_world(back,chest)
# Neck ring is a low gorget with a true central aperture.
gorget=ringpatch('Gorget_NeckRing',c['gorget_rows'],np.linspace(-math.pi,math.pi,21),root,.007); parent_world(gorget,chest)
# Breastplate halves use a closed explicit volume, and remain independently selectable.
hinges={}; shutters={}
for side,sgn in [('L',1),('R',-1)]:
    hinge=empty('ShutterHinge.'+side,(sgn*c['hinge_x'],c['hinge_y'],c['hinge_z']),root)
    hinge['axis_local']='Z'; hinge['closed_angle_degrees']=0.0; hinge['open_angle_degrees']=sgn*c['open_angle_degrees']
    hinge['behaviour']='Proposal only: actual reflected light path must authorize exposure in future gameplay'
    parent_world(hinge,chest); hinges[side]=hinge
    verts=[]; nr=len(c['shutter_rows']); cols=6
    for j,(z,width,front_y) in enumerate(c['shutter_rows']):
        for i in range(cols):
            t=i/(cols-1); xx=sgn*(c['centre_seam_halfgap']*(1-t)+width*t)
            zz=z+(c.get('top_centre_raise',.035)*(1-t) if j==nr-1 else 0)-(c.get('bottom_centre_drop',.028)*(1-t) if j==0 else 0)
            yy=front_y+c.get('plate_convexity',.035)*t*t
            verts.append((xx,yy,zz))
    faces=[(j*cols+i,j*cols+i+1,(j+1)*cols+i+1,(j+1)*cols+i) for j in range(nr-1) for i in range(cols-1)]
    plate=mesh('BreastShutter.'+side,verts,faces,root,.009,.0012); parent_world(plate,hinge); shutters[side]=plate
    # Two small barrels at each vertical hinge. They belong to the fixed cuirass.
    for zi,z in enumerate(c['hinge_barrel_z']):
        bpy.ops.mesh.primitive_cylinder_add(vertices=10,radius=.012,depth=.053,location=(sgn*c['hinge_x'],c['hinge_y'],z))
        o=bpy.context.object; o.name='ShutterHingeBarrel.'+side+'.'+str(zi); relink(o); o.data.materials.append(mat)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(island_margin=.015); bpy.ops.object.mode_set(mode='OBJECT')
        parent_world(o,chest); meshes.append(o); o.select_set(False)
        # Original fixed hinge cleat spans side cuirass to each barrel.
        xx=sgn*(c['hinge_x']-.017); yy=c['hinge_y']+.018
        vv=[(xx+dx,yy+dy,z+dz) for dx in [-.020,.020] for dy in [-.035,.035] for dz in [-.009,.009]]
        ff=[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)]
        cleat=mesh('FixedHingeCleat.'+side+'.'+str(zi),vv,ff,root);parent_world(cleat,chest)
# Helmet crown with a subtly peaked central ridge, plus separate cheek/visor volume and real sight slit.
crown=ringpatch('Helmet_Crown',h['crown_rows'],np.linspace(-math.pi,math.pi,21),root,.007)
parent_world(crown,head)
# Close top with original broad facets (no ornamental crest).
z,rx,ry,cy=h['crown_rows'][-1]
capverts=[(rx*math.sin(t),cy+ry*math.cos(t),z) for t in np.linspace(-math.pi,math.pi,20,endpoint=False)]+[(0,cy,z+.012)]
cap=mesh('Helmet_Top',capverts,[(i,(i+1)%20,20) for i in range(20)],root,.005); parent_world(cap,head)
rear=ringpatch('Helmet_RearSkirt',h['rear_rows'],np.linspace(-h['rear_arc_radians'],h['rear_arc_radians'],19),root,.007); parent_world(rear,head)
# Front visor below sight opening; armour belongs to the same head mount but is not fused.
visor=ringpatch('Helmet_Visor',h['visor_rows'],np.linspace(math.pi-h['visor_half_arc'],math.pi+h['visor_half_arc'],17),root,.007)
parent_world(visor,head)
# Shoulder lames are separate rigid components with deliberate axilla and neck gaps.
for side,sgn in [('L',1),('R',-1)]:
    socket=empty('Shoulder.'+side,(sgn*sh['centre_x'],sh['centre_y'],sh['centre_z']),root)
    socket['attachment_status']='Upper-arm rigid attachment proposal; final skin/animation contract unresolved'
    for layer in range(2):
        vs=[]; fs=[]; rings=4; seg=12
        for j in range(rings):
            t=j/(rings-1); xx=sgn*(sh['start_x']+layer*sh['layer_step']+t*sh['length'])
            radius=sh['radius']*([.30,.91,1.0,.85][j] if layer==0 else [.80,.92,.89,.69][j])
            for i in range(seg+1):
                aa=-math.pi*.68+i/seg*math.pi*1.36
                vs.append((xx,sh['centre_y']+math.sin(aa)*radius,sh['centre_z']+sh.get('slope_z_per_x',0)*(abs(xx)-sh['centre_x'])+math.cos(aa)*radius))
        for j in range(rings-1):
            for i in range(seg): fs.append((j*(seg+1)+i,j*(seg+1)+i+1,(j+1)*(seg+1)+i+1,(j+1)*(seg+1)+i))
        o=mesh('ShoulderLame.'+side+'.'+str(layer+1),vs,fs,root,.006); parent_world(o,socket)
# Invisible authoring sockets, not target art or a invented receiver emblem.
receiver=empty('ReflectedPathReceiver_PROPOSAL',c['receiver'],root); parent_world(receiver,chest)
receiver['status']='Position/occlusion proposal only; ordinary held light must not substitute for placed reflector'
exposure=empty('ExposureCentre_PROPOSAL',c['exposure_centre'],root); parent_world(exposure,chest)
exposure['status']='Unlit core area exposed by separate shutters; attack validity/timing not supplied'

# Closed/open keyframes are inspection states, not combat timing. Keep frame labels explicit.
scene=bpy.context.scene; scene.frame_start=1; scene.frame_end=60
scene.timeline_markers.new('CLOSED_INSPECTION_STATE',frame=1)
scene.timeline_markers.new('OPEN_INSPECTION_STATE_NOT_GAMEPLAY_TIMING',frame=60)
for side,hinge in hinges.items():
    sgn=1 if side=='L' else -1
    hinge.rotation_euler.z=0; hinge.keyframe_insert(data_path='rotation_euler',frame=1)
    hinge.rotation_euler.z=math.radians(sgn*c['open_angle_degrees']); hinge.keyframe_insert(data_path='rotation_euler',frame=60)
    if hinge.animation_data and hinge.animation_data.action:
        hinge.animation_data.action.name='E05_Shutter_'+side+'_InspectionOpenClose'
scene.frame_set(1)
# Soft neutral studio; renders are art/source QA only.
world=bpy.data.worlds.new('NeutralStudio') if not bpy.data.worlds else bpy.data.worlds[0]; scene.world=world; world.use_nodes=True
world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.055,.065,.078,1)
world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.45

def area(name,loc,power,size,target):
    ld=bpy.data.lights.new(name,'AREA'); ld.energy=power; ld.shape='DISK'; ld.size=size
    ob=bpy.data.objects.new(name,ld); scene.collection.objects.link(ob); ob.location=loc; ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
area('Key',(-3,-4,4),900,4,(0,0,1.2)); area('Fill',(3,-2,2.5),650,3,(0,0,1.2)); area('Rim',(0,2,3.5),1000,3,(0,0,1.2))
camdata=bpy.data.cameras.new('ReviewCamera'); cam=bpy.data.objects.new('ReviewCamera',camdata); scene.collection.objects.link(cam); scene.camera=cam
camdata.type='ORTHO'; camdata.ortho_scale=2.42
scene.render.engine='CYCLES'; scene.cycles.samples=24; scene.cycles.use_denoising=False
scene.render.resolution_x=900; scene.render.resolution_y=1000; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='JPEG'; scene.render.image_settings.quality=93; scene.render.film_transparent=False
scene.view_settings.view_transform='AgX'

def render(name,loc,target=(0,0,1.05),ortho=2.42,frame=1):
    scene.frame_set(frame); cam.location=loc; cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler(); camdata.ortho_scale=ortho
    scene.render.filepath=str(OUT/'previews'/name); bpy.ops.render.render(write_still=True)

# Export only shell nodes; the inspected core remains a fit reference in .blend.
bpy.ops.object.select_all(action='DESELECT')
for o in SHELL_COLLECTION.objects: o.select_set(True)
scene.frame_set(1)
bpy.ops.export_scene.gltf(filepath=str(OUT/'bellkeeper_modular_shell.glb'),export_format='GLB',use_selection=True,export_yup=True,export_apply=True,export_animations=True,export_animation_mode='SCENE',export_extras=True,export_materials='EXPORT')
# Capture socket matrices and actual exported triangle counts separately in validator.
metadata={'asset_id':'E05','status':'Original modular source proposal; no runtime import/animation certification',
 'core_source':str(CORE),'core_sha256':hashlib.sha256(CORE.read_bytes()).hexdigest(),
 'fit':FIT,'source_counts':{},'sockets':{},'core_reference_objects':[o.name for o in core_objects]}
for o in meshes:
    o.data.calc_loop_triangles(); metadata['source_counts'][o.name]={'vertices':len(o.data.vertices),'triangles':len(o.data.loop_triangles)}
for o in SHELL_COLLECTION.objects:
    if o.type=='EMPTY': metadata['sockets'][o.name]={'world_matrix_closed':[list(row) for row in o.matrix_world],'properties':dict(o.items())}
metadata['source_shell_triangles_total']=sum(v['triangles'] for v in metadata['source_counts'].values())
metadata['canonical_requirement']='Actual placed-reflector path opens/exposes armour; detailed geometry, timing and valid attack remain proposals'
(OUT/'evidence'/'authoring_metadata.json').write_text(json.dumps(metadata,indent=2))
scene.frame_set(1); cam.location=(3,-5,2.5); cam.rotation_euler=(Vector((0,0,1.1))-cam.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'bellkeeper_modular_shell.blend'),compress=True)
render('01_closed_front.jpg',(0,-5,1.12))
render('02_open_front.jpg',(0,-5,1.12),frame=60)
render('03_open_three_quarter.jpg',(3,-5,2.0),frame=60)
render('04_closed_side.jpg',(5,-.1,1.3))
render('05_helmet_and_hinges.jpg',(2.1,-5,2.0),target=(0,-.03,1.63),ortho=1.1,frame=60)
scene.frame_set(1)
print('SHELL_BUILD_RESULT',json.dumps({'triangles':metadata['source_shell_triangles_total'],'source':str(OUT/'bellkeeper_modular_shell.blend'),'glb':str(OUT/'bellkeeper_modular_shell.glb')}))
