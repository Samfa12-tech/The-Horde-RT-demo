"""Deterministic original Blender forest dressing. No external model input.
Run: blender -b --python source/build_forest_dressing.py
Generate original stone/endgrain maps first with their Python scripts. Shared bark
and leaf maps originate in shared_tree_material_provenance.py; preserve supplied
map bytes rather than running that tree generator here.
"""
import bpy, bmesh, math, json, random, os
from pathlib import Path
from mathutils import Vector, noise
ROOT=Path(__file__).resolve().parents[1]
random.seed(274113); noise.seed_set(274113)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for d in list(bpy.data.materials):bpy.data.materials.remove(d)
assets={}
def material(name,prefix,normal_strength=.5,roughness=.9):
    m=bpy.data.materials.new(name);m.use_nodes=True
    n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Metallic'].default_value=0;p.inputs['Roughness'].default_value=roughness;p.inputs['IOR'].default_value=1.4
    for suffix,socket in [('basecolor','Base Color'),('roughness','Roughness'),('normal',None)]:
        path=ROOT/'textures'/f'{prefix}-{suffix}-512.png'
        if not path.exists():continue
        t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(path),check_existing=True)
        if suffix!='basecolor':t.image.colorspace_settings.name='Non-Color'
        if suffix=='normal':
            a=n.new('ShaderNodeNormalMap');a.inputs['Strength'].default_value=normal_strength;m.node_tree.links.new(t.outputs['Color'],a.inputs['Color']);m.node_tree.links.new(a.outputs['Normal'],p.inputs['Normal'])
        else:m.node_tree.links.new(t.outputs['Color'],p.inputs[socket])
    return m
STONE=material('BH_Original_Slate','horde-stone-original',.50)
BARK=material('BH_Shared_Original_Bark','horde-bark-original',.70)
WOOD=material('BH_Original_Weathered_Endgrain','horde-endgrain-original',.40)
LEAF=material('BH_Shared_Original_Leaf','horde-alder-original',.0,.84)

def mesh_object(name,verts,faces,mat,uvs=None,indices=None,smooth=True):
    m=bpy.data.meshes.new(name);m.from_pydata(verts,[],faces);m.update();o=bpy.data.objects.new(name,m);bpy.context.collection.objects.link(o)
    mats=mat if isinstance(mat,list) else [mat]
    for x in mats:m.materials.append(x)
    for j,f in enumerate(m.polygons):
        f.use_smooth=smooth
        if indices:f.material_index=indices[j]
    uv=m.uv_layers.new(name='UVMap')
    if uvs:
        for poly,co in zip(m.polygons,uvs):
            for li,v in zip(poly.loop_indices,co):uv.data[li].uv=v
    return o

def unwrap(o):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.05,island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')

def rock_part(name,scale,seed,offset=(0,0,0),flatten=.15):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,radius=1);o=bpy.context.object;o.name=name
    for v in o.data.vertices:
        p=v.co.copy();n=noise.noise_vector(p*2.3+Vector((seed,seed*.7,-seed*.3)))
        fac=1+.13*n.x+.045*math.sin(p.z*18+seed)
        # Broad rock planes, rather than a smooth ellipsoid.
        q=p*fac;q.x=max(-.90,min(.94,q.x+.08*n.y));q.y=max(-.92,min(.91,q.y+.075*n.z));q.z=max(-.68,min(.83,q.z+.09*n.y))
        q.z+=.045*math.sin(q.x*4+q.y*3+seed)
        v.co=Vector((q.x*scale[0],q.y*scale[1],q.z*scale[2]))+Vector(offset)
    o.data.materials.append(STONE)
    # Retain controlled visible planes with slight bevels; avoid rounded pebbles.
    dec=o.modifiers.new('Authored broad fracture planes','DECIMATE');dec.ratio=.39
    bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=dec.name)
    for f in o.data.polygons:f.use_smooth=False
    bevel=o.modifiers.new('Narrow worn edges','BEVEL');bevel.width=.018;bevel.segments=1;bevel.affect='EDGES';bevel.angle_limit=.48
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    unwrap(o);return o

r=rock_part('Low boulder',(.90,.68,.52),2);assets['bh_rock_low_boulder']=[r]
r=rock_part('Upright crag',(.56,.50,.85),7)
# Slant the upper portion, giving the crag a distinct leaning broken profile.
for v in r.data.vertices:v.co.x+=max(0,v.co.z)*.22
assets['bh_rock_upright_crag']=[r]
r1=rock_part('Split shelf major',(.78,.60,.32),11,(-.28,.03,0));r2=rock_part('Split shelf minor',(.47,.48,.25),18,(.55,-.02,-.07));r2.rotation_euler[2]=.12
assets['bh_rock_split_shelf']=[r1,r2]

