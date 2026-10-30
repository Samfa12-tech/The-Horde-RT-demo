# One coupled Kit turn retiming comparison

24 October 2026. **Retain the 0.50-second diagnostic candidate; normal gameplay
stays OFF/unadmitted.** The same four-exchange departure is less ponderous,
without a newly observed snap/foot skip at the captured gameplay distance.
This is one comparison, not a tuning loop or final owner comfort acceptance.

| Fixed-step result | Original 0.75 s exchange | Candidate 0.50 s exchange |
| --- | ---: | ---: |
| Large departure to first accepted travel | 3.216667 s / 193 ticks | 2.216667 s / 133 ticks |
| Ordinary 20-degree partial, including entry | 0.950 s / 57 ticks | 0.700 s / 42 ticks |
| Native checks / failures | 696 / 0 | 515 / 0 |
| RT images / render calls | 161 / 168 | 123 / 130 |
| Bounded process duration | 37.812 s | 33.547 s |

The one-second departure improvement is an animation/motor response-time
observation, **not a frame-rate or rendering-performance gain**. Different image
counts explain why process durations must not be compared as performance.

## Single candidate and preserved contracts

The [original prototype](../2026-10-24-kit-shared-turn-start/README.md) used one
0.75-second wall-clock period per complete authored exchange. This candidate
plays that complete path at **1.5x**. A 0.50-second period is a restrained single
trial rather than a snap or a doubled-rate shortcut: average 45-degree turn speed
is 90 degrees/s while the existing spatial foot/body trajectory stays intact.
No additional rates were tried.

`CompanionTurnProfile.h` separates `playbackDuration=.5f` from unchanged authored
`duration=.75f`. Only the controller's normalized-phase clock accelerates. Both
joint sampling (phase * original clip duration) and the external world-yaw curve
consume that same phase. The whole exchange still completes before opposite
intent is consumed. Entry settle, start acceleration, accepted-distance Walk,
1.8 m/s ceiling, six-metre tether, route/collision, saves, original assets and
default gate stay unchanged. No new asset, worker adoption or architecture is
introduced.

The runtime input audit finds only executable identities and the profile header
differ between captures. All other recorded source/assets/settings/save hashes,
shader pair and actual dimensions match. Both cohorts reach the same natural
Idle clock before departure (240 ticks) and partial (600 ticks), rather than
seeding a yaw, pose time or completion flag. Original/control, initial departure
and initial partial RT images are RGB-byte-identical.

## Before/after at real gameplay timing

- [Large departure, side by side at 1x](departure-before-after-1x.mp4)
- [Ordinary partial, side by side at 1x](ordinary-partial-before-after-1x.mp4)
- [Elapsed departure sheet](elapsed-time-comparison.jpg)
- [Elapsed partial sheet](partial-elapsed-comparison.jpg)
- [Matched contact poses](matched-contact-poses.jpg)

These are original native RT crops at the captured three-metre gameplay
distance, without relighting or motion interpolation. Encoding reconstructs the
60 Hz shared fixed-step timeline, holding each actual (normally 30 Hz) RT sample.
It is not observed engine FPS or scanout. Each video has 0.5-second lead/end holds;
the shorter candidate panel is explicitly labelled **Captured endpoint held**
after its actual capture coverage ends. That labelled display hold is not a
gameplay freeze. Metadata is in [comparison.json](comparison.json).

I reviewed timestamped original frames and the matched-pose sheets; no foreground
player or interactive human test was used. On the large turn, the candidate has
finished its first exchange by 0.70 s where the original still has visible turning
to complete; successive supported exchanges finish sooner without changing
their endpoints. At 2.217 s the candidate begins aligned travel while the control
is still turning. The four-shuffle character remains visible, but the shorter
wait reads more purposefully. On the ordinary partial, the shared entry remains
visible through 0.20 s; weight transfer is further along at 0.40 and 0.50 s and returns
to Idle by 0.70 s rather than 0.95 s. No new apparent penetration, silhouette flip or
join pop is identified at this distance. The night lighting still limits subtle
contact and owner comfort judgments.

## Contact, interruption and lifecycle evidence

Nine shared active/end partial recipes at phase 0, 0.133, 0.267, 0.400, 0.533, 0.667,
0.800, 0.933 and 1.000 produce **identical full RGB RT images** in the two builds,
including boots, world yaw and shadows. This preserves the sampled spatial
poses; it is not a zero-slip threshold or proof of continuous terrain contact.
The original contact measurements/limitations remain in their prior receipts.
Faster motion necessarily changes velocity, so pose parity alone is not the
naturalness decision; the timed progression above is also reviewed.

Both complete native runs pass their unchanged 90-second/180-render guards,
mid-turn pause-image equality, single-step resume, latched opposite intent,
tether stop, slope and first-corner start, original-image reset equality and actor
release/EntryMenu retirement. No owner campaign write is attempted. Host checks
add both signed full/partial cycles: correct 30 active steps, complete original
clip endpoint, matched joint/yaw phase and a stationary supported root.
Existing interruption, inactive/readiness freeze, collision/tether, canonical
save/reset and untouched legacy behavior checks remain.

Focused Debug CTests: **3/3**, 24.69 s total (locomotion 4.75 s, dialogue 1.57 s,
campaign 18.34 s), unchanged deadlines. Locomotion reports 2,336 checks and campaign
checkpoints 1,058, all passing. The prior `018fdf9c` exact CI 40061788389 passed all
six jobs. The new checkpoint's exact CI is verified separately after backup.

Exact builds/source hashes, shader/dimension facts and custody are in
[receipt.json](receipt.json); [case timelines](case-timelines.json) retain the
shared snapshots and completed RT identities. GPU: RTX 5050 Laptop/NVIDIA 610.47;
hardware Pipeline, diagnostic High pair, 1232x803 internal/output, DRS off. All 151
parked owner files and original/source/candidate assets stay byte-identical.
No phone, foreground activation, merge, release or settings/schema change.

## Disposition and remaining gate

Keep this single faster timing in the **opt-in diagnostic** controller for
review. Keep the original 0.75-second evidence/build and full clips available.
Do not enable normal gameplay or declare owner comfort, perfect foot locking,
natural campaign, phone/performance or dynamic-worker acceptance. The 2.217-second
large wait and repeated-step style still merit owner pacing review; this result
does not authorize a succession of more aggressive rates, a wider tether or new
assets. Next is review of these two 1x comparisons and the remaining transition/
moving-target/platform acceptance, with a specific correction only if observed.

Local raw receipts/PNGs, two isolated settings/save copies, build/test logs and
custody remain under `reports/kit-turn-retiming-20261024/` and the two fresh
`reports/horde-campaign-fixtures-kit-retiming-{control,candidate}-20261024/`
folders. The same existing hidden controller flag is used; only an ordinary
partial capture and matched natural Idle preparation were added to that fixture.
