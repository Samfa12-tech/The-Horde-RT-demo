"""Original closed distant shell from the reviewed footprints and native terrain.
blender --background --python tools/author-bellwether-shell.py -- ROOT
Refuses existing revisions; no backup writes. Runtime has no town gameplay.
"""
from pathlib import Path
import hashlib, json, sys
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

root = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
out = root / 'assets/models/world/source/bellwether-shell-v2'
runtime = root / 'assets/models/world/runtime/bellwether-shell-v2'
if out.exists() or runtime.exists():
    raise RuntimeError('Preserve all existing source/runtime revisions.')
plan_path = root / 'docs/references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json'
terrain_path = root / 'assets/models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb'
plan = json.loads(plan_path.read_text())
if len(plan['buildings']) != 9:
    raise ValueError('Expected eight town masses and the separate shrine.')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.preferences.filepaths.save_version = 0
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1
bpy.ops.import_scene.gltf(filepath=str(terrain_path))
terrain = [o for o in bpy.context.scene.objects if o.type == 'MESH']
verts, polys = [], []
for obj in terrain:
    base = len(verts)
    verts.extend(obj.matrix_world @ v.co for v in obj.data.vertices)
    polys.extend([base+i for i in p.vertices] for p in obj.data.polygons)
bvh = BVHTree.FromPolygons(verts, polys)
def support(x, z):
    hit, _, _, _ = bvh.ray_cast(Vector((x, -z, 2000)), Vector((0, 0, -1)), 4000)
    if hit is None:
        raise ValueError(f'Missing native ground beneath town: {x},{z}')
    return hit.z
for obj in terrain:
    obj.hide_render = True
    obj.hide_set(True)
materials=[]
for name, color, rough in [('HordeTownStone',(1,1,1,1),1),('HordeTownRoofClay',(1,1,1,1),.92)]:
    m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=color
    bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=color
    bs.inputs['Roughness'].default_value=rough;bs.inputs['Metallic'].default_value=0
    materials.append(m)
objects=[]
def mesh(name, points, faces, mat):
    data=bpy.data.meshes.new(name)
    data.from_pydata([(x,-z,y) for x,y,z in points],[],faces);data.materials.append(materials[mat])
    data.update()
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj)
    uv=data.uv_layers.new(name='metre_projection')
    for polygon in data.polygons:
        axis=max(range(3),key=lambda a:abs(polygon.normal[a]))
        pair=((1,2),(0,2),(0,1))[axis]
        for loop in polygon.loop_indices:
            p=data.vertices[data.loops[loop].vertex_index].co
            uv.data[loop].uv=(p[pair[0]]*.5,p[pair[1]]*.5)
    objects.append(obj)
    return obj
def box(name,x,y,z,w,h,d,mat=0):
    pts=[(x-w/2,y,z-d/2),(x+w/2,y,z-d/2),(x+w/2,y,z+d/2),(x-w/2,y,z+d/2),
         (x-w/2,y+h,z-d/2),(x+w/2,y+h,z-d/2),(x+w/2,y+h,z+d/2),(x-w/2,y+h,z+d/2)]
    return mesh(name,pts,[(0,1,2,3),(4,5,1,0),(5,6,2,1),(6,7,3,2),(7,4,0,3),(7,6,5,4)],mat)
def roof(name,x,y,z,w,d,rise):
    # Closed triangular prism; actual sloping normals, not axial surface tags.
    pts=[(x-w/2,y,z-d/2),(x+w/2,y,z-d/2),(x,y+rise,z-d/2),
         (x-w/2,y,z+d/2),(x+w/2,y,z+d/2),(x,y+rise,z+d/2)]
    return mesh(name,pts,[(0,2,1),(3,4,5),(0,1,4,3),(1,2,5,4),(2,0,3,5)],1)
