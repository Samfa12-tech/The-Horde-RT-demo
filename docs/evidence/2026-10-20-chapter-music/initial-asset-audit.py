"""Verify admitted music bytes/format without an audio output device."""
import hashlib
import io
import json
from pathlib import Path
import subprocess
import wave

ROOT = next(path for path in Path(__file__).resolve().parents
            if (path/"CMakeLists.txt").is_file() and (path/"ASSET_LICENSES.md").is_file())
BASE = ROOT/"assets/audio/music/what-the-dark-keeps"
manifest = json.loads((BASE/"asset.manifest.json").read_text(encoding="utf-8"))
rows = []
for cue in manifest["cues"]:
    for kind in ("body", "tail"):
        entry = cue[kind]
        path = BASE/entry["path"]
        raw = path.read_bytes()
        sha = hashlib.sha256(raw).hexdigest()
        assert sha == entry["sha256"]
        relative = path.relative_to(ROOT).as_posix()
        assert subprocess.check_output(["git", "show", "HEAD:"+relative], cwd=ROOT) == raw
        with wave.open(io.BytesIO(raw), "rb") as wav:
            assert wav.getnchannels() == 2 and wav.getsampwidth() == 2 and wav.getframerate() == 48000
            assert wav.getnframes() == entry["frames"]
        rows.append({"cue": cue["cue"], "part": kind, "path": relative, "bytes": len(raw),
                     "sha256": sha, "frames": entry["frames"], "matchesHeadAndManifest": True})
assert len(rows) == 16 and sum(row["bytes"] for row in rows) == 20160704
for entry in manifest["sources"]:
    path = BASE/entry["path"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == entry["sha256"]
    assert subprocess.check_output(["git", "show", "HEAD:"+path.relative_to(ROOT).as_posix()], cwd=ROOT) == path.read_bytes()
assert subprocess.run(["git", "diff", "--quiet", "HEAD", "--", "third_party/pocket-audio-core"], cwd=ROOT).returncode == 0
receipt = {"sourceBase": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
           "manifestSha256": hashlib.sha256((BASE/"asset.manifest.json").read_bytes()).hexdigest(),
           "runtimeWavBytes": sum(row["bytes"] for row in rows), "decodedPcmBytes": 20160000,
           "clips": rows, "editableSourcesUnchanged": True, "coreTrackedBytesUnchanged": True,
           "coreHeadTree": subprocess.check_output(["git", "rev-parse", "HEAD:third_party/pocket-audio-core"], cwd=ROOT, text=True).strip(),
           "musicAudioPlayback": False, "listeningAcceptance": False}
(ROOT/"reports/chapter-music-assets.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8")
print("PASS: all 16 admitted music WAVs and editable sources unchanged; exact PCM formats/timings; Core tracked bytes unchanged; no playback")
