"""Original Horde 1.7 trees. Deterministic Blender 4.3 source, no external assets.
Run: blender -b --python build_horde_trees.py -- --output /path/to/output
All mesh parts are closed opaque volumes. Export meters, Y-up glTF, origin ground.
"""
import bpy, math, random, json, os, sys, argparse
import numpy as np
from mathutils import Vector
from pathlib import Path

p=argparse.ArgumentParser(); p.add_argument('--output',default=str(Path(__file__).resolve().parent.parent)); p.add_argument('--only',default=''); p.add_argument('--no-render',action='store_true')
a=p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=Path(a.output)
for d in ['source','textures','runtime','previews','qa']: (OUT/d).mkdir(parents=True,exist_ok=True)
TAU=2*math.pi

class Mesh:
    def __init__(self,name,material): self.name=name; self.material=material; self.v=[]; self.f=[]; self.uv=[]
    def face(self,indices,uv=None): self.f.append(indices); self.uv.append(uv or [(0,0)]*len(indices))
    def tube(self,points,radii,sides=7,ridges=.04):
        pts=[Vector(x) for x in points]; base=len(self.v); dist=0
        lens=[0]
        for x,y in zip(pts,pts[1:]): lens.append(lens[-1]+(y-x).length)
        # A consistent axis frame avoids artificial branch corkscrews.
        direction=(pts[-1]-pts[0]).normalized(); ref=Vector((0,0,1)) if abs(direction.z)<.94 else Vector((1,0,0))
        for i,(c,r) in enumerate(zip(pts,radii)):
            tangent=(pts[min(i+1,len(pts)-1)]-pts[max(i-1,0)]).normalized()
            ax=tangent.cross(ref).normalized(); ay=tangent.cross(ax).normalized()
            for j in range(sides):
                th=TAU*j/sides; rr=r*(1+ridges*math.sin(th*5+i*.33)+ridges*.45*math.sin(th*9-i*.21))
                self.v.append(tuple(c+rr*(math.cos(th)*ax+math.sin(th)*ay)))
        for i in range(len(pts)-1):
            for j in range(sides):
                j2=(j+1)%sides
                self.face((base+i*sides+j,base+i*sides+j2,base+(i+1)*sides+j2,base+(i+1)*sides+j),[(j/sides,lens[i]/1.7),((j+1)/sides,lens[i]/1.7),((j+1)/sides,lens[i+1]/1.7),(j/sides,lens[i+1]/1.7)])
        self.face(tuple(base+j for j in reversed(range(sides))),[(.5+.46*math.cos(TAU*j/sides),.5+.46*math.sin(TAU*j/sides)) for j in reversed(range(sides))])
        self.face(tuple(base+(len(pts)-1)*sides+j for j in range(sides)),[(.5+.46*math.cos(TAU*j/sides),.5+.46*math.sin(TAU*j/sides)) for j in range(sides)])
    def leaf(self,center,axis,width,length,rotation,rng,kind='leaf'):
        center=Vector(center); axis=Vector(axis).normalized(); ref=Vector((0,0,1)) if abs(axis.z)<.95 else Vector((1,0,0)); side=axis.cross(ref).normalized(); up=axis.cross(side).normalized()
        side,up=side*math.cos(rotation)+up*math.sin(rotation),-side*math.sin(rotation)+up*math.cos(rotation)
        # Hexagonal, closed lens with a thick midrib. No alpha and no two-sided plane.
        if kind=='leaf': profile=[(0,0),(.30,-.86),(.66,-1),(1,0),(.66,1),(.30,.86)]
        else: profile=[(0,0),(.25,-.75),(.64,-1),(1,0),(.66,1),(.28,.75)]
        base=len(self.v); thick=width*(.16 if kind=='leaf' else .32)
        for l,w in profile: self.v.append(tuple(center+axis*(l-.5)*length+side*w*width+up*(math.sin(l*math.pi)*thick*.2)))
        self.v.extend([tuple(center+up*thick),tuple(center-up*thick*.65)])
        for i in range(6):
            j=(i+1)%6; uvi=(.5+profile[i][1]*.46,profile[i][0]); uvj=(.5+profile[j][1]*.46,profile[j][0])
            self.face((base+i,base+j,base+6),[uvi,uvj,(.5,.5)])
            self.face((base+j,base+i,base+7),[uvj,uvi,(.5,.5)])
    def object(self):
        me=bpy.data.meshes.new(self.name); me.from_pydata(self.v,[],self.f); me.update(); ob=bpy.data.objects.new(self.name,me); bpy.context.collection.objects.link(ob); me.materials.append(self.material)
        uv=me.uv_layers.new(name='UVMap')
        for poly, coords in zip(me.polygons,self.uv):
            poly.use_smooth=True
            for li,co in zip(poly.loop_indices,coords): uv.data[li].uv=co
        return ob

