from pathlib import Path
import subprocess,json,hashlib,struct,array,math,zipfile
root=Path.cwd(); folder=root/'reports/outdoor-music-20261023'; src=folder/'drive-assets'
ffmpeg=Path('C:/Users/sam_s/AppData/Local/Microsoft/WinGet/Packages/Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-9.0-full_build/bin/ffmpeg.exe')
assert ffmpeg.is_file()
archive=src/'Beneath-the-Same-Moon-Outdoor-r2-source (1).zip'
report={'ffmpegSha256':hashlib.sha256(ffmpeg.read_bytes()).hexdigest(),'files':[],'listeningPerformed':False}
with zipfile.ZipFile(archive) as z:
    project=json.loads(z.read('Beneath-the-Same-Moon-Outdoor-r2.json'))
    assert project['title']=='Beneath the Same Moon — Outdoor Exploration r2' and project['bpm']==72
    report['project']={k:project[k] for k in ['title','bpm','projectVersion']}
    report['sourceMembers']=[{'name':i.filename,'size':i.file_size,'sha256':hashlib.sha256(z.read(i)).hexdigest()} for i in z.infolist()]
    for suffix in ['loop','dialogue-bed']:
        path=src/f'Beneath-the-Same-Moon-Outdoor-r2-{suffix}.flac'
        data=path.read_bytes();assert data[:4]==b'fLaC' and data[4]&127==0 and int.from_bytes(data[5:8],'big')==34
        info=data[8:42]; packed=int.from_bytes(info[10:18],'big')
        rate=packed>>44; channels=((packed>>41)&7)+1; bits=((packed>>36)&31)+1;frames=packed&((1<<36)-1)
        assert (rate,channels,bits,frames)==(48000,2,24,5120000)
        result=subprocess.run([str(ffmpeg),'-hide_banner','-v','error','-xerror','-i',str(path),'-f','s24le','-c:a','pcm_s24le','pipe:1'],capture_output=True,check=True)
        pcm24=result.stdout; assert len(pcm24)==frames*channels*3
        digest=hashlib.md5(pcm24).digest();assert digest==info[18:34], 'FLAC decoded MD5 mismatch'
        # Derive statistics from the exact signed 24-bit samples, no processing.
        maximum=0.;sumsq=0.;first=[];last=[];clipped=0
        for i in range(0,len(pcm24),3):
            value=int.from_bytes(pcm24[i:i+3],'little',signed=True)/8388608.
            maximum=max(maximum,abs(value));sumsq+=value*value
            clipped+=abs(value)>=1.
            if i<6:first.append(value)
            if i>=len(pcm24)-6:last.append(value)
        jump=max(abs(a-b) for a,b in zip(first,last));rms=math.sqrt(sumsq/(frames*channels))
        metrics={'peakDbFS':20*math.log10(maximum),'rmsDbFS':20*math.log10(rms),'seamJumpDbFS':20*math.log10(jump) if jump else None}
        expected=json.loads(z.read(f'Beneath-the-Same-Moon-Outdoor-r2-{suffix}.metrics.json'))
        assert abs(metrics['peakDbFS']-expected['peakDbFS'])<.001
        assert abs(metrics['rmsDbFS']-expected['rmsDbFS'])<.001
        assert not clipped
        report['files'].append({'name':path.name,'size':len(data),'sha256':hashlib.sha256(data).hexdigest(),
            'rate':rate,'channels':channels,'bits':bits,'frames':frames,'durationSeconds':frames/rate,
            'decodedPcm24Md5':digest.hex(),'flacStreamInfoMd5Matches':True,'clipped':clipped,
            'measured':metrics,'sourcePackMetrics':expected})
        print(path.name,metrics,'FLAC MD5 verified')
(folder/'drive-audio-verification.json').write_text(json.dumps(report,indent=2)+'\n')
