# Regional water foundation: host evidence

Host geometry, proximity, GPU profile and corridor-omission checks pass. This receipt does **not** accept RT presentation, water appearance, game performance, device behaviour or listening. Root will append final shader/build/capture evidence separately.

[host-validation.json](host-validation.json) records source SHA-256 identities and exact scope. The source snapshot contains uncommitted additions; its recorded HEAD is context, not a final build identity.

## Shared geometry and ABI

The original shared definition follows the pinned [atlas](../../references/horde-1.7/d37f1638/docs/design/world/dungeons/baseline/shared-coordinate-layout.json), SHA-256 `0a4da29bf8d988145c0f7b3f1c7afaf415a5da9b14c34bbd53bd1e00e3a86e59`, with `X=East+78.3, Y=Up-31.55, Z=126.2-North`.

- Fixed 42 triangles: 14 concave lake, 24 main-river/headwater and 4 outflow. Lake area is 36,274 m² at atlas Up 0 / engine Y -31.55 m. River/outflow coverage now extends 5 m to each side with shared miter vertices, under the actual banks. This is finite coverage, **not a uniform 10 m wetted-channel claim**; opaque terrain intersections determine the shore. Water datums remain unchanged.
- Proximity measures Euclidean distance to the actual finite 3D faces and edges. It rejects invalid inputs and preserves the dry lake notch. It does not fabricate ponds, treat tomb runoff as regional water, or enable frog playback.
- Binding 27 keeps its original 16-byte ripple prefix, adds 16-byte metadata and 42 × 112-byte profiles: **4,736 requested SSBO bytes, an increase of 4,720 bytes**. Frame updates touch only the prefix. Profiles keep exact positions/normals, shared downstream phases and authored 0.45 m/s phase speed; this is not fluid simulation.
- The appended geometry requests **1,512 vertex bytes + 504 index bytes + 168 surface-code bytes** for 42 triangles. These are payload counts. **Actual BLAS/allocation cost remains pending capture** and must not be equated with geometry bytes.

The [geometry/profile fixture](../../../tests/ChapterRegionalWaterTests.cpp) also compares deterministic generated GLSL bounds with the shared definition. C++/GLSL record offsets are 0/16/32 for prefix/metadata/array, with a 112-byte profile stride.

## Host validation and closure

Final focused CTest selection passes **3/3 Debug (15.24 s)** and **3/3 Release (3.96 s)**. The standalone geometry/profile fixture reports **3,047 checks, zero failures**. Test elapsed times are not game performance measurements. Commands, source/executable hashes and local build/CTest log identities are in the JSON. This selection includes the geometry/profile, actual forest registry and frame-adapter fixtures.

The [actual forest registry fixture](../../../tests/ForestDressingTests.cpp) verifies all **84,219** selected omission triangles retain ExteriorDressing classification and bit-identical packed/source coordinates. The [additional closure](../../../src/scene/CorridorRegionalWaterClosure.h) requires the existing source-prism certificate, exact 42-face order/float identity and continuous per-face AABB separation strictly greater than **30.04 m**, with downward-rounded lower-bound arithmetic. This encloses the 12 m water continuation plus 18 m ordinary secondary moon query and origin guards.

Root's renderer now passes faces extracted from the actual appended vertex/index inputs and checks their surface codes. Existing owner/generation, retained opaque-face, source-prism, identity/mask and cadence guards remain required. The ten existing opaque witness triangles and original 2,036 immutable triangle identities remain unchanged. This is not a proof that all water is invisible; it does not disable the omission optimisation or invent a south-wall certificate.

A read-only integration review found no actionable mismatch in downstream phase direction, plane normals, SSBO layout/prefix writes, generated NoOpaque bounds or actual-face closure handoff. That review is not a driver or capture pass.

## Coherent render-only survey frame

The first hidden Windows survey reached scene admission, then rejected its first frame: the camera-only override left the torch at its old world pose, producing a **196.034180 m / 1.570796 rad left Grip mismatch**. The [failed manifest](../../../reports/regional-water-runtime-b1/environment-survey.json) remains a failure, with no accepted frame or PNG; job-lifetime termination is not its demonstrated cause.

