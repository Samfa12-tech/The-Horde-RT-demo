"""Synthetic contract tests only; no device or physical performance validation."""
import copy
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from analyze_keeper_mist_fixture import AnalysisError, LOSS_COUNTERS, STAGES, analyze, compare, load_json, main


def make_frame(serial, index, mist=True):
    stages = {name: 0.0 for name in STAGES}
    stages.update(status="valid", skinCpuMs=.6, playerSkinCpuMs=.4, characterSkinCpuMs=.2,
                  wholeFrameCycleCpuMs=1.5, workInvocationCounts=[0, 2, 1, 1] + [0] * 10,
                  byteCounts=[0] * 14, operationCounts=[0] * 14)
    empty = {"stableId": 0, "positionStrength": [0., 0., 0., 0.], "colourIntensity": [0., 0., 0., 0.]}
    return {
        "schema": 1,
        "identity": {"sceneEpoch": 2, "measurementGeneration": 3, "recordAttemptSerial": serial,
                     "recordSerial": serial, "submissionSerial": serial, "completionSerial": serial,
                     "simulationTick": 430, "frameSlot": 0},
        "pipeline": {"executionBackend": "RayTracingPipeline", "instrumentation": "shipping",
                     "dielectricQuality": "mobile", "bundleKey": "synthetic-pair",
                     "opaqueFast": {"key": "synthetic-opaque", "sha256": "a" * 64},
                     "genericDielectric": {"key": "synthetic-generic", "sha256": "b" * 64},
                     "activeStrategy": "generic-dielectric", "activeKey": "synthetic-generic",
                     "activeSha256": "b" * 64, "waterQuality": "mobile"},
        "dispatch": {"sceneReady": True, "rtDispatchRecorded": True, "swapchainCopyRecorded": True},
        "presentation": {"outcome": "presented", "presented": True,
                         "lastSuccessfulPresentSubmissionSerial": serial, "finalIdleCompletion": True},
        "gpu": {"status": "valid", "available": True, "completedSubmissionSerial": serial,
                "wholeRtGpuMs": 900.0 if index < 60 else float(index - 59),
                "timestampValidBits": 64, "timestampPeriodPicoseconds": 1000,
                "sampleCount": serial, "unavailableResultCount": 0, "errorCount": 0, "detail": ""},
        "dielectric": {"status": "compiled-out", "available": False, "compiled": False,
                       "completedSubmissionSerial": 0, "readCount": 0, "resetCount": 0, "counters": None, "detail": ""},
        "stages": stages,
        "fireLighting": {"count": 2, "emitters": [
            {"stableId": 3, "positionStrength": [-33., 1., -17., 1.], "colourIntensity": [1., .5, .1, 1.]},
            {"stableId": 4, "positionStrength": [-33., 1., -13., 1.], "colourIntensity": [1., .5, .1, 1.]},
            copy.deepcopy(empty), copy.deepcopy(empty)]},
        "fireQuality": {"quality": "Mobile", "volumeSteps": 4, "reflectionSamples": 1, "reflectedVolumeSteps": 4},
        "shadowQuality": {"mode": "current", "localPrimarySamples": 1, "skyPrimarySamples": 1, "reserved": 0},
        "resources": {"bufferCount": 20, "memoryAllocationCount": 20,
                      "bottomLevelAccelerationStructureCount": 4, "topLevelAccelerationStructureCount": 1,
                      "tlasInstanceCount": 12, "pipelineCount": 2, "shaderBindingTableCount": 2,
                      "descriptorSetCount": 1, "hostVisibleBytes": 1000, "deviceLocalBytes": 5000},
        "player": {"skinCadenceHz": 60, "skinUpdateCount": 1, "maximumSocketErrorMicrometres": 0,
                   "primaryPixelCountAvailable": False, "primaryPixelCount": None, "primaryVisible": False},
        "actualUploadedMistEnabled": mist, "actualUploadedDustQuality": 1,
        "cpuBenchmarkEligible": True, "benchmarkEligible": True,
    }


