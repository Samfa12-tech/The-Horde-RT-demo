from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json,hashlib,statistics,math,re,zipfile
root=Path.cwd();base=root/'reports/player-dodge-20261020-r1';evidence=root/'docs/evidence/2026-10-20-player-dodge-poses'
def ident(p):return {'path':p.relative_to(root).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
def stats(a):
    a=sorted(a);return {'count':len(a),'median':statistics.median(a),'p95NearestRank':a[math.ceil(.95*len(a))-1],'maximum':a[-1]}
def mesh(p):
    vertices=[];faces=[]
    for line in p.read_text().splitlines():
        if line.startswith('v '):vertices.append(tuple(map(float,line.split()[1:])))
        elif line.startswith('f '):faces.append(tuple(int(x.split('/')[0])-1 for x in line.split()[1:]))
    return vertices,faces
receipt={'date':'2026-10-20','worktree':str(root.resolve()),'sourceBase':'2c9fe54a6a072ff0b2756e8840a489c97a5eda19','branch':'codex/horde-1.7-wp2-vertical-proof','draftPr':27,'runs':{},'pairs':{}}
raw={}
for name in ['pipeline-off','pipeline-on','compute-off','compute-on']:
    path=base/name/'player-dodge-survey.json';native=json.loads(path.read_text());frames=native.pop('frames');raw[name]=frames
    process=json.loads((base/(name+'.process.json')).read_text());assert native['complete'] and len(frames)==64 and native['applicationSnapshotObjectBytesUnchanged']
    assert process['exitCode']==0 and process['settingsUnchanged'] and process['foregroundSamplesUnchanged'] and not process['pending']
    files=[];inventories=[];active=[]
    for index,row in enumerate(frames):
        f=row['completedFrame'];serial=index+1
        assert f['identity']['submissionSerial']==serial and f['identity']['completionSerial']==serial and f['identity']['simulationTick']==row['simulationTick']
        assert f['presentation']['presented'] and f['presentation']['lastSuccessfulPresentSubmissionSerial']==serial and all(f['dispatch'].values())
        assert f['gpu']['status']=='valid' and f['gpu']['completedSubmissionSerial']==serial
        assert f['actualUploadedMistEnabled'] is True and f['actualUploadedDustQuality']==1
        assert f['player']['maximumSocketErrorMicrometres']<=15000
        for key,sha in [('png','pngSha256'),('cpuUploadMesh','meshSha256')]:
            if row[key]:
                item=ident(path.parent/row[key]);assert item['sha256']==row[sha];files.append(item)
        inventories.append(f['resources'])
        if row['publishedPose'][0]>.001:active.append(row)
    assert len({json.dumps(x,sort_keys=True) for x in inventories})==1
    timing={k:stats([r['completedFrame'][kind][field] for r in active]) for k,kind,field in [('wholeRtGpuMs','gpu','wholeRtGpuMs'),('playerSkinCpuMs','stages','playerSkinCpuMs'),('blasRefitRecordCpuMs','stages','blasRefitRecordCpuMs'),('tlasUpdateRecordCpuMs','stages','tlasUpdateRecordCpuMs')]}
    receipt['runs'][name]={'manifest':ident(path),'metadata':native,'frameCount':len(frames),'allMatchingCompletedRtFrames':True,'resourcesStable':True,'resources':inventories[0],'activePoseTimings':timing,'maximumSocketErrorMicrometres':max(x['completedFrame']['player']['maximumSocketErrorMicrometres'] for x in frames),'files':files,'process':{k:v for k,v in process.items() if k not in ['arguments','foregroundSamples']},'foregroundSampleCount':len(process['foregroundSamples']),'foregroundUniqueSamples':sorted(set(map(str,process['foregroundSamples'])))}
    picks=[0,3,35,61]
    save(evidence/(name+'-samples.native.json'),{'fullManifest':ident(path),'evidenceClass':native['evidenceClass'],'frames':[frames[i] for i in picks]})
for backend in ['pipeline','compute']:
    a=receipt['runs'][backend+'-off'];b=receipt['runs'][backend+'-on']
    for key in ['executableSha256','gpu','driverVersion','renderScale','dispatchWidth','dispatchHeight']:
        assert a['metadata'][key]==b['metadata'][key]
    for left,right in zip(raw[backend+'-off'],raw[backend+'-on']):
        for key in ['name','simulationTick','position','yaw','paused','dodgeActive','dodgeElapsedSeconds','publishedPose','heldLightKind']:assert left[key]==right[key],key
        for key in ['pipeline','shadowQuality','fireQuality','actualUploadedMistEnabled','actualUploadedDustQuality']:assert left['completedFrame'][key]==right['completedFrame'][key],key
        if left['name'].endswith('-before'):assert left['meshSha256']==right['meshSha256']
    receipt['pairs'][backend]={'matchedSimulationCameraSettingsAndShaders':True,'resourceDeltas':{k:b['resources'][k]-a['resources'][k] for k in a['resources']},'activePoseMedianDeltas':{k:b['activePoseTimings'][k]['median']-a['activePoseTimings'][k]['median'] for k in a['activePoseTimings']}}
# Orthographic scientific geometry overlay: exact CPU upload, no game-image edit.
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',19);small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',14)
plot=Image.new('RGB',(1600,1030),'white');draw=ImageDraw.Draw(plot)
draw.text((25,12),'Dodge midpoint: exact CPU-upload model geometry (grey = pose off, orange = pose on)',font=font,fill='black')
draw.text((25,40),'Orthographic model metres; root travel removed. This is a geometry diagnostic, not an RT image or GPU readback.',font=small,fill='black')
geometry={}
for col,direction in enumerate(['forward','back','left','right']):
    off,faces=mesh(base/'pipeline-off'/('torch-'+direction+'-6.obj'));on,onFaces=mesh(base/'pipeline-on'/('torch-'+direction+'-6.obj'));assert faces==onFaces and len(off)==len(on)
    geometry[direction]={'vertices':len(on),'triangles':len(faces),'maximumVertexDisplacementMetres':max(math.dist(x,y) for x,y in zip(off,on)),'offBounds':[[min(p[a] for p in off) for a in range(3)],[max(p[a] for p in off) for a in range(3)]],'onBounds':[[min(p[a] for p in on) for a in range(3)],[max(p[a] for p in on) for a in range(3)]]}
    for row,axis in enumerate([0,2]):
        originX=col*400;originY=75+row*465
        draw.text((originX+20,originY),direction+' / '+('front X-Y' if axis==0 else 'side Z-Y'),font=font,fill='black')
        lo=min(min(p[axis] for p in off),min(p[axis] for p in on))-.05;hi=max(max(p[axis] for p in off),max(p[axis] for p in on))+.05
        ylo=min(min(p[1] for p in off),min(p[1] for p in on))-.03;yhi=max(max(p[1] for p in off),max(p[1] for p in on))+.03
        scale=min(350/(hi-lo),395/(yhi-ylo));mid=(lo+hi)*.5
        def point(p):return (originX+200+(p[axis]-mid)*scale,originY+430-(p[1]-ylo)*scale)
        for vertices,colour in [(off,(150,160,170)),(on,(210,102,35))]:
            points=[point(p) for p in vertices]
            for face in faces[::2]:
                coords=[points[i] for i in face];draw.line(coords+[coords[0]],fill=colour,width=1)
        draw.text((originX+20,originY+438),'1:1 geometry; no displacement exaggeration',font=small,fill='black')
