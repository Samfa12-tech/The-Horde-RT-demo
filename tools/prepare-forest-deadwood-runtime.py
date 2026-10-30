"""Copy verified tangent derivatives into a fresh closed runtime revision.

Never overwrites a runtime/master/backup. Blender processing is separately
recorded under the source revision; this step performs no geometry generation.
"""
import argparse
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output-dir', default='assets/models/world/runtime/forest-deadwood-v1')
args = parser.parse_args()
source = root / 'assets/models/world/source/forest-deadwood-v1/candidates'
out = (root / args.output_dir).resolve()
out.relative_to(root)
out.mkdir(exist_ok=False)
for name in ['horde-fallen-log.glb', 'horde-root-tangle.glb', 'horde-gnarled-stump.glb', 'horde-dead-snag-lod2.glb']:
    data = (source / name).read_bytes()
    receipt = json.loads((source / (name + '.tangent-receipt.json')).read_text())
    if hashlib.sha256(data).hexdigest() != receipt['outputSha256']:
        raise ValueError('Candidate hash disagrees with Blender tangent receipt: ' + name)
    with (out / name).open('xb') as f:
        f.write(data)
manifest = json.loads((source / 'asset.manifest.json').read_text())
manifest['distribution'] = 'runtime'
manifest['licenceStatus'] = 'Original supplied log/root and paid-plan Meshy stump with confirmed public Horde rights; Quaternius CC0 dead-snag geometry. Separate original maps/notices and exact derivative receipts remain at assets/models/world/source/forest-deadwood-v1; see ASSET_LICENSES.md. No blanket MIT/CC0 claim.'
(out / 'asset.manifest.json').write_text(json.dumps(manifest, indent=2) + chr(10), newline=chr(10))
print(out)
