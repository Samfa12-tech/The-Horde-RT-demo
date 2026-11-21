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
