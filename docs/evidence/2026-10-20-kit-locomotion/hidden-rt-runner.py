from pathlib import Path
import ctypes,hashlib,json,subprocess,sys,time
root=Path.cwd().resolve();out=root/'reports/kit-locomotion-20261020-r1'
fixture=root/'reports/horde-campaign-fixtures-kit-locomotion-20261020-r1'
debug=root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
release=root/'build/presets/windows-x64-release/Release/HordeLanternRT.exe'
u=ctypes.WinDLL('user32');u.GetForegroundWindow.restype=ctypes.c_void_p
def fg():return u.GetForegroundWindow()
def save(p,r):p.write_bytes((json.dumps(r,indent=2)+'\n').encode('utf-8'))
def identity(p):
    return {'exists':p.exists(),**({'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} if p.exists() else {})}
def launch(args,label,timeout):
    before=fg();start=time.monotonic()
    with (out/(label+'.stdout.log')).open('wb') as stdout,(out/(label+'.stderr.log')).open('wb') as stderr:
        p=subprocess.Popen([str(a) for a in args],cwd=root,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        print(label+' own hidden PID '+str(p.pid),flush=True)
        try:code=p.wait(timeout=timeout)
        except subprocess.TimeoutExpired:code=None
    r={'pid':p.pid,'exitCode':code,'seconds':round(time.monotonic()-start,3),'foregroundBefore':before,'foregroundAfter':fg(),'pending':code is None}
    save(out/(label+'.json'),r);print(json.dumps(r),flush=True)
    if code is None:raise RuntimeError('Own process remains pending; no process killed, no retry.')
    return r
mode=sys.argv[1]
if mode=='guards':
    out.mkdir(exist_ok=False);receipts={}
    for name,args in [
        ('release-reject',[release,'--lookout-validate-hidden','--lookout-inspect-kit-motion','--campaign-test-root',fixture]),
        ('unbound-inspect-reject',[debug,'--lookout-inspect-kit-motion']),
        ('old-mode-inspect-reject',[debug,'--campaign-validate-hidden','--lookout-inspect-kit-motion','--campaign-test-root',fixture])]:
        r=launch(args,name,15);assert r['exitCode']==2 and r['foregroundBefore']==r['foregroundAfter'];receipts[name]=r
    save(out/'guards.json',receipts)
elif mode=='run':
    assert (out/'guards.json').is_file()
    expected={'manifest.json','slot-1-auto.hrd','.slot-1-auto.hrd.lock'}
    assert fixture.parent==root/'reports' and fixture.resolve()==fixture
    assert {p.name for p in fixture.iterdir()}==expected
    assert all(p.is_file() and not p.is_symlink() for p in fixture.iterdir())
    settings=debug.parent/'HordeLanternRT.settings.ini'
    before={p.name:identity(p) for p in fixture.iterdir()};settingsBefore=identity(settings)
    save(out/'before.json',{'fixture':before,'settings':settingsBefore,'debug':identity(debug),'release':identity(release),
        'sourceBase':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()})
    r=launch([debug,'--lookout-validate-hidden','--lookout-inspect-kit-motion','--campaign-test-root',fixture],'hidden-rt',120)
    after={p:identity(fixture/p) for p in expected};settingsAfter=identity(settings)
    custody={'run':r,'fixtureBefore':before,'fixtureAfter':after,'fixtureUnchanged':before==after,
        'settingsBefore':settingsBefore,'settingsAfter':settingsAfter,'settingsUnchanged':settingsBefore==settingsAfter}
    save(out/'custody.json',custody)
    assert before==after and settingsBefore==settingsAfter
    assert r['exitCode']==0 and r['foregroundBefore']==r['foregroundAfter']
    native=json.loads((fixture/'windows-lookout-validation.json').read_text())
    print(json.dumps({'checks':native['count'],'failures':native['failures'],'complete':native['complete'],
        'motionInspection':native['kitMotionInspection']['requested'],'motionFrames':len(native['kitMotionInspection']['frames']),'renderCalls':native['renderCalls']}),flush=True)
else:raise RuntimeError('Explicit guards or run mode required')
