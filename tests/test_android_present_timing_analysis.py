import copy
import io
import json
import sys
import unittest
from contextlib import redirect_stderr
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from analyze_android_present_timing import AnalysisError, analyze_bytes, main


U64_MAX = (1 << 64) - 1


def make_row(generation, scene, serial, present_id, actual, *, tick=2, surface=1, swapchain=1,
             scale=50, width=1080, height=2400, backend='RayTracingPipeline', margin=0):
    return {
        'surfaceGeneration': surface, 'swapchainSerial': swapchain,
        'measurementGeneration': generation, 'sceneEpoch': scene,
        'recordSerial': serial, 'submissionSerial': serial,
        'simulationTick': tick, 'queuedSteadyNs': actual - 1000,
        'presentID': present_id, 'actualPresentTime': actual,
        'earliestPresentTime': actual, 'presentMargin': margin,
        'scalePercent': scale, 'width': width, 'height': height, 'backend': backend,
    }


def make_report():
    frames = []
    timings = [make_row(4, 1, 1, 1, 1_000_000, scale=75, width=1440, height=2980),
               make_row(4, 1, 2, 2, 2_000_000, scale=75, width=1440, height=2980)]
    for offset, (present_id, actual) in enumerate(((3, 100_000_000), (4, 133_333_000), (5, 183_333_000)), start=10):
        frames.append({
            'disposition': 'completed', 'presentationOutcome': 'presented', 'cpuAccepted': True,
            'submittedIdentity': {'sceneEpoch': 2, 'measurementGeneration': 6, 'recordSerial': offset,
                                  'submissionSerial': offset, 'simulationTick': 2},
            'completionIdentity': {'sceneEpoch': 2, 'measurementGeneration': 6, 'recordSerial': offset,
                                   'submissionSerial': offset, 'simulationTick': 2},
        })
        timings.append(make_row(6, 2, offset, present_id, actual, margin=U64_MAX - 50_000_000))
    return {
        'schema': 2, 'result': 'complete', 'status': 'complete', 'invalidRun': False, 'workloadComplete': True,
        'measuredFrames': 3,
        'runId': 'fixture-safe', 'timestampUtc': '2026-10-08T00:00:00Z', 'build': '1.6.2',
        'shader': 'fixture-shader', 'gpu': 'Fixture GPU', 'vulkanApi': '1.3', 'rtMode': 'rt',
        'executionBackend': 'RayTracingPipeline', 'presentMode': 'MAILBOX',
        'workload': 'lantern-held-high-v1', 'simulationPolicy': 'frozen-authored-snapshot',
        'renderScalePercent': 50, 'internalExtent': {'width': 540, 'height': 1200},
        'presentationExtent': {'width': 1080, 'height': 2400}, 'lapsCompleted': 2,
        'lapsRequested': 2, 'routeTraversalComplete': False,
        'completedFrameEvidence': {
            'schema': 1, 'status': 'complete', 'invalidRun': False, 'sceneEpoch': 2, 'measurementGeneration': 6,
            'counts': {'expected': 3, 'completed': 3, 'rejected': 0, 'cancelled': 0,
                       'cpuAccepted': 3, 'outstanding': 0, 'cpuRejected': 0}, 'rows': frames,
        },
        'imagePresentationTiming': {
            'extensionEnabled': True, 'queryCpuWallNanoseconds': 123456,
            'capture': {
                'schemaVersion': 1,
                'status': {'enabled': True, 'bound': True, 'surfaceGeneration': 1,
                           'swapchainSerial': 1, 'pendingCount': 0, 'lastQueryResult': 0},
                'counters': {'queryCalls': 10, 'acceptedPresents': 5, 'incompleteQueries': 1, 'queryErrors': 0,
                             'zeroPresentTimestamps': 0, 'zeroPresentIds': 0, 'unknownPresentIds': 0,
                             'duplicateTimings': 0, 'rowCapacityExhausted': 0,
                             'pendingCapacityExhausted': 0, 'missingOnRebind': 0, 'missingOnUnbind': 0,
                             'invalidRegistrations': 0, 'invalidMetadata': 0,
                             'abandonedPreparedPresents': 0, 'rejectedPresents': 0},
                'rows': timings, 'unresolved': [],
            },
        },
    }


def encoded(report):
    return json.dumps(report, separators=(',', ':')).encode('utf-8')


