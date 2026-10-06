"""Build and independently verify a source/evidence ZIP, not a game-dev package.
Usage: python package_tools.py --asset-dir ROOT --zip-path NEW_ZIP_PATH
"""
from pathlib import Path
import argparse,hashlib,json,zipfile
ap=argparse.ArgumentParser();ap.add_argument('--asset-dir',required=True);ap.add_argument('--zip-path',required=True);args=ap.parse_args();r=Path(args.asset_dir).resolve();z=Path(args.zip_path).resolve()
assert not z.exists(),'Use a fresh ZIP destination; will not overwrite.'
files=sorted(f for f in r.rglob('*') if f.is_file() and not f.name.endswith('.blend1') and f.name!='manifest.json' and '__pycache__' not in f.parts)
manifest={'package_id':'horde.e02.abbey-bell-attendant-tools.v1','package_type':'Original source-plus-evidence proposal ZIP; not game-dev canonical package','version':'1.0.0','canonical_glbs':['abbey_bell_attendant_handbell.glb','abbey_bell_attendant_short_staff.glb'],'files':[{'path':str(f.relative_to(r)),'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in files],'excluded_working_files':['Blender .blend1 automatic backups'],'validation':'evidence/validation.json','rights':'provenance.json; no new public licence assigned'}
(r/'manifest.json').write_text(json.dumps(manifest,indent=2))
with zipfile.ZipFile(z,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
 for f in files+[r/'manifest.json']:out.write(f,arcname=r.name+'/'+str(f.relative_to(r)))
with zipfile.ZipFile(z) as archive:
 assert archive.testzip() is None
 got=json.loads(archive.read(r.name+'/manifest.json'))
 for item in got['files']:
  raw=archive.read(r.name+'/'+item['path']);assert len(raw)==item['bytes'];assert hashlib.sha256(raw).hexdigest()==item['sha256']
 assert len(archive.namelist())==len(got['files'])+1
 assert z.stat().st_size<18000000
 receipt={'path':str(z),'bytes':z.stat().st_size,'sha256':hashlib.sha256(z.read_bytes()).hexdigest(),'file_count':len(archive.namelist()),'zip_crc_test':'PASS','all_manifest_hashes_verified_from_zip':'PASS','package_limit_bytes':18000000,'validation_result':json.load(open(r/'evidence'/'validation.json'))['result'],'contact_sheet':str(r/'previews/contact_sheet.jpg'),'handbell_glb_sha256':hashlib.sha256((r/'abbey_bell_attendant_handbell.glb').read_bytes()).hexdigest(),'short_staff_glb_sha256':hashlib.sha256((r/'abbey_bell_attendant_short_staff.glb').read_bytes()).hexdigest()}
receipt_path=z.with_name(z.stem+'-receipt.json');receipt_path.write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt,indent=2))
