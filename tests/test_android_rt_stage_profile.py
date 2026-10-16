import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from analyze_android_rt_stage_profile import AnalysisError, STAGES, VIEWS, join_stages


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


class StageJoinTests(unittest.TestCase):
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
