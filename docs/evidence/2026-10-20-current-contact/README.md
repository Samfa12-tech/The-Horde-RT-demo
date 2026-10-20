# Current combat observation - local 1.7 milestone

Continue from ad12cdbd in the existing WP2 worktree/branch and draft PR27.
The milestone follows the planned current target/contact diagnostic, which was
blocking informed combat and earned-progression acceptance after the old trace
budget filled. No new content/feature direction or blind gameplay repair.

## Implemented

Both existing developer overlays now consume the same read-only immutable
simulation snapshot observation. It shows the selected active living target ID,
current player/target XZ, planar center distance and relative bearing (+right),
plus snapshot tick and paused state. The newest sword sample has its own target,
tick, sequence and simulation-tick age, separate from the current target. Later
damage events do not replace sword samples. Dead/dormant/nonfinite targets,
overlapping-center bearing and cleared/future/malformed history are unavailable.

The current observation does not depend on the bounded historical trace exporter.
A focused test exhausts that exporter, then changes the current target pose/tick
and confirms the overlay observation still updates. Ring-wrap/old guard versus
current Keeper tests prevent stale target attribution. Ordinary local 430-tick
Keeper reveal independently yields selected Keeper, 3.5 m center distance and
zero bearing error. Shared text explicitly labels centerXZ, sample age and
mesh gap N/A. A later no-contact sample is not a whole-cut miss verdict.

This is opt-in developer information, not normal game HUD, a targeting aid,
measured sword/mesh gap, hit-eligibility or obstruction test. No gameplay,
simulation ownership, input, saves, events, audio/haptics, renderer, quality or
asset changes. No added diagnostic IO and no replenished/unbounded log quota.
The old on-disk showcase debug JSON can still be stale; this milestone does not
turn it or earlier rows into current physical evidence.

## Offline arm correction completed separately

The host helper now owns the exact proposed command sequence: ASCII request ID
without newline through non-PTY shell stdin; verify echo and staged-file readback;
only then atomic request-scoped rename. Its injected runner lets offline tests
prove that empty/old/newline-changed/missing/error bytes never reach publication,
and failures do not retry. All 14 guard/transport tests pass. The prospective
ignored runner now calls that tested helper, was syntax-checked and remains
unexecuted on the phone.

The earlier actual arm file was empty and native armed=false/timedFrames=0.
Those facts and the previous stop remain unchanged. Offline tests do not prove
physical ADB transport, complete the comparison or justify another physical
attempt in this turn. No phone command, preparation, install or game launch was
performed. Keep the [inconclusive original trial](../2026-10-20-keeper-mist/renewed/README.md)
and its exact installed R04/custody evidence separate.

## Validation and boundary

[Receipt, exact code/binary identities and limits](receipt.json):
- Three focused Release host tests pass: observation, ordinary Keeper setup and
  shared overlay formatting. Boundary cases include ring wrap, selected guard
  identity, dead/dormant/nonfinite data, overlapping centers, yaw wrap,
  pause/value ownership, future samples, reset and exhausted exporter quota.
- Windows application builds. Android all four ABIs compile; 361 Java tests pass.
  The initial new host-test link omitted the existing shared simulation library;
  the corrected build passes. No assertion was weakened.
- New APK is compile-only. All 175 assets match installed R04; no shader change.
  No phone/Windows interaction, physical overlay legibility or live contact pass.
- The two previously classified stale native failures remain open; no full-suite
  green claim. No public push/release, owner asset changes or Reef operations.

Banked measured zero-mist exit and owner-selected mobile fresh/reset Mist Off
remain in the ongoing build. No new performance optimization is inferred from
the failed comparison. Audio/haptic manual revalidation required: NO, diagnostic
fields only, no event timing/routing/assets change.

Next bounded acceptance: inspect this current target observation on an explicitly
authorized exact build, retaining contact-sample age and distinguishing center
range from mesh contact, before any informed combat action. The prior denial
does not disappear because diagnostics were added. Natural earned autosave/manual
separation, physical death/load, full chapter and sustained-performance gates
remain open. This source milestone returns to the existing playable 1.7 roadmap;
it does not restart the measurement loop or add heavy outdoors content.