def tube(name,points,radii,sides=16,mat=BARK,cut_ends=True,ridge=.06):
    pts=[Vector(p) for p in points];vs=[];fs=[];uvs=[];mi=[];dist=[0]
    for i in range(1,len(pts)):dist.append(dist[-1]+(pts[i]-pts[i-1]).length)
    for i,(p,r) in enumerate(zip(pts,radii)):
        tangent=(pts[min(i+1,len(pts)-1)]-pts[max(0,i-1)]).normalized()
        ref=Vector((0,0,1)) if abs(tangent.z)<.95 else Vector((0,1,0))
        e1=tangent.cross(ref).normalized();e2=tangent.cross(e1).normalized()
        for j in range(sides):
            a=math.tau*j/sides
            rr=r*(1+ridge*math.sin(a*5+.4)+ridge*.6*math.cos(a*9-.8)+.025*math.sin(i*1.8+a*3))
            v=p+rr*(math.cos(a)*e1+math.sin(a)*e2)
            if not cut_ends and i in [0,len(pts)-1]:v+=tangent*(.020*math.sin(a*7+.3))*(r/.30)
            vs.append(tuple(v))
    for i in range(len(pts)-1):
        for j in range(sides):
            k=(j+1)%sides;fs.append((i*sides+j,i*sides+k,(i+1)*sides+k,(i+1)*sides+j));mi.append(0)
            uvs.append(((j/sides,dist[i]/1.7),((j+1)/sides,dist[i]/1.7),((j+1)/sides,dist[i+1]/1.7),(j/sides,dist[i+1]/1.7)))
    for i in [0,len(pts)-1]:
        ci=len(vs);vs.append(tuple(pts[i]));ring=range(sides)
        for j in ring:
            k=(j+1)%sides
            ids=(ci,i*sides+k,i*sides+j) if i==0 else (ci,i*sides+j,i*sides+k)
            fs.append(ids);mi.append(1 if cut_ends else 0)
            co=[(.5,.5)]
            for vi in ids[1:]:
                a=math.tau*(vi%sides)/sides;co.append((.5+.48*math.cos(a),.5+.48*math.sin(a)))
            uvs.append(co)
    o=mesh_object(name,vs,fs,[mat,WOOD] if cut_ends else mat,uvs,mi,smooth=True)
    # Flat end faces preserve cut-plane relief without side-normal averaging.
    for f in o.data.polygons:
        if f.material_index==1:f.use_smooth=False
    return o

assets['bh_log_fallen_trunk']=[
    tube('Fallen ridged trunk',[(-1.72,0,.35),(-1.40,.01,.32),(-.70,.06,.34),(0,.08,.39),(.8,.02,.35),(1.5,-.06,.33),(1.64,-.055,.32)],[.32,.34,.35,.335,.305,.28,.275],24),
    tube('Broken upward limb',[(-.52,.06,.38),(-.34,.13,.53),(-.12,.21,.76),(.10,.19,.89)],[.125,.115,.078,.067],14),
    tube('Lateral broken limb',[(.56,.02,.39),(.8,.28,.37),(1.02,.43,.29),(1.2,.51,.22)],[.10,.08,.055,.031],12)
]
assets['bh_root_branch_tangle']=[
    tube('Twisted fallen root',[(-1.04,.01,.12),(-.84,.07,.17),(-.42,.10,.30),(.03,.02,.39),(.42,-.12,.27),(.76,-.18,.16),(1.04,-.23,.13)],[.13,.16,.175,.15,.115,.07,.033],16),
    tube('Fork root left',[(-.55,.09,.25),(-.61,.31,.31),(-.38,.61,.22),(-.20,.79,.07)],[.10,.087,.057,.018],12),
    tube('Fork root right',[(.04,.02,.38),(.19,-.31,.45),(.40,-.61,.26),(.66,-.74,.09)],[.095,.079,.047,.017],12),
    tube('Raised broken fork',[(-.12,.055,.34),(-.24,.16,.57),(-.18,.27,.72)],[.09,.055,.035],12),
    tube('Small root spur',[(.48,-.15,.23),(.55,.10,.24),(.85,.26,.11)],[.057,.039,.012],10)
]

def leaflet(name,base,direction,length,width,roll,mat=LEAF):
    d=Vector(direction).normalized();side=Vector((-d.y,d.x,0)).normalized();up=Vector((0,0,1))
    # Closed tapered four-edge lanceolate mass, ridged and curled; no cards.
    outline=[(0,0),(.48,1), (1,0),(.43,-.85)]
    vs=[]
    for t,w in outline:
        p=base+d*(t*length)+side*(w*width)+up*(.045*math.sin(t*math.pi)*length+roll*w*.3)
        vs.append(tuple(p))
    center=base+d*(length*.46)+up*(.010+length*.075);vs.append(tuple(center));vs.append(tuple(center-up*.013))
    fs=[];uvs=[]
    for j in range(4):
        k=(j+1)%4;fs.append((4,j,k));fs.append((5,k,j));
        a=outline[j];b=outline[k];uvs += [[(.48,.52),(.5+a[1]*.45,a[0]),(.5+b[1]*.45,b[0])],[(.48,.52),(.5+b[1]*.45,b[0]),(.5+a[1]*.45,a[0])]]
    return mesh_object(name,vs,fs,mat,uvs,smooth=False)

