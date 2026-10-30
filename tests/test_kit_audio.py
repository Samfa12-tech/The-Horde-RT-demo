"""Closed admission of the approved Kit cuts; no private archive is required."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]


class KitAudioTests(unittest.TestCase):
    def test_admission_refuses_conflicts_before_any_write_and_rejects_unknown_cuts(self):
        spec = importlib.util.spec_from_file_location("prepare_kit", ROOT / "tools/prepare-kit-audio.py")
        recipe = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(recipe)
        scratch_parent = ROOT / "reports/kit-audio-test-scratch"
        scratch_parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch_parent) as scratch:
            self.assertTrue(Path(scratch).resolve().is_relative_to(ROOT.resolve()))
            target = Path(scratch) / "runtime"
            target.mkdir()
            (target / "second.wav").write_bytes(b"preserve unrelated bytes")
            with self.assertRaisesRegex(ValueError, "refusing to overwrite"):
                recipe.admit({"first.wav": b"new", "second.wav": b"different"}, target)
            self.assertFalse((target / "first.wav").exists())
            self.assertEqual((target / "second.wav").read_bytes(), b"preserve unrelated bytes")
            recipe.admit({"second.wav": b"preserve unrelated bytes"}, target)
            self.assertEqual((target / "second.wav").read_bytes(), b"preserve unrelated bytes")
            with self.assertRaisesRegex(ValueError, "outside the closed"):
                recipe.admit({"another.wav": b"new"}, target)
            source = Path(scratch) / "source"
            source.mkdir()
            (source / "manifest.json").write_text(json.dumps({"clips": [{"id": "unknown"}]}))
            with self.assertRaisesRegex(ValueError, "exactly the ordered thirteen"):
                recipe.verify_source(source)

    def test_closed_runtime_cuts_and_authorized_provenance(self):
        directory = ROOT / "assets/audio/kit/runtime"
        manifest = json.loads((directory / "asset.manifest.json").read_text())
        self.assertEqual(manifest["schema"], 1)
        self.assertTrue(manifest["rights"]["publicGameRepositoryDistributionConfirmed"])
        self.assertEqual(manifest["rights"]["evidenceClass"], "owner-explicit-confirmation")
        self.assertTrue(manifest["mastersRemainPrivate"])
        cuts = manifest["cuts"]
        self.assertEqual(len(cuts), 13)
        self.assertEqual(len({cut["stableId"] for cut in cuts}), 13)
        expected = {cut["runtimeFile"] for cut in cuts} | {"asset.manifest.json"}
        self.assertEqual({p.name for p in directory.iterdir()}, expected)
        for cut in cuts:
            path = directory / cut["runtimeFile"]
            payload = path.read_bytes()
            self.assertEqual(hashlib.sha256(payload).hexdigest(), cut["sha256"])
            self.assertEqual(cut["sourceCutSha256"], cut["sha256"])
            self.assertEqual(len(payload), cut["bytes"])
            with wave.open(str(path), "rb") as clip:
                self.assertEqual((clip.getnchannels(), clip.getsampwidth(), clip.getframerate()), (1, 2, 24000))
                self.assertEqual(clip.getnframes(), cut["frames"])
                self.assertAlmostEqual(clip.getnframes() / clip.getframerate(), cut["durationSeconds"], places=8)
            # Exactly format/data: provider/account/source metadata cannot ride along.
            self.assertEqual(len(payload), 44 + cut["frames"] * 2)
            self.assertGreater(cut["peak"], .01)
            self.assertGreater(cut["rms"], .001)
            self.assertLessEqual(cut["durationSeconds"], 6.5)

    def test_native_catalog_paths_exact_words_and_measured_timing(self):
        manifest = json.loads((ROOT / "assets/audio/kit/runtime/asset.manifest.json").read_text())
        catalog = (ROOT / "src/gameplay/dialogue/ChapterDialogue.h").read_text()
        rows = re.findall(r'\{Line::\w+,"([^"]+)","Kit","([^"]+)","([^"]*)",([0-9.]+)f,\d+\}', catalog)
        self.assertEqual(len(rows), 13)
        by_id = {row[0]: row for row in rows}
        for cut in manifest["cuts"]:
            _, text, audio, duration = by_id[cut["stableId"]]
            self.assertEqual(text, cut["text"])
            self.assertEqual(audio, "assets/audio/kit/runtime/" + cut["runtimeFile"])
            self.assertAlmostEqual(float(duration), cut["durationSeconds"], delta=1 / 24000)

    def test_platform_packaging_is_explicit_and_source_free(self):
        manifest = json.loads((ROOT / "assets/audio/kit/runtime/asset.manifest.json").read_text())
        gradle = (ROOT / "android/app/build.gradle").read_text()
        policy = (ROOT / "tools/horde-1.7-kit-asset-policy.ps1").read_text()
        for cut in manifest["cuts"]:
            relative = "audio/kit/runtime/" + cut["runtimeFile"]
            self.assertIn("include '" + relative + "'", gradle)
            self.assertIn("Path='" + relative + "'", policy)
            self.assertIn("Sha256='" + cut["sha256"] + "'", policy)
        self.assertIn("include 'audio/kit/runtime/asset.manifest.json'", gradle)
        self.assertNotIn("include 'audio/kit/**'", gradle)
        encoded = json.dumps(manifest)
        for private in ("C:\\Users", "gold_before", "voice_id", "master_mp3", "api_key"):
            self.assertNotIn(private, encoded)


if __name__ == "__main__":
    unittest.main()
