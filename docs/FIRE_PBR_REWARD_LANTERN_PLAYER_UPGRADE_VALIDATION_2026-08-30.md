# Fire, PBR Props, Reward Lantern, Glass, Physical Carry, and Player Upgrade Validation

Date: 2026-08-30

## Authority and scope

- Authoritative base: `origin/main` commit `b45a1b71aa9b74178df72c8060f44b46b04a46b4`.
- Development branch: `codex/fire-pbr-lantern-player-upgrade`.
- Final runtime commit: `a04dcb9` (`fix: reveal reward chest and preserve lantern carry`). Final provenance corrections are commit `41e9c6c` (`docs: reconcile final asset provenance`); they change metadata and documentation only, not runtime code or packaged runtime bytes.
- Public release identity remains Showcase Alpha 1.5.2 / Android `versionCode 7`. This work did not sign, publish, upload to itch, rewrite history, or change application identity.
- The only Android device used was serial `R5GL219SZGK`, raw model code `SM-S948B`.
- The two external package references were re-opened and visually inspected during the final audit. `lantern_icon_reference.png` hashes to `bc4b237247982a0a1953c403459dc42f8ec579f7b093ec150e78d42ae07639dc`; `current_android_player_reference.jpg` hashes to `3624d805b33878a665aafd5046c26637a3ceade67cf7587c2dc492f6bfe849b9`. They remain non-distributable inputs rather than repository assets.

## Delivered engine and gameplay capabilities

- A validated static GLB/PBR import path now supports node transforms, indexed primitives, material assignment, texture contracts, diagnostics, manifests, runtime budgets, immutable BLAS ownership, and Android ASTC/Windows raw texture packaging.
- Fixed-capacity generated CPU/GLSL instance and material metadata routes new props without adding sword-, torch-, chest-, or lantern-specific primitive-range shading branches.
- Sword, torch, chest, reward lantern, and the reusable skinned-player study pass the same asset-contract and licence/package gates. The staged sword was accepted and processed instead of spending credits on an unnecessary regeneration.
- Sword, torch, and reward lantern use shared sockets, grip/pivot metadata, simulation-authored transforms, wall retraction, and TLAS-only rigid held-prop movement.
- Fire is a deterministic bounded emitter with an RT-visible emissive core, world-space depth-clipped volume, coherent coloured direct light, reflection contribution, and movement-reactive socket placement. The visible emitter, light, reflection, and flicker share one transform/state.
- Water and glass use reusable bounded dielectric helpers with Fresnel reflection, transmission, refraction, IOR, Beer-Lambert attenuation, explicit entry/exit handling, transparent shadow transmittance, and Mobile/High budgets implemented with `rayQueryEXT` inside raygen.
- The post-lich finale owns stable chest and reward-lantern state, locked/seal-breaking/unlocked prompts, monotonic Interact and Raise/Lower commands, authored lid motion, one-shot cues, reveal/equip sequencing, and checkpoint import/freeze support. Lich death begins an exact 120-tick/two-second countdown; the transition emits the unlock cue and activates a warm ray-query-shadowed world light above the physical chest on the same tick.
- Reward-lantern secondary motion is fixed-step, acceleration-driven, damped and clamped. It responds to starts, stops, strafing, turning, dodge, and hand-height transitions without `sin(time)` authority.
- The reusable player loader, render slot, animation layers, IK targets, bone sockets, CPU skin/refit interface, and downward/upward combo remain available for later completion. Owner review rejected the current first-person skinned hands, so normal gameplay deliberately uses the stable block-arm presentation for sword, torch, and reward lantern. Skinned hands are exposed only by named `player-body-*` development checkpoints.
- Windows checks GitHub Releases asynchronously at startup and can open the release/download page; Android offers the equivalent release alert without moving gameplay authority into Java.

## Owner-feedback closure

- The chest is in the lich room, participates in shared player collision, reports locked state before the lich dies, waits two seconds after defeat, then unlocks once, prompts for interaction, and plays the approved unlock/open cues. A physical aged-metal ceiling fixture and true coloured RT light make the closed chest conspicuous without opening the later finale skylight.
- The waterfall torch failure plays the approved extinguish cue and still honestly drops/extinguishes the original torch.
- The reward lantern is held from its top grip/chain route and uses wall-aware bounded motion. The low chest remains a player collider but is no longer misclassified as full-height masonry by held-prop clearance, eliminating the chest-side cull/crash while preserving the proven real-wall response.
- The sword presents its cutting edge forward, sits farther right on phone, and supports a downward cut followed by a second-press upward slice.
- Phone sword and torch composition borders the viewport rather than crowding the centre.
- The final max-upward-pitch checkpoint proves the left block forearm remains visible and attached to the held lantern.
- The owner reported that the preceding installed phone candidate feels good, and separately confirmed: “sound and haptics are good.” The new two-second unlock timing is a deliberate audio timing change and therefore still needs one owner-listening check on the final installed APK.

