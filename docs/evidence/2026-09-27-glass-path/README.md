# Bounded glass-path investigation (September 27)

Investigation only, source baseline `5935db9`. None of the probe macros, output
overrides or temporary Diagnostic footprint allowances is in the candidate.
The original shader budgets were restored before compiling the actual fix.

The isolated production lantern view produced one primary volume failure on
Windows RTX 5050 Laptop, Diagnostic/Mobile, 540x960. An initial colour-only
probe was rejected by the existing descriptor-roster guard after dead-code
elimination; the guard was not weakened. A revised marked-pixel probe retained
ordinary colour: zero orientation-disagreement markers and one second-entry
marker at `(184,459)`. Thus a face-sign mismatch did not explain this ray.

`probe/ray-record.png` encodes this ray's interfaces in its top row. Each
float32 is little-endian: first pixel RGB contains bytes0–2, next pixel R byte3.
There are14 fields per slot: positionXYZ, geometricNormalXYZ, incomingXYZ,
local distance, instance, material, primitive and volumeOpen. Two slots:

| Field | First entry | Later incorrect entry |
| --- | --- | --- |
| Position | (-35.032856, .16051765, -17.305923) | (-35.067791, .15234110, -17.321180) |
| Outward normal | (.49999997, 0, .86602539) | (.49999985, 0, -.86602545) |
| Incoming direction | (-.93660992, -.31922308, .14442492) | (-.89640343, -.21001518, -.39032623) |
| Local distance | 1.6899854 | .03898829 |
| Instance/material/primitive/open | 8/115/55/0 | 8/115/51/1 |

The actual reward body GLB SHA256 is
`34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4`.
Its72 glass triangles form six disjoint12-triangle panes. With the actual
column-major stage transform, socket translation `(0,1.28,.30)` and scale.44,
body origin is approximately `(-35.040192,.33,-17.4)`. Entry triangle55 is in
component4; its outward-facing side exit triangle20 is only26.206 micrometres
along the refracted ray. Later triangle51 is a different pane (component3).

The old70.066-micrometre direction advance skipped the nearby exit; its
35.033-micrometre query minimum was also longer than that exit distance.
The normal offset changes the ray line, so a projected-distance subtraction
must not be presented as an exact overshoot measurement. A read-only review
initially transposed the stage matrix; that result was rejected and corrected.
Current pane AABBs are separated by at least24.7218mm in local coordinates.
General admission checks manifold closure/winding, not component disjointness:
that separate validation gap remains open and is not evidence of asset overlap.

The source-only probe patch and module statistics are retained for provenance,
not as a production build option. Candidate fix and current-device results:
[September30 evidence](../2026-09-30-glass-corner/README.md).
