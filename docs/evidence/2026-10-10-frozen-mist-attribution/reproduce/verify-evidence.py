"""Read-only verification of the captured experiment; never production acceptance."""
import hashlib
import json
from pathlib import Path
from collections import Counter
from PIL import Image, ImageChops

evidence = Path(__file__).resolve().parent.parent
root = evidence.parents[2]
def read(path): return json.loads(path.read_text())
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
receipt = read(evidence/'receipt.json')
runs = [json.loads(line) for line in (evidence/'runs.jsonl').read_text().splitlines()]
assert len(runs) == 6
frames = []
images = {}
for run in runs:
    directory = evidence / run['public_output_directory']
    frame = read(directory/'completed-frame.json')
    manifest = read(directory/'capture-manifest.json')
    assert sha(directory/'frozen-state.json') == receipt['published_frozen_state_sha256']
    assert frame['identity']['simulationTick'] == 601
    assert frame['presentation']['presented'] and frame['dispatch']['rtDispatchRecorded']
    assert frame['dispatch']['swapchainCopyRecorded']
    assert frame['presentation']['lastSuccessfulPresentSubmissionSerial'] == 12
    assert frame['pipeline']['executionBackend'] == 'RayTracingPipeline'
    assert frame['pipeline']['activeSha256'] == run['shader']['spirv_sha256']
    assert frame['shadowQuality']['mode'] == 'higher'
    assert frame['shadowQuality']['localPrimarySamples'] == 4
    assert frame['shadowQuality']['skyPrimarySamples'] == 2
    assert frame['fireLighting'] == read(evidence/'captures/midpoint-all/completed-frame.json')['fireLighting']
    assert run['exit_code'] == 1 and not manifest['complete']
    image = Image.open(directory/'192-rescue-journey-start.png').convert('RGB')
    assert image.size == (1232,803)
    images[run['name']] = image
    frames.append(frame)
assert all(run['exe_sha256'] == receipt['diagnostic_executables']['shaded']['sha256'] for run in runs)
for pair in [('sample-local-all','sample-local-sky'),('midpoint-lantern','sample-local-lantern')]:
    assert ImageChops.difference(images[pair[0]],images[pair[1]]).getbbox() is None

sky = evidence/'query-records/midpoint-sky-query-records'
lantern = evidence/'query-records/midpoint-lantern-query-records'
for directory in [sky,lantern]:
    assert sha(directory/'frozen-state.json') == receipt['published_frozen_state_sha256']
    assert read(directory/'completed-frame.json')['presentation']['presented']
geometry = read(sky/'geometry-state.json')
assert geometry == read(lantern/'geometry-state.json')
assert geometry['worldPrimitiveCount'] == 3204 and geometry['ropeTriangleCount'] == 176
pixels = list(Image.open(sky/'192-rescue-journey-start.png').convert('RGB').get_flattened_data())
indices = [i for i,(r,g,b) in enumerate(pixels) if r==0 and 3028 <= (g<<8)+b < 3204]
assert len(indices) == 6722
base = list(images['midpoint-sky'].get_flattened_data())
local = list(images['sample-local-sky'].get_flattened_data())
assert sum(base[i]!=local[i] for i in indices) == 6427
assert sum(sum(local[i])>sum(base[i]) for i in indices) == 6427
assert sum(sum(local[i])<sum(base[i]) for i in indices) == 0
pixels = list(Image.open(lantern/'192-rescue-journey-start.png').convert('RGB').get_flattened_data())
active = [(r,g,b) for r,g,b in pixels if b>0]
assert len(active) == 469920 and all(g==0 for r,g,b in active)
assert Counter(r for r,g,b in active) == {8:462427,0:7470,2:21,4:2}

