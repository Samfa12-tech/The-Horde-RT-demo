"""Package verified original candidates; manual handoff ZIP, not game-dev canonical package."""
from pathlib import Path
import json,hashlib,zipfile,shutil,sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent.parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
mesh=json.loads((root/'qa/mesh-summary.json').read_text())+json.loads((root/'qa/mesh-summary-lod1.json').read_text())
byteqa=json.loads((root/'qa/runtime-byte-inspection.json').read_text()); lod=json.loads((root/'qa/lod-comparison.json').read_text()); rebuild=json.loads((root/'qa/deterministic-rebuild.json').read_text()); rt=json.loads((root/'qa/blender-roundtrip.json').read_text())+json.loads((root/'qa/blender-roundtrip-lod1.json').read_text())
assert all(x['pass'] for x in lod) and all(x['binary_identical'] for x in rebuild) and all(x['blender_roundtrip_import_pass'] for x in rt)
assert all(x['sha256']==sha(root/x['file']) for x in byteqa)
for obj in mesh:
 runtime=root/'runtime'/f"{obj['asset']}.glb"
 assert runtime.exists() and obj['triangles']<=8000
 # Keep established front proof names current while primary package avoids duplicating PNG bytes.
 shutil.copyfile(root/'previews'/f"{obj['asset']}-runtime-reimport.png",root/'previews'/f"{obj['asset']}-front.png")
summary={'package':'Horde 1.7 original woodland tree pair','version':'1.0.0','status':'art-reviewed mid-distance source and runtime candidates','created_date':'2026-10-06','authorship':'Original geometry and mathematical PBR texture source authored for The Horde','license_notice':'No new public license assigned by this package; owner controls publication','paid_provider_calls':0,'provider_spend':0,'third_party_source_geometry_or_textures':False,'style_reference_url':'https://samfa12.com/games/briarhold/play/assets/world/briarhold-forest-impostor-512.webp','reference_image_included':False,'blender_version':'4.3.2','assets':mesh,'validated_gates':['GLB v2 byte structure and self-contained images','finite positions/normals/UVs, valid indices and zero degenerate triangles','opaque materials and original 512px maps','closed source parts and reimported parts after UV-seam welding','Blender actual-GLB reimport and rendered review','identical centerlines/radii/leaf poses across LODs','wood side-branch start attachment within parent path volume','front/back LOD silhouette comparison','binary-identical deterministic GLB and texture rebuild'],'unperformed_gates':['game-dev canonical package build/verify and policy validation: CLI absent','engine/GPU runtime import and integration','runtime frame-rate, draw call, collision, LOD-switch and wind tests'],'visual_limitations':['Intentionally sparse canopies and chunky solid foliage','Mid-distance source candidates, not hero-closeup finished art or botanical realism','Overlapping branch/root parts and texture discontinuity at joins remain visible close up','Root tips penetrate ground about 7mm'],'lod_metrics':lod,'delivery_note':'Primary ZIP includes final runtime front/back views and shared pair render; duplicate front-name PNGs and intermediate Blender backups/logs are omitted. Full local evidence remains beside the archive.'}
(root/'qa/QA-SUMMARY.json').write_text(json.dumps(summary,indent=2,ensure_ascii=False))
files=[root/'README.txt']
for folder,patterns in [('source',['*.py','*.blend']),('runtime',['*.glb']),('textures',['*.png']),('qa',['*.json','*-silhouette.png']),('previews',['*-runtime-reimport.png','*-back.png','*-bark-detail.png','horde-tree-pair-runtime-comparison.png'])]:
 for pattern in patterns: files.extend((root/folder).glob(pattern))
files=sorted(set(files)); manifest=[{'path':str(p.relative_to(root)),'bytes':p.stat().st_size,'sha256':sha(p)} for p in files if p.name!='SHA256-MANIFEST.json']
(root/'SHA256-MANIFEST.json').write_text(json.dumps(manifest,indent=2)); files.append(root/'SHA256-MANIFEST.json')
zip_path=root.parent/'Horde-1.7-original-tree-pair-source-candidates-v1.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED,compresslevel=7) as z:
 for p in files: z.write(p,Path(root.name)/p.relative_to(root))
with zipfile.ZipFile(zip_path) as z:
 assert z.testzip() is None
 for e in manifest: assert hashlib.sha256(z.read(str(Path(root.name)/e['path']))).hexdigest()==e['sha256']
assert zip_path.stat().st_size<32*1024*1024
receipt={'archive':str(zip_path),'bytes':zip_path.stat().st_size,'sha256':sha(zip_path),'zip_crc_check':'pass','manifest_hash_check':'pass','files':len(files),'below_32_mib':True}
(root.parent/'Horde-1.7-original-tree-pair-package-receipt.json').write_text(json.dumps(receipt,indent=2)); print(json.dumps(receipt,indent=2))
