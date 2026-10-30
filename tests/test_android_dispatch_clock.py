from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from analyze_android_dispatch_clock import AnalysisError, CATEGORIES, admit_clock_rows, admit_raw


def fixture():
    row = dict(clock_status="valid", clock_ticket="7", clock_samples="2")
    for c in CATEGORIES:
        row["clock_"+c] = "0"
    row["clock_trace"] = str((1 << 32)+12)
    raw = struct.pack("<20I", 7,16,16,0, 0,0, 4,1, *([0]*12))
    raw += struct.pack("<20I", 7,48,16,1, 0,0, 8,0, *([0]*12))
    return row, raw


class ClockAdmissionTests(unittest.TestCase):
    def test_exact_raw_grid_owner_and_carries(self):
        row, raw = fixture()
        self.assertEqual(admit_clock_rows([row],64,32),2)
        self.assertEqual(admit_raw(raw,row,64,32)["samples"],2)

    def test_stale_missing_overflow_and_unavailable_never_zero(self):
        row, _ = fixture()
        for field, value in [("clock_status","unavailable-or-mismatched"),("clock_ticket","0"),
                             ("clock_samples","1"),("clock_trace",""),("clock_trace","0"),
                             ("clock_trace",str(1 << 64))]:
            changed = dict(row);changed[field] = value
            with self.subTest(field=field,value=value), self.assertRaises(AnalysisError):
                admit_clock_rows([changed],64,32)
        with self.assertRaises(AnalysisError): admit_clock_rows([row,row],64,32)
        with self.assertRaises(AnalysisError): admit_clock_rows([row],10000,10000)

    def test_raw_corruption_and_missing_samples_rejected(self):
        row, raw = fixture()
        for changed in [raw[:-1],raw+raw,bytes(80)+raw[80:],raw[:80]+raw[:80]]:
            with self.assertRaises(AnalysisError): admit_raw(changed,row,64,32)
        changed = dict(row);changed["clock_trace"]="1"
        with self.assertRaises(AnalysisError): admit_raw(raw,changed,64,32)
        corrupt = bytearray(raw);struct.pack_into("<I",corrupt,12,27)
        with self.assertRaises(AnalysisError): admit_raw(corrupt,row,64,32)


if __name__ == "__main__": unittest.main()
