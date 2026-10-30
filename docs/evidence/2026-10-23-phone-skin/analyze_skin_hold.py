#!/usr/bin/env python3
"""Offline child CPU attribution composed with the unchanged strict hold analyzer.

No device access. The six child skin stages are subsets of existing parents;
BLAS/TLAS are CPU command-record durations, not GPU execution measurements.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import statistics
import sys

ROOT = Path(__file__).resolve().parents[3]
BASE_PATH = Path(__file__).with_name('analyze_hold.py')
spec = importlib.util.spec_from_file_location('preserved_outdoor_hold', BASE_PATH)
BASE = importlib.util.module_from_spec(spec)
spec.loader.exec_module(BASE)
AnalysisError = BASE.AnalysisError
U64_MAX = (1 << 64) - 1
STAGES = ('playerPoseCpuMs', 'playerVerticesCpuMs', 'viewmodelVerticesCpuMs',
          'characterAnimationCpuMs', 'characterVerticesCpuMs', 'characterCopyCpuMs',
          'blasRefitRecordCpuMs', 'tlasUpdateRecordCpuMs')
SUFFIXES = ('ns', 'calls', 'bytes', 'operations')


def uint64(value, label):
    if not isinstance(value, str) or not value.isascii() or not value.isdecimal():
        raise AnalysisError(label + ': expected ASCII unsigned integer')
    result = int(value)
    if result > U64_MAX:
        raise AnalysisError(label + ': exceeds uint64')
    return result


def stats(values, divisor=1):
    if not values:
        return None
    ordered = sorted(values)
    return {'samples': len(values), 'minimum': ordered[0] / divisor,
            'median': statistics.median(ordered) / divisor,
            'mean': statistics.mean(ordered) / divisor,
            'p95NearestRank': ordered[math.ceil(.95 * len(ordered)) - 1] / divisor,
            'maximum': ordered[-1] / divisor}


def summarize(observations, indices):
    selected = [observations[index] for index in indices]
    active = [item for item in selected if item['calls']]
    total_calls = sum(item['calls'] for item in selected)
    total_ns = sum(item['ns'] for item in selected)
    return {'completedFrames': len(selected), 'activeCallFrames': len(active),
            'totalCalls': total_calls, 'totalDurationNs': total_ns,
            'totalBytes': sum(item['bytes'] for item in selected),
            'totalOperations': sum(item['operations'] for item in selected),
            'allFramesMs': stats([item['ns'] for item in selected], 1e6),
            'activeFramesMs': stats([item['ns'] for item in active], 1e6),
            'callsPerFrame': stats([item['calls'] for item in selected]),
            'bytesPerFrame': stats([item['bytes'] for item in selected]),
            'operationsPerFrame': stats([item['operations'] for item in selected]),
            'weightedMeanPerInvocationMs': total_ns / total_calls / 1e6 if total_calls else None,
            'perActiveFrameMeanInvocationMs': stats(
                [item['ns'] / item['calls'] for item in active], 1e6)}


def analyze(raw, resources, expected_scale, expected_settings=None):
    # Preserve all original strict presentation, CPU ownership, phase, residency,
    # settings and live simulation admission. Additional columns never rescue it.
    strict = BASE.analyze(raw, resources, expected_scale, expected_settings)
    report = json.loads(raw, object_pairs_hook=BASE._object,
                        parse_constant=BASE._reject_constant)
    reader = csv.DictReader(io.StringIO(resources))
    fields = reader.fieldnames or []
    required = [stage + '_' + suffix for stage in STAGES for suffix in SUFFIXES]
    if any(fields.count(field) != 1 for field in required):
        raise AnalysisError('each appended child column must occur exactly once')
    rows = list(reader)
    frames = report['motionEvidence']['completedRtFrames']
    states = report['motionEvidence']['states']
    if len(rows) != len(frames):
        raise AnalysisError('child rows must match every strict completed RT owner')
    values = {stage: [] for stage in STAGES}
    for row_index, row in enumerate(rows):
        if None in row or any(value is None for value in row.values()):
            raise AnalysisError('ragged child CSV row ' + str(row_index))
        for stage in STAGES:
            item = {suffix: uint64(row[stage + '_' + suffix],
                                  f'row {row_index} {stage}_{suffix}') for suffix in SUFFIXES}
            if item['calls'] == 0 and any(item[key] for key in ('ns', 'bytes', 'operations')):
                raise AnalysisError(f'row {row_index} {stage}: facts without invocation')
            values[stage].append(item)
    sampling = strict['sampling']
    warm, end = sampling['warmupEndWallNs'], sampling['sampleEndWallNs']
    selected = [index for index, frame in enumerate(frames)
                if warm <= states[frame['stateRow']]['wallNs'] <= end]
    if len(selected) != sampling['completedFramesPostWarm'] or not selected:
        raise AnalysisError('child selection differs from strict post-warm sample count')
    selected_walls = [states[frames[index]['stateRow']]['wallNs'] for index in selected]
    if (selected_walls[0] != sampling['actualSelectedFirstWallNs'] or
            selected_walls[-1] != sampling['actualSelectedLastWallNs']):
        raise AnalysisError('child wall window differs from strict owning-state window')
    return {'schema': 1, 'eligible': True, 'strictHold': strict,
            'childSampling': {'source': 'strict completed owner stateRow.wallNs; inclusive 10s to 40s',
                              'firstCompletedIndex': selected[0], 'lastCompletedIndex': selected[-1],
                              'completedFramesPostWarm': len(selected)},
            'childCpu': {stage: summarize(values[stage], selected) for stage in STAGES},
            'allCompletedChildTotals': {
                stage: {key: sum(item[key] for item in observations) for key in SUFFIXES}
                for stage, observations in values.items()},
            'analyzerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'preservedStrictAnalyzerSha256': hashlib.sha256(BASE_PATH.read_bytes()).hexdigest(),
            'limits': [
                'CSV stage prefixes end CpuMs but _ns values are nanoseconds.',
                'Elapsed stage durations are not scheduler CPU utilization or CPU busy time.',
                'Child stages are subsets of skin parents; do not add overlapping parent/child or CPU/GPU durations.',
                'Per-invocation figures divide each accumulated duration by its call count; individual call timings are unavailable.',
                'Bytes describe observed representation output, not measured memory bus traffic.',
                'BLAS/TLAS durations cover CPU command recording, not GPU AS execution.',
                'Stationary staged live simulation is not natural route, image quality or audio listening acceptance.']}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('resources', type=Path)
    parser.add_argument('--expect-scale', type=int, choices=(20, 33, 40), required=True)
    parser.add_argument('--expect-tuple', help='Optional explicit preserved water/fire/cap/glass/shadow/mist/dust JSON')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args(argv)
    try:
        with args.report.open('rb') as stream:
            raw = stream.read(BASE.MAX_REPORT_BYTES + 1)
        with args.resources.open('r', encoding='utf-8-sig', newline='') as stream:
            resources = stream.read(BASE.MAX_REPORT_BYTES + 1)
        expected = json.loads(args.expect_tuple, object_pairs_hook=BASE._object,
                              parse_constant=BASE._reject_constant) if args.expect_tuple else None
        result = analyze(raw, resources, args.expect_scale, expected)
        text = json.dumps(result, indent=2, allow_nan=False) + '\n'
        if args.output:
            # All locally created artifacts stay in the authorized report root.
            destination = args.output.resolve()
            if not destination.is_relative_to(ROOT / 'reports'):
                raise AnalysisError('raw analysis output must stay inside ignored reports/')
            with destination.open('x', encoding='utf-8', newline='\n') as stream:
                stream.write(text)
        else:
            print(text, end='')
        return 0
    except (AnalysisError, OSError, KeyError, ValueError, TypeError) as error:
        print(json.dumps({'eligible': False, 'error': str(error)}), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