def image_from_array(name,arr,colorspace='sRGB'):
    size=arr.shape[0]; img=bpy.data.images.new(name,width=size,height=size,alpha=False)
    rgba=np.ones((size,size,4),dtype=np.float32); rgba[:,:,:3]=arr if arr.ndim==3 else arr[:,:,None]
    img.colorspace_settings.name=colorspace; img.pixels.foreach_set(rgba.ravel()); img.update()
    img.filepath_raw=str(OUT/'textures'/f'{name}.png'); img.file_format='PNG'; img.save(); img.pack(); return img

def maps():
    n=512; y,x=np.mgrid[0:n,0:n].astype(float)/n; rng=np.random.default_rng(74107)
    # Multi-scale, vertically stretched noise. All textures are original math.
    noise=np.zeros((n,n))
    for i in range(32):
        fx=rng.uniform(3,44); fy=rng.uniform(.4,10); ph=rng.uniform(0,TAU); noise += np.sin(TAU*(x*fx+y*fy)+ph)/(1+fx*.15)
    noise=noise/6
    # Irregular elongated bark plates with broken fissures, rather than periodic stripes.
    warp=x+.012*np.sin(y*TAU*3+x*TAU*3)+.006*np.sin(y*TAU*9+x*TAU)
    wy=y+.035*np.sin(x*TAU*5+y*TAU*2)+noise*.035
    d1=np.full((n,n),1e6); d2=d1.copy(); cellshade=np.zeros((n,n))
    for col in range(22):
        for row in range(5):
            cx=(col+rng.uniform(.05,.95))/22; cy=(row+rng.uniform(.1,.9))/5
            dx=np.abs(warp-cx); dx=np.minimum(dx,1-dx); dy=np.abs(wy-cy); dy=np.minimum(dy,1-dy)
            dd=(dx*22)**2+(dy*5)**2
            nearer=dd<d1; d2=np.where(nearer,d1,np.minimum(d2,dd)); d1=np.minimum(d1,dd); cellshade=np.where(nearer,rng.uniform(-.09,.09),cellshade)
    gap=np.sqrt(d2)-np.sqrt(d1); fissure=np.exp(-np.maximum(gap,0)*25)
    fine=np.maximum(0,np.sin(warp*TAU*61+y*TAU*3)-.68)*.14
    h=np.clip(.57-fissure*.34-fine+noise*.17+cellshade*.7,0,1)
    patches=np.clip(np.sin(x*TAU*4+y*TAU*3)+np.sin(x*TAU*9-y*TAU*2)-1.3,0,1)
    intensity=np.clip(.61-fissure*.23+noise*.22+patches*.08+cellshade,0,1)
    base=np.stack([intensity*.43+patches*.015,intensity*.45+patches*.04,intensity*.40],axis=2)
    bark=image_from_array('horde-bark-original-basecolor-512',base)
    rough=image_from_array('horde-bark-original-roughness-512',np.clip(.88+noise*.12-fissure*.07,.65,1),'Non-Color')
    dy,dx=np.gradient(h); norm=np.stack([-dx*17,-dy*10,np.ones_like(h)],axis=2); norm/=np.linalg.norm(norm,axis=2)[:,:,None]
    normal=image_from_array('horde-bark-original-normal-512',norm*.5+.5,'Non-Color')
    vein=np.exp(-((x-.5)/.013)**2)*.11
    veins=np.maximum(0,1-np.abs(np.sin((y*14+abs(x-.5)*9)*math.pi))/.16)*np.exp(-abs(x-.5)*2)*.045
    leaflight=.86+.13*np.sin(y*math.pi)+noise*.10+vein+veins
    leaf=image_from_array('horde-alder-original-basecolor-512',np.stack([leaflight*.16,leaflight*.225,leaflight*.072],axis=2))
    needlelight=.78+.18*np.sin(y*math.pi)+noise*.10+.08*np.cos(x*TAU*11)
    needle=image_from_array('horde-pine-original-basecolor-512',np.stack([needlelight*.105,needlelight*.171,needlelight*.115],axis=2))
    moss=image_from_array('horde-moss-original-basecolor-512',np.stack([leaflight*.215,leaflight*.25,leaflight*.095],axis=2))
    return bark,rough,normal,leaf,needle,moss

