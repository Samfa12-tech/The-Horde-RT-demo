"""Package only the budget-passing private Shipping/Mobile Opaque experiment pair.

Generic's rejected eight-policy results remain evidence; no production artifact is published.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess


def compile_pair(folder):
    root = Path(__file__).resolve().parents[1]
    folder = folder.resolve()
    if not folder.is_relative_to(root/'reports'):
        raise ValueError('Private reports folder required')
    source = folder/'source'
    key = 'shipping_mobile_opaque_fast'
    compiled = source/'reports/inverse-matrix'/key
    stats = json.loads((compiled/'raygen-stats.json').read_text(encoding='utf-8-sig'))
    budgets = json.loads((root/'tools/raygen-variant-budgets.json').read_text())['budgets']
    ceiling = next(r['max'] for r in budgets if r['key'] == key)
    assert all(stats[k] <= v for k, v in ceiling.items()), 'Selected variant must pass every frozen limit'
    sdk = Path('C:/VulkanSDK/1.4.350.0/Bin')
    log = folder/'compile-private-pair.log'
    if log.exists(): raise ValueError('Refuse to overwrite compiler evidence')

    def run(args):
        result = subprocess.run([str(a) for a in args], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        with log.open('ab') as stream: stream.write(result.stdout)
        if result.returncode: raise RuntimeError(f'Compiler failed; see {log}')

    compute = (compiled/'minimal.rgen.resolved').read_text()
    compute = compute.replace('#extension GL_EXT_ray_tracing : require', '')
    compute = compute.replace('#define HORDE_RT_PIXEL_ID gl_LaunchIDEXT',
                              '#define HORDE_RT_COMPUTE_ENTRY\n#define HORDE_RT_PIXEL_ID gl_GlobalInvocationID')
    compute = compute.replace('#define HORDE_RT_PIXEL_EXTENT gl_LaunchSizeEXT',
                              '#define HORDE_RT_PIXEL_EXTENT imageSize(outputImage)\nlayout(local_size_x=8,local_size_y=8,local_size_z=1) in;')
    compute_source = compiled/'minimal.comp.resolved'
    compute_source.write_text(compute, encoding='utf-8', newline='\n')
    run([sdk/'glslangValidator.exe', '-V', '--target-env', 'vulkan1.2', '-S', 'comp', '-Os',
         '-o', compiled/'minimal.comp.raw.spv', compute_source])
    run([sdk/'spirv-opt.exe', '-O', compiled/'minimal.comp.raw.spv', '-o', compiled/'minimal.comp.spv'])
    records = []
    for stage in ('rgen', 'comp'):
        spv = compiled/f'minimal.{stage}.spv'
        run([sdk/'spirv-val.exe', '--target-env', 'vulkan1.2', spv])
        assembly = compiled/f'minimal.{stage}.spvasm'
        if stage == 'comp': run([sdk/'spirv-dis.exe', spv, '-o', assembly])
        asm = assembly.read_text()
        assert not re.search(r'\bOpAtomic\w+|\bBinding 22\b|OpReadClockKHR', asm)
        assert len(re.findall(r'\bOpFunction\b', asm)) == 1 and 'OpFunctionCall' not in asm
        assert len(re.findall(r'\bOpRayQueryInitializeKHR\b', asm)) == stats['rayQueryInitializations']
        assert ' MatrixInverse ' not in asm
        name = key if stage == 'rgen' else 'rayquery_compute_'+key
        data = spv.read_bytes()
        words = struct.unpack('<'+'I'*(len(data)//4), data)
        text = '\n'.join(', '.join(f'0x{w:08x}u' for w in words[i:i+8])+',' for i in range(0,len(words),8))+'\n'
        inc = source/'src/vulkan/raytracing/variants'/(name+'.inc')
        inc.write_text(text, encoding='utf-8', newline='\n')
        header = source/'src/vulkan/raytracing'/('RtPipelineVariantCatalog.generated.h' if stage=='rgen' else 'RtRayQueryVariantCatalog.generated.h')
        old = header.read_text()
        pattern = rf'("{name}", "src/vulkan/raytracing/variants/{name}\.inc", )"[0-9a-f]+", "[0-9a-f]+", \d+, 0, false'
        updated, count = re.subn(pattern, lambda m: m[1]+f'"{hashlib.sha256(data).hexdigest()}", "{hashlib.sha256(text.encode()).hexdigest()}", {len(words)}, 0, false', old)
        assert count == 1
        header.write_text(updated, encoding='utf-8', newline='\n')
        records.append({'key': name, 'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data),
                        'matrixInverseSites': 0, 'worldToObjectSites': asm.count('OpRayQueryGetIntersectionWorldToObjectKHR')})
    (folder/'private-modules.json').write_text(json.dumps({'modules': records, 'productionAdoption': False,
        'scope': 'Only Shipping/Mobile Opaque changes in the executable; Generic and other policies retain exact baseline artifacts.'}, indent=2))
    print(json.dumps(records, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    compile_pair(parser.parse_args().folder)
