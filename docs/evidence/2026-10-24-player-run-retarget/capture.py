from pathlib import Path
import os,subprocess,json,hashlib,time,ctypes
r=Path.cwd();o=r/'reports/player-run-retarget-20261024';exe=r/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe';setting=exe.parent/'HordeLanternRT.settings.ini';campaign=Path(os.environ['LOCALAPPDATA'])/'HordeLanternRT/campaign-v1'
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def custody():
 return {'settings':h(setting),'campaignExists':campaign.exists(),'campaign':{str(p.relative_to(campaign)):h(p) for p in campaign.rglob('*') if p.is_file()},'parked':{p:h(r/p) for p in json.loads((r/'reports/kit-turn-departure-20261023/owner-untracked-before.json').read_text(encoding='utf-8-sig'))},'foreground':int(ctypes.windll.user32.GetForegroundWindow())}
before=custody();(o/'rt-custody-before.json').write_text(json.dumps(before,indent=2))
for tag,asset in [('rt-control',None),('rt-run',o/'player-run-native-calibrated-proof.glb')]:
 target=o/tag;assert not target.exists()
 env=os.environ.copy();env['HORDE_LOCAL_RUN_RT_PROOF']='1';env.pop('HORDE_LOCAL_RUN_CPU_ASSET',None)
 if asset:env['HORDE_LOCAL_RUN_CPU_ASSET']=str(asset)
 args=[str(exe),'--capture-showcase',str(target),'--capture-hidden','--development-checkpoint','player-viewmodel-forward']
 receipt={'exeSha256':h(exe),'candidateSha256':h(asset) if asset else None,'arguments':args[1:],'started':time.time(),'scope':'isolated hidden real RT presentation, not performance/natural route/phone acceptance','foregroundActivation':False}
 si=subprocess.STARTUPINFO();si.dwFlags|=subprocess.STARTF_USESHOWWINDOW;si.wShowWindow=0
 with (o/(tag+'.stdout.log')).open('w') as stdout,(o/(tag+'.stderr.log')).open('w') as stderr:
  p=subprocess.Popen(args,env=env,startupinfo=si,stdout=stdout,stderr=stderr);receipt['pid']=p.pid
  try:receipt['exit']=p.wait(timeout=180);receipt['timeout']=False
  except subprocess.TimeoutExpired:p.terminate();p.wait(timeout=15);receipt['timeout']=True;receipt['exit']=p.returncode
 receipt['elapsed']=time.time()-receipt['started'];receipt['custodyUnchanged']=custody()==before
 (o/(tag+'.process.json')).write_text(json.dumps(receipt,indent=2)+'\n');print(tag,receipt['exit'],receipt['elapsed'],receipt['custodyUnchanged'],flush=True)
 if not receipt['custodyUnchanged']:raise RuntimeError('Custody changed; stop further captures')
 if receipt['exit']!=0:break
(o/'rt-custody-after.json').write_text(json.dumps(custody(),indent=2)+'\n')
