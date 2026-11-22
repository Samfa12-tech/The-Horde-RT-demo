import argparse, csv, itertools, json, math, pathlib, statistics
import re

HOST_SPANS=('host_previous_tail_ns','host_system_load_ns','host_messages_ns',
            'host_visibility_ns','host_clock_simulation_ns','host_render_to_present_ns')
HOST_FIELDS=(*HOST_SPANS,'host_previous_evidence_ns','host_phases_available')
def verify_host_phases(row):
    # Existing scene allocation telemetry shares the host_ prefix; it is not
    # part of the timing partition. Every other unknown host field still fails.
    assert set(k for k in row if k.startswith('host_'))-{'host_visible_bytes'}==set(HOST_FIELDS)
    if 'host_visible_bytes' in row: assert int(row['host_visible_bytes'])>=0
    assert row['host_phases_available']==row['present_interval_available']
    values={k:int(row[k]) for k in HOST_FIELDS[:-1]}
    assert all(v>=0 for v in values.values())
    if row['host_phases_available']=='1':
        assert sum(values[k] for k in HOST_SPANS)==int(row['present_return_interval_ns'])
        assert values['host_previous_evidence_ns']<=values['host_previous_tail_ns']
    else:
        assert row['host_phases_available']=='0' and not any(values.values())
    return values

def stats(values):
    if not values: return {'count': 0, 'available': False}
    v = sorted(values)
    def rank(p): return v[max(0, math.ceil(len(v)*p)-1)]
    return dict(count=len(v), mean=statistics.fmean(v), median=statistics.median(v),
                p95=rank(.95), p99=rank(.99), maximum=v[-1], minimum=v[0])


BELLWETHER_RESIDENT_SOURCE_IDS = {
    'TavernKeeper': 0x4230314b,
    'RecordKeeper': 0x42303252,
    'Artificer': 0x42303541,
    'Salvager': 0x42303553,
}
def verify_bellwether_resident_roster(rows, intervals=None):
    """Validate the resident sequence shared by the Windows and Android routes."""
    ids = BELLWETHER_RESIDENT_SOURCE_IDS
    role_names = [('TavernKeeper', 'RecordKeeper'), ('TavernKeeper', 'Salvager'),
                  ('Artificer', 'Salvager')]
    town, fork, east = (tuple(ids[name] for name in names) for names in role_names)
    names_by_pair = dict(zip((town, fork, east), map('+'.join, role_names)))
    expected_pairs = {'room': [town], 'doorway-outbound': [town, fork, east],
                      'east-fork': [east], 'doorway-inbound': [east, fork, town],
                      'complete': [town]}
    expected_phases = [(phase, lap) for lap in range(2)
                       for phase in expected_pairs if phase != 'complete']
    expected_phases += [('room', 2), ('complete', 2)]

    pairs = []
    for row in rows:
        fields = ('resident_slot0_id', 'resident_slot1_id')
        values = [row.get(key, '') for key in fields]
        if any(not re.fullmatch(r'[0-9]+', value or '') for value in values):
            raise ValueError(f'Frame {row["submission_serial"]} lacks decimal resident source IDs')
        if any(row[key] != '1' for key in ('resident_mask', 'record_keeper_mask') if key in row):
            raise ValueError(f'Frame {row["submission_serial"]} lost a resident slot mask')
        pairs.append(tuple(map(int, values)))

    observed_phases = []
    for (phase, lap), frames in itertools.groupby(
            zip(rows, pairs), key=lambda entry: (entry[0]['phase'], int(entry[0]['returns']))):
        observed_phases.append((phase, lap))
        observed_pairs = [pair for pair, _ in itertools.groupby(pair for _, pair in frames)]
        if observed_pairs != expected_pairs.get(phase):
            raise ValueError(f'Return {lap + 1} has the wrong resident pair sequence in {phase}')
    if observed_phases != expected_phases:
        raise ValueError('Expected two complete room-doorway-east-fork returns')
    if intervals is not None and len(intervals) != len(rows):
        raise ValueError('Resident transition intervals do not match rows')

    counts = {'outbound': {}, 'inbound': {}}
    transitions = {}
    for index, (before, after) in enumerate(zip(pairs, pairs[1:]), start=1):
        if before == after:
            continue
        row = rows[index]
        direction = row['phase'].removeprefix('doorway-')
        if direction not in counts:
            raise ValueError(f'Frame {row["submission_serial"]} changed residents outside the walk')
        trip = int(row['returns']) + 1
        counts[direction][str(trip)] = counts[direction].get(str(trip), 0) + 1
        name = f'{direction}:{names_by_pair[before]}->{names_by_pair[after]}'
        transition = transitions.setdefault(name, {'returns': [], 'intervals': []})
        transition['returns'].append(trip)
        if intervals is not None:
            interval = intervals[index]
            if interval is None or interval <= 0:
                raise ValueError(f'Frame {row["submission_serial"]} lost its transition present interval')
            transition['intervals'].append(interval)

    for transition in transitions.values():
        values = transition.pop('intervals')
        transition['count'] = len(transition['returns'])
        if values:
            transition['presentReturnIntervalMilliseconds'] = stats(values)
            transition['worstPresentReturnIntervalMilliseconds'] = max(values)
    return dict(sourceIds=ids, phasePairSequences=expected_pairs,
                transitionCounts=counts, transitions=transitions)


