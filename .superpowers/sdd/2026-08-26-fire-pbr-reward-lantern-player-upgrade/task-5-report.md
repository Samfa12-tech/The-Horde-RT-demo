# Task 5 Report — Skinned Player, Animation Layers, IK, and Real Bone Sockets

Date: 2026-08-26 (Australia/Sydney)

## Outcome

Task 5 implements and validates an original textured Gothic traveller/fighter, reusable skinned-player loading, fixed-step authoritative animation state, deterministic locomotion/combat layering, two-bone arm IK, actual `LeftHand`/`RightHand` bone sockets, primary-ray-only head masking, and a bounded 60 Hz CPU skin/refit route.

The procedural player has deliberately **not** been deleted. The exact-device controller ruling and the later owner-feedback correction require a mutually exclusive Debug A/B route until the owner accepts the final art and motion. The skinned route occupies TLAS slot 4, masks procedural slots 5–16, and retains the 20-instance TLAS cap. Normal Android play continues to default to the proven procedural route; `player-body-*` checkpoints select the skinned route and `player-fallback-*` checkpoints select the fallback.

Combat hit/parry authority remains in `SwordCombat`/`GameSimulation`. No imported attack clip controls damage timing, and no platform loop owns animation state.

## Focused commits

- `2617d9c` — `refactor: generalize skinned character asset interface`
- `cf8c549` — `test: add player animation ik and socket contracts`
- `2c7a51b` — `assets: add validated gothic player runtime asset`
- `6ec7ab5` — `feat: bind skinned player rendering to bone sockets`
- `491564d` — `test: expose skinned player phone A B route`
- `9a5f1c3` — `test: capture Android runtime resource stability`
- `3c08e7d` — `fix: widen first person arm composition`
- `d8a393a` — `fix: refine player grip spacing and sword cant`

The final two corrections address direct owner feedback that the first exact-phone candidate converged toward the sternum and held the sword too vertically. The fix moves actual arm-bone descendants to the authoritative lateral shoulders, rather than translating prop geometry independently.

## Runtime architecture

- `SkinnedMeshAsset` is the shared reusable skinned-biped contract. Existing enemy loading/output and one-to-two skeleton pose-bucket behavior remain unchanged.
- `PlayerAnimationSnapshot` is immutable shared simulation output. It carries locomotion clip/blend/time, gameplay-following combat action/time, reaction, lantern blend, two IK targets, and visibility flags.
- The animation state advances only through the 60 Hz simulation fixed step, supports exact reset/import, and converges under 30/60/120 render delivery.
- `HeldItemKinematics` owns shoulder, hand, wall-retraction, torch-drop, sword swing/parry, and rest-cant targets. `PlayerRenderSlot` transforms them into player model space, solves the rig, and resolves prop visuals from final bone transforms.
- The rig uses true `LeftHand` and `RightHand` nodes. Final socket error is measured after skinning, not inferred from intended targets.
- Body, Head, and NearFace primitives have explicit metadata. Head/NearFace are excluded from first-person primary rays while remaining eligible for intended shadow/reflection rays.
- CPU skinning updates the bounded player vertex subrange and refits one player BLAS. Compute skinning was not added.

## Owner-feedback regression contract

The final first-person composition has non-tautological tests requiring:

- at least 0.68 m shoulder separation (authored value 0.72 m);
- at least 0.82 m rest hand separation;
- left/right solved elbows remain beyond -0.30/+0.30 m, retaining separate upper-arm chains;
- the resting sword blade axis has an inward component of at least 0.12, forward component of at least 0.10, and remains principally upright.

The authored final rest is approximately 11.5 degrees inward and 8 degrees forward. `player-body-owner-feedback` (ID 106) preserves a stable first-person regression view. The final Windows capture is in `evidence/task-5/windows-player-body-owner-feedback-v3`; exact-phone evidence is in `reports/android-showcase-runs/run-20260826-180616`.

## Meshy generation and provenance

