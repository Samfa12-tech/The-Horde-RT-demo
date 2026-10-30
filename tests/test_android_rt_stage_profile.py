import copy
import csv
import io
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from analyze_android_rt_stage_profile import AnalysisError, STAGES, VIEWS, OUTDOOR_HOLD_PHASE, analyze, join_stages
from test_android_motion_present_timing import encoded, make_outdoor_report


def fixture():
    rows, frames = [], []
    for phase in VIEWS:
        for tick in range(60, 85):
            serial = len(rows) + 1
            row = dict(serial=str(serial), tick=str(serial * 6), phase=phase, view_ticks=str(tick),
                       x="0", y="1", z="2", yaw="3", pitch="4")
            for stage in STAGES:
                row.update({stage + "_gpu_status": "valid", stage + "_gpu_serial": str(serial),
                            stage + "_gpu_ns": "1234567"})
            rows.append(row)
            frames.append(dict(submission=serial, tick=serial * 6))
    return rows, frames


def outdoor_fixture():
    report = make_outdoor_report(20)
    rows = []
    for frame in report['motionEvidence']['completedRtFrames']:
        row = dict(serial=str(frame['submission']), tick=str(frame['tick']), phase=OUTDOOR_HOLD_PHASE,
                   view_ticks=str(frame['tick']), x='0', y='1', z='2', yaw='3', pitch='4')
        for stage in STAGES:
            # Warmup/final frames must join, but cannot bias the post-warm median.
            value = '1234567' if 10 <= frame['stateRow'] <= 40 else '999999999'
            row.update({stage + '_gpu_status': 'valid', stage + '_gpu_serial': row['serial'], stage + '_gpu_ns': value})
        rows.append(row)
    return report, rows


def csv_text(rows):
    text = io.StringIO()
    writer = csv.DictWriter(text, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)
    return text.getvalue()


class StageJoinTests(unittest.TestCase):
    def test_explicit_outdoor_uses_owner_wall_window_and_actual_phase(self):
        report, rows = outdoor_fixture()
        result = analyze(encoded(report), csv_text(rows), outdoor_hold=True)
        self.assertEqual(result['joinedFrames'], 43)
        self.assertEqual(result['views'][OUTDOOR_HOLD_PHASE]['dispatch']['n'], 31)
        self.assertEqual(result['views'][OUTDOOR_HOLD_PHASE]['dispatch']['medianMs'], 1.234567)
        with self.assertRaises(AnalysisError):
            analyze(encoded(report), csv_text(rows))

    def test_outdoor_never_omits_warmup_or_final_failures(self):
        for index in (0, -1):
            for field, value in (('as_gpu_status', 'unavailable-or-mismatched'),
                                 ('dispatch_gpu_serial', '99'), ('serial', '1'), ('tick', '1'),
                                 ('phase', 'landing-sweep')):
                with self.subTest(index=index, field=field):
                    report, rows = outdoor_fixture()
                    rows[index][field] = value
                    with self.assertRaises(AnalysisError):
                        analyze(encoded(report), csv_text(rows), outdoor_hold=True)
        report, rows = outdoor_fixture()
        with self.assertRaises(AnalysisError):
            analyze(encoded(report), csv_text(rows[:-1]), outdoor_hold=True)
        rows[20]['x'] = '0.1'
        with self.assertRaises(AnalysisError):
            analyze(encoded(report), csv_text(rows), outdoor_hold=True)

    def test_all_including_final_frame(self):
        rows, frames = fixture()
        result = join_stages(rows, frames)
        self.assertEqual(result["joinedFrames"], 100)
        self.assertEqual(result["views"][VIEWS[0]]["dispatch"]["medianMs"], 1.234567)

    def test_missing_final_frame_is_failure(self):
        rows, frames = fixture()
        with self.assertRaises(AnalysisError):
            join_stages(rows[:-1], frames)

    def test_bad_stage_is_never_zero_or_omitted(self):
        for stage in STAGES:
            for field, value in (("status", "unavailable-or-mismatched"), ("serial", "99"),
                                 ("ns", ""), ("ns", "-1"), ("ns", "NaN"),
                                 ("ns", str(1 << 64))):
                with self.subTest(stage=stage, field=field, value=value):
                    rows, frames = fixture()
                    rows[-1][stage + "_gpu_" + field] = value
                    with self.assertRaises(AnalysisError):
                        join_stages(rows, frames)

    def test_wrong_or_duplicate_owner(self):
        rows, frames = fixture()
        for field, value in (("serial", "1"), ("tick", "0")):
            changed = copy.deepcopy(rows)
            changed[-1][field] = value
            with self.assertRaises(AnalysisError):
                join_stages(changed, frames)

    def test_moving_view_or_insufficient_hold(self):
        rows, frames = fixture()
        rows[4]["x"] = "0.1"
        with self.assertRaises(AnalysisError):
            join_stages(rows, frames)
        rows, frames = fixture()
        for row in rows[:25]:
            row["view_ticks"] = "0"
        with self.assertRaises(AnalysisError):
            join_stages(rows, frames)


if __name__ == "__main__":
    unittest.main()