plotPath=base/'dodge-geometry-overlay.png';plot.save(plotPath)
receipt['geometryMidpointComparisons']=geometry;receipt['geometryPlot']=ident(plotPath)
receipt['sharedTimeline']=[{k:r[k] for k in ['name','simulationTick','position','yaw','paused','heldLightKind','dodgeActive','dodgeElapsedSeconds','publishedPose']} for r in raw['pipeline-on']]
receipt['artifacts']={k:ident(root/p) for k,p in [('windowsDebug','build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'),('windowsRelease','build/presets/windows-x64-release/Release/HordeLanternRT.exe'),('androidApkBuildOnly','android/app/build/outputs/apk/debug/app-debug.apk')]}
with zipfile.ZipFile(root/receipt['artifacts']['androidApkBuildOnly']['path']) as z:receipt['androidPackagedAbis']=[x.split('/')[1] for x in z.namelist() if re.fullmatch(r'lib/[^/]+/libhorde_rt_probe_android.so',x)]
freeze=json.loads((evidence/'source-freeze.json').read_text());receipt['sourceFreezeMatches']=all(ident(root/p)['sha256']==v['sha256'] for p,v in freeze['sources'].items());assert receipt['sourceFreezeMatches']
canonical=Path('C:/Users/sam_s/Documents/the Horde RT Demo');receipt['canonicalOwnerFilesUnchanged']=all(hashlib.sha256((canonical/p).read_bytes()).hexdigest()==v['sha256'] for p,v in freeze['canonicalOwnerFiles'].items());assert receipt['canonicalOwnerFilesUnchanged']
receipt['timingLimits']='One finite Debug pair per hardware backend, at matching 960x540/100%/High quality. Timings use the same 34 active-pose snapshot rows, even for the visual-off control. They exclude fixture construction and include captured presentation/readback scheduling effects. No sustained FPS, release CPU cost, phone/thermal or improvement claim.'
save(evidence/'receipt.json',receipt)
print(json.dumps({'pairs':receipt['pairs'],'maximumSocketErrors':{k:v['maximumSocketErrorMicrometres'] for k,v in receipt['runs'].items()},'artifacts':receipt['artifacts'],'geometry':geometry},indent=2))