The authenticated balance was safely read without logging credentials or signed URLs: 805 credits before Task 5 and 735 after. Total Task 5 spend was 70 credits. No third geometry candidate was generated.

- Candidate 1 preview `01a03c89-37b7-743a-940e-9b2c798152f4`, 25 credits, SHA-256 `3cd5ce523e43f700c97e16ea4779e2b4f2c9f7f3cd820ae1f46381b4e3c89c75`: rejected before texture because long cloth bound the legs and the arm/hand silhouette was unsuitable.
- Candidate 2 preview `01a03c8c-1b40-733b-8a8b-0255f1384c22`, 25 credits, SHA-256 `34b65bd4a4a23b27f533be55f4f11f5176c388585400c0c765d1f4b1997f0e89`: accepted silhouette.
- PBR refine `01a03c91-1472-765e-9a2f-d15370325975`, 10 credits, SHA-256 `5cad3beaea75d61e7c0dd25a20153819cb4951e46e44f883cbfb263ed7f54f64`: accepted after texture review; 2K request and `remove_lighting: true`.
- Topology-preserving remesh `01a03c9d-5296-7994-8fe4-e68ecc77c7ac`, 5 credits, SHA-256 `15331254e5b212cc29a1a83e2d19c261a5554a06ba0b3ab404eefdba3b34d1a6`: accepted at 15,595 triangles.
- Rig `01a03ca2-a736-7808-b554-4f4193ec49f4`, 5 credits: rigged SHA-256 `083adf4f86716155fc25ee1bb2f2966b513ceac6b6f3178374218e85b2295e00`; walking source SHA-256 `1a8faa6667300da04e7088ee8e5bcae5506280fe259de33b6796a87f85a19d67`.

Exact prompts, settings, task dates, costs, hashes, accept/reject decisions, and conservative CC BY 4.0 licensing evidence are in `assets/models/player/source/meshy-2026-08-26-gothic-traveller-candidate-2/METADATA.md`. Source/high files are LFS-managed. No key, raw provider response, or signed URL is retained.

## Runtime asset contract

- Runtime: `assets/models/player/runtime/gothic-traveller-lod0.runtime.glb`
- SHA-256: `58aac1ce703405a2a452873c1d74fb72a9bd0fce1651ab3147c59be7f82a19f4`
- Size: 1,581,044 bytes
- 15,595 triangles; 21,019 rendered unique vertices; 46,785 expanded indices
- Three semantic primitives: Body 13,534 triangles; Head 1,813; NearFace 248
- 24 joints, exact `LeftHand`/`RightHand`, at most four influences, zero unweighted vertices
- Idle and Walking clips only
- +Y up, +Z forward, 1.8000003 m height, metre-scale origin/ground processing, UVs/normals/tangents retained
- glTF Validator: zero errors; one reviewed `NODE_SKINNED_MESH_NON_ROOT` warning

Blender 5.2 processed the accepted source and runtime through the audited asset contract. Geometry and textured renders were inspected before acceptance. Runtime textures append the player at array layer 2; sword remains layer 0 and torch layer 1. ASTC identity remains 6x6 base/ORM and 4x4 normal, with no baked lighting.

## Windows cadence, socket, and RT evidence

Release CPU skin benchmark after the owner-feedback correction:

```text
30 Hz: 150 updates, 0.350503 ms/update, 0.175251 ms/tick
60 Hz: 300 updates, 0.346062 ms/update, 0.346062 ms/tick
30 Hz max motion error vs 60 Hz: 0.173031 m
max socket error: 0.000010 m at both cadences
selected: 60 Hz
```

Thirty hertz is rejected because its measured motion delta is visually material for first-person grips. Sixty hertz is comfortably inside the accepted CPU cost and retains 10 micrometre measured socket error. The final owner-feedback Windows RT capture honestly presented at 6.116250 ms median with valid GPU RT timestamps; visual inspection confirms separate lower-corner forearms, torso space, left torch, right sword, and the restrained sword cant.

## Exact SM-S948B evidence

