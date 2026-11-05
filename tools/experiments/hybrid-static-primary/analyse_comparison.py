"""Validate matched native captures and create a local, shareable comparison report."""
import argparse, hashlib, html, json, math, pathlib, shutil, statistics, zipfile
from PIL import Image, ImageChops, ImageStat

p = argparse.ArgumentParser()
p.add_argument('evidence', type=pathlib.Path)
p.add_argument('output', type=pathlib.Path)
p.add_argument('--pattern', default='audited-*')
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
def read(p): return json.loads(p.read_text(encoding='utf-8-sig'))
def percentile(xs, q):
    xs=sorted(xs); pos=(len(xs)-1)*q; lo=int(pos); hi=math.ceil(pos)
    return xs[lo]+(xs[hi]-xs[lo])*(pos-lo)
def stats(xs): return {'median': statistics.median(xs), 'p95': percentile(xs,.95), 'min': min(xs), 'max': max(xs)}
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
runs=[]; captures={}; validation=[]
paths=sorted(a.evidence.glob(a.pattern))
assert len(paths)==14, f'Expected 12 matched runs + 2 validation runs, found {len(paths)}'
exe_hashes=set()
for path in paths:
    launch=read(path/'launch.json'); m=read(path/'capture-manifest.json')
    assert launch['exitCode']==0 and m['complete'] and not m['error'], path
    exe_hashes.add(launch['executableSha256'])
    c=m['captures'][0]; f=c['completedFrame']; hybrid=launch['hybridRequested']
    exp=m['renderingExperiment']
    assert exp['requested']==hybrid and exp['submitted']==hybrid
    assert exp['rasterDrawsRecorded']>0 if hybrid else exp['rasterDrawsRecorded']==0
    assert c['honestlyPresentedRtFrame'] and f['presentation']['presented'] and f['presentation']['finalIdleCompletion']
    assert f['identity']['submissionSerial']==f['identity']['completionSerial']==f['gpu']['completedSubmissionSerial']
    rows=[json.loads(s) for s in next(path.glob('*frames.jsonl')).read_text().splitlines()]
    assert len(rows)==180 and [r['captureFrameIndex'] for r in rows]==list(range(1,181)), path
    serials=[]
    for row in rows:
        r=row['completedFrame']; ident=r['identity']; serials.append(ident['submissionSerial'])
        assert ident['submissionSerial']==ident['completionSerial']==r['gpu']['completedSubmissionSerial']
        assert r['gpu']['status']=='valid' and r['gpu']['wholeRtGpuMs']>0 and r['gpu']['errorCount']==0
        assert r['presentation']['presented'] and r['stages']['status']=='valid'
        assert ('hybrid_static21' in r['pipeline']['activeKey'])==hybrid
    assert len(set(serials))==180 and serials==sorted(serials)
    steady=[r['completedFrame'] for r in rows[60:]]
    settings={k:f[k] for k in ('submittedDimensions','shadowQuality','fireQuality','actualUploadedDustQuality','actualUploadedMistEnabled','fireLighting')}
    settings['camera']=c['camera']; settings['state']=c['state']
    settings['pipeline']={k:f['pipeline'][k] for k in ('executionBackend','instrumentation','dielectricQuality','waterQuality','activeStrategy')}
    settings['fixedAnimationTimeSeconds']=m['fixedAnimationTimeSeconds']
    item={'name':path.name,'checkpoint':launch['checkpoint'],'hybrid':hybrid,'samples':len(steady),
          'gpuMs':stats([r['gpu']['wholeRtGpuMs'] for r in steady]),
          'cpuCycleMs':stats([r['stages']['wholeFrameCycleCpuMs'] for r in steady]),
          'cpuStagesMedianMs':{k:statistics.median(r['stages'][k] for r in steady) for k in f['stages'] if k.endswith('CpuMs')},
          'settings':settings,'rasterDraws':exp['rasterDrawsRecorded'],'rasterTriangles':exp['rasterTrianglesRecorded'],
          'resources':f['resources'],'imageSha256':sha(next(path.glob('*.png')))}
    if launch['validationLayerRequested']:
        log=(path/'stderr.log').read_text()
        assert 'Inserted device layer "VK_LAYER_KHRONOS_validation"' in log, path
        assert not any(token in log for token in ('VUID-','Validation Error','Validation Warning')), path
        item['noVuidOrValidationError']=True
        item['loaderWarningsPresent']='WARNING' in log
        validation.append(item)
    else:
        runs.append(item);captures[path.name]=path
