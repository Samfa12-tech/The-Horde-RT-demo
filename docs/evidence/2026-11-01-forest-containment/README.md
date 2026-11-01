# Forest corridor and temporary demo endpoint

This corrects the owner-reported walk-off gaps in the earlier private woodland
integration. The 97 deliberately open side spans are no longer accepted. The
mapped approach remains open from F04 to its first arrival waypoint, `(22.3,50.2)`
in native X/Z, about 16.6 metres beyond the lookout. A visible timber closure
beyond the turnaround marks the temporary demo endpoint. It is not a permanent
town wall: Bellwether remains the same unwalled, unfinished distant village.

The existing optional chapter-end panel explains the closure, future Bellwether
access and the return to the tomb. It remains available along the approach after
loading completed progress, without a new acknowledgement flag or save format.

## Authored geometry

The closed 226-span contour contains 47 proven existing terrain/solid sections,
155 overlapping low-boulder/crag sections and 24 timber modules with three end
posts. Rock orientation, shape and height vary using two existing approved rock
assets. The actual rendered rock faces also supply contact. There are no invisible
barriers, newly purchased/downloaded assets, textures or rendering roles.

All 480 tree placements and 120 fern placements are byte-for-byte unchanged.
Original terrain, tree contact, landmarks, Kit and rope access remain. The host
clearance comparison preserves the 2.4-metre route and its arrival continuation.

Exact costs compared with the preceding integrated woodland:

| Item | Previous | Corrected |
| --- | ---: | ---: |
| Boundary rock triangles | 0 | 58,486 |
| Fence triangles | 588 | 1,188 |
| Boundary contact triangles | 248 | 58,990 |
| Total CPU support/contact triangles | 73,142 | 131,884 |
| Bounded CPU intake ceiling | 98,304 | 147,456 |
| Added forest raw vertex/index bytes over original64 | 15,001,872 | 26,535,992 |

The raw render-geometry delta is 11,534,120 bytes. The intake ceiling does not
preallocate that many triangles and is not an RT instance increase. Existing
material and primitive rows are reused/consolidated. No phone performance result
is claimed. CPU nonwalkable storage grows by 4,071,952 bytes under the current
`RetainedBytes` accounting (triangle/normal/index arrays and reserved BVH nodes;
unchanged ground/step storage cancels).

## Containment evidence and corrected failures

The original long normal sweeps falsely classified some sloping terrain as a
boundary. An independent movement flood found diagonal, gradual climbs near
`(-20.4,11.35)` and `(-20.75,14.85)` that those sweeps missed. Natural-section
admission was tightened and the remaining gaps received visible geometry.

The final host gate uses the actual chapter step motor at fixed-step displacement
ceilings for run (3.2 m/s), dodge (0.9 m / 0.2 s) and crouch (1 m/s). Its 16,668
normal, oblique and join approaches use the real 0.24-metre radius and standing /
1.2-metre crouched heights. A separate eight-direction flood uses a smaller,
more permissive 0.22-metre radius / 0.75-metre body, gradual 0.05-metre movement
steps and 0.35-metre cells. It must exhaust without escaping and reach all seven
sentinels: F01, reunion, bends, F03, F04 and the endpoint. The original uncontained
scene must fail as a negative control. Final reachability covers 5,319 cells.

Actual `GameSimulation` input tests repeatedly restore the canonical lookout,
run/crouch/dodge into the timber closure, then return to F04. They require actual
3.2 m/s running and authoritative crouch/dodge states. Restore tests publish the
same resident-zone readiness as the production scene adapter and release the
fresh-neutral input barrier before movement. Arbitrary old off-trail coordinates
are invalid checkpoint records; persisted anchors remain unchanged. General
jump/mantle inputs do not exist. Rope traversal is checked separately by the
hidden native round trip.

An initial tall repeated-crag ring failed visual review. The retained lower mixed
stone edge preserves the accepted woodland horizon. Another intermediate build
ran the exhaustive movement checks during every startup and exceeded the hidden
observer's 90-second deadline. Its owned process subsequently finished; no retry
was launched while it was uncertain. The tested 47 natural-section choices are
now frozen in the authored layout, bound to the existing terrain identity.
Runtime retains lightweight ground/contact checks; the exhaustive host gate tests
the same production geometry. Native geometry witness
`8980520670790461166` is unchanged across that move. The corrected final F01
hidden process completed in 32.78 seconds including initialization and capture.
This establishes removal of that observed startup regression, not a frame-rate
benchmark.

Exact final build, test, capture, custody and process outcomes are in `receipt.json`.
Images are actual Vulkan RT output. This work uses no owner phone or foreground
window. The previous phone evidence does not certify this later geometry.
Mobile water behavior, fireflies and the 25–100% scale control remain unchanged.
No hosted CI, merge, signing, release or deployment is claimed; backup commits
use `[skip ci]` under the existing no-LFS-download constraint.

## Visual inspection

F01 keeps the accepted woodland horizon with a lower mixed stone edge. F04 keeps
Bellwether visible down the mapped approach. The short endpoint approach has
conspicuous taller stone sides on its slope; this is the bounded existing-asset
tradeoff, not a claim of owner acceptance of every view. The timber closure is
clearly ahead of the unfinished town and the return path remains readable.

The endpoint forward capture passes after the diagnostic staging path was
extended from F04 through real fixed-step movement. The reverse image is actual
RT output, but its capture admission failed the existing lantern-body pixel
requirement (zero body pixels, nonzero ring pixels) at both tried pitches. Grip
and authority tolerances passed. It is retained as **failed-capture visual
evidence**, not a passing capture or a weakened held-item visibility gate.
No held-prop implementation was changed as part of this containment correction.

Final normal replay completes 1,483 moving RT frames, returns through the tomb and
repeats the rope ascent. All 1,484 residency checks pass with zero static-detail
omissions. A separate explicit diagnostic F04 recreation advances scene epoch
2 to 3 on the same renderer, preserves the simulation tick and resource identity,
and reproduces the same storage-image SHA256. Both owned processes exit zero.
The final Android debug APK builds offline, passes the closed runtime roster
check and the lookout menu unit test; it was not installed. The existing 202-case
asset policy suite passes. The exact stopped-position objective test also passes.

## Reverse-view classification (same-executable control)

A bounded follow-up removes the added scenery/boundary content using the
existing original64 `baked-resident` override. This is the **same current Debug
executable**, not a historical executable baseline. It reaches the same endpoint
through real movement, turns to the same reverse camera and settles 120 fixed
ticks. The gate fails identically: player/ring/body pixels `4687/1164/0`, identical
hinge/body positions and grip/authority errors. The actual control image shows
open ground where the candidate has the rock edge, with the same clipped hand and
missing primary lantern body. The added containment geometry is therefore not
the cause of this failure.

Classify this as an existing carried-lantern framing limitation exposed by the new
reverse diagnostic. The comparison does not prove that the visibility expectation
is invalid, nor identify whether the abrupt reverse-turn history contributes.
The unchanged default gate still requires visible body pixels. Its printed
`wall-retraction=0` means no policy exemption, not a measured absence of physical
carry retraction. No gate, production pose or renderer change was made.

`reverse-control-classification.json` records exact binary/image identities and
metrics. The raw control PNG remains in the ignored local report path listed
there. A bounded future held-item investigation can compare gradual turn input
and carry-pose state at this exact point; this is separate from forest containment.