def make_report(mist=True, request="synthetic-on"):
    frames, rows = [], []
    # Extra setup/arming presents are allowed but cannot substitute for a retained identity.
    for index in range(-12, 180):
        serial = index + 100
        frame = make_frame(serial, index, mist)
        if index >= 0:
            frames.append({"index": index, "completedFrame": frame})
        rows.append({"surfaceGeneration": 4, "swapchainSerial": 1, "sceneEpoch": 2,
                     "measurementGeneration": 3, "recordSerial": serial, "submissionSerial": serial,
                     "simulationTick": 430, "presentID": serial,
                     "queuedSteadyNs": 1_000_000_000 + serial * 10_000_000 - 1_000_000,
                     "actualPresentTime": 1_000_000_000 + serial * 10_000_000,
                     "earliestPresentTime": 1_000_000_000 + serial * 10_000_000,
                     "presentMargin": 0, "scalePercent": 40, "width": 1440, "height": 2971,
                     "backend": "RayTracingPipeline"})
    counters = {key: 0 for key in LOSS_COUNTERS}
    counters.update(preparedPresents=len(rows), acceptedPresents=len(rows), queryCalls=200, incompleteQueries=0)
    return {"schema": 1, "requestId": request, "profile": "keeper-mist", "result": "complete", "error": None,
            "gpu": "Synthetic GPU", "driverVersion": 123, "surfaceGeneration": 4, "setupTicks": 430,
            "simulationTick": 430, "ownerSimulationBytesUnchanged": True, "retirementProven": True, "armed": True,
            "warmupFrames": 60, "sampleFrames": 120, "width": 576, "height": 1188,
            "surfaceWidth": 1440, "surfaceHeight": 2971, "mist": mist, "scale": 40,
            "graphics": {"scalePercent": 40, "water": 1, "fire": 0, "frameCap": 30,
                         "glass": False, "shadow": 1, "mist": mist, "dust": 1},
            "camera": [-31.2, -.95, -15.2, -1.57079637, 0.],
            "keeper": {"x": -34.7, "y": 0., "z": -15.2, "health": 3,
                       "readable": True, "roomMistDensity": 1., "staffLight": 0.},
            "image": {"file": "environment-survey-" + request + "-final.rgba", "width": 576, "height": 1188},
            "frames": frames,
            "presentation": {"extensionEnabled": True, "queryCpuWallNanoseconds": 100,
                             "capture": {"schemaVersion": 1,
                                         "status": {"enabled": True, "bound": True, "surfaceGeneration": 4,
                                                    "swapchainSerial": 1, "pendingCount": 0, "lastQueryResult": 0},
                                         "counters": counters, "rows": rows, "unresolved": []}}}


def thermals(offset=0.):
    return [{"phase": phase, "thermal": {"schemaVersion": 1, "temperatureSource": "current HAL", "frameworkStatus": 0,
             "sensors": {sensor: {"type": {"AP": 0, "BAT": 2, "SKIN": 3}[sensor], "status": 0, "celsius": temperature + offset}
                         for sensor, temperature in (("AP", 40.), ("BAT", 29.), ("SKIN", 33.))}}}
            for phase in ("before-run", "after-run")]


