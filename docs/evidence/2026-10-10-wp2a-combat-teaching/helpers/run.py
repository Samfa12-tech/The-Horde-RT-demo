import subprocess,time,json,hashlib,sys,os,re
from pathlib import Path
from datetime import datetime,timezone
r=Path.cwd(); out=r/'reports/wp2a-combat';cache=(r/'build/presets/windows-x64-debug/CMakeCache.txt').read_text()
cmake=next(l.split('=',1)[1] for l in cache.splitlines() if l.startswith('CMAKE_COMMAND:INTERNAL='))
ctest=next(l.split('=',1)[1] for l in cache.splitlines() if l.startswith('CMAKE_CTEST_COMMAND:INTERNAL='))
label=sys.argv[1]; base_label=label; ordinal=1
while (out/(label+'.private.log')).exists():
 ordinal+=1;label=base_label+'-'+str(ordinal)
args=sys.argv[2:];args=[cmake if x=='<cmake>' else ctest if x=='<ctest>' else x for x in args]
env=dict(os.environ);env['HORDE_VALIDATION_UNSIGNED']='1';env['JAVA_HOME']=r'C:\Program Files\Eclipse Adoptium\jdk-21.0.11.10-hotspot'
args=[str((r/x).resolve()) if x.endswith('.bat') else x for x in args]
def source_manifest():
 paths=set(subprocess.check_output(['git','-c','core.autocrlf=false','diff','--name-only','8bb62ede19b0a06756b89abc9812d4828c61a280'],cwd=r,text=True).splitlines())
 paths.update(subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=r,text=True).splitlines())
 return {p:hashlib.sha256((r/p).read_bytes()).hexdigest() for p in sorted(paths) if (r/p).is_file() and not p.startswith('docs/')}
start_manifest=source_manifest()
start_head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()
log=out/(label+'.private.log');t=time.perf_counter();start=datetime.now(timezone.utc).isoformat()
with log.open('wb') as f:p=subprocess.run(args,cwd=r,stdout=f,stderr=subprocess.STDOUT,env=env)
text=log.read_text(errors='replace').replace(str(r),'<repo>').replace(str(r).replace('\\','/'),'<repo>')
text=re.sub(r'C:[/\\]Users[/\\][^/\\]+', '<user-home>',text,flags=re.I)
text=text.replace(str(r).replace('\\','\\\\'),'<repo>')
text=re.sub(r'C:[/\\]+Users[/\\]+[^/\\]+','<user-home>',text,flags=re.I)
public=out/(label+'.log');public.write_text(text)
diff=subprocess.check_output(['git','-c','core.safecrlf=false','-c','core.autocrlf=false','diff','--binary','8bb62ede19b0a06756b89abc9812d4828c61a280'],cwd=r)
row=dict(label=label,argv=[Path(x).name if x in [cmake,ctest] else x for x in args],cwd='<repo>',start_utc=start,elapsed_seconds=time.perf_counter()-t,exit_code=p.returncode,source_head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip(),tracked_diff_sha256=hashlib.sha256(diff).hexdigest(),log=public.name,log_sha256=hashlib.sha256(public.read_bytes()).hexdigest(),environment={'HORDE_VALIDATION_UNSIGNED':'1'})
row['argv']=[str(x).replace(str(r),'<repo>') for x in row['argv']];row['changed_source_sha256']=start_manifest
row['source_head_at_start']=start_head
row['source_unchanged_during_run']=start_manifest==source_manifest() and start_head==row['source_head']
with (out/'runs.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
print(json.dumps({k:v for k,v in row.items() if k!='changed_source_sha256'}),flush=True)
if p.returncode:print('\n'.join(l for l in text.splitlines() if 'error' in l.lower() or 'fail' in l.lower())[-6000:],flush=True)
sys.exit(p.returncode)
