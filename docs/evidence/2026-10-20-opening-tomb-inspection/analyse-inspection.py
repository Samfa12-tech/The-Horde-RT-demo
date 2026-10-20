from pathlib import Path
import ctypes,hashlib,json
root=Path.cwd().resolve();base=root/'reports/tomb-composition-20261020-r1'
def ident(p):return {'exists':p.exists(),**({'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} if p.is_file() else {})}
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
f=load(base/'source-freeze.json')
for group in ['sources','trackedTombAssets','protected']:
    for name,v in f[group].items():
        p=Path(name) if Path(name).is_absolute() else root/name
        assert ident(p)==v,p
sr=Path(f['campaignDirectory']['path']);assert sr.exists()==f['campaignDirectory']['exists']
saves={str(p.relative_to(sr)):ident(p) for p in sr.rglob('*') if p.is_file()} if sr.exists() else {}
assert saves==f['campaignDirectory']['files']
processes=load(base/'processes.json');assert len(processes)==3
assert [x['exitCode'] for x in processes]==[0,1,1]
assert all(not x['pending'] and x['settingsUnchanged'] and x['foregroundSamplesUnchanged'] for x in processes)
p=base/'opening-survey/environment-survey.json';d=load(p);rows=d['frames'];assert d['complete'] and len(rows)==120 and d['simulationSnapshotObjectBytesUnchanged']
for n,row in enumerate(rows):
    c=row['completedFrame'];i=c['identity'];p=c['presentation']
    assert row['index']==n and i['recordSerial']==i['submissionSerial']==i['completionSerial']==n+1
    assert i['simulationTick']==0 and c['dispatch']['rtDispatchRecorded'] and c['dispatch']['swapchainCopyRecorded']
    assert p['presented'] and p['finalIdleCompletion'] and p['lastSuccessfulPresentSubmissionSerial']==n+1
    assert c['pipeline']['executionBackend']=='RayTracingPipeline'
    if row['file']:assert ident(base/'opening-survey'/row['file'])['sha256']==row['pngSha256']
selected={k:v for k,v in d.items() if k!='frames'}
selected['scope']='Compact exact native rows 0,59,60,119 from the hashed full manifest; not the full 120-row original'
selected['fullManifest']=ident(base/'opening-survey/environment-survey.json')
selected['frames']=[rows[n] for n in [0,59,60,119]]
save(base/'opening-survey-compact.json',selected)
files={str(p.relative_to(base)):ident(p) for p in sorted(base.rglob('*')) if p.is_file() and p.suffix in ['.png','.obj']}
reviews=[
 {'path':'opening-survey/tomb-debris-059.png','status':'complete native survey frame','observed':'Clear forward floor lane, opening portal and side-wall bays; bier/lid only peripheral at lower left. No demonstrated grounding/path defect; not an all-prop composition pass.'},
 {'path':'opening-survey/tomb-debris-119.png','status':'complete native survey frame','observed':'Collapse-side masonry, damaged shoulder and small chips remain related and grounded in this view; no justified relocation.'},
 {'path':'blocked-retreat/192-rescue-journey-start.png','status':'UNADMITTED full capture; context only','observed':'Blocked-retreat masonry appears closed, but the primary-arms ownership gate failed; no accepted initial-view or equipment result.'},
 {'path':'rect-niche/218-chapter-niche-rect.png','status':'UNADMITTED full capture; context only','observed':'Rectangular niche/candle visible and neighboring skull partly in frame; no visually demonstrated float, but the same primary-arm gate failed. Not accepted full-frame niche evidence.'}
]
resource_states={json.dumps(x['completedFrame']['resources'],sort_keys=True) for x in rows}
receipt={'scope':'Fresh native RT opening-tomb inspection; no production/source/asset/lighting/collision/save edit and no phone or listening','baseCommit':f['baseCommit'],'executable':f['executable'],'successfulNativeSurvey':{'frames':120,'pngs':sum(bool(x['file']) for x in rows),'backend':d['backend'],'gpu':d['gpu'],'driverVersion':d['driverVersion'],'dispatch':[d['dispatchWidth'],d['dispatchHeight']],'renderScale':d['renderScale'],'outputExposure':d['outputExposure'],'surveyFogDensityScale':d['surveyFogDensityScale'],'simulationTick':0,'snapshotBytesUnchanged':True,'owningCompletedPresentationAll120':True,'resourceStateCount':len(resource_states),'resources':rows[-1]['completedFrame']['resources'],'settings':{'shadow':rows[-1]['completedFrame']['shadowQuality'],'fire':rows[-1]['completedFrame']['fireQuality'],'dust':rows[-1]['completedFrame']['actualUploadedDustQuality'],'mist':rows[-1]['completedFrame']['actualUploadedMistEnabled']},'fullManifest':ident(base/'opening-survey/environment-survey.json')},'failedFullCaptures':[],'notRun':[{'checkpoint':'chapter-niche-arch','reason':'Stopped after second independent full capture failed the same unresolved primary-arm gate.'},{'checkpoint':'chapter-stone-portal-rear','reason':'Stopped after second independent full capture failed the same unresolved primary-arm gate.'}],'visualInspection':reviews,'originalImageAndMeshFiles':files,'ownerSettingsSourcesAssetsAndCampaignUnchanged':True,'allOwnProcessesExited':True,'noProcessKilled':True,'foregroundObservation':'All foreground API samples were null and unchanged; native survey also checks that its own window is unshown and unfocused. This does not certify a particular external foreground HWND.','audioListeningPerformed':False,'newAudioHapticManualRevalidationRequired':False,'priorAudioManualAcceptanceStillPending':True,'performanceClaim':None,'nextBoundedItem':'Diagnose zero primary-arm pixels in current full captures from retained exact uploaded meshes, model transforms and frozen pose; distinguish pose/frustum/occlusion/diagnostic handling before any patch. Preserve the gate and the ordinary initial camera/lighting.'}
for label in ['blocked-retreat','rect-niche']:
    n=load(base/label/'capture-manifest.json');assert not n['complete'] and n['captures']==[] and 'armPixels=0' in n['error']
    receipt['failedFullCaptures'].append({'label':label,'status':'FAILED, not denied; no retry','error':n['error'],'manifest':ident(base/label/'capture-manifest.json'),'retainedPose':load(base/label/'failed-primary-pose.json')})
save(base/'receipt.json',receipt)
print('PASS: all 120 RT dispatch/copy/present/completion identities; 12 exact PNG hashes')
print('PASS: both failed full captures retained; no retry or further full-capture runs')
print('PASS: source/assets/settings/owner/campaign identities unchanged; all three own processes exited')
print('Stable resource states across survey:',len(resource_states))
