"""Publish sanitized diagnostic evidence; never production packages or EXEs.
Run only after prepare.py, prepare.py --extras, captures and verify.py succeed.
Local reports remain intact, including failed version-1 captures/build logs.
"""
import hashlib,json,re,shutil,zipfile,subprocess
from pathlib import Path
root=Path.cwd();out=root/'reports/mist-interval-prototype'
dest=root/'docs/evidence/2026-10-10-mist-interval-prototype'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p,d):
    p=Path(p);p.parent.mkdir(parents=True,exist_ok=True)
    p.write_text(json.dumps(d,indent=2)+'\n',encoding='utf-8',newline='\n')
def clean(s):
    for r in [str(root),root.as_posix(),str(root).replace('\\','\\\\')]:s=s.replace(r,'<repo>')
    # Remove entire personal absolute paths, including escaped log spellings.
    s=re.sub(r'C:[/\\]+Users[/\\]+[^\r\n"<>]+','<local-path>',s,flags=re.I)
    return re.sub(r'<user-home>[^\r\n"]*','<local-path>',s)
def copytext(src,dst):
    dst.parent.mkdir(parents=True,exist_ok=True)
    text=clean(src.read_text(encoding='utf-8-sig',errors='replace'))
    if dst.suffix=='.log':text='\n'.join(line.rstrip() for line in text.splitlines()).rstrip()+'\n'
    dst.write_text(text,encoding='utf-8',newline='\n')
exe=sha(out/'build/Debug/frozen_mist.exe')
matrix=json.loads((out/'full-metrics.json').read_text());budgets=json.loads((root/'tools/raygen-variant-budgets.json').read_text())
catalog=json.loads((root/'tools/raygen-variant-catalog.json').read_text())
baseline=[]
for entry in catalog['variants']:
    record=json.loads((out/'baseline-matrix'/entry['key']/'raygen-stats.json').read_text())
    assert record['compiledSpirvSha256']==entry['spirvSha256']
    baseline.append(record)
write(dest/'baseline-full-metrics.json',baseline)
for row in matrix:
    entry=next(e for e in catalog['variants'] if e['key']==row['key'])
    assembly=(out/'shaders'/row['name']/'shader.spvasm').read_text()
    observed={k:entry[k] for k in ['instrumentation','quality','material','strategy','shippingAllowed']}
    observed.update(atomicInstructions=row['atomicInstructions'],
        hasDiagnosticsBinding=bool(re.search(r'OpDecorate\s+%\S+\s+Binding\s+22\b',assembly)),
        driverSafeFullyInlined=row['functions']==1 and row['functionCalls']==0 and row['rayQueryInitializations']<=29,
        boundedGenericFunctionsRetained=row['functions']>1 and row['functionCalls']>0 and row['rayQueryInitializations']<=3)
    policy=next(b for b in budgets['budgets'] if b['key']==row['key'])
    row['measured_exact_contract']=observed
    row['exact_violations']={k:dict(expected=v,observed=observed[k]) for k,v in policy['exact'].items() if observed[k]!=v}
    row['extra_diagnostic_bindings']=[28,29]
write(dest/'full-metrics.json',matrix)
for name in ['shader-metrics.json','input-identities.json','correctness.json','analytic-tests.json','tool-binaries.json','cones.json']:
    copytext(out/name,dest/name)
write(dest/'unchanged-authorities.json',dict(source_head='5febb0ded2f5da0c930dc8a2adb1a2e38622ad34',
    budgets=budgets,toolchain=catalog['toolchain'],generator=catalog['generator'],
    hashes={p:sha(root/p) for p in ['tools/raygen-variant-budgets.json','tools/raygen-variant-catalog.json',
    'tools/raygen-variants.json','src/vulkan/raytracing/RtSceneAbi.def','shaders/raytracing/include/rt_atmosphere.glsl',
    'src/vulkan/raytracing/PresentableTinyRtScene.cpp','src/platform/windows/DiagnosticWindow.cpp']}))
all_runs=[];selected=[]
shader_rows={r['name']:r for r in json.loads((out/'shader-metrics.json').read_text())}
for p in sorted((out/'captures').iterdir()):
    if not (p/'run.json').exists():continue
    r=json.loads((p/'run.json').read_text());f=r['completed_frame'];log=(p/'capture.log').read_text(errors='replace')
    failures=[s for s in log.splitlines() if 'capture failed:' in s or 'Scene initialization failed' in s]
    all_runs.append(dict(name=p.name,source_head=r['source_head'],argv=r['argv'],environment=r.get('environment'),
        elapsed_seconds=r['elapsed_seconds'],exit_code=r['exit_code'],exe_sha256=r['exe_sha256'],
        shader=r['shader'],presentation=f and f['presentation'],gpu=f and f['gpu'],files=r['files'],
        failure=clean(failures[-1]) if failures else None))
    current=r['exe_sha256']==exe and r['shader']['spirv_sha256']==shader_rows[r['shader']['name']]['spirv_sha256']
    if not current:continue
    assert f and f['presentation']['presented'] and f['pipeline']['activeSha256']==r['shader']['spirv_sha256']
    target=dest/'captures'/p.name;target.mkdir(parents=True,exist_ok=True)
    for name in ['run.json','completed-frame.json','frozen-state.json','native-resources.json','capture.log']:
        if name.endswith('.json'):
            data=(p/name).read_bytes()
            assert b'C:/Users/' not in data and b'C:\\Users\\' not in data,(p,name)
            shutil.copy2(p/name,target/name) # preserve immutable native newline bytes
        else:copytext(p/name,target/name)
    shutil.copy2(p/'192-rescue-journey-start.png',target/'192-rescue-journey-start.png')
    selected.append(p.name)
