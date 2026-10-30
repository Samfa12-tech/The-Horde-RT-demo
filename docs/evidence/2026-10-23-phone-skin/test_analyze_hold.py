import copy,csv,importlib.util,io,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'tests'))
from test_android_motion_present_timing import make_report,COUNTERS
spec=importlib.util.spec_from_file_location('local_hold',Path(__file__).with_name('analyze_hold.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

def fixture(scale=20):
    report=make_report();scope={'surfaceGeneration':1,'sceneEpoch':2,'measurementGeneration':3}
    scenario='chapter-outdoor-scale-hold'
    for part in (report,report['motionManifest'],report['motionEvidence']):part['scenario']=scenario
    report['workload']='motion-'+scenario+'-v1'
    settings=report['settings'];settings.update(scalePercent=scale,internalWidth=(1440*scale+50)//100,internalHeight=(2980*scale+50)//100,
        nativeWindowWidth=2980,nativeWindowHeight=1440,surfaceCurrentWidth=1440,surfaceCurrentHeight=2980,
        surfaceCurrentTransform=8,swapchainPreTransform=8,rtPreRotationRequired=True,rtPresentationTransform=3,mist=False,dust=1)
    report['outdoorHold']={'enabled':True,'normalFixedStep':True,'warmupWallSeconds':10,'sampleWallSeconds':30,'startWallNanoseconds':1_000_000_000}
    report['motionManifest'].update(scale=scale,mist=False,dust=1,captures=0,compiledQuality='Mobile')
    report['motionImageCaptures']=[]
    e=report['motionEvidence'];e['states']=[];e['completedRtFrames']=[];e['events']=[]
    e['pipelines']=[{'executionMode':0,'activeKey':'shipping_mobile_opaque_fast','activeSha256':'a'*64,'opaqueSha256':'b'*64,'genericSha256':'c'*64}]
    wrapper=report['imagePresentationTiming'];setup=wrapper['capture']['rows'][0]
    timings=[setup];rows=[]
    for i in range(41):
        e['states'].append({'row':i,'wallNs':1_000_000_000+i*1_000_000_000,'tick':i*60,'publication':10+i,'overruns':0,'ticksThisFrame':5,
            'simulationSeconds':i,'axes':[0,0],'player':[13.3,36.2,2.51,-.15,i,0],'paused':False,'playerAlive':True,'damageEnabled':False,'playerAction':0})
        frame={**scope,'stateRow':i,'tick':i*60,'record':11+i,'recordAttempt':11+i,'submission':11+i,'completion':11+i,
            'actualUploadedDustQuality':1,'actualUploadedMistEnabled':False,'cpuStatus':5,'gpuStatus':5,'gpuAvailable':True,'gpuNs':10_000_000,
            'wholeFrameCpuNs':40_000_000,'simulationCpuNs':100,'skinUpdates':10+i,'hostVisibleBytes':2000,'deviceLocalBytes':10000}
        e['completedRtFrames'].append(frame)
        timings.append({**scope,'simulationTick':i*60,'recordSerial':11+i,'submissionSerial':11+i,'presentID':11+i,
            'scalePercent':scale,'width':1440,'height':2980,'backend':'RayTracingPipeline','swapchainSerial':1,
            'actualPresentTime':2_000_000_000+i*1_000_000_000,'earliestPresentTime':0,'presentMargin':0})
        row={'serial':11+i,'tick':i*60,'phase':'lookout-sweep','present_return_ns':1_100_000_000+i*1_000_000_000,'interval_ns':0 if i==0 else 1_000_000_000,
             'cpu_stage_status':'valid','x':13.3,'y':-1.15,'z':36.2,'yaw':2.51,'pitch':-.15,'view_ticks':i*60,'detail_step_cpu_ns':10}
        row.update({key:0 for key in m.RESOURCE_FIELDS})
        row.update(device_local_bytes=10000,host_visible_bytes=2000,tlas_instances=10,blas_count=8)
        for stage in m.CPU_STAGES:row.update({stage+'_cpu_ns':100,stage+'_calls':1})
        row.update(skin_cpu_ns=200,whole_frame_cpu_ns=40_000_000)
        rows.append(row)
    wrapper['capture']['rows']=timings
    wrapper['capture']['counters'].update(preparedPresents=42,acceptedPresents=42)
    wrapper['counterDelta']={key:wrapper['capture']['counters'][key]-wrapper['counterBaseline'][key] for key in COUNTERS}
    report['timingEligibility'].update(matchedCompletedFrames=41,expectedCompletedFrames=41)
    return report,rows

def analyze(report,rows,scale=20):
    s=io.StringIO();writer=csv.DictWriter(s,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    return m.analyze(json.dumps(report).encode(),s.getvalue(),scale)

checks=0
for scale in (20,33,40):
    r,rows=fixture(scale);out=analyze(r,rows,scale)
    assert out['eligibleForCompleteGpuCpuComparison'] and out['sampling']['completedFramesPostWarm']==31
    assert out['gpu']['duration']['medianMs']==10 and out['cpu']['whole_frame']['allFrames']['medianMs']==40
    assert out['simulation']['frozen'] is False and out['sampling']['warmupOrTerminalFramesExcluded']==10
    assert 'observedPresentedImageRatePerSecond' not in out['strictPresentationJoin']
    checks+=1
mutations=[
    ('missing actual owner',lambda r,rows:r['imagePresentationTiming']['capture']['rows'].pop()),
    ('changed scale',lambda r,rows:r['settings'].update(scalePercent=33)),
    ('wrong tuple',lambda r,rows:(r['settings'].update(mist=True),r['motionManifest'].update(mist=True))),
    ('memory owner mismatch',lambda r,rows:rows[20].update(device_local_bytes=999)),
    ('CPU owner mismatch',lambda r,rows:rows[20].update(whole_frame_cpu_ns=999)),
    ('residency changed',lambda r,rows:rows[20].update(run_unloads=1)),
    ('wrong phase',lambda r,rows:rows[20].update(phase='landing-sweep')),
    ('paused state',lambda r,rows:r['motionEvidence']['states'][20].update(paused=True)),
    ('moving state',lambda r,rows:r['motionEvidence']['states'][20].update(axes=[1,0])),
    ('false wall origin',lambda r,rows:r['outdoorHold'].update(startWallNanoseconds=10_000_000_000)),
    ('wrong rotation',lambda r,rows:r['settings'].update(swapchainPreTransform=1)),
    ('missing CSV',lambda r,rows:rows.pop()),
    ('changed BLAS',lambda r,rows:rows[20].update(blas_count=9)),
    ('wrong compiled quality',lambda r,rows:r['motionManifest'].update(compiledQuality='High')),
    ('wrong pipeline',lambda r,rows:r['motionEvidence']['pipelines'][0].update(activeKey='diagnostic_mobile_opaque_fast')),
]
for name,mutate in mutations:
    r,rows=fixture();mutate(r,rows)
    try:analyze(r,rows)
    except m.AnalysisError:checks+=1
    else:raise AssertionError('accepted '+name)
r,rows=fixture();r['motionEvidence']['completedRtFrames'][20].update(gpuAvailable=False,gpuStatus=3,gpuNs=0)
out=analyze(r,rows);assert out['eligible'] and not out['eligibleForCompleteGpuCpuComparison'] and out['gpu']['unavailableSamples']==1
checks+=1
print(str(checks)+' offline analyzer checks passed; synthetic evidence only, no runtime/performance result.')
