from pathlib import Path
import ctypes,hashlib,json,os,subprocess,time
root=Path.cwd().resolve();base=root/'reports/tomb-composition-20261020-r1'
exe=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
settings=exe.parent/'HordeLanternRT.settings.ini'
u=ctypes.WinDLL('user32');u.GetForegroundWindow.restype=ctypes.c_void_p

def identity(p):
    return {'exists':p.exists(),**({'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} if p.is_file() else {})}
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
results=[]
for label,checkpoint in [('opening-survey',None),('blocked-retreat','rescue-journey-start'),('rect-niche','chapter-niche-rect'),('arch-niche','chapter-niche-arch')]:
    out=base/label;assert not out.exists(),out
    args=[str(exe),'--development-rescue-journey','--capture-showcase',str(out),'--capture-hidden']
    args+=['--capture-environment-survey','tomb-debris'] if checkpoint is None else ['--development-checkpoint',checkpoint]
    before=identity(settings);foreground=u.GetForegroundWindow();started=time.monotonic();samples=[]
    with (base/(label+'.stdout.log')).open('wb') as stdout,(base/(label+'.stderr.log')).open('wb') as stderr:
        p=subprocess.Popen(args,cwd=root,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        print(label+' own hidden PID '+str(p.pid),flush=True)
        while p.poll() is None and time.monotonic()-started<120:
            samples.append(u.GetForegroundWindow());time.sleep(.25)
        code=p.poll()
    result={'label':label,'arguments':args,'pid':p.pid,'exitCode':code,'pending':code is None,'seconds':time.monotonic()-started,'executable':identity(exe),'settingsBefore':before,'settingsAfter':identity(settings),'foregroundBefore':foreground,'foregroundAfter':u.GetForegroundWindow(),'foregroundSamples':samples}
    result['settingsUnchanged']=result['settingsBefore']==result['settingsAfter']
    result['foregroundSamplesUnchanged']=all(x==foreground for x in samples) and result['foregroundAfter']==foreground
    results.append(result);save(base/'processes.json',results)
    print(json.dumps({k:result[k] for k in ['label','exitCode','pending','seconds','settingsUnchanged','foregroundSamplesUnchanged']}),flush=True)
    if code is None:raise RuntimeError('Own process pending; no kill or retry.')
    assert code==0 and result['settingsUnchanged'] and result['foregroundSamplesUnchanged'],label
    native=json.loads((out/('capture-manifest.json' if checkpoint else 'environment-survey.json')).read_text(encoding='utf-8-sig'))
    assert native['complete'],label
    if checkpoint is None:assert len(native['frames'])==120 and native['simulationSnapshotObjectBytesUnchanged']
    else:assert len(native['captures'])==1 and native['captures'][0]['honestlyPresentedRtFrame']
    print(label+' native manifest complete',flush=True)
