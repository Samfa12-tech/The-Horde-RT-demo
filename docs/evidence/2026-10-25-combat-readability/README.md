# Attack readability, pause icon and retained Keeper study

The shared current-combat snapshot classifies actual attacks as Neutral,
Parryable or Unblockable. Skeleton wind-up/active attacks use yellow with a
diamond and explicit PARRY text; the Keeper charge/visible discharge uses red
with a cross and DODGE text. Neutral clears the warning immediately. Optional
teaching being disabled, completed or skipped does not remove attack readability.
Pause, death and inactive encounter gates remain authoritative. No new attack
type, yellow guard-break rule, damage timing or defense window is introduced.

Both existing platform prompts consume that same classification. Android adds
spoken descriptions; Windows preserves OS high-contrast colors. Existing distinct
enemy cue audio is unchanged, not newly listening-accepted. The unarmed skeleton
lesson now names its raised arm/attacker rather than a nonexistent enemy blade.

The Android MENU control becomes a circular pause icon at the **same** upper-right
safe-area anchor: 48 dp touch target, 24 dp glyph, existing 16 dp safe margins,
callback, saved opacity and Pause / menu description. Lower movement/look/action
zones are not rearranged. Physical comfort and the new icon on the phone remain
unverified; the frozen `69e46426` APK does not contain this change.

## Heavier Keeper animation: retained, not admitted

The actual existing rig supports a brace, stronger free-arm lift, coordinated
release and 0.60-second upper-body settling. Original clips, roots, legs,
collision, 1.20-second charge, 0.30-second warning lead, 0.18-second light burst
and 1.80-second recovery are preserved. All 108 audited lower-body vertices stay
exactly fixed in three candidate skins. This is magical casting, not an invented
weapon swing. The original two-joint curve remains the normal compile-time choice.

Two hidden native RT runs each completed 279 presented/owned frames and 104
captures through the same public Keeper practice sequence: charge tick 98,
warning 152, discharge 170, neutral return 278. Damage was disabled and no player
attack/guard/dodge edge was issued. This is not a natural Keeper victory, phone,
balance, sustained performance or visible Windows UI acceptance.

The matched frames show a clearer lifted-arm anticipation and stronger release;
the [comparison clip](keeper-paired-real-speed.mp4) places original left and
candidate right, using motor-tick/60 durations. It is unpaced RT footage, not
scanout FPS. Representative [original charge](keeper-control-charge.png),
[candidate charge](keeper-candidate-charge.png) and
[candidate release](keeper-candidate-release.png) retain the actual scene.

Both used actual Diagnostic/High canonical shader pair, 960×540 internal/output,
DRS off and unchanged personal settings. The first wrapper's human tier label
mistakenly said Mobile; submitted pipeline/dimension evidence is authoritative.
No private shader bank was substituted. The second wrapper's aggregate custody
gate failed because foreground changed from handle 0 to another handle during
the hidden run. Readback confirmed settings, campaign bytes and all 151 parked
owner files unchanged. That failed gate is retained, not recast as a full pass.

Work counters prove **16 additional character pose evaluations and BLAS refit
record invocations** during recovery (50→66), processing 189,968 extra vertices
over this sequence. Whole-frame-cycle medians were 33.20→37.66 ms; whole-RT GPU
medians 8.652→8.694 ms. One pair, unpaced Debug capture, foreground activity and
unmeasured cross-project load do not establish a causal performance regression
or gain. The additional refresh work is real. The heavier candidate therefore
stays **unadmitted** rather than silently increasing the mobile workload.

Raw frames, owning evidence, build logs, exact executable hashes and the original
custody failure remain in ignored `reports/held-guard-20261025/`. Compact paired
summary is [keeper-paired-summary.json](keeper-paired-summary.json). Next motion
step needs an equally readable cast within a justified refresh budget; no further
pose tuning or renderer rewrite is included here.

## Validation and retained failures

The seven affected native CTests pass: combat foundation, Windows combat prompt,
character render slot, flask backend, vitality, codec and campaign restore.
The complete Android unit roster passes **458 tests, zero failures, six existing
skips**. All four native ABIs, Debug assembly and lint pass (3m38s). The
[APK receipt](readability-pause-apk-receipt.json) records exact native library
hashes and confirms no new packaged asset paths or source/save payloads. This
APK has not been installed. The original frozen69 retest APK remains unchanged.

The first pause fixture was unattached and did not observe its posted click;
draining callbacks alone did not fix it. Attaching it exposed a second fixture
problem: forced layout dimensions were not the actual Activity viewport. The
corrected fixture configures each real viewport and asserts stable root/button
bounds before and after callback drainage. It retains real touch dispatch,
negative drag-across ownership and positive tap/callback assertions, plus saved
opacity and pause/resume cancellation coverage. All three failed receipts are
retained in ignored reports; production input behavior was not changed to pass.

The original69 CI passed all six jobs. Hosted CI for this new source checkpoint
must be read at its own exact commit; those older results do not certify it.

## Touch dodge proposal

Swipe-dodge is design only; the Dodge button and current mechanics remain.
Downward left-thumb movement already controls backward walking. A speed/distance
threshold alone risks accidental dodges. An optional, explicitly armed prototype
alongside the button needs false-positive and multi-thumb evaluation, explicit
camera-relative direction, start-target ownership, UI/system-edge exclusions,
release/cancel handling and replayable animated teaching with a static
reduced-motion alternative. No gesture recognizer or replacement is enabled.
