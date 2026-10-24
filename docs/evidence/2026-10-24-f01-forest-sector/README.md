# F01 woodland layer: actual RT trial, composition unadmitted

The twelve-root layer adds middle-distance silhouettes, but **still fails the intended surrounding/vast forest**. Both north and west continue to read as isolated open-crown trees over bare land. Keep this diagnostic trial available; do not promote it to normal scenery or call the forest finished. The [side-by-side sheet](index.html) uses original actual RT pixels, with [north before](north-before.png)/[after](north-after.png) and [west before](west-before.png)/[after](west-after.png).

This follows the passing [existing 64 resource/correctness proof](../2026-10-24-shared-forest-route/README.md) at `8ddc08fd` and its [bounded F01 plan](../../design/HORDE_1_7_F01_BACKGROUND_SECTOR_2026_10_24.md). The candidate uses the admitted masters and preserves the original 64 transforms, all deadwood/understory, terrain, route, lighting and distant town. Replacement-canopy/Easy Tree remains deferred; no new model, shader effect or playable1.8 content was added.

## What is implemented and tested

The native helper measures twelve supplemental records without copying either master. All 936 root-tip samples are supported; none is exposed. Maximum fitted burial is 0.146903 m, below the existing 0.65 m guard. World/trunk bounds and exact rigid transforms are retained in the [receipt](receipt.json). The original 64 collision-receipt 2 mm guard and both compatibility digests remain unchanged. The first helper test exposed changed missing-support wording; source wording was restored without altering its assertion.

Explicit Debug `--forest-route-proof f01-sector` appends eight pine/four alder references to the same two BLAS. Actual population is 76 trees /104 physical definitions, 40 pine/36 alder. Shared64 control stays 92; ordinary normal gameplay stays 28 physical roles and its existing 64 baked trees. Actual High packing remains 36 primitives, 32 final materials, 13 asset owners and metadata rows 26/27. No truncation or temporary extra material rows are used.

Windows RTX 5050 Laptop GPU, real Pipeline RT, Diagnostic High, 960x540 internal/output, 100% scale and DRS off. North, west and lookout each run control → candidate → closing control, with two frozen readbacks across same-object recreation. All nine processes exit0 with current presented evidence, visible modelled arms, identical within-variant recreate pixels/resources, unchanged original 64/legacy roles and matching closing-control pixels. All 151 parked owner files, campaign/settings and foreground custody remain unchanged. A separate ordinary launch proves actual 28 despite 104 storage.

| Measured change, shared64 → sector76 | Result |
| --- | ---: |
| Static vertex/index payload |Unchanged:17,686,464 /2,621,076 B |
| Unique assets / BLAS / material rows |Unchanged:13 /22 /32 |
| Tracked device-local allocation increment |5,504 B |
| Tracked host-visible allocation increment |768 B |
| North changed RGB pixels /518,400 |9,754 |
| West changed RGB pixels /518,400 |11,633 |
| Lookout changed pixels |0 |

Host/device memory categories may overlap; do not add their increments. View-dependent inventories are compared within each matched view. These are allocation facts, not total RAM/residency/startup peaks. The small memory increment does not establish mobile feasibility or a speed gain.

| RenderFrame wall medians, initial/recreated ms | Control | Candidate | Closing control |
| --- | --- | --- | --- |
| North |7.116 /6.895 |6.557 /7.097 |11.192 /6.093 |
| West |6.657 /6.780 |6.803 /6.709 |6.842 /7.036 |
| Lookout |6.894 /6.488 |8.858 /6.062 |6.103 /6.878 |

Twelve unpaced call samples include waits/presentation. Single current owning GPU values and CPU stages are retained, not per-stage GPU distributions or scanout FPS. Initial/closing drift prevents causal gain or universal cost claims; no clock/thermal evidence attributes it to temperature. No animated/warm phone acceptance follows. No extra long performance loop was run.

## CI correction: live profile is separate from storage

Exact run [40099145025](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40099145025) at `8ddc08fd` ended with MSVC, Vulkan-host, Android and sanitizers passing; GCC and Clang failed the same Android witness-policy check. The actual runner helper derived expected normal instances from ABI **storage**, which would incorrectly require 92/104 from a 28-role ordinary scene.

The ABI now declares `sceneCounts.ordinaryShowcaseTlasInstances=28` separately. CPU code uses its generated constant and statically checks Keeper 27+1. The runner requires that explicit live profile and validates it within storage; its legacy helper/report names remain compatible. Fixtures distinguish 21/21 from28-live/22-metadata/104-storage and reject absent, malformed or excessive live counts. All existing backend, scoped-log, Home/resume, no-data-clear and current-frame witness assertions remain. Local injected policy coverage passes 92 assertions, without invoking ADB or installing anything. Generator freshness and new negative-count cases pass. New-head hosted results must be read separately; the prior failure is preserved.

## Real remaining admission work

Focused Debug helper and six native contracts pass. The normal Release app compiles and all four relevant Release import/ABI/parser/role tests pass. These do not close visual, collision, phone or sustained-performance acceptance. Both helper failures and prior hosted failures remain retained; no deadline or assertion was relaxed.

All twelve roots must be treated as reachable. The 12 m route band selects readiness rather than blocking movement. Actual-terrain connectivity sampling found paths to every root; this is not native continuous movement acceptance. Current collision still owns only the original 64. The candidate is explicitly `collisionAdmitted:false`, and normal remains unchanged.

If a composition is worth retaining, publish an immutable optional extra-trunk owner atomically with the same terrain/world generation. Default must be empty; reject stale/replacement owners, retain failed-load state and rebind on recreation. Movement, step/posture, held clearance, Kit motion and campaign SafeSupport/load must consume the same exact measured bark bounds. A native approach/stop/backtrack witness is required before adoption. Do not add invisible blockers to the 64-tree control or accept traversable rendered trunks.

The current 12-root arrangement does not justify that wider integration yet. Next prepare a coherent massing/enclosure choice against the original topo and actual north/west/lookout views: enough layered woodland to close the bare middle distance, while preserving clearing, trail and landmark reveals. Denser existing references need a measured regional instance/collision budget; replacing the open crowns requires explicitly revisiting the owner deferral. Avoid another quota-driven scatter loop or mist to hide the missing forest. Normal production shared-BLAS also retains its omission/cutover and Android gates.

No phone, foreground activation, merge, release, deployment or owner-data change occurred. Audio/haptic manual revalidation required: **NO**; this changes diagnostic geometry/validation only. The separate default Debug movement 30-second timeout remains open, and the earlier two-part Debug journey gate remains closed.
