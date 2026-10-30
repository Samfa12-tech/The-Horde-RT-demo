"""Summarize exact native artifacts; images remain ignored diagnostic evidence."""
from pathlib import Path
import csv, hashlib, json, math, statistics
from PIL import Image, ImageChops, ImageDraw, ImageEnhance

HERE = Path(__file__).resolve().parent

def load(path): return json.loads(path.read_text(encoding='utf-8-sig'))
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def stats(values):
    ordered = sorted(values)
    return {'count':len(values), 'median':statistics.median(values),
        'p95':ordered[math.ceil(.95*len(values))-1], 'minimum':ordered[0], 'maximum':ordered[-1]}
def difference(a, b):
    diff = ImageChops.difference(a,b)
    def area(box):
        pixels = list(diff.crop(box).get_flattened_data())
        return {'pixels':len(pixels), 'changed':sum(max(v)>0 for v in pixels),
                'over8':sum(max(v)>8 for v in pixels), 'over32':sum(max(v)>32 for v in pixels),
                'meanAbsoluteRgb':sum(sum(v) for v in pixels)/(3*len(pixels))}
    return {'bbox':diff.getbbox(), 'full':area((0,0,960,540)),
            'upper300':area((0,0,960,300)), 'groundAndHeldProps':area((0,300,960,540))}

rows=[]; summary=[]; contact=Image.new('RGB',(1440,6*302),(18,18,18)); draw=ImageDraw.Draw(contact)
for run, label in enumerate([d+'-'+b for b in ['pipeline','query'] for d in ['near','mid','far']]):
    root=HERE/label; manifest=load(root/'capture-manifest.json'); process=load(HERE/(label+'.process.json'))
    assert process['exitCode']==0 and process['settingsUnchanged'] and process['foregroundApiSamplesUnchanged']
    record={'label':label,'executable':process['executable'],'seconds':process['seconds'],
            'camera':manifest['captures'][0]['camera'],'device':manifest['device'],
            'presentation':manifest['presentation'],'shaderPair':manifest['selectedRtPipelineBundle'],
            'phases':{},'rawArtifacts':{}}
    camera=record['camera']; record['treeBaseDistanceM']=math.hypot(camera['x']+22.298101214,camera['z']-17.699113131)
    serials=[]; phase_resources=[]
    for phase in ['before','candidate','restored']:
        path=root/('static-timing-'+phase+'.json'); raw=load(path)
        assert raw['complete'] and raw['sampleCount']==60 and raw['warmFrames']==12
        gpu=[]; cpu=[]; whole=[]; identities=[]; resources=[]
        for row in raw['samples']:
            f=row['completedFrame']; identity=f['identity']; g=f['gpu']; c=f['stages']
            serial=identity['submissionSerial']; serials.append(serial); identities.append(identity)
            assert f['benchmarkEligible'] and f['presentation']['finalIdleCompletion']
            assert f['presentation']['presented'] and f['presentation']['lastSuccessfulPresentSubmissionSerial']==serial
            assert g['status']=='valid' and g['available'] and g['completedSubmissionSerial']==serial
            assert all(f['dispatch'].values()) and f['actualUploadedMistEnabled'] is True
            gpu.append(g['wholeRtGpuMs']); cpu.append(row['renderCallCpuMs']); whole.append(c['wholeFrameCycleCpuMs'])
            resources.append(f['resources'])
            rows.append([label,phase,raw['staticGeometryGeneration'],identity['sceneEpoch'],identity['measurementGeneration'],
                identity['simulationTick'],serial,identity['completionSerial'],g['wholeRtGpuMs'],row['renderCallCpuMs'],
                c['wholeFrameCycleCpuMs'],c['frameFenceWaitCpuMs'],c['queueSubmitCpuMs'],c['presentCallCpuMs']])
        assert len(raw['samples'])==60 and all(r==resources[0] for r in resources)
        phase_resources.append(resources[0])
        record['phases'][phase]={'staticGeneration':raw['staticGeometryGeneration'],'firstIdentity':identities[0],
            'lastIdentity':identities[-1],'gpuMs':stats(gpu),'renderCallCpuMs':stats(cpu),
            'wholeFrameCycleCpuMs':stats(whole),'resources':resources[0]}
        record['rawArtifacts'][path.name]={'bytes':path.stat().st_size,'sha256':digest(path)}
    assert serials==sorted(set(serials)) and len(serials)==180
    assert phase_resources[0]==phase_resources[2]
    baseline=record['phases']['before']['gpuMs']['median']; candidate=record['phases']['candidate']['gpuMs']['median']
    restored=record['phases']['restored']['gpuMs']['median']
    record['gpuMedianPercentChanges']={'candidateVsBefore':100*(candidate/baseline-1),
        'candidateVsRestored':100*(candidate/restored-1),'restoredVsBefore':100*(restored/baseline-1)}
    record['gpuMedianBudgetBands']={phase:{'within16_667Ms':values['gpuMs']['median']<=16.667,
        'within20Ms':values['gpuMs']['median']<=20,'within33_333Ms':values['gpuMs']['median']<=33.333}
        for phase,values in record['phases'].items()}
    record['cutovers']=load(root/'static-zone-cutover.json')['hops']
    assert record['cutovers'][0]['omittedTriangleCount']==768 and record['cutovers'][1]['completeContributors']
    a,b,c=[Image.open(root/name).convert('RGB') for name in ['cutover-00-before.png','cutover-01-selected.png','cutover-02-restored.png']]
    record['imageDifferences']={'candidateVsBefore':difference(a,b),'restoredVsBefore':difference(a,c)}
    for name in ['cutover-00-before.png','cutover-01-selected.png','cutover-02-restored.png','static-world-source.json','static-zone-cutover.json','capture-manifest.json']:
        p=root/name; record['rawArtifacts'][name]={'bytes':p.stat().st_size,'sha256':digest(p)}
    for column,(name,image) in enumerate([('original',a),('candidate',b),('abs RGB difference x8',ImageEnhance.Brightness(ImageChops.difference(a,b)).enhance(8))]):
        x,y=column*480,run*302; draw.text((x+8,y+4),label+' / '+name,fill=(240,240,240))
        contact.paste(image.resize((480,270)),(x,y+28))
    summary.append(record)
assert len({r['executable']['sha256'] for r in summary})==1
with (HERE/'samples.csv').open('w',newline='',encoding='utf-8') as stream:
    writer=csv.writer(stream);writer.writerow(['viewBackend','phase','staticGeneration','sceneEpoch','measurementGeneration',
        'simulationTick','submissionSerial','completionSerial','gpuRtMs','renderCallCpuMs','wholeFrameCycleCpuMs',
        'frameFenceWaitCpuMs','queueSubmitCpuMs','presentCallCpuMs']);writer.writerows(rows)
result={'schema':1,'scope':'Debug 12 warm +60 unique completed serialized RT submissions per phase; no pacing/sustained/mobile acceptance',
    'sampleCount':len(rows),'percentile':'nearest rank p95; median average middle pair',
    'imageDifferenceBoundary':'upper300 is image region, not an object mask; ground region includes held props; inspect A/B/A controls',
    'runs':summary}
(HERE/'comparison-summary.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
contact.save(HERE/'native-comparison-contact-sheet.png')
for r in summary:
    print(r['label'],{k:round(v['gpuMs']['median'],6) for k,v in r['phases'].items()},
          r['gpuMedianPercentChanges'],r['imageDifferences']['restoredVsBefore']['full']['changed'])
