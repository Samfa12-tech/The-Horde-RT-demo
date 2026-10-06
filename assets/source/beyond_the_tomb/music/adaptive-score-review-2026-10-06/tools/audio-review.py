"""Objective rendered-audio review. Does not stand in for perceptual listening."""
import wave,json,pathlib,numpy as np
from scipy.signal import resample_poly,welch
ROOT=pathlib.Path(__file__).resolve().parents[1]
rows=[]
for file in sorted((ROOT/'section-audio').glob('*/*.wav')):
 with wave.open(str(file),'rb') as w:
  assert w.getsampwidth()==2
  rate=w.getframerate();audio=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,w.getnchannels()).astype(float)/32768
 mono=audio.mean(axis=1);rms=float(np.sqrt(np.mean(audio**2)));mrms=float(np.sqrt(np.mean(mono**2)))
 peaks=[]
 for ch in range(audio.shape[1]):peaks.append(float(np.max(np.abs(resample_poly(audio[:,ch],4,1)))))
 f,power=welch(mono,rate,nperseg=4096)
 den=max(float(np.sum(power)),1e-20)
 windows=np.array([np.sqrt(np.mean(mono[k:k+4410]**2)) for k in range(0,len(mono),4410)])
 quiet=windows<10**(-65/20)
 maxrun=run=0
 for q in quiet:
  run=run+1 if q else 0;maxrun=max(maxrun,run)
 row={'file':str(file.relative_to(ROOT)),'durationSeconds':len(mono)/rate,'peakDbfs':float(20*np.log10(max(np.max(np.abs(audio)),1e-20))),
 'fourTimesOversampledPeakDbfs':float(20*np.log10(max(max(peaks),1e-20))),
 'rmsDbfs':float(20*np.log10(max(rms,1e-20))),'monoRmsRelativeDb':float(20*np.log10(max(mrms,1e-20)/max(rms,1e-20))),
 'stereoCorrelation':float(np.corrcoef(audio.T)[0,1]),'powerAbove8kFraction':float(np.sum(power[f>8000])/den),'powerBelow50HzFraction':float(np.sum(power[f<50])/den),
 'longestBelowMinus65DbWindowSeconds':round(maxrun*.1,2),'sampleClips':int(np.sum(np.abs(audio)>=32767/32768)),
 'audit':'Objective waveform/spectral/mono inspection only. Subjective timbre, emotional fit, and device listening remain unverified.'}
 rows.append(row)
report={'scope':'All completed individual section renders','count':len(rows),'clippingPass':all(r['sampleClips']==0 and r['fourTimesOversampledPeakDbfs']<0 for r in rows),'monoPass':all(r['monoRmsRelativeDb']>-3 for r in rows),'maxOversampledPeakDbfs':max((r['fourTimesOversampledPeakDbfs'] for r in rows),default=None),'rows':rows}
(ROOT/'evidence/audio-review.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='rows'},indent=2))
