import runpy,json,copy,pathlib
ns=runpy.run_path(str(pathlib.Path(__file__).with_name('compose.py')))
ROOT=ns['ROOT'];p=ns['p'];make=ns['make_section'];base=ns['base'];manifest=json.load(open(ROOT/'manifests/cue-manifest.json'))
out=ROOT/'event-scores';out.mkdir(exist_ok=True)
spec={'n':16,'key':'D','scale':'minor','lead':'soft_pluck','second':'bell','third':'soft','rhythm':'still','counter':''}
def new(title):
 q=copy.deepcopy(base);q.update(title=title,projectVersion=16,key='D',scale='minor',bpm=80,swing=0,chordInstrument='harp',chordType='sus2',chordVolume=.16,leadVolume=.42,beatVolume=.2,masterVolume=.7,chordOctave=0,fxDelay=0,fxChorus=0,fxFlanger=0,fxReverb=.2,fxMix=.1,chordsOn=True,bassOn=False,guitarEnabled=False,sidechainOn=False,humanizeOn=False)
 q['sectionBars']={};q['songSequence']=list('ABCDEFGH');return q
banks=[('utility-signals',[
 ('discovery',1,'0:62:2 4:63:2 8:69:4',[0,0,0,0]),
 ('local_tool',2,'0:62:3 8:65:3 16:69:3 24:74:6',[0,5,0,0]),
 ('light_solved',1,'0:64:2 4:69:3 10:74:5',[0,0,0,0]),
 ('access_opened',1,'0:62:2 4:64:2 8:69:6',[0,0,0,0]),
 ('seal_awarded',4,'0:69:4 12:74:4 24:76:4 38:74:5 54:69:7',[0,5,3,0]),
 ('lantern_clue',2,'0:74:3 10:75:3 22:69:7',[0,4,0,0]),
 ('player_defeat',2,'0:62:4 10:61:3 20:57:9',[0,4,0,0]),
 ('retry_pickup',1,'4:57:2 10:62:5',[0,0,0,0])]),
 ('encounter-and-tool-answers',[
 ('prologue_defeat',2,'0:69:3 8:65:4 18:64:4 26:62:5',[0,0,0,0]),
 ('bellkeeper_defeat',2,'0:74:3 8:72:3 16:69:4 26:62:5',[3,0,0,0]),
 ('mintmaster_defeat',2,'0:64:3 6:67:3 14:66:3 24:64:7',[0,0,0,0]),
 ('lucid_keeper_defeat',2,'4:62:4 14:63:3 24:64:6',[0,4,0,0]),
 ('entity_defeat',4,'0:62:4 12:66:4 24:69:5 40:64:5 54:62:9',[0,3,4,0]),
 ('reflector_acquired',2,'0:62:3 8:69:3 18:74:3 26:76:5',[3,0,0,0]),
 ('stand_acquired',2,'0:62:3 6:62:2 14:65:3 24:69:7',[0,5,0,0]),
 ('aperture_acquired',2,'0:74:3 8:77:3 18:76:3 26:74:5',[0,3,0,0])])]
records=[]
for index,(name,events) in enumerate(banks):
 q=new('The Horde event bank — '+name)
 if index: q['compositionProvenance']={'role':'Optional cue variants. Confirmed defeat does not imply death. Authored event playback, not automatic dispatcher.'}
 for sec,(label,bars,notes,harm) in zip('ABCDEFGH',events):
  sp=copy.deepcopy(spec)
  if 'bellkeeper' in label or 'reflector' in label:sp['lead']='bell'
  if 'lucid' in label or 'aperture' in label:sp['lead']='soft'
  make(q,sec,sp,p(notes),harm,bars,'one_shot')
  for lane in q['grid'+sec]:q['grid'+sec][lane]=[0]*64
  records.append({'bank':name+'.json','section':sec,'id':label,'bars':bars,'bpm':80,'behavior':'one_shot','trigger':'Only on confirmed matching gameplay event','priority':'defeat_cancels_pending_rewards' if label=='player_defeat' else 'contextual','replay':'Deduplicate persistent rewards; apply cooldown to discoveries and clue inserts; no automatic return implemented'})
 (out/(name+'.raw.json')).write_text(json.dumps(q,indent=2)+'\n')
manifest['optional_event_banks']=records
manifest['future_engine_artistic_requests']=[
 {'priority':'musical','request':'True compound6/8 meter for Bellwether and Homecoming alternatives; current delivered4/4-lilt scores remain valid. A six-quarter beat counter alone is not6/8 beat-group semantics.'},
 {'priority':'high','request':'Faithful acoustic-seeming renderer voices, full envelope/filter/FX parity between editor/live/offline/native paths, without changing note or register intent.'},
 {'priority':'high','request':'True section-end queueing and cancellable state transitions with stinger playback and verified return.'},
 {'priority':'musical','request':'Longer through-composed sections, variable-length forms and reusable arranged state loops beyond8blocks when engine supports them.'}]
(ROOT/'manifests/cue-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Wrote2 optional event banks,16 authored one-shots covering9 common event families and local variants')
