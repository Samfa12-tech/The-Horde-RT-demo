"""Prove chapter layer reuse against actual packaged Windows and ASTC bytes."""
import json
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
DIRECTORY = ROOT / 'assets/textures/props/runtime'
ALIASES = {6: 4, 7: 5, 9: 8}


class ChapterTextureAliasTests(unittest.TestCase):
    def test_authored_map_identities(self):
        manifest = json.loads((DIRECTORY / 'asset.manifest.json').read_text())
        sources = {row['layer']: row for row in manifest['sourceLayers']}
        for consumer, provider in ALIASES.items():
            for category in ('baseColor', 'normal', 'orm'):
                with self.subTest(consumer=consumer, provider=provider, category=category):
                    self.assertEqual(sources[consumer][category]['sha256'],
                                     sources[provider][category]['sha256'])
                    self.assertEqual(sources[consumer][category]['bytes'],
                                     sources[provider][category]['bytes'])

    def test_every_packaged_mip_and_material_route_is_byte_identical(self):
        for platform in ('windows', 'android'):
            old_bytes = new_bytes = 0
            for category, expected_layers in (('base-color', 18), ('normal', 15), ('orm', 15)):
                data = (DIRECTORY / f'{category}.{platform}.ktx2').read_bytes()
                self.assertEqual(data[:12], b'\xabKTX 20\xbb\r\n\x1a\n')
                layers, faces, levels, compression = struct.unpack_from('<4I', data, 32)
                self.assertEqual((layers, faces, levels, compression), (expected_layers, 1, 11, 0))
                selected = [i for i in range(layers) if i not in ALIASES]
                for level in range(levels):
                    offset, size, decoded_size = struct.unpack_from('<3Q', data, 80 + level*24)
                    self.assertEqual(size, decoded_size)
                    self.assertEqual(size % layers, 0)
                    self.assertLessEqual(offset + size, len(data))
                    layer_bytes = size // layers
                    original = [data[offset+i*layer_bytes:offset+(i+1)*layer_bytes]
                                for i in range(layers)]
                    remapped = [original[i] for i in selected]
                    # Every old material, including consumers, must sample the
                    # same actual compressed or RGBA mip after remapping.
                    for old_layer in range(layers):
                        new_layer = selected.index(ALIASES.get(old_layer, old_layer))
                        with self.subTest(platform=platform, category=category,
                                          mip=level, layer=old_layer):
                            self.assertEqual(original[old_layer], remapped[new_layer])
                    old_bytes += size
                    new_bytes += layer_bytes * len(selected)
            print(f'{platform}: exact encoded PBR mip bytes {old_bytes} -> {new_bytes}; '
                  f'saved {old_bytes-new_bytes}; all material samples preserved')


if __name__ == '__main__':
    unittest.main()
