# Pre-fence preparation: bounded current-source feasibility

Read-only inspection at base02ece650; no runtime/renderer change or experiment.
This rechecks the retained23October Astra review against current Android/source
ownership, rather than treating its historical line numbers as current evidence.
The worker's phone gain is still unverified; normal worker stays OFF. The
interrupted e2 preflight produced no ACK or timing acceptance.

`android_probe_bridge.cpp` still has one frame in flight. `RenderFrame` starts
its evidence record, waits the owning graphics fence, completes prior ownership,
then consumes `gInputMailbox.ConsumeLatest()` and advances shared simulation.
`PresentableTinyRtScene::UpdateDynamicInstances` starts the existing exclusive Kit
job, prepares independent player work and joins Kit before reading its mutable
actor vertices and publishing upload/refit work. Job guards join on early returns;
resource teardown/reload resets the worker. Request tick/generation are checked.

The current worker calls `SkinnedPbrActor::Pose` on an exclusively owned mutable
actor with caches and CPU vertices. Its Result contains identity/status/work,
not a separate complete immutable vertex product. The joined actor state leads
into mapped buffer copies, BLAS refits, TLAS/instance/material publication and
command/query/output reuse. These operations cannot move wholesale before the
fence. No previous GPU owner may observe modified mapped memory or AS/scratch.

A feasible future seam would require a resource-independent CPU result owned by
one request, identified by input publication, simulation tick, asset/rig/route
generation and selected graphics scope. GPU upload/refit/command publication
stays after the existing fence; prepared, uploaded and submitted identities stay
separate. Failed preparation/submit/acquire, cancel, reset/load, surface loss,
asset replacement and shutdown must discard/retire safely without borrowing a
prior result or advancing evidence. Cached no-update poses need the same rules.

Input age is a material constraint. Sampling input/simulation before a long prior
GPU wait can make the next pose older even if CPU critical path shrinks. Reusing
an older snapshot while applying newer commands later would mix authorities.
Holding input until after the fence preserves freshness but leaves no known next
pose to precompute without an explicit speculative/discardable result and state
isolation. This is a small pipeline design, not a call relocation or a reason to
raise frames in flight. No latency tradeoff has been silently accepted.

Recommendation: first validate the current same-APK worker experiment under a
new allocated, matched comparison. Before any pre-fence prototype, specify the
pure CPU result/retirement and input-age contract with fault-injection/parity
checks, then make one opt-in bounded experiment. Do not move simulation, mapped
writes, BLAS/TLAS, scratch, descriptors, query reset or command reuse across the
fence. Measure critical-path/cadence/tails and input age, not worker time alone.
Cycle-minus-GPU remains a non-additive hypothesis/ceiling, not promised savings.
Sustained phone optimization is ongoing and does not reduce1.7 scope or create an
indefinite release-FPS veto. No broad AS rewrite is justified by this inspection.
