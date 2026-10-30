from pathlib import Path
import subprocess,json,time,re,hashlib
root=Path.cwd();out=root/'reports/horde-phone-acceptance-20261020-r1'
adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
serial='R5GL219SZGK';pkg='com.samfa12.hordelanternrt.debug'
def run(args,required=True):
 p=subprocess.run([adb,'-s',serial,*args],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 if required and p.returncode:raise RuntimeError(str(args)+' '+p.stderr.decode(errors='replace'))
 return p.stdout
def save(name,data):
 p=out/name;assert not p.exists();p.write_bytes(data if isinstance(data,bytes) else (json.dumps(data,indent=2)+'\n').encode())
def thermal(label):
 save(label+'-thermal.txt',run(['shell','dumpsys','thermalservice']))
 save(label+'-battery.txt',run(['shell','dumpsys','battery']))
def state(label):
 for name in ['showcase_debug_state.json','vulkan_capability_report.json']:
  data=run(['exec-out','run-as',pkg,'cat','files/reports/'+name],False)
  save(label+'-'+name,data)
pid=run(['shell','pidof',pkg]).decode().strip()
assert pid==(out/'upper-pid.txt').read_text().strip()
prefs=run(['exec-out','run-as',pkg,'cat','shared_prefs/horde_lantern_alpha_settings.xml'])
assert prefs==(out/'before-settings.xml').read_bytes()
save('warm-plan.json',{'utcUnixMs':round(time.time()*1000),'pid':pid,'checkpoint':'chapter-forest-lookout','method':'existing native checkpoint benchmark, 120 warm + 3 x 120 sampled frames; first run is conditioning; second is retained warm baseline','minimumConditioningSeconds':60,'perRunTimeoutSeconds':180,'comparison':False,'limits':'seeded noncombat exterior only, no earned victory, no live Keeper, CPU-present-loop averages are not actual-display intervals or GPU percentiles; no settings overrides'})
state('pre-warm');thermal('pre-warm')
started=time.monotonic();generations=[]
for label in ['conditioning','warm']:
 if label=='warm':
  while time.monotonic()-started<60:time.sleep(min(3,60-(time.monotonic()-started)))
  thermal('warm-start')
 prior=run(['logcat','-d','-v','threadtime','--pid='+pid]).decode(errors='replace')
 prior_generations=[int(x) for x in re.findall(r'HORDE_BENCH begin generation=(\d+)',prior)]
 previous=max(prior_generations,default=0)
 save(label+'-launch.log',run(['shell','am','start','--user','0','--activity-single-top','-n',pkg+'/com.samfa12.hordelanternrt.MainActivity','--es','campaign_test_namespace','ret-20261020-r1','--es','horde.debug.checkpoint','chapter-forest-lookout','--ez','horde.debug.autostart','true','--ez','horde.debug.overlay','false','--ez','horde.debug.gpu_timing','true']))
 began=time.monotonic();generation=None
 print(label+' request dispatched in same isolated process '+pid,flush=True)
 while time.monotonic()-began<180:
  assert run(['shell','pidof',pkg]).decode().strip()==pid
  log=run(['logcat','-d','-v','threadtime','--pid='+pid]).decode(errors='replace')
  matches=re.findall(r'HORDE_BENCH begin generation=(\d+) checkpoint=chapter-forest-lookout scale=40',log)
  current=[int(x) for x in matches if int(x)>previous]
  if current:generation=max(current)
  if generation and re.search(r'HORDE_BENCH complete generation='+str(generation)+r' checkpoint=chapter-forest-lookout scale=40 windows=3',log):break
  time.sleep(3)
 else:
  save(label+'-timeout-logcat.txt',log.encode());raise RuntimeError(label+' did not complete within original 180-second bound')
 save(label+'-logcat.txt',log.encode());state(label);thermal(label+'-end')
 parsed=json.loads((out/(label+'-showcase_debug_state.json')).read_text())
 assert parsed['status']=='complete' and parsed['checkpoint']=='chapter-forest-lookout' and parsed['presented'] is True
 samples=re.findall(r'HORDE_BENCH sample generation='+str(generation)+r' checkpoint=chapter-forest-lookout scale=40 window=(\d+) frames=120 total_ms=([0-9.]+) fence_ms=([0-9.]+) record_ms=([0-9.]+) present_ms=([0-9.]+) zone=([^ ]+) presented=(\d)',log)
 assert len(samples)==3 and all(s[-1]=='1' for s in samples)
 generations.append({'label':label,'generation':generation,'elapsedSeconds':time.monotonic()-began,'elapsedSinceConditioningStart':time.monotonic()-started,'samples':samples})
 print(label+' complete generation='+str(generation)+' windows='+str(samples),flush=True)
 assert run(['exec-out','run-as',pkg,'cat','shared_prefs/horde_lantern_alpha_settings.xml'])==prefs
save('warm-results.json',generations)
print('Bounded warm batch complete; no install/capture/transfer subprocess remains in this helper.',flush=True)
