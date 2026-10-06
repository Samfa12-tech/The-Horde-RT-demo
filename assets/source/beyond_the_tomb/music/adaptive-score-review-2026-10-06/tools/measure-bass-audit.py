#!/usr/bin/env python3
"""Signal measurements on genuine Core/app renders; does not synthesize or edit audio."""
import json
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import find_peaks

ROOT = Path(__file__).resolve().parents[1] / 'validation/bass-audit'
inputs = json.loads((ROOT / 'comparison-inputs.json').read_text())

def read(file):
    rate, x = wavfile.read(ROOT / file)
    scale = float(2 ** (x.dtype.itemsize * 8 - 1)) if np.issubdtype(x.dtype, np.integer) else 1
    x = x.astype(np.float64) / scale
    if x.ndim == 2:
        x = x.mean(axis=1)
    return rate, x

def db(x):
    return float(20 * np.log10(max(1e-30, x)))

def spectral(x, rate, fundamental):
    # Full first note, Hann window. The exact f0/sub are separately measured at their known frequencies.
    n = len(x)
    window = np.hanning(n)
    y = x * window
    nfft = 2 ** int(np.ceil(np.log2(max(16, n * 8))))
    spectrum = np.fft.rfft(y, nfft)
    f = np.fft.rfftfreq(nfft, 1 / rate)
    power = abs(spectrum) ** 2
    total = power.sum()
    weighted_center = float((f * power).sum() / max(total, 1e-30))
    peaks, _ = find_peaks(abs(spectrum))
    top = sorted(peaks, key=lambda i: power[i], reverse=True)[:12]
    dominant = [{'hz': float(f[i]), 'levelRelativeStrongestDb': db(abs(spectrum[i]) / max(abs(spectrum).max(), 1e-30))} for i in top]
    t = np.arange(n) / rate
    amplitudes = {}
    for label, hz in [('sub', fundamental/2), ('fundamental', fundamental)] + [(f'harmonic_{i}', fundamental*i) for i in range(2, 11)]:
        amplitudes[label] = float(abs(np.sum(y * np.exp(-2j*np.pi*hz*t))) * 2 / max(window.sum(), 1e-30))
    a0 = amplitudes['fundamental']
    bands = {}
    for hz in [420, 1000, 3000, 8000, 16000]:
        fraction = float(power[f >= hz].sum() / max(total, 1e-30))
        bands[f'energyAbove{hz}HzFraction'] = fraction
        bands[f'energyAbove{hz}HzDbRelativeTotal'] = float(10*np.log10(max(fraction, 1e-30)))
    return {'fftMethod': 'Whole isolated first note, Hann window, 8x zero-padding; finite-note spectral leakage remains.',
        'dominantPeaks': dominant, 'spectralCentroidHz': weighted_center, **bands,
        'knownFrequencyAmplitude': amplitudes,
        'harmonicsRelativeFundamentalDb': {k: db(v/max(a0, 1e-30)) for k,v in amplitudes.items()}}

