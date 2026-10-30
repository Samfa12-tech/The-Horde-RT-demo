from pathlib import Path
import hashlib,json,struct,subprocess,zlib
root=Path.cwd();logs=root/'reports/windows-route-return-20261020-r3'
fixture=root/'reports/horde-campaign-fixtures-return-guard-matching-other';fixture.mkdir(exist_ok=False)
source=(root/'reports/horde-campaign-fixtures-route-20261020-r3/slot-1-auto.hrd').read_bytes()
other=bytearray(source);other[71]=0;struct.pack_into('<I',other,16,zlib.crc32(other[20:]))
assert other!=source and len(other)==162 and other[64:71]==source[64:71] and other[-10:]==source[-10:]
for name in ['slot-1-manual.hrd','slot-1-auto.hrd']:(fixture/name).write_bytes(other)
(fixture/'manifest.json').write_bytes((root/'reports/horde-campaign-fixtures-route-return-20261020-r2/manifest.json').read_bytes())
exe=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
cpu=exe.with_name('horde_rt_campaign_play_session_tests.exe');rows=[]
for label,args in [('native',[str(exe),'--route-return-recovery-hidden','--campaign-test-root',str(fixture)]),
 ('cpu',[str(cpu),'--check-actual-return-recovery',str(fixture)])]:
 with (logs/('exact-'+label+'.stdout.log')).open('wb') as out,(logs/('exact-'+label+'.stderr.log')).open('wb') as err:
  child=subprocess.Popen(args,cwd=root,stdout=out,stderr=err,creationflags=subprocess.CREATE_NO_WINDOW)
  try:code=child.wait(timeout=20)
  except subprocess.TimeoutExpired:raise SystemExit('Pending guardPID '+str(child.pid)+'; no kill/retry')
 rows.append({'entrypoint':label,'exitCode':code,'pass':code==2})
unchanged=all((fixture/n).read_bytes()==other for n in ['slot-1-manual.hrd','slot-1-auto.hrd'])
result={'cases':rows,'saveBytesUnchanged':unchanged,'originalSha256':hashlib.sha256(source).hexdigest(),
 'matchingOtherRecordSha256':hashlib.sha256(other).hexdigest(),'class':'two matching valid-CRC completed lower records with another held pose; rejected before game restoration'}
(logs/'exact-copy-guard.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
assert all(r['pass'] for r in rows) and unchanged
