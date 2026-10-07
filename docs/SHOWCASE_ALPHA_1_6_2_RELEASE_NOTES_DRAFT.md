# Horde Lantern RT 1.6.2 development notes

**7 October post-reset status:** The existing PR18 branch now contains the fresh no-preview Graphics restore correction, full Android gesture/pause cancellation, Windows unfocused controller suppression/reseeding, and Graphics scroll/focus retention. See [the current input regression evidence](ENGINEERING_1_6_2_INPUT_REGRESSIONS_2026_10_07.md) and [active remaining work/owner gates](ENGINEERING_1_6_2_TOMB_FINISH.md). Combat presentation, player rag torch, equipment/waterfall encounter, selected menu slice and sustained phone comparison are still being implemented or validated; they are not released features. Older exact-build runtime evidence is preserved at `1334cc9c58ec97940ac10d861f0397143ca7a9f4`, not claimed for these new edits. New integrated source/package hashes and aggregate CI will be recorded when that candidate is frozen. Eric's independent audit remains pending.

Package version: `1.6.2`

Android version code: `10`

This is an unreleased development candidate. These notes do not authorize
production signing, merging, tagging or publication. Published 1.6.1 source,
packages and evidence remain frozen.

Implemented source includes presentation semaphore ownership and optional
present-completion fences, explicit AS scratch/SBT alignment, complete cgltf
notices, efficient Windows output resizing and corrected report word boundaries.
Shared Graphics settings provide acknowledged Apply/Revert/Keep, persistence,
recovery and a compact production RT preview with bounded telemetry.

Material foundations include normal strength, glTF scale, tangent handedness,
authorable texture scale and ambient-only AO. Higher-tier bounded area shadows
await matched visual/performance admission. Temporal geometry and dormant DRS
policies are feasibility foundations; GPU history, reconstruction and vendor
upscalers are deferred with evidence recorded in the engineering ledgers.

Polish includes coherent held-torch clearance, bounded moving flame shapes,
deeper physically enclosed waterfall shaft with opaque foliage, reused authored
environment art, a deterministic Keeper reveal, native UI updates and admitted
audio derivatives. The collapsed entry remains behind layout and appearance
approval gates; no unfinished study is described as shipped.

See [the current checkpoint](ENGINEERING_1_6_2_HANDOFF.md) and its slice ledgers
for actual checks, artifact identities and outstanding gates. Physical RTX and
SM-S948B validation, current CI, final package checks and owner visual/audio
acceptance are pending. S24 final coverage is deferred and S25 unverified. The
formal presentation-retirement gap on unextended drivers remains explicit.
