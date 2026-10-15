import argparse, csv, json, math, pathlib, statistics

HOST_SPANS=('host_previous_tail_ns','host_system_load_ns','host_messages_ns',
            'host_visibility_ns','host_clock_simulation_ns','host_render_to_present_ns')
HOST_FIELDS=(*HOST_SPANS,'host_previous_evidence_ns','host_phases_available')
def verify_host_phases(row):
    assert set(k for k in row if k.startswith('host_'))==set(HOST_FIELDS)
    assert row['host_phases_available']==row['present_interval_available']
    values={k:int(row[k]) for k in HOST_FIELDS[:-1]}
    assert all(v>=0 for v in values.values())
    if row['host_phases_available']=='1':
        assert sum(values[k] for k in HOST_SPANS)==int(row['present_return_interval_ns'])
        assert values['host_previous_evidence_ns']<=values['host_previous_tail_ns']
    else:
        assert row['host_phases_available']=='0' and not any(values.values())
    return values

def stats(values):
    if not values: return {'count': 0, 'available': False}
    v = sorted(values)
    def rank(p): return v[max(0, math.ceil(len(v)*p)-1)]
    return dict(count=len(v), mean=statistics.fmean(v), median=statistics.median(v),
                p95=rank(.95), p99=rank(.99), maximum=v[-1], minimum=v[0])

