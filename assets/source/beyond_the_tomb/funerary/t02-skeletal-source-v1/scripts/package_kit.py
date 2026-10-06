"""Create a checksummed source-candidate archive; not a canonical game-dev package."""
import pathlib,hashlib,zipfile,argparse,json
p=argparse.ArgumentParser();p.add_argument('--package',required=True);p.add_argument('--archive',required=True);a=p.parse_args();root=pathlib.Path(a.package);archive=pathlib.Path(a.archive)
files=[p for p in sorted(root.rglob('*')) if p.is_file() and '__pycache__' not in p.parts and p.name!='SHA256SUMS']
sums=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(root).as_posix()+'\n' for p in files)
(root/'SHA256SUMS').write_text(sums);files.append(root/'SHA256SUMS')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in sorted(files):z.write(p,(pathlib.Path(root.name)/p.relative_to(root)).as_posix())
assert archive.stat().st_size<18_000_000
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 for row in sums.splitlines():
  digest,name=row.split('  ',1);assert hashlib.sha256(z.read(root.name+'/'+name)).hexdigest()==digest
report={'archive':archive.name,'bytes':archive.stat().st_size,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'files':len(files),'all_member_hashes_verified':True,'zip_integrity':'PASS','under_18_MB_decimal':True,'package_type':'Source-candidate ZIP, not canonical game-dev certification'}
archive.with_suffix('.receipt.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
