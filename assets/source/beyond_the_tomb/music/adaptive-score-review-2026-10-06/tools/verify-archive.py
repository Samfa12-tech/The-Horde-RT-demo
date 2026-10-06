#!/usr/bin/env python3
"""Read-only archive integrity check. Requires Python 3 and ffmpeg, no engine."""
import base64
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    inventory = json.loads((ROOT / 'archive-sha256.json').read_text())
    expected = {row['path'] for row in inventory['files']}
    actual = {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file() and p.name != 'archive-sha256.json'}
    assert expected == actual, 'Archive file inventory differs'
    for row in inventory['files']:
        p = ROOT / row['path']
        assert p.stat().st_size == row['bytes'] and sha(p) == row['sha256'], row['path']
    pairs = []
    for folder in ('scores', 'event-scores'):
        for p in sorted((ROOT / folder).glob('*.json')):
            if not p.name.endswith('.raw.json'):
                pairs.append((p, ROOT / 'share-codes' / (p.stem + '.pcs1.txt')))
    assert len(pairs) == 17
    pairs += [(p, p.with_suffix('.pcs1.txt')) for p in sorted((ROOT / 'editor-compatible').glob('*.editor-compatible.json'))]
    assert len(pairs) == 28
    for score, code in pairs:
        text = code.read_text().strip()
        assert text.startswith('PCS1:')
        raw = text[5:]
        decoded = json.loads(base64.urlsafe_b64decode(raw + '=' * (-len(raw) % 4)))
        assert decoded == json.loads(score.read_text()), str(score)
    sections = []
    banks = sorted((ROOT / 'section-audio').glob('*/bank-audio-manifest.json'))
    assert len(banks) == 17
    for p in banks:
        bank = json.loads(p.read_text())
        folder = 'event-scores' if bank['bank'].startswith('events-') else 'scores'
        assert sha(ROOT / folder / bank['sourceScore']) == bank['sourceSha256']
        for section in bank['sections']:
            sections.append((p.parent, section))
        full = bank['fullReview']
        mp3 = ROOT / full['mp3']
        report = json.loads((ROOT / full['report']).read_text())
        assert sha(mp3) == report['mp3']['sha256'], str(mp3)
        subprocess.run(['ffmpeg', '-v', 'error', '-i', str(mp3), '-f', 'null', '-'], check=True, capture_output=True)
    assert len(sections) == 136
    def check_section(pair):
        parent, item = pair
        f = parent / item['flac']['filename']
        assert sha(f) == item['flac']['sha256'], str(f)
        pcm = subprocess.check_output(['ffmpeg', '-v', 'error', '-i', str(f), '-f', 's16le', '-acodec', 'pcm_s16le', '-'])
        assert hashlib.sha256(pcm).hexdigest() == item['flac']['pcmSha256'], str(f)
        assert len(pcm) == item['metrics']['frames'] * 2 * 2, str(f)
    with ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(check_section, sections))
    print(f'PASS: {len(expected)} file hashes, 28 JSON/PCS1 pairs, 136 FLAC decoded-PCM hashes and 17 MP3 hashes/decodes.')
    print('Integrity only: no browser/audio-parity, listening acceptance, mastering or runtime-integration claim.')
if __name__ == '__main__':
    main()
