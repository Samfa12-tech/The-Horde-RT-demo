"""One bounded static experiment; no production source/package writes.

Inputs are the exact resolved prototype retained by the previous reproduce
recipe. Run that recipe first if reports/mist-interval-prototype is absent.
Only Diagnostic High Generic and Opaque are compiled until both pass admission.
"""
import hashlib
import json
import re
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
OLD = ROOT / 'docs/evidence/2026-10-10-mist-interval-prototype'
TEMP = ROOT / 'reports/mist-interval-prototype'
OUT = ROOT / 'reports/mist-interval-feasibility/static-common-cell'
SDK = Path('C:/VulkanSDK/1.4.350.0/Bin')
PASSES = {
    'GenericRetained': ['--eliminate-dead-functions', '--eliminate-dead-code-aggressive',
                        '--simplify-instructions', '--eliminate-dead-branches', '--cfg-cleanup'],
    'LegacyInlined': ['-O'],
}
PATTERNS = {
    'instructions': r'(?m)^\s*(?:%\S+\s*=\s*)?Op\w+',
    'branchOperations': r'\bOp(?:Branch|BranchConditional|Switch)\b',
    'loops': r'\bOpLoopMerge\b',
    'selectionMerges': r'\bOpSelectionMerge\b',
    'functions': r'\bOpFunction\b',
    'functionCalls': r'\bOpFunctionCall\b',
    'rayQueryInitializations': r'\bOpRayQueryInitializeKHR\b',
    'atomicInstructions': r'\bOpAtomic\w+\b',
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding='utf-8', newline='\n')


def consolidate(source):
    """Retain sample centres, ordered integration and exact overlap formula.

    Only replace the three unrolled density branches with one ordered loop.
    Discovery, sort, raw/callback capacities, union and overflow remain intact.
    """
    start = source.index('    if (controls.workloadPreset < 0.5)',
                         source.index('vec4 lichGroundMist('))
    end = source.index('    return vec4(scattered, transmittance);', start)
    original = source[start:end]
    assert original.count('integrateLichMistSample(') == 16
    assert original.count('mistCellLength=stepLength;') == 3
    body = '''    float sampleCount = controls.workloadPreset < 0.5 ? 2.0
        : (controls.workloadPreset < 1.5 ? 6.0 : 8.0);
    float stepLength = (marchEnd - marchStart) / sampleCount;
    mistCellLength=stepLength;
    if(!(stepLength>0.0) || isnan(stepLength) || isinf(stepLength)) {
        mistIntervalInvalid=true;return vec4(1.0,0.0,1.0,0.0);
    }
    for (int sampleIndex=0; sampleIndex<int(sampleCount); ++sampleIndex)
    {
        integrateLichMistSample(lichMistSample(rayOrigin + rayDirection
            * (marchStart + stepLength * (float(sampleIndex) + 0.5)), sources),
            stepLength, scattered, transmittance);
    }
'''
    return source[:start] + body + source[end:]


