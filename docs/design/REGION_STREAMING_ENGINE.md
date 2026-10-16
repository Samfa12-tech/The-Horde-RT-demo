# Reusable region loading and streaming

Current [baseline GPU stage attribution](../evidence/2026-10-16-baseline-rt-profile/README.md)
retains the working ordinary cycle and13.59MiB saving/exact restoration on both
phone backends. All1,345frames join; exterior shader dispatch137–142ms dominates
AS-update0.53–0.55ms and transfer0.11–0.12ms. These are execution intervals,
not additive hardware counters or playable FPS. No renderer/quality/policy change.
Optional `hordeFrameStageProbe=true` requires the private Shipping/Mobile chapter
route-timing build; default builds compile it out. Analyze a complete benchmark
and its resources CSV with `tools/analyze_android_rt_stage_profile.py`; missing,
mismatched or final stage rows reject the run. Reuse prior experiment dispositions
before another replay. Smooth mobile play and shader-internal attribution remain open.

Latest attribution: [matched resident/streamed phone pairs](../evidence/2026-10-16-phone-resident-streamed/README.md)
complete four strict ordinary routes on one private Shipping/Mobile APK. Resident
and streamed fixed views have similar large baseline GPU durations (exterior
138–149ms); the measured preparation-step CPU peaks4.60/4.41ms. Both streamed
backends retain13.59MiB saving/exact restoration,12.85MiB observed peak above full,
zero readiness waits. This supports retaining the independent-owner design while
profiling baseline RT cost; it does not establish smooth gameplay, total loading
cost, a phone pressure budget or production adoption. All timing rows join exactly;
failed setup-ID evidence and its collection-start correction remain recorded.
The following earlier measurements remain artifact-specific history.

Latest measured slice: the [16 October phone cycles](../evidence/2026-10-16-phone-streaming-cycle/README.md)
close one ordinary two-way private page residency path on both real RT backends:
13.59MiB actual device-local saving, exact restoration,12.85MiB observed peak above
full, zero readiness waits. This is cached-source loading, not package I/O. Smooth
mobile play remains blocked (83ms median focused presentation intervals and frame
clock overruns); longer full routes hit their120s deadline before unloading.
Keep production unchanged pending matched mobile baseline/transition attribution,
broader lifecycle/pressure and coverage checks. No phone budget is inferred.

Current authority: the owner's14 October direction is the **engine loading system**,
using the existing chapter for validation. No new world art is requested. The world
should support continuous travel on connecting paths, persistent coarse terrain and
distant landmarks, preparation before arrival and reliable return to old regions.

The measured ordinary-owner checkpoint at47aaf662 is the starting implementation,
not the scalable architecture. It retires6.22 MB but temporarily allocates48.07 MB
above the warm scene while rebuilding the combined static owner. Those measured
costs justify independent region allocations; they do not justify unloading the
whole tomb or guessing a mobile memory budget.

## Chosen small architecture

Implemented at the engine-foundation checkpoint: the deterministic catalog/
scheduler, generation-bound acknowledgements, measured/reserved owner ledger,
dependency closure, return priority and quarantine; a compact base/detail CPU
partitioner preserving complete outside actor/held domains; and validated page
address-table construction. These are linked shared native CPU capabilities with
three focused Debug/Release fixtures. The renderer does not call the new page
path yet. Its existing ordinary combined-owner unload/return remains intact.

The next preparation slice supplies a native-source adapter and an independent
offside GPU owner. It uploads compact vertex/index/private-primitive buffers in
bounded copies, submits one immutable opaque BLAS build, polls its fence and
transfers ownership only on exact-generation completion. Known failures release
offside resources and require explicit retry; unknown submission/completion
retains every resource until external retirement proof. Pending cancellation
cannot free build inputs. The backend supplies its actual consumer stages, so
QueryCompute does not receive a pipeline-only shader-stage bit.

Five region fixtures now cover scheduler, partition, table, native imported
source and injected GPU preparation. The new GPU fixture exercises allocation,
mapping, addresses, AS/work failures, retry, cancellation and sole-owner transfer;
it creates no hardware device. The native adapter checks byte-identical imported
attributes/winding, shared material identity and complete held/player domains.
Its CPU allowance covers requested storage and admitted vector capacities; a
vector allocator may transiently over-allocate before rejection. It is not a
hard heap cap, and source/old output/heap bookkeeping remain separate.

This closes offside preparation coverage, not live page integration. Production
still uses the measured combined-owner path; no new savings or presentation
are claimed. The foundation at9d126583 passed all six exact-commit CI jobs in
[run38557046796](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38557046796),
including55 Vulkan-host checks and all seven requested rescue/traversal checks.

Private page-address SPIR-V feasibility passes all eight unchanged raygen budgets
and exact structural checks. Eight private QueryCompute counterparts also compile
and validate through the current generator. No production shader/catalog/include
is replaced. Actual page GPU adoption, retirement, normal scheduling/provider
connection, rendered parity and sustained measurements remain to implement.

- A data catalog names regions, bounds/connecting gates, coarse fallback, shared
  dependencies and their admitted source identities. Topo coordinates inform this
  spatial data; they do not determine memory costs or visibility by themselves.
- A deterministic renderer-owner scheduler has tunable preparation/retention
  distances, hysteresis, a measured region working-set target and a bounded amount
  of preparation/upload per turn. It prioritizes current/next traversal readiness.
  Unknown memory headroom is reported, not replaced by an invented phone budget.
