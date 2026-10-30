# Bounded active-light review and save-slice fallback

Inspected from `f3c494ee8b39c4f8503412ecd35361d927750a7a`, preserving that
diagnostic checkpoint on the existing PR27 branch and working folder:
`C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.7-wp2-vertical-proof`.
No rendering candidate, shader regeneration, APK build/install, phone input,
foreground launch, push, merge or publication occurred in this continuation.

## Hypothesis and disposition

Hypothesis: end the opaque-fast binary shadow query after the first confirmed
opaque blocker, retaining transparent filtering, masks and ray bounds. The
boolean result does not need the nearest blocker. This is attractive for the
measured added-light workload, but **the exact mechanism has already been
tested and rejected**. The [first-blocker receipt](../2026-09-30-first-blocker/README.md)
and [decision](../2026-09-30-first-blocker/DECISION.md) retain 7,352 owning phone
rows from A/B/B/A: no demonstrated gain, restored flags, and an explicit guard
against reintroduction. The descriptive +6.9% route contrast is thermally
confounded, not a causal regression estimate. Do not rebuild or tune it again
on source plausibility alone.

The [existing live reveal](../2026-10-17-keeper-torch-transition/README.md)
has Pipeline dispatch median 66.304 -> 104.590 ms and Query 67.056 -> 109.951 ms,
with AS below 0.5 ms and flat allocation/instance counts. It uses a private
stage-profile shader bank. The 6.33 cm arrival movement and co-activated reveal
prevent a light-only counterfactual. It establishes a useful scenario, not
surface-shadow versus mist/fire cost attribution or a current normal-bank
paired control. The [sparse clock composition](../2026-10-17-rt-dispatch-investigation/README.md)
is elapsed sampled-clock composition, not baseline GPU cost percentages.

Source review of `rt_lighting.glsl`, `rt_atmosphere.glsl` and `rt_fire.glsl`
found the existing diffuse/specular zero-contribution gate, inactive-emitter
gates, and exact-zero mist exit already in place. Area-light offsets/counts,
shadow masks, transparent ordering and finite ray bounds carry correctness
contracts. Distance culling has no exact cutoff in the current attenuation;
cross-sample visibility reuse would change the estimator. Those are not safe
equivalent substitutes. No new defensible implementation was identified in
this bounded review, and no new performance experiment was started.

The authorized fallback is the [concrete logical-save slice](../../design/HORDE_1_7_SAVE_SLICE.md).
This is a proposal grounded in the current owners, not an implemented save
feature. It does not make the unresolved rendering gate disappear.

## Traceable retained 18% result

The retained result belongs to `bd55878d1c51ec4b0a560ca34603dde7511560ef`:

- [Immutable method, results and caveats](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/bd55878d1c51ec4b0a560ca34603dde7511560ef/docs/evidence/2026-10-18-zero-mist-exit/README.md)
- [Immutable measurements, exact poses, thermal gates and raw-file hashes](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/bd55878d1c51ec4b0a560ca34603dde7511560ef/docs/evidence/2026-10-18-zero-mist-exit/measurements.json)
- [Immutable 26-pair pixel-parity and image identities](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/bd55878d1c51ec4b0a560ca34603dde7511560ef/docs/evidence/2026-10-18-zero-mist-exit/visual-parity.json)
- [Immutable excluded attempts](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/bd55878d1c51ec4b0a560ca34603dde7511560ef/docs/evidence/2026-10-18-zero-mist-exit/failed-attempts.json)

The same relative files exist under `docs/evidence/2026-10-18-zero-mist-exit/`
in this working folder. Raw APKs, images, logs and ledgers remain ignored under
`reports/zero-mist-performance-2026-10-18-r01/`; they are not regenerated here.
Pipeline exact cleared-room GPU median is 71.082 -> 57.754 / 57.716 ms
(18.75 / 18.80%); Query is 67.611 -> 55.709 / 55.727 ms (17.60 / 17.58%).
These are whole-RT GPU durations from six short normal Shipping/Mobile routes,
4,324 actual presentations, saved experimental 40% dimensions and CURRENT
shadows, optimized native Debug/O2. One control per backend is compared with
two trials; this is not full ABBA, sustained 30 FPS, an outdoor gain or a live
two-torch improvement. Exact shader/asset/quality budgets remain preserved.

## Evidence and later device decision

[inspection.json](inspection.json) pins inspected source blobs, preserved
receipts and the empty runtime diff. Only document/reference validation is
appropriate for this unchanged-runtime checkpoint; no new host or device
performance is claimed. The game-performance skill's comparability and bounded
iteration rules informed this review; no candidate/run or optimization goal was
created after the hypothesis failed the prior-evidence screen.

When a materially new candidate is supported, first establish its matched
normal-production-bank control at the actual reveal on both RT backends. Keep
dimensions, effects, shadow settings, probe, native build mode, thermal gate,
30/120/2-second limits and image protocol unchanged. Record dispatch and actual
display intervals separately, plus AS, memory, overruns and visual parity.
Retain only a repeatable useful result under the existing 3% criterion; do not
compare a new candidate directly with the historical private-bank absolute
times. That work needs a separately authorized device session. **No phone use
or installation is requested now because there is no new candidate.**
