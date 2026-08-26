# Task 5 Report — Skinned Player, Animation Layers, IK, and Real Bone Sockets

Date: 2026-08-26 (Australia/Sydney)

## Status

Task 5's implementation and host/build gates are complete. The feature worktree contains an original reusable PBR biped, shared fixed-step player animation and two-cut combat authority, real hand-bone IK/sockets, first-person primary-ray masking, a measured 60 Hz CPU-skin/dynamic-BLAS-refit route, and deterministic owner-feedback captures.

Task 5's final acceptance remains open for two explicit reasons:

1. The last exact-device run completed artifact parity, strict ASTC, honest RT presentation, matched A/B timing, four corrected captures, and Home/resume, but its runner reported a false failure because it compared the second staged checkpoint with an absolute process-wide attack sequence. The runner was corrected to compare checkpoint-relative edges, but `adb devices -l` exposed zero devices before that script-only revision could be rerun.
2. Owner subjective art/motion review and the now-required manual audio/haptic replay for the two-cut chain remain pending.

The mutually exclusive procedural route is therefore retained. The skinned route occupies TLAS slot 4, masks procedural slots 5–16, and preserves the 20-instance cap. No publish, signing, versioning, upload, or deployment occurred.

## Focused commits

- `2617d9c` — `refactor: generalize skinned character asset interface`
- `cf8c549` — `test: add player animation ik and socket contracts`
- `2c7a51b` — `assets: add validated gothic player runtime asset`
- `6ec7ab5` — `feat: bind skinned player rendering to bone sockets`
- `491564d` — `test: expose skinned player phone A B route`
- `9a5f1c3` — `test: capture Android runtime resource stability`
- `3c08e7d` — `fix: widen first person arm composition`
- `d8a393a` — `fix: refine player grip spacing and sword cant`
- `609e0be`, `8b43754`, `3f08a9a` — staged skinned-player evidence documentation
- `d081fd5` — `feat: add authoritative upward sword combo`
- `547eccd` — `fix: keep chained player cuts phone-safe`
- `b7b4ea2` — `test: validate relative combo capture edges`
- `cf20064` — `test: keep player socket fixture in rig space`

The final documentation commit is recorded in the handoff after this report is committed.

## Runtime architecture

- `SkinnedMeshAsset` is the reusable skinned-biped seam. Existing skeleton/lich output and the one-to-two skeleton pose-bucket behavior remain unchanged.
- Immutable `PlayerAnimationSnapshot` state advances only through the shared 60 Hz `GameSimulation` fixed step. It carries locomotion clip/blend/time, upper-body action/time, reaction, lantern blend, left/right IK targets, and visibility metadata. Reset/import/freeze and 30/60/120 render delivery are deterministic.
- `HeldItemKinematics` owns lateral shoulders, hand targets, wall retraction, torch drop, sword/parry pose, and combat arcs. `PlayerRenderSlot` transforms the complete shoulder-to-target chain into rig space, solves both two-bone chains, skins the mesh, and composes held items from final `LeftHand`/`RightHand` bone sockets.
- Socket error is measured after the final bone/socket composition. The bound remains 0.015 m; measured Windows cadence error was 0.000010 m and exact-phone state serialized 0.0000 m.
- Body, Head, and NearFace primitives have explicit metadata. Head/NearFace are excluded only from first-person primary rays and retain intended shadow/reflection participation.
- CPU skinning updates the bounded player vertex range and refits one player BLAS. Compute skinning was not added.

Combat hit/parry authority remains in `SwordCombat`/`GameSimulation`; no imported clip owns damage timing and no platform owns animation/gameplay state.

## Authoritative two-cut action

The first accepted attack edge starts the existing downward cut:

- windup 0.18 s;
- active 0.16 s, with one authoritative downward contact/hit pulse on entry;
- recovery 0.22 s when no chain is queued.

A second monotonic attack edge is accepted exactly once only while the first cut is `SwingActive`, from 0.00 through 0.16 s inclusive. It queues a continuous transition into:

- upward windup 0.10 s;
- upward active 0.18 s, with one separately identified upward contact/hit pulse on entry;
- upward recovery 0.24 s, then idle/cooldown completion.