## Fresh Host gate

Final clean run: `reports/programme-completion-host-20260830/run-20260830-203317`

Result: PASS.

Source commit was `41e9c6c1d2f277af26f3aa842d9b9b8bdefaa565`; both pre-run and post-run Git status were empty.

- Raygen compilation, embedded-SPIR-V freshness, negative staleness fixtures, and compiler-strategy contracts passed.
- Fresh Windows Debug: 31/31 Vulkan-enabled CTests passed.
- Fresh Windows Release: 31/31 Vulkan-enabled CTests passed.
- Thirteen deterministic Windows RT captures completed.
- Android Debug, unsigned Release, and lint completed for the configured ABIs.
- GLB/PBR assets, runtime budgets, licences, shader ABI, strict transform arithmetic, and package contents passed.
- Generated validation artifacts are explicitly unpublishable/unsigned.
- The staged Windows executable was also launched from its isolated package directory. It remained live through the smoke interval, wrote a capability report with `rtScene.presented=true`, and was then closed cleanly. This is packaged-launch evidence, not owner play acceptance.

The production shader strategy retains functions for the generic route and keeps a fully inlined legacy comparison variant:

| Variant | SPIR-V bytes | Instructions | Functions | Calls | Ray-query sites |
|---|---:|---:|---:|---:|---:|
| Generic | 224,764 | 13,244 | 59 | 192 | 3 |
| Legacy | 493,244 | 27,152 | 1 | 0 | 23 |

## Exact `SM-S948B` standard route

Benchmark/install run: `reports/chest-guidance-device-final-20260830/run-20260830-194037`

Corrected replay/capture/lifecycle run: `reports/chest-guidance-device-replay-final-20260830/run-20260830-194713`

Debug APK SHA-256: `0b5a59b6e41d2c4d717eff885aaa310b7f5f1512002f6a89cb77e5989ab7edd3`

Result: PASS after correcting one harness-only expected-zone label. The installed bytes matched the local APK. Android 16/API 36, Adreno 840, strict ASTC material selection, `RayTracingPipeline`, GPU timestamps, and honest RT swapchain presentation remained active. The first run completed all six benchmarks, replay, both focused captures, and Home/resume, then reported only that `lantern-chest-held-high` expected `finale-room` while the engine correctly reported canonical zone `finale`. The corrected replay/capture/lifecycle run passed with no warnings or failures; no runtime code or APK changed between the two runs.

| Checkpoint | Scale | Median frame time | Thermal status | Samsung GPU power level | Battery |
|---|---:|---:|---:|---:|---:|
| opening | 75% | 53.392 ms | 0 | 0 | 30.0 C |
| two-enemy-combat | 75% | 48.868 ms | 0 | 0 | 31.6 C |
| worst-bend | 75% | 47.966 ms | 0 | 0 | 33.0 C |
| skylight | 75% | 37.704 ms | 0 | 0 | 33.5 C |
| green | 75% | 44.453 ms | 0 | 1 | 34.0 C |
| lich | 75% | 52.840 ms | 0 | 0 | 34.9 C |

These values describe a sustained Debug validation workload with capture and telemetry instrumentation; they are not claimed as Release frame rates.

## Exact `SM-S948B` feature route and owner-fix follow-up

Final clean 75% run: `reports/programme-completion-device-75-20260830/run-20260830-200903`

Final clean 100% run: `reports/programme-completion-device-100-20260830/run-20260830-201938`

Both runs passed with no warnings or failures from clean source `41e9c6c1d2f277af26f3aa842d9b9b8bdefaa565`. Local and installed Debug APK bytes matched SHA-256 `0b5a59b6e41d2c4d717eff885aaa310b7f5f1512002f6a89cb77e5989ab7edd3`; strict ASTC, `RayTracingPipeline`, raygen SHA-256 `d9bc8eff596cfaee67067329206aa02a2133c2f02e2250317c56def00a0c2bf2`, and honest RT presentation remained active.

