#!/usr/bin/env python3
"""Verify rendered files, source hashes, sample clocks, common gain and FLAC evidence."""
import hashlib
import json
from pathlib import Path
import wave

ROOT = Path(__file__).resolve().parents[1]
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
errors = []
records = []
manifests = sorted((ROOT / 'section-audio').glob('*/bank-audio-manifest.json'))
for p in manifests:
    bank = json.loads(p.read_text())
    event = bank['bank'].startswith('events-')
    source = ROOT / ('event-scores' if event else 'scores') / bank['sourceScore']
    if sha(source) != bank['sourceSha256']:
        errors.append(f'{bank["bank"]}: canonical source hash changed')
    for item in bank['sections']:
        metrics_file = p.parent / item['report']
        report = json.loads(metrics_file.read_text())
        wav = p.parent / item['wav']
        flac = p.parent / item['flac']['filename']
        with wave.open(str(wav), 'rb') as sound:
            if (sound.getnchannels(), sound.getsampwidth(), sound.getframerate()) != (2, 2, 44100):
                errors.append(f'{wav.name}: invalid WAV format')
            if sound.getnframes() != item['metrics']['frames']:
                errors.append(f'{wav.name}: WAV/metrics sample count mismatch')
            pcm_sha = hashlib.sha256(sound.readframes(sound.getnframes())).hexdigest()
        if sha(wav) != report['audio']['sha256'] or sha(flac) != item['flac']['sha256']:
            errors.append(f'{wav.name}: file SHA256 mismatch')
        if pcm_sha != item['flac']['pcmSha256'] or not item['flac']['pcmRoundtripVerified']:
            errors.append(f'{wav.name}: FLAC roundtrip evidence mismatch')
        if abs(report['options']['gainDb'] - bank['commonGainDb']) > 1e-9:
            errors.append(f'{wav.name}: inconsistent section gain')
        if report['metrics']['clippedSamples'] or report['metrics']['nonFiniteSamples']:
            errors.append(f'{wav.name}: clipped or nonfinite audio')
        if report['metrics']['eventCount'] <= 0 or report['metrics']['peak'] <= 0:
            errors.append(f'{wav.name}: empty/silent unexpected render')
        if item['behavior'] in ('loop', 'bridge'):
            expected = round(report['musicalDurationSeconds'] * 44100)
            if report['metrics']['frames'] != expected or not report['options']['loop']:
                errors.append(f'{wav.name}: bad musical loop boundary')
        elif report['options']['loop'] or report['options']['renderedTailSeconds'] < 0.6:
            errors.append(f'{wav.name}: one-shot tail missing')
        records.append({'bank': bank['bank'], 'section': item['id'], 'behavior': item['behavior'],
            'frames': item['metrics']['frames'], 'durationSeconds': item['metrics']['durationSeconds'],
            'peakDbfs': item['metrics']['peakDbfs'], 'rmsDbfs': item['metrics']['rmsDbfs'],
            'loopSeamJump': item['metrics']['loopSeam']['peakAbsoluteJump'],
            'loopSeamWarning': item['metrics']['loopSeam']['warning'] if item['behavior'] != 'one_shot' else False,
            'flacBytes': flac.stat().st_size})
    full = bank['fullReview']
    for key in ('wav', 'mp3', 'report', 'events'):
        if not Path(full[key]).is_file():
            errors.append(f'{bank["bank"]}: missing full-review {key}')
    full_report = json.loads(Path(full['report']).read_text())
    if abs(full_report['options']['gainDb'] - bank['commonGainDb']) > 1e-9:
        errors.append(f'{bank["bank"]}: inconsistent full-review gain')
    if full_report['metrics']['clippedSamples'] or full_report['metrics']['nonFiniteSamples']:
        errors.append(f'{bank["bank"]}: clipped or nonfinite full review')
    if sha(Path(full['wav'])) != full_report['audio']['sha256'] or sha(Path(full['mp3'])) != full_report['mp3']['sha256']:
        errors.append(f'{bank["bank"]}: full-review file hash mismatch')

if len(manifests) != 17 or len(records) != 136:
    errors.append(f'Expected17banks/136sections; found{len(manifests)}banks/{len(records)}sections')
summary = {
    'passed': not errors, 'bankCount': len(manifests), 'familyBankCount': sum(not p.parent.name.startswith('events-') for p in manifests),
    'eventBankCount': sum(p.parent.name.startswith('events-') for p in manifests),
    'sectionCount': len(records), 'fullReviewCount': len(manifests),
    'sampleRate': 44100, 'channels': 2, 'wavBitDepth': 16,
    'allFlacPcmRoundtripsVerified': not any('FLAC' in x for x in errors),
    'loopSectionCount': sum(x['behavior'] in ('loop', 'bridge') for x in records),
    'oneShotCount': sum(x['behavior'] == 'one_shot' for x in records),
    'maximumLoopSeamJump': max((x['loopSeamJump'] for x in records if x['behavior'] != 'one_shot'), default=0),
    'loopSeamWarningCount': sum(x['loopSeamWarning'] for x in records),
    'sectionAudioSeconds': sum(x['durationSeconds'] for x in records),
    'flacTotalBytes': sum(x['flacBytes'] for x in records),
    'maxSectionPeakDbfs': max((x['peakDbfs'] for x in records), default=None),
    'errors': errors, 'sections': records,
    'scope': 'Objective render/file verification; not an artistic listening approval, production master, or v68 tone-parity claim.'
}
dest = ROOT / 'validation/audio-delivery-verification.json'
dest.write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps({k: v for k, v in summary.items() if k != 'sections'}, indent=2))
raise SystemExit(bool(errors))
