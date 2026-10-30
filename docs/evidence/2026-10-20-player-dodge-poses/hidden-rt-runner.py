from pathlib import Path
import ctypes, hashlib, json, subprocess, time
root=Path.cwd().resolve(); base=root/'reports/player-dodge-20261020-r1'
assert not base.exists(); base.mkdir()
exe=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
settings=exe.parent/'HordeLanternRT.settings.ini'
u=ctypes.WinDLL('user32');u.GetForegroundWindow.restype=ctypes.c_void_p

def identity(p):
    return {'exists':p.exists(),**({'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} if p.exists() else {})}
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
for name,disabled,compute in [('pipeline-off',True,False),('pipeline-on',False,False),('compute-off',True,True),('compute-on',False,True)]:
    out=base/name;assert not out.exists()
    args=[str(exe),'--development-rescue-journey','--capture-showcase',str(out),'--capture-hidden','--capture-environment-survey','player-dodge']
    if disabled:args+=['--survey-dodge-disabled']
    if compute:args+=['--require-rayquery-compute']
    before=identity(settings);foreground=u.GetForegroundWindow();started=time.monotonic();samples=[]
    with (base/(name+'.stdout.log')).open('wb') as stdout,(base/(name+'.stderr.log')).open('wb') as stderr:
        p=subprocess.Popen(args,cwd=root,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        print(name+' own hidden PID '+str(p.pid),flush=True)
        while p.poll() is None and time.monotonic()-started<120:
            samples.append(u.GetForegroundWindow());time.sleep(.25)
        code=p.poll()
    result={'arguments':args,'pid':p.pid,'exitCode':code,'pending':code is None,'seconds':time.monotonic()-started,'executable':identity(exe),'settingsBefore':before,'settingsAfter':identity(settings),'foregroundBefore':foreground,'foregroundAfter':u.GetForegroundWindow(),'foregroundSamples':samples}
    result['settingsUnchanged']=result['settingsBefore']==result['settingsAfter']
    result['foregroundSamplesUnchanged']=all(x==foreground for x in samples) and result['foregroundAfter']==foreground
    save(base/(name+'.process.json'),result)
    print(json.dumps({k:result[k] for k in ['exitCode','pending','seconds','settingsUnchanged','foregroundSamplesUnchanged']}),flush=True)
    if code is None:raise RuntimeError('Own process is pending; no kill or retry.')
    assert code==0 and result['settingsUnchanged'] and result['foregroundSamplesUnchanged'],name
    native=json.loads((out/'player-dodge-survey.json').read_text(encoding='utf-8-sig'))
    assert native['complete'] and len(native['frames'])==64 and native['dodgePoseRequested']==(not disabled)
    print(name+' 64 completed native RT frames',flush=True)
