"""Reproduce the fixed private chapter forest payload; no device or release action."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/models/world/source/forest-surroundings-v1'
RUNTIME = ROOT / 'assets/models/world/runtime/forest-surroundings-v1'
NAMES = ('pine-middle.glb', 'alder-middle.glb', 'pine-far.glb', 'alder-far.glb',
         'boundary-panel.glb', 'woodland-low-fern.glb')


def outputs():
    spec = importlib.util.spec_from_file_location('forest_author', SOURCE / 'author.py')
    author = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(author)
    for name, data in author.produce().items():
        if (SOURCE / name).read_bytes() != data:
            raise ValueError('Stale authored source: ' + name)
    files = {name: (SOURCE / 'candidates' / name).read_bytes() for name in NAMES}
    manifest = json.loads((SOURCE / 'candidates/asset.manifest.json').read_text())
    manifest.update(asset='forest-surroundings-v1-runtime', distribution='private-chapter-1.7')
    encode = lambda value: (json.dumps(value, indent=2) + '\n').encode()
    files['asset.manifest.json'] = encode(manifest)
    files['runtime-roster.json'] = encode({
        'schema': 1, 'policy': 'closed-roster', 'asset': 'forest-surroundings-v1',
        'source': 'assets/models/world/source/forest-surroundings-v1',
        'sourceLayoutSha256': hashlib.sha256((SOURCE / 'native-layout.generated.h').read_bytes()).hexdigest(),
        'files': [{'path': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                  for name, data in files.items()],
        'rights': author.RIGHTS,
        'contact': 'Every additional tree has exact rendered lower wood contact; no unreachable-tree exemption.'})
    return files


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--write', action='store_true')
    group.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for name, data in outputs().items():
        path = RUNTIME / name
        if args.check:
            if not path.is_file() or path.read_bytes() != data:
                raise ValueError('Stale private runtime payload: ' + name)
        else:
            if path.is_symlink():
                raise ValueError('Linked runtime destination')
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
    print('Private forest runtime identity passed (8 fixed files).')
