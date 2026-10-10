# Rescue rope playtest corrections

This continues the existing development journey on `codex/horde-1.7-wp2-vertical-proof` and draft PR27. Starting local and remote custody was `64a98f718c76aeaa27ad2658c7cc4004948e28de`. Source/test checkpoint: `b9f85e11f3580d1a821a8e1cf3d529b1e6f496d2`; final native source: `53347791b0bd1331219651b01ea3012bafcab26d`, whose only additional change is a Windows GDI measurement cast required by MSVC. The primary checkout's unrelated modifications and untracked files and all other worktrees were left untouched. No game launch, device operation, merge, release, signing change, acquisition or next work package occurred.

The owner's missing climb prompt, requested L3 run, rope/mist artifact and clipped dust label belong to the previously launched executable `4e265d7fb26603a170c34d7ca7cfababaca68a0f3ef8cb8ae29611d1ddc69a61`. [Owner observations](../../evidence/2026-10-10-rescue-playtest-corrections/owner-observations.json) retain the two game-only screenshot identities and read-only L3 mapping observation. They do not accept this new candidate. All earlier reports/receipts remain unchanged.

## Scoped correction

The authoritative rope interaction radius is now 0.75 m instead of 0.10 m. Eligibility revalidates claimed ownership, the correct support, readiness, generation and the complete approach capsule path. A bounded approach moves at most 25 mm per fixed tick before gripping; upper approach cannot cross the coping at the wrong height. Failed/stale context is consumed without an attack. The visible prompt names left-click/A.

Lantern claim starts the existing one-second opening, followed by a provisional 0.65 s pause and a 2.0 s interval reserved for future Kit voice/subtitle completion. This is a development clock placeholder, not integrated dialogue or a voiced warning. No new on-screen throw warning is added. Then a compact coil is released with downward/lateral velocity and pays out at 6 m/s. The same 12-particle, 4.8 m line uses gravity **-9.81 m/s²**, Verlet velocity, damping, tension-only length constraints, shaft/coping/cantilever contacts and player-capsule contact. Full payout/contact/settling gates interaction. Pause freezes it, reset cancels it, and reconstruction retains paid-out nodes/history without a second throw.

Gravity is present in this bounded rope simulation. General player falling/jumping gravity is **not implemented**: walking still resolves support height, so limited step transitions can snap. Rope climbing remains a constrained motor coupled to solved grips, with a bounded load proxy; this is not a general rigid-body or cloth engine.

The fixed anchor is now at (-33.7, 3.678, -15.5), 42 mm below the beam's lower face, joined by an original 12-triangle retained C++ collar. Existing Blender/GLB shaft source files are unchanged. Grip selection follows the actual paid-out line; the final grips avoid the fitting and the separate pull-up retains support before releasing the hands. Player contact remains active under hand load. Loaded root targets stay on the authored front side (`z >= anchor.z + 0.30`); support-first resolution retains the existing 25 mm total step budget. Half-grip advance is provisionally 0.14 s. Existing arm reach, 40 mm beam clearance, 15 mm actual socket tolerance, root/grip relationship and continuity thresholds are unchanged.

Rope rings now follow solved tangents with outward winding. Actual facet normals choose the existing six-direction material-normal transport, and matching surface metadata is written under the existing host barrier/fence before BLAS/TLAS updates and rays. This remains a cardinal-normal approximation. Material, masks and off-camera ray/light relevance are retained. The owner's **broad angular mist-shadow band remains an unresolved physical visual issue**: topology/normal corrections are not proof of its cause or removal. A matched exact-candidate view/motion reproduction is required; no mist quality, shader, ABI, budget or accepted effect was removed to hide it.

Windows L3 toggles the existing shared run intent once per fresh press. The recognized WinMM controller uses the owner-observed mask 0x2000; XInput uses LEFT_THUMB 0x0040. Unknown WinMM mappings are unchanged; existing RB hold remains. Selected-device acquisition and focus reseeding baseline held buttons/triggers; pause clears running. These are host contracts, not new physical hotplug evidence. Same-slot hardware replacement without an intervening missed poll is not separately detected by the existing WinMM slot identity path. Android mappings are unchanged.

Graphics buttons get a full preset row, wider toggles and measured DPI-aware text fitting with a 9-DIP minimum. Preview motion/reset get full-width rows. Real GDI fixtures cover all captions at 760/900/1232-DIP widths, 96/120/144/192 DPI and Segoe UI/Georgia fonts; description/recovery text and telemetry also fit the minimum panel. The minimum-height layout retains all buttons within the client. This does not replace a real windowed-menu acceptance check.

## Exact-source validation

Selected CTest regressions: **54/54 Debug** and **46/46 Release** pass. Eight configuration-independent PowerShell checks run once in Debug and are excluded from Release repetition; no registered limit changes. Debug coverage: 89,973 traversal checks, 19,443 journey checks, 16,427 real-rig checks across 941 poses, zero failures. Max root/landing steps 0.0250017/0.0193812 m; left/right reach 0.612580/0.619561 m. Graphics: 515 GDI/source checks pass. The retained CPU blockout is 1,022 triangles, including the added 12-triangle collar and 76 separately labelled effect markers; rope/lid are separate. No GPU capacity/residency conclusion is drawn.

The unchanged original full torch sweep passes at **169.73 s Debug / 14.72 s Release**. Debug margin is **10.27 s** under 180; outer wall is 169.7703639 s. Both logs independently retain 3,287 poses, 1,812 roof/entry skinned poses, 3,178 plane and 57,970 disk samples, zero assertion failures. The preceding 171.46 s / 8.54 s margin remains historical; one new pass does not establish repeatability, GPU performance or the cause of the original WP0 timeout.