The accepted `PlayerSwing` payload identifies downward/upward cuts as 1/2. A late second edge is consumed under the documented normal unavailable-command behavior and is not buffered into a later false action. Tests require two accepted edges to publish exactly two swing events and two hit events, transition angle discontinuity below 0.12 rad, no duplication across pause/resume, exact reset/import behavior, and equivalent commands/events/final phase under 30/60/120 render delivery.

Android and Windows continue to drain the same ordered shared `PlayerSwing` records. Inspection confirms each accepted record maps once to existing swing playback; Android also maps each record once to the established swing haptic. No new audio or haptic asset was added. A live exact-phone two-cut feedback replay could not be completed after the device disconnected, so manual revalidation remains open.

## Owner-feedback geometry contracts

The final first-person composition is bone/socket-driven rather than a detached prop translation:

- shoulders are authored at -0.36/+0.36 m, requiring at least 0.68 m separation;
- rest hands are -0.24/+0.25 m with at least 0.48 m separation and separate left/right elbow chains beyond -0.30/+0.30 m;
- torch remains left and sword right with torso space between the chains;
- the RightHand/Grip basis applies an 80-degree (1.3962634 rad) long-axis roll to the authored sword basis (+Y blade-long, +X sharpened edge, +Z flat normal), not to detached prop geometry;
- edge-forward projection is at least 0.94 at rest and at least 0.90 through pitch, wall retraction, and the sampled attack arc;
- the resting blade also retains restrained inward/forward cant.

The 75% portrait contract uses the audited torch Grip/Flame/Light positions and the production sword bounds relative to `Grip`, rather than hand points alone. At 1440:3120 portrait aspect, the final real-bounds extrema are:

| Pose | NDC x bounds | Contract |
| --- | ---: | --- |
| rest / owner feedback | `[-0.835022, +0.873479]` | inside `[-0.94, +0.94]` |
| downward cut | `[-0.835022, +0.712988]` | inside `[-0.94, +0.94]` |
| upward slice | `[-0.835022, +0.936234]` | inside `[-0.94, +0.94]` |

The contract samples the complete down-to-up arc at 25 points and requires the edge-forward basis throughout. Checkpoints 106/107/108 are `player-body-owner-feedback`, `player-body-downward-cut`, and `player-body-upward-slice`.

## Meshy generation and provenance

The authenticated balance was read without logging credentials or signed URLs: 805 credits before Task 5 and 735 after. Total spend was 70 credits. No third geometry candidate was generated.

- Candidate 1 preview `01a03c89-37b7-743a-940e-9b2c798152f4`, 25 credits, SHA-256 `3cd5ce523e43f700c97e16ea4779e2b4f2c9f7f3cd820ae1f46381b4e3c89c75`: rejected before texturing because long cloth bound the legs and the arm/hand silhouette was unsuitable.
- Candidate 2 preview `01a03c8c-1b40-733b-8a8b-0255f1384c22`, 25 credits, SHA-256 `34b65bd4a4a23b27f533be55f4f11f5176c388585400c0c765d1f4b1997f0e89`: accepted after multi-angle geometry/silhouette inspection.
- PBR refine `01a03c91-1472-765e-9a2f-d15370325975`, 10 credits, SHA-256 `5cad3beaea75d61e7c0dd25a20153819cb4951e46e44f883cbfb263ed7f54f64`: accepted after textured inspection; 2K request and `remove_lighting: true`.
- Topology-preserving remesh `01a03c9d-5296-7994-8fe4-e68ecc77c7ac`, 5 credits, SHA-256 `15331254e5b212cc29a1a83e2d19c261a5554a06ba0b3ab404eefdba3b34d1a6`: accepted.
- Rig `01a03ca2-a736-7808-b554-4f4193ec49f4`, 5 credits: rigged SHA-256 `083adf4f86716155fc25ee1bb2f2966b513ceac6b6f3178374218e85b2295e00`; walking source SHA-256 `1a8faa6667300da04e7088ee8e5bcae5506280fe259de33b6796a87f85a19d67`.

Exact prompts, settings, dates, costs, hashes, accept/reject decisions, and the conservative Meshy Free-plan CC BY 4.0 attribution route are recorded in `assets/models/player/source/meshy-2026-08-26-gothic-traveller-candidate-2/METADATA.md` and `ASSET_LICENSES.md`. Source/high GLBs are Git-LFS evidence. Only audited runtime files are packaged.

