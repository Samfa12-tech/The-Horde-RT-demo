"""Original Blender-authored A17/T03 source kit, deterministic seed 1703.
Run: blender --background --factory-startup --python source/build_props.py
No network/provider data. Produces candidate sources, not native runtime admission.
"""
import bpy, math, random, json, os
from mathutils import Vector
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
random.seed(1703)
os.makedirs(ROOT+'/models',exist_ok=True)
os.makedirs(ROOT+'/source',exist_ok=True)
def mat(name,c,rough):
 m=bpy.data.materials.new(name);m.use_nodes=True;m.use_backface_culling=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=0
 m.diffuse_color=(*c,1);return m
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
stone=mat('Horde_existing_masonry_binding_required',(0.25,.27,.245),.88)
clay=mat('Original_unglazed_earthenware',(.25,.115,.065),.84)
claybreak=mat('Original_exposed_clay_fracture',(.34,.18,.115),.91)
wax=mat('Original_aged_beeswax',(.57,.43,.23),.62)
wickmat=mat('Original_charred_wick',(.025,.019,.013),.95)
manifest=[]
def mesh(name,vs,fs,ma,smooth=False):
 me=bpy.data.meshes.new(name);me.from_pydata(vs,[],fs);me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);ob.data.materials.append(ma)
 for p in me.polygons:p.use_smooth=smooth
 return ob

def bevel(ob,width=.008,segments=2):
 mo=ob.modifiers.new('Rounded worn edges','BEVEL');mo.width=width;mo.segments=segments
 mo=ob.modifiers.new('Weighted broad-surface normals','WEIGHTED_NORMAL');mo.keep_sharp=True;mo.weight=30