- Coarse world and required distant landmarks stay resident. Detail can retire
  only when its validated contribution/fallback rules permit it for primary,
  reflected/transmitted and shadow rays. Distance or camera visibility alone is
  insufficient. Unknown contributors remain pinned. Existing quality stays fixed.
- Each static geometry page owns its compact vertex/index/primitive buffers and
  BLAS. Shared materials/textures have independent dependency ownership. Preparing
  or retiring one page does not replace the base world or the other pages.
- Immutable loader results carry world/source/request generations. One bounded
  producer queue handles I/O/CPU payload preparation; only the existing renderer
  owner adopts GPU work. Cancellation/stale results cannot publish readiness.
- A ready page enters the TLAS only after upload/AS completion and fresh frame
  metadata recording. Retirement first removes descriptor/TLAS references, proves
  graphics completion and invalidates obsolete command references, then frees the
  page. Collision/render readiness precedes traversal; failed preparation keeps a
  valid safe side and its already-present world.

The scheduler is independent of named tomb objects, tree species and game dialogue.
The first content adapter supplies the current proven forest-detail selection and
retained coarse/tomb base. That is a validation fixture for the engine capability,
not its hard-coded policy. Future towns/dungeons register data through the same API.

## Bounded RT address/slot path to verify

Production ModelledViewmodel leaves legacy limb TLAS lanes unused. A scene-profile
slot pool can lease only lanes whose actual owner/mask state is unused; it must
reject incompatible diagnostic rigs/fixtures rather than overwriting them. The
current26 metadata /28 TLAS /13 logical asset /32 primitive /32 material /18 texture
limits remain the target. The first independent page needs one free lane. More
resident pages are constrained by the actual free pool and measured allocation
headroom, not the number of catalog regions in the game.

The shared static vertex/index bindings currently prevent releasing individual
geometry ranges. The feasible prototype uses buffer-device-address geometry pages,
with128 bytes appended after the unchanged832-byte instance-metadata prefix.
Page0 names the retained base's storage; pages1–3 can name independent detail.
One uniform material/index/static-vertex address domain avoids branching between
old and new storage. Player body/viewmodel vertices retain their separate roles.
Page bits8–9 are in instance flags; `reserved1` retains existing actor semantics.
The CPU admission rejects unknown flag bits, overflowing/misaligned/truncated
addresses, inactive owners and wrong role/primitive spans before publication.
One common material/triangle lookup serves both RT backends. No new
descriptor binding, texture family, shadow query, sample reduction or capacity
increase is assumed. Actual scene slot/AS availability can impose a smaller
resident-page count than the table permits. A private primitive page observes the32-row
bound; shared material IDs refer to the existing table.

The Khronos [uvec2 buffer-reference extension](https://github.com/KhronosGroup/GLSL/blob/main/extensions/ext/GLSL_EXT_buffer_reference_uvec2.txt)
supports constructing addresses from two32-bit words without requiring shader
64-bit integer arithmetic. Compiler/SPIR-V validation must establish the actual
capabilities; the phone must establish real driver compatibility. Source wording
alone does not admit the new shader path.

The first branch-based attempt exceeded High Generic/Opaque byte budgets by
3024/3780 bytes. Uniform addressing and reused address handles removed executable
work; concise new record names removed serialized OpName overhead without
stripping diagnostics/code or changing compiler strategy. Final Generic byte
sizes exactly equal the frozen ceilings; Diagnostic High is295960 bytes,
17618 instructions,83 functions and2 query sites. High Opaque is586624 bytes,
32298 instructions,1 function and29 query sites. There is no Generic byte
headroom. These are compile facts, not driver cost or visual acceptance.

The CPU partition allowance counts actual returned vector capacities, provenance,
planning storage and worst remap scratch, separately from caller-owned input/
old output. Counting precedes result allocation. Driver allocations and real heap
bookkeeping still need the live owner/resource inventory; CPU geometry payload
bytes never stand in for device memory savings.

## Implementation and acceptance slices

The current [private live integration receipt](../evidence/2026-10-15-independent-region-pages/README.md)
closes the first cached-source hardware adoption/retirement/return slice on both
Windows RT backends: actual V/I/P/BLAS freeing, safe reload before return and
inspected matched rendering. It doubles scoped static savings and sharply reduces
temporary overlap versus rebuilding the combined owner. Production/Android
adoption and optimized Release pacing remain gated; package I/O is not inferred
from a retained CPU payload. The existing scene currently admits one detail page
within its unchanged instance/AS limits, despite four address-table slots.

1. Add generic catalog/scheduler, compact page payload/ownership and slot leases.
   Test generation/cancellation, shared dependencies, budget reservation, pressure
   handling, hysteresis and reliable reverse demand without hardware claims.
2. Compile the page-address path against the unchanged frozen shader budgets and
   structural checks. Admit one independent page in the current chapter; validate
   both actual RT backends, retained base, pose/equipment and all query families.
3. Connect normal location/contributor rules to bounded page preparation/adoption/
   retirement/reload. Record actual per-owner/heap allocations and overlap. A
   cached CPU source is labeled as such until package I/O/lifetime is proven.
4. Measure an optimized unchanged-quality out/back route, transition hitches,
   restoration, shadows/reflections/collision, and inspect matched full/resident
   versus retired/restored views. Validate the concrete candidate on the connected
   phone, including both backends, lifecycle and sustained pressure/thermal state.

Do not call a planner-only API, CPU batching or a masked TLAS swap the completed
loading system. At a pause, state the implemented slice, real savings/peak and the
exact missing admission/physical gates. Do not replace useful existing art or
inflate performance claims to close a gate.
