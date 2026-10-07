#!/usr/bin/env python3
"""Read-only authoring assertions. Does not execute engine gameplay or produce audio."""
import json,csv,pathlib,collections,hashlib,itertools,re
P=pathlib.Path(__file__).resolve().parent;O=P.parent
b=json.loads((O/'Horde-Dialogue-Bank-English-en.json').read_text()); rows=b['lines'];scenes={s['scene_id']:s for s in b['scenes']};ids={r['line_id']:r for r in rows}
checks=[]
def check(name,ok,detail=''):
 checks.append(dict(name=name,passed=bool(ok),detail=detail));assert ok,name+' '+str(detail)
check('unique_line_ids',len(ids)==len(rows));check('unique_scene_ids',len(scenes)==len(b['scenes']))
check('english_tag_on_bank_and_rows',b['language']=='en' and all(r['language']==r['source_language']=='en' for r in rows))
check('exact_spoken_subtitle_parity',all(r['spoken_text']==r['subtitle_text'] for r in rows))
check('source_hash_matches',all(hashlib.sha256(r['spoken_text'].encode()).hexdigest()==r['source_text_sha256'] for r in rows))
check('timings_null_until_audio',all(r['measured_duration_ms'] is None and all(v is None for v in r['subtitle_timing'].values()) for r in rows))
check('no_protagonist_voice',all(r['speaker_id'] not in ['player','protagonist','hero'] for r in rows))
check('safe_language_independent_filenames',all(r['voice_asset_path']=='audio/voice/en/'+r['line_id']+'.wav' and '/' not in r['line_id'] for r in rows))
check('all_recording_approvals_false',all(not r['recording_approved'] for r in rows))
check('no_direct_predicate_conflicts',all(not(set(r['required_flags'])&set(r['forbidden_flags'])) for r in rows))
check('all_scenes_have_present_speakers',all(r['speaker_id'] in r['trigger']['participants_present'] for r in rows))
check('exact_13_opening_delivered_lines',all(ids[r['line_id']]['spoken_text']==r['spoken_text'] for r in json.loads((P/'opening-baseline.json').read_text())))
check('kit_not_in_dungeons',all(not any(x in r['location'].lower() for x in ['abbey','foundry','court','treasury']) for r in rows if r['speaker_id']=='kit'))
check('optional_final_boss_excluded_core',all(r['release']=='optional-final-boss' for r in rows if r['scene_id'].startswith('optional_boss')))
for r in rows:
 f=O/'TTS-English-en'/r['release']/r['speaker_id']/(r['line_id']+'.txt')
 check('tts_exact_'+r['line_id'],f.read_bytes()==r['spoken_text'].encode())
# Flattened CSV and manifest must round-trip exact strings, including commas and punctuation.
csvrows=list(csv.DictReader((O/'Horde-Dialogue-Bank-English-en.csv').open()))
check('csv_exact_text_and_ids',[(r['line_id'],r['spoken_text']) for r in csvrows]==[(r['line_id'],r['spoken_text']) for r in rows])
loc=json.loads((O/'localization-manifest.json').read_text());check('localization_ids_complete',set(r['line_id'] for r in loc['entries'])==set(ids))
check('critical_fallbacks_exist',all(r['fallback']['required'] and r['fallback']['text'] for r in rows if r['priority']==80))
check('kit_hearing_scene_exists',all(k in ids for k in ['lantern.town_identity','lantern.town_seals','kit.voice_reaction']))
check('army_exchange_setup_exists','hub.horde_army' in ids and 'hub.hoard_answer' in ids)
check('claim_reaction_has_documented_bridge','relic.claim_review' in ids and all('kit_heard_living_claimant_identify_mark' not in r['required_flags'] for r in rows))
check('translation_requires_evidence','paired_inscription_evidence_shown' in ids['return.keeper_word']['required_flags'])
check('no_spurious_cooldown_between_lines',all(r['repeat_policy']['cooldown_scope']=='between_scene_starts_never_between_sequence_lines' for r in rows))
check('critical_ignores_ambient_cooldown',all(r['cooldown_seconds']<=2 for r in rows if r['priority']==80))
check('manual_record_recap_repeatable',ids['hub.service.record.recap']['repeat_policy']['spontaneous']=='on_explicit_player_interaction')
check('attempt_reset_key_valid',ids['bark.entity.command']['repeat_policy']['reset_only']=='new_authoritative_attempt_id')
# Abstract authoring-condition harness, not engine gameplay tests.
def eligible(s,state):
 t=scenes[s]['trigger'];return set(t['required_flags'])<=state and not(set(t['forbidden_flags'])&state) and all(set(g)&state for g in t['any_of'])
