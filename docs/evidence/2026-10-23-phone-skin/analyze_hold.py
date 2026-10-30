#!/usr/bin/env python3
"""Offline, private20261022 outdoor-hold analyzer.

The strict presentation validator below is copied from the repository tool at
this checkpoint, scoped only to chapter-outdoor-scale-hold and exact20/33/40.
Existing tools/production sources are unmodified. Raw evidence is never rewritten.
It reports durations rather than reciprocal timing or frame-cycle FPS.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import csv
import io
import statistics
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools"))
from typing import Any

from analyze_android_present_timing import (
    AnalysisError, MAX_REPORT_BYTES, MAX_TIMING_ROWS, MAX_PENDING_ROWS,
    _object, _reject_constant, _mapping, _array, _integer, _text,
    _percentile_nearest_rank,
)

U64_MAX = (1 << 64) - 1
SCENARIOS = CHAPTER_SCENARIOS = frozenset(("chapter-outdoor-scale-hold",))

POLICY = ("shared GameSimulation.AdvanceFrame with raw monotonic input-owner delta and normal "
          "fixed-step catch-up; generated motion axes/edges enter through the existing "
          "InputMailbox owner; no per-render fixed-delta override")
ADVERSE_COUNTERS = ("rejectedPresents", "invalidRegistrations", "invalidMetadata",
    "pendingCapacityExhausted", "queryErrors", "zeroPresentTimestamps", "zeroPresentIds",
    "unknownPresentIds", "duplicateTimings", "rowCapacityExhausted", "missingOnRebind",
    "missingOnUnbind", "abandonedPreparedPresents")
COUNTERS = ("preparedPresents", "acceptedPresents", "queryCalls", "incompleteQueries") + ADVERSE_COUNTERS
LIMITS = {"wallSeconds": 120, "presentationDrainMilliseconds": 2000, "stateRows": 16384, "eventRows": 1024, "rtRows": 16384,
          "captures": 64, "presentationRows": MAX_TIMING_ROWS, "presentationPending": MAX_PENDING_ROWS}


def _u64(value: Any, label: str, minimum: int = 0) -> int:
    return _integer(value, label, minimum, U64_MAX)


def _number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise AnalysisError(f"{label} must be a finite number")
    return float(value)


def _scope(row: dict[str, Any], label: str) -> tuple[int, int, int]:
    return tuple(_u64(row.get(field), f"{label}.{field}", 1) for field in
                 ("surfaceGeneration", "sceneEpoch", "measurementGeneration"))


def _identity(row: dict[str, Any], label: str, ledger: bool = False) -> tuple[int, ...]:
    return _scope(row, label) + tuple(_u64(row.get(field), f"{label}.{field}", minimum)
        for field, minimum in (("record" if ledger else "recordSerial", 1),
                               ("submission" if ledger else "submissionSerial", 1),
                               ("tick" if ledger else "simulationTick", 0)))


def validate_presentation_bytes(raw: bytes) -> dict[str, Any]:
    if not raw or len(raw) > MAX_REPORT_BYTES:
        raise AnalysisError("motion report is empty or exceeds the 64 MiB limit")
    try:
        report = _mapping(json.loads(raw, object_pairs_hook=_object,
                                    parse_constant=_reject_constant), "report")
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AnalysisError(f"invalid JSON report: {exc}") from exc
    if _integer(report.get("schema"), "schema") != 1 or report.get("result") != "complete":
        raise AnalysisError("motion report requires schema 1 and result complete")
    run_id = _text(report.get("runId"), "runId")
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", run_id):
        raise AnalysisError("unsafe motion runId")
    scenario = _text(report.get("scenario"), "scenario")
    if scenario not in SCENARIOS or report.get("workload") != f"motion-{scenario}-v1":
        raise AnalysisError("unknown or mismatched motion scenario/workload")
    build_id = _text(report.get("buildId"), "buildId")
    if report.get("simulationPolicy") != POLICY:
        raise AnalysisError("motion simulationPolicy differs from the admitted shared-clock path")
    limits = _mapping(report.get("limits"), "limits")
    for field, expected in LIMITS.items():
        if _integer(limits.get(field), f"limits.{field}") != expected:
            raise AnalysisError(f"motion limit differs in {field}")
    settings = _mapping(report.get("settings"), "settings")
    scale = _integer(settings.get("scalePercent"), "settings.scalePercent")
    if scale not in (20, 33, 40):
        raise AnalysisError("outdoor hold scale must be exactly20,33,40")
    width = _integer(settings.get("width"), "settings.width", 1, 16384)
    height = _integer(settings.get("height"), "settings.height", 1, 16384)
    internal_width = _integer(settings.get("internalWidth"), "settings.internalWidth", 1, width)
    internal_height = _integer(settings.get("internalHeight"), "settings.internalHeight", 1, height)
    backend = _text(settings.get("backend"), "settings.backend")
    if backend not in ("RayTracingPipeline", "RayQueryCompute"):
        raise AnalysisError("motion settings require a real admitted RT backend")
    for field, maximum in (("water", 2), ("fire", 2), ("shadow", 2), ("dust", 2)):
        _integer(settings.get(field), f"settings.{field}", 0, maximum)
    _integer(settings.get("cap"), "settings.cap", 15, 60)
    for field in ("glass", "mist"):
        if not isinstance(settings.get(field), bool):
            raise AnalysisError(f"settings.{field} must be a boolean")
    manifest = _mapping(report.get("motionManifest"), "motionManifest")
    evidence = _mapping(report.get("motionEvidence"), "motionEvidence")
    for label, obj in (("manifest", manifest), ("evidence", evidence)):
        if _integer(obj.get("schema"), f"{label}.schema") != 1 or obj.get("runId") != run_id or obj.get("scenario") != scenario:
            raise AnalysisError(f"{label} identity differs from the motion report")
    if any(manifest.get(field) is not True for field in ("finished", "complete", "armed")):
        raise AnalysisError("motion manifest is not finished, complete and armed")
    if evidence.get("scenarioComplete") is not True or evidence.get("failure") != "":
        raise AnalysisError("motion ledger is incomplete or failed")
    isolation = _mapping(manifest.get("isolation"), "manifest.isolation")
    for field in ("secondSimulation", "fixedDeltaOverride", "phaseForced", "preferencesWritten", "ownerAcceptance"):
        if isolation.get(field) is not False:
            raise AnalysisError(f"motion isolation lacks false {field}")
    if _integer(manifest.get("scale"), "manifest.scale") != scale:
        raise AnalysisError("manifest scale differs from actual settings")
    for field in ("water", "fire", "cap", "glass", "shadow", "mist", "dust"):
        if type(manifest.get(field)) is not type(settings[field]) or manifest[field] != settings[field]:
            raise AnalysisError(f"manifest {field} differs from actual settings")
    _integer(manifest.get("captures"), "manifest.captures", 0 if scenario in CHAPTER_SCENARIOS else 1, 64)
    if scenario in CHAPTER_SCENARIOS and manifest["captures"] != 0:
        raise AnalysisError("chapter pacing interval must not contain image readbacks")
    if _integer(evidence.get("pendingSubmissionCount"), "ledger.pendingSubmissionCount") != 0 or evidence.get("currentScopePresented") is not True:
        raise AnalysisError("motion ledger has pending work or no current-scope presentation")
    if _array(evidence.get("retiredSubmissions"), "retiredSubmissions", 16384):
        raise AnalysisError("retired motion submissions cannot support continuous displayed-image intervals")
    scopes = _array(evidence.get("resourceScopes"), "resourceScopes", 64)
    if len(scopes) != 1:
        raise AnalysisError("interval summary requires one continuous motion resource scope")
    scope = _scope(_mapping(scopes[0], "scope"), "scope")
    if _scope(manifest, "manifest") != scope or _integer(scopes[0].get("nextStateRow"), "scope.nextStateRow") != 0:
        raise AnalysisError("manifest/resource scope mismatch")
    states = _array(evidence.get("states"), "states", 16384)
    if not states:
        raise AnalysisError("motion ledger has no state rows")
    previous_wall = previous_tick = previous_publication = previous_overrun = 0
    for index, raw_state in enumerate(states):
        state = _mapping(raw_state, f"state {index}")
        if _integer(state.get("row"), "state.row") != index:
            raise AnalysisError("motion state row indices are discontinuous")
        wall = _u64(state.get("wallNs"), "state.wallNs", 1)
        tick = _u64(state.get("tick"), "state.tick")
        publication = _u64(state.get("publication"), "state.publication")
        overrun = _u64(state.get("overruns"), "state.overruns")
        _u64(state.get("ticksThisFrame"), "state.ticksThisFrame")
        if wall < previous_wall or tick < previous_tick or publication < previous_publication or overrun < previous_overrun:
            raise AnalysisError("motion wall/tick/publication/overrun state regressed")
        if _number(state.get("simulationSeconds"), "state.simulationSeconds") < 0:
            raise AnalysisError("negative motion simulation time")
        axes = _array(state.get("axes"), "state.axes", 2)
        if len(axes) != 2 or any(abs(_number(axis, "axis")) > 1.00001 for axis in axes):
            raise AnalysisError("motion state axes are outside admitted bounds")
        previous_wall, previous_tick, previous_publication, previous_overrun = wall, tick, publication, overrun
    if states[-1]["wallNs"] - states[0]["wallNs"] > 120_000_000_000:
        raise AnalysisError("motion state span exceeds the wall-time limit")
    events = _array(evidence.get("events"), "events", 1024)
    last_event = 0
    for index, raw_event in enumerate(events):
        event = _mapping(raw_event, f"event {index}")
        seq = _u64(event.get("sequence"), "event.sequence", 1)
        _integer(event.get("observedStateRow"), "event.observedStateRow", 0, len(states) - 1)
        if seq <= last_event:
            raise AnalysisError("motion semantic event order is not strictly increasing")
        last_event = seq
    frames = _array(evidence.get("completedRtFrames"), "completedRtFrames", 16384)
    if len(frames) < 2:
        raise AnalysisError("motion interval summary requires at least two completed RT frames")
    selected: dict[tuple[int, ...], dict[str, Any]] = {}
    for index, raw_frame in enumerate(frames):
        frame = _mapping(raw_frame, f"frame {index}")
        key = _identity(frame, f"frame {index}", True)
        if key[:3] != scope:
            raise AnalysisError("completed motion frame has a mixed resource scope")
        if key in selected:
            raise AnalysisError("duplicate completed motion frame identity")
        state_index = _integer(frame.get("stateRow"), "frame.stateRow", 0, len(states) - 1)
        if key[-1] != states[state_index]["tick"]:
            raise AnalysisError("completed motion frame tick differs from its bound state")
        _u64(frame.get("recordAttempt"), "frame.recordAttempt", 1)
        _u64(frame.get("completion"), "frame.completion", 1)
        if "actualUploadedDustQuality" in frame and frame["actualUploadedDustQuality"] != settings["dust"]:
            raise AnalysisError("actual uploaded dust differs from settings")
        if "actualUploadedMistEnabled" in frame and frame["actualUploadedMistEnabled"] is not settings["mist"]:
            raise AnalysisError("actual uploaded mist differs from settings")
        selected[key] = frame
    images = _array(report.get("motionImageCaptures"), "motionImageCaptures", 64)
    if len(images) != manifest["captures"]:
        raise AnalysisError("motion image count differs from the completed manifest")
    for index, raw_image in enumerate(images):
        image = _mapping(raw_image, f"motion image {index}")
        if image.get("file") != f"android-motion-{run_id}-{index}.rgba":
            raise AnalysisError("motion image filename is not its exact owned run/index")
        rt_index = _integer(image.get("rtRow"), "image.rtRow", 0, len(frames) - 1)
        if _integer(image.get("stateRow"), "image.stateRow", 0, len(states) - 1) != frames[rt_index]["stateRow"]:
            raise AnalysisError("motion image lacks its exact RT/state binding")
        if _integer(image.get("width"), "image.width", 1) != internal_width or \
           _integer(image.get("height"), "image.height", 1) != internal_height or \
           _integer(image.get("bytes"), "image.bytes", 1) != internal_width * internal_height * 4:
            raise AnalysisError("motion image dimensions/bytes differ from the traced resource extent")
    first_key = min(selected, key=lambda key: key[4])
    if _identity(_mapping(report.get("startingFrameIdentity"), "startingFrameIdentity"), "startingFrameIdentity") != first_key:
        raise AnalysisError("starting frame identity differs from the first completed motion frame")
    eligibility = _mapping(report.get("timingEligibility"), "timingEligibility")
    if eligibility.get("eligible") is not True or any(_integer(eligibility.get(field), field) != len(frames) for field in
                                                     ("matchedCompletedFrames", "expectedCompletedFrames")):
        raise AnalysisError("native timing eligibility/counts are incomplete")
    wrapper = _mapping(report.get("imagePresentationTiming"), "imagePresentationTiming")
    capture = _mapping(wrapper.get("capture"), "capture")
    status = _mapping(capture.get("status"), "capture.status")
    if wrapper.get("extensionEnabled") is not True or status.get("enabled") is not True or status.get("bound") is not True:
        raise AnalysisError("actual image presentation collector is unavailable or unbound")
    if _integer(capture.get("schemaVersion"), "capture.schemaVersion") != 1:
        raise AnalysisError("unsupported presentation collector schema")
    if _integer(status.get("pendingCount"), "capture.status.pendingCount") or _array(capture.get("unresolved"), "unresolved", 256):
        raise AnalysisError("presentation collector has unresolved IDs")
    counters = _mapping(capture.get("counters"), "capture.counters")
    baseline = _mapping(wrapper.get("counterBaseline"), "counterBaseline")
    delta = _mapping(wrapper.get("counterDelta"), "counterDelta")
    for field in COUNTERS:
        total, before, change = (_u64(obj.get(field), f"{label}.{field}") for obj, label in
                                 ((counters, "counters"), (baseline, "baseline"), (delta, "delta")))
        if before > total or total - before != change:
            raise AnalysisError(f"presentation counter delta is inconsistent in {field}")
        if field in ADVERSE_COUNTERS and change:
            raise AnalysisError(f"adverse presentation run counter {field}")
    timings = _array(capture.get("rows"), "capture.rows", MAX_TIMING_ROWS)
    row_baseline = _integer(wrapper.get("rowIndexBeforeRun"), "rowIndexBeforeRun", 0, len(timings))
    if counters["acceptedPresents"] != len(timings) or row_baseline > baseline["acceptedPresents"]:
        raise AnalysisError("presentation row/counter baseline mismatch")
    if counters["preparedPresents"] != counters["acceptedPresents"]:
        raise AnalysisError("presentation collector has an unregistered prepared image")
    matched: dict[tuple[int, ...], dict[str, Any]] = {}
    seen_ids: set[int] = set()
    for index, raw_timing in enumerate(timings):
        timing = _mapping(raw_timing, f"timing {index}")
        key = _identity(timing, f"timing {index}")
        present_id = _integer(timing.get("presentID"), "presentID", 1, (1 << 32) - 1)
        if present_id in seen_ids:
            raise AnalysisError("duplicate presentation ID")
        seen_ids.add(present_id)
        if key not in selected:
            if key[4] >= first_key[4]:
                raise AnalysisError("unmatched presentation interleaves or follows the motion span")
            continue  # Startup owns earlier submissions; it is not a motion interval.
        if index < row_baseline or key in matched:
            raise AnalysisError("duplicate or pre-run motion timing identity")
        if _integer(timing.get("scalePercent"), "timing.scalePercent") != scale or timing.get("backend") != backend or \
           _integer(timing.get("width"), "timing.width", 1) != width or _integer(timing.get("height"), "timing.height", 1) != height:
            raise AnalysisError("motion timing metadata differs from settings")
        if key[0] != _u64(status.get("surfaceGeneration"), "status.surfaceGeneration", 1) or \
           _u64(timing.get("swapchainSerial"), "timing.swapchainSerial", 1) != _u64(status.get("swapchainSerial"), "status.swapchainSerial", 1):
            raise AnalysisError("motion timing collector has mixed surface/swapchain ownership")
        _u64(timing.get("actualPresentTime"), "actualPresentTime", 1)
        _u64(timing.get("earliestPresentTime"), "earliestPresentTime")
        _u64(timing.get("presentMargin"), "presentMargin")
        matched[key] = timing
    if matched.keys() != selected.keys():
        raise AnalysisError("completed motion frames and presentation identities do not join exactly")
    ordered = sorted(matched.items(), key=lambda item: item[0][4])
    for (previous_key, previous), (key, current) in zip(ordered, ordered[1:]):
        if key[3] != previous_key[3] + 1 or key[4] != previous_key[4] + 1 or current["presentID"] != previous["presentID"] + 1:
            raise AnalysisError("motion record/submission/presentation IDs contain a gap")
        if current["actualPresentTime"] <= previous["actualPresentTime"]:
            raise AnalysisError("actual presentation timestamps are not strictly increasing")
    times = [timing["actualPresentTime"] for _, timing in ordered]
    intervals = [b - a for a, b in zip(times, times[1:])]
    sorted_intervals = sorted(intervals)
    median = (sorted_intervals[(len(intervals) - 1) // 2] + sorted_intervals[len(intervals) // 2]) // 2
    duration = times[-1] - times[0]
    return {"eligible": True, "reportSha256": hashlib.sha256(raw).hexdigest(),
        "runId": run_id, "workload": report["workload"], "buildId": build_id, "settings": settings,
        "scope": {"surfaceGeneration": scope[0], "sceneEpoch": scope[1], "measurementGeneration": scope[2],
            "matchedCompletedFrames": len(frames), "intervalCount": len(intervals),
            "durationNanoseconds": duration, "setupPresentationRowsExcluded": len(timings) - len(frames)},
        "intervalsNanoseconds": {"median": median,
            "p90NearestRank": _percentile_nearest_rank(sorted_intervals, .90),
            "p95NearestRank": _percentile_nearest_rank(sorted_intervals, .95), "maximum": max(intervals),
            "over33333000Nanoseconds": sum(value > 33_333_000 for value in intervals),
            "over250000000Nanoseconds": sum(value > 250_000_000 for value in intervals)},

        "simulation": {"stateRows": len(states), "eventRows": len(events),
            "ticksThisFrameMaximum": max(state["ticksThisFrame"] for state in states),
            "catchUpOverrunCountFirst": states[0]["overruns"], "catchUpOverrunCountLast": states[-1]["overruns"],
            "catchUpOverrunDelta": states[-1]["overruns"] - states[0]["overruns"]},
        "collector": {"runCounterDelta": delta,
            "queryCpuWallNanoseconds": _u64(wrapper.get("queryCpuWallNanoseconds"), "queryCpuWallNanoseconds"),
            "presentMarginAtLeast2Pow63CountIgnored": sum(timing["presentMargin"] >= (1 << 63) for _, timing in ordered)},
        "limits": [("Bounded stationary staged outdoor hold with zero interval image readbacks; recorder overhead remains." if scenario in CHAPTER_SCENARIOS else "Short harness-generated moving scenario with milestone RT readbacks; total instrumentation affects pacing."),
                   "One continuous resource scope; retry/rotation/lifecycle discontinuities require separate evidence.",
                   "Actual inter-present intervals; not sustained FPS, scanout, photons, GPU reciprocal time, touch latency or owner acceptance.",
                   "Trace extents are native resource observations; this analyzer does not certify appearance, thermals or power."]}



CPU_STAGES=('simulation','skin','player_skin','character_skin','pose_copy','dynamic_upload','fence_wait','queue_submit','whole_frame')
DEFAULT_EXPECTED={'water':1,'fire':0,'cap':30,'glass':False,'shadow':1,'mist':False,'dust':1}
RESOURCE_FIELDS=('device_local_bytes','host_visible_bytes','omitted_triangles','unloads','restores','saved_owner_bytes','peak_overlap_bytes','wait_frames','run_unloads','run_restores','tlas_instances','blas_count')


def durations(values):
    if not values:return None
    values=sorted(values)
    return {'samples':len(values),'minimumMs':values[0]/1e6,'medianMs':statistics.median(values)/1e6,
            'meanMs':statistics.mean(values)/1e6,'p95NearestRankMs':values[math.ceil(.95*len(values))-1]/1e6,
            'maximumMs':values[-1]/1e6}


def span(values):
    return {'minimum':min(values),'median':statistics.median(values),'maximum':max(values)}


def analyze(raw, resources, expected_scale, expected_settings=None):
    strict=validate_presentation_bytes(raw)
    # Parsing again retains the validated raw numeric facts; no scale/schema adapter.
    report=json.loads(raw,object_pairs_hook=_object,parse_constant=_reject_constant)
    settings=report['settings'];hold=_mapping(report.get('outdoorHold'),'outdoorHold')
    if hold.get('enabled') is not True or hold.get('normalFixedStep') is not True or hold.get('warmupWallSeconds')!=10 or hold.get('sampleWallSeconds')!=30:
        raise AnalysisError('exact live40s hold /10s warm /30s sample declaration is absent')
    start=_u64(hold.get('startWallNanoseconds'),'hold.startWallNanoseconds',1)
    warm=start+10_000_000_000;end=start+40_000_000_000
    if settings['scalePercent']!=expected_scale:raise AnalysisError('actual scale differs from the requested comparison scale')
    expected=DEFAULT_EXPECTED if expected_settings is None else expected_settings
    if set(expected)!=set(DEFAULT_EXPECTED):raise AnalysisError('expected settings must explicitly contain the entire non-scale tuple')
    for key,value in expected.items():
        if type(settings.get(key)) is not type(value) or settings[key]!=value:
            raise AnalysisError('actual/requested non-scale tuple differs in '+key)
    for dimension,internal in (('width','internalWidth'),('height','internalHeight')):
        if settings[internal]!=(settings[dimension]*expected_scale+50)//100:
            raise AnalysisError('internal dimension is not exact fixed scale: '+internal)
    for key in ('nativeWindowWidth','nativeWindowHeight','surfaceCurrentWidth','surfaceCurrentHeight'):
        _integer(settings.get(key),'settings.'+key,1,16384)
    transform=_integer(settings.get('rtPresentationTransform'),'settings.rtPresentationTransform',0,7)
    current=_integer(settings.get('surfaceCurrentTransform'),'settings.surfaceCurrentTransform',1,128)
    pre=_integer(settings.get('swapchainPreTransform'),'settings.swapchainPreTransform',1,128)
    if current!=pre or pre!=1<<transform:raise AnalysisError('current/pre/RT presentation transforms disagree')
    if settings.get('rtPreRotationRequired') is not (transform!=0):raise AnalysisError('pre-rotation declaration differs from transform')
    swap=transform in (1,3,5,7)
    logical=(settings['height'],settings['width']) if swap else (settings['width'],settings['height'])
    if logical!=(settings['nativeWindowWidth'],settings['nativeWindowHeight']):raise AnalysisError('logical view does not match the native window extent')
    evidence=report['motionEvidence'];states=evidence['states'];frames=evidence['completedRtFrames']
    if report['motionManifest'].get('compiledQuality')!='Mobile':raise AnalysisError('compiled quality is not Mobile')
    pipelines=_array(evidence.get('pipelines'),'pipelines',8)
    if not pipelines:raise AnalysisError('compiled pipeline identities absent')
    for pipeline in pipelines:
        if not pipeline.get('activeKey','').startswith('shipping_mobile_'):
            raise AnalysisError('active pipeline is not an exact Shipping/Mobile artifact')
        for key in ('activeSha256','opaqueSha256','genericSha256'):
            if not re.fullmatch('[0-9a-f]{64}',pipeline.get(key,'')):raise AnalysisError('invalid pipeline artifact hash '+key)
    if states[0]['wallNs']<start or states[-1]['wallNs']<end or states[-1]['wallNs']-start>120_000_000_000:
        raise AnalysisError('states do not cover the declared complete bounded40s hold')
    if states[-1]['tick']<=states[0]['tick']:raise AnalysisError('frozen simulation cannot support the live hold')
    sample_state=[s for s in states if warm<=s['wallNs']<=end]
    if not sample_state:raise AnalysisError('no post-warm simulation state')
    camera=[s.get('player') for s in states]
    if any(not isinstance(p,list) or len(p)!=6 or any(not math.isfinite(x) for x in p) for p in camera):raise AnalysisError('invalid player state array')
    for k,name in enumerate(('x','z','yaw','pitch')):
        if max(p[k] for p in camera)-min(p[k] for p in camera)>.0011:raise AnalysisError('stationary hold changed '+name)
    if math.hypot(camera[0][0]-13.3,camera[0][1]-36.2)>.04 or abs(camera[0][2]-2.51)>.001 or abs(camera[0][3]+.15)>.001:
        raise AnalysisError('actual hold framing differs from checkpoint203 lookout')
    if any(s.get('axes')!=[0,0] or s.get('damageEnabled') is not False or s.get('playerAlive') is not True or s.get('playerAction')!=0 for s in states):
        raise AnalysisError('hold contains movement axes, damage admission, death or a player action')
    if any(s.get('paused') for s in sample_state):raise AnalysisError('post-warm hold contains paused simulation states')
    if any(e.get('type') in (1,2,3,5,6,7,8,9,12,17,18,19,24,25) for e in evidence['events']):
        raise AnalysisError('stationary noncombat hold contains a combat/reveal event')
    if not isinstance(resources,str) or len(resources.encode('utf-8'))>MAX_REPORT_BYTES:raise AnalysisError('resource CSV missing or exceeds64MiB')
    reader=csv.DictReader(io.StringIO(resources));rows=list(reader)
    if not rows or len(rows)!=len(frames):raise AnalysisError('resource/CPU rows do not match every completed RT frame')
    timing={_identity(t,'timing'):t for t in report['imagePresentationTiming']['capture']['rows']}
    sample=[];previous_return=previous_serial=0
    for i,(row,frame) in enumerate(zip(rows,frames)):
        def integer(name):
            value=row.get(name,'')
            if not isinstance(value,str) or not value.isdecimal():raise AnalysisError('invalid CSV integer '+name)
            return int(value)
        if integer('serial')!=frame['submission'] or integer('tick')!=frame['tick'] or integer('serial')<=previous_serial:
            raise AnalysisError('CSV completed-frame serial/tick ownership mismatch')
        returned=integer('present_return_ns')
        if returned<=previous_return or integer('interval_ns')!=(returned-previous_return if i else 0):
            raise AnalysisError('CSV return timestamps/intervals are not continuous')
        previous_return=returned;previous_serial=integer('serial')
        if row.get('phase')!='lookout-sweep':raise AnalysisError('hold resource phase differs from lookout')
        if row.get('cpu_stage_status')!='valid' or frame.get('cpuStatus')!=5:raise AnalysisError('owning CPU stages unavailable')
        for stage in CPU_STAGES:
            integer(stage+'_cpu_ns');integer(stage+'_calls')
        if integer('skin_cpu_ns')!=integer('player_skin_cpu_ns')+integer('character_skin_cpu_ns'):raise AnalysisError('skin aggregate differs from owned player+character work')
        if integer('whole_frame_cpu_ns')!=frame.get('wholeFrameCpuNs') or integer('simulation_cpu_ns')!=frame.get('simulationCpuNs'):
            raise AnalysisError('CSV CPU facts differ from completed RT owner')
        if integer('device_local_bytes')!=frame.get('deviceLocalBytes') or integer('host_visible_bytes')!=frame.get('hostVisibleBytes'):
            raise AnalysisError('CSV memory facts differ from completed RT owner')
        for key in RESOURCE_FIELDS:integer(key)
        if integer('run_unloads') or integer('run_restores') or integer('saved_owner_bytes') or integer('omitted_triangles'):
            raise AnalysisError('full-scene hold changed residency or omitted geometry')
        state=states[frame['stateRow']]
        # Window selection uses this submission's bound simulation/input-owner wall time,
        # never a later CSV fence/return timestamp, simulation tick or display-clock guess.
        if warm<=state['wallNs']<=end:sample.append(i)
    if len(sample)<20:raise AnalysisError('fewer than20 completed post-warm samples')
    sample_walls=[states[frames[i]['stateRow']]['wallNs'] for i in sample]
    if sample_walls[-1]-sample_walls[0]<25_000_000_000:raise AnalysisError('post-warm completed samples cover less than25 of the declared30 seconds')
    def cpu(stage):
        all_values=[int(rows[i][stage+'_cpu_ns']) for i in sample]
        active=[int(rows[i][stage+'_cpu_ns']) for i in sample if int(rows[i][stage+'_calls'])]
        return {'allFrames':durations(all_values),'activeFrames':durations(active),'invocations':sum(int(rows[i][stage+'_calls']) for i in sample)}
    valid_gpu=[i for i in sample if frames[i].get('gpuStatus')==5 and frames[i].get('gpuAvailable') is True and isinstance(frames[i].get('gpuNs'),int) and not isinstance(frames[i]['gpuNs'],bool) and frames[i]['gpuNs']>0]
    sample_set=set(sample)
    present_intervals=[]
    for i in sample:
        if i-1 in sample_set:
            present_intervals.append(timing[_identity(frames[i],'frame',True)]['actualPresentTime']-timing[_identity(frames[i-1],'frame',True)]['actualPresentTime'])
    resource={key:span([int(rows[i][key]) for i in sample]) for key in RESOURCE_FIELDS}
    if any(resource[key]['minimum']!=resource[key]['maximum'] for key in ('unloads','restores','tlas_instances','blas_count')):
        raise AnalysisError('post-warm acceleration-structure/residency counts changed')
    selected_states=[states[frames[i]['stateRow']] for i in sample]
    tick_delta=selected_states[-1]['tick']-selected_states[0]['tick']
    skin_updates=[_u64(frames[i].get('skinUpdates'),'frame.skinUpdates') for i in sample]
    if any(b<a for a,b in zip(skin_updates,skin_updates[1:])):raise AnalysisError('player skin update count regressed')
    strict.pop('intervalsNanoseconds',None)
    return {'eligible':True,'eligibleForCompleteGpuCpuComparison':len(valid_gpu)==len(sample),
        'scope':'Private staged lookout203 stationary40s hold; post-warm completed RT frames only; not owner traversal acceptance',
        'reportSha256':hashlib.sha256(raw).hexdigest(),'resourcesSha256':hashlib.sha256(resources.encode('utf-8')).hexdigest(),
        'runId':report['runId'],'buildId':report['buildId'],'requestedExpectedTuple':dict(expected,scalePercent=expected_scale),
        'actualSettings':settings,'pipelines':evidence.get('pipelines',[]),
        'dimensions':{'nativeWindow':[settings['nativeWindowWidth'],settings['nativeWindowHeight']],
            'surfaceCurrent':[settings['surfaceCurrentWidth'],settings['surfaceCurrentHeight']],
            'swapchain':[settings['width'],settings['height']],'rtDispatch':[settings['internalWidth'],settings['internalHeight']],
            'logicalView':list(logical),'logicalAspect':logical[0]/logical[1],
            'dispatchAspect':settings['internalWidth']/settings['internalHeight'],'transformIndex':transform,
            'swapchainPreTransform':pre,'surfaceCurrentTransform':current},
        'sampling':{'originWallNs':start,'warmupEndWallNs':warm,'sampleEndWallNs':end,
            'completedFramesAll':len(frames),'completedFramesPostWarm':len(sample),
            'actualSelectedFirstWallNs':sample_walls[0],'actualSelectedLastWallNs':sample_walls[-1],
            'actualSelectedSpanSeconds':(sample_walls[-1]-sample_walls[0])/1e9,
            'warmupOrTerminalFramesExcluded':len(frames)-len(sample)},
        'gpu':{'scope':'completed owning GPU duration; never reciprocal FPS','availableSamples':len(valid_gpu),
            'unavailableSamples':len(sample)-len(valid_gpu),'statusCounts':{str(s):sum(frames[i].get('gpuStatus')==s for i in sample) for s in set(frames[i].get('gpuStatus') for i in sample)},
            'duration':durations([frames[i]['gpuNs'] for i in valid_gpu])},
        'cpu':{stage:cpu(stage) for stage in CPU_STAGES},
        'actualImagePresentationIntervals':durations(present_intervals),
        'resources':resource,'pose':{key:span([float(rows[i][key]) for i in sample]) for key in ('x','y','z','yaw','pitch')},
        'simulation':{'frozen':tick_delta==0,'tickDeltaPostWarm':tick_delta,
            'ticksThisFrame':span([s['ticksThisFrame'] for s in selected_states]),
            'simulationSecondsPostWarm':selected_states[-1]['simulationSeconds']-selected_states[0]['simulationSeconds'],
            'catchUpOverrunDelta':selected_states[-1]['overruns']-selected_states[0]['overruns'],
            'playerSkinUpdateDelta':skin_updates[-1]-skin_updates[0],
            'playerSkinCalls':sum(int(rows[i]['player_skin_calls']) for i in sample),
            'characterSkinCalls':sum(int(rows[i]['character_skin_calls']) for i in sample),
            'skinCacheOrIdleCaveat':sum(int(rows[i]['skin_calls']) for i in sample)==0,
            'phaseCounts':{phase:sum(rows[i]['phase']==phase for i in sample) for phase in set(rows[i]['phase'] for i in sample)}},
        'strictPresentationJoin':strict,
        'limitations':['GPU and CPU/wait durations overlap; never add them or call their reciprocal FPS.',
            'Actual image presentation intervals are VK_GOOGLE_display_timing evidence, not scanout or photons.',
            'Thermals, power, visual correctness, listening and natural playthrough require separate evidence.',
            'A live simulation can legitimately cache stationary skinning; call counts and update deltas are retained.',
            'This raw report cannot certify APK identity, asset/LOD contents or geometry appearance; retain separate build/asset custody.']}


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report',type=Path);parser.add_argument('resources',type=Path)
    parser.add_argument('--expect-scale',type=int,choices=(20,33,40),required=True)
    parser.add_argument('--expect-tuple',help='Explicit JSON water/fire/cap/glass/shadow/mist/dust; default is preserved owner tuple.')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args(argv)
    try:
        with args.report.open('rb') as f:raw=f.read(MAX_REPORT_BYTES+1)
        with args.resources.open('r',encoding='utf-8-sig',newline='') as f:resources=f.read(MAX_REPORT_BYTES+1)
        expected=json.loads(args.expect_tuple,object_pairs_hook=_object,parse_constant=_reject_constant) if args.expect_tuple else None
        result=analyze(raw,resources,args.expect_scale,expected)
        text=json.dumps(result,indent=2,allow_nan=False)+'\n'
        if args.output:args.output.write_text(text,encoding='utf-8')
        else:print(text,end='')
        return 0
    except (AnalysisError,OSError,KeyError,ValueError,TypeError) as exc:
        print(json.dumps({'eligible':False,'error':str(exc)}),file=sys.stderr)
        return 2


if __name__=='__main__':raise SystemExit(main())
