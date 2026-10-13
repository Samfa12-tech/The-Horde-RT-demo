import csv, json, math, pathlib, statistics, sys, hashlib

def stats(values):
    if not values: return {'count': 0, 'available': False}
    v = sorted(values)
    def rank(p): return v[max(0, math.ceil(len(v)*p)-1)]
    return dict(count=len(v), mean=statistics.fmean(v), median=statistics.median(v),
                p95=rank(.95), p99=rank(.99), maximum=v[-1], minimum=v[0])

def analyze(name):
    root=pathlib.Path(name); sealed=(root/'completed-frames.csv').exists()
    directory=root if sealed else root/'shipping-route'
    run=json.loads((directory/'run.json').read_text())
    receipt=json.loads((root/'launch.candidate.json' if sealed else pathlib.Path(name+'.candidate.json')).read_text(encoding='utf-8-sig'))
    manifest=json.loads((root/'capture-manifest.json').read_text())
    rows=list(csv.DictReader((directory/'completed-frames.csv').open()))
    identities=[json.loads(line) for line in (directory/'identities.jsonl').read_text().splitlines()]
    assert run['complete'] and run['durationSeconds']>=60 and run['durationSeconds']<=150
    assert len(rows)==run['completedSamples']==run['renderedFrames']
    assert run['normalStaticRegionResidencySavingBytes']==0
    by_serial={int(r['submission_serial']):r for r in rows}
    assert len(by_serial)==len(rows)
    serials=list(by_serial); assert serials==list(range(serials[0],serials[-1]+1))
    assert len({(r['scene_epoch'],r['generation']) for r in rows})==1
    assert all(float(a['elapsed_s'])<=float(b['elapsed_s']) and int(a['tick'])<=int(b['tick'])
               for a,b in zip(rows,rows[1:]))
    stage_columns=[k for k in rows[0] if k.endswith('_ns') and k!='gpu_ns']
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
            if k=='simulation_step_cpu_ns': continue  # external probe owner, deliberately separate
            assert abs(int(r[k])/1e6-e['stages'][k[:-3]])<=.000001
        settings.append((e['shadowQuality']['mode'],e['pipeline']['waterQuality'],
                         e['fireQuality']['quality'],e['actualUploadedDustQuality']))
    assert len(set(settings))==1
    warm=[r for r in rows if float(r['elapsed_s'])>=run['warmupSeconds']]
    assert warm and all(r['cpu_eligible']=='1' for r in warm)
    summary=dict(scope=run['scope'],backend=receipt['backend'],executableSha256=receipt['executableSha256'],
                 hardware=manifest['device'],run=run,settings=list(settings[0]),
                 warmSamples=len(warm),gpuStatusCounts={},shaderIdentities=sorted({r['shader_sha256'] for r in rows}),
                 resourcePeaks={k:max(int(r[k]) for r in rows) for k in ('device_local_bytes','host_visible_bytes')},
                 cpuMilliseconds={k:stats([int(r[k])/1e6 for r in warm]) for k in stage_columns},phases={})
    for r in warm: summary['gpuStatusCounts'][r['gpu_status']]=summary['gpuStatusCounts'].get(r['gpu_status'],0)+1
    summary['gpuMilliseconds']=stats([int(r['gpu_ns'])/1e6 for r in warm if r['gpu_status']=='valid' and int(r['gpu_ns'])>0])
    for phase in dict.fromkeys(r['phase'] for r in warm):
        p=[r for r in warm if r['phase']==phase]
        summary['phases'][phase]=dict(samples=len(p),gpuMilliseconds=stats([int(r['gpu_ns'])/1e6 for r in p
                                  if r['gpu_status']=='valid' and int(r['gpu_ns'])>0]),
                                  cpuCycleMilliseconds=stats([int(r['wholeFrameCycleCpuMs_ns'])/1e6 for r in p]),
                                  simulationStepMilliseconds=stats([int(r['simulation_step_cpu_ns'])/1e6 for r in p]))
    target=directory/'analysis.json'
    if target.exists():
        assert json.loads(target.read_text())==summary,'stored analysis differs from verified arithmetic'
    else:
        assert not sealed,'sealed packet must already contain its analysis'
        target.write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:summary[k] for k in ('backend','warmSamples','settings','gpuMilliseconds','resourcePeaks')}))
    print('CPU stages (median/p95 ms):',json.dumps({k:(v['median'],v['p95']) for k,v in summary['cpuMilliseconds'].items()}))
    print('phase GPU (median/p95 ms):',json.dumps({k:(v['gpuMilliseconds'].get('median'),v['gpuMilliseconds'].get('p95')) for k,v in summary['phases'].items()}))

for name in sys.argv[1:]: analyze(name)