assert len(exe_hashes)==1
scenes=[]
for checkpoint in dict.fromkeys(r['checkpoint'] for r in runs):
    group=[r for r in runs if r['checkpoint']==checkpoint]
    assert len(group)==4 and [r['hybrid'] for r in group]==[False,True,True,False]
    assert all(r['settings']==group[0]['settings'] for r in group), checkpoint
    rt=[r for r in group if not r['hybrid']]; hy=[r for r in group if r['hybrid']]
    reference=Image.open(next(captures[rt[0]['name']].glob('*.png'))).convert('RGB')
    candidate=Image.open(next(captures[hy[0]['name']].glob('*.png'))).convert('RGB')
    assert reference.size==candidate.size==(960,540)
    diff=ImageChops.difference(reference,candidate)
    channels=diff.split(); max_channel=ImageChops.lighter(ImageChops.lighter(channels[0],channels[1]),channels[2])
    rt_ms=statistics.mean(r['gpuMs']['median'] for r in rt);hy_ms=statistics.mean(r['gpuMs']['median'] for r in hy)
    scene={'checkpoint':checkpoint,'referenceGpuMedianMeanMs':rt_ms,'hybridGpuMedianMeanMs':hy_ms,
           'gpuChangePercent':(hy_ms/rt_ms-1)*100,
           'referenceGpuRunMediansMs':[r['gpuMs']['median'] for r in rt],
           'hybridGpuRunMediansMs':[r['gpuMs']['median'] for r in hy],
           'meanAbsoluteRgbError255':statistics.mean(ImageStat.Stat(diff).mean),'maxChannelError255':max(hi for lo,hi in diff.getextrema()),
           'pixelsAnyChannelOver8Percent':sum(max_channel.histogram()[9:])/(reference.width*reference.height)*100}
    scenes.append(scene)
    for label,arr in [('rt',reference),('hybrid',candidate)]:
        arr.save(a.output/f'{checkpoint}-{label}.png')
    diff.point(lambda v:min(v*8,255)).save(a.output/f'{checkpoint}-difference8x.png')
result={'method':'ABBA per fixed checkpoint; 180 completed/presented frames per process; first 60 excluded; production optimisation policy matched',
        'executableSha256':next(iter(exe_hashes)), 'device':'NVIDIA GeForce RTX 5050 Laptop GPU',
        'scenes':scenes,'runs':runs,'validationRuns':validation}
(a.output/'comparison-results.json').write_text(json.dumps(result,indent=2)+'\n')
with zipfile.ZipFile(a.output/'matched-raw-evidence.zip','w',zipfile.ZIP_DEFLATED) as z:
    for path in paths:
        for f in path.iterdir():
            if f.suffix!='.obj': z.write(f,f'{path.name}/{f.name}')
