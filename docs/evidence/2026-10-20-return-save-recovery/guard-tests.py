from pathlib import Path
import hashlib,json,subprocess
root=Path.cwd();logs=root/'reports/windows-route-return-20261020-r2';fixture=root/'reports/horde-campaign-fixtures-route-return-20261020-r2'
exe=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
def hashes(d):return {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in d.iterdir() if p.is_file()}
before=hashes(fixture);cases=[]
for label,flags in [
 ('missing-root',[]),
 ('mixed-modes',['--route-validate-hidden','--campaign-test-root',str(fixture)]),
 ('nonfixture-root',['--campaign-test-root',str(root)]),
 ('used-fixture',['--campaign-test-root',str(fixture)])]:
 args=[str(exe),'--route-return-recovery-hidden',*flags]
 with (logs/(label+'.stderr.log')).open('wb') as err:
  child=subprocess.Popen(args,cwd=root,stdout=subprocess.DEVNULL,stderr=err,creationflags=subprocess.CREATE_NO_WINDOW)
  try:code=child.wait(timeout=20)
  except subprocess.TimeoutExpired:raise SystemExit('Guard pending PID '+str(child.pid)+'; no kill/retry')
 cases.append({'case':label,'exitCode':code,'pass':code==2})
for label,bad_manifest in [('mismatched-copy',False),('bad-manifest',True)]:
 d=root/'reports'/('horde-campaign-fixtures-return-guard-'+label)
 d.mkdir(exist_ok=False)
 actual=(fixture/'slot-1-manual.hrd').read_bytes()
 (d/'slot-1-manual.hrd').write_bytes(actual)
 (d/'slot-1-auto.hrd').write_bytes(actual if bad_manifest else (root/'reports/horde-campaign-fixtures-route-20261020-r3/slot-2-manual.hrd').read_bytes())
 (d/'manifest.json').write_bytes(b'wrong\n' if bad_manifest else (fixture/'manifest.json').read_bytes())
 child=subprocess.Popen([str(exe),'--route-return-recovery-hidden','--campaign-test-root',str(d)],cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=subprocess.CREATE_NO_WINDOW)
 try:code=child.wait(timeout=20)
 except subprocess.TimeoutExpired:raise SystemExit('Guard pending PID '+str(child.pid)+'; no kill/retry')
 cases.append({'case':label,'exitCode':code,'pass':code==2})
result={'cases':cases,'existingFixtureBytesUnchanged':before==hashes(fixture),'fixtureSha256':hashes(fixture)}
(logs/'guard-tests.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'cases':cases,'existingFixtureBytesUnchanged':result['existingFixtureBytesUnchanged']},indent=2))
assert all(c['pass'] for c in cases) and result['existingFixtureBytesUnchanged']