def analyze(name, analysis_name='analysis-v3.json'):
    root=pathlib.Path(name); sealed=(root/'completed-frames.csv').exists()
    directory=root if sealed else root/'shipping-route'
    run=json.loads((directory/'run.json').read_text())
    receipt=json.loads((root/'launch.candidate.json' if sealed else pathlib.Path(name+'.candidate.json')).read_text(encoding='utf-8-sig'))
    manifest=json.loads((root/'capture-manifest.json').read_text())
    rows=list(csv.DictReader((directory/'completed-frames.csv').open()))
    identities=[json.loads(line) for line in (directory/'identities.jsonl').read_text().splitlines()]
    assert run['schema'] in (2,3,4) and run['complete'] and 60<=run['durationSeconds']<=150
    legacy=run['schema']==2
    assert run['scope'].startswith('Optimized Release CPU')
    assert run['unoccludedClientChecks'] and 'Release CPU' in receipt['scope']
    assert len(rows)==run['completedSamples']==run['renderedFrames']
    streaming='staticDetailUnloads' in run
    if streaming:
        assert run['staticDetailUnloads']>0 and run['staticDetailRestores']==run['staticDetailUnloads']
        assert run['finalStaticDetailSavingBytes']==0
        assert max(int(r['static_saved_owner_bytes']) for r in rows)>0
        assert max(int(r['static_omitted_triangles']) for r in rows)==84219
        assert int(rows[-1]['static_saved_owner_bytes'])==int(rows[-1]['static_omitted_triangles'])==0
        assert int(rows[-1]['static_restores'])==run['staticDetailRestores']
    else:
        assert run['normalStaticRegionResidencySavingBytes']==0
    by_serial={int(r['submission_serial']):r for r in rows}
    assert len(by_serial)==len(rows)
    serials=list(by_serial); assert serials==list(range(serials[0],serials[-1]+1))
    assert len({(r['scene_epoch'],r['generation']) for r in rows})==1
    assert all(float(a['elapsed_s'])<=float(b['elapsed_s']) and int(a['tick'])<=int(b['tick'])
               for a,b in zip(rows,rows[1:]))
    assert all(r['unoccluded_client']=='1' for r in rows)
    if legacy:
        assert all(int(r['present_return_interval_ns'])>0 for r in rows)
    else:
        assert rows[0]['present_interval_available']=='0' and int(rows[0]['present_return_interval_ns'])==0
        assert all(r['present_interval_available']=='1' and int(r['present_return_interval_ns'])>0 for r in rows[1:])
    if run.get('foregroundRequested'):
        assert receipt['foregroundRequested'] and all(r['foreground_client']=='1' for r in rows)
    # Printed elapsed seconds have six significant figures. Check the interval
    # journal against their independent differences with only formatting error.
    assert all(abs((float(b['elapsed_s'])-float(a['elapsed_s']))-int(b['present_return_interval_ns'])/1e9)<.0002
               for a,b in zip(rows,rows[1:]))
    stage_columns=[k for k in rows[0] if k.endswith('_ns') and k!='gpu_ns' and k not in HOST_FIELDS]
    if run['schema']==4:
        for row in rows: verify_host_phases(row)
    assert all(int(r[k])>=0 for r in rows for k in stage_columns)
    assert all(int(r[k])==0 for r in rows for k in r if k.endswith('_overflow'))
    settings=[]
    for item in identities:
        e=item['evidence']; identity=e['identity']; r=by_serial[identity['submissionSerial']]
        assert item['phase']==r['phase'] and item['station']==int(r['station'])
        assert identity['simulationTick']==int(r['tick'])
        assert e['pipeline']['instrumentation']=='shipping' and e['pipeline']['dielectricQuality']=='high'
        assert e['pipeline']['executionBackend']==receipt['backend']==manifest['executionBackend']
        assert e['pipeline']['activeSha256']==r['shader_sha256']
        assert not e['dielectric']['compiled'] and not e['dielectric']['available'] and e['dielectric']['counters'] is None
        assert e['dielectric']['readCount']==e['dielectric']['resetCount']==0
        assert e['presentation']['outcome']=='presented'
        assert e['presentation']['lastSuccessfulPresentSubmissionSerial']==identity['submissionSerial']
        assert e['resources']['deviceLocalBytes']==int(r['device_local_bytes'])
        assert e['resources']['hostVisibleBytes']==int(r['host_visible_bytes'])
        assert e['gpu']['status']==r['gpu_status']
        for k in stage_columns:
            if k in ('simulation_step_cpu_ns','present_return_interval_ns'): continue  # external owner, deliberately separate
            assert abs(int(r[k])/1e6-e['stages'][k[:-3]])<=.000001
        settings.append((e['shadowQuality']['mode'],e['pipeline']['waterQuality'],
                         e['fireQuality']['quality'],e['actualUploadedDustQuality']))
    assert len(set(settings))==1
    warm=[r for r in rows if float(r['elapsed_s'])>=run['warmupSeconds']
          and (legacy or r['present_interval_available']=='1')]
    assert warm and all(r['cpu_eligible']=='1' for r in warm)
    summary=dict(scope=run['scope'],backend=receipt['backend'],executableSha256=receipt['executableSha256'],
                 hardware=manifest['device'],run=run,settings=list(settings[0]),
                 warmSamples=len(warm),gpuStatusCounts={},shaderIdentities=sorted({r['shader_sha256'] for r in rows}),
                 resourcePeaks={k:max(int(r[k]) for r in rows) for k in ('device_local_bytes','host_visible_bytes')},
                 cpuMilliseconds={k:stats([int(r[k])/1e6 for r in warm]) for k in stage_columns},phases={})
    for r in warm: summary['gpuStatusCounts'][r['gpu_status']]=summary['gpuStatusCounts'].get(r['gpu_status'],0)+1
    summary['gpuMilliseconds']=stats([int(r['gpu_ns'])/1e6 for r in warm if r['gpu_status']=='valid' and int(r['gpu_ns'])>0])
    intervals=[int(r['present_return_interval_ns']) for r in warm]
    interval_key='postFrameObservationIntervalMilliseconds' if legacy else 'presentReturnIntervalMilliseconds'
    throughput_key='postFrameObservationThroughputHz' if legacy else 'presentReturnThroughputHz'
    summary['verifiedIntervalMeaning']=('legacy post-RenderFrame observation intervals; NOT actual present API return timestamps'
        if legacy else run['intervalMeaning'])
    summary[interval_key]=stats([v/1e6 for v in intervals])
    summary[throughput_key]=len(intervals)/(sum(intervals)/1e9)
    if run['schema']==4:
        summary['hostMilliseconds']={k:stats([int(r[k])/1e6 for r in warm]) for k in HOST_FIELDS[:-1]}
    summary['scanoutTimingMeasured']=False
    if streaming:
        transitions=[]
        for previous, current in zip(rows, rows[1:]):
            before, after = int(previous['static_saved_owner_bytes']), int(current['static_saved_owner_bytes'])
            if before == after: continue
            centre=float(current['elapsed_s'])
            window=[r for r in rows if abs(float(r['elapsed_s'])-centre)<=1.0 and r['present_interval_available']=='1']
            transitions.append(dict(kind='unload' if after>before else 'restore',
                elapsedSeconds=centre,submissionSerial=int(current['submission_serial']),
                phase=current['phase'],savedBytesBefore=before,savedBytesAfter=after,
                deviceLocalBytesBefore=int(previous['device_local_bytes']),
                deviceLocalBytesAfter=int(current['device_local_bytes']),
                windowRadiusSeconds=1.0,
                presentReturnMilliseconds=stats([int(r['present_return_interval_ns'])/1e6 for r in window]),
                peakDeviceLocalBytes=max(int(r['device_local_bytes']) for r in window)))
        assert sum(t['kind']=='unload' for t in transitions)==run['staticDetailUnloads']
        assert sum(t['kind']=='restore' for t in transitions)==run['staticDetailRestores']
        summary['staticDetailTransitions']=transitions
    load_path=directory/'system-load.csv'
    if not legacy:
        load=list(csv.DictReader(load_path.open()))
        assert load and load[0]['available']=='0'
        valid=[float(r['total_cpu_busy_percent']) for r in load if r['available']=='1']
        assert all(0<=v<=100 for v in valid)
        summary['machineCpuBusyPercent']=stats(valid)
        summary['machineLoadScope']='aggregate machine CPU time sampled about once per second; no process or per-stall attribution'
    summary['foregroundFrameCounts']={str(value):sum(r.get('foreground_client')==str(value) for r in rows) for value in (0,1)}
    summary['largestIntervals']=[{k:r[k] for k in ('elapsed_s','present_return_interval_ns','phase','station','tick',
        'gpu_ns','simulation_step_cpu_ns','imageAcquireCpuMs_ns','frameFenceWaitCpuMs_ns','wholeFrameCycleCpuMs_ns')}
        for r in sorted(rows,key=lambda r:int(r['present_return_interval_ns']),reverse=True)[:12]]
    if run['schema']==4:
        for item,row in zip(summary['largestIntervals'],sorted(rows,key=lambda r:int(r['present_return_interval_ns']),reverse=True)):
            item['hostPhasesNanoseconds']=verify_host_phases(row)
    for phase in dict.fromkeys(r['phase'] for r in warm):
        p=[r for r in warm if r['phase']==phase]
        summary['phases'][phase]=dict(samples=len(p),gpuMilliseconds=stats([int(r['gpu_ns'])/1e6 for r in p
                                  if r['gpu_status']=='valid' and int(r['gpu_ns'])>0]),
                                  cpuCycleMilliseconds=stats([int(r['wholeFrameCycleCpuMs_ns'])/1e6 for r in p]),
                                  simulationStepMilliseconds=stats([int(r['simulation_step_cpu_ns'])/1e6 for r in p]))
        intervals=[int(r['present_return_interval_ns']) for r in p]
        summary['phases'][phase][interval_key]=stats([v/1e6 for v in intervals])
        summary['phases'][phase]['resources']={k:{'minimum':min(int(r[k]) for r in p),
            'maximum':max(int(r[k]) for r in p)} for k in ('device_local_bytes','host_visible_bytes')}
    assert pathlib.Path(analysis_name).name==analysis_name and analysis_name.endswith('.json')
    target=directory/analysis_name
    if target.exists():
        assert json.loads(target.read_text())==summary,'stored analysis differs from verified arithmetic'
    else:
        assert not sealed,'sealed packet must already contain its analysis'
        target.write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:summary[k] for k in ('backend','warmSamples','settings','gpuMilliseconds','resourcePeaks')}))
    print('CPU stages (median/p95 ms):',json.dumps({k:(v['median'],v['p95']) for k,v in summary['cpuMilliseconds'].items()}))
    print('phase GPU (median/p95 ms):',json.dumps({k:(v['gpuMilliseconds'].get('median'),v['gpuMilliseconds'].get('p95')) for k,v in summary['phases'].items()}))
    print(interval_key,':',json.dumps(summary[interval_key]), 'throughput Hz:',summary[throughput_key])

