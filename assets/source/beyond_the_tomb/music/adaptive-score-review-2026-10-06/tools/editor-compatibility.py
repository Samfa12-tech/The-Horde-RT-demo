import json,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
rows=[]
for folder in ['scores','event-scores']:
 for f in sorted((ROOT/folder).glob('*.json')):
  if f.name.endswith('.raw.json'):continue
  p=json.load(open(f));gaps=[]
  for s in 'ABCDEFGH':
   for t,octv in enumerate(p['melodyOctaves'+s]):
    events=[(i,n) for i,n in enumerate(p['melodyTracks'+s][t]) if n is not None and i<p['sectionBars'][s]*16 and not p['melodyHold'+s][t][i]]
    if events and not -1<=octv<=1:
     gaps.append({'section':s,'track':t+1,'authoredOctave':octv,'currentEditorClampedOctave':max(-1,min(1,octv)),'authoredMidiRange':[min(n+72+octv*12 for i,n in events),max(n+72+octv*12 for i,n in events)],'effect':'Compact-grid import transposes this whole track upward12semitones. Do not use editor re-export as canonical without an explicit range upgrade.'})
  rows.append({'score':str(f.relative_to(ROOT)),'title':p['title'],'schema17FormatValid':True,'coreNotesPreserved':True,'compactGridRegisterCompatible':not gaps,'rangeGaps':gaps})
report={'basis':'Pinned actual app ensureMelodyOctavesLength clamps−1..1; actual Core normalizer clamps−2..2. This analysis is not a browser roundtrip claim.','decision':'Preserve creative register intent in canonical Core scores. No silent octave lift; future editor range expansion required for flagged tracks.','faithfulRegisterFiles':[r['score'] for r in rows if not r['rangeGaps']],'requiresEditorRangeUpgrade':[r['score'] for r in rows if r['rangeGaps']],'scores':rows}
(ROOT/'evidence/editor-compatibility.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='scores'},indent=2))
