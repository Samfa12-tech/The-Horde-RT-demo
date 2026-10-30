"""Original Horde grave markers. Fresh versioned destinations only; no backup writes.

blender --background --factory-startup --python tools/author-grave-markers.py -- --revision r01
No external assets, texture generation, provider jobs or old master edits.
"""
import argparse
import hashlib
import importlib.util
import json
import math
import sys
from pathlib import Path
import bpy

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--revision',required=True)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
if not args.revision.startswith('r') or not args.revision[1:].isdigit():
    raise RuntimeError('Revision must be rNN; no arbitrary output paths')
SOURCE=ROOT/'assets/models/world/source'/('grave-markers-'+args.revision)
RUNTIME=ROOT/'assets/models/world/runtime'/('grave-markers-'+args.revision)
if SOURCE.exists() or RUNTIME.exists():
    raise RuntimeError('Refusing to overwrite an existing source/runtime revision')
SOURCE.mkdir(parents=True); RUNTIME.mkdir(parents=True)
bpy.context.preferences.filepaths.save_version=0
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
mat=bpy.data.materials.new('MedievalWall02')
mat.use_nodes=True
bsdf=mat.node_tree.nodes.get('Principled BSDF')
bsdf.inputs['Base Color'].default_value=(.42,.37,.30,1)
bsdf.inputs['Roughness'].default_value=.86
families={}

def finish(obj,collection):
    for c in list(obj.users_collection): c.objects.unlink(obj)
    collection.objects.link(obj); obj.data.materials.append(mat)
    bpy.context.view_layer.objects.active=obj
    mod=obj.modifiers.new('Stone arris bevel 12mm','BEVEL'); mod.width=.012; mod.segments=2
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    # Real-world planar mapping: two metres per texture repeat, no atlas change.
    uv=obj.data.uv_layers.new(name='UVMap')
    for face in obj.data.polygons:
        axis=max(range(3),key=lambda i:abs(face.normal[i]))
        for li in face.loop_indices:
            p=obj.data.vertices[obj.data.loops[li].vertex_index].co
            pair=(p.y,p.z) if axis==0 else ((p.x,p.z) if axis==1 else (p.x,p.y))
            uv.data[li].uv=(pair[0]*.5,pair[1]*.5)
    mod=obj.modifiers.new('Native triangles','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=mod.name)

def marker(name,damaged=False):
    col=bpy.data.collections.new(name); scene.collection.children.link(col)
    half=.26 if not damaged else .23
    base=.06; shoulder=.64 if not damaged else .48
    if damaged:
        profile=[(-half,base),(half,base),(half,.58),(.04,.69),(-.08,.59),(-half,.65)]
    else:
        profile=[(-half,base),(half,base)]+[(half*math.cos(t),shoulder+half*math.sin(t))
            for t in [i*math.pi/10 for i in range(11)]]
    n=len(profile)
    verts=[(x,y,z) for y in [-.07,.07] for x,z in profile]
    faces=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh=bpy.data.meshes.new(name+'-closed-stone'); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); col.objects.link(obj); finish(obj,col)
    bpy.ops.mesh.primitive_cube_add(size=1,location=(0,0,.015))
    foot=bpy.context.object; foot.name=name+'-foot'
    foot.dimensions=(.62 if not damaged else .56,.34,.13)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    finish(foot,col)
    families[name]=col

marker('grave-marker-round'); marker('grave-marker-damaged',True)
blend=SOURCE/'grave-markers.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
# Reuse only the deterministic GLB accessor/order canonicalizer, not its authoring.
spec=importlib.util.spec_from_file_location('tomb_canonical',ROOT/'tools/author-tomb-dressing.py')
module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
receipt={'schema':1,'authoring':'Original Horde Blender geometry; no third-party input',
    'sourceBlendSha256':hashlib.sha256(blend.read_bytes()).hexdigest(),
    'scriptSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'blenderVersion':bpy.app.version_string,'units':'metres','nativeAxes':'+Y up / +Z forward',
    'uvScale':'2m per repeat; production reuses admitted MedievalWall02',
    'texturesAdded':0,'assets':[]}
for name,col in families.items():
    bpy.ops.object.select_all(action='DESELECT')
    for obj in col.objects: obj.select_set(True)
    path=RUNTIME/(name+'.runtime.glb')
    bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,
        export_yup=True,export_apply=True,export_materials='EXPORT',export_normals=True,
        export_texcoords=True,export_tangents=True,export_animations=False)
    module.canonicalize_glb(path)
    triangles=sum(len(obj.data.polygons) for obj in col.objects)
    manifest={'schema':1,'asset':name,'metresPerUnit':1,'coordinateSystem':{'up':'+Y','forward':'+Z'},
        'budgets':{'maxVertices':triangles*3,'maxIndices':triangles*3,'maxPrimitives':2,
            'maxMaterials':1,'maxTextureLayersPerKind':1},'lods':[{'name':'lod0','maxTriangles':triangles}],
        'requiredSockets':[], 'materialOverrides':[],
        'runtimeTextureProfile':{'android':'astc','windows':'rgba8','mipmapped':True,'resolution':1024},
        'distribution':'candidate','licenceStatus':'Original Horde Blender geometry; no third-party input'}
    (RUNTIME/(name+'.manifest.json')).write_text(json.dumps(manifest,indent=2)+'\n')
    receipt['assets'].append({'name':name,'file':path.name,'triangles':triangles,
        'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size})
(SOURCE/'derivation-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
(SOURCE/'RIGHTS.md').write_text('# Original Horde grave markers\n\nOriginal code-authored Blender geometry, created for Horde. No provider generation, third-party model, image or texture is embedded. The runtime uses the separately admitted MedievalWall02 texture family; its existing CC0 notice remains applicable. These markers do not change any other asset licence.\n')
(SOURCE/'README.md').write_text('# Grave markers '+args.revision+'\n\nTwo original closed, bevelled stone forms. Native metre scale, +Y up/+Z forward. Editable master and reproducible authoring remain outside runtime packages. Plain markers reveal no new lore. Nonemissive; no new lights or textures. Derivation receipt binds the master, script and exports.\n\nReproduce into a NEW revision with `blender --background --factory-startup --python tools/author-grave-markers.py -- --revision rNN`. Existing revision directories are refused and Blender backup writes are disabled before saving. Final native admission and RT inspection are separate gates.\n')
print(json.dumps(receipt,indent=2))