def analyze(name, analysis_name='analysis-v3.json'):
    root=pathlib.Path(name); sealed=(root/'completed-frames.csv').exists()
    directory=root if sealed else root/'shipping-route'
    run=json.loads((directory/'run.json').read_text())
    receipt=json.loads((root/'launch.candidate.json' if sealed else pathlib.Path(name+'.candidate.json')).read_text(encoding='utf-8-sig'))
    manifest=json.loads((root/'capture-manifest.json').read_text())
    rows=list(csv.DictReader((directory/'completed-frames.csv').open()))
    identities=[json.loads(line) for line in (directory/'identities.jsonl').read_text().splitlines()]
    assert run['schema'] in (2,3,4) and run['complete'] and 60<=run['durationSeconds']<=150
    legacy=run['schema']==2
    assert run['scope'].startswith('Optimized Release CPU')
    assert run['unoccludedClientChecks'] and 'Release CPU' in receipt['scope']
    assert len(rows)==run['completedSamples']==run['renderedFrames']
    streaming='staticDetailUnloads' in run
    if streaming:
        assert run['staticDetailUnloads']>0 and run['staticDetailRestores']==run['staticDetailUnloads']
        assert run['finalStaticDetailSavingBytes']==0
        assert max(int(r['static_saved_owner_bytes']) for r in rows)>0
        assert max(int(r['static_omitted_triangles']) for r in rows)==84219
        assert int(rows[-1]['static_saved_owner_bytes'])==int(rows[-1]['static_omitted_triangles'])==0
        assert int(rows[-1]['static_restores'])==run['staticDetailRestores']
    else:
        assert run['normalStaticRegionResidencySavingBytes']==0
    by_serial={int(r['submission_serial']):r for r in rows}
    assert len(by_serial)==len(rows)
    serials=list(by_serial); assert serials==list(range(serials[0],serials[-1]+1))
    assert len({(r['scene_epoch'],r['generation']) for r in rows})==1
    assert all(float(a['elapsed_s'])<=float(b['elapsed_s']) and int(a['tick'])<=int(b['tick'])
               for a,b in zip(rows,rows[1:]))
    assert all(r['unoccluded_client']=='1' for r in rows)
    if legacy:
        assert all(int(r['present_return_interval_ns'])>0 for r in rows)
    else:
        assert rows[0]['present_interval_available']=='0' and int(rows[0]['present_return_interval_ns'])==0
        assert all(r['present_interval_available']=='1' and int(r['present_return_interval_ns'])>0 for r in rows[1:])
    if run.get('foregroundRequested'):
        assert receipt['foregroundRequested'] and all(r['foreground_client']=='1' for r in rows)
    # Printed elapsed seconds have six significant figures. Check the interval
    # journal against their independent differences with only formatting error.
    assert all(abs((float(b['elapsed_s'])-float(a['elapsed_s']))-int(b['present_return_interval_ns'])/1e9)<.0002
               for a,b in zip(rows,rows[1:]))
    stage_columns=[k for k in rows[0] if k.endswith('_ns') and k!='gpu_ns' and k not in HOST_FIELDS]
    if run['schema']==4:
        for row in rows: verify_host_phases(row)
    assert all(int(r[k])>=0 for r in rows for k in stage_columns)
    assert all(int(r[k])==0 for r in rows for k in r if k.endswith('_overflow'))
    settings=[]
    for item in identities:
        e=item['evidence']; identity=e['identity']; r=by_serial[identity['submissionSerial']]
        assert item['phase']==r['phase'] and item['station']==int(r['station'])
        assert identity['simulationTick']==int(r['tick'])
        assert e['pipeline']['instrumentation']=='shipping' and e['pipeline']['dielectricQuality']=='high'
        assert e['pipeline']['executionBackend']==receipt['backend']==manifest['executionBackend']
        assert e['pipeline']['activeSha256']==r['shader_sha256']
        assert not e['dielectric']['compiled'] and not e['dielectric']['available'] and e['dielectric']['counters'] is None
        assert e['dielectric']['readCount']==e['dielectric']['resetCount']==0
        assert e['presentation']['outcome']=='presented'
        assert e['presentation']['lastSuccessfulPresentSubmissionSerial']==identity['submissionSerial']
        assert e['resources']['deviceLocalBytes']==int(r['device_local_bytes'])
        assert e['resources']['hostVisibleBytes']==int(r['host_visible_bytes'])
        assert e['gpu']['status']==r['gpu_status']
        for k in stage_columns:
            if k in ('simulation_step_cpu_ns','present_return_interval_ns'): continue  # external owner, deliberately separate
            assert abs(int(r[k])/1e6-e['stages'][k[:-3]])<=.000001
        settings.append((e['shadowQuality']['mode'],e['pipeline']['waterQuality'],
                         e['fireQuality']['quality'],e['actualUploadedDustQuality']))
    assert len(set(settings))==1
    warm=[r for r in rows if float(r['elapsed_s'])>=run['warmupSeconds']
          and (legacy or r['present_interval_available']=='1')]
    assert warm and all(r['cpu_eligible']=='1' for r in warm)
    summary=dict(scope=run['scope'],backend=receipt['backend'],executableSha256=receipt['executableSha256'],
                 hardware=manifest['device'],run=run,settings=list(settings[0]),
                 warmSamples=len(warm),gpuStatusCounts={},shaderIdentities=sorted({r['shader_sha256'] for r in rows}),
                 resourcePeaks={k:max(int(r[k]) for r in rows) for k in ('device_local_bytes','host_visible_bytes')},
                 cpuMilliseconds={k:stats([int(r[k])/1e6 for r in warm]) for k in stage_columns},phases={})
    for r in warm: summary['gpuStatusCounts'][r['gpu_status']]=summary['gpuStatusCounts'].get(r['gpu_status'],0)+1
    summary['gpuMilliseconds']=stats([int(r['gpu_ns'])/1e6 for r in warm if r['gpu_status']=='valid' and int(r['gpu_ns'])>0])
    intervals=[int(r['present_return_interval_ns']) for r in warm]
    interval_key='postFrameObservationIntervalMilliseconds' if legacy else 'presentReturnIntervalMilliseconds'
    throughput_key='postFrameObservationThroughputHz' if legacy else 'presentReturnThroughputHz'
    summary['verifiedIntervalMeaning']=('legacy post-RenderFrame observation intervals; NOT actual present API return timestamps'
        if legacy else run['intervalMeaning'])
    summary[interval_key]=stats([v/1e6 for v in intervals])
    summary[throughput_key]=len(intervals)/(sum(intervals)/1e9)
    if run['schema']==4:
        summary['hostMilliseconds']={k:stats([int(r[k])/1e6 for r in warm]) for k in HOST_FIELDS[:-1]}
    summary['scanoutTimingMeasured']=False
    if streaming:
        transitions=[]
        for previous, current in zip(rows, rows[1:]):
            before, after = int(previous['static_saved_owner_bytes']), int(current['static_saved_owner_bytes'])
            if before == after: continue
            centre=float(current['elapsed_s'])
            window=[r for r in rows if abs(float(r['elapsed_s'])-centre)<=1.0 and r['present_interval_available']=='1']
            transitions.append(dict(kind='unload' if after>before else 'restore',
                elapsedSeconds=centre,submissionSerial=int(current['submission_serial']),
                phase=current['phase'],savedBytesBefore=before,savedBytesAfter=after,
                deviceLocalBytesBefore=int(previous['device_local_bytes']),
                deviceLocalBytesAfter=int(current['device_local_bytes']),
                windowRadiusSeconds=1.0,
                presentReturnMilliseconds=stats([int(r['present_return_interval_ns'])/1e6 for r in window]),
                peakDeviceLocalBytes=max(int(r['device_local_bytes']) for r in window)))
        assert sum(t['kind']=='unload' for t in transitions)==run['staticDetailUnloads']
        assert sum(t['kind']=='restore' for t in transitions)==run['staticDetailRestores']
        summary['staticDetailTransitions']=transitions
    load_path=directory/'system-load.csv'
    if not legacy:
        load=list(csv.DictReader(load_path.open()))
        assert load and load[0]['available']=='0'
        valid=[float(r['total_cpu_busy_percent']) for r in load if r['available']=='1']
        assert all(0<=v<=100 for v in valid)
        summary['machineCpuBusyPercent']=stats(valid)
        summary['machineLoadScope']='aggregate machine CPU time sampled about once per second; no process or per-stall attribution'
    summary['foregroundFrameCounts']={str(value):sum(r.get('foreground_client')==str(value) for r in rows) for value in (0,1)}
    summary['largestIntervals']=[{k:r[k] for k in ('elapsed_s','present_return_interval_ns','phase','station','tick',
        'gpu_ns','simulation_step_cpu_ns','imageAcquireCpuMs_ns','frameFenceWaitCpuMs_ns','wholeFrameCycleCpuMs_ns')}
        for r in sorted(rows,key=lambda r:int(r['present_return_interval_ns']),reverse=True)[:12]]
    if run['schema']==4:
        for item,row in zip(summary['largestIntervals'],sorted(rows,key=lambda r:int(r['present_return_interval_ns']),reverse=True)):
            item['hostPhasesNanoseconds']=verify_host_phases(row)
    for phase in dict.fromkeys(r['phase'] for r in warm):
        p=[r for r in warm if r['phase']==phase]
        summary['phases'][phase]=dict(samples=len(p),gpuMilliseconds=stats([int(r['gpu_ns'])/1e6 for r in p
                                  if r['gpu_status']=='valid' and int(r['gpu_ns'])>0]),
                                  cpuCycleMilliseconds=stats([int(r['wholeFrameCycleCpuMs_ns'])/1e6 for r in p]),
                                  simulationStepMilliseconds=stats([int(r['simulation_step_cpu_ns'])/1e6 for r in p]))
        intervals=[int(r['present_return_interval_ns']) for r in p]
        summary['phases'][phase][interval_key]=stats([v/1e6 for v in intervals])
        summary['phases'][phase]['resources']={k:{'minimum':min(int(r[k]) for r in p),
            'maximum':max(int(r[k]) for r in p)} for k in ('device_local_bytes','host_visible_bytes')}
    assert pathlib.Path(analysis_name).name==analysis_name and analysis_name.endswith('.json')
    target=directory/analysis_name
    if target.exists():
        assert json.loads(target.read_text())==summary,'stored analysis differs from verified arithmetic'
    else:
        assert not sealed,'sealed packet must already contain its analysis'
        target.write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:summary[k] for k in ('backend','warmSamples','settings','gpuMilliseconds','resourcePeaks')}))
    print('CPU stages (median/p95 ms):',json.dumps({k:(v['median'],v['p95']) for k,v in summary['cpuMilliseconds'].items()}))
    print('phase GPU (median/p95 ms):',json.dumps({k:(v['gpuMilliseconds'].get('median'),v['gpuMilliseconds'].get('p95')) for k,v in summary['phases'].items()}))
    print(interval_key,':',json.dumps(summary[interval_key]), 'throughput Hz:',summary[throughput_key])

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Verify completed exact-identity Release route evidence; preserve legacy raw measurements.')
    parser.add_argument('directories',nargs='+')
    parser.add_argument('--analysis-name',default='analysis-v3.json')
    args=parser.parse_args()
    for name in args.directories: analyze(name,args.analysis_name)
