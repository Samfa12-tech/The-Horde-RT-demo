"""Read-only native-storage shadow-segment evidence; no image editing."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image

FIELDS = ('case ox oy oz dx dy dz distance r g b near far rawCandidateCount '
          'nearInstance farInstance nearEntering farEntering material transmission '
          'attenR attenG attenB attenuationDistance flags').split()

def decode(path):
    source = path.read_bytes()
    if path.suffix == '.png':
        with Image.open(path) as image:
            width, height = image.size
            data = image.convert('RGBA').tobytes()
    else:
        width, height = struct.unpack('<II', source[:8])
        data = source[8:]
    assert len(data) == width * height * 4 and width >= 50 and height >= 7
    rows = []
    for row in range(7):
        values = {}
        for i, name in enumerate(FIELDS):
            offset = 4 * (row * width + i * 2)
            values[name] = struct.unpack('<f', data[offset:offset+3] + data[offset+4:offset+5])[0]
        assert values['case'] == row
        relevant = ('millimetre' in path.as_posix()) == (row == 6)
        length = None
        crossed = None
        if relevant:
            if row in (0, 1, 5, 6):
                assert values['nearEntering'] == 1 and values['farEntering'] == 0
                assert values['far'] > values['near'] > 0
                length = values['far'] - values['near']
                crossed = 2
            elif row == 2:
                assert values['nearEntering'] == 0 and values['farEntering'] == 0
                length = values['near']
                crossed = 1
            elif row == 3:
                assert values['nearEntering'] == 1 and values['farEntering'] == 1
                length = values['distance'] - values['near']
                crossed = 1
            elif row == 4:
                assert values['rawCandidateCount'] == 0
        expected = None
        error = None
        if length is not None:
            expected = [values['transmission'] ** crossed *
                        values['atten'+channel] ** (length / values['attenuationDistance'])
                        for channel in ('R','G','B')]
            error = max(abs(values[channel] - value)
                        for channel, value in zip(('r','g','b'), expected))
        rows.append(dict(record=values, relevant=relevant, geometricLength=length,
                         crossedInterfaces=crossed, analyticalRgb=expected,
                         maximumAbsoluteError=error,
                         formulaPassed=error <= 2e-5 if error is not None else None,
                         admissionGap='Both endpoints inside unknown initial medium; boundary-free segment cannot infer absorption.'
                         if row == 4 and relevant else None))
    return dict(path=str(path), sha256=hashlib.sha256(source).hexdigest(),
                extent=[width,height], cases=rows)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('files', nargs='+', type=Path)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
result = dict(schema=1, investigationOnly=True,
              tolerance=2e-5, captures=[decode(path) for path in args.files])
text = json.dumps(result, indent=2, allow_nan=False) + '\n'
if args.output:
    args.output.write_text(text, encoding='utf-8', newline='\n')
else:
    print(text, end='')
