"""Small synthetic corruption checks; no runtime or performance acceptance."""
import copy
import csv
import importlib.util
import io
import json
from pathlib import Path
import sys
import unittest

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE))
import analyze_skin_hold as candidate

old_path = HERE / 'test_analyze_hold.py'
spec = importlib.util.spec_from_file_location('preserved_hold_fixture', old_path)
fixture_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture_module)


def fixture():
    report, rows = fixture_module.fixture()
    for index, row in enumerate(rows):
        for stage in candidate.STAGES:
            row.update({stage + '_ns': index * 2000, stage + '_calls': 2,
                        stage + '_bytes': 128, stage + '_operations': 24})
    return report, rows


def run(report, rows):
    stream = io.StringIO()
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
    return candidate.analyze(json.dumps(report).encode(), stream.getvalue(), 20)


class ChildAttributionTests(unittest.TestCase):
    def test_exact_sample_window_and_call_means(self):
        report, rows = fixture()
        result = run(report, rows)
        self.assertEqual(result['childSampling']['firstCompletedIndex'], 10)
        self.assertEqual(result['childSampling']['lastCompletedIndex'], 40)
        self.assertEqual(result['childSampling']['completedFramesPostWarm'], 31)
        for stage in candidate.STAGES:
            value = result['childCpu'][stage]
            self.assertEqual(value['totalCalls'], 62)
            self.assertEqual(value['totalBytes'], 3968)
            self.assertEqual(value['allFramesMs']['median'], .05)
            self.assertEqual(value['weightedMeanPerInvocationMs'], .025)
            self.assertEqual(value['perActiveFrameMeanInvocationMs']['median'], .025)

    def test_invalid_child_facts_in_excluded_warmup_also_reject(self):
        bad_values = ('-1', '1.2', 'NaN', str(1 << 64), '\u0661')
        for suffix in candidate.SUFFIXES:
            for bad in bad_values:
                with self.subTest(suffix=suffix, value=bad):
                    report, rows = fixture()
                    rows[0][candidate.STAGES[0] + '_' + suffix] = bad
                    with self.assertRaises(candidate.AnalysisError):
                        run(report, rows)

    def test_missing_child_column_rejects(self):
        report, rows = fixture()
        for row in rows:
            row.pop(candidate.STAGES[-1] + '_bytes')
        with self.assertRaises(candidate.AnalysisError):
            run(report, rows)

    def test_zero_calls_require_zero_facts(self):
        report, rows = fixture()
        rows[20][candidate.STAGES[0] + '_calls'] = 0
        with self.assertRaises(candidate.AnalysisError):
            run(report, rows)
        for suffix in candidate.SUFFIXES:
            rows[20][candidate.STAGES[0] + '_' + suffix] = 0
        value = run(report, rows)['childCpu'][candidate.STAGES[0]]
        self.assertEqual(value['activeCallFrames'], 30)
        self.assertEqual(value['totalCalls'], 60)

    def test_strict_owner_corruption_not_rescued(self):
        report, rows = fixture()
        rows[20]['serial'] = 9999
        with self.assertRaises(candidate.AnalysisError):
            run(report, rows)

    def test_selection_uses_owner_wall_not_return_or_ticks(self):
        report, rows = fixture()
        # Return timestamps may be later than the owning state, but selection must
        # still choose index10; shifting all returns preserves strict continuity.
        for row in rows:
            row['present_return_ns'] += 9_000_000_000
        result = run(report, rows)
        self.assertEqual(result['childSampling']['firstCompletedIndex'], 10)
        report['outdoorHold']['startWallNanoseconds'] = 10_000_000_000
        with self.assertRaises(candidate.AnalysisError):
            run(report, rows)


if __name__ == '__main__':
    unittest.main()