class KeeperMistAnalysisTests(unittest.TestCase):
    def setUp(self):
        self.report = make_report()

    def frame(self, report=None):
        return (self.report if report is None else report)["frames"][80]["completedFrame"]

    def reject(self, mutation, pattern=None):
        report = copy.deepcopy(self.report)
        mutation(report)
        with self.assertRaisesRegex(AnalysisError, pattern or ".+"):
            analyze(report)

    def test_accepts_exact_join_and_excludes_all_warmup_gpu_samples(self):
        result = analyze(self.report)
        self.assertEqual(result["joinedFrames"], 180)
        self.assertEqual(result["gpu"], {"n": 120, "medianMs": 60.5, "p95Ms": 114., "maximumMs": 120.})
        self.assertEqual(result["diagnosticDisplayIntervals"], {"n": 119, "medianMs": 10., "p95Ms": 10., "maximumMs": 10.})
        self.assertEqual(result["cpuStages"]["wholeFrameCycleCpuMs"]["medianMs"], 1.5)
        self.assertIn("not moving combat", result["limits"])

    def test_native_complete_flags_cannot_replace_actual_evidence(self):
        self.reject(lambda r: r.pop("presentation"))
        self.reject(lambda r: r["presentation"]["capture"].update(rows=[]))

    def test_missing_or_changed_each_six_part_actual_identity_rejected(self):
        for key in ("surfaceGeneration", "sceneEpoch", "measurementGeneration", "recordSerial", "submissionSerial", "simulationTick"):
            with self.subTest(key=key):
                self.reject(lambda r, key=key: r["presentation"]["capture"]["rows"][92].update({key: 99999}))

    def test_unretained_present_cannot_replace_missing_retained_same_tick(self):
        def replace(r):
            r["presentation"]["capture"]["rows"][92].update(recordSerial=99999, submissionSerial=99999)
        self.reject(replace, "identity join")

    def test_duplicate_actual_identity_and_present_id_rejected(self):
        def duplicate(r):
            capture = r["presentation"]["capture"]
            capture["rows"].append(copy.deepcopy(capture["rows"][92]))
            capture["counters"].update(preparedPresents=193, acceptedPresents=193)
        self.reject(duplicate, "duplicate actual")
        self.reject(lambda r: r["presentation"]["capture"]["rows"][92].update(presentID=88), "duplicate present")

    def test_zero_and_non_increasing_actual_timestamps_rejected(self):
        self.reject(lambda r: r["presentation"]["capture"]["rows"][92].update(actualPresentTime=0))
        self.reject(lambda r: r["presentation"]["capture"]["rows"][92].update(
            actualPresentTime=r["presentation"]["capture"]["rows"][91]["actualPresentTime"], queuedSteadyNs=1), "non-increasing")

    def test_every_collector_error_or_capacity_loss_rejected(self):
        for key in LOSS_COUNTERS:
            with self.subTest(counter=key):
                self.reject(lambda r, key=key: r["presentation"]["capture"]["counters"].update({key: 1}))

    def test_pending_unbound_and_unaccounted_rows_rejected(self):
        self.reject(lambda r: r["presentation"]["capture"]["status"].update(pendingCount=1))
        self.reject(lambda r: r["presentation"]["capture"]["status"].update(bound=False))
        self.reject(lambda r: r["presentation"]["capture"]["counters"].update(acceptedPresents=191))

    def test_terminal_failure_owner_change_and_wrong_frame_group_rejected(self):
        for key, value in (("result", "failed"), ("error", "cancelled"), ("armed", False),
                           ("ownerSimulationBytesUnchanged", False), ("retirementProven", False),
                           ("setupTicks", 0), ("warmupFrames", 59), ("sampleFrames", 121)):
            with self.subTest(key=key):
                self.reject(lambda r, key=key, value=value: r.update({key: value}))
        self.reject(lambda r: r["frames"].pop())
        self.reject(lambda r: r["frames"][80].update(index=79))

    def test_owning_gpu_present_and_real_rt_copy_required(self):
        for section, key, value in (("gpu", "completedSubmissionSerial", 1), ("gpu", "status", "unsupported"),
                                    ("gpu", "errorCount", 1), ("gpu", "wholeRtGpuMs", 0),
                                    ("gpu", "wholeRtGpuMs", float("nan")), ("gpu", "timestampValidBits", 0),
                                    ("presentation", "lastSuccessfulPresentSubmissionSerial", 1),
                                    ("presentation", "finalIdleCompletion", False), ("presentation", "outcome", "failed"),
                                    ("dispatch", "rtDispatchRecorded", False), ("dispatch", "swapchainCopyRecorded", False),
                                    ("pipeline", "executionBackend", "Raster"), ("pipeline", "activeSha256", "c" * 64)):
            with self.subTest(section=section, key=key, value=value):
                self.reject(lambda r, section=section, key=key, value=value: self.frame(r)[section].update({key: value}))

    def test_declared_shipping_mobile_pipeline_is_mandatory(self):
        for key, value in (("instrumentation", "diagnostic"), ("dielectricQuality", "high"),
                           ("executionBackend", "RayQueryCompute")):
            with self.subTest(key=key):
                self.reject(lambda r, key=key, value=value: self.frame(r)["pipeline"].update({key: value}), "declared")

    def test_nonmonotonic_serial_changed_tick_scope_and_multiple_frame_slots_rejected(self):
        for key, value in (("submissionSerial", 1), ("completionSerial", 1), ("simulationTick", 431),
                           ("measurementGeneration", 4), ("sceneEpoch", 3), ("frameSlot", 1)):
            with self.subTest(key=key):
                self.reject(lambda r, key=key, value=value: self.frame(r)["identity"].update({key: value}))

    def test_uploaded_lights_and_mist_must_be_exact_and_stable(self):
        self.reject(lambda r: self.frame(r)["fireLighting"].update(count=1))
        self.reject(lambda r: self.frame(r)["fireLighting"]["emitters"][1].update(stableId=3))
        self.reject(lambda r: self.frame(r)["fireLighting"]["emitters"][0]["positionStrength"].__setitem__(0, -32.))
        self.reject(lambda r: self.frame(r)["fireLighting"]["emitters"][2]["colourIntensity"].__setitem__(0, 1.))
        self.reject(lambda r: self.frame(r).update(actualUploadedMistEnabled=False))

    def test_cpu_stages_are_available_finite_and_canonical_count_compatible(self):
        self.reject(lambda r: self.frame(r)["stages"].pop("poseCopyCpuMs"))
        self.reject(lambda r: self.frame(r)["stages"].update(status="error"))
        self.reject(lambda r: self.frame(r)["stages"].update(skinCpuMs=.8))
        self.reject(lambda r: self.frame(r)["stages"].update(byteCounts=[0] * 13))
        self.reject(lambda r: self.frame(r)["stages"].update(poseCopyCpuMs=-1.))

    def test_fixed_scene_and_graphics_cannot_drift(self):
        self.reject(lambda r: self.frame(r)["resources"].update(tlasInstanceCount=13))
        self.reject(lambda r: self.frame(r)["fireQuality"].update(volumeSteps=2))
        self.reject(lambda r: r["graphics"].update(scalePercent=True))
        self.reject(lambda r: r["graphics"].pop("glass"))
        self.reject(lambda r: r["camera"].__setitem__(0, -30.))
        self.reject(lambda r: r["camera"].__setitem__(1, 0.))

    def test_pair_matches_all_fixed_workload_except_mist(self):
        off = make_report(False, "synthetic-off")
        pair = compare(off, self.report)
        self.assertTrue(pair["mistOn"]["workload"]["graphics"]["mist"])
        self.assertFalse(pair["thermalMatch"]["verified"])
        self.assertEqual(pair["gpuOnMinusOffMs"], {"medianMs": 0., "p95Ms": 0.})
        for mutate in (lambda r: r["graphics"].update(frameCap=31), lambda r: r.update(driverVersion=124),
                       lambda r: r["camera"].__setitem__(1, .01)):
            other = copy.deepcopy(off)
            mutate(other)
            with self.assertRaises(AnalysisError):
                compare(self.report, other)
        with self.assertRaises(AnalysisError):
            compare(self.report, make_report(True, "also-on"))

    def test_pair_light_position_or_shader_identity_difference_rejected(self):
        off = make_report(False, "synthetic-off")
        for row in off["frames"]:
            row["completedFrame"]["fireLighting"]["emitters"][0]["positionStrength"][0] = -32.
        with self.assertRaises(AnalysisError):
            compare(self.report, off)
        off = make_report(False, "synthetic-off")
        for row in off["frames"]:
            pipeline = row["completedFrame"]["pipeline"]
            pipeline["genericDielectric"]["sha256"] = pipeline["activeSha256"] = "c" * 64
        with self.assertRaises(AnalysisError):
            compare(self.report, off)

    def test_thermal_pair_start_and_end_thresholds_are_separate(self):
        off = make_report(False, "synthetic-off")
        self.assertTrue(compare(self.report, off, thermals(), thermals(1.5))["thermalMatch"]["verified"])
        for endpoint in (0, 1):
            other = thermals()
            other[endpoint]["thermal"]["sensors"]["AP"]["celsius"] += 1.5001
            with self.assertRaisesRegex(AnalysisError, "exceeds 1.5"):
                compare(self.report, off, thermals(), other)
        with self.assertRaisesRegex(AnalysisError, "both thermal"):
            compare(self.report, off, thermals(), None)

    def test_noncurrent_hal_missing_sensor_and_nonzero_framework_status_rejected(self):
        for mutate in (lambda t: t[0]["thermal"].update(temperatureSource="cached"),
                       lambda t: t[1]["thermal"].update(frameworkStatus=1),
                       lambda t: t[0]["thermal"]["sensors"].pop("SKIN"),
                       lambda t: t[1]["thermal"]["sensors"]["BAT"].update(status=1)):
            data = thermals()
            mutate(data)
            with self.assertRaises(AnalysisError):
                analyze(self.report, data)

    def test_json_duplicate_keys_nan_and_boolean_id_rejected(self):
        for raw in (b'{"schema":1,"schema":1}', b'{"x":NaN}'):
            with self.assertRaises(AnalysisError):
                load_json(raw)
        self.reject(lambda r: r.update(surfaceGeneration=True))

    def test_wrong_current_hal_sensor_types_rejected(self):
        receipt = thermals()
        receipt[0]["thermal"]["sensors"]["AP"]["type"] = 3
        with self.assertRaises(AnalysisError):
            analyze(self.report, receipt)

    def test_remote_missing_file_diagnostic_is_never_a_report(self):
        with self.assertRaises(AnalysisError):
            load_json(b"cat: /data/user/0/example/files/reports/fixture.json: No such file or directory\n")

    def test_cli_pair_output_and_failure_exit(self):
        # The Windows executor may redirect the OS temp directory outside its
        # writable roots. Generated files stay in this workspace and are removed
        # by TemporaryDirectory; no device report is used or overwritten.
        with tempfile.TemporaryDirectory(prefix="keeper-mist-analyzer-test-",
                                         dir=Path(__file__).resolve().parents[1]) as directory:
            directory = Path(directory)
            on, off, output = (directory / name for name in ("on.json", "off.json", "summary.json"))
            on.write_text(json.dumps(self.report), encoding="utf-8")
            off.write_text(json.dumps(make_report(False, "synthetic-off")), encoding="utf-8")
            self.assertEqual(main([str(on), "--compare", str(off), "--output", str(output)]), 0)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(len(result["inputSha256"]), 2)
            rejected = copy.deepcopy(self.report)
            rejected["frames"].pop()
            on.write_text(json.dumps(rejected), encoding="utf-8")
            with redirect_stderr(io.StringIO()) as errors, redirect_stdout(io.StringIO()) as stdout:
                self.assertEqual(main([str(on)]), 1)
            self.assertIn("rejected", errors.getvalue())
            self.assertEqual(stdout.getvalue(), "")


if __name__ == "__main__":
    unittest.main()