def analyze_bellwether(name, analysis_name, *, print_phases=True):
    root = pathlib.Path(name)
    directory = root / 'bellwether-doorway'
    run = json.loads((directory / 'run.json').read_text())
    receipt = json.loads(pathlib.Path(name + '.candidate.json').read_text(encoding='utf-8-sig'))
    manifest = json.loads((root / 'capture-manifest.json').read_text())
    with (directory / 'completed-frames.csv').open(newline='') as source:
        rows = list(csv.DictReader(source))
    journal = [json.loads(line) for line in (directory / 'identities.jsonl').read_text().splitlines()]
    assert run['schema'] in (4, 5) and run['complete'] and run['quietReturns'] == 2
    door_witness = run['schema'] == 5
    if door_witness:
        hashes = receipt['tavernAssetHashes']
        original_assets = {'asset.manifest.json', 'bellwether-tavern-v1.glb',
                           'south-door.manifest.json', 'bellwether-south-door.glb'}
        drive_assets = {'mill-bench-drive.manifest.json', 'mill-bench-drive.glb'}
        wheel_assets = {'mill-waterwheel.manifest.json', 'mill-waterwheel.glb'}
        assert set(hashes) in (original_assets, original_assets | drive_assets,
                               original_assets | drive_assets | wheel_assets)
        assert all(re.fullmatch(r'[0-9a-f]{64}', digest) for digest in hashes.values())
    assert 60 <= run['durationSeconds'] <= 150 and run['simulationHz'] == 60
    assert not run['unoccludedClientChecks'] and not run['foregroundRequested']
    assert run['scope'].startswith('Debug CPU') and receipt['residentFixtureHashes']
    record_keeper_included = run.get('recordKeeperIncluded', False)
    assert record_keeper_included == ('record-keeper/resident.glb' in receipt['residentFixtureHashes'])
    assert manifest['complete'] and receipt['backend'] == manifest['executionBackend']
    assert manifest['captures'][0]['bellwetherVisit']['quietReturns'] == 2
    assert len(rows) == run['completedSamples'] == run['renderedFrames']
    by_serial = {int(row['submission_serial']): row for row in rows}
    serials = list(by_serial)
    assert len(serials) == len(rows) and serials == list(range(serials[0], serials[-1] + 1))
    assert len({(row['scene_epoch'], row['generation']) for row in rows}) == 1
    ticks = [int(row['tick']) for row in rows]
    assert ticks == sorted(ticks) and ticks[-1] - ticks[0] >= 3600
    assert all(row['present_interval_available'] == row['present_return_interval_ns'] ==
               row['unoccluded_client'] == '0' for row in rows)
    phases = [phase for phase, _ in itertools.groupby(row['phase'] for row in rows)]
    assert phases == ['room', 'doorway-outbound', 'square', 'doorway-inbound'] * 2 + ['room', 'complete']
    for row in rows:
        verify_host_phases(row)
        assert all(int(row[column]) == 0 for column in row if column.endswith('_overflow'))
        if row['phase'] == 'room':
            assert [row[column] for column in ('fire_count', 'kit_mask', 'window_mask', 'resident_mask')] == ['3', '1', '1', '1']
        if record_keeper_included:
            assert row['record_keeper_mask'] == '1'
        if door_witness:
            assert row.get('tavern_door_mask') == '1', 'Frame lost the open tavern door instance'
    keys = set()
    for item in journal:
        frame = item['evidence']
        identity = frame['identity']
        serial = identity['submissionSerial']
        row = by_serial[serial]
        assert serial not in keys
        keys.add(serial)
        assert identity['sceneEpoch'] == int(row['scene_epoch'])
        assert identity['measurementGeneration'] == int(row['generation'])
        assert identity['completionSerial'] == serial
        assert identity['simulationTick'] == int(row['tick']) and item['phase'] == row['phase']
        assert item['station'] == int(row['station'])
        if door_witness:
            assert item.get('tavernDoorMask') == int(row['tavern_door_mask'])
        assert frame['presentation']['outcome'] == 'presented'
        assert frame['presentation']['lastSuccessfulPresentSubmissionSerial'] == serial
        assert frame['pipeline']['executionBackend'] == receipt['backend']
        assert frame['pipeline']['activeSha256'] == row['shader_sha256']
        assert frame['gpu']['status'] == row['gpu_status']
        if frame['gpu']['status'] == 'valid':
            assert frame['gpu']['completedSubmissionSerial'] == serial
            assert math.isclose(frame['gpu']['wholeRtGpuMs'], int(row['gpu_ns']) / 1e6, abs_tol=0.0001)
        assert frame['resources']['deviceLocalBytes'] == int(row['device_local_bytes'])
        assert frame['resources']['tlasInstanceCount'] == int(row['tlas_instances'])
        assert frame['resources']['bottomLevelAccelerationStructureCount'] == int(row['blas_owners'])
        assert frame['fireLighting']['count'] == int(row['fire_count'])
    assert serials[0] in keys and serials[-1] in keys
    assert {item['phase'] for item in journal} == set(phases)
    summary = dict(scope=run['scope'], backend=receipt['backend'], executableSha256=receipt['executableSha256'],
                   residentFixtureHashes=receipt['residentFixtureHashes'], hardware=manifest['device'],
                   presentation=manifest['presentation'], run=run, phases={})
    if door_witness:
        summary.update(openDoorWitness=True, tavernAssetHashes=hashes)
    for phase in dict.fromkeys(phases[:-1]):
        samples = [row for row in rows if row['phase'] == phase and float(row['elapsed_s']) >= run['warmupSeconds']]
        valid = [int(row['gpu_ns']) / 1e6 for row in samples if row['gpu_status'] == 'valid' and int(row['gpu_ns']) > 0]
        assert valid
        summary['phases'][phase] = dict(samples=len(samples), gpuMilliseconds=stats(valid),
            resourcePeaks={column: max(int(row[column]) for row in samples)
                           for column in ('device_local_bytes', 'host_visible_bytes', 'tlas_instances', 'blas_owners')},
            fireCounts=sorted({int(row['fire_count']) for row in samples}))
    assert pathlib.Path(analysis_name).name == analysis_name and analysis_name.endswith('.json')
    target = directory / analysis_name
    if target.exists():
        assert json.loads(target.read_text()) == summary, 'stored doorway analysis differs from verified arithmetic'
    else:
        target.write_text(json.dumps(summary, indent=2) + '\n')
    if print_phases:
        print(json.dumps(summary['phases']))
    return summary


