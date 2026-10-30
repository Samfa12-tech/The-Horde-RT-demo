# Coherent held pose after zero-time geometry/viewport changes

The full-size forest's **baked control** exposed an existing paused-recreation defect before shared trees ran. With fresh renderer terrain temporarily detached, recovery solved the carry against the historical no-terrain fallback. Re-publication marked the geometry ready without refreshing hands/animation. The real recreated RT frame lost its modelled arms; exported vertices demonstrated a changed solved pose, rather than just absent GPU data.

Fresh accepted terrain bindings now refresh held kinematics, reanchor the lantern, and resolve animation/fire at zero delta. Repeated same-pointer/same-generation publications do no additional pose work. Existing stale/replacement rejection remains. The reanchor composes the existing angular state at the corrected hinge and rebases derivative origins; it does not integrate physics or reset swing angles/velocities.

A second reproduced omission affected the first viewport-aspect change: hands moved but the lantern body retained its old anchor, causing a five-centimetre body offset in the strict RT witness. `SetPresentationAspect` now uses the same reanchor/fire refresh, preserving its existing unchanged-aspect early return.

## Evidence and limits

The synthetic SafeExit regression tests this exact readiness/pose boundary without replaying the whole route. It is **not natural journey, victory, or earned-save acceptance**. Its assertions retain tick, position/camera, input publication and command-consumption counters, event count/order, world generation, swing state and fire phase. Repeated/stale/changed publications and repeated aspect are checked. Pendulum tests cover idempotence, invalid anchors and absence of a false next-tick impulse.

- Original runtime: both coherent-arm and lantern-body assertions failed; the combined Debug movement invocation also reached its unchanged 30-second deadline.
- Before the aspect correction: 21 checks, one body-anchor failure.
- Corrected registered rebind and pendulum CTests pass. The independent regression uses the existing executable with an explicit selector and its own unchanged 30-second CTest deadline. Every original default movement assertion remains in the default invocation.
- The unchanged default Debug movement test still timed out at 30.02 seconds after the new fixture was separated. That gate remains open; no deadline or assertion was relaxed. It is distinct from the previously resolved two-part Debug journey gate.
- The normal Release app compiles; all three relevant Release tests pass (movement 4.12 s, rebind 0.14 s, pendulum 1.28 s). The actual native RT control then passes identical pixels at tick 922 across recreation with arms visible and stable resources. This establishes neither phone acceptance nor Debug deadline completion.

Raw local logs remain in `reports/paused-terrain-rebind-20261024/`; the compact receipt preserves failures and final results. Native RT evidence belongs to the [full-size forest checkpoint](../2026-10-24-shared-forest-route/README.md). The comparison uses the existing initial paused-input barrier and then keeps the same frozen tick/pose while fresh world readiness rebinds normally; it does not restore stale readiness tokens.

No save schema, healing/recovery policy, gameplay tick, owner data, phone or foreground activation changed. This does not establish the cause of the reported phone black-world clip or close the separate tight-wall torch clearance/framing gap. Audio/haptic manual revalidation required: **NO**; no semantic event or audible/haptic route changed, and event ordering is asserted.
