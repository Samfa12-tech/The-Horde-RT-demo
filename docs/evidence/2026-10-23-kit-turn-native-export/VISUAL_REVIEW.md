# Kit turn readiness review — 24 October 2026

Reviewed source: `9928d23b8d8c5d5f92ea576486557570c93f07b6`.
**Ready for an isolated RT trial; not ready for production integration.**
No additional authored correction is justified by the sampled views.

## Visual findings

[Left preview](left-cleanup-preview.mp4) · [right preview](right-cleanup-preview.mp4).
Reviewed their original full-body sequence frames 7, 24 and 40 in both directions,
plus the [left contact exchange](foot-left-0024.png) and
[right support view](foot-right-0035.png). Review is of decoded/rendered frame
sequences, not a claim of owner video playback or in-game feel acceptance.
Preview file hashes still match the committed [receipt](receipt.json).

Both directions read as two deliberate steps with a modest pelvis settle and
weight transfer. The swing foot clears the floor and the supporting boot remains
visually close to its planted position. There is no obvious knee inversion or
large body translation in these sampled views. Small sole/boot deformation
remains near the exchange; the recorded 6.856 mm maximum is evidence, not a newly
approved contact tolerance. These views do not justify another numerical cleanup.

The upper body uses the frozen Idle-zero base. Natural speech/recognition overlays,
arbitrary Idle phase, repeated turns and final 0.75-second pacing still need their
actual game context. Neither those nor a real Turn-to-Walk join is represented
by a standalone studio take. Original clips/rig and production behavior remain
unchanged; the earlier native import/single-yaw proof remains valid in its stated
CPU scope.

## Why the current hidden RT path cannot close the next gate

The supported nonactivating hidden route capture exists, but it cannot currently
select this candidate:

- `PresentableTinyRtScene.cpp:2782–2788` loads the literal original Kit runtime
  asset and only `Warden|idle` / `Warden|walk`; the remaining clip slots are empty.
- Its serial pose preparation at `:7013–7025` always samples Idle with Walking as
  the blend. Optional-worker preparation at `:6631–6642` makes the same selection.
- `windows_campaign_validation.inl:267–290` can override an immutable companion
  pose/camera, but that pose has only existing clocks, gait and acting fields.
  `CompanionSnapshot` has no authored turn selector or turn-phase lifecycle state.

Therefore running the unchanged fixture would still render the original clips.
It would not establish candidate RT joins, mid-turn pause/resume, cancellation or
retirement. No fresh candidate RT capture or lifecycle pass is claimed here.
This is a **missing validation capability**, not an access/approval denial or a
phone-dependent blocker. Owner review alone cannot supply the absent RT evidence.

## Smallest next validation slice

Add one explicitly Debug-only candidate asset/clip/time inspection adapter to
the existing hidden read-only fixture, with default production loading untouched.
Include inspection selection/phase in both serial and optional-worker pose-cache
invalidation; apply the already verified asset-space yaw once. Preserve original
actor restoration and clear every override before the fixture's Resume/retirement.
Avoid asset-name substitutions or relabeling original-clip captures as candidate
evidence.

Then exercise Idle (including nonzero phase) -> left/right turn -> Walking,
repeated/opposite turns and frozen mid-turn pause/resume/clear/retirement through
actual completed RT frames. Distinguish diagnostic playback lifecycle from future
shared gameplay turn-state acceptance. Keep the candidate unadmitted until those
results and contact/feel review support it. This review does not implement that
adapter, alter production bindings, or add another capture framework.

All 151 parked owner-file hashes and preview identities were checked unchanged.
No build, unchanged fixture rerun, phone operation or foreground activation was
needed for this documentation/source-path assessment. Audio/haptic manual
revalidation required: **NO** for the review itself.