The new [shared helper](../../../src/vulkan/raytracing/EnvironmentSurveyFrame.h) rebases attached player item/light/fire transforms and history together. Hand-held items follow the exact production view basis; body-stowed equipment follows yaw/height. Local kinematics/IK/animation, crouch, frozen simulation/progression and world-owned/detached actors/lights remain unchanged. Invalid, out-of-cap pitch or transient ownership rejects without mutation. Renderer Grip validators remain active.

The adapter fixture reproduces the former >100 m failure from a fresh ordinary production snapshot and checks <1 mm restored socket agreement, body stow, carried lantern/fire ownership, round trip and transactional failures. Both configurations pass. Windows consumes the helper and Android integration uses the same API, but these host tests do not compile either complete application or accept a post-fix RT capture. Camera/terrain/visual gates remain separate.

## Final frozen-bank identity

The affected Release ABI/selector/provider/ownership subset passed **14/14 in 82.81 s**, including all four instrumentation/quality provider fixtures and their exact stream-containment checks. Its [receipt](../../../reports/regional-water-final-release-validation.json) pins the tested catalog hashes.

After the separate look2 appearance correction, a fresh [identity-only check](../../../reports/regional-water-look2-abi-catalog-hashes.json) passes for all **16** pipeline/compute artifacts: raw SPIR-V count/magic/hash, canonical include hashes, recorded shader dependencies and exact generated catalog policy/metadata. The existing [catalog freshness check](../../../reports/regional-water-look2-catalog-check.log) also passes. No SPIR-V was recompiled in this lane. These are the latest artifact identities; the earlier compiled provider objects/tests are not relabelled as a post-look2 run. Root owns final shader-budget, complete application and runtime/visual evidence.

## Preserved failures and limits

- The prior 3 m coverage half-width passed host shape/profile tests but failed measured r22 shore coverage: its ~3.028 m edge stopped before dry contacts at 3.899/4.327 m, leaving ~0.87/1.30 m of submerged bed uncovered. The finite half-width is now 5 m; counts and datums remain fixed. This is host-checked coverage, not visual acceptance. The earlier 3 m source hashes/results are preserved in `historicalSnapshots`.
- Initial geometry checks had two failures: the generated include was not yet present, and the dry-notch test coordinate was actually inside the original polygon. The include and coordinate were corrected; no lake geometry was changed to satisfy the test.
- A strengthened negative closure fixture failed in both configurations because the old certificate already rejected its far-X bounds. A near-lake box now explicitly passes the old certificate and fails the new distance guard. Final reruns pass.
- A broader all-water first-cell-exit proof was rejected: an actual river target can leave the open east portal. A temporary larger retained-wall extraction experiment failed to compile from missing includes and was abandoned. Neither establishes a certificate.
- Initial regeneration with SDK CMake 3.22 hit mixed-module/cache compiler detection failure. Matching VS CMake 3.31 produced the passing host builds. No toolchain migration was made.

The ignored local [proof report](../../../reports/corridor-water-proof-2026-10-18/validation.json) preserves the failed-fixture and extraction history. Its `integrationPending` text describes the earlier snapshot; actual-face extraction was subsequently integrated and reviewed. Larger trial artifacts remain local; none are duplicated here. Shader trials belong to root's separate final shader/build append.

The [r23 terrain source/native receipt](../../../assets/models/world/source/chapter-terrain-r23/README.md), committed as `48662cc1`, now passes source conformance 12/12, regional foundation 5/5, unchanged-limit native intake and native support (2,781 queries / 616 sweeps / zero failures), plus glTF validation with zero errors or warnings. Its [bank receipt](../../../assets/models/world/source/chapter-terrain-r23/bank-conformance-receipt.json) records the bounded new-crest exceptions and finite sample limits. Actual shoreline appearance, device acceptance and game performance remain separate pending gates; source/native acceptance does not certify every shoreline point.

The earlier focused geometry/forest/adapter matrix used r22 terrain. Its source and binary identities remain historical; it is not relabelled as a new r23 test run.

The separate [r22 terrain receipt](../../../assets/models/world/source/chapter-terrain-r22/README.md) covers channel-conforming terrain, finite source samples and native admission. This host/proof lane did not independently resample exported GLB depths. Its finite samples do not certify every shoreline point or an arbitrary minimum depth.

Remaining acceptance: exact final shader/build identity, real RT captures and shoreline/motion inspection, baseline/candidate frame cost and actual RT memory/geometry cost, and any affected device/listening checks. No gameplay collision, swimming, saves, owner data or audio acceptance is earned here.
