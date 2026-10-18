import hashlib
import json
from pathlib import Path
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]

class ForestBirdAudioTests(unittest.TestCase):
    def test_exact_candidate_is_bounded_pcm_and_provenanced(self):
        directory = ROOT / "assets/audio/filmcow"
        manifest = json.loads((directory / "forest-distant-bird.manifest.json").read_text())
        runtime = directory / manifest["runtimeFile"]
        self.assertEqual(hashlib.sha256(runtime.read_bytes()).hexdigest(), manifest["runtimeSha256"])
        self.assertEqual(manifest["sourceSha256"], "eeec1e8ebae3af6395640fb0086cb2ebf006724efbb4859e93fb9c5b4a9de8a4")
        self.assertIn("not CC0", manifest["license"])
        self.assertIn("pending listening", manifest["acceptance"])
        self.assertFalse((directory / "distand bird.wav").exists())
        with wave.open(str(runtime), "rb") as wav:
            self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()), (1, 2, 48000, 397831))
            self.assertLess(wav.getnframes() * wav.getnchannels() * wav.getsampwidth(), 1024 * 1024)

if __name__ == "__main__":
    unittest.main()
