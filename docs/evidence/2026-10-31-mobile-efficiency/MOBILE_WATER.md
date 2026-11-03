# Mobile water: omit optional contact droplets, retain the waterfall

**KEEP this bounded quality reduction.** Mobile water now retains the waterfall,
water surfaces and contact ripples without the small transmissive splash
droplets. High water retains its original droplets and reflections. Existing
saved tier values are unchanged; Android and Windows settings explain the
difference. This addresses a measured waterfall hitch, **not whole-chapter
playability or a general FPS improvement**. No RT backend, shader, asset,
lighting, gameplay, allocation ceiling, forest or worker default was changed.

The native Android fresh/reset scale was also corrected from 50 to the already
approved 33, matching Java. Historical baseline, invalid-value recovery and
explicit saved choices remain separate. The comparisons already used saved 33,
so that correction does not explain their results.

## Controlled phone comparison, 1 November 2026

SM-S948B / Android 17 / Adreno 840 / Vulkan 1.4.295 / driver 2150932499.
Source base `77cfc160589aa4821cc7fb464664cce0ab4e952e` plus an archived private
same-APK switch; APK
`2eba5e5f5ec0ee1298b0b9ec2e9cd19ef362ea4c4c977d0765c65bca213b4bf2`.
Shipping/Mobile RT Pipeline, saved 33%, 475x980 internal / 1440x2971 output;
Mobile water/fire, Current shadows, Glass/Mist Off, Low dust. Menu cap 30 does
not pace these Showcase routes. All 189 assets and 16 shader payloads matched
the baseline. [Receipt, exact artifacts and private input hashes](mobile-water-receipt.json).

One resident process, 30 seconds initial preparation, three conditioning routes,
then A/B/B/A. A retains authored droplets; B hides only their fixed contributor.
Every measured route has a newly acknowledged equal starting fingerprint.
There were 2,844 strictly joined completed RT frames/stage/presentation rows.
No image readback occurred inside timed routes. Rates below use actual image
presentation timestamps, not inverse GPU durations.

| Case | Whole route images/s | Water window images/s | Water median / p95 ms |
| --- | ---: | ---: | ---: |
| A1 authored spray | 16.707 | 12.333 | 91.667 /120.833 |
| B1 spray Off | 16.691 | 13.567 | 62.500 /116.667 |
| B2 spray Off | 16.353 | 16.000 | 54.167 /100.000 |
| A2 authored spray | 15.924 | 11.793 | 91.667 /129.167 |

The predefined approach window is simulation seconds 15-18. Both B passes
improve its actual image rate against both controls by 10.0-35.7%. A's first
active-contact dispatches took 158.737 / 179.364 ms and selected GenericDielectric;
B retained simulated contact but never selected that strategy. The waterfall
itself stayed enabled. This identifies an optional effect that can make the
entire frame use the more costly transmission variant.

Limits matter: closing whole-route control drift was -4.682%, close to the 5%
limit. B1's whole-route p95 was 95.833 ms versus 83.333 ms in A1, almost exactly
the 15% limit; B2's whole-route rate was 2.119% lower than A1. The declared local
gain/nonregression gates pass, but they support the local hitch reduction only.
There is no claimed average whole-game saving, stable 30 FPS or future-forest gain.
All sampled framework thermal statuses were 0; measured starts differed by at
most 0.3 C across AP/BAT/SKIN. Readable thermal power levels varied 0-4. GPU clock
access was denied and was not retried or bypassed; exact frequency equivalence
is unknown.

## Appearance, final implementation and checks

Two separate untimed native routes captured active simulated contact. Six exact
frame-bound stills were inspected, including lit water and the doused-torch
transition. Water presentation remains; tiny spray is difficult to distinguish
in these ordinary views. Camera positions differ, so this is **not pixel parity**
or complete moving-camera/high-quality visual acceptance. Raw images stay local;
receipt hashes do not substitute for their bytes. The owner independently
reports never noticing spray. High remains available rather than removing it.

The private experiment switch/capture plumbing was archived and removed. Final
normal-policy APK
`277a41f2a30c3b69ad92f43c4530a95c294388e38c011ffd1c75054f6703ab17`
completed a separate generated transit: 793 completed frames, 28 active-contact
frames, all 28 on the expected OpaqueFast shader with Mobile water still enabled.
No private spray switch remains. This is final-package correctness evidence,
not another performance comparison or natural chapter acceptance.

Four relevant Release CTests passed: graphics defaults/transactions, Windows
benchmark ownership, water-contact simulation and RT resource inventory. The
last verifies High -> Mobile -> High restores identical live droplet geometry
without changing topology. Windows Release built; Android arm64 built, 24 Java
preference/scale tests passed, and lint had 0 errors / 92 existing warnings. High
material/shader/assets are unchanged. Audio/haptic manual revalidation required:
**NO**, because these changes do not alter audio/haptics or feedback events.

An initial attempt stopped after its first conditioning run. The next fresh
checkpoint differed only in the diagnostic damage flag derived from the prior
paused state. It contributed no A/B result. R2 explicitly treats the first run
as priming; all subsequent conditioning/measured fingerprints must match fully.
The earlier 31 October failed opaque-spray cohort remains failed, not superseded
by a relaxed reinterpretation of its gate.

Final release **00:49:29 UTC**: original baseline Debug APK restored/stopped,
preferences and 15 original campaign files unchanged, automatic brightness and
rotation restored, production/actor-worker packages untouched. No owned device
activity remains. All 151 parked owner files were preserved. No Windows foreground
or new LFS download was used. Hosted CI is not claimed: the development backup
uses `[skip ci]` while the no-LFS-download restriction remains.

The remaining phone-default work is representative sustained opening/combat/
outdoor validation and other measured effect choices. The existing light-range
candidate remains unmeasured and unadmitted. This change does not finish that work.

## Paused checkpoint and next bounded investigation

Owner requested a pause after this completed session; no automatic continuation.
First, investigate direct fire-light visibility cost using the existing unadmitted
light-range source as a hypothesis, not a retained optimisation. The older Fire
Low option changed flame-volume sampling only and its comparison was inconclusive.
Use a common current baseline containing this Mobile-water change; do not compare
old light-range APKs against the newer policy. Preserve full light reach in High
and assess any shorter-range Mobile/Low option visually before admission.

Use isolated `perf-20261031` state, verify actual saved settings and current APK/
shader hashes, then run `keeper-torch-transition` and `chapter-performance-route`
with their fresh native acknowledgements. Reuse the recorded one-process warm
conditioning/equal-start method where the probe permits it. A new comparison
needs its own predeclared controls and receipt names. Match opening, lit Keeper
and outdoor views, actual presentation tails, light counts, thermal state and
readable power level; stop on invalid controls instead of averaging drift away.
Do not retry the denied GPU-clock path. The native frame-stage CSV can separate
AS/dispatch/CPU costs without that access.

The broader default still needs sustained ordinary gameplay and control feel on
the intended phone settings. Current whole-route rates around 16-17 images/s do
not establish that goal. No new density, reconstruction, actor-worker, inverse-
transform or previously rejected unchanged renderer trial follows from this
checkpoint. Future dense-forest cost remains unknown.
