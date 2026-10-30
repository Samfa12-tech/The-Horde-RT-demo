# Restore the live torch measurement route after crouch

The supplemental native checks during Android Save/Load integration failed at
all four input rates: the live torch probe walked standing into the low opening
and stalled around `(0, -2.995)`. Those failed Debug/Release logs remain hashed in
the [integration receipt](../2026-10-18-android-campaign-integration/validation.json).
The collision was correct; the probe needed the newly supported crouch input.

The live probe now releases inherited held/toggled crouch using coherent command
floors, then queries shared authored standing clearance over a 30 cm lookahead
and holds crouch where required. Ordinary simulation movement, collision and
stance transition time produce the crossing. It stands again once clear and
reaches the unchanged real baseline/two-torch reveal. It does not import a pose,
encounter phase, light state or simulation time.

Both Debug and Release motion suites pass, including actual lowering greater
than 0.5 m beneath the opening and subsequent standing at 15/30/60/120 Hz. Both
Windows applications rebuild; Android Debug builds all four ABIs. The compact
[receipt](validation.json) records the failed baseline and successful outputs.
This repairs diagnostic input coverage; it is not a GPU timing or sustained
performance result. Existing exterior/two-torch performance gates remain open.

The later diagnostic APK `74811b2a…d973` was **not installed**. The accepted phone
Save/Load/restart evidence stays bound to source `6d4033d3` and installed APK
`5eb3282a…9128`, as recorded in the
[physical follow-up](../2026-10-18-android-campaign-readiness-followup/README.md).
