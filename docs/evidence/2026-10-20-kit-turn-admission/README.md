# Kit turn-step admission decision - 20 October 2026

**UNADMITTED.** One bounded desktop control/candidate/control comparison could
not establish acceptable comparable cost for `b97823fb`'s turn-step experiment.
Its six owned runtime/dependent-test files are restored byte-for-byte from
`feef1471`. The prior cadence, start/stop blend, proper facing, speech acting,
trail-root fix, route/save rules and owner work remain. Stationary turning is
again the previous idle swivel; no foot-locking or visual-quality pass is claimed.

The [original experiment](../2026-10-20-kit-turn/README.md), its failed trials and
raw identities remain intact. [Unadmitted experiment patch](unadmitted-turn.patch)
retains only those six files for review. There is no automatic retry or new
audio cue. No phone parity is claimed.

## Bounded method

The [fixed plan](plan.json) was recorded before execution: exactly A1 control,
B candidate, A2 control. The existing hidden native fixture adds twelve warmups
and sixty measured frames for each of two groups. Six actual production-route
walking snapshots (ticks 20/35/50/65/80/100) and six stationary-turn snapshots
(10/20/30/40/60/80) replay in the same order. Clocks/roots change between samples,
so skin/upload/refit is exercised rather than cached away. This is serialized
immutable-pose component profiling, not natural traversal or ordinary pacing.

Both Debug builds use the same harness, assets, 100% 1232x803, Diagnostic/High
trace-rays, PC Mist On, RTX 5050 Laptop and driver 610.47. Their sole renderer
difference is the original two-joint `CompanionUpperBodyRotations` call versus
the experimental eight-joint `CompanionPoseRotations` call. Both execute the same
transient simulation oscillator; its incremental CPU cost is not isolated.
No renderer quality, power, fan or clock policy was changed. Both executables
remain local ignored artifacts, with exact identities in [the source freeze](source-freeze.json).

Admission required GPU/character-skin median and p95 within 15% of the lower
control bracket, no correctness/resource/custody regression, AC stability, group
median temperatures within 3 C and graphics clocks within 10% of both controls.
Whole-frame cycle CPU is also retained as a secondary regression signal. These
criteria were not relaxed after seeing results. Pre-run temperature conditioning
was bounded to 30 seconds, within 2 C of the first control. No extra comparison
was attempted after the three prescribed runs.

The unchanged **180-frame / 90-second native limits** remain enforced. Each run
passes **58 checks / 178 completed RT frames** (534 total); process durations
are A1 23.860 s, B 23.454 s, A2 22.219 s. Sixty admitted current-submission GPU
timestamps per group/run give 360 samples. All have successful RT presentation,
matching completion serials and valid GPU timing with zero reported timing errors.

## Absolute results

All values below are **median / nearest-rank p95, milliseconds**. See
[samples](samples.csv), [arithmetic and receipts](analysis.json) and
[coarse thermal samples](thermal.json).

| Group / metric | Control A1 | Candidate B | Control A2 |
| --- | --- | --- | --- |
| Walking GPU | 9.738 / 18.099 | 7.953 / 18.116 | 9.634 / 10.870 |
| Turning GPU | 3.511 / 7.230 | 3.513 / 7.214 | 3.519 / 7.304 |
| Walking character-skin CPU | 23.180 / 30.259 | 23.010 / 27.048 | 23.215 / 26.474 |
| Turning character-skin CPU | 23.188 / 26.636 | 23.111 / 28.026 | 23.214 / 34.062 |
| Walking whole-frame cycle CPU | 53.665 / 375.712 | 53.890 / 363.686 | 53.615 / 62.666 |
| Turning whole-frame cycle CPU | 25.514 / 31.870 | 25.641 / 33.902 | 26.131 / 39.329 |

The changed-turn GPU median is within 0.1% of both controls; character-skin
medians differ by less than 1%. The earlier 17.3% aggregate increase is therefore
not reproduced as a consistent median animation cost in this comparison.
However, walking GPU p95 is 66.7% above A2, and whole-frame walking tails vary
by about sixfold between the controls. A1 already exhibits the large tail without
the leg layer. This demonstrates instability in an unchanged view, **not** a
causal explanation or permission to classify the original rise as harmless noise.
Sampling/driver/background/pacing contributions were not isolated.

GPU-component medians are below 16.667 ms; character-skin medians lie between
20 and 33.333 ms. These separate component bands are not game FPS, and are not
added together to invent frame pacing. The whole-cycle figures are for this
serialized Debug workload only.

| Measured group | A1 temperature / graphics clock / power median | B | A2 |
| --- | --- | --- | --- |
| Walking | 43 C / 780 MHz / 12.24 W | 43 C / 750 MHz / 11.92 W | 44 C / 757 MHz / 12.035 W |
| Turning | 43 C / 337 MHz / 11.99 W | 43 C / 405 MHz / 6.54 W | 45 C / 675 MHz / 14.34 W |

AC stays connected and temperatures span 42-45 C. The turn-group graphics-clock
envelope fails the predeclared 10% criterion. Read-only `nvidia-smi` observations
are coarse, with only 3-6 readings per measured group; they can sample idle gaps
and may perturb the driver. They are not per-frame clock proof. CPU temperatures
were unavailable. Consequently the result is **incomparable for admission**, even
though the temperature condition itself is close. Cost and comparability gates
remain failed; no sustained or phone parity can be inferred.

## Correctness, restoration and remaining work

All control A1/A2 PNGs are byte-identical. Timed walking PNGs also match B exactly;
turn PNGs show the intended leg-only difference. All measured frames retain the
same resource inventory: 51 buffers, 60 allocations, 20 BLAS, one TLAS, 28
instances, 48,278,656 host-visible and 374,395,104 device-local bytes. Original
PNG/native JSON files remain under ignored `reports/horde-campaign-fixtures-kit-turn-admission-20261020-*`;
their sizes/hashes are pinned in the receipt, rather than committing large captures.

The safe restoration removes only the owned production turn state/recipe and
dependent experiment tests from the current candidate. The new Debug-only timing
fixture remains bounded and guarded; normal gameplay does not select it. The
restored runtime is checked by Windows Debug/Release builds, five affected Release
suites and a four-ABI Android Shipping/Mobile Debug build. Restored locomotion
has its original 2,278 checks and 2,254 flat-route ticks. No further RT comparison
or campaign fixture rerun follows the decision. Exact current artifact identities
and original-byte custody are in [receipt.json](receipt.json).

All native processes exited; no owner process was killed. Windows remained
unshown/nonactivating; sampled foreground HWNDs were null, which does not identify
a particular external foreground app. Source fixtures, settings and canonical
owner-dirty hashes are unchanged; owner campaign data remains absent. Owner
untracked assets, mobile Mist Off, Reef and its preview are untouched. No phone,
push, release, credentials, spending or budget action occurred.

The earlier campaign Debug timeout stays open: 30.01 s at its unchanged 30 s
gate; the separate 145-check/zero-failure 39.609 s diagnostic is not a timing pass.
Audio/haptic manual revalidation required: **NO**; restoration affects visual
posing only. Existing listening acceptance stays pending.

Next unblocked existing 1.7 slice: a bounded provenance/suitability audit of
already licensed settling-grit/structural-creak material from master 4.1b/5.5/8.4/11.
That authorizes no unsuitable cue or listening claim. The
[six recovery decisions](../../design/HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md)
remain pending; natural victory/claim/death, physical controls/lifecycle, actual
listening/art and warm Keeper/exterior acceptance remain separate requirements.
