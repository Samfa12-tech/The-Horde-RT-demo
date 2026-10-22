from pathlib import Path
import hashlib,json,math,statistics,subprocess
root=Path.cwd();base=root/'reports/skin-attribution-20261022'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def stats(v):
    assert len(v)==240 and all(type(x) is int and x>=0 for x in v)
    return {'count':len(v),'medianMs':statistics.median(v)/1e6,'meanMs':statistics.mean(v)/1e6,'minimumMs':min(v)/1e6,'maximumMs':max(v)/1e6,'p95NearestRankMs':sorted(v)[math.ceil(.95*len(v))-1]/1e6}
runs={}
for label,name in [('A','a-baseline.jsonl'),('B','b-candidate.jsonl'),('C','c-control.jsonl')]:
    source=base/name;rows=[json.loads(v) for v in source.read_text(encoding='utf-8-sig').splitlines()]
    assert len(rows)==3
    runs[label]={'rawFile':str(source.relative_to(root)),'sha256':sha(source),'scenarios':[]}
    for row in rows:
        assert row['samples']==240 and row['warmup']==30 and row['verticesPerPose']==26945 and row['jointsPerPose']==24
        s={k:v for k,v in row.items() if not isinstance(v,list)}
        s['statistics']={k:stats(row[k]) for k in ('animationNs','vertexNs','copyNs','poseNs','unobservedPoseNs')}
        runs[label]['scenarios'].append(s)
comparisons=[]
for a,b,c in zip(*(runs[x]['scenarios'] for x in 'ABC')):
    assert a['scenario']==b['scenario']==c['scenario']
    same=a['outputFnv1a64']==b['outputFnv1a64']==c['outputFnv1a64']
    def median(row,key):return row['statistics'][key]['medianMs']
    comparisons.append({'scenario':a['scenario'],'outputDigestsMatch':same,
        'vertexReductionPercentVsA':100*(1-median(b,'vertexNs')/median(a,'vertexNs')),
        'vertexReductionPercentVsC':100*(1-median(b,'vertexNs')/median(c,'vertexNs')),
        'poseReductionPercentVsC':100*(1-median(b,'poseNs')/median(c,'poseNs')),
        'closingControlVertexDriftPercent':100*(median(c,'vertexNs')/median(a,'vertexNs')-1),
        'closingControlPoseDriftPercent':100*(median(c,'poseNs')/median(a,'poseNs')-1)})
goal=read(base/'goal.json');candidate=runs['B']['scenarios'][0]['statistics']['vertexNs']['medianMs']*1e6
images=[]
for label,checkpoint in [('rt','203-chapter-forest-lookout'),('kit','207-chapter-kit-reunion')]:
    a=base/(label+'-baseline')/(checkpoint+'.png');b=base/(label+'-candidate')/(checkpoint+'.png')
    ma=read(a.parent/'capture-manifest.json');mb=read(b.parent/'capture-manifest.json')
    assert ma['complete'] and mb['complete'] and ma['presentation']==mb['presentation']
    assert ma['captures'][0]['camera']==mb['captures'][0]['camera']
    images.append({'checkpoint':checkpoint,'baseline':str(a.relative_to(root)),'candidate':str(b.relative_to(root)),
        'baselineSha256':sha(a),'candidateSha256':sha(b),'byteIdentical':a.read_bytes()==b.read_bytes(),
        'presentation':ma['presentation'],'gpu':ma['device']['gpuName'],'frozenDiagnosticScope':True})
result={'schema':1,'scope':'CPU asset microbenchmark; no frame-FPS, GPU-AS or Android gain claim',
    'sourceBase':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
    'goal':goal,'goalResult':{'targetMet':candidate<=goal['target'],'candidateMedianNs':candidate,'state':'exhausted','iterations':1},
    'clockThermalLimitations':'No CPU frequency or temperature capture. No owned build/capture during A/B/C. External scheduling not isolated; paired controls and batch thread CPU retained.',
    'runs':runs,'comparisons':comparisons,'images':images,
    'playerFixture':read(base/'player-clean.json'),
    'binaryHashes':{str(p.relative_to(root)):sha(p) for p in sorted((base/'binaries').glob('*.exe'))},
    'phoneOperations':0}
(base/'summary.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'goalResult':result['goalResult'],'comparisons':comparisons,'imageParity':[x['byteIdentical'] for x in images]},indent=2))