Windows Debug/Release builds passed. Android Debug and unsigned Release native outputs were built for arm64-v8a, armeabi-v7a, x86 and x86_64; regular Debug/Release lint passed. The 269 existing Java tests in 40 suites remain zero-failure cached results because Java/test inputs are unchanged. The final-source Gradle refresh is explicitly cached, following the Windows-only type fix; no new Java execution or different Android bytes is inferred.

All eight Pipeline and eight Query source/dependency/catalogue/freshness routes and generated adapters remain current. No shader source, ABI, generated shader package, budget, strategy, optimizer, asset or original torch-test policy changed. Artifact/manifest negative and transactional rollback tests pass. Actual Windows Debug Diagnostic/High and Release Shipping/High executable containment checks inspect both selected backend pairs. Actual Android Debug Diagnostic/Mobile and unsigned Release Shipping/Mobile shader inspection covers **ARM64 ONLY**. Separate checks verify all four ABIs' APK entries match stripped libraries and expected ELF identities/16 KiB LOAD alignment; that is not all-ABI shader inspection. Editable asset sources are excluded from APKs.

Exact candidate artifacts (local builds; no publication):

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| build/presets/windows-x64-debug/Debug/HordeLanternRT.exe | 11858944 | `79e1e8634717bb563928611553019506288a78ce0df2cd1032d7a164e45a6a75` |
| build/presets/windows-x64-release/Release/HordeLanternRT.exe | 4171264 | `29d3c1085ea124847b731aad44dc576fc69d7ede879f9cbc2efda640bd572359` |
| android/app/build/outputs/apk/debug/app-debug.apk | 133692395 | `deb5f7a0dedc294bd00c7e08c6f9e928b27186baa8d4fa705d61df634bc28c30` |
| android/app/build/outputs/apk/release/app-release-unsigned.apk | 121490514 | `79fe09f5482aba2814a5abbd7cf84b173f9adeaac9a89d983428fdc9f847fe80` |


Complete sanitized argv/cwd/source identities, elapsed times, exit statuses and log hashes—including failures—are in [the run ledger](../../evidence/2026-10-10-rescue-playtest-corrections/runs.jsonl). [Receipt](../../evidence/2026-10-10-rescue-playtest-corrections/receipt.json), [source/blob links](../../evidence/2026-10-10-rescue-playtest-corrections/source-path-manifest.json), [actual binary fixtures](../../evidence/2026-10-10-rescue-playtest-corrections/fixture-identities.json), [shader identities](../../evidence/2026-10-10-rescue-playtest-corrections/shader-identities.json) and [all-ABI package identities](../../evidence/2026-10-10-rescue-playtest-corrections/android-all-abi-identities.json) separate each evidence class. Earlier dirty diagnostics have their exact base/diff hashes; they are not represented as final-source passes. The logs retain the failed numerical/contact approaches, stale expected anchor/count assertions, test declaration-order build error and native LONG/int build mismatch. Their scoped corrections preserve assertions and deadlines.

The large repeated-failure log is stored as lossless `.log.gz`; its ledger row records both compressed-file and uncompressed sanitized-payload hashes. Decompression was verified byte-for-byte against the retained complete local sanitized log. No failures or repeated messages were removed.

## Prepared owner check

From the existing worktree, verify the Debug EXE hash below and launch only when authorized:

```powershell
.\build\presets\windows-x64-debug\Debug\HordeLanternRT.exe --development-rescue-journey
```

For an explicitly supported Query run add `--require-rayquery-compute` and verify the reported backend. Ordinary launch must retain the legacy route. This report does not launch, install or select a device.

1. Claim the lantern; watch opening, pause, throw and actual unfurling. Bump the line and watch it settle. No temporary warning UI should appear.
2. Approach the lower rope around (-33.7, -15.2), use the indicated left-click/A, travel to the lookout and return; repeat both directions. Look up/down and sideways while checking both grips, free hands, stowed sword/lantern, flame/light, fitting/coping clearance and restored gear.
3. Toggle L3, hold it to check one-shot behavior, and test pause/focus recovery. Confirm windowed graphics captions at the actual window/DPI, including indoor dust.
4. Reproduce the old rope/mist view at matched settings and inspect moving/off-camera shadow, reflection and transmission contributors. Retain failures; do not accept the broad band from host tests.
5. Complete authorized Windows focus/resize/restart and Android lifecycle/orientation/surface/reconstruction cases separately on exact candidates. Listen/feel traversal, movement and combat feedback; silence is not acceptance.

**HOST/PACKAGE-READY for the scoped engineering correction. DEVICE-VERIFIED: NO. OWNER AUDIO/HAPTIC ACCEPTANCE: PENDING. WP3 OUTDOOR BUDGET ACCEPTANCE: OPEN.** Audio/haptic revalidation is required because changed traversal/attachment/input timing can alter emitted listener positions and feedback. Prior owner acceptance is not transferred. Actual outdoor mist/fireflies, final forest/Kit cost, GPU residency choice, dedicated directional/climbing clips, durable saves and future voice remain open. Final pushed evidence SHA/CI are verified externally to avoid recursive commit provenance. No automatic advance follows this checkpoint.

Existing forgiving unobstructed sword contact and the inactive conservative proxy are unchanged. Authored planar masonry protection is not proof of arbitrary 3D shaft contact occlusion; the previous measured contact gaps and deferred moving-target/wall/corner acceptance retain their own evidence.
