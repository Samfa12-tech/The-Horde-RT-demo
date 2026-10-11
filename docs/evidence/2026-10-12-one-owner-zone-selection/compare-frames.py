import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops

root = Path.cwd()
rows = []
for backend in ('pipeline', 'query'):
    candidate = root / f'reports/zone-one-owner-visual/bend-{backend}'
    prior = root / f'reports/gpu-buffer-batch-visual/bend-{backend}'
    a = json.loads((candidate / 'capture-manifest.json').read_text())
    b = json.loads((prior / 'capture-manifest.json').read_text())
    assert a['complete'] and b['complete']
    assert a['executionBackend'] == b['executionBackend']
    assert a['selectedRtPipelineBundle'] == b['selectedRtPipelineBundle']
    frame, previous = a['captures'][0], b['captures'][0]
    for field in ('camera', 'rescueTraversal', 'width', 'height'):
        assert frame[field] == previous[field], field
    assert frame['honestlyPresentedRtFrame'] and previous['honestlyPresentedRtFrame']
    ca, cb = candidate / frame['file'], prior / previous['file']
    with Image.open(ca) as ia, Image.open(cb) as ib:
        delta = ImageChops.difference(ia.convert('RGB'), ib.convert('RGB'))
        pixels = list(delta.getdata())
        rows.append({'backend': a['executionBackend'], 'changedPixels': sum(any(p) for p in pixels),
            'maximumChannelDifference': max(max(p) for p in pixels),
            'candidateSha256': hashlib.sha256(ca.read_bytes()).hexdigest(),
            'parentSha256': hashlib.sha256(cb.read_bytes()).hexdigest(),
            'parentSource': '37310bd5', 'camera': frame['camera']})
(root / 'reports/zone-one-owner-frame-comparisons.json').write_text(json.dumps(rows, indent=2) + '\n')
print(json.dumps(rows, indent=2))