receipt=[]
wall_heights=[6.2,6.8,4.3,4.8,5.4,4.0,3.7,3.1,3.0]
for i,b in enumerate(plan['buildings']):
    x,z=b['x']+78.3,126.2-b['y'];w,d=b['width'],b['depth']
    samples=[support(x+u*w/2,z+v*d/2) for u,v in [(-1,-1),(-1,0),(-1,1),(0,-1),(0,0),(0,1),(1,-1),(1,0),(1,1)]]
    proposed=b['floor_z']-31.55
    floor=max(proposed,max(samples)+.08);bottom=min(samples)-.2
    box(b['id']+'_stone_foundation',x,bottom,z,w,floor-bottom,d)
    box(b['id']+'_closed_walls',x,floor-.02,z,w,wall_heights[i],d)
    eave=floor+wall_heights[i]-.07
    roof(b['id']+'_closed_gable',x,eave,z,w+.5,d+.5,min(w*.27,4.2))
    if i in (0,2,5):
        box(b['id']+'_cold_chimney',x+w*.23,eave,z-d*.12,1.1,2.3,1.0)
    if i==1:
        tx,tz=x-w*.29,z-d*.29
        box('B02_bell_tower',tx,floor,tz,4.6,14.8,4.6)
        mesh('B02_stone_spire',[(tx-2.55,floor+14.75,tz-2.55),(tx+2.55,floor+14.75,tz-2.55),
             (tx+2.55,floor+14.75,tz+2.55),(tx-2.55,floor+14.75,tz+2.55),(tx,floor+19.3,tz)],
             [(0,1,2,3),(4,1,0),(4,2,1),(4,3,2),(4,0,3)],1)
    receipt.append(dict(id=b['id'],label=b['label'],engineX=x,engineZ=z,width=w,depth=d,
         proposedFloorY=proposed,actualFloorY=floor,foundationBottomY=bottom,supportSamples=samples,
         adjustmentMetres=floor-proposed,wallHeight=wall_heights[i],nonplayable=True))
out.mkdir(parents=True);runtime.mkdir(parents=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'bellwether-shell-v2.blend'))
# Export copies; retain separately editable originals and terrain context.
bpy.ops.object.select_all(action='DESELECT')
copies=[]
for obj in objects:
    copy=obj.copy();copy.data=obj.data.copy();bpy.context.collection.objects.link(copy)
    copy.hide_set(False);copy.select_set(True);copies.append(copy)
bpy.context.view_layer.objects.active=copies[0]
bpy.ops.object.join()
combined=bpy.context.object;combined.name='Original_Bellwether_closed_distant_shell'
path=runtime/'bellwether-shell-v2.glb'
bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,
    export_apply=True,export_texcoords=True,export_normals=True,export_tangents=True,
    export_animations=False,export_extras=True)
manifest=dict(schemaVersion=1,asset='original-bellwether-distant-shell',units='metres',upAxis='Y',
    budgets=dict(maxVertices=4000,maxIndices=4000,maxPrimitives=2,maxMaterials=2,maxTextureLayersPerKind=1),
    lods=[dict(name='lod0',maxTriangles=1000)])
# Preserve the established importer schema rather than inventing one.
reference=json.loads((root/'assets/models/world/runtime/forest-understory-v1/asset.manifest.json').read_text())
reference['asset']=manifest['asset'];reference['budgets']=manifest['budgets'];reference['lods']=manifest['lods']
(runtime/'asset.manifest.json').write_text(json.dumps(reference,indent=2)+'\n')
(out/'authoring-receipt.json').write_text(json.dumps(dict(schema=1,originalHordeGeometry=True,
    planSha256=hashlib.sha256(plan_path.read_bytes()).hexdigest(),terrainSha256=hashlib.sha256(terrain_path.read_bytes()).hexdigest(),
    transform='X=East+78.3; Y=Up-31.55; Z=126.2-North',buildings=receipt,
    policy='Distant closed nonplayable shell; original sources, no new light/texture/transparency',
    runtimeSha256=hashlib.sha256(path.read_bytes()).hexdigest()),indent=2)+'\n')
print('Nine original closed masses exported; terrain-aware foundations; originals preserved.')
