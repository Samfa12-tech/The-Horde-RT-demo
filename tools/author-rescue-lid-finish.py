"""Source-only Blender round trip of the shared original moving-lid recipe.
Run: blender --background --python tools/author-rescue-lid-finish.py -- ROOT
Never overwrites an existing source revision or enables Blender backups.
"""
from pathlib import Path
import hashlib, json, re, sys
import bpy

root = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
target = root / 'assets/source/development-rescue-lid-finish-v1'
if target.exists():
    raise RuntimeError('Preserve existing source revision; choose a new revision for changes.')
recipe = root / 'src/scene/RescueLidFinish.h'
boxes = re.findall(r'\{\{\{([^{}]+)\}\},\{\{([^{}]+)\}\},0u\}', recipe.read_text())
if len(boxes) != 6:
    raise RuntimeError('Expected the six reviewed closed stone pieces.')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.preferences.filepaths.save_version = 0
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1
stone = bpy.data.materials.new('DryStone')
stone.diffuse_color = (1, 1, 1, 1)
stone.use_nodes = True
stone.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value = 1
for index, (lo, hi) in enumerate(boxes):
    lower = [float(v.strip().removesuffix('f')) for v in lo.split(',')]
    upper = [float(v.strip().removesuffix('f')) for v in hi.split(',')]
    centre = [(a+b)/2 for a,b in zip(lower, upper)]
    span = [b-a for a,b in zip(lower, upper)]
    bpy.ops.mesh.primitive_cube_add(size=1, location=(centre[0], -centre[2], centre[1]))
    obj = bpy.context.object
    obj.name = f'Original_lid_stone_{index:02}'
    obj.dimensions = (span[0], span[2], span[1])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(stone)
target.mkdir(parents=True)
bpy.ops.wm.save_as_mainfile(filepath=str(target / 'rescue-lid-finish.blend'))
bpy.ops.export_scene.gltf(filepath=str(target / 'rescue-lid-finish-roundtrip.glb'),
    export_format='GLB', export_yup=True, export_tangents=True, export_animations=False)
manifest = json.loads((root / 'assets/source/development-rescue-ground-stake-v1/asset.manifest.json').read_text())
manifest['asset'] = 'original-rescue-lid-finish'
manifest['budgets'] = dict(maxVertices=200, maxIndices=300, maxPrimitives=6, maxMaterials=1,
    maxTextureLayersPerKind=1)
manifest['lods'] = [dict(name='lod0', maxTriangles=72)]
(target / 'asset.manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', newline='\n')
hash_file = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
receipt = dict(schema=1, blender=bpy.app.version_string, units='metres', runtimeAuthority=str(recipe.relative_to(root)),
    recipeSha256=hash_file(recipe), authoringScriptSha256=hash_file(Path(__file__)),
    bounds=dict(minimum=[-34.94,2.05,-16.64],maximum=[-32.46,2.19,-13.76]),
    boxes=6, triangles=72, sourceOnly=True, files={p.name:dict(bytes=p.stat().st_size,sha256=hash_file(p))
        for p in sorted(target.iterdir()) if p.is_file()})
(target / 'derivation-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', newline='\n')
(target / 'RIGHTS.md').write_text('Original Horde geometry authored from the shared engine recipe by Codex.\n'
    'No external geometry, texture, provider output or audio is included.\n'
    'Runtime stone textures retain their separate existing asset notices.\n', newline='\n')
(target / 'README.md').write_text('Source-only editable rescue grave lid and native GLB round trip.\n'
    'The runtime uses the same six-box C++ recipe, not an additional GLB resource.\n'
    'All relief remains within the accepted slab bounds. No light, texture, loot or inscription is added.\n'
    'Reproduce with tools/author-rescue-lid-finish.py; existing revisions are never overwritten.\n', newline='\n')
