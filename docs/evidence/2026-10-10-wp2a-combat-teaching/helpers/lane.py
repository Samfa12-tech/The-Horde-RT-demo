import json,sys,subprocess
from pathlib import Path
rows=[json.loads(l) for l in Path('reports/wp2a-combat/runs.jsonl').read_text().splitlines()]
old=next(r for r in rows if r['label']=='affected-debug-build')['argv']
targets=old[old.index('--target')+1:old.index('--parallel')]
targets += ['horde_rt_pipeline_variants_tests','horde_rt_pipeline_variant_provider_tests',
 'horde_rt_pipeline_bundle_contracts_tests','horde_rt_provider_shipping_mobile_fixture',
 'horde_rt_provider_shipping_high_fixture','horde_rt_provider_diagnostic_mobile_fixture',
 'horde_rt_provider_diagnostic_high_fixture','horde_rt_sword_target_contact_region_witness']
selection=next(r['argv'][-1] for r in rows if r['label']=='affected-debug-tests')
selection=selection.replace('waterfall_production_route_tests|','waterfall_production_route_tests|waterfall_legacy_route_tests|')
selection=selection.replace('scene_abi_tests|','scene_abi_tests|skeleton_render_pose_contracts|combat_sword_authority_agreement|')
mode=sys.argv[1]; kind,action=mode.split('-')
preset='windows-x64-'+kind
if action=='build': args=['<cmake>','--build','--preset',preset,'--target',*targets,'--parallel','2']
elif action=='tests': args=['<ctest>','--preset',preset,'--no-tests=error','--output-on-failure','-V','-R',selection]
elif action=='packages': args=['<ctest>','--preset',preset,'--no-tests=error','--output-on-failure','-V','-R',
 '^horde_rt_(pipeline_variants_tests|pipeline_variant_provider_tests|pipeline_bundle_contracts_tests|provider_(shipping|diagnostic)_(mobile|high)_fixture(_evidence)?|final_windows_bundle_containment(_controls|_registration)?)$']
else: raise ValueError(mode)
sys.exit(subprocess.call([sys.executable,'reports/wp2a-combat/run.py','candidate-'+mode,*args]))
