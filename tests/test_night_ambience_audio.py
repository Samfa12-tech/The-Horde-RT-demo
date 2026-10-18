import hashlib
import json
import math
from pathlib import Path
import struct
import unittest
import wave

ROOT=Path(__file__).resolve().parents[1]

class NightAmbienceAudioTests(unittest.TestCase):
    def test_all_prepared_assets_have_pinned_provenance_bounded_pcm_and_headroom(self):
        directory=ROOT/'assets/audio/night-ambience'
        manifest=json.loads((directory/'manifest.json').read_text())
        self.assertEqual(len(manifest['assets']),4)
        for asset in manifest['assets']:
            with self.subTest(asset=asset['key']):
                path=directory/asset['file']
                self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),asset['runtimeSha256'])
                self.assertEqual(len(asset['sourceSha256']),64)
                self.assertTrue(asset['licenseUrl'].startswith('https://creativecommons.org/licenses/'))
                self.assertTrue(asset['creator'] and asset['page'] and asset['changes'])
                self.assertIn('Unheard',asset['acceptance'])
                self.assertEqual(asset['admitted'],asset['file'].startswith('runtime/'))
                with wave.open(str(path),'rb') as wav:
                    self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()),(1,2,24000))
                    self.assertEqual(wav.getnframes(),asset['frames'])
                    samples=struct.unpack('<'+'h'*wav.getnframes(),wav.readframes(wav.getnframes()))
                self.assertLess(len(samples)*2,1024*1024)
                self.assertLess(max(abs(x) for x in samples),16000)
                self.assertEqual(samples[0],0)
                self.assertLessEqual(abs(samples[-1]),2)
                peak=20*math.log10(max(abs(x) for x in samples)/32768)
                self.assertAlmostEqual(peak,asset['peakDbfs'],places=6)
        cricket=next(a for a in manifest['assets'] if a['key']=='evening_cricket')
        self.assertEqual(cricket['license'],'CC-BY-SA-4.0')
        for key in ['pond_frogs_crickets','periodical_cicada']:
            self.assertFalse(next(a for a in manifest['assets'] if a['key']==key)['admitted'])

if __name__=='__main__': unittest.main()
