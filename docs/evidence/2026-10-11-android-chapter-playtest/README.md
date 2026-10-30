# Android owner chapter playtest and live-start correction

Owner-operated test of source `2d72d978e181e11e2f9b547259c6eb0f756ec1a8`,
APK `eb50a06a1bfab9612f2a40fe87390c7e36567388cf540d16338417ad6944ba0e`,
Samsung SM-S948B. Actual Pipeline Diagnostic/Mobile, saved 50% render scale.
The preceding [startup receipt](../2026-10-11-android-atlas-startup/README.md)
establishes installed identity and bounded RT presentation only. This longer
playtest does not pass chapter acceptance.

## Owner observations, distinct from technical attribution

- No dialogue/subtitles, combat teaching or slowdown observed.
- Mist remained after Keeper death; the Keeper body also remained.
- "The Dawn Returns" appeared after lantern selection.
- Keeper fight frame rate felt very poor. The owner's two-light suggestion is
  a hypothesis, not an attributed performance result.
- Touch controls were very busy. The paused menu capture also shows its top
  content clipped; compact control/layout work remains open.
- Outside looked pretty good and tree shadows were visible, but no moon was
  seen. Ground needs to extend much farther without visible drop-offs, and the
  forest needs substantially more trees with collision to guide the route.
- Subsequently requested restrained bevels at tomb wall joins/door surrounds.
  Preserve accepted niches, clearances and the accepted tomb's layout.

The app remained paused. With foreground verified, the agent captured its own
screen and read app-only diagnostics, then closed only the Debug app as the
owner authorized. No data clearing, release-app changes or automated gameplay
input. Raw logs/screenshots remain private. The saved debug-state JSON still
described the opening checkpoint; it is not later Keeper/paused-state evidence.
Aggregate GPU timing and paused frame time cannot attribute the combat slowdown.

## Reproduced cause and bounded correction

Android's noncapture `rescue-journey-start` imported `ApplyShowcaseCheckpoint(0)`.
That deliberately enables historical capture rules: the current tutorial and
dialogue director are bypassed, and the Keeper retains legacy mist presentation.
The renderer could still draw the development chapter, hiding this gameplay
policy mismatch. Independently, Android published the raw legacy ending phase
instead of respecting chapter continuation.

An explicit owner-thread live journey start now resets ordinary gameplay and
enters rescue mode. Android uses it only for noncapture journey startup; frozen
imports retain their historical rules. Live startup no longer starts benchmark
sampling or its initial damage suppression. A snapshot presentation accessor
suppresses the legacy ending for the rescue chapter while preserving its raw
progression data and the ordinary demo's ending.

Preserved failing tests reproduce disabled current teaching/readable cast and
the absent grate line. The correction enables both; actual native simulation at
a walkable point near the grate publishes its dialogue. Negative tests retain
historical capture behavior and the ordinary demo ending. A first test position
was the grate source inside a wall and correctly reset to spawn; the regression
now explicitly checks its valid player position before testing the trigger.

This fixes the identified startup policy, not every observation. Keeper corpse
disappearance/Kit ownership remains unfinished. Optional slowdown still obeys
the saved preference (default off on Android); no preference was silently
changed. Actual dialogue playback, teaching feel and death-mist clearing require
fresh corrected-candidate phone review.

## Validation and candidate

- Windows Debug: six affected tests PASS, 27.30 s. Release: six PASS, 6.48 s.
  Selection: chapter dialogue, combat foundation, combat timing trace, water
  contact, rescue journey and authored-terrain journey. Existing foundation tests
  include authoritative death-mist timing, three-hit cancellation/reward ordering,
  slowdown lifecycle and legacy capture preservation.
- Debug/Release executables built. Android Debug build/unit/lint PASS, 59 tasks;
  283 Java tests, zero failures/errors/skips. Actual APK package/ASTC check PASS.
- Parent `2d72d978` passed all six CI jobs in
  [run 38111322941](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38111322941).
  The correction's exact CI is checked separately after push.
- [Exact source-file and artifact hashes](receipt.json). New Debug APK:
  `cc53485d6170dc97fe41c8641c142fc35f49cf0f0469325561bd3a83bc70c065`.
  It has not been installed or physically accepted.

Continue the existing chapter goal: attribution/profiling, restrained touch UI,
bevels, broader land and dense colliding woodland, visible moon with consistent
RT direction, graveyard/lid/ground anchor, visible Kit and real GPU zone lifetimes.
No new texture-layer ceiling, shader budget, quality reduction or release.
