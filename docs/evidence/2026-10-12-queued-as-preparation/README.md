# Queued AS preparation and submitted-pose ownership

Parent `8c7f84ca494b337b2942b6ce22fb9814071006e8` passed all six jobs in
[run38196590055](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38196590055).
MSVC137/137 includes the report focus/Unicode coverage; Vulkan45/45 includes
the seven requested traversal executions. The preceding MSVC failure remains
in the report-viewport evidence. This milestone's CI belongs to its new SHA.

Candidate collapse BLAS and TLAS builds now submit their own command/fence
leases. Later render turns poll completion; pending work retains all referenced
buffers, scratch and AS objects. Cancellation cannot free an in-flight build.
Unproved completion fails closed. Startup's existing synchronous admission is
preserved. Queue submission/polling introduces no queue-idle wait.

A reproduced stale-pose defect also affected transform-only TLAS UPDATEs:
the old cache advanced only for BUILD. Every successful frame submission now
publishes the exact staged definitions; unsubmitted recording cannot advance
them. The BUILD/UPDATE decision remains unchanged. A completed candidate can
refresh only its TLAS from that latest pose, without restarting CPU copying,
uploads or collapse BLAS construction. Stale publication still rejects.

Retained failing fixtures cover missing queued ownership, submitted poses,
TLAS refresh and uncertain device-lost submission. Final focused Debug3/3
(31.32s), Release3/3 (5.59s), four-ABI Android assemble/lint/Java (58s), exact
Android package/ASTC and held-item checks pass.
The new Vulkan host lease fixture covers allocation/record/submit failures,
pending polls, failed proof, completed retirement and idempotent cleanup.
These injected checks do not certify a device-loss recovery on real hardware.
The review reproduced an unsafe cleanup after device-lost submit; the correction
retains command/fence/AS/buffers with sticky unproved ownership until external
teardown proof. Only pre-submit failures and guaranteed-unaffected allocation
errors clean up immediately, following the
[Vulkan submit failure contract](https://docs.vulkan.org/refpages/latest/refpages/source/vkQueueSubmit.html).

Both actual native backend views were opened. Four replacement/restoration
comparisons have **zero changed pixels**. The receipt pins executable/APK,
source, shader, camera, quality, rope and light identities. Each of the four
frozen hops submitted three AS operations and observed three completed polls;
none happened to observe a pending poll. CPU mock coverage supplies that edge.

These captures are finite correctness checks, not sustained optimization proof.
Final preparation spans347 render turns (about2.11–4.08s); maximum advance is
7.10–8.81ms and cutover3.09–8.70ms on this Debug run. Allocations/reserves and
the final publication boundary can still hitch. Each owner allocates67,334,720
bytes; scratch-inclusive overlap is155,579,904 bytes. The44,904,764-byte CPU
source cache remains. Classifications can overlap on UMA.

**Normal-play residency saving is still zero.** Normal bidirectional scheduling,
safe exclusion sets, moving-frame publication, reverse recovery, full-route
inspection and sustained measurements remain required. Current primary and
glass-transmission rays reach10,000m; ordinary reflections/bounces reach12m,
sky shadows18m, and local lights have no hard influence cutoff. Distance alone
therefore cannot prove a tomb omission safe. Shared world families also span
interior/exterior geometry. Keep real contributors until stronger retention
proof exists; do not equate CPU staging or frozen owner exchange with streaming.
Phone CURRENT/HIGHER and saved LOWER migration, final chapter audio/haptics and
physical acceptance remain pending. Shader budgets, quality,18 layers and three
actor slots remain unchanged. Easy Tree/canopy and mist28 stay deferred.

`e0881ae2` failed its Vulkan host compilation: the new lease fixture used
`std::uintptr_t`/`std::uint32_t` without its own `<cstdint>` include. MSVC's
transitive headers hid this omission. The retained exact compiler log identifies
the cause; the follow-up adds the required include without changing assertions
or runtime code. Local focused lease coverage passes; fresh exact CI remains
required for the correction.

Final identities/measurements are in `final/candidate-receipt.json`; the earlier
receipt/captures remain historical evidence before the device-loss fix. Gallery:
[Pipeline](final/pipeline/cutover-01-selected.png),
[QueryCompute](final/query/cutover-01-selected.png).
