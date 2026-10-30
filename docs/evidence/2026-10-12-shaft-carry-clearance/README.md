# Shaft carry clearance, 12 October 2026

Development milestone on PR27, based on `a5ac1299045adef5177b1983a677707789af2df8`.
The full tomb-to-woodland chapter remains incomplete.

## Reproduction and correction

The owner observed arms retracting as if an invisible wall activated their IK.
The frozen landing north/down-shaft views reproduce zero primary-arm pixels on
both hardware RT backends. A failing native authored-terrain regression confirms
that carry clearance used the movement/support sweep: the unsupported opening
was treated as a wall even when the player stood on the valid landing.

Held-item clearance now queries a fixed-height swept volume against the same
immutable terrain triangles, actual trunks and retained shaft fittings. Terrain
triangles are clipped to the height band before projected segment/capsule tests;
both windings, vertical/oblique walls and downward-facing solids remain blocking.
Empty space underneath an item is clear. Player movement still requires the
unchanged continuous ground sweep; starting inside the unsupported shaft still
rejects, and no collision, reach, combat or immunity assertion is removed.

The conservative upright carry band is 0.25–2.30 m above support, using the existing
player collision radius. This is a reversible engineering envelope, not a general
rotating-item collision solver. An unobstructed full reach still needs one sweep
before the existing 2.5 cm search fallback. BVH traversal uses fixed six-point
clipping arrays and allocates no memory during a query.

The first positive test incorrectly expected full reach directly through the
existing overhead cantilever. Discriminating queries show terrain is clear, but
that real solid correctly limits reach to 2.36 m. The revised regression separately
requires full reach beside it and the correct obstruction directly ahead. Its
earlier failure is retained in `shaft-carry-release-tests.log`; the original
support-induced failure is retained in `shaft-carry-before-tests.log`.
The requested ground stake/rim-contact rope integration remains next; this
milestone does not disguise the old frame as a stake.

## Actual RT comparison

RTX 5050 Laptop GPU, 960×540 at 100%, Diagnostic High optical bundle, identical
checkpoint staging and settled camera states. North is yaw 0, pitch −0.15;
the requested downward view is safely clamped by the existing camera to −0.32.
No foreground input was sent. All four candidate frames were opened and inspected,
and the before north/Pipeline and shaft/Query frames were reopened for comparison.

| View | RayTracingPipeline | RayQueryCompute |
|---|---|---|
| North | [before](pipeline-north-before.png) / [after](pipeline-north-after.png) | [before](query-north-before.png) / [after](query-north-after.png) |
| Down shaft | [before](pipeline-shaft-before.png) / [after](pipeline-shaft-after.png) | [before](query-shaft-before.png) / [after](query-shaft-after.png) |

Each image has an adjacent same-prefix `.json` native record. Both candidate
backends pass visibility and physical lantern/ring authority checks: north has
58,534 primary player pixels and the down-shaft view 55,584, versus zero before.
Ring position/orientation errors are zero in all four candidate records. The
lantern and its light follow the solved carry transform. These scene-only staged
frames do not prove full encounter input, restored sword presentation or owner
acceptance; the native repeat-traversal/attachment tests cover the separate state.
Twelve captured GPU frames are correctness evidence, not sustained performance.

## Checks and exact identities

Windows Debug and Release affected applications and test executables rebuilt.
Both selected CTest lanes pass 13/13, including authored terrain, repeat traversal,
rescue attachment/adversarial cases, held-item clearance/transition, vertical
support and directional rig. The original final torch sweep passes unchanged in
171.45 s Debug and 21.83 s Release under its existing 180-second policy.
Android Debug builds all four ABIs; Java unit-test/lint tasks succeed (unchanged
Java/lint analyses reused their valid Gradle outputs). Exact closed-package and
held-item/source-separation checks pass. No APK was installed.

Shader validation status is recorded in the retained logs. The initial sandbox
run could not create QueryCompute compiler output; the SDK run passes that check.
Two independent raygen tests then reproduced stale compatibility/opaque-word
witnesses left by the indexed-PBR Kit guard at `62a688ae`. Fresh compatibility
compilation matches the existing embedded words. Only those pinned test identities
are refreshed; all mutation, negative, shape, capacity and frozen budget checks
remain. Production shaders, catalogs, quality and budgets are unchanged by this
milestone. Matrix-manifest validation passes in 32.94 s, the complete artifact/
freshness/negative suite in 89.72 s, and QueryCompute freshness in 35.70 s. An
independent fresh eight-key catalog replay also passes. Intermediate stale-pin
and sandbox failures remain alongside the successful records.

[Exact source-file, executable, APK and shader/budget identities](identities.json)
and [fresh compatibility identities](compatibility-identities.json) make the
candidate reproducible independently of the historical `1.6.2` development build
label. Failed-run evidence is retained. The untracked Blender backup remains at
SHA-256 `0dae1e8fb803336cef2d045f48c5f4c9195a6463f45161cb0b6c7434350769fb`.

## Remaining chapter gates

Ground stake, graveyard lid/rim composition, correctly reachable rim pull-up grips,
real two-way GPU zone preparation/retirement and full inspected chapter traversal
remain required. Supplied trees stay; replacement-canopy work and mist issue28
remain deferred. Current/Higher shadows and saved Lower→Current migration on a
phone, chapter lifecycle/touch, sustained performance and owner audio/haptic/visual
acceptance remain physically pending. No release, merge or new capacity is claimed.

Audio/haptic manual revalidation required: **NO for this carry/query/test fix**;
the wider changed chapter still requires its existing owner listening/haptic review.

Next owner route: after the next requested launch, climb, look around and down the
shaft, raise/lower the lantern, approach a real trunk/bank, then descend and ascend
again. Report any arm retraction in empty space or item clipping through a solid.
