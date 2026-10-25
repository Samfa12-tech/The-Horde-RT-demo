# Preserve practice intent across inactive rule profiles

Exact `a2bc755ac99b604b8f7cb40e6f9eeac77f9d2ba7` failed
[40195872478](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40195872478):
GCC, Clang and MSVC each failed only `horde_rt_motion_evidence_scenario_tests`.
Android Debug/lint, Vulkan host and the selected ASan/UBSan contracts passed.
The unchanged motion test also reproduces locally at 15/30/60/120 Hz: the
`keeper-retry-reward` scenario dies during Fight, outside its intended initial
death/retry stage. Its original hosted and local failures remain retained.

The four-skill change passed `CurrentCombatRules() && tutorialEnabled` into one
enabled argument. The new deliberate-disable logic interpreted the legacy seed's
inactive rule profile as a human choosing to leave practice and retained Skipped.
Ordinary Retry returns to current rules, so the still-requested lesson unexpectedly
lost its protection. This was a runtime eligibility/user-intent conflation, not a
compiler failure, timeout or reason to weaken the motion gate.

The correction passes the raw user preference and a separate `rulesActive` value.
An inactive rule profile cancels only transient cues/protection/slowdown. It keeps
the requested preference, learned progress, stage and any explicit pending exit.
Actual user disable or Skip still uses the same active safe boundary; changing
eligibility cannot undo that choice or grant new immunity.

Regression checks use actual GameSimulation legacy import and ordinary Retry,
including an earned block followed by preserved Parry-stage progress. They also
check a real user disable, persistent Skip, pending Skip through inactivity, and
raw disable on reactivation. No flags are granted to make the motion scenario
pass. No scenario input schedule, assertion, deadline or workflow is changed.

All six affected native CTests pass: motion scenarios (11.93 s), combat foundation
(866 assertions, zero failures; 8.98 s), campaign checkpoints (17.78 s), combat
timing (6.01 s), Windows practice/prompt (1.86 s), and shared gameplay (9.80 s).
Every 15/30/60/120 Hz retry/reward run now reaches its existing terminal outcome.
The Windows application builds and all four Android Debug ABIs compile.

The motion evidence scope is important: its initial legacy-profile death is real;
its post-retry fight uses **default-enabled practice protection**. Its driver
approaches and attacks without defensive input. It is a bounded checkpoint-seeded
motion/retry/reward fixture, not unprotected combat balance, human victory,
natural journey or phone acceptance. This limitation also describes the prior
passing fixture; the correction does not create a new combat-success claim.

The corrected Debug APK is preserved privately, 206,293,867 bytes, SHA256
`6233d23b911063467e4bc0578864498c387e288e3ed658ce2177cb47b451012e`.
All 185 assets match a2bc755a. No Java or resource source changed in this fix;
the earlier 490-test/zero-error lint cohort is distinct from its C++-only local
assembly check. Replacement exact-head hosted CI is still required.
**No installation or phone access occurred.** The installed f27982cd remains.

New bridge/audio work was frozen for this correction. The one private authored
stand `.blend` was saved before that freeze, but has no reviewed motion preview,
native export or admission. No new runtime asset, ring, scream, bridge enablement,
renderer-capacity increase or revival blend is included. Owner files, saves and
settings are preserved. No owned local build/render/transfer remains.

Private raw evidence is under
`reports/combat-practice-20261025/ci-a2bc755a/`; do not replace or erase the original
failures. [Sanitized source and artifact receipt](receipt.json).
Audio/haptic manual revalidation remains **YES** for the overall new damage and
practice rules; this correction adds no sound/backend or haptic cue.
