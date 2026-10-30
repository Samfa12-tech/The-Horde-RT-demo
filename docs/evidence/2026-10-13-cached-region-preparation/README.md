# Cached triangle selection across render frames

13 October 2026. Implementation parent `549152ff3863b5aab3c54a1c159acbf6f340fe2d`.
**Normal-play residency saving is still ZERO; the chapter is incomplete.**
This milestone removes the roughly half-second synchronous candidate-packing
step measured in the [preceding checkpoint](../2026-10-13-triangle-region-candidates/README.md).
It does not authorize unproved omissions or enable automatic zone eviction.

## Implemented bounded preparation

The admitted immutable packed source now carries CPU-only triangle provenance.
The existing copy owner copies it across frames, then scans and compacts only the
selected owner's indices. Each copy and each scan is independently bounded by
the existing256KiB turn limit. Source leases, cancellation and retry remain
explicit; no worker accesses mutable scene state. Other owner spans and aliases
are rejected. Emptying a fixed primitive row is rejected before publication.

This reversible first method deliberately retains **all original vertex bytes**,
including vertices no longer referenced by selected indices. It preserves exact
normals, tangents, UVs, vertex offsets, player-role buffers, transforms, material
and texture records, fixed primitive rows and unrelated BLAS-local geometry.
There is no decimation, quality reduction or added row/descriptor/layer capacity.
It saves less GPU memory than synchronous vertex compaction; the reduction is
measured below. Full restoration uses the same immutable cache.

Native admission now rejects malformed labels and bad local indices before a
source can become eligible for cached selection. A new negative test reproduced
the prior bad-index admission gap before the guard was added. Retained Shared and
graveyard geometry remains exact. **A provenance tag still is not an occlusion
certificate.** Partial diagnostic owners continue to fail ordinary zone readiness
closed; no primary, secondary or light contributor policy has been weakened.

## Actual Windows evidence

RTX5050 Laptop GPU, Diagnostic/High/Higher, 960×540 at100%, the same frozen
`chapter-forest-lookout` view. Both backend captures complete real moving-pose
adoption, successful submission/presentation and old-owner retirement, then full
restoration. Selected/restored images were opened; both have zero changed pixels
within their respective runs. Default complete frames are also pixel-identical
to the preceding checkpoint. These are bounded transactions and this viewpoint,
not normal-play motion, all-angle visibility or sustained performance acceptance.

| Partial-candidate measurement | Pipeline | QueryCompute |
| --- | ---: | ---: |
| Begin CPU time | 0.1705ms | 0.0864ms |
| Maximum CPU copy/scan turn | 6.2440ms | 6.6478ms |
| Total stepped CPU preparation | 46.4926ms | 47.9220ms |
| Full preparation wall time | 2,347.16ms | 2,343.30ms |
| Partial cutover | 39.0193ms | 39.3129ms |
| Restore cutover | 39.1003ms | 38.5944ms |
| CPU copy/scan turns | 212 /27 scans | 212 /27 scans |

Both actual device-local allocations shrink405,550,288→404,554,192 bytes, then
restore exactly: **996,096-byte diagnostic saving**. Primitive/material identities
and texture allocations remain fixed. The whole vertex array stays resident.
Peak device-local overlap reaches473,908,432 bytes; host-visible bytes shrink
49,407,744→49,245,440, then restore. These memory classifications can overlap on
UMA and are not additive. Complete retirement/reference proofs remain required; the roughly39ms cutover
is still a hitch that needs ordinary-play scheduling/measurement. Allocator and
AS steps have separate measured maxima in each transaction receipt. Bounded byte
work is not a hard wall-clock guarantee and not a phone performance pass.

![Selected — Pipeline](windows/pipeline/cutover-01-selected.png)
![Restored — QueryCompute](windows/query/cutover-02-restored.png)

[Pipeline identities and counters](windows/pipeline/capture-manifest.json),
[QueryCompute identities and counters](windows/query/capture-manifest.json).
Each directory retains before/selected/restored PNGs, exact allocation/AS/upload/
CPU transaction data, stdout/stderr and pixel comparison. Prior startup failures
remain in the preceding checkpoint; these successful runs do not explain them.

## Validation and artifacts

Debug and Release pass all five affected CTests: static GLB, real forest/native
packing, initialization preflight, static cutover and resource inventory. The
final middle-owner fixture reruns in Debug; it places an unrelated prefix before
the selected owner and a role-separated suffix after it, preserving foreign
tags and testing offset rebasing through cancellation/retry. Real combined
chapter tests preserve all eleven rows and all6,184 graveyard triangles. An
independent read-only review found this fixture gap; it is now covered.

Initial RED had four assertions; the bad-index negative RED had one. Both are
retained. Cancellation at every copy/scan boundary, invalid tags/masks/instances,
shared-owner rejection and empty rows are covered. These CPU tests supplement
the actual RT transaction captures; they do not certify omitted contributors.

The full native final-torch runner retains the original1,475-pose assertion and
unchanged180-second deadline. Debug passes3,287 total cases (1,475 legacy plus
1,812 new roof cases) in178.32s, with zero failures; this is close to the deadline,
which remains unchanged. Release passes the same3,287 cases in17.80s.
Android four-ABI build/lint and286 cached Java tests pass with zero failures,
errors or skips; exact runtime/ASTC and held-item package checks pass. Exact results
are retained alongside the [exact candidate receipt](candidate-receipt.json).
The earlier `--original-sweep`352-case subset is not substituted for the full gate.
Android remains four native ABIs plus assemble/lint and existing Java contracts;
no shader, production quality, material/layer capacity or assertion changed.
Parent549152ff passed all six CI jobs in run38359636405. New exact-commit CI is
reported after push separately from those parent results.

The new APK SHA-256 is
`1a6f553f9f5761e7d4979ae0fc5b029ca46fe07f4a9bf648be89c71c758f8abc`
(183,107,002 bytes), immutable local
`reports/phone-candidate-1a6f553f9f57/horde-debug.apk`.
It is prepared but not installed: the owner temporarily disconnected
the phone. Last installed source remains42d00817, APK24aaf0b5894e57a38804f39875c06fbf34b801dfd0c83997b76c8aea8fa9233e,
already identified/verified once. Its frozen Diagnostic40% phone samples and all
their limitations remain in the preceding evidence. No fresh device evidence is
claimed here, and no installed identity is repeatedly queried. Original phone
settings were restored and Debug stopped before disconnection.

No new audio/haptic manual revalidation is triggered by this CPU/index ownership
change. The full chapter's fresh owner listening, haptic and touch acceptance
remains pending. Unrelated/historical masters, backups and released1.6.2 artifacts
remain untouched; PR27 is draft with base conflicts. No merge or release.

## Next required integration

Prove contributor eligibility against actual opaque surfaces, apertures and
active ray/light domains before permitting any subset during normal traversal.
Then connect preparation/readiness/commit and deferred completed-work retirement
to both rope directions, measure moving cutover/resource peaks and inspect the
complete route. Clearance bounds or one matched viewpoint are insufficient.
Normal-play residency saving remains zero until that integration is real.
Final phone/owner journey, sustained Shipping performance, lifecycle/orientation,
audio/haptics and persisted Lower→Current confirmation stay pending. Easy Tree
canopy and mist issue28 stay deferred; this work does not reopen them.