rows=''.join(f'<tr><td>{html.escape(s["checkpoint"])}</td><td>{s["referenceGpuMedianMeanMs"]:.3f}</td><td>{s["hybridGpuMedianMeanMs"]:.3f}</td><td>{s["gpuChangePercent"]:+.1f}%</td></tr>' for s in scenes)
details=''.join(f'<tr><td>{html.escape(r["name"])}</td><td>{r["gpuMs"]["median"]:.3f}</td><td>{r["gpuMs"]["p95"]:.3f}</td><td>{r["cpuCycleMs"]["median"]:.3f}</td><td>{1000/r["cpuCycleMs"]["median"]:.1f}</td></tr>' for r in runs)
images=''.join(f'''<section><h2>{html.escape(s['checkpoint'])}</h2><div class="pair"><figure><img src="{s['checkpoint']}-rt.png" alt="Full RT reference"><figcaption>Full RT reference</figcaption></figure><figure><img src="{s['checkpoint']}-hybrid.png" alt="Partial hybrid candidate"><figcaption>Partial static assets hybrid</figcaption></figure></div><details><summary>Difference amplified 8×; error metrics</summary><img src="{s['checkpoint']}-difference8x.png" alt="Amplified absolute RGB difference"><p>Mean channel error {s['meanAbsoluteRgbError255']:.6f}/255; maximum {s['maxChannelError255']}; pixels with any channel difference over 8/255: {s['pixelsAnyChannelOver8Percent']:.6f}%.</p></details></section>''' for s in scenes)
report='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Horde real-assets hybrid experiment</title><style>body{font:16px/1.5 system-ui;background:#111820;color:#e7edf5;max-width:1250px;margin:auto;padding:24px}a{color:#9ad1ff}h1,h2{line-height:1.2}table{border-collapse:collapse;display:block;overflow:auto}td,th{padding:8px 14px;border-bottom:1px solid #455363;text-align:left;font-variant-numeric:tabular-nums}.pair{display:grid;grid-template-columns:1fr 1fr;gap:12px}figure{margin:0}img{max-width:100%;height:auto}section{margin:36px 0}summary,button{cursor:pointer}aside{padding:16px;background:#283541}@media(max-width:700px){.pair{grid-template-columns:1fr}body{padding:12px}}</style>
<h1>Horde: actual assets, partial hybrid experiment</h1><p>Local research prototype · 5 November 2026 · RTX 5050 Laptop GPU · 960 × 540 · Shipping instrumentation / Mobile dielectric quality.</p>
<aside>This is a playable native Horde experiment. Only immutable opaque static asset owner 21 uses raster primary visibility. World geometry, actors, water and other primary hits still use RT; production materials, lighting and secondary RT remain. Full RT stays the default. Phone performance is unmeasured.</aside>
<h2>Matched desktop captures</h2><p>Each checkpoint ran Full RT → Hybrid → Hybrid → Full RT in separate processes. Each run has 180 completed/presented frames; the first 60 are excluded. Table values are the mean of each mode’s two run medians. Lower GPU milliseconds is better. This measures the whole GPU command buffer, including AS work, shading and copy; it does not isolate the raster pass.</p>
<table><tr><th>Checkpoint</th><th>Full RT GPU ms</th><th>Hybrid GPU ms</th><th>Hybrid change</th></tr>'''+rows+'''</table><p>The candidate adds a full-resolution 16-byte visibility attachment, depth buffer, extra TLAS and nonindexed vertex pulling. Any slowdown or gain describes this bounded implementation. It does not settle whether a redesigned forward hybrid renderer would help phones.</p>
<p>Both modes use identical cameras, geometry, textures, lighting, effect settings, fixed simulation time, resolution and matched production shader optimisation policies. Full RT is the current production shader/reference path in this experiment executable, not a measurement of the published application binary. Screenshots omit UI overlays.</p>
'''+images+'''<h2>Per-run measurements</h2><p>CPU cycle time includes presentation waiting. The derived cycle rate is a short, frozen capture rate, affected by the desktop display cap; it is not sustained gameplay FPS. GPU reciprocal time is not FPS. GPU clocks, power, thermals and phone behaviour were not measured.</p><table><tr><th>Run</th><th>GPU median ms</th><th>GPU p95 ms</th><th>CPU cycle median ms</th><th>Derived cycle/s</th></tr>'''+details+'''</table><h2>Verification and limits</h2><p>The final candidate completed captures through both RT-pipeline and ray-query-compute backends with the Khronos validation device layer loaded and no VUID/validation errors or warnings. Loader environment/registry warnings remain. Final native Windows build, Android arm64 build and Android lint passed (0 errors, 96 app warnings). Ordinary experiment-off syntax checks passed for the changed renderer and Windows entry point.</p><p>Android install, touch interaction, runtime switches, suspend/resume, rotation, physical resize and sustained gameplay have not been tested. The APK has a separate application ID and leaves the released app installed. Windows uses an isolated experiment save folder. Source admission checks reject unsupported static transmission and buffer replacement. Allocation failure and all mobile limits are not exhaustively tested.</p><p>Use the native app’s Rendering comparison menu on Windows or Graphics page on Android. Keep settings fixed. Existing public benchmark submission is disabled in both experimental modes. Capture frame eligibility fields are diagnostic contracts, not permission to submit these runs as production benchmarks.</p><p><a href="comparison-results.json">Measurements and validation details</a> · <a href="matched-raw-evidence.zip">Raw final captures and logs</a> · <a href="README.md">Build, launch and phone test instructions</a></p><p>Earlier synthetic and pre-optimisation runs are excluded from this report.</p></html>'''
(a.output/'comparison.html').write_text(report,encoding='utf-8')
print(json.dumps({'scenes':scenes,'validationRuns':[r['name'] for r in validation],'executableSha256':next(iter(exe_hashes))},indent=2))
