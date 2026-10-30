"""Bounded native Debug fixture; never requests foreground or accesses a device.

Default: inject Esc through the real HWND handler at the isolated detach seam.
--observe-original-poll retains the former external-poll gate and logs where
the cross-thread message is actually delivered. A missed witness remains red.
"""
from pathlib import Path
import argparse
import ctypes as c
import ctypes.wintypes as w
import hashlib
import json
import os
import shutil
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('label')
parser.add_argument('--observe-original-poll', action='store_true')
parser.add_argument('--action', choices=['detached-cancel','detached-close','close-after-apply'],
                    default='detached-cancel')
args = parser.parse_args()
if args.observe_original_poll and args.action != 'detached-cancel':
    parser.error('The original polling diagnosis applies only to detached-cancel')
assert os.name == 'nt', 'Windows native fixture only'
assert args.label.replace('-', '').isalnum(), 'Use a new alphanumeric/hyphen label'
root = Path(__file__).resolve().parents[1]
output = root / 'reports/benchmark-menu-entry-20261023' / args.label
output.mkdir(exist_ok=False)
fixture = root / 'reports' / ('horde-campaign-fixtures-benchmark-menu-' + args.label)
fixture.mkdir(exist_ok=False)
(fixture / 'manifest.json').write_bytes(
    b'{"evidenceClass":"isolated opening-menu benchmark; no campaign progress"}\n')
source = root / 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
exe = output / source.name
shutil.copyfile(source, exe)
settings = output / 'HordeLanternRT.settings.ini'
settings.write_text('''[graphics]
schema=5
pending=0
confirmedScale=75
confirmedWater=1
confirmedFire=2
confirmedCap=45
confirmedGlass=0
confirmedShadow=2
confirmedMist=0
confirmedDust=2
[audio]
sfxVolume=0
sfx=0
musicVolume=0
voiceVolume=0
[display]
renderScale=75
waterQuality=1
''')
link = subprocess.run(['cmd', '/c', 'mklink', '/J', str(output / 'assets'),
                       str(root / 'assets')], capture_output=True)
assert link.returncode == 0, link.stderr
user = c.WinDLL('user32', use_last_error=True)
user.SetProcessDPIAware()
user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
user.GetDlgItem.argtypes = [w.HWND, c.c_int]
user.GetDlgItem.restype = w.HWND
user.IsWindowVisible.argtypes = [w.HWND]
user.IsWindowEnabled.argtypes = [w.HWND]
user.GetWindowLongPtrW.argtypes = [w.HWND, c.c_int]
user.GetWindowLongPtrW.restype = c.c_ssize_t
user.GetClientRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
user.GetWindowRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
user.GetWindowTextW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
user.SendMessageTimeoutW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM,
                                    w.UINT, w.UINT, c.POINTER(c.c_size_t)]
user.SendMessageTimeoutW.restype = c.c_ssize_t
owner = Path(os.environ['LOCALAPPDATA']) / 'HordeLanternRT/campaign-v1'
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def custody():
    return {'settings': sha(settings),
            'sourceSettings': sha(source.parent / settings.name),
            'ownerExists': owner.exists(),
            'ownerFiles': {str(p.relative_to(owner)): sha(p)
                           for p in owner.rglob('*') if p.is_file()} if owner.exists() else {}}
before = custody()
(output / 'custody-before.json').write_text(json.dumps(before, indent=2) + '\n')
events = []
hwnd = None
error = None
original = None
def log(): return (output / 'stderr.log').read_text(errors='replace')
def wait(test, timeout=65):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        value = test()
        if value: return value
        if process.poll() is not None:
            raise RuntimeError('Owned application exited ' + str(process.returncode))
        time.sleep(.05)
    raise RuntimeError('Bounded native interaction timed out')
def owned_window():
    found = []
    @c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    def visit(window, _):
        pid = w.DWORD()
        user.GetWindowThreadProcessId(window, c.byref(pid))
        if pid.value == process.pid and user.GetDlgItem(window, 119): found.append(window)
        return True
    user.EnumWindows(visit, 0)
    return found[0] if found else None
def send(window, message, wp=0):
    result = c.c_size_t()
    if not user.SendMessageTimeoutW(window, message, wp, 0, 2, 10000, c.byref(result)):
        raise RuntimeError('Native message timeout, error ' + str(c.get_last_error()))
def click(control):
    button = user.GetDlgItem(hwnd, control)
    assert button and user.IsWindowVisible(button) and user.IsWindowEnabled(button), control
    send(button, 0xF5)
    events.append({'clicked': control})
def window_state():
    outer, client = w.RECT(), w.RECT()
    user.GetWindowRect(hwnd, c.byref(outer))
    user.GetClientRect(hwnd, c.byref(client))
    return {'outer': [outer.left, outer.top, outer.right, outer.bottom],
            'client': [client.left, client.top, client.right, client.bottom],
            'style': user.GetWindowLongPtrW(hwnd, -16),
            'exStyle': user.GetWindowLongPtrW(hwnd, -20)}
command = [str(exe), '--benchmark-menu-validate', '--campaign-test-root', str(fixture)]
if not args.observe_original_poll and args.action != 'close-after-apply':
    command.append('--benchmark-menu-' + args.action)