def uv(ob):
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob;bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.remove_doubles(threshold=0.000001);bpy.ops.mesh.dissolve_degenerate(threshold=0.000001);bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.uv.smart_project(angle_limit=math.radians(60),island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')

def lathe(name,profile,n,ma,jagged=False,sector=None):
 # Profile traverses from underside outer foot up outer wall, over lip,
 # down inner wall and onto the interior floor; closed cyclic section.
 steps=n if sector is None else n+1;vs=[]
 for j,(r,z) in enumerate(profile):
  for i in range(steps):
   a=2*math.pi*i/n if sector is None else sector[0]+(sector[1]-sector[0])*i/n
   dr=1+.012*math.sin(5*a+.7)+.008*math.cos(9*a)
   zz=z
   if jagged and j in (len(profile)//2-1,len(profile)//2):zz+=.038*math.sin(5*a+.2)+.017*math.cos(9*a)
   vs.append((r*dr*math.cos(a),r*dr*math.sin(a),zz))
 fs=[]
 for j in range(len(profile)):
  k=(j+1)%len(profile)
  for i in range(n):
   ni=(i+1)%steps;fs.append((j*steps+i,j*steps+ni,k*steps+ni,k*steps+i))
 if sector is not None:
  fs.append(tuple(j*steps for j in reversed(range(len(profile)))))
  fs.append(tuple(j*steps+n for j in range(len(profile))))
 ob=mesh(name,vs,fs,ma,True)
 return ob

def export(name,objects,role,collision):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:
  uv(o);o.select_set(False)
 for o in objects:
  o.select_set(True);o.modifiers.new('Export triangles and tangent basis','TRIANGULATE')
 bpy.context.view_layer.objects.active=objects[0]
 # Static authored geometry, meter-scale source and base-centered pivot.
 for o in objects:
  o['asset_id']=name;o['plan']='A17/T03';o['units']='meters';o['collision_guidance']=collision
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
 bpy.ops.wm.save_as_mainfile(filepath=ROOT+'/source/'+name+'.blend',compress=True)
 bpy.ops.export_scene.gltf(filepath=ROOT+'/models/'+name+'.glb',export_format='GLB',use_selection=True,export_apply=True,export_texcoords=True,export_normals=True,export_tangents=True,export_materials='EXPORT',export_yup=True,export_extras=True)
 manifest.append({'id':name,'role':role,'collision':collision,'source':'source/'+name+'.blend','model':'models/'+name+'.glb','plan':'A17/T03','pivot':'floor/base-centered at origin; GLB Y-up, Blender Z-up','materials':[m.name for o in objects for m in o.data.materials]})
 for o in objects:bpy.data.objects.remove(o,do_unlink=True)

# Tapered lid with softened perimeter, raised central panel, damaged corners.
poly=[(-.32,-.96),(.31,-.96),(.37,-.84),(.39,-.40),(.345,.86),(.27,.98),(-.27,.98),(-.355,.85),(-.395,-.40),(-.37,-.83)]
vs=[(x,y,z) for z in (0,.145) for x,y in poly];n=len(poly)
fs=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
lid=mesh('lid_body',vs,fs,stone);bevel(lid,.018,3)
# Relief mass adds legible old funerary craft without inscriptions or symbols.
inner=[(x*.80,y*.86) for x,y in poly];vs=[(x,y,z) for z in (.14,.185) for x,y in inner]
fs=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
panel=mesh('lid_raised_panel',vs,fs,stone);bevel(panel,.012,2)
export('t03_displaced_lid',[lid,panel],'Small human-scale stone lid; displace onto credible support, not a new Keeper stone. No inscription.','Single oriented box or convex hull fitted to visible lid; static. Never occupy walking/combat lanes.')

# One low footed open offering bowl: real inside wall and underside.
bowl=lathe('offering_bowl',[(0,0),(.105,0),(.11,.025),(.16,.045),(.225,.09),(.235,.11),(.232,.118),(.215,.114),(.203,.092),(.146,.057),(.09,.04),(0,.039)],48,clay)
# Profile includes rounded lip, no extra bevel topology.
export('t03_offering_bowl',[bowl],'Empty unglazed offering bowl, 0.47 m diameter. No loot or interaction semantics.','Nonblocking decorative prop by default; simple convex proxy only if placement demands physical blocking.')

# Lower urn: open broken top, thick wall and actual base. Original parent shape
# remains reproducible here. Fracture surface has warmer exposed clay colour.
prof=[(0,0),(.10,0),(.135,.055),(.175,.13),(.19,.225),(.178,.295),(.16,.302),(.171,.223),(.156,.13),(.116,.058),(.082,.029),(0,.029)]
urn=lathe('broken_urn_lower',prof,40,clay,True);urn.data.materials.append(claybreak)
for p in urn.data.polygons:
 if p.index//40==5:p.material_index=1;p.use_smooth=False
bevel(urn,.002,1)
export('t03_urn_broken_base',[urn],'Broken lower urn with jagged open rim and readable thick terracotta wall.','Nonblocking detail; no triangle-soup collision. If large instances block passage, use one low convex hull and test clearance.')

# Curved upper wall/rim shard, authored solid shell and honest fractured edges.
prof=[(.176,0),(.172,.08),(.14,.155),(.102,.205),(.095,.22),(.112,.227),(.112,.243),(.09,.249),(.081,.224),(.087,.199),(.124,.15),(.155,.078)]
shard=lathe('urn_curved_rim_shard',prof,20,clay,False,(-.95,1.25));shard.data.materials.append(claybreak)
for p in shard.data.polygons:
 if p.index>=len(prof)*20 or p.index//20==11:p.material_index=1;p.use_smooth=False
# Lay the shard onto its convex side; bake origin at its true support plane.
shard.rotation_euler[1]=math.radians(77);bpy.context.view_layer.objects.active=shard;shard.select_set(True);bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);shard.select_set(False)
minz=min(v.co.z for v in shard.data.vertices);xs=[v.co.x for v in shard.data.vertices];ys=[v.co.y for v in shard.data.vertices];cx=(min(xs)+max(xs))/2;cy=(min(ys)+max(ys))/2
for v in shard.data.vertices:v.co.x-=cx;v.co.y-=cy;v.co.z-=minz
bevel(shard,.0015,1)
export('t03_urn_rim_shard',[shard],'Resting curved urn-rim shard, thick and open-ended; compose sparsely beside lower urn.','Nonblocking fragment. Rest on support plane; avoid floating fragments or repeated identical scatter.')

for k,(rad,h) in enumerate([(.046,.082),(.039,.134),(.055,.043)],1):
 n=40;vs=[]
 # Integral droop contours and uneven melted basin, no added disconnected wax drops.
 rings=[(1,0),(1,.12),(1,.63),(1,.86),(.94,1),(.70,.96),(.35,.79),(.04,.76)]
 for j,(rr,hh) in enumerate(rings):
  for i in range(n):
   a=2*math.pi*i/n
   melt=.045*math.sin(3*a+k)+.029*math.sin(7*a)
   drip=max(0,math.cos(4*a+k))**12
   r=rad*rr*(1+.025*math.sin(5*a)+(.065*drip if j in (1,2,3) else 0))
   z=h*hh+(h*melt if j>=3 else 0)
   vs.append((r*math.cos(a),r*math.sin(a),max(0,z)))
 fs=[tuple(reversed(range(n)))]
 for j in range(len(rings)-1):
  fs.extend((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i) for i in range(n))
 fs.append(tuple((len(rings)-1)*n+i for i in range(n)))
 ob=mesh('melted_wax_stub',vs,fs,wax,True)
 bpy.ops.mesh.primitive_cylinder_add(vertices=8,radius=.0025,depth=.015,location=(0,0,h*.79+.006));wick=bpy.context.object;wick.name='cold_charred_wick';wick.rotation_euler=(.16,.08,0);wick.data.materials.append(wickmat)
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 export('t03_candle_stub_'+str(k),[ob,wick],'Cold extinguished beeswax candle stub variation '+str(k)+'. No flame, emission, light or gameplay state.','Nonblocking tabletop/niche detail. Disable at distance if subpixel; no individual collider or light.')
open(ROOT+'/asset-manifest.json','w').write(json.dumps({'name':'Horde 1.7 A17 T03 funerary utility source kit','version':'0.1.0','stage':'authored candidate; native runtime admission pending','authorship':'Original deterministic Blender geometry authored for the user; no third-party model or texture bytes','license_note':'No third-party asset restrictions introduced. Original generated project source; no exclusive copyright or legal warranty claimed.','provider_spend':0,'textures':0,'assets':manifest},indent=2))
print('BUILT',len(manifest),'assets')
