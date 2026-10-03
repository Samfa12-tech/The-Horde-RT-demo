# Opening-room lighting investigation — not an optimisation candidate

September 30, 2026. These local SM-S948B measurements follow the
[frozen Shipping ABBA and GPU-stage evidence](../2026-09-30-shipping-phone/README.md).
They do not close glass correctness, visual acceptance, sustained performance,
backend parity, or release gates. No isolate source is active on the engineering
branch, and no accepted player presentation was changed.

## Exact experiment boundary

Control C: immutable export-only Shipping/Mobile APK
`ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae`,
sourceeafbf82; its actual shader bytes equal frozen B. Temporary detached source
7a095c7 plus the retained three-file patch and mode header produces:

- S, APK `23aa0cf67f5f0612290f8607078d47e7e47bdf81202d9290ab15cc796dd73fea`:
  omit only fire-emitter visibility queries inside fireEmitterDirectLighting,
  retaining BRDF, emitter authority and all other light visibility. This makes
  fire direct lighting deliberately unoccluded: **nonphysical, never ship it**.
- V, APK `e063e7cfaef964f8b023cb318f86620597f3b4aeaaab40ad7899bec951e7c749`:
  omit only integrateFireEmitters volume/ember composition on its callers.
  This removes a required visual feature: **never ship it**.

Only Shipping/Mobile is gated; no geometry, material, ray-mask, player animation/
grip, transport-budget, resolution or gameplay change. Both use the normal
whole warm-up lap and full measured route, unchanged owning timestamps and
strategy export—not the query/logging stage probe. DeviceSM-S948B/R5GL219SZGK,
Android16/Adreno840/driver2150932499, native RayTracingPipeline, strictASTC,
MAILBOX,75%/1080x2235 internal/1440x2980 presentation. Only the authorised
development-signed `.benchmark` package is installed/pulled back byte-identically;
no data clear, force-stop, private/Home/video capture, stable-app mutation or
production signing/publication. Final control installation restores normal shaders.

## Measurements and cooling boundary

Values below are opening-zone median/p95 milliseconds. GPU p95 is nearest-rank
from160 completion-owned rows; native cycle is render entry through present
return, **not display pacing or live FPS**.

| Run | Native cycle median/p95 | GPU command median/p95 | Post-launch battery C | Thermal / GPU power levels |
| --- | --- | --- | --- | --- |
| C1 | 77.860/85.771 | 66.670/73.673 | 17.3→25.9 | 0 / 0 |
| S1, fire shadow isolate | 64.131/69.941 | 52.867/57.493 | 20.7→27.8 | 0 / 0 |
| V1, fire volume isolate | 71.741/79.492 | 63.364/70.780 | 25.3→31.8 | 0 / 0–1 |
| V2, same volume artifact | 74.868/83.004 | 63.484/70.992 | 31.9→33.5 | 0 / 0,1,3 |
| C2, uncooled after report | 77.721/85.632 | 66.423/73.667 | 28.5→34.3 | 0 / 0–1 |

Initial plan C1,S1,V1,V2,S2,C2 was stopped afterV2 when the owner reported the
ice brick had thawed and was removed. Report observed09:11:04UTC; actual removal/
thaw timeline was not timestamped. V2was already complete before receipt (last
retained context09:08:23), but that is not proof of cooling effectiveness through
its lap. Preserve this uncertainty. S2is **not run**. C2is a new uncooled-after-
removal control, not a cooled matched member or steady-state soak. Do not pool
these conditions or relabel the planned six as a completed matched sequence.
Context sampling starts after launch and is not frame-aligned; actual GPU clocks
remain unavailable. Different power states, cooldown/thaw and changed shader
register pressure/layout/occupancy prohibit a causal gain or exact ray-cost claim.

All five full ledgers PASS:9,190/9,190 completed/presented/valid GPU rows, zero
failure/rejection/cancellation, all seven identity fields join. Every route uses
OpaqueFast in1,838/1,838 rows, including all160 opening rows per run. None of the
800 opening GPU intervals is within33.333ms. Whole-route native medians are
C1 58.8834,S1 53.5834,V1 54.0813,V2 57.1172,C2 61.0903ms; these are not ordinary
display FPS, a pooled gain or a30FPS pass. Final exact control install/pullback
was verified beforeC2; automation completes normally without a forced app stop.

## What this demonstrates—and what it does not

The previous stage probe separates GPU AS/update cost (~0.50ms median) from
opening dispatch (~65.63ms). These new counterfactuals support investigating fire
visibility work ahead of fire-volume removal: the S1 opening GPU interval is
lower, while the two V intervals remain about63.4ms. **Torch lighting is not
proven to explain the entire ordinary-gameplay problem.** Dispatch still includes
other shadows, reflections, material work and player-geometry traversal.
No percentage is accepted as a real-renderer optimisation gain. Do not subtract
or add these medians as if additive engine counters.

Neither test intentionally deleting rendering work reaches the33.333ms GPU
budget. The current ordinary/lantern-heavy30FPS minimum remains an engineering
target, not a demonstrated achievable result. Keep actual display pacing,
uncooled sustained warm runs and honest30/50/60FPS bands separate.

Next bounded quality-preserving candidate: visibilityMask returns only whether
any admitted opaque blocker exists, but currently completes nearest-hit traversal.
Stopping at its first **confirmed opaque** hit can preserve the existing binary
answer while retaining transparent rejection, masks and ray bounds. The
[Vulkan traversal specification](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html)
supports this use of TerminateOnFirstHit. This is a reviewed candidate, **not
implemented or measured here**. Do not apply it to nearest-hit or ordered
dielectric shadow transport: those require distances/interfaces/attenuation.
Require focused shader contracts, compilation/disassembly, unchanged affected
native RT images and matched normal Shipping timings before accepting it.

## Build and evidence integrity

Both ARM64 benchmark builds PASS (S30s after regenerating the stale generated
pipeline adapter; V14s). Original rejected command/build logs are retained,
not rewritten as success. All16 generated modules per isolate compile/validate;
actual four packaged modules independently validate/disassemble and have0atomic
instructions,0OpImageRead and noBinding22. OpaqueFast pipeline footprint:
normal125253words/23static query sites; S122773words/14sites;
V117908words/23sites. Static sites are not dynamic query counts.
Both asset receipts compare53payloads:50byte-identical, twoJSON formatting-only
with semantic equality, licenceMarkdown line content identical. No asset/licence
or anatomical change is hidden by the probe.

The derived lighting parser keeps full admission checks and binds actual loaded
shader hashes to catalog keys, packaged module hashes and exact APK receipts,
not a module-size heuristic. Fresh control positive test and4/4 negative tamper
cases PASS (forged APK, shader, completion identity, opening strategy). Raw reports
are never edited; retained gzip round trips reproduce original hashes/lengths.
Scripts, exact installation receipts, source-patch/header hashes, catalogs,
build/containment checks, complete thermal samples and raw ledgers are retained.
APK/ELF/SPIR-V binaries remain external/local. SHA256SUMS.json records the allowlist.

Current7a095c7 push36690246101/PR36690251617 both pass45portable+11Vulkan-host,
logs freshly inspected. These are host coverage, not WindowsRTX visual acceptance,
phone compute, exactS24/S25 or final candidate acceptance. Phase4 interface/recovery,
shadow attenuation/live/changed-image/backend gates remain open. Deferred FPS
counter, music/consent reporting/resource/pacing and final matrix remain in scope.
Audio/haptic manual revalidation required:NO (no semantic/playback changes).