def run(label, args, cwd):
    started = time.perf_counter()
    result = subprocess.run([str(x) for x in args], cwd=cwd,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    raw = result.stdout.decode('utf-8', errors='replace')
    safe = raw.replace(str(ROOT), '<repo>').replace(ROOT.as_posix(), '<repo>')
    safe = re.sub(r'C:[/\\]+Users[/\\]+[^/\\\s"<>]+', '<user-home>', safe, flags=re.I)
    write(cwd / (label + '.log'), safe)
    row = dict(stage=label, argv=[str(x).replace(str(ROOT), '<repo>') for x in args],
               cwd=str(cwd.relative_to(ROOT)), exit=result.returncode,
               seconds=time.perf_counter()-started, log_sha256=sha(cwd/(label+'.log')))
    with (OUT/'runs.jsonl').open('a', encoding='utf-8') as log:
        log.write(json.dumps(row)+'\n')
    assert result.returncode == 0, (label, safe[-1500:])


def compile_row(key, source_path, strategy, budget):
    folder = source_path.parent
    legacy = strategy == 'LegacyInlined'
    run('compile', [SDK/'glslangValidator.exe', '-V', '--target-env', 'vulkan1.2',
                    *(['-Os'] if legacy else []), '-S', 'rgen', '-o',
                    'raw.spv', 'source.rgen'], folder)
    run('optimize', [SDK/'spirv-opt.exe', *PASSES[strategy], 'raw.spv', '-o', 'shader.spv'], folder)
    run('validate', [SDK/'spirv-val.exe', '--target-env', 'vulkan1.2', 'shader.spv'], folder)
    run('disassemble', [SDK/'spirv-dis.exe', 'shader.spv', '-o', 'shader.spvasm'], folder)
    asm = (folder/'shader.spvasm').read_text()
    metrics = {name: len(re.findall(pattern, asm)) for name, pattern in PATTERNS.items()}
    metrics.update(bytes=(folder/'shader.spv').stat().st_size,
                   words=(folder/'shader.spv').stat().st_size//4)
    exact = dict(instrumentation='Diagnostic', quality='High',
                 material='OpaqueFast' if legacy else 'GenericDielectric', strategy=strategy,
                 shippingAllowed=False, atomicInstructions=metrics['atomicInstructions'],
                 hasDiagnosticsBinding=bool(re.search(r'OpDecorate %\S+ Binding 22\b', asm)),
                 driverSafeFullyInlined=metrics['functions']==1 and metrics['functionCalls']==0
                    and metrics['rayQueryInitializations']<=29,
                 boundedGenericFunctionsRetained=metrics['functions']>1 and metrics['functionCalls']>0
                    and metrics['rayQueryInitializations']<=3)
    return dict(key=key, source_sha256=sha(source_path), raw_spirv_sha256=sha(folder/'raw.spv'),
                spirv_sha256=sha(folder/'shader.spv'), disassembly_sha256=sha(folder/'shader.spvasm'),
                metrics=metrics, measured_exact=exact,
                excess={name:metrics[name]-limit for name,limit in budget['max'].items()
                        if metrics[name]>limit},
                exact_violations={name:dict(expected=value, observed=exact[name])
                                  for name,value in budget['exact'].items() if exact[name]!=value})


def surface_only(source):
    start=source.index('    vec4 lichMist = lichGroundMist(')
    end=source.index('    imageStore(outputImage,',start)
    return source[:start]+source[end:]


def main():
    # Never overwrite a preceding attempted candidate or its failed evidence.
    assert not OUT.exists(), 'preserve earlier run: select a new explicit output name'
    OUT.mkdir(parents=True)
    tools = json.loads((OLD/'tool-binaries.json').read_text())
    for tool in tools:
        assert sha(SDK/tool['tool']) == tool['sha256'], ('toolchain drift', tool['tool'])
    before = {row['key']:row for row in json.loads((OLD/'full-metrics.json').read_text())}
    budgets = {row['key']:row for row in json.loads((ROOT/'tools/raygen-variant-budgets.json').read_text())['budgets']}
    rows=[]
    for key,strategy in [('diagnostic_high_generic_dielectric','GenericRetained'),
                         ('diagnostic_high_opaque_fast','LegacyInlined')]:
        original = TEMP/'shaders'/(key+'-interval')/'source.rgen'
        assert sha(original) == before[key]['source_sha256'], ('prototype source drift', key)
        stats=json.loads((TEMP/'baseline-matrix'/key/'raygen-stats.json').read_text())
        for dep in stats['dependencies']:
            normalized='\n'.join((ROOT/dep['path']).read_text().splitlines())+'\n'
            assert hashlib.sha256(normalized.encode()).hexdigest()==dep['sha256'], dep['path']
        source=consolidate(original.read_text())
        folder=OUT/key
        write(folder/'source.rgen',source)
        # The ordinary scene visibility and interval discovery functions are
        # byte-identical source. Their distinct static initializers are NOT solved.
        row=compile_row(key,folder/'source.rgen',strategy,budgets[key])
        row['before']=before[key]
        row['delta_from_prototype']={name:row['metrics'][name]-before[key][name] for name in PATTERNS}
        row['delta_from_prototype']['bytes']=row['metrics']['bytes']-before[key]['bytes']
        row['generic_third_query_site']='UNRESOLVED; cone AS initialization remains distinct'
        rows.append(row)
        print(key,row['metrics'],row['excess'],flush=True)
    write(OUT/'full-metrics.json',json.dumps(rows,indent=2)+'\n')
    write(OUT/'input-identities.json',json.dumps(dict(evidence_reference='3cac217f78bd000925a534823f7076c5fc186090',
        tools=tools,budget_sha256=sha(ROOT/'tools/raygen-variant-budgets.json'),
        prototype_metrics_sha256=sha(OLD/'full-metrics.json'),
        compiler_script_sha256=sha(ROOT/'tools/compile-raygen.ps1'),
        experiment_script_sha256=sha(Path(__file__)), passes=PASSES),indent=2)+'\n')
    folder=OUT/'common-cell-surface'
    write(folder/'source.rgen',surface_only((OUT/'diagnostic_high_generic_dielectric'/'source.rgen').read_text()))
    row=compile_row('diagnostic_high_generic_dielectric',folder/'source.rgen',
                    'GenericRetained',budgets['diagnostic_high_generic_dielectric'])
    previous=TEMP/'shaders/interval-surface/shader.spv'
    row['historical_surface_spirv_sha256']=sha(previous)
    row['byte_identical_to_historical_surface_module']=(folder/'shader.spv').read_bytes()==previous.read_bytes()
    row['new_gpu_pixels_tested']=False
    write(OUT/'surface-module-preservation.json',json.dumps(row,indent=2)+'\n')
    assert row['byte_identical_to_historical_surface_module'], 'protected surface-only module differs'
    assert not all(not row['excess'] and not row['exact_violations'] for row in rows), \
        'unexpected static fit: owner gate requires further bounded checks before GPU measurement'


if __name__=='__main__':
    main()