assert len(selected)==24,('Exactly16 matched core and8 adjacent-view captures',len(selected))
write(dest/'runs.json',all_runs)
for name in ['current-comparison.png','higher-comparison.png']:
    shutil.copy2(out/name,dest/name)
# Preserve failed setup/capture logs, without private paths or desktop content.
for p in (out/'captures').iterdir():
    if (p/'capture.log').exists():copytext(p/'capture.log',dest/'logs'/'captures'/(p.name+'.log'))
for p in out.glob('*.log'):
    if 'private' not in p.name:copytext(p,dest/'logs'/p.name)
failed=out/'version1-invalid-cellwidth'
for name in ['full-metrics.json','shader-metrics.json','input-identities.json','correctness.json','native-resources.json']:
    copytext(failed/name,dest/'failed-version1'/name)
write(dest/'failed-version1/disposition.json',dict(status='FAILED weighted validation; not a correction',
    executable_sha256=sha(failed/'frozen_mist.exe'),cause='Cell-width insertion missed all three sampleCount branches; GPU weighted fraction0 where CPU expected0.0380447224',
    geometric_discovery_only=True,colour_strength_valid=False,original_files_retained_locally=True))
# Exact full resolved/preprocessed GLSL, raw/optimized SPIR-V and disassembly.
# Ordinary zip is a diagnostic code artifact, never a shipped shader package.
archive=dest/'diagnostic-artifacts.zip';entries={}
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for base,patterns in [(out/'baseline-matrix',['*.resolved','*.preprocessed','*.spv','*.spvasm']),
                           (out/'shaders',['source.rgen','raw.spv','shader.spv','shader.spvasm']),
                           (failed/'shaders',['source.rgen','raw.spv','shader.spv','shader.spvasm'])]:
        for folder in sorted(p for p in base.iterdir() if p.is_dir()):
            for pattern in patterns:
                for p in folder.glob(pattern):
                    data=p.read_bytes();assert b'C:/Users/' not in data and b'C:\\Users\\' not in data,p
                    rel=p.relative_to(out).as_posix();z.writestr(rel,data);entries[rel]=sha(p)
write(dest/'diagnostic-artifact-manifest.json',dict(archive_sha256=sha(archive),entries=entries))
cap=json.loads((out/'build/Debug/reports/vulkan_capability_report.json').read_text())
# Explicit hardware allowlist, no device identifiers/UUIDs from the raw report.
hardware=dict(gpu=cap['gpuName'],device_class='Windows RTX laptop',driver=cap['driverProperties']['driverInfo'],
    vulkan_api=cap['vulkanApiVersionText'],actual_backend='RayTracingPipeline',
    actual_profile='Diagnostic / High / GenericDielectric',presentation_source='selected completed-frame records; capability report alone is not presentation')
write(dest/'receipt.json',dict(schema=1,status='WINDOWS_DIAGNOSTIC_ONLY; production admission blocked',
    source_head='5febb0ded2f5da0c930dc8a2adb1a2e38622ad34',branch='codex/horde-1.7-wp2-vertical-proof',
    production_exe_sha256=sha(root/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'),
    diagnostic_exe_sha256=exe,hardware=hardware,selected_captures=selected,
    protected_source_changed=False,production_budgets_changed=False,production_packages_changed=False,
    six_point_reference_shipped=False,android_tested=False,rayquery_compute_tested=False,
    native_dynamic_rope_tested=False,owner_visual_acceptance=False,
    numeric_discovery_and_weighted_subset='PASS',surface_before_mist='EXACT RGBA MATCH both qualities',
    original_showcase_admission='FAIL all selected captures: retained low-lantern framing/grip rule',
    version1_weighting='FAIL; retained',production_static_contract='FAIL all8; unchanged limits',
    audio_haptic_manual_revalidation_required=False))
print(json.dumps(dict(selected_captures=len(selected),all_attempts=len(all_runs),archive_bytes=archive.stat().st_size,
    diagnostic_exe_sha256=exe),indent=2))
