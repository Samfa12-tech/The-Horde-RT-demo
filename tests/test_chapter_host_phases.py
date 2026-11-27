import importlib.util
import json
import pathlib
import csv
from contextlib import redirect_stdout
from io import StringIO
from tempfile import TemporaryDirectory

repo=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('measurement',repo/'tools/analyze-chapter-route-measurement.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
row=dict(zip(m.HOST_SPANS,map(str,(10,10,600,10,10,10))))
row.update(host_previous_evidence_ns='5',host_phases_available='1',present_interval_available='1',present_return_interval_ns='650')
assert m.verify_host_phases(row)['host_messages_ns']==600
def reject(**changes):
    candidate=dict(row,**changes)
    try:m.verify_host_phases(candidate)
    except AssertionError:return
    raise AssertionError(('invalid host attribution accepted',changes))
reject(host_previous_evidence_ns='11')
reject(host_messages_ns='599')
reject(host_system_load_ns='-1')
reject(host_phases_available='0')
reject(host_unknown_ns='1')
reject(host_unknown_bytes='1')
reject(host_visible_bytes='-1')
# Actual first two completed frames from the c80df058 Release hardware run.
# This includes the pre-existing memory field that the synthetic row omitted.
fixture=repo/'tests/fixtures/chapter-route-host-phases/schema4-completed-frames.csv'
with fixture.open() as captured:
    actual=list(csv.DictReader(captured))
assert len(actual)==2 and all('host_visible_bytes' in r for r in actual)
assert [r['present_interval_available'] for r in actual]==['0','1']
for captured in actual: m.verify_host_phases(captured)
first={k:'0' for k in m.HOST_FIELDS}
first.update(present_interval_available='0',present_return_interval_ns='0')
assert not any(m.verify_host_phases(first).values())
first['host_messages_ns']='1'
try:m.verify_host_phases(first)
except AssertionError:pass
else:raise AssertionError('first present borrowed bootstrap work')
print('Exact host interval partition, stale/subset/negative/unknown and first-present negatives passed.')


def verify_android_route():
    phases = [(phase, lap) for lap in range(2)
              for phase in ('room', 'doorway-outbound', 'square', 'doorway-inbound')]
    rows = []
    for phase, lap in phases + [('room', 2), ('complete', 2)]:
        for _ in range(1 if phase == 'complete' else 125):
            serial = 100 + len(rows)
            elapsed = (len(rows) + 1) * 67_000_000
            rows.append(dict(phase=phase, station=1, returns=lap, elapsed_ns=elapsed,
                present_return_ns=200_000_000_000 + elapsed, scene_epoch=4, generation=3,
                record_serial=serial, submission_serial=serial, tick=4 * len(rows),
                completion_serial=serial, present_serial=serial, gpu_serial=serial,
                gpu_ns=40_000_000, cpu_cycle_ns=66_000_000, backend='RayTracingPipeline',
                shader_sha256='a' * 64, internal_width=475, internal_height=980,
                output_width=1440, output_height=2971, glass_enabled=0,
                fire_count=3, tlas_instances=37, blas_owners=28,
                device_local_bytes=231_192_576, host_visible_bytes=78_070_656,
                kit_mask=1, window_mask=0, resident_mask=1, record_keeper_mask=1))

    with TemporaryDirectory() as root:
        report = pathlib.Path(root) / 'bellwether-doorway.csv'

        def analyze(rows, name='analysis.json'):
            with report.open('w', newline='') as output:
                writer = csv.DictWriter(output, fieldnames=rows[0])
                writer.writeheader()
                writer.writerows(rows)
            with redirect_stdout(StringIO()):
                m.analyze_android_bellwether(root, name)

        analyze(rows)
        analyze(rows)
        assert 'openDoorWitness' not in json.loads((report.parent / 'analysis.json').read_text())
        rows = [dict(row, tavern_door_mask=1) for row in rows]
        analyze(rows, 'door-analysis.json')
        assert json.loads((report.parent / 'door-analysis.json').read_text())['openDoorWitness']
        for column, bad, message in (
                ('gpu_serial', 0, 'GPU-to-present join'),
                ('record_keeper_mask', 0, 'lost the record keeper'),
                ('tavern_door_mask', 0, 'lost the open tavern door'),
                ('window_mask', 1, 'changed the staffed tavern workload'),
                ('generation', 2, 'changed scene'),
                ('present_return_ns', 0, 'broke the clock')):
            invalid = [dict(rows[0], **{column: bad}), *rows[1:]]
            try:
                analyze(invalid, 'door-analysis.json')
            except ValueError as error:
                assert message in str(error), error
            else:
                raise AssertionError(f'Bad Android frame passed: {column}')
        try:
            analyze(rows[:-1], 'door-analysis.json')
        except ValueError as error:
            assert 'two complete' in str(error), error
        else:
            raise AssertionError('A truncated Android route passed')


verify_android_route()
print('Android doorway verification rejected stale GPU, missing residents, mixed generations and truncated runs.')


def verify_android_resident_roundtrip(west=False):
    source_ids = {
        'TavernKeeper': 0x4230314b,
        'RecordKeeper': 0x42303252,
        'Artificer': 0x42303541,
        'Salvager': 0x42303553,
    }
    phase_roles = {
        'room': ('TavernKeeper', 'RecordKeeper'),
        'doorway-outbound': ('TavernKeeper', 'Salvager'),
        'east-fork': ('Artificer', 'Salvager'),
        'doorway-inbound': ('TavernKeeper', 'Salvager'),
        'complete': ('TavernKeeper', 'RecordKeeper'),
    }
    phase_sequences = {
        'room': [phase_roles['room']],
        'doorway-outbound': [phase_roles['room'], phase_roles['doorway-outbound'],
                             ('Artificer', 'Salvager')],
        'east-fork': [('Artificer', 'Salvager')],
        'doorway-inbound': [('Artificer', 'Salvager'), phase_roles['doorway-inbound'],
                            phase_roles['room']],
        'complete': [phase_roles['complete']],
    }
    destination = 'stock-yard' if west else 'east-fork'
    if west:
        source_ids['RelicDealer'] = 0x42303644
        town = phase_roles['room']
        dealer = ('TavernKeeper', 'RelicDealer')
        phase_sequences.update({'doorway-outbound': [town, dealer], 'stock-yard': [dealer],
                                'doorway-inbound': [dealer, town]})
    phases = [(phase, lap) for lap in range(2)
              for phase in ('room', 'doorway-outbound', destination, 'doorway-inbound')]
    rows = []
    for phase, lap in phases + [('room', 2), ('complete', 2)]:
        for role0, role1 in phase_sequences[phase]:
            for frame in range(360 if phase == 'stock-yard' else 1 if phase == 'complete' else 55):
                serial = 100 + len(rows)
                elapsed = (len(rows) + 1) * 67_000_000
                rows.append(dict(phase=phase, station=1, returns=lap, elapsed_ns=elapsed,
                    present_return_ns=200_000_000_000 + elapsed, scene_epoch=4, generation=3,
                    record_serial=serial, submission_serial=serial, tick=4 * len(rows),
                    completion_serial=serial, present_serial=serial, gpu_serial=serial,
                    gpu_ns=40_000_000, cpu_cycle_ns=66_000_000, backend='RayTracingPipeline',
                    shader_sha256='a' * 64, internal_width=475, internal_height=980,
                    output_width=1440, output_height=2971, glass_enabled=0,
                    fire_count=3 if phase in ('room', 'complete') else 2,
                    tlas_instances=37, blas_owners=28,
                    device_local_bytes=231_192_576, host_visible_bytes=78_070_656,
                    kit_mask=1, window_mask=0, resident_mask=1, record_keeper_mask=1,
                    tavern_door_mask=1,
                    resident_slot0_id=str(source_ids[role0]),
                    resident_slot1_id=str(source_ids[role1])))
                if west:
                    rows[-1].update(stock_head_instances=3, stock_pose_mask=([0, 1, 2, 4][frame % 4]
                        if phase == 'stock-yard' else 0))

    with TemporaryDirectory() as root:
        report = pathlib.Path(root) / ('bellwether-west-roundtrip.csv' if west else 'bellwether-resident-roundtrip.csv')

        def analyze(rows, name='resident-analysis.json'):
            with report.open('w', newline='') as output:
                writer = csv.DictWriter(output, fieldnames=rows[0])
                writer.writeheader()
                writer.writerows(rows)
            with redirect_stdout(StringIO()):
                m.analyze_android_bellwether(root, name, resident_roundtrip=True, west=west)

        analyze(rows)
        summary = json.loads((report.parent / 'resident-analysis.json').read_text())
        assert summary['residentSourceIds'] == source_ids
        swaps = 1 if west else 2
        assert summary['transitionCounts'] == {'outbound': {'1': swaps, '2': swaps},
                                                'inbound': {'1': swaps, '2': swaps}}
        if west:
            assert len(summary['stockYardVisits']) == 2
            assert all(visit['sampledTicks'] == 1436 and visit['poseMasks'] == [0, 1, 2, 4]
                       for visit in summary['stockYardVisits'])
        else:
            assert summary['transitions']['outbound:TavernKeeper+RecordKeeper->TavernKeeper+Salvager']['count'] == 2
            assert summary['transitions']['inbound:Artificer+Salvager->TavernKeeper+Salvager']['count'] == 2
        assert all('worstPresentReturnIntervalMilliseconds' in phase
                   for phase in summary['phases'].values())
        assert 'presentReturnThroughputHz' not in summary
        host_rows = [{key: row[key] for key in
                      ('phase', 'returns', 'submission_serial', 'resident_slot0_id', 'resident_slot1_id')}
                     for row in rows]
        host_proof = m.verify_bellwether_resident_roster(
            host_rows, [None] + [67.0] * (len(host_rows) - 1), west=west)
        assert host_proof['transitionCounts'] == summary['transitionCounts']

        stale = [dict(row) for row in rows]
        stale[230]['resident_slot1_id' if west else 'resident_slot0_id'] = str(
            source_ids['RecordKeeper' if west else 'TavernKeeper'])
        try:
            analyze(stale, 'stale-roster.json')
        except ValueError as error:
            assert ('stale resident roster' in str(error) or
                    'wrong resident pair sequence' in str(error)), error
        else:
            raise AssertionError('A stale destination resident roster passed')

        faulty_join = [dict(row) for row in rows]
        faulty_join[230]['gpu_serial'] = 0
        try:
            analyze(faulty_join, 'faulty-join.json')
        except ValueError as error:
            assert 'GPU-to-present join' in str(error), error
        else:
            raise AssertionError('A faulty resident-roundtrip timing join passed')

        if west:
            missing_head = [dict(row) for row in rows]
            missing_head[230]['stock_head_instances'] = 2
            try:
                analyze(missing_head, 'missing-head.json')
            except ValueError as error:
                assert 'three submitted head instances' in str(error), error
            else:
                raise AssertionError('Accepted a missing stock head during the measured dwell')


verify_android_resident_roundtrip()
verify_android_resident_roundtrip(west=True)
print('Android resident round-trip verification accepted exact roles and rejected stale roster and faulty timing join.')
