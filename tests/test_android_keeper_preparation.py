"""Synthetic ADB response tests. No phone access or performance evidence."""
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from android_keeper_preparation import PreparationReadError, request_path, request_json_response, pid_response, arm_token, validate_arm_readback, validate_image_response

class PreparationTests(unittest.TestCase):
    def setUp(self):
        self.id = "keeper20r-a1"
        self.ready = {"schema": 1, "requestId": self.id, "phase": "prepared", "setupTicks": 430,
                      "twoUploadedLights": [3,4], "mist": True, "width": 576, "height": 1188}
    def parse(self, value, **overrides):
        args = dict(returncode=0, stdout=json.dumps(value).encode(), stderr=b"",
                    request_id=self.id, kind="ready", mist=True)
        args.update(overrides)
        return request_json_response(**args)
    def test_exact_missing_file_is_pending_on_either_stream(self):
        for kind in ("ready", "report"):
            msg = ("cat: " + request_path(self.id, kind) + ": No such file or directory\n").encode()
            for out, err in ((msg,b""),(b"",msg)):
                for rc in (0,1):
                    self.assertIsNone(request_json_response(rc,out,err,self.id,kind,True))
    def test_old_truthy_stdout_bug_cannot_be_a_report(self):
        msg = ("cat: " + request_path(self.id,"report") + ": No such file or directory").encode()
        self.assertTrue(msg)
        self.assertIsNone(request_json_response(1,msg,b"",self.id,"report",True))
        self.assertIsNone(request_json_response(0,msg,b"",self.id,"report",True))
    def test_disconnect_permission_and_wrong_missing_target_stop(self):
        for rc, out, err in ((1,b"",b"cannot connect to daemon"),(1,b"permission denied",b""),
                            (1,b"cat: other.json: No such file or directory",b""),
                            (0,b"",b""),(2,b"",b""),(1,b"",b"")):
            with self.subTest(rc=rc,out=out,err=err), self.assertRaises(PreparationReadError):
                request_json_response(rc,out,err,self.id,"ready",True)
    def test_exact_ready_and_terminal_are_separate(self):
        self.assertEqual(self.parse(self.ready),self.ready)
        value = {"schema":1,"requestId":self.id,"profile":"keeper-mist","result":"failed","error":"native failed"}
        self.assertEqual(self.parse(value,kind="report"),value)
        with self.assertRaises(PreparationReadError):
            self.parse(self.ready,kind="report")
    def test_stale_wrong_type_mist_light_and_extent_stop(self):
        for key,value in (("requestId","old"),("schema",True),("mist",1),("mist",False),
                          ("setupTicks",0),("twoUploadedLights",[3,True]),("width",True),("height",0)):
            with self.subTest(key=key,value=value), self.assertRaises(PreparationReadError):
                self.parse({**self.ready,key:value})
    def test_malformed_duplicate_empty_and_extra_bytes_stop(self):
        for raw in (b"{",b"",b"[]",b'{"schema":1,"schema":1}',json.dumps(self.ready).encode()+b"garbage",b"x"*4097):
            with self.subTest(raw=raw[:20]), self.assertRaises(PreparationReadError):
                self.parse(self.ready,stdout=raw)
        with self.assertRaises(PreparationReadError):
            self.parse({**self.ready,"unexpected":True})
        with self.assertRaises(PreparationReadError):
            self.parse(self.ready,stderr=b"warning")
    def test_owned_pid_or_exact_absence_only(self):
        self.assertEqual(pid_response(0,b"12345\n",b""),"12345")
        self.assertIsNone(pid_response(1,b"",b""))
        for rc,out,err in ((1,b"",b"daemon failed"),(0,b"123 456",b""),(0,b"",b""),(0,b"0",b"")):
            with self.subTest(rc=rc,out=out,err=err), self.assertRaises(PreparationReadError):
                pid_response(rc,out,err)
    def test_complete_terminal_mist_must_match_request(self):
        complete = {"schema":1,"requestId":self.id,"profile":"keeper-mist","result":"complete",
                    "mist":True,"graphics":{"mist":True}}
        self.assertEqual(self.parse(complete,kind="report"),complete)
        for value in ({**complete,"mist":False},{**complete,"graphics":{"mist":False}},
                      {**complete,"mist":1}):
            with self.assertRaises(PreparationReadError):
                self.parse(value,kind="report")
    def test_empty_stale_or_newline_changed_arm_is_never_published(self):
        token = arm_token(self.id)
        self.assertEqual(token,b"keeper20r-a1")
        validate_arm_readback(self.id,token)
        for raw in (b"",b"old",token+b"\\n",token+b"\\r\\n",b"cat: missing file"):
            with self.assertRaises(PreparationReadError):
                validate_arm_readback(self.id,raw)
    def test_image_requires_exact_identity_dimensions_and_bytes(self):
        report = {"requestId":self.id,"result":"complete","width":4,"height":2,
                  "image":{"file":"environment-survey-"+self.id+"-final.rgba","width":4,"height":2}}
        self.assertEqual(validate_image_response(report,self.id,0,b"x"*32,b""),b"x"*32)
        for rc,raw,err in ((0,b"cat: missing file",b""),(0,b"",b""),(0,b"x"*31,b""),
                           (1,b"x"*32,b""),(0,b"x"*32,b"error")):
            with self.assertRaises(PreparationReadError):
                validate_image_response(report,self.id,rc,raw,err)
        for image in ({**report["image"],"file":"other.rgba"},{**report["image"],"width":5}):
            with self.assertRaises(PreparationReadError):
                validate_image_response({**report,"image":image},self.id,0,b"x"*32,b"")

    def test_request_paths_are_closed(self):
        for identity in ("../x","", "x"*65,"a b","x;touch"):
            with self.assertRaises(PreparationReadError):
                request_path(identity,"ready")
        with self.assertRaises(PreparationReadError):
            request_path(self.id,"unknown")
if __name__ == "__main__":
    unittest.main()
