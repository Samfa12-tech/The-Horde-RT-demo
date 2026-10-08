import copy
from contextlib import redirect_stderr
import io
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from analyze_android_motion_present_timing import AnalysisError, COUNTERS, LIMITS, POLICY, analyze_bytes, main


def make_report():
    settings = {"scalePercent": 50, "width": 1440, "height": 2980, "internalWidth": 720,
                "internalHeight": 1490, "backend": "RayTracingPipeline", "water": 1, "fire": 0,
                "cap": 30, "glass": False, "shadow": 1, "mist": True, "dust": 0}
    scope = {"surfaceGeneration": 1, "sceneEpoch": 2, "measurementGeneration": 3}
    states, frames, timings = [], [], []
    for i in range(3):
        states.append({"row": i, "wallNs": 1_000_000_000 + i * 50_000_000,
            "tick": i * 3, "publication": 10, "overruns": int(i == 2), "ticksThisFrame": 3,
            "simulationSeconds": i * .05, "axes": [1., 0.]})
        frames.append({**scope, "stateRow": i, "tick": i * 3, "record": 11 + i,
            "recordAttempt": 11 + i, "submission": 11 + i, "completion": 11 + i,
            "actualUploadedDustQuality": 0, "actualUploadedMistEnabled": True})
        timings.append({**scope, "simulationTick": i * 3, "recordSerial": 11 + i,
            "submissionSerial": 11 + i, "presentID": 11 + i, "scalePercent": 50,
            "width": 1440, "height": 2980, "backend": "RayTracingPipeline", "swapchainSerial": 1,
            "actualPresentTime": 2_000_000_000 + i * 50_000_000,
            "earliestPresentTime": 0, "presentMargin": (1 << 64) - 1})
    setup = copy.deepcopy(timings[0])
    setup.update(measurementGeneration=2, recordSerial=10, submissionSerial=10,
                 simulationTick=0, presentID=10, actualPresentTime=1_500_000_000)
    counters = dict.fromkeys(COUNTERS, 0)
    counters.update(preparedPresents=4, acceptedPresents=4, queryCalls=5, incompleteQueries=1)
    baseline = dict.fromkeys(COUNTERS, 0)
    baseline.update(preparedPresents=1, acceptedPresents=1, queryCalls=1)
    delta = {field: counters[field] - baseline[field] for field in COUNTERS}
    return {"schema": 1, "result": "complete", "runId": "fixture-01", "scenario": "waterfall-equipment",
        "workload": "motion-waterfall-equipment-v1", "buildId": "fixture-source", "simulationPolicy": POLICY,
        "limits": dict(LIMITS), "settings": settings,
        "startingFrameIdentity": {**scope, "recordSerial": 11, "submissionSerial": 11, "simulationTick": 0},
        "motionManifest": {"schema": 1, "runId": "fixture-01", "scenario": "waterfall-equipment", **scope,
            "finished": True, "complete": True, "armed": True, "scale": 50, "water": 1, "fire": 0,
            "cap": 30, "glass": False, "shadow": 1, "mist": True, "dust": 0, "captures": 3,
            "isolation": dict.fromkeys(("secondSimulation", "fixedDeltaOverride", "phaseForced",
                                         "preferencesWritten", "ownerAcceptance"), False)},
        "motionEvidence": {"schema": 1, "runId": "fixture-01", "scenario": "waterfall-equipment",
            "scenarioComplete": True, "failure": "", "pendingSubmissionCount": 0, "currentScopePresented": True,
            "states": states, "events": [{"sequence": 1, "observedStateRow": 0}],
            "completedRtFrames": frames, "resourceScopes": [{**scope, "nextStateRow": 0}], "retiredSubmissions": []},
        "motionImageCaptures": [{"file": f"android-motion-fixture-01-{i}.rgba", "width": 720,
            "height": 1490, "bytes": 720 * 1490 * 4, "stateRow": i, "rtRow": i} for i in range(3)],
        "timingEligibility": {"eligible": True, "matchedCompletedFrames": 3, "expectedCompletedFrames": 3},
        "imagePresentationTiming": {"extensionEnabled": True, "queryCpuWallNanoseconds": 1100,
            "rowIndexBeforeRun": 1, "counterBaseline": baseline, "counterDelta": delta,
            "capture": {"schemaVersion": 1, "status": {"enabled": True, "bound": True,
                "pendingCount": 0, "surfaceGeneration": 1, "swapchainSerial": 1},
                "counters": counters, "rows": [setup] + timings, "unresolved": []}}}


