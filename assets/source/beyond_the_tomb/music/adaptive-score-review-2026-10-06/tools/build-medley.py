import pathlib,wave,json,subprocess,numpy as np,sys
ALT="--soft-bass" in sys.argv
ROOT=pathlib.Path(__file__).resolve().parents[1]
manifest=json.load(open(ROOT/'manifests/cue-manifest.json'))
# A concise coherent tour, ending on the newly answered human theme.
choices=[(1,'A'),(2,'B'),(4,'B'),(6,'B'),(7,'B'),(3,'B'),(3,'D'),(11,'B'),(14,'E'),(15,'H')]
frames=[];chapters=[];offset=0;rate=44100
for n,s in choices:
 family=manifest['families'][n-1];bank=pathlib.Path(family['score']).stem
 file=(ROOT/'previews/bass-voice-alternative'/f'{bank}-{s}.soft-upright-context.wav') if ALT else next((ROOT/'section-audio'/bank).glob(s+'-*.wav'))
 with wave.open(str(file),'rb') as w:
  a=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,2).copy()
 # Medley-only10ms edge fades prevent presenting unrelated key jumps as adaptive transitions.
 edge=min(441,len(a)//2)
 a[:edge]=(a[:edge].astype(float)*np.linspace(0,1,edge)[:,None]).astype('<i2')
 a[-edge:]=(a[-edge:].astype(float)*np.linspace(1,0,edge)[:,None]).astype('<i2')
 chapters.append({'startSeconds':offset/rate,'endSeconds':(offset+len(a))/rate,'title':family['title'],'section':s,'state':family['sections'][s]['name']})
 frames.append(a);frames.append(np.zeros((17640,2),dtype='<i2'));offset+=len(a)+17640
arr=np.concatenate(frames);file=ROOT/'previews'/('The-Horde-Listening-Tour-Soft-Upright-Alternative.wav' if ALT else 'The-Horde-First-Listening-Tour-Core-Reference.wav')
with wave.open(str(file),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(rate);w.writeframes(arr.tobytes())
mp3=file.with_suffix('.mp3')
subprocess.run(['ffmpeg','-v','error','-y','-i',str(file),'-c:a','libmp3lame','-q:a','3','-metadata','title=The Horde — First Listening Tour (Core Reference)','-metadata','comment=Actual Pocket Audio Core synthesized score review; timbre/mastering pending; transitions are editorial excerpts, not runtime integration.',str(mp3)],check=True)
report={'title':'The Horde — First Listening Tour','audio':mp3.name,'durationSeconds':len(arr)/rate,'classification':'Core reference audition; not live-render parity or production mastering','edits':'10ms editorial edge fades and400ms gaps between excerpts; canonical section PCM is unchanged','chapters':chapters}
(ROOT/'previews'/('soft-bass-tour-chapters.json' if ALT else 'listening-tour-chapters.json')).write_text(json.dumps(report,indent=2)+'\n')
text=['The Horde — First Listening Tour','Actual Pocket Audio Core reference synthesis. This is a composition review, not a production master.','']
for c in chapters:
 sec=round(c['startSeconds']);text.append(f"{sec//60}:{sec%60:02d} — {c['title']} ({c['state'].replace('_',' ')})")
if ALT:text+=['','ALTERNATE BASS VOICE ONLY: identical notes/timings and original per-bank gain. Existing Core soft_upright voice applied on cloned bass events because Standard-profile normalization discards a raw bassTone change. Canonical scores are unchanged. This is not the app filtered renderer or a completed engine fix.']
text+=['','The original theme is preserved. New companion scores share its chromatic signature while changing phrase, harmonic context, pace and role.','Editorial gaps/fades in this medley are not in-game adaptive transitions.','Engine filters/FX differ from the accepted historical app voice render; owner listening acceptance is pending.']
(ROOT/'previews'/('Soft-Bass-Listening-Guide.txt' if ALT else 'Listening-Tour-Guide.txt')).write_text('\n'.join(text)+'\n')
print(json.dumps({'mp3':str(mp3),'bytes':mp3.stat().st_size,'duration':len(arr)/rate,'chapters':chapters},indent=2))
