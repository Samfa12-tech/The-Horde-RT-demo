import array
import hashlib
import json
import re
from pathlib import Path
import unittest
import wave


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets/audio/pixabay/keeper-death.manifest.json"
SOURCE_HASH = "d16f67856849c0bb87d9dd6a9c09b15fd729436bdf59ec6c269108b0ebc58d35"


def read_wave(path):
    with wave.open(str(path), "rb") as source:
        channels, width, rate, frames, *_ = source.getparams()
        samples = array.array("h")
        samples.frombytes(source.readframes(frames))
        return channels, width, rate, frames, samples


def metrics(samples):
    return (max(map(abs, samples)) / 32768.0,
            (sum(float(value) * value for value in samples) / len(samples)) ** 0.5 / 32768.0)


class KeeperDeathAudioTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_source_provenance_keeps_owner_master_external(self):
        source = self.manifest["source"]
        self.assertEqual(source["sha256"], SOURCE_HASH)
        self.assertEqual(source["url"], "https://pixabay.com/sound-effects/film-special-effects-monster-demon-voice-death-defeat-scream-582531/")
        self.assertEqual(source["contributor"], "PhatPhrogStudio")
        self.assertEqual(source["published"], "2026-08-17")
        self.assertEqual(source["license"], "Pixabay Content License")
        self.assertFalse(source["includedInRepository"])
        self.assertNotIn("C:\\Users", json.dumps(source))

    def test_full_scream_derivatives_match_pins_and_measured_pcm(self):
        decoded = self.manifest["decodedSource"]
        frames = decoded["frames"]
        self.assertGreater(frames, 96000)
        self.assertLess(frames, 100000)
        self.assertAlmostEqual(decoded["durationSeconds"], frames / 48000, places=7)
        self.assertEqual(len(self.manifest["cues"]), 2)
        for cue in self.manifest["cues"]:
            path = ROOT / cue["path"]
            payload = path.read_bytes()
            self.assertEqual(len(payload), cue["bytes"])
            self.assertEqual(hashlib.sha256(payload).hexdigest(), cue["sha256"])
            channels, width, rate, count, samples = read_wave(path)
            self.assertEqual((channels, width, rate, count),
                             (cue["channels"], 2, 48000, frames))
            self.assertAlmostEqual(cue["durationSeconds"], count / rate, places=7)
            peak, rms = metrics(samples if channels == 1 else samples[::2])
            self.assertAlmostEqual(peak, cue["outputPeak"], delta=1 / 32768)
            self.assertAlmostEqual(rms, cue["outputRms"], delta=1 / 32768)
            self.assertLess(peak, 1.0)
            self.assertGreater(rms, 0.01)

    def test_android_file_is_dual_mono_of_the_windows_sample_stream(self):
        windows = ROOT / "assets/audio/pixabay/keeper_death.wav"
        android = ROOT / "assets/audio/pixabay/keeper_death_core.wav"
        w_channels, w_width, w_rate, w_frames, w_samples = read_wave(windows)
        a_channels, a_width, a_rate, a_frames, a_samples = read_wave(android)
        self.assertEqual((w_channels, w_width, w_rate), (1, 2, 48000))
        self.assertEqual((a_channels, a_width, a_rate), (2, 2, 48000))
        self.assertEqual(w_frames, a_frames)
        self.assertTrue(all(a_samples[index] == a_samples[index + 1]
                            for index in range(0, len(a_samples), 2)))
        self.assertEqual(array.array("h", a_samples[::2]), w_samples)

    def test_fades_preserve_content_and_zero_only_the_outer_edges(self):
        path = ROOT / "assets/audio/pixabay/keeper_death.wav"
        _, _, _, _, samples = read_wave(path)
        self.assertEqual(samples[0], 0)
        self.assertEqual(samples[-1], 0)
        self.assertGreater(max(map(abs, samples[1000:-2000])), 10000)
        self.assertEqual(self.manifest["processing"]["fadeInSeconds"], 0.01)
        self.assertEqual(self.manifest["processing"]["fadeOutSeconds"], 0.03)

    def test_existing_defeat_event_uses_one_positional_cue_on_each_platform(self):
        windows = (ROOT / "src/platform/windows/DiagnosticWindow.cpp").read_text(encoding="utf-8")
        android = (ROOT / "android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java").read_text(encoding="utf-8")
        w_case = re.search(r'case GameplayEventType::LichDefeated:(.*?)break;', windows, re.S).group(1)
        a_case = re.search(r'case PLATFORM_EVENT_LICH_DEFEATED:(.*?)break;', android, re.S).group(1)
        self.assertEqual(w_case.count("PlayPositionalSoundEffect("), 1)
        self.assertIn('"keeper_death.wav", 0.36f, event, "pixabay"', w_case)
        self.assertEqual(a_case.count("playSpatialSound("), 1)
        self.assertIn('"keeper_death", 0.28f, stereoGains, verticalMetadata', a_case)
        self.assertIn('loadSound("keeper_death", "audio/pixabay/keeper_death_core.wav")', android)
        gradle = (ROOT / "android/app/build.gradle").read_text(encoding="utf-8")
        package = (ROOT / "tools/package-alpha.ps1").read_text(encoding="utf-8")
        self.assertIn("include 'audio/pixabay/keeper_death_core.wav'", gradle)
        self.assertIn('"assets/audio/pixabay/keeper_death.wav"', package)
        self.assertNotIn(self.manifest["source"]["filename"], package)


if __name__ == "__main__":
    unittest.main()