## Runtime asset contract

- Runtime GLB: `assets/models/player/runtime/gothic-traveller-lod0.runtime.glb`
- SHA-256: `58aac1ce6ec280e4abc6968f6ad8f61013b22ca20a169d1d5cd29e52c316c4ab`
- Size: 1,581,044 bytes
- Geometry: 15,595 triangles; 21,019 rendered unique vertices; 46,785 expanded indices
- Primitive semantics: Body 13,534 triangles; Head 1,813; NearFace 248
- Rig: 24 joints; exact `LeftHand`/`RightHand`; at most four influences; zero unweighted vertices
- Clips: Idle and Walking only
- Transform contract: +Y up, +Z forward, 1.8000003 m height, metre scale, grounded origin
- PBR contract: UVs/normals/tangents retained; no baked lighting; player is array layer 2 while sword remains 0 and torch 1; ASTC remains 6x6 base/ORM and 4x4 normal
- Khronos glTF Validator: zero errors and one reviewed `NODE_SKINNED_MESH_NON_ROOT` warning

Blender 5.2 performed the texture-before-rig restoration, coordinate/scale/origin normalization, semantic split, tangent generation, and idle/walk-only packaging through the same audited model contract.

## Windows measurements and captures

Three isolated Release benchmark samples before the final gate measured:

| Sample | 30 Hz ms/update | 60 Hz ms/update | 30 Hz ms/tick | 60 Hz ms/tick |
| --- | ---: | ---: | ---: | ---: |
| 1 | 0.325719 | 0.318726 | 0.162859 | 0.318726 |
| 2 | 0.331553 | 0.331425 | 0.165776 | 0.331425 |
| 3 | 0.313972 | 0.309590 | 0.156986 | 0.309590 |

Each run processed 150 vs 300 updates over 300 fixed ticks and 21,019 vertices. Both cadences retained 0.000010 m maximum socket error. The 30 Hz route differed from the 60 Hz reference by as much as 0.173031 m, so the runtime selects 60 Hz for first-person grip motion. CPU cost passed; compute skinning was unnecessary.

Fresh corrected Windows owner captures are under `.superpowers/sdd/2026-08-26-fire-pbr-reward-lantern-player-upgrade/evidence/task-5/windows-player-owner-combo-v2/`. All three are scene-only RT-storage-image captures with honest presentation and valid GPU RT timestamps:

- rest SHA-256 `5eb451efd3dc26d88f81531a387ce599a78f396ebe657b1747725238cd079b5a`, median 6.111750 ms;
- downward SHA-256 `69b9d2ac035458da36fd38466637a8727478b1d0e6f1b4962adda6ce11807516`, median 6.064400 ms;
- upward SHA-256 `fd8b6464f7f462f9b53e7455b9efc5ac85d9a9b5e7a2d16e4b8faf4cb24e6a65`, median 6.099400 ms.

Visual inspection confirms distinct lower-left/lower-right arm chains, torso space, torch left, thin edge-forward sword right, a readable downward diagonal, and an inward upward slice without head/camera intersection.

## Exact `SM-S948B` evidence

The final corrected exact-device run is `reports/android-showcase-runs/run-20260826-192607`:

- serial `R5GL219SZGK`; raw model `SM-S948B`; Android 16/API 36; Adreno 840;
- clean production source `547eccdd8d039fde9e3342a642662c40eca861ac`;
- local and installed Debug APK SHA-256 both `bceeb06ec90d003e848e78c576b83a9376548a5ad1e6c3f698b2c675cba87b96`;
- later commits `b7b4ea2` and `cf20064` change only the host runner and a host smoke-test fixture, so packaged inputs are unchanged;
- strict ASTC and honest RT presentation retained at 75%/Mobile/Authored, internal extent 1080x2235, swapchain 1440x2980, TLAS count 20;
- selected player cadence 60 Hz; benchmark state CPU pose/IK/skin average 3.1066 ms; maximum socket error 0.0000 m;
- fallback windows 36.736 / 45.699 / 36.638 ms, median 36.736 ms;
- skinned windows 42.131 / 42.176 / 42.199 ms, median 42.176 ms;
- matched median delta +14.81%, below but close to the 15% investigation threshold;
- timing rows recorded Android thermal status 0 and Samsung GPU thermal power level 0, with battery 34.0 to 35.3 C; the broader route ended at thermal status 1;
- Graphics 252096 -> 233604 KB, PSS 539029 -> 510628 KB, RSS 650440 -> 623856 KB, native heap 236052 -> 224088 KB, and process thread records 35 -> 33: no growth signature;
- Home/resume recreated the surface and honestly presented again.

