"""Prepare one private inverse-transform candidate; never publish shader artifacts."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

OLD = 'mat3 normalToWorld = transpose(inverse(objectToWorld));'
NEW = 'mat3 normalToWorld = transpose(mat3(rayQueryGetIntersectionWorldToObjectEXT(query, true)));'
HIT_DECODE = Path('shaders/raytracing/include/rt_hit_decode.glsl')


def prepare(source, output):
    root = Path(__file__).resolve().parents[1]
    source, output = source.resolve(), output.resolve()
    if not output.is_relative_to(root/'reports') or output.exists():
        raise ValueError('Choose a new private reports directory; no overwrite')
    original = (source/HIT_DECODE).read_text(encoding='utf-8')
    if original.count(OLD) != 1 or 'rtClockSwitch' in original:
        raise ValueError('Expected exactly one original normal transform and no clock observer')
    if 'uvec4 pages[8];' not in (source/'shaders/raytracing/include/rt_scene_abi.generated.glsl').read_text():
        raise ValueError('Expected the admitted region-page ABI')
    shutil.copytree(source, output/'source')
    (output/'source'/HIT_DECODE).write_text(original.replace(OLD, NEW), encoding='utf-8', newline='\n')
    receipt = {
        'nativeBaseHead': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root).decode().strip(),
        'shaderProfile': 'Explicit private region-page input; not the current production shader tree',
        'shaderProfileReceipt': json.loads((source.parent/'source-receipt.json').read_text())
                                if (source.parent/'source-receipt.json').exists() else None,
        'source': str(source), 'changedFile': str(HIT_DECODE), 'old': OLD, 'new': NEW,
        'maximumCandidates': 1, 'productionAdoption': False,
        'metric': 'matched held-view dispatch GPU median milliseconds; lower is better',
        'acceptance': 'No quality/capacity/budget changes, normal/tangent correctness and matched images; '
                      'any measured gain needs uninstrumented warm controls, not static instruction counts.',
        'sourceShaderHashes': {str(p.relative_to(source)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in (source/'shaders').rglob('*') if p.is_file()},
        'candidateShaderHashes': {str(p.relative_to(output/'source')): hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in (output/'source/shaders').rglob('*') if p.is_file()},
        'productionBudgetSha256': hashlib.sha256((root/'tools/raygen-variant-budgets.json').read_bytes()).hexdigest(),
    }
    (output/'source-receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    return output/'source'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    print(prepare(args.source, args.output))