results = []
for c in inputs['comparisons']:
    event = c['events'][0]
    rate, x = read(c['firstNoteFiles']['44100']['filename'])
    _, bandlimited_diagnostic = read(c['firstNoteOversampledDownsampledFile'])
    frames = int(np.floor(max(0.02, event['duration']) * rate))
    note = x[:frames]
    count = min(len(x), len(bandlimited_diagnostic))
    residual = x[:count] - bandlimited_diagnostic[:count]
    noteRms = np.sqrt(np.mean(x[:count] ** 2))
    # Interior excludes attack/termination, minimizing sample-grid envelope-end differences.
    lo = int(max(0.02, event['duration'] * .12) * rate)
    hi = int(event['duration'] * .85 * rate)
    interior = x[lo:hi] - bandlimited_diagnostic[lo:hi]
    internalRms = np.sqrt(np.mean(x[lo:hi] ** 2))
    attack = int(np.ceil(frames * .08))
    fundamental = float(440 * 2 ** ((event['midi'] - 69) / 12))
    result = {
        'name': c['name'], 'midi': event['midi'], 'fundamentalHz': fundamental,
        'subOscillatorHz': fundamental/2, 'authoredFirstEventSeconds': event['duration'],
        'fixedBankGainDb': c['fixedBankGainDb'], 'firstEventRenderFrames': frames,
        'coreFirstNote': spectral(note, rate, fundamental),
        'transient': {
            'maxAdjacentSampleDelta': float(abs(np.diff(note)).max()),
            'peakAmplitude': float(abs(note).max()),
            'maxDeltaOverPeak': float(abs(np.diff(note)).max()/max(abs(note).max(), 1e-30)),
            'coreAttackSeconds': frames*.08/rate, 'appAttackSecondsFromSource': .01,
            'coreEnvelopeJunctionDropFractionFromSource': float(1 - .92**1.8),
            'measuredAdjacentDeltaAtEnvelopeJunction': float(x[attack] - x[attack-1]),
            'corePostDurationReleaseSeconds': 0, 'appAdsrPostDurationReleaseSecondsFromSource': .2
        },
        'sampleRateDependenceDiagnostic': {
            'method': 'Same exact first Core event at 176.4 kHz, ffmpeg aresample=44100:filter_size=128:cutoff=0.95, compared to direct 44.1 kHz render.',
            'residualRmsRelativeDirectDb': db(np.sqrt(np.mean(residual**2))/max(noteRms, 1e-30)),
            'interiorResidualRmsRelativeDirectDb': db(np.sqrt(np.mean(interior**2))/max(internalRms, 1e-30)),
            'peakAbsoluteResidual': float(abs(residual).max()),
            'caveat': 'Contains aliasing plus resampler/phase-grid/envelope differences. Do not label this an isolated alias-distortion percentage.'
        },
        'beatVolume100xChangeHasIdenticalCorePcm': c['mixSensitivity']['identicalPcmDespite100xBeatVolume']
    }
    # Optionally consume a matching actual-app first note after the app worker provides it.
    app_first = ROOT / f'{c["name"]}.app-first-note.wav'
    if app_first.is_file():
        app_rate, app = read(app_first.name)
        result['appFirstNote'] = spectral(app[:int((event['duration']+.25)*app_rate)], app_rate, fundamental)
    results.append(result)
report = {'scope': 'Source-backed bass renderer diagnostic, not an artistic approval or score edit.',
    'commonFindings': {
        'actualBassNotesPreserved': True,
        'coreMainOscillator': 'Non-bandlimited mathematical sawtooth with no classic-bass recipe low-pass applied.',
        'appMainOscillator': 'Web Audio oscillator through a 420 Hz low-pass; 220 Hz low-pass on octave-lower sine.',
        'coreEnvelope': 'Attack occupies first 8% of note then discontinuously jumps to (1-position)^1.8; continuously decays to note end without added release.',
        'appEnvelope': '10 ms attack, 60 ms decay to 0.7 sustain, hold until note duration, 200 ms exponential release.',
        'mixBug': 'Core compact bass event velocity is fixed at0.34/0.42 and default stem gain includes bass volume0.86, so source beatVolume has no effect in these tests.',
        'aliasQualification': 'Sawtooth discontinuities imply aliasing without bandlimiting. The measured oversampling residual is broader than pure aliasing.',
        'separateIssue': 'Editor melody octave normalization is outside this bass test and must not be used as an assumed bass pitch fix.'
    }, 'comparisons': results}
(ROOT / 'signal-measurements.json').write_text(json.dumps(report, indent=2)+'\n')
for x in results:
    print(x['name'], 'mainHz', round(x['fundamentalHz'],2), 'top', round(x['coreFirstNote']['dominantPeaks'][0]['hz'],2),
          '>3k%', round(x['coreFirstNote']['energyAbove3000HzFraction']*100,3),
          'residualdB', round(x['sampleRateDependenceDiagnostic']['interiorResidualRmsRelativeDirectDb'],2),
          'attackms', round(x['transient']['coreAttackSeconds']*1000,2))
