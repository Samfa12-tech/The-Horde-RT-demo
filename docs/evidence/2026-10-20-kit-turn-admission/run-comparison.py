from pathlib import Path
import csv,ctypes,hashlib,json,os,subprocess,time
root=Path.cwd().resolve();out=root/'reports/kit-turn-admission-20261020-r1'
assert out.is_dir() and not (out/'plan.json').exists()
def identity(p):return {'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def save(p,r):
    assert not p.exists()
    p.write_bytes((json.dumps(r,indent=2)+'\n').encode())
plan={'order':['control-a1','candidate-b','control-a2'],'maximumRuns':3,'maximumSecondsPerNativeRun':120,
 'nativeLimitsUnchanged':{'seconds':90,'renderCalls':180},'samplesPerGroup':60,'warmupPerGroup':12,
 'groups':['walk','turn'],'metrics':['wholeRtGpuMs','characterSkinCpuMs','wholeFrameCycleCpuMs'],
 'admission':'All measured GPU and character-skin median/p95 values must be <=1.15 times the lower control bracket. No image/custody/resource regression; thermal comparison must pass.',
 'thermalAdmission':'AC unchanged; group median temperature within 3 C and median graphics clock within 10 percent of both controls. Pre-run temperature within 2 C of first control, max 30s wait.',
 'controlScope':'Only renderer CompanionUpperBodyRotations vs CompanionPoseRotations; same candidate simulation oscillator on both. No total simulation-cost claim.',
 'scope':'one bounded Debug desktop control/candidate/control comparison; not phone or sustained-gameplay parity'}
save(out/'plan.json',plan)
user=ctypes.WinDLL('user32');user.GetForegroundWindow.restype=ctypes.c_void_p
class Power(ctypes.Structure):
    _fields_=[('ac',ctypes.c_ubyte),('flag',ctypes.c_ubyte),('percent',ctypes.c_ubyte),('system',ctypes.c_ubyte),('life',ctypes.c_uint32),('full',ctypes.c_uint32)]
def power():
    p=Power();return dict(ac=p.ac,percent=p.percent) if ctypes.windll.kernel32.GetSystemPowerStatus(ctypes.byref(p)) else None
def gpu():
    fields=['index','name','driver_version','temperature.gpu','clocks.gr','clocks.mem','power.draw','pstate','utilization.gpu']
    p=subprocess.run(['C:/Windows/system32/nvidia-smi.exe','--query-gpu='+','.join(fields),'--format=csv,noheader,nounits'],capture_output=True,timeout=5,creationflags=subprocess.CREATE_NO_WINDOW)
    if p.returncode:raise RuntimeError('Read-only desktop GPU query failed.')
    rows=list(csv.reader(p.stdout.decode().strip().splitlines()));assert len(rows)==1
    d=dict(zip(fields,[v.strip() for v in rows[0]]));d['utcUnixMs']=round(time.time()*1000);d['powerStatus']=power()
    return d
initial=gpu();save(out/'initial-gpu.json',initial)
settings=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.settings.ini'
owner=root.parents[1]
files=[settings,owner/'ASSET_LICENSES.md',owner/'PROJECT_MEMORY.md']
campaign=Path(os.environ['LOCALAPPDATA'])/'HordeLanternRT/campaign-v1'
assert not campaign.exists()
custody={str(p):identity(p) for p in files};save(out/'owner-before.json',custody)
for label in plan['order']:
    variant='candidate' if label.startswith('candidate') else 'control'
    exe=out/variant/'HordeLanternRT.exe';assert exe.is_file()
    fixture=root/('reports/horde-campaign-fixtures-kit-turn-admission-20261020-'+label)
    assert not fixture.exists();fixture.mkdir()
    export=subprocess.run([str(root/'build/presets/windows-x64-debug/Debug/horde_rt_campaign_play_session_tests.exe'),'--export-lookout-fixture',str(fixture)],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    (out/(label+'-export.log')).write_bytes(export.stdout+export.stderr);assert export.returncode==0
    original={p.name:identity(p) for p in fixture.iterdir()}
    thermal=[];limit=time.monotonic()+30
    while True:
        g=gpu();thermal.append(g)
        if abs(float(g['temperature.gpu'])-float(initial['temperature.gpu']))<=2:break
        if time.monotonic()>=limit:
            save(out/(label+'-precondition.json'),thermal)
            raise RuntimeError('Comparable pre-run temperature unavailable within bound; no extra run.')
        time.sleep(1)
    fg=user.GetForegroundWindow();started=time.monotonic()
    with (out/(label+'-stdout.log')).open('wb') as stdout,(out/(label+'-stderr.log')).open('wb') as stderr:
        command=[str(exe),'--lookout-validate-hidden','--lookout-inspect-kit-timing','--campaign-test-root',str(fixture)]
        child=subprocess.Popen(command,cwd=root,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        print(label+' hidden PID '+str(child.pid),flush=True)
        while child.poll() is None and time.monotonic()-started<120:
            thermal.append(gpu());time.sleep(.5)
        code=child.poll()
    result={'command':command,'pid':child.pid,'exitCode':code,'pending':code is None,'elapsedSeconds':time.monotonic()-started,
      'foregroundBefore':fg,'foregroundAfter':user.GetForegroundWindow(),'executable':identity(exe),'thermalSamples':thermal,
      'fixtureOriginal':original,'fixtureUnchanged':all(identity(fixture/n)==v for n,v in original.items()),
      'settingsCopyUnchanged':identity(out/variant/'HordeLanternRT.settings.ini')==identity(settings),
      'ownerUnchanged':custody=={str(p):identity(p) for p in files} and not campaign.exists()}
    save(out/(label+'-receipt.json'),result)
    print(json.dumps({k:result[k] for k in ('pid','exitCode','pending','elapsedSeconds','fixtureUnchanged','ownerUnchanged')}),flush=True)
    if code is None:raise RuntimeError('Owned native process remains pending; no kill or retry.')
    assert code==0 and fg==result['foregroundAfter'] and result['fixtureUnchanged'] and result['ownerUnchanged'] and result['settingsCopyUnchanged']
save(out/'owner-after.json',{str(p):identity(p) for p in files})
print('Finite control/candidate/control comparison finished; no further run allowed by this plan.',flush=True)
