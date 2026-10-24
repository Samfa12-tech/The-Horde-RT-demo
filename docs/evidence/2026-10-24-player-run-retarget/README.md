# Owned Run retarget: unadmitted preparation study

The owned Kit Run is not a drop-in player animation. This bounded study retains a reproducible derivative and the failed native grounding witness; **production assets, bindings, gameplay speed and first-person viewmodel remain unchanged**. It does not finish the player running-art gap.

## Retarget and cadence

Player and Kit have the same 24 deforming joint names and parent relationships, but different global rest orientations (Hips differs by about116 degrees). The player retains its26 joints including the original Grip nodes, original local bone lengths, all five mesh groups, materials and original Idle. The complete2,516,028-byte original BIN prefix remains exact. World-space rotation deltas from the owned Warden Run are transferred to the player's bind basis; hips translation scales by0.994780768, derived from the two leg-length ratios. No root-motion travel is added.

The corrected offline sampler uses explicit four-component quaternion normalization and native-compatible nlerp. The existing native importer linearly samples the dense source keys, including five STEP-labelled samplers; this study does not claim exact glTF STEP playback. An initial helper mistakenly normalized only XYZ while retaining W. Its exploded-scale and Idle-relative results are invalid fixture outputs, retained in the raw reports, not evidence against the owned Run or the production loader. The earlier single-mesh contact helper was also corrected in memory to include every player primitive; its original historical file is untouched.

The20mm contact-window fit estimates2.204526435m per cycle. To keep the current distance clock and3.2m/s motor, the diagnostic file's period is1.160277071s at the1.9m/s base, giving a0.688914511s wall cycle at run speed (about174steps/min).10/20/30mm sensitivity results remain in [calibration](calibration-native-nlerp.json); those windows are measurement choices, not new acceptance tolerances. The supplied scripts reproduce separate diagnostic files under `reports/player-run-retarget-20261024/`. Run is mapped onto Walking only inside that file to inspect existing native preparation; it is not a proposed production semantic binding. The2,645,708-byte calibrated GLB remains local, hash-pinned in [receipt](receipt.json), to avoid committing another generated runtime duplicate.

## Actual native result and remaining correction

The original control passes192 actual simulation-driven poses, including existing Grip and sole bounds. The candidate reaches3.2m/s but fails the unchanged grounding witness at view0/tick36: floor−0.95m, soles−0.844604/−0.838484m, blend0.952936, phase0.687719. Both feet are about105mm above the floor. A run can legitimately have a flight phase; this is not by itself proof of visibly bad art.

The current `SkinnedMeshAsset::BootGroundingMinimumY` accepts only the audited±75mm walk envelope. `PlayerRenderSlot::BootGroundingOffsetMetres` falls back to its safety offset when the retarget exceeds it. The fixture root shifts from−0.993902m at tick35 to−0.949750m at tick36, a44.15mm one-tick change. Reusing the walk preparation therefore cannot be admitted as a running implementation. No envelope/assertion was enlarged, and no flight phase was silently flattened. The smallest next implementation decision is an explicit Run preparation/binding with coherent contact-versus-flight root behavior and joins, followed by visual validation; this checkpoint does not implement it or claim an accepted gait.

## Actual RT presentation and limits

One temporary Debug direct-read selector loads the separate candidate pose asset while original static geometry and viewmodel remain unchanged. One binary runs both variants at960x540 internal/output,100% scale, Diagnostic High, RayTracingPipeline. Each completes180 fixed ticks and32 storage-image captures; all sampled frames match successful presentation/completion identities and pass existing dedicated-player ownership and visible-arm gates. Run/stop/pause are exercised with ordinary input, without combat commands, earned progression flags or campaign writes. These unpaced fixed-tick runs are not FPS measurements or natural journey evidence.

The intended look-down input−0.95 was clamped by current simulation to−0.32; this is a fixture coverage error, not a verified look-down/body comparison. The current normal-input pitch range is−0.32..+0.28. The final-room approach reaches the mirror, but the live Keeper obscures the relevant body reflection. No enemy reset, artificial victory, lighting change or view exemption was used to manufacture coverage. Hands remain visible and agree with the existing grip checks; **body contact, run silhouette, reflection and shadow-motion acceptance remain open**. Sampled actual images were inspected:

| Scope | Original | Candidate |
| --- | --- | --- |
| Run tick36, clamped ordinary first-person view | [Control](rt-control-6.png) | [Run](rt-run-6.png) |
| Final-room tick72, Keeper obstructs mirror | [Control](rt-control-28.png) | [Run](rt-run-28.png) |

## Custody, failures and restoration

Creating the prior isolated-assets junction was denied by the OS: `cmd /c mklink /J`, target `reports/player-run-retarget-20261024/control/assets/audio`, source `assets/audio`, Access is denied. It created no junction. That action was not retried via another linking/copy route. Independent direct file loading through existing loaders requires no asset-tree links or original asset swaps; it supplied the narrower CPU/RT evidence above. The exact [denial receipt](junction-denial.json) is retained.

Two accidentally overlapping build sessions were cancelled before any proof; one sequential build then succeeded. Neither cancelled session is called a passing check. All three temporary source hooks were restored byte-exact with concurrent-edit hash guards and rebuilt. Settings and campaign hashes/existence remained unchanged, as did all151 parked owner files; foreground readbacks were unchanged. There was no activation request, phone operation, new license/provider call, public release or runtime asset replacement. Original failed receipts remain alongside the corrected-sampler analysis. Relevant restored-source test results are recorded separately.
