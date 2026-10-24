import json, os, subprocess, time, uuid
from pathlib import Path
root = Path.cwd()
out = root / 'reports/kit-debug-journey-organization-20261024'
out.mkdir(parents=True, exist_ok=True)
exe = root / 'build/presets/windows-x64-debug/Debug/horde_rt_campaign_play_session_tests.exe'
release = root / 'build/presets/windows-x64-release/Release/horde_rt_campaign_play_session_tests.exe'
base = root / 'build/presets/windows-x64-debug/Testing/horde-earned-journey-Debug'
ctest = Path(r'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe')
token = uuid.uuid4().hex
env = os.environ.copy()
env['HORDE_RT_EARNED_JOURNEY_RUN'] = token
fixture = base / token
rows = []

def run(label, args, expected=0, scope=env):
    start = time.monotonic()
    r = subprocess.run([str(a) for a in args], env=scope, capture_output=True, text=True, timeout=60)
    (out / (label + '.log')).write_text(r.stdout + r.stderr, encoding='utf-8')
    rows.append(dict(label=label, expected=expected, actual=r.returncode, seconds=round(time.monotonic()-start, 3)))
    assert r.returncode == expected, (label, r.returncode, r.stdout[-500:], r.stderr[-500:])
    return r

run('guard-missing', [exe, '--consume-earned-fixture'], 1)
run('guard-real-producer', [exe, '--produce-earned-fixture'])
assert fixture.is_dir()
original = {p.name: p.read_bytes() for p in fixture.iterdir()}
assert set(original) == {'owner.txt', 'ready.txt', 'slot-1-manual.hrd', 'slot-1-auto.hrd'}
run('guard-stale-producer', [exe, '--produce-earned-fixture'], 1)
assert original == {p.name: p.read_bytes() for p in fixture.iterdir()}
(fixture/'ready.txt').write_bytes(original['ready.txt'].replace(b'horde-earned', b'other-earned', 1))
run('guard-receipt-mismatch', [exe, '--consume-earned-fixture'], 1)
(fixture/'ready.txt').write_bytes(original['ready.txt'])
b = bytearray(original['slot-1-auto.hrd'])
b[-1] ^= 1
(fixture/'slot-1-auto.hrd').write_bytes(b)
run('guard-payload-tamper', [exe, '--consume-earned-fixture'], 1)
(fixture/'slot-1-auto.hrd').write_bytes(original['slot-1-auto.hrd'])
(fixture/'owner.txt').write_bytes(original['owner.txt'] + b'wrong-build\n')
run('guard-build-mismatch', [exe, '--consume-earned-fixture'], 1)
(fixture/'owner.txt').write_bytes(original['owner.txt'])
other = uuid.uuid4().hex
env_other = env.copy()
env_other['HORDE_RT_EARNED_JOURNEY_RUN'] = other
otherroot = base/other
otherroot.mkdir()
for name, data in original.items():
    (otherroot/name).write_bytes(data)
(otherroot/'owner.txt').write_bytes(original['owner.txt'].replace(token.encode(), other.encode()))
run('guard-cross-run-receipt', [exe, '--consume-earned-fixture'], 1, env_other)
run('guard-other-run-cleanup', [exe, '--cleanup-earned-fixture'], 0, env_other)
assert not otherroot.exists() and fixture.exists()
run('guard-cross-configuration', [release, '--consume-earned-fixture'], 1)
assert fixture.exists()
run('guard-real-consumer', [exe, '--consume-earned-fixture'])
assert (fixture/'slot-1-auto.hrd').read_bytes() == original['slot-1-auto.hrd']
assert (fixture/'slot-1-manual.hrd').read_bytes() == original['slot-1-manual.hrd']
run('guard-consume-once', [exe, '--consume-earned-fixture'], 1)

# Force actual setup failure against this stale same-token root. Recovery
# must be skipped and cleanup must still run for the owned fixture.
project = out/'failed-setup'
project.mkdir(exist_ok=True)
(project/'CTestTestfile.cmake').write_text(f'''add_test(earn "{exe.as_posix()}" --produce-earned-fixture)
add_test(recover "{exe.as_posix()}" --consume-earned-fixture)
add_test(clean "{exe.as_posix()}" --cleanup-earned-fixture)
set_tests_properties(earn PROPERTIES FIXTURES_SETUP Earned TIMEOUT 30)
set_tests_properties(recover PROPERTIES FIXTURES_REQUIRED Earned TIMEOUT 30)
set_tests_properties(clean PROPERTIES FIXTURES_CLEANUP Earned TIMEOUT 30)
set_tests_properties(earn recover clean PROPERTIES ENVIRONMENT "HORDE_RT_EARNED_JOURNEY_RUN={token}")
''', encoding='utf-8')
r = run('guard-failed-setup', [ctest, '--test-dir', project, '-R', '^recover$', '-V'], 8)
assert 'Failed test dependencies: earn' in r.stdout and 'Not Run' in r.stdout
assert not fixture.exists()

# Unknown files must be preserved. Remove only our exact guard witness.
fixture.mkdir()
(fixture/'owner.txt').write_bytes(original['owner.txt'])
(fixture/'foreign.txt').write_text('owned guard witness')
run('guard-cleanup-refuses-extra', [exe, '--cleanup-earned-fixture'], 1)
assert (fixture/'foreign.txt').read_text() == 'owned guard witness' and (fixture/'owner.txt').exists()
(fixture/'foreign.txt').unlink()
run('guard-cleanup-success', [exe, '--cleanup-earned-fixture'])
assert not fixture.exists()
(out/'fixture-guards.json').write_text(json.dumps(dict(results=rows, allPassed=True, actualEarnedBytesUsed=True, fixtureRemoved=True), indent=2)+'\n', encoding='utf-8')
print(json.dumps(rows, indent=2))
