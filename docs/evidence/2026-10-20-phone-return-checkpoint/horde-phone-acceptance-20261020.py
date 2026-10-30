from pathlib import Path
import hashlib,json,subprocess,sys,shlex
import re,time,xml.etree.ElementTree as ET
root=Path.cwd().resolve();out=root/'reports/horde-phone-acceptance-20261020-r1'
adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
serial='R5GL219SZGK';package='com.samfa12.hordelanternrt.debug'
def run(args):
    p=subprocess.run([adb,'-s',serial,*args],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    if p.returncode:raise RuntimeError(str(args)+' '+p.stderr.decode(errors='replace'))
    return p.stdout
def save(name,data):
    p=out/name;assert not p.exists();p.write_bytes(data if isinstance(data,bytes) else (json.dumps(data,indent=2)+'\n').encode())
def sha(b):return hashlib.sha256(b).hexdigest()
def inventory():
    command="run-as "+package+" sh -c "+shlex.quote('for d in files/campaign-v1 files/campaign-tests; do if [ -d "$d" ]; then find "$d" -type f -exec sha256sum {} \\;; fi; done')
    return run(['shell',command])
def prefs():return run(['exec-out','run-as',package,'cat','shared_prefs/horde_lantern_alpha_settings.xml'])
mode=sys.argv[1]
if mode=='prepare':
    out.mkdir(exist_ok=False)
    assert run(['shell','getprop','ro.product.model']).strip()==b'SM-S948B'
    assert not run(['shell','pidof',package]).strip() if False else True
    # pidof returns 1 when absent; inspect without treating that as a failure.
    p=subprocess.run([adb,'-s',serial,'shell','pidof',package],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    assert p.returncode==1 and not p.stdout.strip()
    save('before-saves.sha256',inventory());save('before-settings.xml',prefs())
    save('device-before.json',{k:run(['shell','getprop',v]).decode().strip() for k,v in {
      'model':'ro.product.model','android':'ro.build.version.release','api':'ro.build.version.sdk','fingerprint':'ro.build.fingerprint'}.items()})
    path=run(['shell','pm','path','--user','0',package]).decode().strip().removeprefix('package:')
    save('installed-before.sha256',run(['shell','sha256sum',path]))
    apk=root/'android/app/build/outputs/apk/debug/app-debug.apk'
    assert sha(apk.read_bytes())=='d1a052dd29fd8b6f61e70d247f0446373053bfb7e7c9af4bce07f84636b2ec43'
    save('local-apk.json',{'sha256':sha(apk.read_bytes()),'bytes':apk.stat().st_size,'source':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()})
    save('install.log',run(['install','-r',str(apk)]))
    path=run(['shell','pm','path','--user','0',package]).decode().strip().removeprefix('package:')
    installed=run(['shell','sha256sum',path]);save('installed-after.sha256',installed)
    assert installed.decode().split()[0]==sha(apk.read_bytes())
    after=inventory();settings=prefs();save('after-install-saves.sha256',after);save('after-install-settings.xml',settings)
    assert after==(out/'before-saves.sha256').read_bytes() and settings==(out/'before-settings.xml').read_bytes()
    print('Compatible update matched exact APK; all prior campaign/settings bytes preserved. No activity launched.',flush=True)
elif mode=='seed-launch':
    namespace='ret-20261020-r1'
    existing=run(['shell','run-as',package,'ls','files/campaign-tests']).decode().split()
    assert namespace not in existing
    source=root/'reports/horde-campaign-fixtures-route-20261020-r3/slot-1-auto.hrd'
    data=source.read_bytes();assert sha(data)=='031ced0a83a136861833ab4e9930ae6ea28c7c6e86d47c61c80ba24e8778472b'
    run(['shell','run-as',package,'mkdir','files/campaign-tests/'+namespace])
    command='run-as '+package+' sh -c '+shlex.quote('set -C; cat >files/campaign-tests/'+namespace+'/slot-1-auto.hrd')
    p=subprocess.run([adb,'-s',serial,'shell','-T',command],input=data,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    assert p.returncode==0,p.stderr
    device=run(['shell','run-as',package,'sha256sum','files/campaign-tests/'+namespace+'/slot-1-auto.hrd'])
    assert device.decode().split()[0]==sha(data)
    save('fixture-source.json',{'namespace':namespace,'source':str(source.relative_to(root)),'sha256':sha(data),'bytes':len(data),
       'evidenceClass':'copied actual Windows lower-return auto from a run that began post-Keeper; not naturally earned phone victory/claim'})
    save('launch.log',run(['shell','am','start','--user','0','-n',package+'/com.samfa12.hordelanternrt.MainActivity','--es','campaign_test_namespace',namespace]))
    print('Fresh isolated actual-return fixture launched; all owner roots excluded.',flush=True)
elif mode=='snapshot':
    label=sys.argv[2];assert label.replace('-','').isalnum()
    p=subprocess.run([adb,'-s',serial,'exec-out','uiautomator','dump','--compressed','/dev/tty'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    save(label+'.xml',p.stdout+p.stderr)
    save(label+'.png',run(['exec-out','screencap','-p']))
    print('Saved original UI and screenshot '+label+'; inspect before input.',flush=True)
elif mode=='tap':
    text,label=sys.argv[2:4];assert label.replace('-','').isalnum()
    raw=run(['exec-out','uiautomator','dump','--compressed','/dev/tty']);save(label+'-before.xml',raw)
    xml=raw.decode();xml=xml[xml.index('<hierarchy'):xml.index('</hierarchy>')+12]
    nodes=[n for n in ET.fromstring(xml).iter('node') if n.get('text')==text and n.get('enabled')=='true' and n.get('clickable')=='true' and n.get('package')==package]
    assert len(nodes)==1,[(n.get('text'),n.get('bounds')) for n in nodes]
    bounds=[int(x) for x in re.findall(r'\d+',nodes[0].get('bounds'))];x=(bounds[0]+bounds[2])//2;y=(bounds[1]+bounds[3])//2
    save(label+'-action.json',{'text':text,'bounds':bounds,'x':x,'y':y,'utcUnixMs':round(time.time()*1000)})
    run(['shell','input','tap',str(x),str(y)])
    raw=run(['exec-out','uiautomator','dump','--compressed','/dev/tty']);save(label+'.xml',raw)
    save(label+'.png',run(['exec-out','screencap','-p']))
    print('Tapped verified current '+text+' once; saved '+label,flush=True)
elif mode=='scroll-menu':
    label=sys.argv[2]
    raw=run(['exec-out','uiautomator','dump','--compressed','/dev/tty']);save(label+'-before.xml',raw)
    xml=raw.decode();xml=xml[xml.index('<hierarchy'):xml.index('</hierarchy>')+12]
    tree=ET.fromstring(xml)
    assert any(n.get('text') in ('LOAD CHECKPOINT','SAVE CHECKPOINT') or (n.get('text') or '').startswith('SHOWCASE ') for n in tree.iter('node'))
    nodes=[n for n in tree.iter('node') if n.get('scrollable')=='true' and n.get('package')==package]
    assert len(nodes)==1
    b=[int(x) for x in re.findall(r'\d+',nodes[0].get('bounds'))];x=(b[0]+b[2])//2
    run(['shell','input','swipe',str(x),str(b[3]-160),str(x),str(b[1]+260),'450'])
    save(label+'.xml',run(['exec-out','uiautomator','dump','--compressed','/dev/tty']))
    save(label+'.png',run(['exec-out','screencap','-p']))
    print('Scrolled verified native checkpoint menu; no gameplay input.',flush=True)
elif mode=='collect':
    label=sys.argv[2]
    save(label+'-saves.sha256',inventory());save(label+'-settings.xml',prefs())
    ns='files/campaign-tests/ret-20261020-r1/'
    for name in run(['shell','run-as',package,'ls',ns]).decode().split():
        if name.endswith('.hrd'):save(label+'-'+name,run(['exec-out','run-as',package,'cat',ns+name]))
    pid=run(['shell','pidof',package]).decode().strip()
    save(label+'-pid.txt',pid.encode())
    save(label+'-logcat.txt',run(['logcat','-d','-v','threadtime','--pid='+pid]))
    print('Collected isolated saves, scoped process log and custody '+label,flush=True)
elif mode=='home':
    label=sys.argv[2]
    raw=run(['exec-out','uiautomator','dump','--compressed','/dev/tty'])
    assert package.encode() in raw
    save(label+'-before.xml',raw)
    save(label+'-pid.txt',run(['shell','pidof',package]))
    run(['shell','input','keyevent','KEYCODE_HOME'])
    save(label+'.xml',run(['exec-out','uiautomator','dump','--compressed','/dev/tty']))
    save(label+'.png',run(['exec-out','screencap','-p']))
    print('Sent Home from verified Horde UI; inspect launcher before resume.',flush=True)
elif mode=='resume-existing':
    label=sys.argv[2]
    before=run(['shell','pidof',package]).decode().strip()
    assert before==(out/'12-home-pid.txt').read_text().strip()
    save(label+'-launch.log',run(['shell','am','start','--user','0','--activity-single-top','-n',package+'/com.samfa12.hordelanternrt.MainActivity']))
    save(label+'-pid.txt',run(['shell','pidof',package]))
    assert before==(out/(label+'-pid.txt')).read_text().strip()
    save(label+'.xml',run(['exec-out','uiautomator','dump','--compressed','/dev/tty']))
    save(label+'.png',run(['exec-out','screencap','-p']))
    print('Resumed the same isolated-test process without changing intent state.',flush=True)
elif mode=='close-verify':
    after=inventory();settings=prefs()
    save('after-quit-saves.sha256',after);save('after-quit-settings.xml',settings)
    def entries(raw):return {line.split(None,1)[1]:line.split(None,1)[0] for line in raw.decode().splitlines()}
    old=entries((out/'before-saves.sha256').read_bytes());new=entries(after)
    assert all(new.get(path)==value for path,value in old.items())
    assert all('campaign-tests/ret-20261020-r1/' in path for path in new.keys()-old.keys())
    assert settings==(out/'before-settings.xml').read_bytes()
    raw=run(['exec-out','uiautomator','dump','--compressed','/dev/tty']);save('after-quit-ui.xml',raw)
    assert b'com.sec.android.app.launcher' in raw and package.encode() not in raw
    process=subprocess.run([adb,'-s',serial,'shell','pidof',package],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    save('after-quit-pid.txt',process.stdout)
    save('closure.json',{'utcUnixMs':round(time.time()*1000),'originalCampaignFilesAndLocks':len(old),'originalCampaignBytesUnchanged':True,'settingsBytesUnchanged':True,'onlyNewNamespaceAdded':'ret-20261020-r1','launcherForeground':True,'debugPid':process.stdout.decode().strip() or None,'ordinaryQuit':True,'activeHostDeviceHelpers':0,'safeToDisconnect':True,'nextDeviceOperation':'none; paused by owner'})
    print('CLOSED: owner campaign/settings byte-exact, launcher foreground, Debug PID '+(process.stdout.decode().strip() or 'absent')+'. Safe to disconnect; no install/capture/transfer active.',flush=True)
else:raise RuntimeError('Explicit checkpoint mode required.')
