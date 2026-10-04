"""Offline Android Core-admitted ambience candidate; original remains untouched."""
import array
import hashlib
import json
import math
import wave
from pathlib import Path

root=Path(__file__).resolve().parent.parent
source=root/"assets/audio/pixabay/waterfall_loop.wav"
output=root/"assets/audio/pixabay/waterfall_core_loop.wav"
frames=552000
overlap=36000
with wave.open(str(source),"rb") as stream:
    if (stream.getnchannels(),stream.getframerate(),stream.getsampwidth())!=(1,48000,2):
        raise ValueError("Unexpected admitted original waterfall format")
    source_frames=stream.getnframes()
    samples=array.array("h",stream.readframes(source_frames))
if source_frames<frames+overlap:
    raise ValueError("Original lacks the requested real continuation for overlap")
mono=samples[:frames]
for i in range(overlap):
    blend=i/(overlap-1)
    # The start is the actual continuation after the last loop sample. Blend
    # toward the original head over750ms; both interval joins are continuous.
    mono[i]=round(samples[frames+i]*(1-blend)+samples[i]*blend)
stereo=array.array("h")
for sample in mono:
    stereo.extend((sample,sample))
with wave.open(str(output),"wb") as stream:
    stream.setnchannels(2);stream.setframerate(48000);stream.setsampwidth(2)
    stream.writeframes(stereo.tobytes())
record={"schema":1,"status":"bounded Android gapless-worker candidate; owner exact-candidate audition pending",
        "source":"waterfall_loop.wav","sourceSha256":hashlib.sha256(source.read_bytes()).hexdigest(),
        "sourceFrames":source_frames,"sourceDurationSeconds":source_frames/48000,
        "runtimePath":output.name,"sha256":hashlib.sha256(output.read_bytes()).hexdigest(),
        "frames":frames,"sampleRate":48000,"channels":2,"bitsPerSample":16,"bytes":output.stat().st_size,
        "durationSeconds":11.5,"decodedPcmBytes":len(stereo)*2,"overlapFrames":overlap,
        "processing":"Use first11.5s plus750ms real continuation; crossfade continuation into head with linear weights. Dual-mono stereo for unchanged strict Pocket Audio Core admission. No normalization or hard padding; original21.323s retained.",
        "peakDbfs":20*math.log10(max(abs(v) for v in mono)/32767),
        "wrapStepPcm16":mono[0]-mono[-1],"actualSourceAdjacentStepPcm16":samples[frames]-samples[frames-1],
        "tradeoff":"Android candidate repeats after11.5s to fit the existing12s Core capability. Windows keeps accepted full21.323s PCM loop. Shorter repetition/character needs owner listening; no actual MediaPlayer gap/underrun cause has been reproduced.",
        "license":"Existing admitted DRAGON-STUDIO/Pixabay waterfall source licence, retained in ASSET_LICENSES.md; no new recording or generation."}
(output.parent/"waterfall-core.manifest.json").write_text(json.dumps(record,indent=2)+"\n",encoding="utf-8")
print(json.dumps(record,indent=2))
