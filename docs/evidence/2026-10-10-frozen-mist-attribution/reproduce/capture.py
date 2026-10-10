import os,sys,json,time,subprocess,hashlib,re,shutil
from pathlib import Path
root=Path.cwd();out=root/'reports/frozen-mist';exe=out/'build/Debug/frozen_mist.exe'
rows=json.loads((out/'shader-metrics.json').read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
i=int(sys.argv[1]);name=rows[i]['name'];target=out/'captures'/name
if target.exists():
 n=2
 while target.with_name(name+f'-{n}').exists():n+=1
 target=target.with_name(name+f'-{n}')
target.mkdir(parents=True)
args=[str(exe),'--capture-showcase',str(target),'--development-checkpoint','rescue-journey-start','--capture-dust','standard']
t=time.perf_counter();env={**os.environ,'HORDE_MIST_PROBE_MODE':str(i)}
si=subprocess.STARTUPINFO();si.dwFlags=subprocess.STARTF_USESHOWWINDOW;si.wShowWindow=0
with (target/'capture.private.log').open('wb') as log:
 try:p=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,env=env,startupinfo=si,timeout=180);exitcode=p.returncode
 except subprocess.TimeoutExpired:exitcode=124
elapsed=time.perf_counter()-t
s=(target/'capture.private.log').read_text(errors='replace').replace(str(root),'<repo>').replace(root.as_posix(),'<repo>')
s=re.sub(r'C:[/\\]+Users[/\\]+[^/\\\s"<>]+','<user-home>',s,flags=re.I)
(target/'capture.log').write_text(s,encoding='utf-8',newline='\n')
if (exe.parent/'reports').exists():shutil.copytree(exe.parent/'reports',target/'native-reports')
row=dict(name=name,source_head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),argv=['frozen_mist.exe',*[(a.replace(str(root),'<repo>')) for a in args[1:]]],mode=i,elapsed_seconds=elapsed,exit_code=exitcode,exe_sha256=sha(exe),shader=rows[i],files={str(p.relative_to(target)):sha(p) for p in target.rglob('*') if p.is_file()})
(target/'run.json').write_text(json.dumps(row,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(name=name,exit_code=exitcode,elapsed_seconds=elapsed)),flush=True)
print(s[-1500:])
sys.exit(exitcode)