def compare_bellwether_keeper(baseline, candidate, analysis_name):
    """Compare verified Windows routes with only the tavern keeper fixture changed."""
    runs = []
    for name in (baseline, candidate):
        directory = pathlib.Path(name) / 'bellwether-doorway'
        summary = analyze_bellwether(name, analysis_name, print_phases=False)
        receipt = json.loads(pathlib.Path(name + '.candidate.json').read_text(encoding='utf-8-sig'))
        with (directory / 'completed-frames.csv').open(newline='') as source:
            rows = list(csv.DictReader(source))
        assert summary.get('openDoorWitness') and summary['run']['recordKeeperIncluded']
        assert all(row['gpu_status'] == 'valid' and int(row['gpu_ns']) > 0 for row in rows), (
            'Both routes need a valid GPU interval for every presented frame')
        actor_masks = ('kit_mask', 'window_mask', 'resident_mask', 'record_keeper_mask')
        assert all(all(row[key] == '1' for key in actor_masks) for row in rows), (
            'Both routes must retain all three actors and the window')
        fixtures = summary['residentFixtureHashes']
        keeper = {key: value for key, value in fixtures.items() if key.startswith('tavern-keeper/')}
        assert {'tavern-keeper/asset.manifest.json', 'tavern-keeper/resident.glb'} <= keeper.keys()
        render_settings = []
        for line in (directory / 'identities.jsonl').read_text().splitlines():
            sample = json.loads(line)['evidence']
            pipeline = sample['pipeline']
            render_settings.append((
                sample['submittedDimensions'], sample['shadowQuality'], sample['fireQuality'],
                sample['actualUploadedDustQuality'], sample['actualUploadedMistEnabled'],
                pipeline['instrumentation'], pipeline['waterQuality'], pipeline['dielectricQuality'],
                pipeline['opaqueFast'], pipeline['genericDielectric']))
        assert all(item == render_settings[0] for item in render_settings)
        route = summary['run']
        comparison_identity = {
            key: receipt[key] for key in ('backend', 'executableSha256', 'buildProfile',
                                          'scope', 'sources', 'tavernAssetHashes')}
        comparison_identity.update(
            otherResidents={key: value for key, value in fixtures.items() if key not in keeper},
            hardware=summary['hardware'], presentation=summary['presentation'],
            renderSettings=render_settings[0], simulationTicks=route['simulationTicks'],
            tickBounds=[int(rows[0]['tick']), int(rows[-1]['tick'])],
            simulationHz=route['simulationHz'], warmupSeconds=route['warmupSeconds'])
        runs.append((summary, comparison_identity, keeper))

    (old, old_identity, old_keeper), (new, new_identity, new_keeper) = runs
    mismatches = [key for key, value in old_identity.items() if value != new_identity[key]]
    assert not mismatches, f'Unmatched routes: {", ".join(mismatches)}'
    assert old_keeper['tavern-keeper/resident.glb'] != new_keeper['tavern-keeper/resident.glb'], (
        'The two fixtures use the same tavern keeper')
    result = dict(scope=old['scope'], backend=old['backend'], executableSha256=old['executableSha256'],
                  hardware=old['hardware'], presentation=old['presentation'],
                  simulationTicks=old['run']['simulationTicks'],
                  baselineKeeper=old_keeper, candidateKeeper=new_keeper, phases={})
    for phase, before in old['phases'].items():
        after = new['phases'][phase]
        old_p95, new_p95 = (item['gpuMilliseconds']['p95'] for item in (before, after))
        result['phases'][phase] = dict(
            baselineSamples=before['samples'], candidateSamples=after['samples'],
            baselineGpuP95Ms=old_p95, candidateGpuP95Ms=new_p95,
            changeGpuP95Ms=new_p95 - old_p95,
            baselineResourcePeaks=before['resourcePeaks'], candidateResourcePeaks=after['resourcePeaks'])
    print(json.dumps(result, indent=2))