start = time.monotonic()
with (output / 'stdout.log').open('xb') as stdout, (output / 'stderr.log').open('xb') as stderr:
    process = subprocess.Popen(command, cwd=root, stdout=stdout, stderr=stderr,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        hwnd = wait(owned_window)
        wait(lambda: 'HORDE_OPENING_MENU_STAGE ui-ready' in log())
        assert 'HORDE_OPENING_MENU_STAGE isolated-window no-activate' in log()
        click(208)
        wait(lambda: user.IsWindowVisible(user.GetDlgItem(hwnd, 119)) and
                     user.IsWindowEnabled(user.GetDlgItem(hwnd, 119)))
        original = window_state()
        click(119)
        click(230 if args.action == 'close-after-apply' else 231)
        click(232)
        if args.observe_original_poll:
            wait(lambda: user.GetWindowLongPtrW(hwnd, -21) == 0, 5)
            events.append({'observedContextNull': True})
            send(hwnd, 0x100, 0x1B)
        if args.action == 'close-after-apply':
            wait(lambda: 'HORDE_OPENING_BENCHMARK applied fixed=' in log())
            send(hwnd, 0x10)
        wait(lambda: 'HORDE_OPENING_BENCHMARK entry-presented' in log())
        closing = args.action != 'detached-cancel'
        if closing:
            wait(lambda: process.poll() is not None)
            assert 'HORDE_OPENING_BENCHMARK final-entry-retired=1 window-alive=0' in log()
        else:
            assert window_state() == original, 'Window not restored'
            wait(lambda: user.IsWindowVisible(user.GetDlgItem(hwnd, 119)) and
                         user.IsWindowEnabled(user.GetDlgItem(hwnd, 119)))
        lifecycle = json.loads((output / 'reports/windows-benchmark-profile-lifecycle.json').read_text())
        if args.action == 'close-after-apply':
            assert lifecycle['actualApplyAcknowledged'] and lifecycle['actualRestorationAcknowledged']
            assert lifecycle['window']['exactWindowStateRestored']
            assert lifecycle['requestedProfile'] == 'windows-rt-provisional-v1'
            applied = lifecycle['applied']['completedFrame']['submittedDimensions']
            assert applied['internalExtent'] == [1920, 1080] and applied['outputExtent'] == [1920, 1080]
            assert not applied['dynamicResolutionEnabled']
            assert lifecycle['restored']['completedFrame']['submittedDimensions'] == lifecycle['original']['completedFrame']['submittedDimensions']
        else:
            witness = 'detached-close' if closing else 'detached-cancel'
            assert 'HORDE_OPENING_MENU_STAGE ' + witness in log(), 'Did not witness detached input path'
            if not args.observe_original_poll:
                assert 'detached-inject context-null=1 phase=queued pending-before=0' in log()
                delivery = 'close-delivery' if closing else 'escape-delivery'
                assert delivery + ' context-attached=0 phase=1' in log()
                assert 'detached-inject-result pending-after=1' in log()
            assert not lifecycle['actualApplyAcknowledged'], 'Cancelled handoff applied the profile'
        assert lifecycle['closeRequested'] == closing, 'Close intent missing from successor lifecycle'
        assert lifecycle['cancelRequested'] and not lifecycle['campaignAttached']
        assert not lifecycle['ranked'] and not lifecycle['measurementAndRestorationPassed']
        assert not lifecycle['settingsWritesPermitted'] and lifecycle['confirmedTupleUnchanged']
        if closing:
            events.append({'freshEntryPresentedThenRetired': True})
        else:
            caption = c.create_unicode_buffer(180)
            user.GetWindowTextW(user.GetDlgItem(hwnd, 103), caption, 180)
            assert caption.value == 'BENCHMARK CANCELLED', caption.value
            events.append({'restored': True, 'resultCaption': caption.value})
    except Exception as exc:
        error = str(exc)
    finally:
        if process.poll() is None and hwnd:
            try: send(hwnd, 0x10)
            except Exception as exc: events.append({'closeError': str(exc)})
        try: process.wait(timeout=60)
        except subprocess.TimeoutExpired: events.append({'pendingOwnedPid': process.pid})
result = {'pid': process.pid, 'exitCode': process.poll(), 'seconds': time.monotonic() - start,
          'error': error, 'action': args.action, 'originalPolling': args.observe_original_poll, 'events': events,
          'custodyUnchanged': before == custody(), 'exeSha256': sha(exe),
          'isolatedFixtureFiles': [str(p.relative_to(fixture)) for p in fixture.rglob('*') if p.is_file()],
          'harnessForegroundRequestAttempted': False, 'nativeForegroundRequestsSuppressed': True,
          'screenshotsAttempted': False,
          'evidenceClass': 'isolated muted Debug native HWND cancellation and actual RT return; no physical input acceptance'}
(output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result), flush=True)
assert process.poll() is not None, 'Owned app pending: inspect; do not start another'
assert process.returncode == 0, 'Owned app failed during final shutdown: ' + str(process.returncode)
assert result['custodyUnchanged'], 'Custody changed'
assert not error, error