The standard pre-A/B exact-debug route passed in `reports/android-showcase-runs/run-20260826-173051`: six 75% timing views, 13-waypoint replay, 13 deterministic captures, strict ASTC, honest RT presentation, and Home/resume. That run used pre-toggle APK SHA-256 `290d0da14a19ca2b7faaf54ba026f2d7a3ee7adbea9fb8d8ffa1c146809a3955` at clean `6ec7ab5`.

The same final installed owner-feedback APK then passed the complete standard route in `reports/android-showcase-runs/run-20260826-181248` from clean documentation HEAD `8b43754`: exact local/installed SHA-256 remained `15ee8396b77fc1f6b90a15522ae486800a44201b7b5de2e054753885b220b6be`; the six ordered 75% medians were 33.851 / 33.654 / 26.477 / 31.152 / 29.454 / 44.052 ms for opening / two-enemy / worst-bend / skylight / green / lich; replay reached 13/13, all 13 captures completed, and Home/resume honestly presented again. Thermal status progressed 0/0/1/1/1/2 and Samsung GPU power was 0/0/1/1/1/0. These are descriptive sustained bands, not a weakened pass criterion.

The final owner-feedback A/B run is `reports/android-showcase-runs/run-20260826-180616`:

- exact device: serial `R5GL219SZGK`, model `SM-S948B`, Android 16/API 36, Adreno 840;
- clean source `d8a393a6224c5380b03d070bfa7c5b0b386d27e7`;
- local and installed APK SHA-256 both `15ee8396b77fc1f6b90a15522ae486800a44201b7b5de2e054753885b220b6be`;
- strict ASTC material selection and honest RT presentation retained;
- same 75%/Mobile/Authored workload and 1080x2235 internal extent;
- procedural median 33.879 ms; skinned median 37.874 ms; +11.79%, below the 15% matched-regression investigation threshold;
- Android thermal status 0 then 1; Samsung GPU thermal power level 0 for both;
- selected cadence 60 Hz, 20 TLAS instances, socket error serialized as 0.0000 m;
- Graphics 252096->233604 KB; PSS 519375->504434 KB; RSS 629656->617800 KB; process thread records 35->33; native heap 216780->219544 KB (+1.27%); no accumulating resource signature;
- three stable scene-only captures including `player-body-owner-feedback`, followed by successful Home/resume and renewed honest presentation.

Agent visual inspection of the final exact-phone capture confirms the objective owner-feedback correction. It does **not** manufacture owner subjective art/motion acceptance. The procedural route therefore remains available.

## Automated behavior coverage

Focused tests cover idle/walk mapping, locomotion/combat precedence, swing/parry timing continuity, lantern high/low continuity, two-bone reach/clamp, lateral shoulder/elbow/hand composition, sword axis cant, actual socket tolerance, reset/import, 30/60/120 equivalence, held-item attachment/detachment, torch drop continuity, wall-aware grip depth, mutual route masking, TLAS count, and head/near-face primary visibility.

Deterministic captures cover the downward body/grip view and owner-feedback composition. Existing shared simulation/capture suites retain walk, pitch, attack, parry, wall retraction, torch drop, body/shadow/reflection, and lifecycle coverage. Combat semantics and event timing were not changed.

## Boundaries and follow-up

- Owner subjective first-person art and motion acceptance is still required. Automated and agent visual inspection only establish the objective geometry/capture contracts.
- The procedural fallback must remain until that owner acceptance; this task does not perform the plan's originally proposed retirement commit.
- CPU skin timing is measured directly. Vulkan frame/GPU timing includes the BLAS refit route; no standalone GPU-refit timestamp was added.
- Compute skinning was unnecessary: 60 Hz CPU skinning passed measured Windows and exact-phone gates.
- No publish, upload, signing, version bump, or deployment occurred.

Audio/haptic manual revalidation required: NO — player rendering, animation, IK, and sockets change while combat event timing, listener pose/yaw, source coordinates, playback, transport, and haptic feedback remain unchanged.