def encoded(report):
    return json.dumps(report, separators=(",", ":")).encode()


class AndroidMotionPresentTimingTests(unittest.TestCase):
    def reject(self, report, fragment):
        with self.assertRaisesRegex(AnalysisError, fragment):
            analyze_bytes(encoded(report))

    def test_real_join_retains_overrun_and_excludes_setup_and_margin(self):
        result = analyze_bytes(encoded(make_report()))
        self.assertTrue(result["eligible"])
        self.assertEqual(result["scope"]["matchedCompletedFrames"], 3)
        self.assertEqual(result["scope"]["setupPresentationRowsExcluded"], 1)
        self.assertEqual(result["intervalsNanoseconds"]["median"], 50_000_000)
        self.assertEqual(result["observedPresentedImageRatePerSecond"], 20.)
        self.assertEqual(result["simulation"]["catchUpOverrunDelta"], 1)
        self.assertEqual(result["collector"]["presentMarginAtLeast2Pow63CountIgnored"], 3)

    def test_exact_identity_includes_state_tick_and_starting_owner(self):
        for section, field, value, fragment in (("completedRtFrames", "tick", 7, "bound state"),
                ("completedRtFrames", "record", 99, "starting frame|join exactly|interleaves")):
            with self.subTest(field=field):
                report = make_report()
                report["motionEvidence"][section][1][field] = value
                self.reject(report, fragment)
        report = make_report()
        report["startingFrameIdentity"]["submissionSerial"] = 12
        self.reject(report, "starting frame")

    def test_missing_duplicate_and_interleaving_presentations_are_ineligible(self):
        report = make_report()
        report["imagePresentationTiming"]["capture"]["rows"][1]["simulationTick"] = 9
        self.reject(report, "interleaves")
        report = make_report()
        report["imagePresentationTiming"]["capture"]["rows"][2]["presentID"] = 11
        self.reject(report, "duplicate presentation ID")
        report = make_report()
        report["motionEvidence"]["completedRtFrames"][1] = copy.deepcopy(report["motionEvidence"]["completedRtFrames"][0])
        self.reject(report, "duplicate completed")

    def test_counter_deltas_unresolved_and_prepared_tail_fail(self):
        for mutate, fragment in ((lambda w: w["counterDelta"].update(queryCalls=0), "inconsistent"),
                (lambda w: w["capture"]["status"].update(pendingCount=1), "unresolved"),
                (lambda w: w["capture"].update(unresolved=[{"presentID": 5}]), "unresolved"),
                (lambda w: (w["capture"]["counters"].update(queryErrors=1), w["counterDelta"].update(queryErrors=1)), "adverse"),
                (lambda w: (w["capture"]["counters"].update(preparedPresents=5), w["counterDelta"].update(preparedPresents=4)), "unregistered")):
            with self.subTest(fragment=fragment):
                report = make_report()
                mutate(report["imagePresentationTiming"])
                self.reject(report, fragment)

    def test_resource_discontinuity_retirement_and_swapped_output_fail(self):
        for mutate, fragment in ((lambda r: r["motionEvidence"]["resourceScopes"].append(dict(r["motionEvidence"]["resourceScopes"][0])), "one continuous"),
                (lambda r: r["motionEvidence"].update(retiredSubmissions=[{}]), "retired motion"),
                (lambda r: r["motionEvidence"]["completedRtFrames"][1].update(surfaceGeneration=2), "mixed resource"),
                (lambda r: r["imagePresentationTiming"]["capture"]["rows"][2].update(swapchainSerial=2), "mixed surface"),
                (lambda r: r["imagePresentationTiming"]["capture"]["rows"][2].update(width=2980, height=1440), "metadata differs")):
            report = make_report()
            mutate(report)
            self.reject(report, fragment)

    def test_native_eligibility_does_not_bypass_id_gaps_or_nonmonotonic_time(self):
        for mutate, fragment in ((lambda r: r["imagePresentationTiming"]["capture"]["rows"][2].update(presentID=99), "contain a gap"),
                (lambda r: r["imagePresentationTiming"]["capture"]["rows"][2].update(actualPresentTime=2_000_000_000), "strictly increasing")):
            report = make_report()
            mutate(report)
            self.reject(report, fragment)

    def test_scale_tuple_and_actual_upload_admission(self):
        for mutate, fragment in ((lambda r: r["settings"].update(scalePercent=75), "33, 40 or 50"),
                (lambda r: r["settings"].pop("internalWidth"), "internalWidth"),
                (lambda r: r["motionManifest"].update(dust=1), "manifest dust differs"),
                (lambda r: r["motionEvidence"]["completedRtFrames"][1].update(actualUploadedDustQuality=1), "uploaded dust"),
                (lambda r: r["motionEvidence"]["completedRtFrames"][1].update(actualUploadedMistEnabled=False), "uploaded mist")):
            report = make_report()
            mutate(report)
            self.reject(report, fragment)

    def test_incomplete_and_nonshared_simulation_cannot_pass(self):
        for mutate, fragment in ((lambda r: r.update(result="invalid"), "result complete"),
                (lambda r: r.update(simulationPolicy="fixed delta"), "simulationPolicy differs"),
                (lambda r: r["motionEvidence"].update(scenarioComplete=False), "incomplete or failed"),
                (lambda r: r["motionManifest"]["isolation"].update(fixedDeltaOverride=True), "isolation"),
                (lambda r: r["timingEligibility"].update(matchedCompletedFrames=2), "eligibility/counts")):
            report = make_report()
            mutate(report)
            self.reject(report, fragment)

    def test_state_time_axis_event_order_capacity_and_bool_identity_fail(self):
        for mutate, fragment in ((lambda r: r["motionEvidence"]["states"][1].update(wallNs=9), "regressed"),
                (lambda r: r["motionEvidence"]["states"][2].update(wallNs=130_000_000_000), "wall-time"),
                (lambda r: r["motionEvidence"]["states"][1].update(axes=[2, 0]), "bounds"),
                (lambda r: r["motionEvidence"]["events"].append({"sequence": 1, "observedStateRow": 1}), "event order"),
                (lambda r: r["motionEvidence"]["completedRtFrames"][0].update(submission=True), "integer"),
                (lambda r: r["limits"].update(stateRows=99999), "limit differs")):
            report = make_report()
            mutate(report)
            self.reject(report, fragment)

    def test_image_filename_extent_and_state_binding_are_exact(self):
        for mutate, fragment in ((lambda r: r["motionImageCaptures"][0].update(file="../other.rgba"), "filename"),
                (lambda r: r["motionImageCaptures"][1].update(stateRow=0), "RT/state binding"),
                (lambda r: r["motionImageCaptures"][1].update(bytes=4), "dimensions/bytes"),
                (lambda r: r["motionImageCaptures"].pop(), "image count")):
            report = make_report()
            mutate(report)
            self.reject(report, fragment)

    def test_startup_pending_row_after_baseline_is_excluded_without_counter_fiction(self):
        report = make_report()
        report["imagePresentationTiming"]["rowIndexBeforeRun"] = 0
        self.assertEqual(analyze_bytes(encoded(report))["scope"]["setupPresentationRowsExcluded"], 1)

    def test_parser_rejects_duplicate_keys_nonstandard_and_overflow_float(self):
        with self.assertRaisesRegex(AnalysisError, "duplicate JSON"):
            analyze_bytes(b'{"schema":1,"schema":1}')
        with self.assertRaisesRegex(AnalysisError, "nonstandard"):
            analyze_bytes(b'{"schema":NaN}')
        raw = encoded(make_report()).replace(b'"simulationSeconds":0.0', b'"simulationSeconds":1e999', 1)
        with self.assertRaisesRegex(AnalysisError, "finite"):
            analyze_bytes(raw)

    def test_cli_failure_is_nonzero_and_machine_readable(self):
        stderr = io.StringIO()
        with redirect_stderr(stderr):
            code = main(["absent-report.json"])
        self.assertEqual(code, 2)
        self.assertFalse(json.loads(stderr.getvalue())["eligible"])


if __name__ == "__main__":
    unittest.main()
