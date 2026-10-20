# Phone performance and mobile Mist Off checkpoint - 20 October 2026

Mobile **fresh-install / staged Reset Defaults now selects Mist Off**, following
Sam's report that mist plus the two lights made the Keeper fight chuggy. Existing
explicit saved choices, historical presets and Windows defaults are preserved.
The current phone's saved Mist On value therefore remains unchanged. The cleared
route measured here cannot accept the live Keeper workload or overturn that
owner choice.

Work remains in the existing actual worktree:
`C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`,
branch `codex/horde-1.7-wp2-vertical-proof`, existing draft PR27. No worktree copy,
stash/reset, public push, release, Reef operation or destructive cleanup occurred.

## What the optimisation work established

The earlier [exact-zero-mist exit](../2026-10-18-zero-mist-exit/README.md) retained
repeated exact lower-tomb GPU gains of 18.75-18.80% Pipeline and 17.58-17.60%
QueryCompute with pixel-identical pairs. It did not achieve sustained 30 FPS or
solve the live two-torch workload. Closed first-blocker/selective-opacity,
inverse-transform and lower-indirect trials were not reopened.

This fresh baseline uses the current r23 terrain, ordinary production
Shipping/Mobile shaders, real Pipeline RT and the preserved 40% phone setting:
576 x 1188 internal, 1440 x 2971 output. It measures player skinning about
6.4-6.8 ms, another roughly 5 ms for Kit in the exterior holds, player pose-copy
about 0.7-0.8 ms and upload about 0.36-0.45 ms. Exterior GPU cost remains about
137 ms. The [baseline receipt](baseline-receipt.json) and
[CPU ownership assessment](cpu-preparation.md) distinguish active CPU work,
submission/fence wait and GPU time. **No CPU scheduling optimisation is adopted.**
The fresh simulation snapshot currently follows the sole fence; moving existing
work mechanically would change input freshness or reuse stale poses. An owned,
immutable preparation bundle retaining current order is the safe prerequisite.

## Bounded quality comparisons

[Exact run identities, per-view medians/tails, poses, thermal admissions and raw
hashes](quality-receipt.json) preserve all six valid routes / 4,209 owning
completed frames. Seven timed attempts were used, including one failed attempt,
within an eight-attempt maximum. Two preparations stopped before timing.
Controls and their trials use the same exact APK within each cohort; separate
APKs are never treated as one causal comparison. Only the lower and repeat-ascent
poses match exactly. Landing and passage remain descriptive.

| Exact view / cohort | Control GPU ms | Trial GPU ms | Actual display median, control / trial |
| --- | ---: | ---: | ---: |
| Lower, A2 / Mist Off B1 | 53.848 | 54.133 | 62.500 / 62.500 ms |
| Repeat ascent, A2 / B1 | 137.337 | 137.806 | 154.167 / 154.167 ms |
| Lower, A5 / combined E1 | 53.992 | 54.419 | 62.500 / 62.500 ms |
| Repeat ascent, A5 / E1 | 137.023 | 137.623 | 154.167 / 154.167 ms |

Combined E1 is Mist Off / Fire Low / Dust Off / Current shadows / Mobile water,
40%, Glass Off and real hardware RT. Mist and dust actual-upload fields match
throughout; native graphics serial 2 presents the exact full tuple before motion
arms. No preferences are written by these profiles. Whole-route display medians
remain about 75 ms, p95 about 158 ms; every interval exceeds 33.333 ms. This is
not a gameplay 30 FPS pass, even though the saved preview cap is 30.

Neither quality comparison establishes the declared 3% useful gain. No additional
Fire/Dust default is adopted. Fire Low reduces fire sampling; Dust Off removes
participating indoor dust and Mist Off removes mist. Visual tradeoffs for these
new tiers are not accepted: no image readback contaminated timing, and the
terminal pause-menu screenshot is excluded. Individual Fire Low and Dust Off
costs remain unmeasured. The live Keeper/two-light case remains the relevant
next workload, rather than another cleared-room quality loop.

