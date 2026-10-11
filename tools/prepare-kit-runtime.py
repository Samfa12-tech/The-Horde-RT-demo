"""Preserve the approved Warden skin/animation bytes; externalize its PBR maps.

No Blender round-trip, mesh simplification, retargeting, colour conversion or
provider call. Runtime GLB keeps four tiny routing images like the existing PBR
pipeline. Actual 1K pixels are separately compiled with the established KTX tool.
"""
import argparse
import hashlib
import io
import json
import struct
from pathlib import Path
from PIL import Image

SOURCE_SHA = '47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def write_preserving(path, data):
    if path.exists() and path.read_bytes() != data:
        raise RuntimeError(f'Refusing to overwrite different existing bytes: {path}')
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        path.write_bytes(data)

def png(image):
    output = io.BytesIO()
    image.save(output, format='PNG')
    return output.getvalue()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = args.source.read_bytes()
    if sha(source) != SOURCE_SHA:
        raise RuntimeError('Kit source differs from the owner-approved exact master')
    magic, version, length = struct.unpack_from('<III', source)
    json_length, kind = struct.unpack_from('<II', source, 12)
    if (magic, version, length, kind) != (0x46546c67, 2, len(source), 0x4e4f534a):
        raise RuntimeError('Unexpected GLB header')
    document = json.loads(source[20:20+json_length])
    binary_length, binary_kind = struct.unpack_from('<II', source, 20+json_length)
    binary = source[28+json_length:28+json_length+binary_length]
    if binary_kind != 0x004e4942 or len(document['images']) != 4:
        raise RuntimeError('Unexpected Kit image/binary roster')
    maps = {'emission': 'emissive', 'normal': 'normal',
            'base_color': 'base-color', 'metallic-roughness': 'orm'}
    replacements = {}
    receipts = []
    for image in document['images']:
        name = maps[image['name']]
        index = image['bufferView']
        view = document['bufferViews'][index]
        start = view.get('byteOffset', 0)
        original = binary[start:start+view['byteLength']]
        decoded = Image.open(io.BytesIO(original)).convert('RGBA')
        if decoded.size != (1024, 1024):
            raise RuntimeError('Kit map resolution changed')
        # glTF has metallic/roughness but no occlusion map. Encode AO=1, preserve
        # the authored G/B channels; other maps retain their decoded RGBA bytes.
        if name == 'orm':
            _, green, blue, _ = decoded.split()
            decoded = Image.merge('RGBA', (Image.new('L', decoded.size, 255), green, blue,
                                          Image.new('L', decoded.size, 255)))
        encoded = png(decoded)
        write_preserving(args.output/'texture-inputs'/f'{name}.png', encoded)
        thumbnail = png(decoded.resize((4, 4), Image.Resampling.LANCZOS))
        replacements[index] = thumbnail
        receipts.append({'name': name, 'sourceImageSha256': sha(original),
                         'textureInputSha256': sha(encoded),
                         'decodedRgbaSha256': sha(decoded.tobytes())})
    output_binary = bytearray()
    preserved = []
    for index, view in enumerate(document['bufferViews']):
        if view.get('buffer', 0) != 0:
            raise RuntimeError('Unexpected external buffer')
        old_start = view.get('byteOffset', 0)
        old = binary[old_start:old_start+view['byteLength']]
        chunk = replacements.get(index, old)
        output_binary += bytes((-len(output_binary)) % 4)
        view['byteOffset'], view['byteLength'] = len(output_binary), len(chunk)
        output_binary += chunk
        if index not in replacements:
            preserved.append({'bufferView': index, 'bytes': len(old), 'sha256': sha(old)})
    document['buffers'][0]['byteLength'] = len(output_binary)
    output_binary += bytes((-len(output_binary)) % 4)
    output_json = json.dumps(document, separators=(',', ':'), ensure_ascii=False).encode()
    output_json += b' ' * ((-len(output_json)) % 4)
    runtime = (struct.pack('<III', magic, version, 28+len(output_json)+len(output_binary)) +
               struct.pack('<II', len(output_json), kind) + output_json +
               struct.pack('<II', len(output_binary), binary_kind) + output_binary)
    write_preserving(args.output/'kit-warden-lod0.runtime.glb', runtime)
    receipt = {'schema': 1, 'sourceSha256': SOURCE_SHA, 'runtimeSha256': sha(runtime),
               'sourceBytes': len(source), 'runtimeBytes': len(runtime),
               'preservedNonImageViews': preserved, 'textures': receipts,
               'animations': [a['name'] for a in document['animations']],
               'derivation': 'Only image buffer views replaced; all skin, geometry, animation and material data preserved. ORM R set to 255 because source has no AO.',
               'runtimeAdmission': 'Requires native import, PBR skinning, texture compilation and scene validation.'}
    write_preserving(args.output/'processing-receipt.json', (json.dumps(receipt, indent=2)+'\n').encode())
    print(json.dumps({'runtimeBytes': len(runtime), 'runtimeSha256': sha(runtime),
                      'preservedViews': len(preserved)}))

if __name__ == '__main__':
    main()