def analyze_android_bellwether(name, analysis_name, *, resident_roundtrip=False):
    directory = pathlib.Path(name)
    filename = 'bellwether-resident-roundtrip.csv' if resident_roundtrip else 'bellwether-doorway.csv'
    with (directory / filename).open(newline='') as source:
        reader = csv.DictReader(source)
        fields = set(reader.fieldnames or ())
        door_witness = 'tavern_door_mask' in fields
        rows = list(reader)

    def require(condition, detail):
        if not condition:
            raise ValueError(detail)

    require(bool(rows), 'Bellwether route report is empty')
    destination = 'east-fork' if resident_roundtrip else 'square'
    phases = ('room', 'doorway-outbound', destination, 'doorway-inbound')
    if resident_roundtrip:
        require({'resident_slot0_id', 'resident_slot1_id', 'tavern_door_mask'} <= fields,
                'Resident round-trip report lacks source IDs or the tavern door witness')
    else:
        groups = [pair for pair, _ in itertools.groupby(
            (row['phase'], int(row['returns'])) for row in rows)]
        expected = [(phase, lap) for lap in range(2) for phase in phases]
        require(groups == expected + [('room', 2), ('complete', 2)],
                'Expected two complete room-doorway-square returns')
    backend = rows[0]['backend']
    identity = rows[0]['scene_epoch'], rows[0]['generation']
    shader_hash = rows[0]['shader_sha256']
    setting_fields = ('internal_width', 'internal_height', 'output_width', 'output_height', 'glass_enabled')
    width, height, surface_width, surface_height, glass = tuple(int(rows[0][key]) for key in setting_fields)
    require(backend in ('RayTracingPipeline', 'RayQueryCompute') and all(int(key) > 0 for key in identity),
            'Missing admitted RT backend or scene identity')
    require(0 < width <= surface_width and 0 < height <= surface_height and glass in (0, 1),
            'Missing render extent or glass setting')
    serials = [int(row['submission_serial']) for row in rows]
    ticks = [int(row['tick']) for row in rows]
    elapsed = [int(row['elapsed_ns']) for row in rows]
    returned = [int(row['present_return_ns']) for row in rows]
    require(serials[0] > 0 and serials == list(range(serials[0], serials[0] + len(rows))),
            'Submissions must be continuous')
    deadline_ns = 241e9 if resident_roundtrip else 151e9
    require(elapsed[0] > 0 and 60e9 <= elapsed[-1] <= deadline_ns and ticks[-1] - ticks[0] >= 3600,
            'Missing a full simulated or wall-clock route')
    clock_base = returned[0] - elapsed[0]
    intervals = []
    for index, row in enumerate(rows):
        serial = serials[index]
        require((row['scene_epoch'], row['generation']) == identity and row['backend'] == backend,
                f'Frame {serial} changed scene, measurement generation or backend')
        require(tuple(int(row[key]) for key in setting_fields) == (width, height, surface_width, surface_height, glass),
                f'Frame {serial} changed render extents or glass')
        require(all(int(row[key]) == serial for key in ('completion_serial', 'present_serial', 'gpu_serial')),
                f'Frame {serial} lost the GPU-to-present join')
        require(int(row['record_serial']) > 0 and int(row['gpu_ns']) > 0 and int(row['cpu_cycle_ns']) > 0 and
                re.fullmatch(r'[0-9a-f]{64}', row['shader_sha256']), f'Frame {serial} lacks timing or shader evidence')
        require(not resident_roundtrip or row['shader_sha256'] == shader_hash,
                f'Frame {serial} changed the active shader')
        if resident_roundtrip:
            require(row['resident_mask'] == row['record_keeper_mask'] == '1',
                    f'Frame {serial} lost a resident slot mask')
        else:
            require(row['record_keeper_mask'] == '1', f'Frame {serial} lost the record keeper instance')
        if door_witness:
            require(row['tavern_door_mask'] == '1', f'Frame {serial} lost the open tavern door instance')
        if row['phase'] == 'room' or (resident_roundtrip and row['phase'] == 'complete'):
            require([row[key] for key in ('fire_count', 'kit_mask', 'window_mask', 'resident_mask')] == ['3', '1', str(glass), '1'],
                    f'Frame {serial} changed the staffed tavern workload')
        require(returned[index] - elapsed[index] == clock_base and
                (index == 0 or (returned[index] > returned[index - 1] and 0 <= ticks[index] - ticks[index - 1] <= 4)),
                f'Frame {serial} broke the clock or 60 Hz catch-up bound')
        intervals.append(None if index == 0 else (returned[index] - returned[index - 1]) / 1e6)

    summary = dict(backend=backend, scope='Android Debug RT submissions and present-call returns',
                   totalFrames=len(rows), durationSeconds=elapsed[-1] / 1e9,
                   simulationTicks=ticks[-1] - ticks[0], shaderHashes=sorted({row['shader_sha256'] for row in rows}),
                   internalExtent=[width, height], outputExtent=[surface_width, surface_height], glassEnabled=bool(glass),
                   phases={})
    if door_witness:
        summary['openDoorWitness'] = True
    if resident_roundtrip:
        roster = verify_bellwether_resident_roster(rows, intervals)
        summary.update(scope='Android Debug RT resident round-trip submissions and present-call returns',
                       residentSourceIds=roster['sourceIds'],
                       residentSlotSequencesByPhase={phase: [list(pair) for pair in pairs]
                                                     for phase, pairs in roster['phasePairSequences'].items()},
                       transitionCounts=roster['transitionCounts'], transitions=roster['transitions'])
    for phase in phases:
        indices = [i for i, row in enumerate(rows) if row['phase'] == phase and elapsed[i] >= 5e9]
        require(bool(indices), f'No post-warmup frames for {phase}')
        resources = {}
        for key in ('device_local_bytes', 'host_visible_bytes', 'tlas_instances', 'blas_owners'):
            values = [int(rows[i][key]) for i in indices]
            resources[key] = [min(values), max(values)]
        phase_intervals = [intervals[i] for i in indices
                           if i and rows[i - 1]['phase'] == phase and elapsed[i - 1] >= 5e9]
        phase_summary = dict(
            gpuMilliseconds=stats([int(rows[i]['gpu_ns']) / 1e6 for i in indices]),
            nativeCycleMilliseconds=stats([int(rows[i]['cpu_cycle_ns']) / 1e6 for i in indices]),
            presentReturnIntervalMilliseconds=stats(phase_intervals),
            resourceMinMax=resources)
        if resident_roundtrip:
            phase_summary['worstPresentReturnIntervalMilliseconds'] = max(phase_intervals, default=None)
        summary['phases'][phase] = phase_summary
    require(pathlib.Path(analysis_name).name == analysis_name and analysis_name.endswith('.json'),
            'Expected a plain JSON analysis filename')
    target = directory / analysis_name
    if target.exists():
        require(json.loads(target.read_text()) == summary, 'Stored analysis differs from verified arithmetic')
    else:
        target.write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Verify completed exact-identity chapter route evidence.')
    parser.add_argument('directories',nargs='+')
    parser.add_argument('--analysis-name',default='analysis-v3.json')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--bellwether-doorway', action='store_true')
    mode.add_argument('--compare-bellwether-keeper', action='store_true')
    mode.add_argument('--android-bellwether-doorway', action='store_true')
    mode.add_argument('--android-bellwether-resident-roundtrip', action='store_true')
    args=parser.parse_args()
    if args.compare_bellwether_keeper:
        if len(args.directories) != 2:
            parser.error('--compare-bellwether-keeper expects a baseline and a candidate directory')
        compare_bellwether_keeper(*args.directories, args.analysis_name)
        raise SystemExit(0)
    for name in args.directories:
        if args.android_bellwether_doorway or args.android_bellwether_resident_roundtrip:
            analyze_android_bellwether(name, args.analysis_name,
                                      resident_roundtrip=args.android_bellwether_resident_roundtrip)
        elif args.bellwether_doorway:
            analyze_bellwether(name, args.analysis_name)
        else:
            analyze(name, args.analysis_name)