Both accepted pairs pass the unchanged 1.5 C per-sensor start and end comparisons
at current-HAL framework status 0. A5 starts AP/BAT/SKIN 42.3/29.6/34.6 C and ends
40.6/32.6/36.1 C; E1 starts 43.2/30.7/35.5 C and ends 42.1/33.9/37.1 C. USB charging
is recorded. GPU clocks/power are not fixed or measured; thermal status alone
is not causal proof. There is only one combined trial. Closing control A6 never
launches: the 180 s passive cooling bound ends at 31.8/29.7/31.8 C, above its
BAT 29.4 C preparation ceiling. No retry or relaxed admission follows.

## Failed evidence and the verified fix

Fire C1 fails preparation at AP 40.1 C versus the original A3 reference 42.5 C,
outside 1.5 C. Its copied common latest report is stale and excluded. Fire C2
arms under Fire Mobile before the asynchronous Fire Low command reaches the
native owner; the existing isolation guard aborts. Neither is Fire Low timing.
Their [rejection receipts](validation/pipeline-fire-c2-rejection.json) remain.

The repaired profile path waits for exact request serial, current surface,
requested and effective tuple, zero failure reasons and successful Showcase RT
presentation. A cold launch binds its first surface once; timeout remains30 s.
Pause cancels pending work, rejected replacements retain their original gate,
and WAIT cannot dispatch an older checkpoint/practice/replay. Native motion
interruption guards are unchanged. A5 and E1 then complete through the strict
actual-presentation analyzer. A separate read-only review found no remaining
material integration issue.

Local implementation checkpoints are `95354ad0` (measurement) and `9f9da695`
(Mist Off default and transient presentation ordering). The installed exact
Debug APK is `e699bfa82a784827ade52b440e40eb0e4df39da4faa03f3c269959566e09d57e`,
compiled at 9f9da695, optimized native code, all four ABIs, production
Shipping/Mobile shaders and route timing. All 175 packaged assets match the
instrumented baseline. No shader math or quality default besides Android mist
changes. The compatible Debug signer and installed hash are verified.

## Checks, custody and remaining acceptance

Android build and **360/360 tests**, CPU analyzer **5/5**, and native mobile
settings **1/1 Release** pass. The earlier focused CPU-preparation Release lane
is **7/9**: existing resource-roster counts and source-string smoke expectations
remain mismatched. Their original failures and source proof are preserved in
[the ownership assessment](cpu-preparation.md). A newly introduced survey early
return regression was caught by its existing test, preserved and corrected;
no assertion was weakened. These results do not constitute the full host gate.

[Final custody](final-custody.json) verifies all 12 campaign records/locks and the
Horde settings file byte-exact; Debug is stopped, installed artifact confirmed,
release app untouched. Android's own ActivityThread ID metadata changed earlier;
no restoration was attempted. No combat input was issued.

The current milestone map remains coherent: lantern/rescue/rope/reunion and
night continuation exist; combat/dodge/controls/hearts/subtitles/VO and three-slot
manual/latest-auto saves have code and bounded proofs. Full natural earned-save,
death Load, rope/equipment in both directions, touch/orientation/lifecycle,
listening/mix/haptics and owner acceptance remain open. Current r23 regional
terrain/water and finite night ambience are local development, not a finished
outdoor acceptance or permission for heavy content expansion. WP10 remains open.

Next bounded slice: establish a current code-level contact/target-separation
publication and a noncombat live two-light Keeper performance/appearance fixture
at 40% / Mist Off. Resolve that critical-path evidence before more blind combat
input or heavy outside content. The separate immutable CPU bundle proposal can
then be implemented with current order and freshness/failure tests before any
overlap trial.

The earlier automatic approval rejection of phone Resume -> 180-degree turn ->
Swing at the isolated Keeper remains in force: repeated blind non-contact
retries lacked a valid current target separation. It was not retried. Natural
combat/earned campaign acceptance is therefore still incomplete.

## Actual devlog images

Two original, unedited 960 x 540 PNGs are saved locally under
`reports/devlog-2026-10-20/` and successfully uploaded to Library. They are dark,
work-in-progress Windows RTX 5050 Laptop diagnostic captures from the existing
b2d548a2 Debug binary, not phone captures or evidence of earned progress.
[Full provenance, shader policy, setup, hashes and Library IDs](devlog-provenance.json)
distinguishes the low-held tomb lantern (Higher shadows / High dust) from the
river-bend environment survey (Current shadows / Low dust); both use High fire
and water. One arch candidate failed its existing primary-arms visibility guard
and is excluded. No image enhancement, crop, public post or desktop foreground
input occurred.
