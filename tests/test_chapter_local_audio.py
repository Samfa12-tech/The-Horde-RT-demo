import hashlib
import json
from pathlib import Path
import struct
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]

class ChapterLocalAudioTests(unittest.TestCase):
    def test_closed_provenanced_finite_roster_and_pcm_headroom(self):
        directory = ROOT/'assets/audio/chapter-local-ambience'
        manifest = json.loads((directory/'asset.manifest.json').read_text())
        self.assertFalse(manifest['listeningAcceptance'])
        self.assertIn('not CC0', manifest['license'])
        self.assertEqual(hashlib.sha256((ROOT/manifest['licenseEvidence']).read_bytes()).hexdigest(),
                         manifest['licenseSha256'])
        self.assertEqual({p.relative_to(directory).as_posix() for p in directory.rglob('*') if p.is_file()},
                         {'asset.manifest.json','runtime/tomb_drip.wav','runtime/rope_contact.wav'})
        self.assertEqual({a['id'] for a in manifest['assets']}, {'tomb_drip','rope_contact'})
        total = 0
        for asset in manifest['assets']:
            with self.subTest(asset=asset['id']):
                path=directory/asset['runtimeFile']; raw=path.read_bytes(); total+=len(raw)
                self.assertEqual(hashlib.sha256(raw).hexdigest(), asset['runtimeSha256'])
                self.assertEqual(len(raw), asset['runtimeBytes'])
                with wave.open(str(path),'rb') as wav:
                    self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()),(1,2,48000))
                    self.assertEqual(wav.getnframes(),asset['frames'])
                    self.assertLessEqual(wav.getnframes(),48000)
                    samples=struct.unpack('<'+'h'*wav.getnframes(),wav.readframes(wav.getnframes()))
                self.assertEqual(samples[0],0);self.assertEqual(samples[-1],0)
                self.assertLess(max(abs(x) for x in samples), 8000)
                self.assertEqual(asset['sourceCropFrames'][1]-asset['sourceCropFrames'][0],asset['frames'])
                self.assertEqual(asset['edgeFadeFrames'],{'in':480,'out':3840})
                self.assertEqual(len(asset['sourceSha256']),64)
        self.assertEqual(total,191288)

    def test_both_platform_package_rosters_admit_only_runtime_derivatives(self):
        gradle=(ROOT/'android/app/build.gradle').read_text()
        windows=(ROOT/'tools/horde-night-audio-policy.ps1').read_text()
        for relative in ('audio/chapter-local-ambience/asset.manifest.json',
                         'audio/chapter-local-ambience/runtime/tomb_drip.wav',
                         'audio/chapter-local-ambience/runtime/rope_contact.wav'):
            self.assertIn(relative,gradle);self.assertIn(relative,windows)
        for private in ('rope tighten 1.wav','ambience - dripping water 1.wav'):
            self.assertNotIn(private,gradle);self.assertNotIn(private,windows)
        self.assertIn('Copy-HordeNightAudioRuntimeAssets', (ROOT/'tools/package-alpha.ps1').read_text())

if __name__=='__main__': unittest.main()