| Checkpoint | 75% median / mean | 100% median / mean |
|---|---:|---:|
| `pbr-sword-closeup` | 58.364 / 61.338 ms | 99.447 / 100.828 ms |
| `pbr-torch-fire` | 60.283 / 63.948 ms | 98.515 / 99.025 ms |
| `player-body-grips` | 75.683 / 79.919 ms | 128.615 / 128.187 ms |
| `lantern-chest-unlock` | 53.481 / 56.879 ms | 83.954 / 84.171 ms |
| `lantern-held-high` | 90.342 / 91.906 ms | 147.773 / 147.888 ms |
| `lantern-held-low` | 81.011 / 85.387 ms | 140.519 / 139.712 ms |
| `lantern-glass-transmission` | 89.890 / 92.803 ms | 154.484 / 153.472 ms |
| `lantern-motion-extreme` | 91.176 / 95.421 ms | 153.942 / 154.364 ms |

The 75% run completed eleven exact-device captures and Home/resume. It ran at Android thermal status 3 and 42.1-43.2 C. Total PSS changed 578,768 -> 576,514 KB, RSS 698,300 -> 691,312 KB, native-heap private memory 255,172 -> 254,576 KB, EGL mtrack 121,100 -> 103,800 KB, GL mtrack 157,032 -> 156,868 KB, and scoped thread-list lines 32 -> 33. No growing graphics/native allocation signature was observed. These are instrumented Debug results and are candidly below the 30 FPS reference line for every measured feature checkpoint; the separate 100% run is roughly 6.5-11.9 FPS and is image/full-extent evidence, not a recommended play tier.

Final Windows feature captures are under `reports/programme-completion-windows-features-20260830`. All eleven honestly presented RT at 960x540 / 100% on the RTX 5050, with 12.055-16.884 ms medians. Agent inspection confirmed the lit closed chest, retained look-up arm, sword/torch composition, held high/low lantern, glass, and motion states. The skinned development checkpoint remains visually distinct from the accepted block-arm gameplay checkpoint.

| Windows checkpoint | PNG SHA-256 | Median |
|---|---|---:|
| `pbr-sword-closeup` | `5529530e3522da8cf143d69123cc0aa1bc2929bdfc98e5161bdd9b94991da426` | 13.619 ms |
| `pbr-torch-fire` | `4621db04d4b041067c1c69d16fb4bb0e27eaa7d2376244d0f3df16a463d13ed1` | 14.628 ms |
| `player-body-grips` | `4e086f856185bf1513c6214bfb3bd564c0abbc93eb6a456c2a2c2c6ca6bffe50` | 14.498 ms |
| `player-fallback-grips` | `a3d999626dfe9142b276f369295a08538eecba41986c8274daff61948489d217` | 13.152 ms |
| `lantern-chest-unlock` | `528aa37be3906d61385dd77f0f50c5c10b1fb81ff8d97dd5bae3edb1687d8cf1` | 16.884 ms |
| `lantern-held-high` | `f05c711feccfb4ece547fd842d809df2a7d3a96db10294cc6414bec1955fa222` | 13.956 ms |
| `lantern-held-low` | `d9947dfb4d673b775586a796bcd4ca894af59e6bbca6779dc7b1bf32d0d2912b` | 15.185 ms |
| `lantern-glass-transmission` | `5858f8d0fba48ac0dd4084d6929faf959cf1e8b90387fbbffa21a3892f5ebdf4` | 13.146 ms |
| `lantern-motion-extreme` | `ca620b83a232943ecf46c85bbe7fd4e36bf3f8660858ff67a5bc683a0d8cc7d0` | 12.055 ms |
| `lantern-held-look-up` | `d0512e52fd1af81dfee0ed057e13a4fd7838fc73c4c6c12c0625e844f0b41901` | 12.584 ms |
| `lantern-chest-held-high` | `9cd7ec2d6f925f9c48d3e6b8f1792d79fc0040f7deb017e142dd648061506b88` | 13.418 ms |

Run: `reports/final-owner-feature-device-20260830/run-20260830-182904`

Result: PASS using the same installed Debug APK hash. It captured:

- `pbr-sword-closeup`
- `pbr-torch-fire`
- `player-fallback-grips`
- `lantern-chest-unlock`
- `lantern-held-high`
- `lantern-held-low`
- `lantern-glass-transmission`
- `lantern-motion-extreme`
- `lantern-held-look-up`

Home/resume produced a new honest presentation marker. The max-upward-pitch capture preserves the block arm. The sustained warm `lantern-held-high` 75% Debug sample measured 99.145 ms at Android thermal status 2, Samsung GPU power level 1, and 40.9 C battery temperature.