class AndroidPresentTimingAnalysisTests(unittest.TestCase):
    def test_valid_join_ignores_warmup_and_wrapped_margin(self):
        result = analyze_bytes(encoded(make_report()))
        self.assertTrue(result['eligible'])
        self.assertEqual(result['settings']['result'], 'complete')
        self.assertEqual(result['settings']['measuredFrames'], 3)
        self.assertEqual(result['scope']['matchedCompletedFrames'], 3)
        self.assertEqual(result['scope']['intervalCount'], 2)
        self.assertEqual(result['intervalsNanoseconds']['median'], 41_666_500)
        self.assertEqual(result['intervalsNanoseconds']['p90NearestRank'], 50_000_000)
        self.assertEqual(result['intervalsNanoseconds']['over33333000Nanoseconds'], 1)
        self.assertEqual(result['collector']['presentMarginAtLeast2Pow63CountIgnored'], 3)
        self.assertEqual(result['scope']['durationNanoseconds'], 83_333_000)
        self.assertAlmostEqual(result['observedPresentedImageRatePerSecond'], 24.000096)
        self.assertFalse(result['settings']['routeTraversalComplete'])

    def assert_ineligible(self, report, message_fragment):
        with self.assertRaisesRegex(AnalysisError, message_fragment):
            analyze_bytes(encoded(report))

    def test_rejects_duplicate_present_and_join_ids(self):
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'][3]['presentID'] = 3
        self.assert_ineligible(report, 'duplicate presentID')
        report = make_report()
        duplicate = copy.deepcopy(report['completedFrameEvidence']['rows'][0])
        report['completedFrameEvidence']['rows'].append(duplicate)
        report['completedFrameEvidence']['counts']['expected'] = 4
        report['completedFrameEvidence']['counts']['completed'] = 4
        report['completedFrameEvidence']['counts']['cpuAccepted'] = 4
        report['measuredFrames'] = 4
        self.assert_ineligible(report, 'duplicate completed-frame identity')

    def test_rejects_missing_join_row_and_id_or_serial_gap(self):
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'].pop()
        self.assert_ineligible(report, 'timing row count')
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'][3]['presentID'] = 9
        self.assert_ineligible(report, 'IDs contain a gap')
        report = make_report()
        report['completedFrameEvidence']['rows'][1]['submittedIdentity']['submissionSerial'] = 12
        report['completedFrameEvidence']['rows'][1]['completionIdentity']['submissionSerial'] = 12
        report['imagePresentationTiming']['capture']['rows'][3]['submissionSerial'] = 12
        self.assert_ineligible(report, 'IDs contain a gap')

    def test_rejects_cross_generation_or_mixed_swapchain_rows(self):
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'][3]['measurementGeneration'] = 4
        self.assert_ineligible(report, 'timing row count')
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'][4]['swapchainSerial'] = 2
        self.assert_ineligible(report, 'mixed surface generations')

    def test_rejects_nonincreasing_actual_times_and_metadata_mismatch(self):
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'][4]['actualPresentTime'] = 99_000_000
        self.assert_ineligible(report, 'strictly increase')
        report = make_report()
        report['imagePresentationTiming']['capture']['rows'][4]['actualPresentTime'] = 100_000_000
        self.assert_ineligible(report, 'strictly increase')
        for field, value, expected_error in (
            ('simulationTick', 3, 'simulationTick differs'),
            ('scalePercent', 40, 'scalePercent differs'),
            ('width', 1000, 'extent differs'),
            ('backend', 'RayQueryCompute', 'backend differs'),
        ):
            with self.subTest(field=field):
                report = make_report()
                report['imagePresentationTiming']['capture']['rows'][3][field] = value
                self.assert_ineligible(report, expected_error)

    def test_rejects_invalid_incomplete_rejected_or_unresolved_evidence(self):
        for mutate, fragment in (
            (lambda r: r.update(invalidRun=True), 'marks the run invalid'),
            (lambda r: r['completedFrameEvidence'].update(status='partial'), 'status is not complete'),
            (lambda r: r['completedFrameEvidence']['counts'].update(rejected=1), 'nonzero rejected'),
            (lambda r: r['completedFrameEvidence']['counts'].update(cpuAccepted=2), 'cpuAccepted count differs'),
            (lambda r: r['completedFrameEvidence']['counts'].update(outstanding=1), 'nonzero outstanding'),
            (lambda r: r['imagePresentationTiming']['capture']['status'].update(pendingCount=1), 'unresolved presents'),
            (lambda r: r['imagePresentationTiming']['capture'].update(unresolved=[{'presentID': 99}]), 'exported unresolved'),
            (lambda r: r['imagePresentationTiming']['capture']['counters'].update(acceptedPresents=4), 'accepted presentation count'),
            (lambda r: r['imagePresentationTiming']['capture']['counters'].update(missingOnUnbind=1), 'nonzero missingOnUnbind'),
            (lambda r: r['imagePresentationTiming']['capture']['counters'].update(queryErrors=1), 'nonzero queryErrors'),
            (lambda r: r['imagePresentationTiming']['capture']['counters'].update(rowCapacityExhausted=1), 'nonzero rowCapacityExhausted'),
        ):
            with self.subTest(fragment=fragment):
                report = make_report()
                mutate(report)
                self.assert_ineligible(report, fragment)

    def test_cli_emits_json_error_and_nonzero_exit_for_ineligible_data(self):
        stderr = io.StringIO()
        with redirect_stderr(stderr):
            exit_code = main(['missing-report.json'])
        self.assertEqual(exit_code, 2)
        error = json.loads(stderr.getvalue())
        self.assertFalse(error['eligible'])
        self.assertTrue(error['error'].startswith('cannot read report:'))

    def test_top_level_optional_invalid_marker_may_be_absent(self):
        report = make_report()
        report.pop('invalidRun')
        self.assertTrue(analyze_bytes(encoded(report))['eligible'])

    def test_incomplete_query_count_is_informational_when_rows_join(self):
        report = make_report()
        report['imagePresentationTiming']['capture']['counters']['incompleteQueries'] = 6
        self.assertTrue(analyze_bytes(encoded(report))['eligible'])


if __name__ == '__main__':
    unittest.main()
