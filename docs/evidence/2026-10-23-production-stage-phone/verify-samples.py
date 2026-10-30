"""Reproduce sealed CSV arithmetic; this does not repeat device admission."""
from pathlib import Path
import csv
import hashlib
import json
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from analyze_android_rt_stage_profile import summary

receipt = json.loads((HERE / 'receipt.json').read_text())
stage = json.loads((HERE / 'stage-analysis.json').read_text())
cpu = json.loads((HERE / 'cpu-analysis.json').read_text())
raw = (HERE / 'resources.csv').read_bytes().replace(b'\r\n', b'\n')
assert hashlib.sha256(raw).hexdigest() == receipt['resourcesCanonicalLfSha256']
window_raw = (HERE / 'sample-window.csv').read_bytes().replace(b'\r\n', b'\n')
assert hashlib.sha256(window_raw).hexdigest() == receipt['sampleWindowSha256']
rows = list(csv.DictReader(raw.decode().splitlines()))
windows = list(csv.DictReader(window_raw.decode().splitlines()))
assert len(rows) == len(windows) == stage['joinedFrames'] == 838
selected = []
start = receipt['outdoorHold']['startWallNanoseconds']
for row, window in zip(rows, windows):
    assert row['serial'] == window['serial'] and row['tick'] == window['tick']
    sample = start + 10_000_000_000 <= int(window['owner_wall_ns']) <= start + 40_000_000_000
    assert sample == (window['sample_10_to_40_seconds'] == '1')
    assert row['cpu_stage_status'] == 'valid'
    for name in ['as', 'dispatch', 'transfer']:
        assert row[name + '_gpu_status'] == 'valid'
        assert row[name + '_gpu_serial'] == row['serial']
    if sample:
        selected.append(row)
assert len(selected) == cpu['sampleFrames'] == 626
for name in ['as', 'dispatch', 'transfer']:
    assert summary([int(r[name + '_gpu_ns']) for r in selected]) == stage['views']['lookout-sweep'][name]
    assert summary([int(r[name + '_gpu_ns']) for r in rows]) == stage['allFrames'][name]
for metric, expected in cpu['cpuStageWallTimings'].items():
    assert summary([int(r[metric]) for r in selected]) == expected
print('838 retained rows; 626 owner-wall-selected samples; GPU/CPU arithmetic and hashes match.')
