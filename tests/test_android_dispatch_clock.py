from pathlib import Path
import csv
import json
import struct
import tempfile
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from analyze_android_dispatch_clock import (
    AnalysisError, CATEGORIES, HEARTH_SHADOW_CATEGORIES, admit_clock_rows, admit_raw, analyze_hearth,
)


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


    def test_hearth_witness_belongs_to_one_completed_room_frame(self):
        generic, raw = fixture()
        digest = "f" * 64
        ids = {"TavernKeeper": 11, "RecordKeeper": 12}
        row = dict(phase="room", returns=0, station=13, elapsed_ns=100, present_return_ns=1100,
                   scene_epoch=2, generation=3, record_serial=4, submission_serial=5,
                   completion_serial=5, present_serial=5, gpu_serial=5, gpu_stage_serial=5,
                   shader_sha256=digest, backend="RayTracingPipeline", tick=9300,
                   internal_width=64, internal_height=32, output_width=192, output_height=96,
                   kit_mask=1, resident_mask=1, record_keeper_mask=1, tavern_door_mask=1,
                   glass_enabled=0, window_mask=0, resident_slot0_id=11, resident_slot1_id=12,
                   fire_ids="1;2;3;4", gpu_ns=10000, gpu_dispatch_ns=9000)
        stem = "bellwether-westroundtrip-hearth-clock-1000-return-1-sample-1-5"
        receipt = dict(format="horde-rt-dispatch-clock-v1", route="west-roundtrip",
                       room_id="bellwether_tavern_staffed_hearth", phase="room", measurement_checkpoint_id=349,
                       checkpoint_id=4, return_index=1, sample_index=1, room_dwell_ticks=300,
                       record_size_bytes=80, record_count=2, clock_ticket=7, category_order=CATEGORIES,
                       category_ticks=[int(generic["clock_"+k]) for k in CATEGORIES], raw_file=stem+".bin",
                       scene_epoch=2, measurement_generation=3, record_serial=4, submission_serial=5,
                       completion_serial=5, presented_serial=5, simulation_tick=9300, internal_width=64,
                       internal_height=32, output_width=192, output_height=96, execution_mode=row["backend"],
                       pipeline_sha256=digest)
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            with (folder/"bellwether-west-roundtrip.csv").open("w", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=row)
                writer.writeheader()
                writer.writerow(row)
            (folder/(stem+".bin")).write_bytes(raw)
            sidecar = folder/(stem+".json")
            sidecar.write_text(json.dumps(receipt))
            def verify_room(frame):
                self.assertEqual(frame["fire_ids"], "1;2;3;4")
            result = analyze_hearth(folder, digest, verify_room, ids)
            self.assertFalse(result["completeSampleSet"])
            self.assertEqual(result["witnesses"][0]["samples"], 2)
            for key, value in (("completion_serial", 6), ("scene_epoch", 3), ("clock_ticket", 9),
                               ("record_count", 1), ("sample_index", 2), ("room_dwell_ticks", 299),
                               ("pipeline_sha256", "d"*64), ("raw_file", "../other.bin"),
                               ("category_order", tuple(reversed(HEARTH_SHADOW_CATEGORIES)))):
                sidecar.write_text(json.dumps(dict(receipt, **{key: value})))
                with self.subTest(key=key), self.assertRaises(AnalysisError):
                    analyze_hearth(folder, digest, verify_room, ids)

            split = bytearray(raw)
            struct.pack_into("<Q", split, 16 + 3*8, 7)
            struct.pack_into("<Q", split, 16 + 7*8, 11)
            totals = list(receipt["category_ticks"])
            totals[3], totals[7] = 7, 11
            sidecar.write_text(json.dumps(dict(receipt, category_order=HEARTH_SHADOW_CATEGORIES, category_ticks=totals)))
            sidecar.with_suffix(".bin").write_bytes(split)
            result = analyze_hearth(folder, digest, verify_room, ids)["witnesses"][0]
            self.assertEqual(result["sampledClockTicks"]["other_shadow"], 7)
            self.assertEqual(result["sampledClockTicksByPrimaryClass"][0]["direct_fire_shadow"], 11)
            self.assertEqual(result["sampledClockTicksByPrimaryClass"][1]["direct_fire_shadow"], 0)

            next_row = dict(row, elapsed_ns=200, present_return_ns=1200, submission_serial=6,
                            completion_serial=6, present_serial=6, gpu_serial=6, gpu_stage_serial=6, tick=9360)
            with (folder/"bellwether-west-roundtrip.csv").open("a", newline="") as stream:
                csv.DictWriter(stream, fieldnames=row).writerow(next_row)
            next_stem = "bellwether-westroundtrip-hearth-clock-1000-return-1-sample-2-6"
            next_receipt = dict(receipt, sample_index=2, room_dwell_ticks=360, simulation_tick=9360,
                                submission_serial=6, completion_serial=6, presented_serial=6,
                                clock_ticket=8, raw_file=next_stem+".bin")
            (folder/(next_stem+".json")).write_text(json.dumps(next_receipt))
            with self.assertRaisesRegex(AnalysisError, "Mixed runs.*clock categories"):
                analyze_hearth(folder, digest, verify_room, ids)


if __name__ == "__main__": unittest.main()