def fern(name,count,size,seed):
    rng=random.Random(seed);objects=[]
    for f in range(count):
        a=math.tau*f/count+rng.uniform(-.15,.15);L=size*rng.uniform(.72,.92);H=size*rng.uniform(.60,.83)
        radial=Vector((math.cos(a),math.sin(a),0));lateral=Vector((-math.sin(a),math.cos(a),0));base=Vector((rng.uniform(-.035,.035),rng.uniform(-.035,.035),.05))
        def path(t):return base+radial*(L*t)+lateral*(math.sin(t*math.pi)*.05)+Vector((0,0,H*math.sin(t*math.pi*.87)))
        points=[path(j/6) for j in range(7)];rads=[size*(.010-.007*j/6) for j in range(7)]
        objects.append(tube(f'{name} rachis {f}',points,rads,4,LEAF,False,.0))
        pairs=11 if count>4 else 8
        for k in range(pairs):
            t=.13+.82*k/pairs;p=path(t)
            leaf_len=size*(.23*math.sin(math.pi*(t**.68))+.014)
            for sign in [-1,1]:
                d=(lateral*sign*.90+radial*.32+Vector((0,0,.09*(1-t)))).normalized()
                objects.append(leaflet(f'{name} pinna {f}.{k}.{sign}',p,d,leaf_len*(1+rng.uniform(-.12,.12)),leaf_len*.14,size*.01*rng.uniform(-1,1)))
        # Terminal pinna keeps each frond visibly tapered.
        objects.append(leaflet(f'{name} terminal {f}',path(.93),radial,.095*size,.012*size,0))
    return objects
assets['bh_fern_open_clump']=fern('Open fern',5,1.0,114)
assets['bh_fern_small_clump']=fern('Small fern',4,.66,337)

# Join each authored form, ground its lowest point at zero, and set an origin at
# horizontal bounds center. No collision is invented or exported as render mesh.
manifest=[];joined={}
for name,obs in assets.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in obs:o.select_set(True)
    bpy.context.view_layer.objects.active=obs[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    coords=[o.matrix_world@v.co for v in o.data.vertices];mn=Vector([min(v[j] for v in coords) for j in range(3)]);mx=Vector([max(v[j] for v in coords) for j in range(3)])
    offset=Vector(((mn.x+mx.x)/2,(mn.y+mx.y)/2,mn.z))
    for v in o.data.vertices:v.co=o.matrix_world@v.co-offset
    o.location=(0,0,0);o.rotation_euler=(0,0,0);o.scale=(1,1,1)
    # Recompute consistent outward normals for all closed components.
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.object.mode_set(mode='OBJECT')
    tri=o.modifiers.new('Runtime triangulation','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=0.000001);bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=0.000001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bmesh.ops.triangulate(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    o.data.update();bpy.context.view_layer.update()
    o['provenance']='Original deterministic Blender geometry, 2026-10-06';o['units']='meters';o['collision']='None supplied';o['runtime_validation']='Not tested in Horde1.7/native/mobile/RT'
    bpy.ops.export_scene.gltf(filepath=str(ROOT/'assets'/f'{name}.glb'),export_format='GLB',use_selection=True,export_yup=True,export_texcoords=True,export_normals=True,export_materials='EXPORT',export_animations=False)
    textures=set()
    for m in o.data.materials:
        if m and m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type=='TEX_IMAGE' and n.image:textures.add(Path(n.image.filepath).name)
    finite=all(math.isfinite(c) for v in o.data.vertices for c in v.co) and all(math.isfinite(c) for v in o.data.vertices for c in v.normal) and all(math.isfinite(c) for d in o.data.uv_layers.active.data for c in d.uv)
    manifest.append(dict(name=name,triangles=len(o.data.polygons),vertices=len(o.data.vertices),materials=len(o.data.materials),material_names=[m.name for m in o.data.materials],source_texture_count=len(textures),source_textures=sorted(textures),dimensions_blender_xyz_m=[round(v,4) for v in o.dimensions],ground_min_z=round(min(v.co.z for v in o.data.vertices),7),finite_positions_normals_uvs=finite))
    joined[name]=o

(ROOT/'validation'/'authoring-inventory.json').write_text(json.dumps(manifest,indent=2))
# Editable scene uses exploded layout; every export above retains origin grounding.
layout=[(-3.4,2.5,0),(-1.1,2.5,0),(1.6,2.5,0),(-2.1,-.2,0),(2.0,-.2,0),(-1.7,-2.4,0),(1.15,-2.4,0)]
for (name,o),loc in zip(joined.items(),layout):o.location=loc
for img in bpy.data.images:
    if img.filepath:img.filepath=bpy.path.relpath(img.filepath,start=str(ROOT/'source'))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'source'/'Horde-1.7-forest-dressing.blend'))
print('DRESSING_AUTHORING_COMPLETE',json.dumps(manifest))
