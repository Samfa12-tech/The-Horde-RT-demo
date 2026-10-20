"""Offline admission of request-scoped ADB reads; no device operations."""
from __future__ import annotations
import re
from analyze_keeper_mist_fixture import load_json

class PreparationReadError(ValueError):
    pass

def request_path(request_id: str, kind: str) -> str:
    if not isinstance(request_id, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", request_id):
        raise PreparationReadError("invalid request identity")
    if kind not in ("ready", "report"):
        raise PreparationReadError("invalid read kind")
    return "files/reports/environment-survey-" + request_id + ("-ready.json" if kind == "ready" else ".json")

def request_json_response(returncode: int, stdout: bytes, stderr: bytes,
                          request_id: str, kind: str, mist: bool):
    """None only for the exact requested missing file; every other failure stops."""
    path = request_path(request_id, kind)
    if type(mist) is not bool or type(returncode) is not int:
        raise PreparationReadError("typed process/result values required")
    if not isinstance(stdout, bytes) or not isinstance(stderr, bytes):
        raise PreparationReadError("raw byte streams required")
    expected = ("cat: " + path + ": No such file or directory").encode()
    # exec-out can lose the remote cat exit code and return host exit 0.
    # Exact missing-path text is pending for either observed host code, never
    # JSON/evidence. Other payloads and transport failures are not suppressed.
    if returncode in (0, 1) and (stdout + stderr).strip() == expected:
        return None
    if returncode:
        raise PreparationReadError("ADB/remote read failed; not an absent report")
    if stderr or not stdout or len(stdout) > (4096 if kind == "ready" else 8 * 1024 * 1024):
        raise PreparationReadError("unexpected stderr, empty or oversized successful read")
    try:
        report = load_json(stdout)
    except (ValueError, TypeError) as error:
        raise PreparationReadError("malformed existing JSON report") from error
    if not isinstance(report, dict) or type(report.get("schema")) is not int or report["schema"] != 1 or report.get("requestId") != request_id:
        raise PreparationReadError("JSON schema/request ownership mismatch")
    if kind == "ready":
        if set(report) != {"schema", "requestId", "phase", "setupTicks", "twoUploadedLights", "mist", "width", "height"}:
            raise PreparationReadError("unexpected preparation field roster")
        if report["phase"] != "prepared" or type(report["setupTicks"]) is not int or report["setupTicks"] != 430:
            raise PreparationReadError("ordinary setup not proved")
        if report["twoUploadedLights"] != [3, 4] or any(type(x) is not int for x in report["twoUploadedLights"]):
            raise PreparationReadError("production two-light preparation missing")
        if type(report["mist"]) is not bool or report["mist"] != mist:
            raise PreparationReadError("prepared Mist mismatch")
        if any(type(report[key]) is not int or not 1 <= report[key] <= 16384 for key in ("width", "height")):
            raise PreparationReadError("invalid prepared extent")
    elif report.get("profile") != "keeper-mist" or report.get("result") not in ("failed", "complete"):
        raise PreparationReadError("unrecognized terminal report")
    if kind == "report" and report["result"] == "complete":
        if type(report.get("mist")) is not bool or report["mist"] != mist:
            raise PreparationReadError("terminal requested Mist mismatch")
        graphics = report.get("graphics")
        if not isinstance(graphics, dict) or type(graphics.get("mist")) is not bool or graphics["mist"] != mist:
            raise PreparationReadError("terminal graphics Mist mismatch")
    # Terminal parsing is not evidence acceptance: analyze_keeper_mist_fixture
    # separately validates every owning GPU and actual-presentation identity.
    return report

def pid_response(returncode: int, stdout: bytes, stderr: bytes):
    if returncode == 1 and not stdout.strip() and not stderr.strip():
        return None
    if returncode != 0 or stderr:
        raise PreparationReadError("PID read failed; process absence is unproved")
    text = stdout.decode("ascii", errors="strict").strip()
    if not re.fullmatch(r"[1-9][0-9]*", text):
        raise PreparationReadError("expected exactly one PID")
    return text

def arm_token(request_id: str) -> bytes:
    request_path(request_id, "report")
    return request_id.encode("ascii")

def validate_arm_readback(request_id: str, raw: bytes) -> None:
    if raw != arm_token(request_id):
        raise PreparationReadError("arm token transport not byte exact; do not publish")

def validate_image_response(report: dict, request_id: str, returncode: int,
                            stdout: bytes, stderr: bytes) -> bytes:
    request_path(request_id, "report")
    expected = "environment-survey-" + request_id + "-final.rgba"
    image = report.get("image")
    if report.get("requestId") != request_id or report.get("result") != "complete" or not isinstance(image, dict):
        raise PreparationReadError("completed request-scoped image required")
    if image.get("file") != expected:
        raise PreparationReadError("image request identity mismatch")
    for key in ("width", "height"):
        if type(image.get(key)) is not int or not 1 <= image[key] <= 4096 or image[key] != report.get(key):
            raise PreparationReadError("image extent mismatch")
    if returncode != 0 or stderr or not isinstance(stdout, bytes) or len(stdout) != image["width"] * image["height"] * 4:
        raise PreparationReadError("image read must contain the exact RGBA payload")
    return stdout
