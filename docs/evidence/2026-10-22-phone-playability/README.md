# Phone playability corrections - host checkpoint, 22 October 2026

Continues `3e29b599` in the existing WP2 worktree and draft PR 27. Owner's
landscape clipping/quiet-speech reports authorize these focused corrections.
This first checkpoint contains source/host evidence; phone presentation and
listening are **pending**, not inferred from Robolectric or CPU tests.

## Subtitle layout

Landscape touch placement uses the measured clear lane between actual side
control hit regions, including left/right system insets. Only obstacles which
intersect that lane constrain its height. The preference divider does not cut a
connected free area in half when neither half fits a complete line. User Top/
Bottom preferences, input-generation latching and system font size remain.

Captions and the 44 dp Skip target share one scroll owner for extreme text
sizes. Diagnostic notices no longer consume caption space or replace dialogue;
overflow is a developer log. The complete caption remains accessible. Normal
caption fit and touch non-overlap still require physical phone inspection.

Eight focused Java tests pass. The original extreme-caption failure is retained.
The landscape host case passed before the fix and does not reproduce the owner's
physical clipping by itself. The broader suite exposed a separate existing
closed-registry mismatch: native pine cameras 223-225 had no Android name/pose
mapping. Those six entries now match the unchanged native registry exactly;
the original failure and passing contract are retained. This is mapping evidence,
not physical pine-view or LOD acceptance. All **390 Java tests / 54 suites pass**
after these corrections, with zero skipped tests.

## Speech acoustic domain

Speech already consumes its own volume preference, independently of SFX. Current
Kit PCM peaks near full scale; increasing every bus or normalizing assets is not
the demonstrated remedy. The legacy dungeon XZ wall union was incorrectly used
for forest sources/listeners, applying 0.38 gain (-8.4 dB) in open space.

An explicit shared acoustic domain now selects the existing dungeon policy for
dungeon, unknown and legacy states, and known chapter exterior zones for outdoor
dialogue. Android publishes domain and frozen dialogue tuple under the same
mutex/generation guard; Windows uses the same immutable simulation publication.
Only dialogue opts in. There is no new outdoor acoustic-wall solver. Base gain
0.36, distance/pan/range, PCM, ducking, saved gains, frozen coordinates, playback
cursors and session behavior are unchanged. Actual source/listener tracking is a
separate remaining issue; no corrected listening/mix pass is claimed.

The focused Release spatial-audio target compiles and CTest passes. An initial
new range fixture failed because a nominal 14 m subtraction at the upper landing
is actually 13.999999046 m in float, legitimately inside range. Its failed log is
retained. Corrected coverage uses actual outdoor positions beyond 14 m and an
exactly representable 14 m boundary; runtime thresholds are unchanged. Indoor
attenuation, unknown/default guards, distance/pan, range and mute remain covered.

## Phone custody and ordering

Connected SM-S948B / Android 17 API 37 is verified. Owner settings currently are
33%, Mist Off, Speech 100%, SFX 20%, Music 55%, system font 1.7. Original saves/
settings are retained locally before any install. No production save is used by
the new isolated test namespace. The owner's supplied 29.9-second clip is kept
local and ignored; it covers tomb traversal, not Kit's outdoor captions. Its
brief dark view remains an investigation, not an asserted renderer cause.

The time-bound phone performance batch takes priority through 19:55 UTC. Its
exact build, measurements and physical interaction evidence will be recorded
separately. The standardized benchmark/leaderboard plan is not activated by this
host checkpoint, and no release or merge is authorized.
