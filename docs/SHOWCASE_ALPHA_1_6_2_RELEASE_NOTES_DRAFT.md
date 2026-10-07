# Horde Lantern RT 1.6.2 development notes

**7 October post-reset status:** The existing PR18 branch now contains the fresh no-preview Graphics restore correction, full Android gesture/pause cancellation, Windows unfocused controller suppression/reseeding, and Graphics scroll/focus retention. See [the current input regression evidence](ENGINEERING_1_6_2_INPUT_REGRESSIONS_2026_10_07.md) and [active remaining work/owner gates](ENGINEERING_1_6_2_TOMB_FINISH.md). Combat presentation, player rag torch, equipment/waterfall encounter, selected menu slice and sustained phone comparison are still being implemented or validated; they are not released features. Older exact-build runtime evidence is preserved at `1334cc9c58ec97940ac10d861f0397143ca7a9f4`, not claimed for these new edits. New integrated source/package hashes and aggregate CI will be recorded when that candidate is frozen. Eric's independent audit remains pending.

The owner approved the compact hanging-lantern direction and requested phone Play centered below the lantern, More bottom left and Settings bottom right. Actual portrait inspection confirms that placement at existing font scale 1.7. Grouped Graphics now has optional details and a fixed native Use/Keep/Restore/Back dock. The real opening crash in `88548fc1` is reproduced and corrected in exact phone package `4d801445`; affected Java checks and actual phone opening/scroll/action visibility pass. Landscape native controls are readable, but RT orientation fails; pushed `422c1b1a` surface policy still needs local device validation. That source has all 12 aggregate CI checks successful. The memory pipeline cache shows no measured improvement: Glass Off 16.659 s and return On 16.589 s, mostly pipeline compilation. Compiled-pipeline reuse is under bounded investigation; exact ACK, normal confirmation and confirmed saved preferences are preserved. [Exact source/packages and evidence limits](ENGINEERING_1_6_2_MENU_PHONE_2026_10_07.md) remain separate from final review-candidate, sustained performance and owner acceptance. Equipment/waterfall activation is still gated on corrected physical stow/scabbard, audio and integrated route checks. Earlier red CI and failed inspections remain preserved.

Latest intermediate source `359a57112fd198351edcc6c4b6eae52f28bcd2a6` passes
all 12 aggregate CI checks (push `37610890737`, PR `37610897316`). Exact
four-ABI Debug APK is SHA-256
`e8f0dee22d0093c69be8b704a61cf651003a6764eb227f89fab6e3c12c3eb48d`,
138,462,724 bytes; Windows Debug executable is
`cd5a903269b5d65ad7518282e6cd367558e77a1480e5c1c2aef261e8f10ffe40`.
Installed phone pullback matches. Owner-requested colon labels and clear
scrollbar pass actual large-font portrait inspection and 22 affected Java
checks. The compiled-pair cache now has measured phone hits: Glass Off 0.367 s,
return On 0.326 s, exact Use/Restore ACK and unchanged saved preferences.
The earlier driver-cache negative remains valid. Landscape RT output is now
upright but a repeated recreation loop prevents recovery; this remains a
failed gate. Selected FilmCow attachment cues and authored scabbard are
integrated, with moving/owner acceptance and production activation still
pending. See the current phone/cache/scabbard ledgers; the preceding paragraphs
retain their historical results. This is not the final review candidate.

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
