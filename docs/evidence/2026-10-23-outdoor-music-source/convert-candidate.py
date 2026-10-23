from pathlib import Path
import subprocess,hashlib,wave,json,struct
folder=Path('reports/outdoor-music-20261023'); dest=folder/'pcm-candidates'
dest.mkdir(exist_ok=True)
ffmpeg=Path('C:/Users/sam_s/AppData/Local/Microsoft/WinGet/Packages/Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-9.0-full_build/bin/ffmpeg.exe')
receipts=[]
for suffix in ['loop','dialogue-bed']:
    src=folder/'drive-assets'/f'Beneath-the-Same-Moon-Outdoor-r2-{suffix}.flac'
    target=dest/f'{suffix}.wav';assert not target.exists()
    args=[str(ffmpeg),'-hide_banner','-v','error','-xerror','-i',str(src),'-af','aresample=dither_method=none',
          '-c:a','pcm_s16le','-f','s16le','pipe:1']
    pcm=subprocess.run(args,capture_output=True,check=True).stdout
    assert len(pcm)==5120000*2*2
    with wave.open(str(target),'wb') as out:
        out.setnchannels(2);out.setsampwidth(2);out.setframerate(48000);out.writeframes(pcm)
    assert target.stat().st_size==20480044
    first=struct.unpack('<hh',pcm[:4]);last=struct.unpack('<hh',pcm[-4:])
    receipts.append({'name':target.name,'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
     'size':target.stat().st_size,'frames':5120000,'rate':48000,'channels':2,'bits':16,
     'pcmBytes':len(pcm),'pcmSha256':hashlib.sha256(pcm).hexdigest(),
     'wrapDeltaLsb':max(abs(a-b) for a,b in zip(first,last)),
     'processing':'FFmpeg pcm_s16le, dither_method=none; Python wave strict 44-byte header; no trim, gain, resampling or normalization',
     'runtimeAdmission':'unadmitted; current canonical Core decoder/mixer maximum is 576000 frames'})
    print(target.name,receipts[-1]['sha256'],'wrapDeltaLsb',receipts[-1]['wrapDeltaLsb'])
(folder/'pcm-candidate-receipts.json').write_text(json.dumps(receipts,indent=2)+'\n')
