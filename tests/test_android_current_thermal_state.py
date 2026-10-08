import copy
import importlib.util
from pathlib import Path
import sys
import unittest

path = Path(__file__).resolve().parents[1] / "tools/android_current_thermal_state.py"
spec = importlib.util.spec_from_file_location("android_current_thermal_state", path)
thermal = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = thermal
spec.loader.exec_module(thermal)

def dump(ap="30.0", battery="28.0", skin="31.0", status=0):
    return f"""Thermal Status: {status}
Cached temperatures:
  Temperature{{mValue=99.0, mType=0, mName=AP, mStatus=3}}
Current temperatures from HAL:
  Temperature{{mValue={ap}, mType=0, mName=AP, mStatus=0}}
  Temperature{{mValue={battery}, mType=2, mName=BAT, mStatus=0}}
  Temperature{{mValue={skin}, mType=3, mName=SKIN, mStatus=0}}
Current cooling devices from HAL:
  Temperature{{mValue=88.0, mType=0, mName=AP, mStatus=2}}
"""

class CurrentThermalStateTests(unittest.TestCase):
    def test_only_explicit_current_hal_section_is_used(self):
        state = thermal.parse_current_thermal_state(dump())
        self.assertEqual(30.0, state["sensors"]["AP"]["celsius"])
        self.assertEqual(28.0, state["sensors"]["BAT"]["celsius"])
        self.assertEqual("current HAL", state["temperatureSource"])

    def test_cached_and_missing_hal_data_never_establish_admission(self):
        for invalid in [dump().replace("Current temperatures from HAL:", "Cached temperatures again:"),
                        dump().replace("  Temperature{mValue=28.0, mType=2, mName=BAT, mStatus=0}\n", "")]:
            with self.assertRaises(thermal.ThermalEvidenceError):
                thermal.parse_current_thermal_state(invalid)

    def test_ambiguous_section_sensor_or_framework_status_is_rejected(self):
        for invalid in [dump().replace("Current temperatures from HAL:", "Current temperatures from HAL:\nCurrent temperatures from HAL:"),
                        dump().replace("Current cooling devices from HAL:", "  Temperature{mValue=31.0, mType=0, mName=AP, mStatus=0}\nCurrent cooling devices from HAL:"),
                        "Thermal Status: 1\n" + dump()]:
            with self.assertRaises(thermal.ThermalEvidenceError):
                thermal.parse_current_thermal_state(invalid)

    def test_invalid_or_wrong_type_sensors_are_rejected(self):
        for invalid in [dump(ap="NaN"), dump(ap="Infinity"), dump(ap="not a temperature"),
                        dump().replace("mType=2, mName=BAT", "mType=0, mName=BAT")]:
            with self.assertRaises(thermal.ThermalEvidenceError):
                thermal.parse_current_thermal_state(invalid)

    def test_framework_throttling_prevents_a_status_zero_start(self):
        state = thermal.parse_current_thermal_state(dump(status=1))
        self.assertFalse(thermal.admit_start(state)["admitted"])

    def test_reference_matches_all_three_typed_temperatures(self):
        before = thermal.parse_current_thermal_state(dump())
        at_limit = thermal.parse_current_thermal_state(dump(ap="31.5", battery="29.5", skin="32.5"))
        self.assertTrue(thermal.admit_start(at_limit, before, max_delta_c=1.5)["admitted"])
        for changed in [dump(ap="31.6"), dump(battery="29.6"), dump(skin="32.6")]:
            self.assertFalse(thermal.admit_start(thermal.parse_current_thermal_state(changed), before)["admitted"])

    def test_current_ceilings_block_a_warm_preview_start(self):
        state = thermal.parse_current_thermal_state(dump(ap="44.4", battery="37.6", skin="39.7", status=1))
        self.assertFalse(thermal.admit_start(state, maxima={"AP": 31.5, "BAT": 29.5, "SKIN": 32.5})["admitted"])

    def test_reference_type_roster_source_and_sensor_status_cannot_silently_change(self):
        state = thermal.parse_current_thermal_state(dump())
        for mutate in [lambda r: r.update(temperatureSource="cached"),
                       lambda r: r["sensors"].pop("SKIN"),
                       lambda r: r["sensors"]["AP"].update(type=1),
                       lambda r: r["sensors"]["AP"].update(celsius=float("nan")),
                       lambda r: r["sensors"]["AP"].update(celsius=True),
                       lambda r: r["sensors"]["AP"].update(status="0")]:
            reference = copy.deepcopy(state); mutate(reference)
            with self.assertRaises(thermal.ThermalEvidenceError):
                thermal.admit_start(state, reference)
        reference = copy.deepcopy(state); reference["sensors"]["SKIN"]["status"] = 1
        self.assertFalse(thermal.admit_start(state, reference)["admitted"])

    def test_size_and_invalid_admission_limits_are_bounded(self):
        with self.assertRaises(thermal.ThermalEvidenceError):
            thermal.parse_current_thermal_state("x" * (thermal.MAX_DUMP_BYTES + 1))
        state = thermal.parse_current_thermal_state(dump())
        for malformed in [[], {"schemaVersion": 1, "temperatureSource": "current HAL", "sensors": []}]:
            with self.assertRaises(thermal.ThermalEvidenceError):
                thermal.admit_start(state, malformed)
        for kwargs in [{"max_delta_c": -1}, {"max_delta_c": float("nan")}, {"required_status": 7},
                       {"maxima": {"AP": float("inf")}}, {"maxima": {"UNKNOWN": 30}}]:
            with self.assertRaises(thermal.ThermalEvidenceError):
                thermal.admit_start(state, **kwargs)

if __name__ == "__main__":
    unittest.main()
