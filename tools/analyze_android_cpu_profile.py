#!/usr/bin/env python3
"""Join CPU stage CSV rows to the existing strict actual-presentation route."""
from __future__ import annotations
import argparse,csv,io,json,math,statistics
from pathlib import Path
from analyze_android_motion_present_timing import AnalysisError,analyze_bytes,MAX_REPORT_BYTES

CPU=('simulation','skin','player_skin','character_skin','pose_copy','dynamic_upload','fence_wait','queue_submit','whole_frame')
VIEWS=('landing-sweep','lower-sweep','passage-sweep','repeat-ascent')
def stats(values):
    if not values:return None
    v=sorted(values)
    return {'n':len(v),'meanMs':statistics.mean(v)/1e6,'medianMs':statistics.median(v)/1e6,
            'p95Ms':v[math.ceil(.95*len(v))-1]/1e6,'maximumMs':v[-1]/1e6}
def analyze(report:bytes,resources:str):
    strict=analyze_bytes(report)
    if len(resources.encode('utf-8'))>MAX_REPORT_BYTES:raise AnalysisError('CPU CSV exceeds bound')
    rows=list(csv.DictReader(io.StringIO(resources)));j=json.loads(report)
    frames=j['motionEvidence']['completedRtFrames']
    def identity(r):
        return tuple(r.get(a,r.get(b)) for a,b in [('surfaceGeneration','surfaceGeneration'),
            ('sceneEpoch','sceneEpoch'),('measurementGeneration','measurementGeneration'),
            ('record','recordSerial'),('submission','submissionSerial'),('tick','simulationTick')])
    actual={identity(r):r['actualPresentTime'] for r in j['imagePresentationTiming']['capture']['rows']}
    try:present=[actual[identity(f)] for f in frames]
    except KeyError as e:raise AnalysisError('Missing owning actual display timestamp') from e
    if not rows or len(rows)!=len(frames):raise AnalysisError('CPU rows must match every completed frame')
    last=0
    for r,f in zip(rows,frames):
        try:
            serial=int(r['serial']);tick=int(r['tick'])
            if serial!=f['submission'] or tick!=f['tick'] or serial<=last:raise ValueError('identity mismatch')
            if r['cpu_stage_status']!='valid':raise ValueError('CPU stages unavailable')
            for s in CPU:
                if int(r[s+'_cpu_ns'])<0 or int(r[s+'_calls'])<0:raise ValueError('negative CPU fact')
            if int(r['skin_cpu_ns'])!=int(r['player_skin_cpu_ns'])+int(r['character_skin_cpu_ns']):raise ValueError('skin aggregate mismatch')
            if int(r['whole_frame_cpu_ns'])!=f['wholeFrameCpuNs']:raise ValueError('whole-frame owner mismatch')
            last=serial
        except (KeyError,ValueError,TypeError) as e:raise AnalysisError(str(e)) from e
    def summarize(indices):
        return {s:{'all':stats([int(rows[i][s+'_cpu_ns']) for i in indices]),
                   'active':stats([int(rows[i][s+'_cpu_ns']) for i in indices if int(rows[i][s+'_calls'])]),
                   'calls':sum(int(rows[i][s+'_calls']) for i in indices)} for s in CPU}
    views={}
    for phase in VIEWS:
        ix=[i for i,r in enumerate(rows) if i and r['phase']==phase and rows[i-1]['phase']==phase and
            60<=int(r['view_ticks'])<=300 and 60<=int(rows[i-1]['view_ticks'])<=300]
        if len(ix)<20:raise AnalysisError(phase+': fewer than20 settled samples')
        pose={k:[float(rows[i][k]) for i in ix] for k in ('x','y','z','yaw','pitch')}
        if any(not all(math.isfinite(x) for x in v) or max(v)-min(v)>=.0001 for v in pose.values()):raise AnalysisError(phase+': moving view')
        views[phase]={'pose':{k:statistics.median(v) for k,v in pose.items()},'cpu':summarize(ix),
                      'gpu':stats([frames[i]['gpuNs'] for i in ix]),
                      'actualDisplay':stats([present[i]-present[i-1] for i in ix])}
    return {'strictPresentation':strict,'joinedFrames':len(frames),'allRouteCpu':summarize(range(len(rows))),
            'views':views,'limits':'Completed owning CPU stages; Skin aggregates PlayerSkin+CharacterSkin. PoseCopy is separate player CPU repack output, not bus traffic. Companion CharacterSkin includes inseparable repack. Never add GPU to CPU/wait durations or infer display FPS from GPU.'}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('report',type=Path);p.add_argument('resources',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    try:r=analyze(a.report.read_bytes(),a.resources.read_text(encoding='utf-8'))
    except (AnalysisError,OSError,ValueError,KeyError) as e:p.exit(1,'CPU profile rejected: '+str(e)+'\n')
    text=json.dumps(r,indent=2)+'\n'
    if a.output:a.output.write_text(text,encoding='utf-8')
    else:print(text,end='')