def material(name,image,roughness=.9,normal=None,rough=None):
    mat=bpy.data.materials.new(name); mat.use_nodes=True; mat.use_backface_culling=True; mat.diffuse_color=(.2,.25,.12,1)
    nodes=mat.node_tree.nodes; links=mat.node_tree.links; bs=nodes.get('Principled BSDF'); bs.inputs['Roughness'].default_value=roughness; bs.inputs['Metallic'].default_value=0; bs.inputs['IOR'].default_value=1.4
    tex=nodes.new('ShaderNodeTexImage'); tex.image=image; links.new(tex.outputs['Color'],bs.inputs['Base Color'])
    if rough:
        tx=nodes.new('ShaderNodeTexImage'); tx.image=rough; links.new(tx.outputs['Color'],bs.inputs['Roughness'])
    if normal:
        tx=nodes.new('ShaderNodeTexImage'); tx.image=normal; nm=nodes.new('ShaderNodeNormalMap'); nm.inputs['Strength'].default_value=.70; links.new(tx.outputs['Color'],nm.inputs['Color']); links.new(nm.outputs['Normal'],bs.inputs['Normal'])
    return mat

bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
bark_img,rough_img,normal_img,leaf_img,needle_img,moss_img=maps()
BARK=material('Original charcoal-gray ridged bark',bark_img,normal=normal_img,rough=rough_img)
LEAF=material('Original opaque alder leaves',leaf_img,.84)
NEEDLE=material('Original opaque pine needle sprays',needle_img,.9)
MOSS=material('Original opaque hanging woodland moss',moss_img,.98)

def branch(mesh,start,end,radius,rng,bend=.14,sides=7):
    s,e=Vector(start),Vector(end); d=e-s; points=[]
    side=Vector((-d.y,d.x,0)).normalized() if d.xy.length>.0001 else Vector((1,0,0))
    for i in range(5):
        t=i/4; co=s+d*t+side*(math.sin(t*math.pi)*bend+math.sin(t*TAU)*bend*.35)+Vector((0,0,math.sin(t*math.pi)*bend*.65)); points.append(tuple(co))
    mesh.tube(points,[radius,radius*.79,radius*.56,radius*.31,.009],sides,.065)
    return [Vector(p) for p in points]

def roots(wood,rng,radius=.35,spread=1.05):
    for i in range(6):
        th=i*TAU/6+rng.uniform(-.18,.18); dr=Vector((math.cos(th),math.sin(th),0)); start=dr*radius*.25+Vector((0,0,.50+rng.random()*.2)); end=dr*(spread*rng.uniform(.8,1.15)); mid=dr*spread*.50+Vector((0,0,.12))
        wood.tube([start,start*.55+mid*.45,mid,end*.94+Vector((0,0,.03)),end+Vector((0,0,.007))],[radius*.45,radius*.40,radius*.21,.035,.008],8,.08)

def moss_drape(moss,anchor,rng,length=.7):
    anchor=Vector(anchor)
    for j in range(2):
        q=anchor+Vector((rng.uniform(-.06,.06),rng.uniform(-.06,.06),0)); le=length*rng.uniform(.6,1.2)
        pts=[q,q+Vector((.04,.01,-le*.25)),q+Vector((-.025,.03,-le*.53)),q+Vector((.04,-.01,-le*.8)),q+Vector((.055,.015,-le))]
        moss.tube(pts,[.025,.021,.017,.011,.002],4,.05)

