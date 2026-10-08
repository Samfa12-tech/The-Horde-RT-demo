# 1.6.2 isolated motion and actual-presentation evidence — 9 October 2026

The existing fixed-route benchmark advances its authored replay by one simulation step per rendered frame. Its actual image-presentation intervals remain valid for that declared workload, but it cannot establish ordinary movement/combat timing. Preserve the earlier 75% results and the [warm 50% observation](ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md) in their original scope.

Runtime `a6439e0b5998adefb7fdd87ea316132bc928549c` adds an explicitly admitted isolated Shipping/Mobile motion-validation path. Ordinary Debug/release keep minimum 50% and this feature Off. The isolated opt-in requires benchmark admission, actual `VK_GOOGLE_display_timing`, minimum33, non-Debug compilation and the unchanged Mobile shaders/assets; staged shader and viewmodel experiments are excluded. Existing menu controls expose 33/40 as experiments; no default is lowered.

## Authority and bounded evidence

The adapter reuses `MotionEvidenceScenario`, the current InputMailbox owner/admission lock and the normal `GameSimulation.AdvanceFrame` branch with raw monotonic input-owner delta and existing 60 Hz catch-up. Harness axes and timestamped command edges modify that one consumed input tuple; there is no second simulation, authoritative per-frame player-pose replay or fixed-render-delta override. Real UI/input intervention, changed profile/resources or suspension fail the run. One existing checkpoint seed is allowed; ordinary mechanics produce the subsequent poses/events. This does not measure human input latency.

Each state retains wall time, input publication/commands, tick, pose, semantic events, ticks per frame and catch-up overruns. Exact submitted/completed RT identities bind those states to the actual presentation collector. Settings include native `DispatchExtent()` traced dimensions and full water/fire/glass/shadow/cap/mist/dust tuple. Milestone storage-image readbacks retain their exact RT/state row and actual extent, and are streamed from the app-owned private directory into a fresh run-owned external export with bounded counts, lengths and paths.

Limits remain 30 seconds to arm, 120 seconds scenario wall time, 16,384 state/RT rows, 1,024 events, 64 readbacks, 32,768 timing rows and 256 pending presents. At terminal completion a maximum two-second owner poll yields every 10 ms, submits no new frames and never forces a dummy presentation. Missing timestamps, adverse counters, cancelled work or lost lifecycle ownership remain invalid. Existing failed-GPU-idle ownership is preserved.

The [separate strict offline analyzer](../tools/analyze_android_motion_present_timing.py) checks schema/admission, counter baseline and deltas, exact state/RT/presentation joins, image metadata, contiguous presentation/record/submission identities and increasing actual timestamps. Earlier startup rows are excluded; missing/interleaved rows, outstanding IDs and mixed resource scopes fail. It accepts only a single continuous scope for interval statistics: a retry scenario's retained multi-scope evidence does not become one unified pacing pass. It reports real inter-present intervals and preserves catch-up overruns. Readback and collection overhead are part of these short instrumented observations. Neither GPU reciprocal time, a median nor this harness closes sustained FPS or owner appearance/feel gates.

## Exact artifacts and checks

[Sanitized receipt](evidence/2026-10-09-motion-presentation/receipt.json) records tree `03781313c18939bd19b0fafab9462660564d6081` and both sealed packages:

- Isolated benchmark APK: `d305f4ab8d3d61f57620f6fcd2eb40db4fce4aa0b76a6c90e44cfbcfb91cff73` (121,553,614 bytes), non-debuggable, version `1.6.2-benchmark`.
- Ordinary Debug APK: `ccf1aeb2130881be658b2bd30680812959f11c91a01ff561d5530cd53195d222` (138,462,962 bytes), not installed at this checkpoint.

All four ABI builds and 16 KiB ELF/benchmark ZIP alignment pass. All 94 runtime assets match the accepted earlier package byte-for-byte. Android Debug and isolated benchmark each pass 249 tests in 38 suites, with zero failures/errors/skips; both lint checks pass. Three affected host suites (Android motion policy, shared motion scenario/ledger, presentation collector), 13 new offline fixtures, nine unchanged standard-route analyzer fixtures and the actual Gradle/CMake admission matrix pass.

Runtime CI is **12/12** aggregate success: [push 37791531274](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37791531274), [PR 37791538576](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37791538576). The earlier analyzer checkpoint separately passes 12/12. Initial fixture/build-admission failures and their corrected reruns are preserved in the private logs and receipt; none is a persistence failure or phone controller pass.

## Phone checkpoint and next gate

The intended SM-S948B / Android 16 is verified connected. Only the existing isolated package is updated; pulled installed-base SHA matches the seal. Primary gameplay settings and the accepted menu mix remain byte-identical. The isolated original 75%/Mobile/Glass On/Current/cap30/Mist On/Dust Off save survives install. Its temporary mobile-default 50%/Glass Off comparison save is applied through ordinary Use → exact native acknowledgement → enabled Keep. It must be restored to the original tuple at the comparison checkpoint.

First `mobile50-motion01` is refused **before app launch** because current HAL AP/BAT/SKIN temperatures are 38.2/34.6/36.2°C, above declared 31.5/29.5/32.5°C starting ceilings. Android framework status0 alone is insufficient. Cached sensor values are ignored; later starts require a fresh current-HAL sample, status0 and all three temperatures within 1.5°C of the first accepted cohort start. Those constraints bound initial comparability, not GPU clocks, throughout-run thermal parity, power or causal savings. Owned apps are stopped.

**Keep** the optional measurement slice; appearance/profile decisions remain pending. No accepted 50/40/33 motion cohort exists yet. Compare the same exact APK/backend/output/scenario and settings, then stop for the owner's quality feedback. Ordinary play, expensive Keeper/reward overlap, sustained thermals/display pacing, reliable power/memory interpretation, remaining physical/secondary-view checks, Eric's independent audit and final release approval stay open. Dust shafts, seamless music handover and closed renderer experiments remain deferred; no release, merge, signing or tag is performed.
