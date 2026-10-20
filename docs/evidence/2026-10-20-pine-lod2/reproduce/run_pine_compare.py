"""Owned hidden native Debug processes; never installs, shows or kills anything."""
from pathlib import Path
import ctypes, hashlib, json, subprocess, sys, time

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
EXE = ROOT/'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
SETTINGS = EXE.parent/'HordeLanternRT.settings.ini'
user32 = ctypes.WinDLL('user32')
user32.GetForegroundWindow.restype = ctypes.c_void_p

def identity(path):
    return {'exists': path.exists(), **({'bytes': path.stat().st_size,
        'sha256': hashlib.sha256(path.read_bytes()).hexdigest()} if path.exists() else {})}

def write(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False)+'\n', encoding='utf-8')

def run(label, checkpoint, query=False, omissions=None):
    destination = OUT/label
    assert not destination.exists(), 'Fresh output required'
    args = [str(EXE), '--development-rescue-journey', '--capture-showcase', str(destination),
            '--capture-hidden', '--development-checkpoint', checkpoint,
            '--inspect-static-world-source', 'ForestPine']
    if query: args += ['--require-rayquery-compute']
    if omissions: args += ['--validate-static-sparse-cutover', ' '.join(map(str, omissions))]
    settings_before = identity(SETTINGS)
    foreground = user32.GetForegroundWindow()
    started = time.monotonic()
    foreground_samples = []
    with (OUT/(label+'.stdout.log')).open('wb') as stdout, (OUT/(label+'.stderr.log')).open('wb') as stderr:
        process = subprocess.Popen(args, cwd=ROOT, stdout=stdout, stderr=stderr,
            creationflags=subprocess.CREATE_NO_WINDOW)
        print(label+' own hidden PID '+str(process.pid), flush=True)
        while process.poll() is None and time.monotonic()-started < 90:
            foreground_samples.append(user32.GetForegroundWindow())
            time.sleep(.25)
        code = process.poll()
    row = {'label': label, 'arguments': args, 'pid': process.pid, 'exitCode': code,
        'pending': code is None, 'deadlineSeconds': 90, 'seconds': time.monotonic()-started,
        'executable': identity(EXE), 'settingsBefore': settings_before, 'settingsAfter': identity(SETTINGS),
        'foregroundBefore': foreground, 'foregroundAfter': user32.GetForegroundWindow(),
        'foregroundSamples': foreground_samples}
    row['settingsUnchanged'] = row['settingsBefore'] == row['settingsAfter']
    row['foregroundApiSamplesUnchanged'] = all(x == foreground for x in foreground_samples) and row['foregroundAfter'] == foreground
    write(OUT/(label+'.process.json'), row)
    print(json.dumps({k: row[k] for k in ['exitCode','pending','seconds','settingsUnchanged','foregroundApiSamplesUnchanged']}), flush=True)
    if code is None: raise RuntimeError('Own process pending; no kill or retry.')
    assert code == 0 and row['settingsUnchanged'] and row['foregroundApiSamplesUnchanged'], label
    manifest = json.loads((destination/'capture-manifest.json').read_text(encoding='utf-8-sig'))
    assert manifest['complete'] and len(manifest['captures']) == 1
    shot = manifest['captures'][0]
    assert shot['checkpoint'] == checkpoint and shot['honestlyPresentedRtFrame']
    assert shot['completedFrame']['pipeline']['executionBackend'] == ('RayQueryCompute' if query else 'RayTracingPipeline')
    assert shot['completedFrame']['actualUploadedMistEnabled'] is True
    assert manifest['presentation']['renderScale'] == 1
    assert identity(destination/shot['file'])['sha256'] == shot['pngSha256']
    inspection = json.loads((destination/'static-world-source.json').read_text())
    assert inspection['selected']['materialName'] == 'ForestPine' and inspection['completeContributors']
    for file in inspection['selected']['files']:
        assert identity(destination/file['name'])['sha256'] == file['sha256']
    if omissions:
        reference = json.loads((OUT/'native-inspection/static-world-source.json').read_text())
        assert inspection['selected'] == reference['selected']
        assert inspection['primitives'] == reference['primitives']
        serials = []
        for phase in ['before','candidate','restored']:
            receipt = json.loads((destination/('static-timing-'+phase+'.json')).read_text())
            assert receipt['complete'] and receipt['sampleCount'] == 60 and receipt['warmFrames'] == 12
            assert len(receipt['samples']) == 60
            assert receipt['dispatchWidth'] == 960 and receipt['dispatchHeight'] == 540
            for sample in receipt['samples']:
                frame = sample['completedFrame']
                serials.append(frame['identity']['submissionSerial'])
                assert frame['benchmarkEligible'] and frame['presentation']['presented']
                assert frame['presentation']['finalIdleCompletion'] and frame['actualUploadedMistEnabled'] is True
        assert len(set(serials)) == 180 and serials == sorted(serials)
        cutover = json.loads((destination/'static-zone-cutover.json').read_text())
        assert len(cutover['hops']) == 2
        assert cutover['hops'][0]['omittedTriangleCount'] == len(omissions)
        assert cutover['hops'][1]['completeContributors']
    print(label+' completed native proof', flush=True)

if sys.argv[1:] == ['inspect']:
    run('native-inspection', 'chapter-pine-mid')
elif sys.argv[1:] == ['compare']:
    omissions = json.loads((OUT/'global-omissions.json').read_text())['globalTriangleIds']
    assert len(omissions) == 768 and omissions == sorted(set(omissions))
    for query in [False, True]:
        for distance in ['near','mid','far']:
            run(distance+('-query' if query else '-pipeline'), 'chapter-pine-'+distance, query, omissions)
else:
    raise SystemExit('usage: run_pine_compare.py inspect|compare')
