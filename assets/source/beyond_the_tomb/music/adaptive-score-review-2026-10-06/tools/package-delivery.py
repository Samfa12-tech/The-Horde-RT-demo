import pathlib,zipfile,json,hashlib,zlib
ROOT=pathlib.Path(__file__).resolve().parents[1];OUT=ROOT/'delivery';OUT.mkdir(exist_ok=True)
MAX=17_800_000
readmes=[ROOT/'START-HERE.txt',ROOT/'README.md',ROOT/'TRACKLIST.txt']
def make(name,files):
 path=OUT/name
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in files:z.write(f,str(f.relative_to(ROOT)))
 with zipfile.ZipFile(path) as z:
  assert z.testzip() is None
 assert path.stat().st_size<18_000_000,(path,path.stat().st_size)
 return {'filename':name,'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'files':len(files)}
files=readmes.copy()
for folder in ['scores','event-scores','editor-compatible','share-codes','manifests','evidence','source-reference','tools']:
 files += [p for p in sorted((ROOT/folder).rglob('*')) if p.is_file() and not p.name.endswith('.raw.json') and p.suffix in ['.json','.txt','.md','.csv','.py','.mjs','.cjs']]
files += [ROOT/'validation/audio-delivery-verification.json',ROOT/'validation/external-mp3-audio-metrics.json',ROOT/'validation/editor-node-vm/report.json',ROOT/'validation/editor-node-vm/README.md']
files += [p for p in sorted((ROOT/'validation/bass-audit').glob('*')) if p.suffix in ['.json','.md']]
files += [p for p in sorted((ROOT/'previews/bass-voice-alternative').glob('*')) if p.suffix in ['.json','.md']]
# Include browser verification reports if generated, but never upstream app/synth code.
files += [p for p in sorted((ROOT/'validation').glob('app-*.json')) if p not in files]
results=[make('The-Horde-Editable-Scores-and-Cue-Guide.zip',files)]
# Keep a whole family together whenever possible; all parts are independently usable.
for category,groups in [
 ('Core-Reference-Full-Tracks',[[p] for p in sorted((ROOT/'previews/full').glob('*.mp3'))]),
 ('Core-Reference-Lossless-Sections',[[p for p in sorted(d.glob('*')) if p.suffix in ['.flac','.json']] for d in sorted((ROOT/'section-audio').iterdir()) if d.is_dir()])]:
 groups=[g for g in groups if g];part=[];size=0;number=1
 for group in groups:
  gsize=sum(len(zlib.compress(p.read_bytes(),6))+256 for p in group)
  if part and size+gsize>MAX:
   results.append(make(f'The-Horde-{category}-Part-{number}.zip',readmes+part));number+=1;part=[];size=0
  assert gsize<MAX
  part+=group;size+=gsize
 if part:results.append(make(f'The-Horde-{category}-Part-{number}.zip',readmes+part))
(OUT/'delivery-index.json').write_text(json.dumps({'packages':results,'audioStatus':'Core reference synthesis. Review first; not production-mastered or native-integrated.','sampleRate':44100,'pcmBitDepth':16},indent=2)+'\n')
print(json.dumps(results,indent=2))
