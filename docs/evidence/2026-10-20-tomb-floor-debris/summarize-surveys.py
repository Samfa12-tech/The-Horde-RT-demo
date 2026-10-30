from pathlib import Path
import hashlib,json,statistics,math,re,zipfile,xml.etree.ElementTree as ET
from PIL import Image,ImageChops
root=Path.cwd(); base=root/'reports/tomb-debris-20261020-r1'; evidence=root/'docs/evidence/2026-10-20-tomb-floor-debris'
def ident(p):return {'path':str(p.relative_to(root)).replace('\\','/'),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def write(p,data):p.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
def stats(values):
    a=sorted(values); return {'count':len(a),'median':statistics.median(a),'p95NearestRank':a[math.ceil(.95*len(a))-1],'maximum':a[-1]}
receipt={'date':'2026-10-20','worktree':str(root.resolve()),'branch':'codex/horde-1.7-wp2-vertical-proof','draftPr':27,'sourceBase':'25ba90c8a396745463712b4cbc6dc3f13c78dc21','runs':{},'pairs':{}}
for name in ['pipeline-off','pipeline-on','compute-off','compute-on']:
    p=base/name/'environment-survey.json';j=json.loads(p.read_text());process=json.loads((base/(name+'.process.json')).read_text());frames=j.pop('frames')
    assert j['complete'] and len(frames)==120 and j['simulationSnapshotObjectBytesUnchanged'] and process['exitCode']==0 and not process['pending']
    assert process['settingsUnchanged'] and process['foregroundSamplesUnchanged']
    inventories=[]
    for i,r in enumerate(frames):
        f=r['completedFrame'];identity=f['identity'];serial=i+1
        assert r['index']==i and identity['submissionSerial']==serial and identity['completionSerial']==serial and identity['simulationTick']==0
        assert f['presentation']['presented'] and f['presentation']['lastSuccessfulPresentSubmissionSerial']==serial and all(f['dispatch'].values())
        assert f['gpu']['available'] and f['gpu']['status']=='valid' and f['gpu']['completedSubmissionSerial']==serial
        assert f['actualUploadedMistEnabled'] is True and f['actualUploadedDustQuality']==1
        inventories.append(f['resources'])
        if r['file']:assert ident(p.parent/r['file'])['sha256']==r['pngSha256']
    unique={json.dumps(x,sort_keys=True) for x in inventories};assert len(unique)==1
    gpu={view:stats([r['completedFrame']['gpu']['wholeRtGpuMs'] for r in frames if lo<=r['index']<hi]) for view,lo,hi in [('opening',10,60),('debris',70,120)]}
    receipt['runs'][name]={'manifest':ident(p),'processManifest':ident(base/(name+'.process.json')),'nativeMetadata':j,'frameCount':len(frames),'allMatchingCompletedRtFrames':True,'resourcesStableAllFrames':True,'resources':inventories[0],'gpuMs':gpu,'pngs':[{'frame':r['index'],**ident(p.parent/r['file'])} for r in frames if r['file']],'process':{k:process[k] for k in process if k not in ['foregroundSamples','arguments']},'foregroundSampleCount':len(process['foregroundSamples']),'foregroundUniqueSamples':sorted(set(str(x) for x in process['foregroundSamples']))}
    write(evidence/(name+'-samples.native.json'),{'fullManifest':ident(p),'evidenceClass':j['evidenceClass'],'samples':[frames[i] for i in [0,59,60,119]]})
for backend in ['pipeline','compute']:
    off=receipt['runs'][backend+'-off'];on=receipt['runs'][backend+'-on']
    for key in ['executableSha256','gpu','driverVersion','renderScale','dispatchWidth','dispatchHeight','surfaceWidth','surfaceHeight','outputExposure','waterQuality','surveyFogDensityScale','shaderPair','cameras','terrainSha256']:
        assert off['nativeMetadata'][key]==on['nativeMetadata'][key],key
    deltas={k:on['resources'][k]-off['resources'][k] for k in on['resources']}
    timing={v:{'offMedianMs':off['gpuMs'][v]['median'],'onMedianMs':on['gpuMs'][v]['median'],'deltaMs':on['gpuMs'][v]['median']-off['gpuMs'][v]['median'],'deltaPercent':100*(on['gpuMs'][v]['median']/off['gpuMs'][v]['median']-1)} for v in ['opening','debris']}
    pixels={}
    for index in [59,119]:
        a=Image.open(base/(backend+'-off')/f'tomb-debris-{index:03}.png').convert('RGB');b=Image.open(base/(backend+'-on')/f'tomb-debris-{index:03}.png').convert('RGB');d=ImageChops.difference(a,b)
        pixels[str(index)]={'changedRgbPixels':sum(x!=(0,0,0) for x in d.getdata()),'differenceBounds':d.getbbox(),'size':a.size}
    receipt['pairs'][backend]={'matchedSettingsAndShaders':True,'resourceDeltas':deltas,'gpuMs':timing,'pixels':pixels}
receipt['artifacts']={name:ident(root/p) for name,p in [('windowsDebug','build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'),('windowsRelease','build/presets/windows-x64-release/Release/HordeLanternRT.exe'),('androidApkBuildOnly','android/app/build/outputs/apk/debug/app-debug.apk')]}
with zipfile.ZipFile(root/receipt['artifacts']['androidApkBuildOnly']['path']) as z:
    receipt['androidPackagedAbis']=[x.split('/')[1] for x in z.namelist() if re.fullmatch(r'lib/[^/]+/libhorde_rt_probe_android.so',x)]
receipt['androidGraphicsUnitTests']=[]
for p in (root/'android/app/build/test-results/testDebugUnitTest').glob('TEST-*.xml'):
    x=ET.parse(p).getroot()
    if x.attrib['name'].split('.')[-1] in ['GraphicsPreferencesTest','PerformanceProfileRequestTest']:
        assert x.attrib['failures']=='0' and x.attrib['errors']=='0';receipt['androidGraphicsUnitTests'].append({k:x.attrib[k] for k in ['name','tests','failures','errors','skipped']})
freeze=json.loads((evidence/'source-freeze.json').read_text())
receipt['sourceFreezeMatches']=all(ident(root/p)['sha256']==x['sha256'] for p,x in freeze['sources'].items());assert receipt['sourceFreezeMatches']
canonical=Path('C:/Users/sam_s/Documents/the Horde RT Demo')
receipt['canonicalOwnerFilesUnchanged']=all(hashlib.sha256((canonical/p).read_bytes()).hexdigest()==x['sha256'] for p,x in freeze['canonicalOwnerFiles'].items());assert receipt['canonicalOwnerFilesUnchanged']
receipt['limits']=['One Debug render-only off/on pair per native hardware RT backend at 960x540; 50 retained timed frames per view after 10 settling frames. No sustained FPS, phone performance, thermal, earned journey, collision/contact, player artistic or owner acceptance claim.','Signed timing deltas are observations and do not prove chips improve performance.','Host-visible and device-local classifications may overlap and must not be summed.','The final test-only yaw correction did not change either executable or production camera. Earlier failed test logs remain retained.']
write(evidence/'receipt.json',receipt)
print(json.dumps({'pairs':receipt['pairs'],'artifacts':receipt['artifacts'],'androidPackagedAbis':receipt['androidPackagedAbis']},indent=2))
