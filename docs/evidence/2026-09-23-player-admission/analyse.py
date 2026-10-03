import hashlib
import json
import sys
from pathlib import Path

from PIL import Image, ImageChops

root = Path(sys.argv[1])
output = Path(sys.argv[2])
if output.exists():
    raise RuntimeError('Comparison output must be new')
checkpoints = ['player-body-grips', 'player-body-forward', 'player-body-owner-feedback',
               'player-body-downward-cut', 'player-body-upward-slice']
controls = ['schemaVersion', 'source', 'sceneOnly', 'overlaysIncluded', 'settlingFrames',
            'fixedAnimationTimeSeconds', 'buildId', 'executionBackend',
            'selectedRtPipelineBundle', 'device', 'presentation']
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
rows = []
for checkpoint in checkpoints:
    folders = [root / side / checkpoint for side in ['baseline', 'candidate']]
    manifests = [json.loads((folder / 'capture-manifest.json').read_text(encoding='utf-8-sig')) for folder in folders]
    before, after = manifests
    for manifest in manifests:
        assert manifest['complete'] and manifest['source'] == 'rt-storage-image'
        assert manifest['sceneOnly'] and not manifest['overlaysIncluded']
        assert manifest['executionBackend'] == 'RayTracingPipeline'
        assert len(manifest['captures']) == 1 and manifest['captures'][0]['checkpoint'] == checkpoint
    for control in controls:
        assert before[control] == after[control], f'{checkpoint}: mismatched {control}'
    paths = [folder / manifest['captures'][0]['file'] for folder, manifest in zip(folders, manifests)]
    images = [Image.open(path).convert('RGB') for path in paths]
    assert images[0].size == images[1].size
    width, height = images[0].size
    for manifest in manifests:
        assert manifest['captures'][0]['width'] == width and manifest['captures'][0]['height'] == height
    delta = ImageChops.difference(images[0], images[1])
    maximum = max(channel[1] for channel in delta.getextrema())
    different = sum(max(pixel) > 1 for pixel in delta.getdata())
    fraction = different / (height * width)
    rows.append(dict(checkpoint=checkpoint, width=width, height=height,
                     baselinePngSha256=digest(paths[0]), candidatePngSha256=digest(paths[1]),
                     baselineManifestSha256=digest(folders[0] / 'capture-manifest.json'),
                     candidateManifestSha256=digest(folders[1] / 'capture-manifest.json'),
                     maximumChannelDifference=maximum, pixelsDifferentByMoreThanOne=different,
                     differentFraction=fraction, passed=maximum <= 3 and fraction <= .001))
report = dict(schema=1, scope='Matched frozen native RT player-asset admission images, not motion, performance, backend parity or owner acceptance',
              controls={key: before[key] for key in controls},
              imageGate=dict(maximumChannelDifference=3, maximumFractionDifferentByMoreThanOne=.001),
              comparisons=rows, passed=all(row['passed'] for row in rows))
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
sys.exit(0 if report['passed'] else 1)
