"""Bounded original facade refinement of the accepted v2 shell.
blender --background --python tools/refine-bellwether-shell.py -- ROOT
Never overwrites source, runtime or backup files. No new material/texture/light.
"""
from pathlib import Path
import hashlib,json,sys
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

root=Path(sys.argv[sys.argv.index('--')+1]).resolve()
source=root/'assets/models/world/source/bellwether-shell-v2'
out=root/'assets/models/world/source/bellwether-shell-v4'
runtime=root/'assets/models/world/runtime/bellwether-shell-v4'
if out.exists() or runtime.exists(): raise RuntimeError('Preserve all existing revisions')
master=source/'bellwether-shell-v2.blend'
receipt=json.loads((source/'authoring-receipt.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(master))
bpy.context.preferences.filepaths.save_version=0
materials=[bpy.data.materials['HordeTownStone'],bpy.data.materials['HordeTownRoofClay']]
objects=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.hide_get()]
assert len(objects)>0
# The preserved master includes hidden native terrain as authoring context.
terrain=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.hide_get()]
assert len(terrain)>0
verts,polys=[],[]
for obj in terrain:
    base=len(verts);verts.extend(obj.matrix_world @ v.co for v in obj.data.vertices)
    polys.extend([[base+i for i in poly.vertices] for poly in obj.data.polygons])
bvh=BVHTree.FromPolygons(verts,polys)
def support(x,z):
    hit,_,_,_=bvh.ray_cast(Vector((x,-z,2000)),Vector((0,0,-1)),4000)
    if hit is None:raise ValueError('Missing native ground beneath facade footing')
    return hit.z
new=[]
stepContacts=[]
def box(name,x,y,z,w,h,d,mat=0):
    pts=[(x-w/2,-(z-d/2),y),(x+w/2,-(z-d/2),y),(x+w/2,-(z+d/2),y),(x-w/2,-(z+d/2),y),
         (x-w/2,-(z-d/2),y+h),(x+w/2,-(z-d/2),y+h),(x+w/2,-(z+d/2),y+h),(x-w/2,-(z+d/2),y+h)]
    faces=[(0,1,2,3),(4,5,1,0),(5,6,2,1),(6,7,3,2),(7,4,0,3),(7,6,5,4)]
    data=bpy.data.meshes.new(name);data.from_pydata(pts,[],faces);data.materials.append(materials[mat]);data.update()
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj)
    uv=data.uv_layers.new(name='metre_projection')
    for poly in data.polygons:
        axis=max(range(3),key=lambda a:abs(poly.normal[a]));pair=((1,2),(0,2),(0,1))[axis]
        for loop in poly.loop_indices:
            p=data.vertices[data.loops[loop].vertex_index].co;uv.data[loop].uv=(p[pair[0]]*.5,p[pair[1]]*.5)
    objects.append(obj);new.append(name)
    return obj
witnesses=[]
for i,b in enumerate(receipt['buildings']):
    x,z,floor,w,d=b['engineX'],b['engineZ'],b['actualFloorY'],b['width'],b['depth']
    front=z-d/2;door=1.8 if i!=1 else 2.8;height=2.45 if i!=1 else 3.2
    # Shallow solid stone returns frame the already closed facade. No dark cards,
    # invisible interior, gameplay door, new light or collision invitation.
    for side in (-1,1):
        px=x+side*(door/2+.14)
        box(b['id']+'_door_post_'+str(side),px,floor-.03,front-.10,.28,height,.28)
        witnesses.append(dict(name=b['id']+'_door_post_'+str(side),x=px,y=floor+height*.5,
                              frontZ=front-.24,oldFacadeZ=front))
    box(b['id']+'_door_lintel',x,floor+height-.04,front-.10,door+.56,.30,.28)
    stepWidth=door+1
    surveyed=[support(x+u*stepWidth/2,front-.45+v*.45) for u in (-1,0,1) for v in (-1,0,1)]
    stepBottom=min(surveyed)-.10;stepTop=floor+.02
    if stepBottom>=stepTop:raise ValueError('Facade footing cannot have an inverted interval')
    box(b['id']+'_front_step',x,stepBottom,front-.45,stepWidth,stepTop-stepBottom,.90)
    stepContacts.append(dict(id=b['id'],bottomY=stepBottom,topY=stepTop,supportSamples=surveyed))
    if i in (0,2,3,5):
        box(b['id']+'_cold_closed_upper_shutter',x+w*.25,floor+height+.55,front-.035,.9,1.3,.07,1)
    if i==1:
        tx,tz=x-w*.29,z-d*.29
        for face in range(4):
            wy=floor+10.6
            if face<2:
                fz=tz+(-1 if face==0 else 1)*2.35
                for side in (-1,1):box('B02_bell_window_'+str(face)+'_post_'+str(side),tx+side*.63,wy,fz,.22,2.0,.24)
                box('B02_bell_window_'+str(face)+'_lintel',tx,wy+1.95,fz,1.48,.24,.24)
            else:
                fx=tx+(-1 if face==2 else 1)*2.35
                for side in (-1,1):box('B02_bell_window_'+str(face)+'_post_'+str(side),fx,wy,tz+side*.63,.24,2.0,.22)
                box('B02_bell_window_'+str(face)+'_lintel',fx,wy+1.95,tz,.24,.24,1.48)
assert len(new)==52
out.mkdir(parents=True);runtime.mkdir(parents=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'bellwether-shell-v4.blend'))
bpy.ops.object.select_all(action='DESELECT');copies=[]
for obj in objects:
    copy=obj.copy();copy.data=obj.data.copy();bpy.context.collection.objects.link(copy)
    copy.hide_set(False);copy.select_set(True);copies.append(copy)
bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join()
combined=bpy.context.object;combined.name='Original_Bellwether_closed_distant_shell'
path=runtime/'bellwether-shell-v4.glb'
bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,export_apply=True,
    export_texcoords=True,export_normals=True,export_tangents=True,export_animations=False,export_extras=True)
manifest=json.loads((root/'assets/models/world/runtime/bellwether-shell-v2/asset.manifest.json').read_text())
manifest['licenceStatus']='Original Horde-owned Blender geometry; public source/runtime distribution authorized by the chapter task. Shared texture-specific notices remain in ASSET_LICENSES.md.'
(runtime/'asset.manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8',newline='\n')
receipt.update(parentMasterSha256=hashlib.sha256(master.read_bytes()).hexdigest(),newSolidDetails=new,
    facadeRayWitnesses=witnesses,stepContactSurveys=stepContacts,expectedTriangles=966,
    runtimeSha256=hashlib.sha256(path.read_bytes()).hexdigest(),
    policy='Original bounded solid facade/tower relief; all nine footprints and foundations retained; two existing materials; no new light, texture or village gameplay')
(out/'authoring-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Exported grounded original v4 facade derivative; expected966 triangles; v2/master/backups preserved')
