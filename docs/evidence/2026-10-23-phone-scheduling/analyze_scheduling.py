from pathlib import Path
import json,subprocess,csv,io,hashlib,re,sys
base=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
tp=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else Path('C:/Users/sam_s/AppData/Local/Temp/horde-perfetto-20261023/trace_processor_shell.exe')
trace=base/'scheduler-lf-normalized.perfetto-trace';analysis=json.loads((base/'traced33/analysis.json').read_text());sample=analysis['strictHold']['sampling']
request=json.loads((base/'traced33/request.json').read_text());pid=int(request['pid'])
log=(base/'traced33/owned-logcat.txt').read_text()
lines=[s for s in log.splitlines() if 'HORDE_MOTION armed id=stdin23-traced33' in s];assert len(lines)==1
tid=int(lines[0].split()[3]);assert int(lines[0].split()[2])==pid
def query(name,sql):
 (base/(name+'.sql')).write_text(sql+'\n')
 p=subprocess.run([str(tp),'query',str(trace),sql],capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
 (base/(name+'.csv')).write_bytes(p.stdout);(base/(name+'.stderr.txt')).write_bytes(p.stderr)
 assert p.returncode==0,p.stderr.decode(errors='replace')
 return list(csv.DictReader(io.StringIO(p.stdout.decode())))
clocks=query('clock-map',"SELECT ts,clock_value,ts-clock_value AS offset_ns FROM clock_snapshot WHERE clock_name='MONOTONIC';")
offsets={int(r['offset_ns']) for r in clocks};assert len(offsets)==1 and len(clocks)>=2
offset=offsets.pop();lo=sample['warmupEndWallNs']+offset;hi=sample['sampleEndWallNs']+offset
bounds=query('bounds','SELECT * FROM trace_bounds;');assert int(bounds[0]['start_ts'])<lo<hi<int(bounds[0]['end_ts'])
prefix=f'WITH b(lo,hi) AS (VALUES({lo},{hi})) '
clip='MAX(0,MIN(CASE WHEN s.dur<0 THEN hi ELSE s.ts+s.dur END,hi)-MAX(s.ts,lo))'
owner=f'FROM sched s JOIN thread t USING(utid) JOIN process p USING(upid) CROSS JOIN b WHERE p.pid={pid} AND s.ts<hi AND (s.dur<0 OR s.ts+s.dur>lo)'
threads=query('thread-oncore',prefix+f'SELECT t.tid,t.name,SUM({clip}) AS oncore_ns,COUNT(*) AS slices {owner} GROUP BY t.utid ORDER BY oncore_ns DESC;')
states=query('owner-states',prefix+f'SELECT s.state,COUNT(*) AS spans,SUM({clip}) AS duration_ns FROM thread_state s JOIN thread t USING(utid) CROSS JOIN b WHERE t.tid={tid} AND s.ts<hi AND (s.dur<0 OR s.ts+s.dur>lo) GROUP BY s.state ORDER BY duration_ns DESC;')
cores=query('owner-cores',prefix+f'SELECT s.cpu,SUM({clip}) AS oncore_ns,COUNT(*) AS slices {owner} AND t.tid={tid} GROUP BY s.cpu ORDER BY s.cpu;')
device=query('device-core-busy',prefix+f'SELECT s.cpu,SUM(CASE WHEN s.utid!=0 THEN {clip} ELSE 0 END) AS nonidle_ns,SUM({clip}) AS observed_ns FROM sched s CROSS JOIN b WHERE s.ts<hi AND (s.dur<0 OR s.ts+s.dur>lo) GROUP BY s.cpu ORDER BY s.cpu;')
freq=query('frequency-range',prefix+", f AS (SELECT c.ts,LEAD(c.ts,1,(SELECT hi FROM b)) OVER(PARTITION BY t.id ORDER BY c.ts) AS end_ts,c.value,t.cpu FROM counter c JOIN cpu_counter_track t ON t.id=c.track_id WHERE t.name='cpufreq') SELECT cpu,MIN(value) AS minimum_raw,MAX(value) AS maximum_raw,COUNT(*) AS intervals FROM f,b WHERE ts<hi AND end_ts>lo GROUP BY cpu ORDER BY cpu;")
errors=query('trace-errors',"SELECT name,idx,severity,source,value FROM stats WHERE value!=0 AND (severity IN ('data_loss','error') OR name GLOB '*overrun*' OR name GLOB '*lost*' OR name GLOB '*discard*');")
assert not errors,errors
state_total=sum(int(r['duration_ns']) for r in states);assert state_total==hi-lo,(state_total,hi-lo)
render=next(r for r in threads if int(r['tid'])==tid);busy=int(render['oncore_ns'])
running=sum(int(r['duration_ns']) for r in states if r['state']=='Running');assert running==busy
for rows,key in ((threads,'oncore_ns'),(states,'duration_ns'),(cores,'oncore_ns'),(device,'nonidle_ns')):
 for row in rows:row['percentOf30SecondWindow']=int(row[key])/(hi-lo)*100
result={'schema':1,'scope':'Actual scheduling events for the traced post-warm30s stationary normal-cache hold; qualitative current-build attribution, not an admitted overhead/performance comparison.',
 'pid':pid,'renderOwnerTid':tid,'renderOwnerIdentityEvidence':lines[0],'clockOffsetBoottimeMinusMonotonicNs':offset,'clockSnapshots':clocks,'traceSampleStartNs':lo,'traceSampleEndNs':hi,'sampleDurationNs':hi-lo,'fullTraceBounds':bounds,
 'threadOnCore':threads,'renderOwnerStates':states,'renderOwnerStateCoverageNs':state_total,'renderOwnerOnCoreByCpu':cores,'allProcessNonidleByCpu':device,'cpuFrequencyRawRanges':freq,'traceLossOrErrorStats':errors,
 'provenance':{'traceSha256':hashlib.sha256(trace.read_bytes()).hexdigest(),'rawTransportSha256':hashlib.sha256((base/'scheduler.perfetto-trace').read_bytes()).hexdigest(),'normalization':json.loads((base/'transport-normalization.json').read_text()),'analyzerExeSha256':hashlib.sha256(tp.read_bytes()).hexdigest(),'analysisScriptSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
 'limits':['Raw Windows shell stdout required a separate inverse CRLF transport normalization. Raw bytes retained; no on-device trace hash.','Closing control was thermally rejected. Observer overhead and thermal/context drift cannot be separated; this profile is qualitative.','Scheduling is actual on-core/runnable/sleeping time, not sampled stacks or exact CPU costs within animation/skin/AS phases.','No app stage markers or blocked-function call stacks; sleeping state alone does not identify Vulkan fence waits.','Frequency raw counter values and scheduling events do not establish GPU clocks, GPU stages, power or sustained acceptance.','Timed workload mutes dialogue/music and is seeded post-Keeper stationary, not ordinary full-route play.']}
(base/'scheduling-summary.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'renderOwnerTid':tid,'onCoreSeconds':busy/1e9,'states':states,'onCoreByCpu':cores,'frequencyRaw':freq,'errors':errors},indent=2))
