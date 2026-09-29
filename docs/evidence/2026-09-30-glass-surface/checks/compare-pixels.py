"""Read-only exact image differences; unchanged existing pixel-equivalence gate."""
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path('C:/Dev/tmp/horde-glass-barycentric-20260930')
REPO = Path.cwd()  # Run from the engineering repository root.
SCENES = ('lantern-glass-production', 'player-viewmodel-lantern-high-look-up',
          'player-viewmodel-lantern-low-parry', 'glass-edge-fresnel')


def compare(before, after, label):
    with Image.open(before) as left, Image.open(after) as right:
        assert left.size == right.size
        channels = ImageChops.difference(left.convert('RGB'), right.convert('RGB')).split()
        maximum = ImageChops.lighter(ImageChops.lighter(channels[0], channels[1]), channels[2])
        hist = maximum.histogram()
        count = sum(hist[2:])
        pixels = left.width * left.height
        peak = max(i for i, n in enumerate(hist) if n)
        mask = maximum.point([0, 0] + [255] * 254)
        positions = []
        if count:
            bounds = mask.getbbox()
            for y in range(bounds[1], bounds[3]):
                for x in range(bounds[0], bounds[2]):
                    if maximum.getpixel((x, y)) >= max(4, peak - 2) and len(positions) < 16:
                        positions.append({'xy': [x, y], 'before': left.getpixel((x, y)),
                                          'after': right.getpixel((x, y))})
        else:
            bounds = None
        return dict(label=label, before=str(before), after=str(after),
                    beforeSha256=hashlib.sha256(before.read_bytes()).hexdigest(),
                    afterSha256=hashlib.sha256(after.read_bytes()).hexdigest(),
                    size=list(left.size), pixels=pixels, pixelsOverOne=count,
                    fractionOverOne=count/pixels, maximumChannelDifference=peak,
                    equivalencePassed=peak <= 3 and count/pixels <= .001,
                    boundsOverOne=bounds, peakExamples=positions)


results = []
for quality in ('mobile', 'high'):
    for scene in SCENES:
        folder = ROOT/'windows'/quality
        baseline = next((folder/'baseline'/scene).glob('*.png'))
        candidate = next((folder/'candidate'/scene).glob('*.png'))
        compute = next((folder/'compute'/scene).glob('*.png'))
        results.append(compare(baseline, candidate, f'RTX/{quality}/correctness-AB/{scene}'))
        results.append(compare(candidate, compute, f'RTX/{quality}/pipeline-compute/{scene}'))
phone_before = REPO/'reports/android-showcase-runs/run-20260930-064746'
phone_after = REPO/'reports/android-showcase-runs/run-20260930-080757'
for before in sorted(phone_before.glob('capture-*-75.png')):
    results.append(compare(before, phone_after/before.name, f'SM-S948B/correctness-AB/{before.name}'))
results.append(compare(Path('C:/Dev/tmp/horde-glass-no-duplicate-20260930/high-standard/11-finale-roof.png'),
                       ROOT/'windows/high-standard/11-finale-roof.png', 'RTX/High/standard-finale'))
report = dict(pixelTolerance=dict(maximumChannelDifference=3, maximumFractionOverOne=.001),
              note='Correctness AB failures are not automatically regressions. Backend equivalence failures remain open. Phone images are ADB screenshots, not raw cross-backend image parity.',
              results=results)
print(json.dumps(report, indent=2))
