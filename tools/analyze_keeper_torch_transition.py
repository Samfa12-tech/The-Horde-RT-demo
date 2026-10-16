"""Attribute the natural authored Keeper reveal transition, without changing rendering."""
import argparse
import csv
import io
import json
import math
from pathlib import Path

from analyze_android_motion_present_timing import AnalysisError, analyze_bytes, _identity
from analyze_android_rt_stage_profile import STAGES, join_stages, summary


def join_transition(report, rows):
    evidence = report['motionEvidence']
    frames, states = evidence['completedRtFrames'], evidence['states']
    stages = join_stages(rows, frames, held_views=())
    events = [e for e in evidence['events'] if e.get('type') == 17]  # append-only KeeperRevealStarted
    if len(events) != 1:
        raise AnalysisError('expected one authoritative Keeper reveal event')
    activation_row = events[0]['observedStateRow']
    active = []
    for index, (row, frame) in enumerate(zip(rows, frames)):
        state = states[frame['stateRow']]
        if row['phase'] != state['stage'] or state['keeper']['staff'] != 0 or state['keeper']['damagePulse']:
            raise AnalysisError('wrong owning stage or lightning contaminated the reveal measurement')
        light = frame.get('fireLighting')
        if not light or not 0 <= light.get('count', -1) <= 4:
            raise AnalysisError('missing owning uploaded fire-light prefix')
        emitters = light['emitters'][:light['count']]
        ids = [e['stableId'] for e in emitters]
        expected = [3, 4] if frame['stateRow'] >= activation_row else []
        if sorted(i for i in ids if i in (3, 4)) != expected or (
                row['phase'] in ('torch-baseline', 'torch-lit', 'complete') and sorted(ids) != expected):
            raise AnalysisError('uploaded torch IDs do not match the authoritative reveal boundary')
        for emitter in emitters:
            values = emitter['positionStrength'] + emitter['colourIntensity']
            if not all(math.isfinite(v) for v in values) or emitter['positionStrength'][3] <= 0:
                raise AnalysisError('invalid active torch upload')
        active.append(bool(expected))
    first = next((i for i, on in enumerate(active) if on), None)
    if first is None or first == 0 or not all(active[first:]):
        raise AnalysisError('missing off/on transition or light continuity')
    timings = {_identity(t, 'timing'): t['actualPresentTime'] for t in
               report['imagePresentationTiming']['capture']['rows']}
    times = [timings[_identity(f, 'frame', ledger=True)] for f in frames]
    groups = {}
    for label, phase in [('unlit', 'torch-baseline'), ('lit', 'torch-lit')]:
        indices = [i for i, r in enumerate(rows) if i and r['phase'] == phase and rows[i-1]['phase'] == phase]
        if not indices:
            raise AnalysisError('missing held ' + label + ' frames')
        # Discard the first second of each held view using real display time.
        start = times[indices[0]]
        indices = [i for i in indices if times[i] - start >= 1_000_000_000]
        if len(indices) < 20:
            raise AnalysisError('fewer than 20 settled ' + label + ' samples')
        for axis in ('x', 'y', 'z', 'yaw', 'pitch'):
            values = [float(rows[i][axis]) for i in indices]
            if not all(math.isfinite(v) for v in values) or max(values)-min(values) >= .0001:
                raise AnalysisError(label + ' camera is not held')
        groups[label] = {
            'firstSubmission': frames[indices[0]]['submission'], 'lastSubmission': frames[indices[-1]]['submission'],
            'viewAnchor': {a: float(rows[indices[0]][a]) for a in ('x', 'y', 'z', 'yaw', 'pitch')},
            'viewAnchorConvention': 'x/z player origin; y player support height, not optical eye; yaw/pitch are rendered view angles',
            'actualInterPresent': summary([times[i]-times[i-1] for i in indices]),
            'gpuStages': {s: summary([int(rows[i][s+'_gpu_ns']) for i in indices]) for s in STAGES},
            'deviceLocalBytes': sorted({frames[i]['deviceLocalBytes'] for i in indices}),
            'hostVisibleBytes': sorted({frames[i]['hostVisibleBytes'] for i in indices}),
            'tlasInstances': sorted({int(rows[i]['tlas_instances']) for i in indices}),
            'blasCount': sorted({int(rows[i]['blas_count']) for i in indices}),
            'skinUpdates': sorted({frames[i]['skinUpdates'] for i in indices}),
            'skinUpdatesPerFrame': sorted({frames[i]['skinUpdates']-frames[i-1]['skinUpdates'] for i in indices}),
            'pipelineIndices': sorted({frames[i]['pipelineIndex'] for i in indices}),
            'simulationTicksPerFrame': sorted({states[frames[i]['stateRow']]['ticksThisFrame'] for i in indices}),
            'catchUpOverrunIncrease': states[frames[indices[-1]]['stateRow']]['overruns']-
                states[frames[indices[0]-1]['stateRow']]['overruns'],
        }
    if len(groups['unlit']['pipelineIndices'])!=1 or groups['unlit']['pipelineIndices']!=groups['lit']['pipelineIndices']:
        raise AnalysisError('held views switched shader pipelines')
    window = range(max(1, first-3), min(len(frames), first+7))
    return {'stageJoin': stages, 'heldViews': groups, 'firstLitSubmission': frames[first]['submission'],
        'transitionFrames': [{'submission': frames[i]['submission'], 'tick': frames[i]['tick'],
            'actualIntervalMs': (times[i]-times[i-1])/1e6,
            'asMs': int(rows[i]['as_gpu_ns'])/1e6, 'dispatchMs': int(rows[i]['dispatch_gpu_ns'])/1e6,
            'uploadedLights': frames[i]['fireLighting'], 'keeper': states[frames[i]['stateRow']]['keeper']}
            for i in window],
        'limits': 'Natural reveal co-activates actor presentation and two lights. Small ordinary arrival displacement is reported; this is not a light-only counterfactual or sustained gameplay FPS.'}


def analyze(raw, csv_text):
    strict = analyze_bytes(raw)
    report = json.loads(raw)
    if report['scenario'] != 'keeper-torch-transition':
        raise AnalysisError('requires the actual authored chapter transition')
    result = join_transition(report, list(csv.DictReader(io.StringIO(csv_text))))
    result['strictTiming'] = strict
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('resources', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = analyze(args.report.read_bytes(), args.resources.read_text())
    except (AnalysisError, ValueError, KeyError, OSError) as error:
        parser.exit(1, f'Torch transition rejected: {error}\n')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
