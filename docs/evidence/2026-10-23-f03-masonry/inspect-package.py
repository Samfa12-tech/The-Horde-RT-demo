"""Inspect the built APK and one three-file ZIP fixture; never install/publish."""
import hashlib
from datetime import datetime, timezone
import io
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[3]
apk = ROOT / "android/app/build/outputs/apk/debug/app-debug.apk"
base = "assets/models/world/runtime/forest-road-clue-v1/"
names = [base + name for name in ("asset.manifest.json", "horde-road-kerb.glb", "horde-road-waystone.glb")]
rows = []
with zipfile.ZipFile(apk) as archive:
    assert len(archive.namelist()) == len(set(archive.namelist()))
    forbidden = [n for n in archive.namelist() if n.startswith("assets/source/") or
                 (n.startswith("assets/") and n.lower().endswith((".flac", ".zip", ".blend"))) or
                 "outdoor-quiet" in n or "dialogue-bed" in n]
    assert not forbidden, forbidden
    assert sorted(n for n in archive.namelist() if n.startswith(base)) == sorted(names)
    for name in names:
        entry = archive.getinfo(name)
        data = archive.read(name)
        assert data == (ROOT / name).read_bytes()
        rows.append({"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                     "apkCompressedBytes": entry.compress_size, "apkCompressionMethod": entry.compress_type})
    abis = sorted(n.split("/")[1] for n in archive.namelist() if n.startswith("lib/") and n.endswith("/libhorde_rt_probe_android.so"))
    assert abis == ["arm64-v8a", "armeabi-v7a", "x86", "x86_64"], abis
fixture = io.BytesIO()
with zipfile.ZipFile(fixture, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for name in names:
        archive.writestr(name, (ROOT / name).read_bytes())
tests = dict(tests=0, failures=0, errors=0, skipped=0)
for path in (ROOT / "android/app/build/test-results/testDebugUnitTest").glob("TEST-*.xml"):
    suite = ET.parse(path).getroot()
    for key in tests:
        tests[key] += int(suite.get(key, "0"))
assert tests["tests"] and not tests["failures"] and not tests["errors"]
metadata = json.loads((apk.parent / "output-metadata.json").read_text())
with apk.open("rb") as stream:
    apk_hash = hashlib.file_digest(stream, "sha256").hexdigest()
print(json.dumps({"artifact": str(apk.relative_to(ROOT)), "bytes": apk.stat().st_size,
                  "sha256": apk_hash, "builtUtc": datetime.fromtimestamp(apk.stat().st_mtime, timezone.utc).isoformat(),
                  "version": metadata["elements"][0], "abis": abis, "javaTests": tests,
                  "runtimeFiles": rows, "threeFileWindowsStyleZipFixtureBytes": len(fixture.getvalue()),
                  "scope": "Actual local Debug APK inventory, no install. Three-file ZIP fixture is not a complete Windows distribution. No original/quiet masters admitted."}, indent=2))