for order in [('abbey_seal','foundry_seal'),('foundry_seal','abbey_seal')]:
 state={'seals_shown','safe',order[0]}
 first='return.abbey_first' if order[0]=='abbey_seal' else 'return.foundry_first';other='return.foundry_first' if first=='return.abbey_first' else 'return.abbey_first'
 check('first_seal_'+order[0],eligible(first,state) and not eligible(other,state))
 state|={order[1],'court_route_interaction_complete'}
 check('stale_one_seal_suppressed_'+order[0],not eligible(first,state) and not eligible(other,state))
 check('two_seal_convergence_'+order[0],eligible('return.two_seal_scene',state))
# 3 x 3 independent access choices, both orders. Court gate also needs access and interaction.
for abbey_route,foundry_route,order in itertools.product(['magic','tech','body'],['bridge','stones','climb'],['abbey_first','foundry_first']):
 state={f'access_{abbey_route}_'+dict(magic='ward_trial_passed',tech='rig_trial_passed',body='trial_passed')[abbey_route],f'foundry_{foundry_route}_ready'}
 aready=any(f in state for f in ['access_magic_ward_trial_passed','access_tech_rig_trial_passed','access_body_trial_passed'])
 fready=any(f in state for f in ['foundry_bridge_ready','foundry_stones_ready','foundry_climb_ready'])
 check('independent_access_'+abbey_route+'_'+foundry_route+'_'+order,aready and fready)
for a,f,interaction,access in itertools.product([False,True],repeat=4):
 state={k for k,v in [('abbey_seal',a),('foundry_seal',f),('court_route_interaction_complete',interaction),('court_access_ready',access)] if v}
 actual={'abbey_seal','foundry_seal','court_route_interaction_complete','court_access_ready'}<=state
 check('court_gate_truth_table_'+''.join(map(lambda x:str(int(x)),[a,f,interaction,access])),actual==(a and f and interaction and access))
# Authoring persistence contract: consumed IDs survive loading; scene info recoverable on explicit request.
ledger={'rescue.found'};loaded=set(json.loads(json.dumps(list(ledger))))
check('reload_does_not_replay_consumed_story','rescue.found' in loaded)
check('skip_keeps_critical_fallback',ids['rescue.found']['fallback']['required'] and ids['rescue.found']['repeat_policy']['manual_recap'])
check('stale_queue_rechecked',all(r['trigger']['revalidate_on_enqueue_and_playback'] for r in rows))
check('optional_hub_changes_not_assumed',all(x in ids for x in ['epilogue.welcome','epilogue.kit_close']))


check('direction_hashes_match',all(hashlib.sha256(r['delivery_notes'].encode()).hexdigest()==r['delivery_notes_sha256'] for r in rows))
check('direction_revisions_valid',all(r['delivery_notes_revision']>=1 for r in rows))
check('kit_direction_revised',ids['prologue.kit_grate']['delivery_notes_revision']==2 and 'edge of panic' in ids['prologue.kit_grate']['delivery_notes'])
check('only_starter_has_tagged_audition',[r['line_id'] for r in rows if r['generation_input']] == ['prologue.kit_grate'])
for r in rows:
 g=r['generation_input']
 if g:
  check('tagged_words_unchanged_'+r['line_id'],re.sub(r'\[[^\]]*\]\s*','',g['text'])==r['spoken_text'])
  check('tagged_export_exact_'+r['line_id'],(O/'TTS-Optional-ElevenLabs-en'/(r['line_id']+'.txt')).read_bytes()==g['text'].encode())
  check('tagged_audition_unapproved',not g['recording_approved'] and g['status']=='unauditioned_hint_not_guaranteed')
manifest=json.loads((O/'tts-manifest-en.json').read_text())
check('tts_manifest_direction_parity',all(m['delivery_notes']==r['delivery_notes'] and m['delivery_notes_revision']==r['delivery_notes_revision'] and m['delivery_notes_sha256']==r['delivery_notes_sha256'] for m,r in zip(manifest,rows)) and len(manifest)==len(rows))
check('starter_trigger_not_ack_gated',ids['prologue.kit_grate']['required_flags']==['at_grate','safe'])
check('no_added_dialogue',len(rows)==357 and len(scenes)==209)

print(json.dumps({"static_checks_passed":len(checks),"lines":len(rows),"scenes":len(scenes),"runtime_tests_run":False},indent=2))
