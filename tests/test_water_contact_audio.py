import array
import hashlib
import json
from pathlib import Path
import unittest
import wave


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets/audio/pixabay/water-contact.manifest.json"


def read_pcm(path):
    with wave.open(str(path), "rb") as source:
        channels, width, rate, frames, *_ = source.getparams()
        samples = array.array("h")
        samples.frombytes(source.readframes(frames))
    return channels, width, rate, frames, samples


def rms(samples):
    return (sum(float(value) * value for value in samples) / len(samples)) ** 0.5 / 32768.0


class WaterContactAudioTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.cues = cls.manifest["cues"]

    def test_owner_source_is_provenanced_without_shipping_the_master(self):
        source = next(item for item in self.manifest["sources"] if item["filename"].endswith(".mp3"))
        self.assertEqual(source["sha256"], "70f548d544985d6fa7e14463684d3792c51d87da5a987b631d51f3e589f401a9")
        self.assertEqual(source["url"], "https://pixabay.com/sound-effects/footsteps-water-01-73731/")
        self.assertEqual(source["originalSourceUrl"], "https://freesound.org/people/aglinder/sounds/265582/")
        self.assertEqual(source["originalSourceLicense"], "CC0 1.0 (as stated on the Freesound source page)")
        self.assertFalse(source["includedInRepository"])

    def test_four_distinct_wet_steps_hit_relative_mix_target_without_clipping(self):
        wet_windows = [entry for entry in self.cues
                       if entry["path"].endswith(("water_wet_step_1.wav", "water_wet_step_2.wav",
                                                   "water_wet_step_3.wav", "water_wet_step_4.wav"))]
        self.assertEqual(len(wet_windows), 4)
        self.assertEqual(len({entry["sha256"] for entry in wet_windows}), 4)
        target = self.manifest["levelMatch"]["targetWetCueRmsBeforeEventGain"]
        self.assertLessEqual(self.manifest["levelMatch"]["peakLimit"], 0.45)
        for entry in wet_windows:
            path = ROOT / "assets" / entry["path"].removeprefix("assets/")
            payload = path.read_bytes()
            self.assertEqual(hashlib.sha256(payload).hexdigest(), entry["sha256"])
            channels, width, rate, frames, samples = read_pcm(path)
            self.assertEqual((channels, width, rate, frames), (1, 2, 48000, int(.38 * 48000)))
            self.assertAlmostEqual(rms(samples), target, delta=target * 0.01)
            self.assertLessEqual(max(map(abs, samples)) / 32768.0, 0.45)

    def test_android_variants_are_byte_sample_identical_dual_mono(self):
        for variant in range(1, 5):
            windows = next(entry for entry in self.cues
                           if entry["path"].endswith(f"water_wet_step_{variant}.wav"))
            android = next(entry for entry in self.cues
                           if entry["path"].endswith(f"water_wet_step_{variant}_core.wav"))
            self.assertEqual(windows["sourceStartSeconds"], android["sourceStartSeconds"])
            path = ROOT / "assets" / android["path"].removeprefix("assets/")
            channels, width, rate, frames, samples = read_pcm(path)
            self.assertEqual((channels, width, rate, frames), (2, 2, 48000, int(.38 * 48000)))
            self.assertTrue(all(samples[i] == samples[i + 1] for i in range(0, len(samples), 2)))
            self.assertAlmostEqual(rms(samples), android["outputRms"], delta=1e-6)

    def test_post_mix_wet_steps_are_about_half_the_reduced_dry_mix(self):
        level = self.manifest["levelMatch"]
        actual_ratio = (level["targetWetCueRmsBeforeEventGain"] * level["wetMeanEventIntensity"] /
                        (sum(level["dryReferenceRms"]) / 2 * level["dryPlayerMixGain"]))
        self.assertAlmostEqual(actual_ratio, 0.5, delta=0.01)


if __name__ == "__main__":
    unittest.main()
