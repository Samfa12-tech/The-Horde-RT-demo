import importlib.util
import pathlib
import csv

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