def alder():
    rng=random.Random(7319); wood=Mesh('Alder | closed trunk roots branches',BARK); foliage=Mesh('Alder | individually closed opaque leaves',LEAF); moss=Mesh('Alder | closed moss tendrils',MOSS)
    points=[(0,0,.04),(.01,.01,.35),(.015,.02,1),(.09,0,1.9),(.16,.04,2.8),(.07,.1,3.6),(.12,.04,4.5),(.23,.0,5.4),(.35,.09,6.2),(.43,.12,6.9),(.34,.12,7.5),(.40,.08,7.85)]
    wood.tube(points,[.37,.31,.255,.228,.202,.178,.151,.121,.09,.062,.032,.006],13,.075); roots(wood,rng,.38,1.1)
    # Upright narrow asymmetric crown, with clear forks and a little lower damage.
    branches=[((.13,.02,2.6),(-.58,.10,3.7),.105),((.10,.08,3.5),(1.05,.30,5.9),.127),((.12,.04,4.2),(-1.22,-.16,6.5),.115),((.16,.04,4.8),(.48,-1.11,6.65),.098),((.26,0,5.2),(-.59,.87,7.16),.091),((.36,.09,6),(1.11,.49,7.45),.072),((.4,.12,6.7),(-.56,-.52,7.75),.058)]
    sprigs=[]
    for bi,(s,e,r) in enumerate(branches):
        pts=branch(wood,s,e,r,rng,bend=.16*(-1 if bi%2 else 1),sides=8 if r>.10 else 7)
        if bi==0: # A partly snapped lower branch: open negative space, no crown ball.
            branch(wood,pts[3],pts[3]+Vector((-.21,-.15,.35)),.035,rng,sides=5); continue
        d=(Vector(e)-Vector(s)).normalized()
        for k in range(3):
            t=.47+k*.19; st=Vector(s).lerp(Vector(e),t); th=bi*2.23+k*2.3; direction=Vector((math.cos(th)*.7,math.sin(th)*.7,.55)).normalized(); en=st+direction*rng.uniform(.55,.9)
            bp=branch(wood,st,en,r*(1-t)*.55,rng,bend=rng.uniform(-.08,.08),sides=5)
            sprigs.append((st,en))
            # A terminal twig and two side sprays each carry actual 3D leaves.
            for j in range(2):
                ss=st.lerp(en,.52+j*.29); td=(direction+Vector((math.cos(th+j*2)*.38,math.sin(th+j*2)*.38,.25))).normalized(); ee=ss+td*rng.uniform(.36,.58)
                branch(wood,ss,ee,.019,rng,bend=.03,sides=4); sprigs.append((ss,ee))
        sprigs.append((pts[-2],Vector(e)+Vector((0,0,.3))))
        if bi in (1,3,4): moss_drape(moss,pts[2],rng,.6)
    # Leaf blades, closed lenses with a real midrib, arranged along open twiglets.
    for st,en in sprigs:
        direction=(en-st).normalized(); perpendicular=Vector((-direction.y,direction.x,.25)).normalized()
        for k in range(5):
            t=.28+k*.145; anchor=st.lerp(en,t); sign=1 if k%2 else -1
            axis=(direction*.5+perpendicular*sign*.95+Vector((0,0,.08+rng.uniform(-.2,.25)))).normalized(); size=rng.uniform(.21,.31)
            foliage.leaf(anchor+axis*size*.28,axis,size*.30,size,rng.uniform(-.55,.55),rng)
    # A few upper twiglets make the leader read as a tree at gameplay distance.
    for i in range(5):
        st=Vector(points[9]).lerp(Vector(points[-1]),i/5); th=i*2.4; en=st+Vector((math.cos(th)*.45,math.sin(th)*.45,.32))
        branch(wood,st,en,.025,rng,bend=.04,sides=5)
        for k in range(6):
            anchor=st.lerp(en,.30+k*.13); axis=Vector((math.cos(th+k*2),math.sin(th+k*2),.3)).normalized(); foliage.leaf(anchor,axis,.071,.265,rng.uniform(-.5,.5),rng)
    return [wood.object(),foliage.object(),moss.object()]

