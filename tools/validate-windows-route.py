"""Create-only isolated hidden native route probe. Never kills a timed-out game.

Run from the existing checkout: python tools/validate-windows-route.py r1
This is continuation from a declared seeded post-Keeper checkpoint, not victory.
"""
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[1]
recovery = len(sys.argv) == 3 and sys.argv[2] == "--recover-lower-return"
suffix = sys.argv[1] if len(sys.argv) == 2 or recovery else ""
if not suffix or not all(c.isalnum() or c == "-" for c in suffix):
    raise SystemExit("Supply a new simple attempt suffix.")
output = root / "reports" / ("horde-campaign-fixtures-route-" + suffix)
logs = root / "reports" / ("windows-route-" + suffix)
if output.exists() or logs.exists():
    raise SystemExit("Both attempt directories must be new; retained results are never reused.")
logs.mkdir()
output.mkdir()
build = root / "build/presets/windows-x64-debug/Debug"
exe = build / "HordeLanternRT.exe"
exporter = build / "horde_rt_campaign_play_session_tests.exe"
owner_root = root.parents[1]
paths = [exe, exporter, build / "HordeLanternRT.settings.ini", Path(__file__).resolve(),
         owner_root / "ASSET_LICENSES.md", owner_root / "PROJECT_MEMORY.md",
         root / "src/platform/windows/DiagnosticWindow.cpp",
         root / "src/platform/windows/windows_campaign_validation.inl",
         root / "src/platform/windows/windows_route_validation.inl",
         root / "src/gameplay/dialogue/CompanionRouteFixture.h",
         root / "src/gameplay/simulation/GameSimulationCampaign.cpp",
         root / "src/gameplay/validation/RecordedReturnCheckpoint.h",
         root / "tests/CampaignPlaySessionTests.cpp", root / "tests/ForestDressingTests.cpp"]
campaign = Path(os.environ["LOCALAPPDATA"]) / "HordeLanternRT/campaign-v1"
source = root / "reports/horde-campaign-fixtures-route-20261020-r3/slot-1-auto.hrd"
provenance = root / "docs/evidence/2026-10-20-native-route-recovery/r3-receipt.json"
if recovery:
    paths.extend([source, provenance])

def digest(p):
    return {"bytes": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()} if p.is_file() else None

def custody():
    return {"files": {str(p): digest(p) for p in paths},
            "ownerCampaignExists": campaign.exists(),
            "ownerCampaignFiles": {str(p.relative_to(campaign)): digest(p) for p in campaign.rglob("*") if p.is_file()} if campaign.exists() else {}}

before = custody()
(logs / "before.json").write_text(json.dumps(before, indent=2))
if recovery:
    expected = json.loads(provenance.read_text())["finalFiles"]["slot-1-auto.hrd"]
    if source.resolve() != source or digest(source) != expected:
        raise SystemExit("Original R3 lower-return source must match its committed receipt exactly.")
    # Byte copies only. No codec encode, health grant or completion flag edit.
    data = source.read_bytes()
    (output / "slot-1-auto.hrd").write_bytes(data)
    (output / "slot-1-manual.hrd").write_bytes(data)  # immutable source witness
    manifest = {"evidenceClass": "recovery of actual R3 lower-return autosave; original post-Keeper seed remains explicit",
                "record": "slot-1-auto.hrd", "sourceSha256": expected["sha256"]}
    (output / "manifest.json").write_bytes((json.dumps(manifest, separators=(",", ":")) + "\n").encode())
else:
    export = subprocess.run([str(exporter), "--export-route-fixture", str(output)], cwd=root,
                            capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
    (logs / "export.stdout.log").write_bytes(export.stdout)
    (logs / "export.stderr.log").write_bytes(export.stderr)
    if export.returncode:
        raise SystemExit(f"Fixture export failed: {export.returncode}")
initial = {p.name: digest(p) for p in output.iterdir() if p.is_file()}
command = [str(exe), "--route-return-recovery-hidden" if recovery else "--route-validate-hidden", "--campaign-test-root", str(output)]
foreground = ctypes.windll.user32.GetForegroundWindow
foreground.restype = ctypes.c_void_p
initial_foreground = foreground()
started = time.monotonic()
with (logs / "native.stdout.log").open("wb") as out, (logs / "native.stderr.log").open("wb") as err:
    child = subprocess.Popen(command, cwd=build, stdout=out, stderr=err, creationflags=subprocess.CREATE_NO_WINDOW)
    (logs / "process.json").write_text(json.dumps({"pid": child.pid, "command": command, "executable": digest(exe)}, indent=2))
    try:
        code = child.wait(timeout=420)
    except subprocess.TimeoutExpired:
        code = None  # No terminate/kill or automatic retry.
after = custody()
result = {"exitCode": code, "pid": child.pid, "elapsedSeconds": time.monotonic() - started,
          "processPending": code is None, "initialForeground": initial_foreground, "finalForeground": foreground(),
          "custodyUnchanged": before == after, "initialFixture": initial,
          "finalFiles": {p.name: digest(p) for p in output.iterdir() if p.is_file()}}
(logs / "after.json").write_text(json.dumps(after, indent=2))
(logs / "receipt.json").write_text(json.dumps(result, indent=2))
print(json.dumps({k: v for k, v in result.items() if k not in ("initialFixture", "finalFiles")}, indent=2))
raise SystemExit(0 if code == 0 and before == after else 1)
