# Incremental native GPU preparation

Continues `d130386a19df914532f7a9f442de2103d454f9b7` in PR27. The chapter
still has zero normal-play residency saving. This milestone makes the existing
offside upload owner advance across render turns; it does not activate zone
eviction or claim seamless streaming.

Each advance allocates at most one buffer and copies at most the caller's byte
limit. Candidate allocations remain visible in resource inventory throughout
preparation. Payloads stay borrowed from the candidate's owned immutable slot;
cancel, move and destruction release only that candidate. Atomic take preserves
an admitted destination. Device lifetime remains the platform's responsibility.

The scene now separates upload, collapse BLAS and TLAS preparation. The TLAS
uses the latest submitted definitions. Changed poses, masks or BLAS references
reject cutover before publication, including changes during the external
retirement proof. Existing descriptor rollback and completed-work retirement
remain intact. A stale candidate requires refresh/repreparation; it cannot
restore old held-item or actor state.

| RayTracingPipeline | RayQueryCompute |
| --- | --- |
| [Replaced owner](pipeline/cutover-01-selected.png) | [Replaced owner](query/cutover-01-selected.png) |

Both actual RT images were opened. Each backend's selected and restored frames
are byte-identical to its admitted frame: four zero-pixel comparisons. Real
partial allocation cancellation also preserves the admitted allocation totals.
The same 960x540,100%, High/HIGHER frozen view retains rocks, fern bank, tree
shadows, lantern/glass and primary equipment. No foreground input was sent.
Exact build, shader, camera, rope/light and resource identities are recorded in
[candidate-receipt.json](candidate-receipt.json).

The diagnostic copied41,692,348 bytes over164 upload turns with a256KiB copy
limit, continuing to render between incomplete steps. Preparation spans166
turns including the two AS stages. Measured Debug packing is37.9–43.3ms;
maximum individual advance is4.7–6.1ms and cutover3.2–9.1ms across the four
hops. These captures ran alongside builds and are **not sustained performance**.
CPU packing and each AS build still run synchronously. Vulkan allocation/map
latency has no time guarantee. Shared textures and the complete world remain
resident; retirement replaces67,334,720-byte owners without reducing residency.
The calculated owner overlap includes actual transient scratch allocations.

The new bounded-upload and stale-pose tests first fail, with those logs retained.
Focused Debug4/4 and Release4/4 pass. Expanded upload checks pass separately in
both modes: exact data/mapping,96 synchronous/incremental injected Vulkan
failures,33 cancellation boundaries, move/self-move, inventory and preflight.
Android four Debug ABIs, assemble/lint/unit tasks and exact APK/ASTC/held-item
package checks pass. Java implementation is unchanged. No shader or capacity
limit changed, and original torch coverage/deadline is preserved.

Parent `d130386a` passed all six CI jobs in
[run38191982003](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38191982003):
MSVC137, GCC/Clang126 each, Vulkan45. Actual logs verify each of the seven
requested traversal CTests and the native rock test runs once in Vulkan. This
milestone's exact CI result belongs to its subsequent pushed SHA.

Next required work is normal-play readiness/ownership, bounded CPU/AS
preparation and conservative spatial contributor retention, followed by the
remaining prepared wood/stump/snag, full two-way gallery and sustained route
measurements. Phone Current/Higher, saved Lower migration, lifecycle and owner
audio/haptic/feel acceptance remain pending. The18-layer/three-actor ceilings,
frozen shaders and quality remain unchanged. Easy Tree/canopy and mist28 stay
deferred; no merge, device installation or release.