def pine():
    rng=random.Random(4126); wood=Mesh('Pine | closed trunk roots branches',BARK); foliage=Mesh('Pine | closed needle-spray volumes',NEEDLE); moss=Mesh('Pine | closed moss tendrils',MOSS)
    points=[(0,0,.035),(.01,.01,.4),(.035,.02,1.2),(.03,.09,2.3),(-.05,.18,3.4),(.03,.20,4.6),(.15,.24,5.5),(.3,.18,6.45),(.18,.11,7.25),(.32,.06,7.95),(.45,.04,8.42),(.49,.02,8.68)]
    wood.tube(points,[.33,.28,.231,.215,.185,.158,.131,.112,.09,.060,.025,.003],13,.08); roots(wood,rng,.33,1.0)
    # Uneven bough tiers; trunk windows and asymmetric silhouette avoid a toy cone.
    tiers=[(3.6,2.7,1.35,.078),(4.35,.1,1.55,.081),(4.75,3.9,1.7,.088),(5.35,1.8,1.28,.075),(5.85,5.3,1.48,.079),(6.25,3.15,1.08,.059),(6.65,.45,1.30,.062),(7.1,2.3,1.05,.052),(7.55,4.3,.86,.045),(8.02,.4,.64,.035)]
    for bi,(z,th,length,r) in enumerate(tiers):
        base=Vector((.02+(z/8)*.2,.16,z)); dr=Vector((math.cos(th),math.sin(th),0)); end=base+dr*length+Vector((0,0,.2+rng.random()*.18))
        # Characteristic bough droop followed by raised terminal needles.
        pts=[base,base+dr*length*.3+Vector((0,0,-.18)),base+dr*length*.65+Vector((0,0,-.14)),base+dr*length*.9+Vector((0,0,.08)),end]
        wood.tube(pts,[r,r*.77,r*.50,r*.25,.006],7,.06)
        if bi in (1,4,6): moss_drape(moss,pts[2],rng,.5)
        for k in range(4):
            st=pts[1].lerp(end,.22+k*.24); side=1 if k%2 else -1; sd=(dr*.45+Vector((-dr.y,dr.x,0))*side*.75+Vector((0,0,.24))).normalized(); tip=st+sd*rng.uniform(.43,.7)
            branch(wood,st,tip,r*.30,rng,bend=.04,sides=5)
            # Compact needles are closed tapered lens forms distributed in real 3D.
            for q in range(7):
                t=.28+q*.105; origin=st.lerp(tip,t); angle=q*2.399+bi; sideways=Vector((math.cos(angle),math.sin(angle),rng.uniform(-.16,.5)))
                axis=(sd*.45+sideways*.75+Vector((0,0,.25))).normalized(); lengthN=rng.uniform(.23,.40)
                foliage.leaf(origin+axis*.07,axis,rng.uniform(.046,.068),lengthN,angle,rng,'needle')
        for q in range(7):
            angle=q*2.399; axis=(dr*.4+Vector((math.cos(angle)*.5,math.sin(angle)*.5,.35))).normalized(); foliage.leaf(end+axis*.08,axis,.054,.32,angle,rng,'needle')
    for q in range(16):
        th=q*2.399; z=8.05+q*.035; axis=Vector((math.cos(th)*.5,math.sin(th)*.5,.6)).normalized(); foliage.leaf((.4+math.cos(th)*.11,.04+math.sin(th)*.11,z),axis,.047,.29,th,rng,'needle')
    # Two bare lower stubs are useful readable forest age, not another dead snag.
    branch(wood,(.01,.15,2.0),(-.50,-.21,2.23),.07,rng,bend=-.1,sides=6)
    branch(wood,(0,.13,2.8),(.55,.65,2.99),.066,rng,bend=.06,sides=6)
    return [wood.object(),foliage.object(),moss.object()]

def aim(obj,point): obj.rotation_euler=(Vector(point)-obj.location).to_track_quat('-Z','Y').to_euler()
def setup_scene():
    sc=bpy.context.scene; sc.unit_settings.system='METRIC'; sc.unit_settings.scale_length=1
    sc.render.engine='CYCLES'; sc.cycles.samples=48; sc.cycles.use_denoising=False
    sc.render.resolution_x=1100; sc.render.resolution_y=1300; sc.render.resolution_percentage=100
    sc.world.color=(.13,.13,.13); sc.world.use_nodes=True; bg=sc.world.node_tree.nodes.get('Background'); bg.inputs['Color'].default_value=(.19,.23,.27,1); bg.inputs['Strength'].default_value=.45
    sc.view_settings.view_transform='AgX'; sc.render.image_settings.file_format='PNG'
    bpy.ops.object.camera_add(location=(12,-19,12)); cam=bpy.context.object; cam.name='QA camera'; cam.data.type='ORTHO'; cam.data.ortho_scale=10.4; aim(cam,(0,0,4.3)); sc.camera=cam
    for name,loc,energy,size,color in [('Large overcast key',(-5,-7,12),1450,8,(.85,.91,1)),('Soft forest fill',(7,-1,8),700,7,(.58,.71,.82)),('Warm rim',(-3,5,10),1700,6,(1,.9,.73))]:
        bpy.ops.object.light_add(type='AREA',location=loc); light=bpy.context.object; light.name=name; light.data.energy=energy; light.data.shape='DISK'; light.data.size=size; light.data.color=color; aim(light,(0,0,4))
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.014)); floor=bpy.context.object; floor.name='QA-only ground (not exported)'; mat=bpy.data.materials.new('QA neutral ground'); mat.diffuse_color=(.052,.064,.067,1); mat.use_nodes=True; mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.052,.064,.067,1); mat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.96; floor.data.materials.append(mat)
    return cam

