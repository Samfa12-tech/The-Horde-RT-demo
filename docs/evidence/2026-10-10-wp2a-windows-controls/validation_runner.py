import subprocess,sys,json,time,hashlib,re,os
from pathlib import Path
from datetime import datetime,timezone
root=Path.cwd(); out=root/'reports/wp2a-windows-controls';out.mkdir(exist_ok=True)
cache=(root/'build/presets/windows-x64-debug/CMakeCache.txt').read_text()
def cached(k):return next(l.split('=',1)[1] for l in cache.splitlines() if l.startswith(k+':INTERNAL='))
paths={'<cmake>':cached('CMAKE_COMMAND'),'<ctest>':cached('CMAKE_CTEST_COMMAND')}
label=sys.argv[1];args=[paths.get(a,a) for a in sys.argv[2:]]
assert not (out/(label+'.log')).exists()
def git(*a):return subprocess.check_output(['git',*a],text=True).strip()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def scrub(s):
 s=s.replace(str(root),'<repo>').replace(str(root).replace('\\','/'),'<repo>')
 s=re.sub(r'C:[/\\]+Users[/\\]+[^/\\\s"<>]+','<user-home>',s,flags=re.I)
 return s
head=git('rev-parse','HEAD');diff=subprocess.check_output(['git','diff','--binary','HEAD'])
start=datetime.now(timezone.utc).isoformat();t=time.perf_counter()
with (out/(label+'.private.log')).open('wb') as f:
 p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,env={**os.environ,'HORDE_VALIDATION_UNSIGNED':'1'})
elapsed=time.perf_counter()-t
log=out/(label+'.log');content=scrub((out/(label+'.private.log')).read_text(errors='replace'))
log.write_bytes(('\n'.join(l.rstrip() for l in content.splitlines()).rstrip()+'\n').encode())
row=dict(label=label,argv=[Path(a).name if a in paths.values() else scrub(a) for a in args],cwd='<repo>',
 source_head=head,source_head_at_end=git('rev-parse','HEAD'),tracked_diff_sha256=hashlib.sha256(diff).hexdigest(),
 start_utc=start,elapsed_seconds=elapsed,exit_code=p.returncode,log=log.name,log_sha256=sha(log),
 source_unchanged_during_run=head==git('rev-parse','HEAD') and diff==subprocess.check_output(['git','diff','--binary','HEAD']))
with (out/'runs.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
print(json.dumps(row),flush=True)
if p.returncode:print('\n'.join(l for l in content.splitlines() if 'fail' in l.lower() or 'error' in l.lower())[-2000:])
sys.exit(p.returncode)
