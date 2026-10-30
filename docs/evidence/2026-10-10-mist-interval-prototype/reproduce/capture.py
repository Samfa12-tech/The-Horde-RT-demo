"""Hidden game-owned RT captures. Nonzero admission exits are retained.
Usage: capture.py MODE CURRENT|HIGHER (never passes desktop/private pixels).
"""
import os,sys,json,time,subprocess,hashlib,re,shutil
from pathlib import Path
root=Path.cwd();out=root/'reports/mist-interval-prototype';exe=out/'build/Debug/frozen_mist.exe'
rows=json.loads((out/'shader-metrics.json').read_text());mode=int(sys.argv[1]);quality=sys.argv[2]
assert quality in ['CURRENT','HIGHER'] and 0<=mode<len(rows)
name=quality.lower()+'-'+rows[mode]['name'];target=out/'captures'/name
if len(sys.argv)>3:
    yaw=float(sys.argv[3]);assert abs(yaw)<=.25
    name+='-yaw'+str(yaw);target=out/'captures'/name
if target.exists():
    n=2
    while target.with_name(name+f'-{n}').exists():n+=1
    target=target.with_name(name+f'-{n}')
target.mkdir(parents=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
args=[str(exe),'--capture-showcase',str(target),'--development-checkpoint','rescue-journey-start','--capture-dust','standard']
env={**os.environ,'HORDE_MIST_PROBE_MODE':str(mode),'HORDE_MIST_PROBE_SHADOW':quality}
if len(sys.argv)>3:env['HORDE_MIST_VIEW_YAW']=sys.argv[3]
startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
t=time.perf_counter()
with (target/'capture.private.log').open('wb') as log:
    try:p=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,env=env,startupinfo=startup,timeout=180);code=p.returncode
    except subprocess.TimeoutExpired:code=124
elapsed=time.perf_counter()-t
log=(target/'capture.private.log').read_text(errors='replace').replace(str(root),'<repo>').replace(root.as_posix(),'<repo>')
log=re.sub(r'C:[/\\]+Users[/\\]+[^/\\\s"<>]+','<user-home>',log,flags=re.I)
(target/'capture.log').write_text(log,encoding='utf-8',newline='\n')
if (out/'native-resources.json').exists():shutil.copy2(out/'native-resources.json',target/'native-resources.json')
frame=json.loads((target/'completed-frame.json').read_text()) if (target/'completed-frame.json').exists() else None
row=dict(source_head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),argv=[Path(args[0]).name,*[a.replace(str(root),'<repo>') for a in args[1:]]],environment={k:env[k] for k in ['HORDE_MIST_PROBE_MODE','HORDE_MIST_PROBE_SHADOW','HORDE_MIST_VIEW_YAW'] if k in env},mode=mode,quality=quality,elapsed_seconds=elapsed,exit_code=code,exe_sha256=sha(exe),shader=rows[mode],completed_frame=frame,files={str(p.relative_to(target)):sha(p) for p in target.iterdir() if p.is_file() and 'private' not in p.name})
(target/'run.json').write_text(json.dumps(row,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(name=target.name,exit_code=code,elapsed_seconds=elapsed,presented=frame and frame['presentation']['presented'])),flush=True)
print(log[-1000:]);sys.exit(code)
