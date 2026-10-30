from pathlib import Path
import hashlib,json,statistics
root=Path.cwd().resolve()
labels=('baseline-r1','candidate-r1','candidate-r2')
def identity(p):return {'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
runs={};native={};images={}
for label in labels:
    fixture=root/('reports/horde-campaign-fixtures-kit-turn-20261020-'+label)
    logs=root/('reports/kit-turn-20261020-'+label)
    n=json.loads((fixture/'windows-lookout-validation.json').read_text());native[label]=n
    frames=n['kitMotionInspection']['frames']
    images[label]={p.name:identity(p) for p in fixture.glob('*.png')}
    resources={json.dumps(f['rtEvidence']['resources'],sort_keys=True) for f in frames}
    runs[label]={'process':json.loads((logs/'hidden-rt.json').read_text()),
      'count':n['count'],'failures':n['failures'],'complete':n['complete'],'renderCalls':n['renderCalls'],
      'failedChecks':[c for c in n['checks'] if not c['pass']],
      'foregroundSamples':n['foregroundSampleCount'],'ownerShown':n['ownerShown'],
      'ownerObservedForeground':n['ownerObservedForeground'],
      'foregroundContinuous':n['foregroundContinuous'],
      'resources':[json.loads(r) for r in sorted(resources)],
      'custody':json.loads((logs/'custody.json').read_text()),
      'frames':[{'stage':f['stage'],**{k:f[k] for k in ('root','yaw','walking','idleSeconds','walkPhase','walkWeight')},
        'leftTurnStep':f.get('leftTurnStep',0),'rightTurnStep':f.get('rightTurnStep',0),
        'identity':f['rtEvidence']['identity'],'dispatch':f['rtEvidence']['dispatch'],
        'presentation':f['rtEvidence']['presentation'],
        'characterSkinCpuMs':f['rtEvidence']['stages']['characterSkinCpuMs'],
        'gpuMs':f['rtEvidence']['gpu']['wholeRtGpuMs']} for f in frames]}
base={f['stage']:f for f in native['baseline-r1']['kitMotionInspection']['frames']}
candidate={f['stage']:f for f in native['candidate-r2']['kitMotionInspection']['frames']}
assert all(all(base[s][k]==candidate[s][k] for k in ('root','yaw','walking','idleSeconds','walkPhase','walkWeight')) for s in base)
matched={}
for label in ('baseline-r1','candidate-r2'):
    fs=[f for f in runs[label]['frames'] if f['stage'] in base]
    matched[label]={k:statistics.median(f[k] for f in fs) for k in ('characterSkinCpuMs','gpuMs')}
summary={'evidenceClass':'matched seeded fixed-step route snapshots in hidden native RT, not live input or sustained performance',
 'sameRootYawWalkingClocksPhaseWeightOnAll20MatchedStages':True,
 'matchedMediansShortDebugReadbacksOnly':matched,
 'baselineCandidatePngIdentical':[n for n in images['baseline-r1'] if images['baseline-r1'][n]==images['candidate-r2'].get(n)],
 'candidateR1R2PngIdentical':all(v==images['candidate-r2'].get(n) for n,v in images['candidate-r1'].items()),
 'images':images,'runs':runs}
out=root/'reports/kit-turn-20261020-comparison.json'
assert not out.exists()
out.write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k not in ('runs','images')},indent=2))