catalog = read(root/'tools/raygen-variant-catalog.json')
catalog_rows = {row['key']:row for row in catalog['variants']}
budgets = {row['key']:row for row in read(root/'tools/raygen-variant-budgets.json')['budgets']}
matrix = read(evidence/'sample-local-full-metrics.json')
assert len(matrix) == 8 and {row['key'] for row in matrix} == set(catalog_rows)
for row in matrix:
    baseline = row['baseline']; candidate = row['candidate']; key=row['key']
    assert baseline['compiledSpirvSha256'] == catalog_rows[key]['spirvSha256']
    assert baseline['dependencySha256'] == catalog_rows[key]['dependencySha256']
    assert baseline['dependencies'] == catalog_rows[key]['dependencies']
    assert baseline['manifestSha256'] == catalog['authorities']['manifest']['sha256']
    # Match the registered compiler guards, including their query-site bounds.
    assert candidate['driverSafeFullyInlined'] == (candidate['functions']==1 and candidate['functionCalls']==0 and candidate['rayQueryInitializations']<=29)
    assert candidate['boundedGenericFunctionsRetained'] == (candidate['functions']>1 and candidate['functionCalls']>0 and candidate['rayQueryInitializations']<=3)
    failures = [dict(metric=m,measured=candidate[m],maximum=v) for m,v in budgets[key]['max'].items() if candidate[m]>v]
    exact = [dict(field=m,measured=candidate[m],expected=v) for m,v in budgets[key]['exact'].items() if candidate[m]!=v]
    assert failures == row['violations'] and exact == row['exact_violations']
    assert len(failures) == (7 if candidate['material']=='OpaqueFast' else 0)
    assert len(exact) == (1 if candidate['material']=='OpaqueFast' else 0)
extension_runs=[json.loads(line) for line in (evidence/'extension-runs.jsonl').read_text().splitlines()]
assert len(extension_runs)==34
families=Counter(row['family'] for row in extension_runs)
assert dict(families)=={'quality':12,'owner-view':12,'owner-weights':8,'owner-midpoint-rope':2}
family_exes={}
family_states={}
for run in extension_runs:
    directory=evidence/run['public_output_directory']
    frame=read(directory/'completed-frame.json')
    manifest=read(directory/'capture-manifest.json')
    assert run['exit_code']==1 and not manifest['complete']
    assert frame['presentation']['presented'] and frame['dispatch']['rtDispatchRecorded'] and frame['dispatch']['swapchainCopyRecorded']
    assert frame['pipeline']['executionBackend']=='RayTracingPipeline'
    assert frame['pipeline']['activeSha256']==run['shader']['spirv_sha256']
    assert frame['identity']['simulationTick']==601 and frame['presentation']['lastSuccessfulPresentSubmissionSerial']==12
    assert frame['shadowQuality']['mode']==run['shadow'].lower()
    assert [frame['shadowQuality']['localPrimarySamples'],frame['shadowQuality']['skyPrimarySamples']]==([1,1] if run['shadow']=='CURRENT' else [4,2])
    assert sha(directory/'frozen-state.json')==run['published_frozen_state_sha256']
    family_exes.setdefault(run['family'],set()).add(run['exe_sha256'])
    family_states.setdefault(run['family'],set()).add(run['native_frozen_state_sha256'])
assert all(len(x)==1 for x in family_exes.values())
assert all(len(x)==1 for x in family_states.values())
assert family_states['owner-view']==family_states['owner-weights']==family_states['owner-midpoint-rope']
import math,struct
for run in extension_runs:
    if run['family'] not in ['owner-weights','owner-midpoint-rope'] or run['shader']['name'].endswith('masks'):continue
    im=Image.open(evidence/run['public_output_directory']/'192-rescue-journey-start.png').convert('RGBA')
    values=[v[0] for v in struct.iter_unpack('<f',im.tobytes('raw','BGRA'))]
    assert -1.0 in values
    assert all(math.isfinite(x) and (x==-1 or -2e-7<=x<=1.0000002) for x in values)
oracle=read(evidence/'continuous-rope-oracle.json')
assert len(oracle['rows'])==32 and oracle['accepted_rays']==32 and oracle['rejected_rays']==0
assert all(row['discrete_hardware_agreement'] and row['gpu_six_sample_rope_mask']==row['cpu_six_sample_rope_mask'] for row in oracle['rows'])
band=[row for row in oracle['rows'] if row['category']=='midpoint-band']
assert len(band)==16 and all(row['weighted_rope_fraction']['6']==0 and row['weighted_rope_fraction']['4096']>0 for row in band)
print('Verified 42 presented experiments with 42 retained showcase failures; two frozen views/CURRENT-HIGHER; exact float32 maps; 32 geometric hardware agreements; eight unchanged-budget reference decisions.')
