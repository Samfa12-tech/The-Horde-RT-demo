#!/usr/bin/env python3
"""Regenerate portable English CSV and exact per-line TTS from the authoritative bank.
No API calls, audio generation, runtime edits or paid services. Python 3 standard library only.
"""
import argparse,csv,hashlib,json,pathlib,collections
p=argparse.ArgumentParser();p.add_argument('bank',type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path);a=p.parse_args()
b=json.loads(a.bank.read_text(encoding='utf-8'));rows=b['lines'];a.output.mkdir(parents=True,exist_ok=True)
ids=set()
for r in rows:
 assert r['line_id'] not in ids,r['line_id'];ids.add(r['line_id'])
 assert r['spoken_text']==r['subtitle_text'],r['line_id']
 assert hashlib.sha256(r['spoken_text'].encode()).hexdigest()==r['source_text_sha256'],r['line_id']
 assert r['language']==b['language'] and r['source_language']==b['source_language']
 assert '/' not in r['line_id'] and '\\' not in r['line_id']
with (a.output/'Horde-Dialogue-Bank-English-en.csv').open('w',newline='',encoding='utf-8') as f:
 w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(list,dict)) else v for k,v in r.items()} for r in rows)
manifest=[];groups=collections.defaultdict(list)
for r in rows:
 rel=pathlib.Path('TTS-English-en')/r['release']/r['speaker_id']/(r['line_id']+'.txt')
 dest=a.output/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(r['spoken_text'].encode('utf-8'))
 groups[(r['release'],r['speaker_id'])].append(r)
 manifest.append(dict(language='en',source_language='en',line_id=r['line_id'],speaker_id=r['speaker_id'],input_file=str(rel),output_file=r['voice_asset_path'],spoken_text=r['spoken_text'],source_text_revision=r['source_text_revision'],source_text_sha256=r['source_text_sha256'],delivery_notes=r['delivery_notes'],delivery_notes_revision=r['delivery_notes_revision'],delivery_notes_sha256=r['delivery_notes_sha256'],generation_input_file=str(pathlib.Path('TTS-Optional-ElevenLabs-en')/(r['line_id']+'.txt')) if r['generation_input'] else '',generation_input_status=r['generation_input']['status'] if r['generation_input'] else 'not_authored',release=r['release'],recording_approved=r['recording_approved']))
for (release,speaker),rs in groups.items():
 dest=a.output/'TTS-English-en'/release/(speaker+'-all-spoken-lines-en.txt')
 dest.write_text('\n'.join(r['spoken_text'] for r in rs)+'\n',encoding='utf-8')
 with dest.with_suffix('.csv').open('w',newline='',encoding='utf-8') as f:
  w=csv.DictWriter(f,fieldnames=['combined_line_number','line_id','spoken_text','delivery_notes','delivery_notes_revision','voice_asset_path']);w.writeheader();w.writerows(dict(combined_line_number=i,line_id=r['line_id'],spoken_text=r['spoken_text'],delivery_notes=r['delivery_notes'],delivery_notes_revision=r['delivery_notes_revision'],voice_asset_path=r['voice_asset_path']) for i,r in enumerate(rs,1))
for r in rows:
 if r['generation_input']:
  dest=a.output/'TTS-Optional-ElevenLabs-en'/(r['line_id']+'.txt');dest.parent.mkdir(parents=True,exist_ok=True)
  dest.write_bytes(r['generation_input']['text'].encode('utf-8'))
with (a.output/'tts-manifest-en.csv').open('w',newline='',encoding='utf8') as f:
 w=csv.DictWriter(f,fieldnames=list(manifest[0]));w.writeheader();w.writerows(manifest)
(a.output/'tts-manifest-en.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print('Exported',len(rows),'exact English line inputs. No audio generated.')
