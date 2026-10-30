"""Read-only reproduction of the matched timing reduction; no game launch."""
from pathlib import Path
import csv, hashlib, json, statistics

base = Path(__file__).resolve().parent
summary = json.loads((base/'summary.json').read_text())
labels = [r['label'] for r in summary['runs']]
maps, poses = {}, {}
for run in summary['runs']:
    path = base/(run['label']+'.csv')
    assert hashlib.sha256(path.read_bytes()).hexdigest() == run['framesSha256']
    rows = list(csv.DictReader(path.open()))
    assert len(rows) == run['rawRows']
    seen, completed = set(), {}
    for r in rows:
        assert r['gpu_status']=='5' and r['stage_owner_match']=='1'
        assert r['stage_simulation_tick']==r['completed_tick']
        assert r['stage_submission_serial']==r['completed_serial']
        for stage in ('as','dispatch','transfer'):
            assert r['gpu_'+stage+'_status']=='1'
            assert r['gpu_'+stage+'_serial']==r['completed_serial']
        key = tuple(r[k] for k in ('stage_scene_epoch','stage_measurement_generation','stage_submission_serial'))
        assert key not in seen
        seen.add(key); completed[int(r['completed_tick'])]=r
    maps[run['label']] = completed
    p={int(r['submitted_tick']):([float(r[k]) for k in
        ('submitted_x','submitted_y','submitted_z','submitted_yaw','submitted_pitch')],r['phase'],r['station']) for r in rows}
    p[run['initialOwner']['simulationTick']] = (run['initialCamera'],'landing-sweep','2')
    poses[run['label']]=p
common = set.intersection(*(set(maps[l]) for l in labels))
matched = [t for t in sorted(common) if poses[labels[0]][t][1] not in ('rope-return','repeat-ascent')]
assert len(matched)==summary['matchedPrefix']['samplesPerRun']==1308
for t in matched:
    assert poses[labels[0]][t]==poses[labels[1]][t]==poses[labels[2]][t]
for phase in summary['matchedPhases']:
    ticks=[t for t in matched if poses[labels[0]][t][1]==phase['phase']]
    assert len(ticks)==phase['samplesPerRun']
    for label in labels:
        for field in ('gpu_ns','gpu_as_ns','gpu_dispatch_ns','gpu_transfer_ns'):
            values=sorted(int(maps[label][t][field])/1e6 for t in ticks)
            expected=phase['runs'][label][field]
            assert statistics.median(values)==expected['median']
            assert values[int(.95*(len(values)-1))]==expected['p95NearestLower']
            assert values[0]==expected['minimum'] and values[-1]==expected['maximum']
    print(phase['phase'],len(ticks),*[round(phase['runs'][l]['gpu_ns']['median'],6) for l in labels])
print('PASS: 4531 unique valid owner joins, 1308 matched prefix samples/run; summary medians/ranges/P95 reproduced.')
