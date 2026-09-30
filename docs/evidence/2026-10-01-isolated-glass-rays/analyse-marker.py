"""Bounded native-byte marker analysis; not production pixel acceptance."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

COLORS = {
    b'\x00\x00\xff\xff': 'interfaceBudget',
    b'\xff\xff\x00\xff': 'certifiedBudgetReason2',
    b'\xff\x00\x00\xff': 'certifiedTerminalReason1',
    b'\xff\x00\xff\xff': 'certifiedUnresolvedReason16',
}


def load(path):
    data = path.read_bytes()
    width, height = struct.unpack('<II', data[:8])
    if len(data) != 8 + 4 * width * height:
        raise ValueError('Invalid native byte roster')
    return data, width, height


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('control', type=Path)
    parser.add_argument('marker', type=Path)
    args = parser.parse_args()
    control, width, height = load(args.control)
    marker, candidate_width, candidate_height = load(args.marker)
    if (width, height) != (candidate_width, candidate_height):
        raise ValueError('Capture extents disagree')
    coordinates = {name: [] for name in COLORS.values()}
    old_marker_counts = {name: 0 for name in COLORS.values()}
    unaffected_max = 0
    unaffected_over_one = 0
    unaffected_different = 0
    all_diff = 0
    for pixel in range(width * height):
        offset = 8 + pixel * 4
        previous = control[offset:offset + 4]
        current = marker[offset:offset + 4]
        if previous in COLORS:
            old_marker_counts[COLORS[previous]] += 1
        name = COLORS.get(current)
        if name is not None:
            coordinates[name].append([pixel % width, pixel // width])
        if previous != current:
            all_diff += 1
            if name is None:
                delta = max(abs(previous[i] - current[i]) for i in range(3))
                unaffected_max = max(unaffected_max, delta)
                unaffected_over_one += delta > 1
                unaffected_different += 1
    result = {
        'schema': 1,
        'scope': 'Temporary output-marker localization, not a physical fix or weakened production image gate',
        'controlSha256': hashlib.sha256(control).hexdigest(),
        'markerSha256': hashlib.sha256(marker).hexdigest(),
        'extent': {'width': width, 'height': height},
        'baselineMarkerColorCollisions': old_marker_counts,
        'markerCounts': {name: len(values) for name, values in coordinates.items()},
        'markerCoordinates': coordinates,
        'changedPixels': all_diff,
        'unmarkedDifferences': {
            'pixels': unaffected_different,
            'maximumChannelDifference': unaffected_max,
            'pixelsOverOne': unaffected_over_one,
        },
        'productionAcceptance': False,
    }
    print(json.dumps(result, indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
