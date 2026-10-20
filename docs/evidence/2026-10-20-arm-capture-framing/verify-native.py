from pathlib import Path
import ctypes,hashlib,json,os,subprocess,time
root=Path.cwd().resolve();base=root/'reports/arm-admission-20261020-r1';exe=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
u=ctypes.WinDLL('user32');u.GetForegroundWindow.restype=ctypes.c_void_p

def ident(p):return {'exists':p.exists(),**({'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} if p.is_file() else {})}
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
assert ident(exe)['sha256']=='223b1f02398f852f05bbf6d26239a748b996b2d4e0d448d16cad899827c85579'
settings=exe.parent/'HordeLanternRT.settings.ini';results=[]
for label,checkpoint,expected in [('framing-start','rescue-journey-start',1),('framing-niche','chapter-niche-rect',1),('positive-forest','chapter-forest-first-bend',0)]:
    out=base/label;assert not out.exists(),out
    args=[str(exe),'--development-rescue-journey','--capture-showcase',str(out),'--capture-hidden','--development-checkpoint',checkpoint]
    before=ident(settings);foreground=u.GetForegroundWindow();start=time.monotonic();samples=[]
    with (base/(label+'.stdout.log')).open('wb') as stdout,(base/(label+'.stderr.log')).open('wb') as stderr:
        process=subprocess.Popen(args,cwd=root,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        print(label+' own hidden PID '+str(process.pid),flush=True)
        while process.poll() is None and time.monotonic()-start<120:
            samples.append(u.GetForegroundWindow());time.sleep(.25)
        code=process.poll()
    row={'label':label,'arguments':args,'pid':process.pid,'exitCode':code,'expectedNativeExit':expected,'pending':code is None,'seconds':time.monotonic()-start,'executable':ident(exe),'settingsBefore':before,'settingsAfter':ident(settings),'foregroundBefore':foreground,'foregroundAfter':u.GetForegroundWindow(),'foregroundSamples':samples}
    row['settingsUnchanged']=row['settingsAfter']==before;row['foregroundSamplesUnchanged']=all(x==foreground for x in samples) and row['foregroundAfter']==foreground
    results.append(row);save(base/'processes.json',results)
    if code is None:raise RuntimeError('Own process remains pending; do not kill or retry.')
    assert code==expected and row['settingsUnchanged'] and row['foregroundSamplesUnchanged'],row
    manifest=json.loads((out/'capture-manifest.json').read_text(encoding='utf-8-sig'))
    if expected:
        assert not manifest['complete'] and manifest['captures']==[] and 'armPixels=0' in manifest['error']
        row['result']='Predicted gate rejection observed; full capture remains UNADMITTED'
    else:
        assert manifest['complete'] and len(manifest['captures'])==1 and manifest['captures'][0]['honestlyPresentedRtFrame']
        assert manifest['captures'][0]['visibility']['primaryPixels']['player']>0
        row['result']='Current positive control complete with actual primary player pixels'
    save(base/'processes.json',results);print(label+': '+row['result'],flush=True)
