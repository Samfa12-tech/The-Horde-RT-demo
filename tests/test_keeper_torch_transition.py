import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from analyze_keeper_torch_transition import join_transition, AnalysisError


def fixture():
    frames, states, rows, timings = [], [], [], []
    scope = dict(surfaceGeneration=1, sceneEpoch=2, measurementGeneration=3)
    for i in range(120):
        on = i >= 60
        phase = 'torch-lit' if on else 'torch-baseline'
        states.append(dict(stage=phase, keeper=dict(staff=0, damagePulse=False),ticksThisFrame=3,overruns=0))
        emitters = [dict(stableId=j, positionStrength=[0, 1, 0, .88], colourIntensity=[1, .5, .1, .88]) for j in (3, 4)] if on else []
        frames.append(dict(**scope, stateRow=i, submission=i+1, record=i+1, tick=i*3,
            fireLighting=dict(count=len(emitters), emitters=emitters),
            deviceLocalBytes=1, hostVisibleBytes=2, skinUpdates=3, pipelineIndex=0))
        row = dict(serial=str(i+1), tick=str(i*3), phase=phase, x='-.05' if on else '0', y='0', z='0',
                   yaw='1', pitch='0', tlas_instances='28', blas_count='22')
        for s in ('as', 'dispatch', 'transfer'):
            row.update({s+'_gpu_status':'valid', s+'_gpu_serial':str(i+1), s+'_gpu_ns':'1000000'})
        rows.append(row)
        timings.append(dict(**scope, simulationTick=i*3, submissionSerial=i+1, recordSerial=i+1,
                            actualPresentTime=1_000_000_000+i*50_000_000))
    return dict(motionEvidence=dict(states=states, completedRtFrames=frames,
        events=[dict(type=17, observedStateRow=60)]),
        imagePresentationTiming=dict(capture=dict(rows=timings))), rows


class TorchTransitionTests(unittest.TestCase):
    def test_complete_owning_join(self):
        report, rows = fixture()
        result = join_transition(report, rows)
        self.assertEqual(result['firstLitSubmission'], 61)
        self.assertEqual(result['heldViews']['lit']['actualInterPresent']['medianMs'], 50)

    def test_light_before_event(self):
        report, rows = fixture()
        report['motionEvidence']['completedRtFrames'][59]['fireLighting'] = copy.deepcopy(report['motionEvidence']['completedRtFrames'][60]['fireLighting'])
        with self.assertRaises(AnalysisError): join_transition(report, rows)

    def test_missing_second_light(self):
        report, rows = fixture()
        report['motionEvidence']['completedRtFrames'][80]['fireLighting']['count'] = 1
        with self.assertRaises(AnalysisError): join_transition(report, rows)

    def test_wrong_gpu_owner(self):
        report, rows = fixture()
        rows[80]['dispatch_gpu_serial'] = '80'
        with self.assertRaises(AnalysisError): join_transition(report, rows)

    def test_lightning_contamination(self):
        report, rows = fixture()
        report['motionEvidence']['states'][80]['keeper']['staff'] = .1
        with self.assertRaises(AnalysisError): join_transition(report, rows)

    def test_camera_drift(self):
        report, rows = fixture()
        rows[100]['x'] = '.1'
        with self.assertRaises(AnalysisError): join_transition(report, rows)

    def test_duplicate_event(self):
        report, rows = fixture()
        report['motionEvidence']['events'] *= 2
        with self.assertRaises(AnalysisError): join_transition(report, rows)

    def test_shader_switch_is_not_light_attribution(self):
        report, rows = fixture()
        report['motionEvidence']['completedRtFrames'][100]['pipelineIndex'] = 1
        with self.assertRaises(AnalysisError): join_transition(report, rows)


if __name__ == '__main__': unittest.main()
