# Kit stationary turn articulation - 20 October 2026

Kit now takes restrained alternating in-place steps while turning at rest. The
existing route root, 2.4 rad/s facing limit, movement/collision, dialogue clocks,
save schema and story rules are unchanged. A 0.36 s sin-squared step folds the
admitted rig's hip/knee/ankle by -0.25/+0.50/-0.25 radians at its peak. The same
indexed PBR skin pass applies the six additive joints beside existing head/chest
acting. There is no walking clip at zero route speed, new asset, GPU actor or IK
system. Walking fades a raised foot out; stopped facing finishes the current
step. Pause freezes it and restore/reset clear the transient state.

This is **partial turn articulation, not world-space foot locking**. A planted
foot still follows the logical root's yaw arc. Physical/artistic acceptance and
longer sequences remain open. The bounded layer is retained because the actual
rig and fresh native frames demonstrate alternating lift/replacement instead
of the previous two-planted-boot swivel.

## Measured evidence

The actual runtime GLB remains SHA-256
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.
Measurements use 947/942 geometric low-boot vertices selected at neutral idle
(Y < 0.20 m, absolute X > 0.035 m). These are contact proxies, not pressure or
biomechanical measurements. Raw skin and floor-normalized admitted actor are
both checked; re-grounding cannot conceal downward boot motion.

| Native real-rig measurement | Result |
| --- | --- |
| Peak raw sole lift, left/right | 47.893 / 47.974 mm |
| Peak admitted sole lift, left/right | 47.893 / 47.920 mm |
| Opposite support boot displacement before floor normalization | 0 |
| Opposite boot displacement after floor normalization | at most 0.0542 mm |
| Peak swing-boot horizontal displacement at full fold | under 27.4 mm |
| Two turns and settle through production route, 201 frames | 94 frames have a sole above 15 mm; root fixed and grounded |
| Summed low-contact centroid travel, same root/yaw, 15 mm proxy | 2.06665 m idle swivel; 1.47366 m candidate |
| Largest candidate centroid displacement per fixed step | 12.0882 mm |

The contact-travel reduction counts fewer low-contact samples as the boots lift;
it does not show that the remaining planted foot stops sliding. Tests retain
finite normalized normals/tangents, topology, UVs/materials and exact neutral
pose bytes. The [focused native log](focused-LastTest.log) retains the measurements.

## Fresh hardware RT and retained failures

The [comparison](comparison.json) binds all original PNG hashes, native identities,
matched motion states, resource observations and short timing samples. Original
PNGs remain in ignored `reports/horde-campaign-fixtures-kit-turn-20261020-*`.
No PNG, executable, GLB or APK is added to Git.

- Fresh baseline: 51/51 checks, 31 completed RT frames, 13.422 s.
- Candidate R1: 54 checks, one failure, 34 completed RT frames, 12.953 s.
  All stages completed, but the strict fixture allowlist rejected the three new
  turn-60/-80/-100 images. Independent byte custody confirms unchanged source
  save/manifest/lock and settings. This failed result remains rejected.
- Candidate R2: **54/54 checks**, 34 completed RT frames, 13.266 s. The fixture
  correction adds exactly those three output names; arbitrary files remain
  disallowed. All 27 R1/R2 PNGs are byte-identical. No runtime correction was
  needed after R1. The initial draft's backward knee signs were independently
  rejected before RT use; the [draft diff](rejected-axis-recipe.diff) is retained.

Every matched stage retains exactly the same root, yaw, walking flag, idle clock,
walk phase and weight. Eighteen baseline/candidate PNGs, including normal walking,
all stop samples and the saved/speech views, are byte-identical. Matched turn-10
before/after, candidate turn-30 and settled turn-100 were inspected as original
native frames: alternating boots lift, the body remains intact and both return
to idle. Original retained turn-0/-20/-40 views were inspected first. This is
seeded render-input evidence, not live interaction or naturally earned progress.

RTX 5050 Laptop / 610.47, 100% 1232x803, Diagnostic/High trace-rays and PC Mist On
remain unchanged. All motion frames retain 51 buffers, 60 allocations, 20 BLAS,
one TLAS, 28 instances, 48,278,656 host-visible and 374,395,104 device-local bytes.
The four changed turn samples have median GPU time 12.3544 -> 12.2141 ms. Across
all 20 matched samples the GPU median instead rises 10.4548 -> 12.2656 ms (+17.3%).
Inspection finds this rise in byte-identical walking views (median 10.4548 ->
17.2307 ms); candidate R1 also has that rise while its images equal R2. There is
no allocation/workload growth or attributed turn-layer GPU regression, but these
short Debug readbacks do **not** resolve the aggregate timing variation, certify
performance parity or establish a sustained/phone gain. Character-skin median
25.2564 -> 23.0868 ms is equally noncertifying. Preserve the warm acceptance gap.

## Validation and custody

Five focused Release suites pass: actual Kit source/runtime rig, locomotion,
acting, chapter dialogue and campaign checkpoints. Locomotion passes 2,464 checks
in Debug and Release; the unchanged flat-support route still takes 2,254 ticks.
Windows Debug/Release and build-only Android Debug Shipping/Mobile for all four
ABIs pass. [Exact artifact/source/custody receipt](receipt.json).

No phone operation or Java change occurred. Audio/haptic manual revalidation
required: **NO**; this visual layer changes no semantic timing, listener/source,
gain, playback, haptic or damage/death input. Earlier actual listening stays pending.

Native owned windows stayed hidden and were never observed foreground; all API
foreground samples were null, which does not identify an active external HWND.
All owned processes exited. Owner campaign data remains absent, settings and both
canonical dirty-file hashes are preserved, and owner untracked assets are untouched.
Same existing WP2 worktree/branch; no public push, release, phone, Reef or preview
operation. Raw logs are byte-preserved and indexed in the receipt.
Source/authored-document whitespace checks pass; immutable Windows raw logs
retain CRLF/trailing output bytes and are excluded from that formatting check.

Passing route/repeat-ascent fixtures remain parked. The earlier Debug campaign
CTest timeout remains **open**: 30.01 s at its 30 s gate; its separate 145-check,
zero-failure 39.609 s diagnostic does not pass that timing gate. No limit was
raised or campaign test rerun here.

The [remaining scope](../../design/HORDE_1_7_REMAINING_SCOPE_2026_10_20.md) keeps all
six recovery decisions pending. Next meaningful independent content work is a
bounded audit of existing licensed settling-grit/structural-creak material,
without admitting an unsuitable cue or claiming listening. Recovery implementation
awaits Sam's policy; natural victory/claim/death, controls/lifecycle, actual
listening/art and sustained warm Keeper/exterior acceptance remain separate.
