from pathlib import Path
import hashlib,json,subprocess
root=Path.cwd();logs=root/'reports/windows-route-20261020-r3';fixture=root/'reports/horde-campaign-fixtures-route-20261020-r3'
exe=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
exporter=exe.with_name('horde_rt_campaign_play_session_tests.exe')
def hashes():return {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in fixture.iterdir() if p.is_file()}
before=hashes();results=[]
for label,args in [
 ('missing-root',[str(exe),'--route-validate-hidden']),
 ('mixed-modes',[str(exe),'--route-validate-hidden','--lookout-validate-hidden','--campaign-test-root',str(fixture)]),
 ('nonfixture-root',[str(exe),'--route-validate-hidden','--campaign-test-root',str(root)]),
 ('used-fixture',[str(exe),'--route-validate-hidden','--campaign-test-root',str(fixture)]),
 ('nonempty-export',[str(exporter),'--export-route-fixture',str(fixture)])]:
    with (logs/(label+'.stdout.log')).open('wb') as out,(logs/(label+'.stderr.log')).open('wb') as err:
        child=subprocess.Popen(args,cwd=root,stdout=out,stderr=err,creationflags=subprocess.CREATE_NO_WINDOW)
        try:code=child.wait(timeout=20)
        except subprocess.TimeoutExpired:raise SystemExit('Guard process pending: '+str(child.pid)+'; no kill or retry')
    results.append({'case':label,'exitCode':code,'expectedExitCode':2,'pass':code==2})
after=hashes();result={'cases':results,'allFixtureBytesUnchanged':before==after,'fixtureSha256':after}
(logs/'guard-tests.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'cases':results,'allFixtureBytesUnchanged':before==after},indent=2))
assert all(r['pass'] for r in results) and before==after