The final `a04dcb9` follow-up captured `lantern-chest-unlock` and `lantern-chest-held-high` at the physical 1440x3120 display size with 1080x2235 internal RT extent. Agent inspection confirms the closed chest is strongly readable beneath its warm overhead light, the later roof remains closed at unlock, and the post-claim lantern remains present at the exact 1.30 m chest stand-off. Both frames honestly presented RT, retained twenty TLAS instances, used the hybrid block-primary route, and survived Home/resume. PNG SHA-256 values are `ce7db60577a8ff6a3fc8487729c0804c3677be05542aba042ea3453de69637a5` and `1074a12bfccd42d6eecbfae5f5ca44a7bd4fb9bd9b8b4cd6b09f5208bf216f31` respectively.

## Glass performance investigation

The feature brief required investigation above a matched 15% regression. The production path therefore went through controlled shader comparisons without changing scale, RT identity, or the physical model:

| Candidate | 75% lantern/glass median | Decision |
|---|---:|---|
| Original retained generic shader | 99.423 ms | Investigate |
| Exact zero-contribution query gating | 90.980 ms | Keep principle |
| Nonphysical scalar shadow isolation | 62.942 ms | Rejected and reverted |
| Physical world-bounds fast path | 83.796 ms | Best physical result; retained |
| Compact primitive ABI experiment | 84.992 ms | Neutral; reverted |
| Active dielectric sphere experiment | 88.190 ms | Worse; reverted |
| Final sustained warm route | 99.145 ms | Honest thermal/governor result |

The retained implementation skips transparent-shadow traversal only when the finite segment provably cannot cross a transparent world bound. It does not replace RGB transmittance with a scalar, lower render scale, remove glass, disable shadows, or weaken RT presentation. The owner’s physical play report is positive, but the warm glass path remains the clearest optimisation target for a later measured update.

## Review artifacts

| Artifact | SHA-256 | Status |
|---|---|---|
| `Horde-Lantern-RT-validation-20260830-203317-Windows-x64-UNPUBLISHABLE.zip` | `a6b3f82d03544ad81ed5ed92a539ba6e5b548e9793265a046643b70cbeca0cf6` | Local owner review only |
| `Horde-Lantern-RT-validation-20260830-203317-Android-UNSIGNED-DO-NOT-PUBLISH.apk` | `a7315f7b5058ca5f8320c60d74f4430f65bc5551fa902758ed2579472929ac26` | Unsigned all-ABI build evidence; not the installed Debug APK |

No artifact in this record is a signed or published release candidate.

## Known limitations and next-update boundaries

- First-person skinned gauntlets/hands are not player-facing. Their anatomy, left-hand quality, sleeve closure, and all-scenario grip quality need a later asset/animation pass before replacing block arms.
- Arm/body appearance in shadows and reflections is explicitly deferred by owner direction to the next update.
- The Mobile glass route records finite pane-stack/primary-volume budget terminations at ordinary held-lantern pitch as well as upward/grazing angles. For example, `lantern-chest-held-high` recorded six transport/pane-stack/primary-volume-budget terminals at pitch `-0.05`, with no shadow overflow, unclosed volume, mismatched exit, or open terminal. This is bounded recovery rather than recursion or a crash, but it can affect pixels and remains a correctness/performance target.
- The sustained warm glass route is expensive. Its physical model and 75% scale were preserved rather than manufacturing a pass.
- The GitHub release updater requires a future real release newer than the current identity for an end-to-end public notification test.
- Signing, version bump, publication, Butler upload, and itch verification remain owner-gated and were not performed.

## Owner review checklist

- [x] Fire appearance accepted by owner.
- [x] Phone control/held-item feel accepted by owner.
- [x] Downward/upward sword combo accepted by owner.
- [ ] Owner confirms the new two-second post-lich unlock sound timing on the final installed APK; other audio and haptics were accepted on the immediately preceding candidate.
- [x] Max-upward lantern arm remains visible in deterministic exact-device evidence.
- [ ] Owner completes a natural full-route chest unlock/open/claim replay on the final installed Debug candidate before release preparation.
- [ ] Owner explicitly approves a future version, signing, and itch upload.

Audio/haptic manual revalidation required: YES — the chest unlock event now intentionally fires two seconds after lich death, so the final installed APK needs one owner-listening check even though the cue asset, playback backend, routing, haptics, and previously accepted feedback are otherwise unchanged.
