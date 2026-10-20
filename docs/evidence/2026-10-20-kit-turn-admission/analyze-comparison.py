from pathlib import Path
import csv,hashlib,json,math,statistics
root=Path.cwd().resolve();out=root/'reports/kit-turn-admission-20261020-r1'
assert not (out/'analysis.json').exists()
order=json.loads((out/'plan.json').read_text())['order'];runs={};flat=[];thermals=[]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def stats(xs):
    xs=sorted(xs);return {'count':len(xs),'min':xs[0],'median':statistics.median(xs),'p95':xs[math.ceil(.95*len(xs))-1],'max':xs[-1]}
for label in order:
    receipt=json.loads((out/(label+'-receipt.json')).read_text())
    directory=root/('reports/horde-campaign-fixtures-kit-turn-admission-20261020-'+label)
    native=json.loads((directory/'windows-lookout-validation.json').read_text())
    assert receipt['exitCode']==0 and not receipt['pending'] and receipt['fixtureUnchanged'] and receipt['ownerUnchanged']
    assert native['complete'] and native['failures']==0 and native['renderCalls']<=180 and not native['ownerShown'] and not native['ownerObservedForeground']
    samples=native['kitTimingInspection']['samples'];assert len(samples)==120
    resources=set();serials=set();groups={}
    for sample in samples:
        e=sample['rtEvidence'];serial=e['identity']['submissionSerial']
        assert serial not in serials;serials.add(serial)
        assert e['presentation']['presented'] and e['presentation']['lastSuccessfulPresentSubmissionSerial']==serial
        assert e['gpu']['status']=='valid' and e['gpu']['available'] and e['gpu']['completedSubmissionSerial']==serial and e['gpu']['errorCount']==0
        assert e['dispatch']['rtDispatchRecorded'] and e['dispatch']['swapchainCopyRecorded']
        resources.add(json.dumps(e['resources'],sort_keys=True))
        flat.append({'run':label,'group':sample['group'],'sample':sample['sample'],'pose':sample['pose'],'utcUnixMs':sample['utcUnixMs'],
          'gpuMs':e['gpu']['wholeRtGpuMs'],'characterSkinCpuMs':e['stages']['characterSkinCpuMs'],
          'wholeFrameCycleCpuMs':e['stages']['wholeFrameCycleCpuMs'],'submissionSerial':serial})
    for group in ('walk','turn'):
        rows=[s for s in flat if s['run']==label and s['group']==group];assert len(rows)==60
        begin=min(s['utcUnixMs'] for s in rows);end=max(s['utcUnixMs'] for s in rows)
        thermal=[t for t in receipt['thermalSamples'] if begin<=t['utcUnixMs']<=end]
        assert len(thermal)>=3
        thermals.extend({'run':label,'group':group,**t} for t in thermal)
        groups[group]={k:stats([s[k] for s in rows]) for k in ('gpuMs','characterSkinCpuMs','wholeFrameCycleCpuMs')}
        groups[group]['thermal']={k:stats([float(t[k]) for t in thermal]) for k in ('temperature.gpu','clocks.gr','clocks.mem','power.draw','utilization.gpu')}
        groups[group]['thermal']['acStable']=all(t['powerStatus'] and t['powerStatus']['ac']==1 for t in thermal)
    runs[label]={'groups':groups,'checks':native['count'],'failures':native['failures'],'frames':native['renderCalls'],
      'resources':[json.loads(r) for r in resources],'images':{p.name:digest(p) for p in directory.glob('*.png')},
      'nativeReceipt':{'path':str((directory/'windows-lookout-validation.json').relative_to(root)),'sha256':digest(directory/'windows-lookout-validation.json')},
      'process':receipt}
assert all(len(r['resources'])==1 for r in runs.values())
sameResources=all(r['resources']==runs[order[0]]['resources'] for r in runs.values())
comparisons={};costPass=True;thermalPass=True
for group in ('walk','turn'):
    candidate=runs['candidate-b']['groups'][group];comparisons[group]={}
    for metric in ('gpuMs','characterSkinCpuMs','wholeFrameCycleCpuMs'):
        comparisons[group][metric]={}
        for stat in ('median','p95'):
            controls=[runs[c]['groups'][group][metric][stat] for c in ('control-a1','control-a2')]
            baseline=min(controls);ratio=candidate[metric][stat]/baseline
            passed=ratio<=1.15
            costPass=costPass and passed
            comparisons[group][metric][stat]={'controlA1':controls[0],'candidate':candidate[metric][stat],'controlA2':controls[1],
                'ratioToLowerControl':ratio,'percentToLowerControl':(ratio-1)*100,'within15Percent':passed}
    ct=candidate['thermal'];thermalPass=thermalPass and ct['acStable']
    for control in ('control-a1','control-a2'):
        t=runs[control]['groups'][group]['thermal']
        thermalPass=thermalPass and t['acStable'] and abs(ct['temperature.gpu']['median']-t['temperature.gpu']['median'])<=3
        thermalPass=thermalPass and abs(ct['clocks.gr']['median']/t['clocks.gr']['median']-1)<=.10
controlsEqual=runs['control-a1']['images']==runs['control-a2']['images']
walkEqual=all(runs[c]['images']['windows-kit-timing-walk.png']==runs['candidate-b']['images']['windows-kit-timing-walk.png'] for c in ('control-a1','control-a2'))
admit=costPass and thermalPass and sameResources and controlsEqual and walkEqual
result={'evidenceClass':'one bounded serialized Debug desktop A/B/A component comparison; no phone or sustained parity',
 'admission':admit,'costWithin15Percent':costPass,'thermalComparable':thermalPass,'resourcesEqual':sameResources,
 'controlPngsAllIdentical':controlsEqual,'timedWalkingPngIdentical':walkEqual,
 'comparisons':comparisons,'runs':runs,
 'limitations':['Both variants execute the same transient simulation oscillator; its incremental CPU cost is not isolated.',
 'Repeated immutable-pose profiling serializes GPU completion; it is not ordinary frame pacing or a sustained warm game run.',
 'GPU telemetry samples are coarse; CPU temperature and per-frame GPU clock were unavailable.',
 'Debug-only/native RT desktop evidence does not establish phone parity or close the prior campaign Debug timeout.']}
(out/'analysis.json').write_text(json.dumps(result,indent=2)+'\n')
with (out/'samples.csv').open('w',newline='') as f:
    w=csv.DictWriter(f,fieldnames=flat[0]);w.writeheader();w.writerows(flat)
with (out/'thermal.json').open('w') as f:json.dump(thermals,f,indent=2)
print(json.dumps({k:result[k] for k in result if k!='runs'},indent=2))