Four 75% captures were preserved: matched procedural fallback, corrected rest, downward cut, and upward slice. Agent inspection matches the Windows composition findings and keeps all intended props/blade bounds on screen.

The staged shared checkpoint evidence recorded one consumed edge/one swing event for the downward capture, and two consumed edges/two swing events for the upward capture. Enemy-hit count is zero in these frozen opening views because no target is in range; shared host tests separately prove two in-range hit events. Checkpoint staging deliberately clears feedback before freezing.

The run summary is marked `FAIL` only because the old runner expected the upward checkpoint's absolute process-wide consumed sequence to equal 2 after the earlier downward checkpoint had already incremented it. `b7b4ea2` adds the non-tautological relative-edge comparison. The device disconnected before a clean rerun or live two-cut audio/haptic transport replay; the latest `adb devices -l` output is empty. A reverse-order warmed repeat is also unavailable, so the single +14.81% matched sample is reported honestly rather than generalized.

## Automated validation

Focused RED/GREEN coverage includes clip mapping, locomotion/combat precedence, down/up continuity, late-chain rejection, exact once swing/hit/event identity, pause/resume non-duplication, reset/import/freeze, lantern high/low continuity, IK reach/clamp, real bone sockets, pitch/wall basis stability, full-arc safe framing, primary-ray masking, route mutual exclusion, TLAS count, and 30/60/120 equivalence.

The initial fresh full run `run-20260826-193132` exposed a 1.195 m socket error only in `horde_rt_skinned_character_model_smoke`. Systematic instrumentation traced the complete target -> model/root -> joint local/global -> world -> socket chain. Production transforms both shoulder and target together and the phone measured zero error; the smoke fixture had converted the hand target to rig space while leaving the shoulder in view space after shoulder targeting was enabled. A RED relative shoulder-to-target regression preceded the single test-fixture correction. No production offset was added and the 0.015 m tolerance was not weakened.

The final coherent full Host run is `reports/foundation-runs/run-20260826-194410` at clean implementation head `cf20064801e9e87c3352151f888bc48cda80aa8b`:

- shader freshness and both negative safety gates: pass;
- fresh Windows Debug: 24/24 CTests;
- fresh Windows Release: 24/24 CTests;
- deterministic Windows RT captures: 13/13;
- clean Android all-ABI Debug and unsigned Release: pass;
- `lintRelease`: pass;
- runtime asset/package/licence and evidence-hash gates: pass;
- validation artifacts are explicitly unpublishable.

Raygen remained fresh: 494,096 bytes, 123,524 words, 27,665 instructions, 3,937 branch operations, 58 loops, and 1,581 selection merges. Compiled SPIR-V SHA-256 is `346f3ec6aa6f405979da97ee52164234438253704dc04bb51cd2c9f79913f3bb`; embedded include SHA-256 is `10f855345c6aecc9c3cdac12bc348ea24fa3f25f0f1f8dcd6a3c4f6bb04a8e8b`. No shader was changed by the final owner-feedback/combo corrections.

## Remaining acceptance boundaries

- Reconnect exactly one authorised `SM-S948B`, rebuild/install the unchanged production inputs with the corrected runner, and rerun the standard route/replay, relative staged edges, live two-cut platform feedback, Home/resume non-duplication, and a reverse-order warmed A/B sample.
- Owner must review the exact corrected phone rest/down/up art and motion. Automation and agent inspection do not establish subjective acceptance.
- Manual audio/haptic revalidation is required for the new accepted second-cut timing and identity.
- Retain the procedural fallback until those checks pass; the planned retirement commit is intentionally absent.
- CPU skin timing is direct; Vulkan frame/GPU timing includes BLAS refit, but no standalone GPU-refit timestamp was added.

Audio/haptic manual revalidation required: YES — the chained upward slice changes accepted attack timing and attack event identity/timing even though playback backends/assets are unchanged.