cam=setup_scene(); records=[]
for slug,builder in [('horde-upright-alder-v1',alder),('horde-irregular-pine-v1',pine)]:
    if a.only and a.only not in slug: continue
    objects=builder()
    for ob in objects: ob['authorship']='Original deterministic Blender source; no provider or third-party geometry'; ob['runtime_geometry']='Closed opaque volume; no cards or billboards'; ob['asset_version']='1.0.0'
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects: ob.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    # Stats before export and before any QA stage objects can enter selection.
    verts=[]; tris=0; degenerate=0; nonfinite=0; mesh_counts=[]
    for ob in objects:
        me=ob.data; me.calc_loop_triangles(); tris+=len(me.loop_triangles)
        verts.extend([ob.matrix_world@v.co for v in me.vertices]); mesh_counts.append({'name':ob.name,'vertices':len(me.vertices),'triangles':len(me.loop_triangles)})
        for t in me.loop_triangles:
            aa,bb,cc=[me.vertices[i].co for i in t.vertices]
            if (bb-aa).cross(cc-aa).length<1e-9: degenerate+=1
        for v in me.vertices:
            if not all(math.isfinite(c) for c in v.co): nonfinite+=1
    bounds={'min':[min(v[i] for v in verts) for i in range(3)],'max':[max(v[i] for v in verts) for i in range(3)]}
    stats={'asset':slug,'units':'meters','origin':[0,0,0],'bounds_blender_xyz':bounds,'dimensions_m':[bounds['max'][i]-bounds['min'][i] for i in range(3)],'triangles':tris,'meshes':mesh_counts,'materials':len({m.name for ob in objects for m in ob.data.materials}),'texture_resolution':[512,512],'opaque_only':True,'closed_leaf_geometry':True,'alpha_cards':False,'degenerate_triangles':degenerate,'nonfinite_vertices':nonfinite,'engine_import_tested':False,'canonical_game_dev_validation':'CLI not installed; not run','source_seed':7319 if 'alder' in slug else 4126}
    bpy.ops.export_scene.gltf(filepath=str(OUT/'runtime'/f'{slug}.glb'),use_selection=True,export_format='GLB',export_yup=True,export_apply=True,export_texcoords=True,export_normals=True,export_materials='EXPORT',export_extras=True)
    cam.location=(11,-20,10.7); cam.data.ortho_scale=10.3; aim(cam,(0,0,4.15 if 'alder' in slug else 4.4))
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'source'/f'{slug}.blend'))
    if not a.no_render:
        for view,loc in [('front',(11,-20,10.7)),('back',(-14,17,10.2))]:
            cam.location=loc; cam.data.ortho_scale=10.3; aim(cam,(0,0,4.15 if 'alder' in slug else 4.4)); bpy.context.scene.render.filepath=str(OUT/'previews'/f'{slug}-{view}.png'); bpy.ops.render.render(write_still=True)
        cam.location=(4.1,-6.1,3.4); cam.data.ortho_scale=3.4; aim(cam,(0,0,1.25)); bpy.context.scene.render.resolution_x=1000; bpy.context.scene.render.resolution_y=1000; bpy.context.scene.render.filepath=str(OUT/'previews'/f'{slug}-bark-detail.png'); bpy.ops.render.render(write_still=True)
        bpy.context.scene.render.resolution_x=1100; bpy.context.scene.render.resolution_y=1300
    (OUT/'qa'/f'{slug}-stats.json').write_text(json.dumps(stats,indent=2)); records.append(stats)
    for ob in objects: bpy.data.objects.remove(ob,do_unlink=True)
(OUT/'qa'/'mesh-summary.json').write_text(json.dumps(records,indent=2))
print('HORDE_TREE_OUTPUT',str(OUT)); print(json.dumps(records,indent=2))
