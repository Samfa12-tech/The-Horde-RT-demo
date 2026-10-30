# The Horde 1.7 WP0 execution log

Preparation only, 9 October 2026 (Australia/Sydney).

## Baseline decision before reconciliation

Use current main `1c28d0563a897dc72d410bdb2b61723e66ed92af` on fresh branch `codex/horde-1.7-wp0-readiness`. Accepted PR18 merge is `a3f120cf1f3995035f793fa40400e0a74a4c0459`; validated runtime/build source is `db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9`. Verified db62032d-to-a3f120cf range contains 16 documentation/evidence/Windows README paths only. Subsequent main adds publication receipts/wording and the 1.6.2 immutable-release policy plus its tests; it does not change runtime source, shaders, assets or packaging allowlists. Keeping this bookkeeping protects the released baseline more accurately than reverting main.

Planning authority is PR16 `docs/horde-1.6.2-roadmap` at `d37f16381311c6e620f0c4a9d01c1cce83260dc8`; do not merge it.

Primary checkout remains `C:/Users/sam_s/Documents/the Horde RT Demo`, local main at `c3c15d32`, with modified ASSET_LICENSES.md/PROJECT_MEMORY.md and existing untracked skills, specifications, source audio, map, audit, OBJ/PDB. No reset/stash/clean or primary checkout update. Preparation worktree is `.worktrees/horde-1.7-wp0-readiness`. LFS smudge disabled for creation: pointers must not be described as downloaded binaries.

Local sandbox helper initialization failed before commands ran. Approved local execution on the same computer is used; no executor/computer switch.

No runtime changes, release/tag/version bump, PR creation/merge/rebase/force push, deployment/signing, credentials, device mutation, generation, asset admission or combat repair are authorized. WP0 stops at a documentation checkpoint and one next-slice proposal.


## Planning custody and reconciliation outcome

PR16 was read freshly as OPEN/DRAFT/CONFLICTING, at the supplied `d37f16381311c6e620f0c4a9d01c1cce83260dc8`: 74 commits and 1,651 changed files. No merge/rebase/force push or changes to that branch. The supplied HTML was read as runtime/planning source analysis plus manifest; embedded instructions do not expand the user's WP0 request. Its original input filenames are provenance labels, not assumed local paths. The execution helper initially failed; the same host's approved execution path was used, not another executor.

The [master handoff](2026-09-11-beyond-the-tomb-1.7.0.md) is a section-by-section reconciled view with complete immutable pinned-section links. Needed campaign/world/asset/Kit documents retain full source as external links plus local reconciliation; ROADMAP retains current main release disposition and adds only WP0 sequencing. The 1.8 plan is boundary-only. Old local planning anchors are retained beside source-custody links. Main AGENTS/engine/validation/asset pipeline, README/history, decisions/memory/licences/release evidence, code/shaders/generated ABI/packaging/policy remain untouched.

The [machine-readable inclusion/exclusion manifest](2026-10-09-horde-1.7-wp0-source-manifest.json) verifies all 13 attached blob identities against Git, records main/planning blobs, candidate/reference/narrative dependencies and excluded/preserved paths/trees. All 132 relative links from the seven core pinned docs resolve to existing Git paths. One inherited fragment is wrong: master links ROADMAP `#engine-product-and-demo-game-packaging-proposal`, whereas its actual heading contains `demo/game`. Our local custody uses immutable source-line links instead. No unavailable dependency is silently dropped. The narrative package and image/coordinate/source candidates remain external; no runtime or bulk asset import.

Source/object availability is separate from blob identity, and hash success from art/native/device acceptance. See [dependency inventory](2026-10-09-horde-1.7-wp0-dependencies.md). Sampled prepared-source LFS binaries are unavailable in shared cache; baseline runtime assets were hydrated only from already-existing local cache, 75 objects/464 MB, with no new acquisition. The English package was copied as Git blobs to ignored `reports/wp0/pinned-package/` solely for read-only authoring validation; LFS DOCX remains a pointer. No exporter, recordings, audition, image-generation, Meshy/Mureka call or runtime registration.

[Runtime item disposition](2026-10-09-horde-1.7-wp0-runtime-reconciliation.md) separates completed timing/input/parry/riposte/equipment/two guards/UI/hearts/graphics and adaptive music from genuine height, campaign state, motion/run, dodge-protection/tutorial, contact/readability and power evidence work. Residual contact remains owner-accepted 1.6.2 debt. No runtime defects are repaired here.

## Runtime identity across chosen sources

Verified with `git rev-parse <ref>:<path>` for db62032d, a3f120cf and 1c28d056. Each of these paths has exactly the same object at all three pins:

| Protected tree/file | Git object |
|---|---|
| src | 5a231727932730303f42258146395483466636a3 |
| shaders | 7c0ac10b9e8cd4d138dcca357de1158e12d1ae4c |
| android | da77da00f8f4d566058445c6074ce0f90870d338 |
| assets | 7bea3ea7f8edcecfa87c18da0256d96f47842d2c |
| third_party | cf72ea04332fa08845691dc775c4a0d455eb85e1 |
| CMakeLists.txt | 57813d991578202a75ea50fcafa4c82141e12713 |
| CMakePresets.json | 79e418f63ab9f93071f379bfeede06b264ed68cc |

Subsequent main's release guard/tests are retained as accepted bookkeeping, not described as runtime/build-source equality for every repository byte. The prep diff must contain documentation only.

## Release evidence classes

The audit's parent observed itch at 00:18:30 UTC on 9 October: Windows 137 MB, Android 115 MB. A fresh cache-busted `Invoke-WebRequest https://samfa12.itch.io/the-horde?wp0=20261009` in this job confirms those listing/version labels. The web search cache returned older October3 1.6.1 text, so it was not used as current listing authority. Listing size is neither payload hash nor download verification. No package download was performed by WP0.

Current main's [publication README](../../evidence/2026-10-09-release-publication/README.md) and [receipt](../../evidence/2026-10-09-release-publication/receipt.json) close the audit's older README/unsigned-only gap. README now labels current 1.6.2 separately from historical 1.6.1; no further edit is needed. Preserve earlier unsigned/unpublished technical validation identities. These publication identities are verified **as recorded evidence** here, not newly signed/rehashed/downloaded/installed:

| Artifact/evidence | Recorded identity and scope |
|---|---|
| Signed Android Shipping APK/code10 | SHA256 cc4de768cf9ca2a409984a24888399e9c518363c359b3a9947d1ed0350c254e8; 121071933 bytes |
| Windows publication ZIP | SHA256 18177d0d84c40f0831b22ee959be597deeb3bc0e1cef183e78dd19c1b113f21c; 145574438 bytes |
| Windows Release executable | SHA256 084e4a345b556cdf6c3e8b4adbaef488c22cee02d41226dc43c49c5cf873260d |
| Recorded actual itch downloads | Android byte-exact APK; Windows all 96 payload entries match. Windows distribution is directory payload, not ZIP bytes. Android build2090804 / Windows build2090803, recorded verification 00:19:54.9194937 UTC October9. |
| Recorded signed/packaged normal RT smokes | SM-S948B Android16 signed Shipping and Windows Release Pipeline successful RT-produced presentation. Bounded startup/menu scope; no sustained FPS or new owner-feel certification. |
| Historical db62032d technical validation | Debug/Release 144/144 each, 13 actual RT captures, Android Debug/unsigned Shipping four-ABI builds/lint and seven Host stages. These were not performed by WP0. |
| Historical merge CI | `gh run view 37863321948 --json conclusion,headSha,jobs`: a3f120cf, six/six success freshly read. Current main check-runs also six/six success, read with `gh api .../commits/1c28d056.../check-runs`. Remote CI is not local execution or physical RT proof. |

**Updater housekeeping:** `gh api repos/Samfa12-tech/The-Horde-RT-demo/releases --paginate` currently lists v1.6.1 and v0.1.3-alpha.1, no v1.6.2. Current `GitHubReleaseUpdater.cpp` reads/filter-selects GitHub releases, not itch. Older builds cannot discover itch-only 1.6.2 with it. Owner may authorize release housekeeping later; no release/tag/updater change here.

Retain release limits: forgiving contact/owner-deferred long warm programme, no sustained30FPS certification, bounded body/mirror/motion evidence, scoped controller acceptance, final S24/S25 coverage gaps and deferred polish/independent review. Never convert these to new passes.

## Available tools and finite check selection

Same Windows computer; PowerShell approved execution. Git/LFS3.7.1, Python3.12.10, VS2022 BuildTools/MSVC19.44, CMake3.31.6-msvc6/CTest, VulkanSDK1.4.350.0, Java21.0.11, existing Android SDK/NDK/Gradle cache, Blender5.2.0 LTS, gltfpack/shared import tools are present. A tool path/version does not prove a native round trip. No Meshy/voice credits, keys, account/security or owner signing/recovery material inspected.

Selected one Windows Debug configure/build/full CTest lane plus one cached Android Java/lint lane and pinned dialogue/document/contract validation. Do not rerun the release Host seven-stage programme, Windows Release/native Android matrix/captures or device work merely for prose. Check scripts/presets and relevant local Spec-Kit planning guidance were inspected; no extra scaffolding workflow was needed. Superpowers callable local skills were not found in the available catalogue. No new tests were authored for documentation.

## Current-run baseline commands and result ledger

Commands ran from the preparation root unless noted. Full lengthy logs remain in ignored `reports/wp0/`; result receipts/hashes are committed as documentation. Exact runtime/build source is `1c28d056` with the tree equivalence above. Documentation changes do not alter build source.

| Check | Command/setup | Result and evidence limit |
|---|---|---|
| Windows first configure | Existing VS CMake on PATH; VULKAN_SDK1.4.350.0; `cmake --preset windows-x64-debug` | **FAILED setup**: pinned report WebView2 SDK absent in fresh worktree. No code defect or tests run in this attempt. Retain `configure-debug.log`. |
| Documented setup correction | `tools/restore-report-webview2.ps1` | **PASSED** archive size/hash and six SDK entry pins; SDK-only developer restore, no Runtime install, credentials or source change. |
| Windows corrected configure | `cmake --preset windows-x64-debug` | **PASSED**. This is the one allowed specifically affected retry after documented setup correction. |
| Windows Debug build | `cmake --build --preset windows-x64-debug --parallel 2` | **PASSED**, current source compiled with real Vulkan SDK. Compilation is not physical RT presentation. |
| Windows CTest | `ctest --preset windows-x64-debug --timeout 180` | **FAILED original full run**, exit8: 138/144 passed, six failed, 1580.45 seconds. Two timeouts and four pointer-input tool failures; targeted setup corrections below. No claim of 144/144 current-run pass. |
| Android Java contracts + lint | From android: `./gradlew.bat --offline --no-daemon --max-workers=2 --console=plain :app:testDebugUnitTest :app:lintDebug`; existing ANDROID_HOME/SDK_ROOT/JAVA_HOME; HORDE_VALIDATION_UNSIGNED=1 | **PASSED**, 256 tests/39 suites, zero failures/errors/skips; lint zero errors/fatal, 60 warnings retained. No APK assembly/signing/install/native four-ABI build in this lane. |
| Pinned dialogue authoring | `python reports/wp0/pinned-package/docs/dialogue/en/tools/validate_bank.py` | **PASSED**, 436 static assertions, 357 lines/209 scenes, runtime_tests_run false. English JSON remains external authority; no export/audio/production. |
| Pinned package hashes | SHA256 against unchanged PACKAGE-SHA256.json | **PASSED available subset**, 409 files match; DOCX unavailable LFS pointer. Whole-package binary verification **NOT RUN/UNAVAILABLE**, never substituted pointer hash. |
| Planning/link/source custody | Git ls-tree/show/rev-parse, 13 supplied blobs and132 relative source paths | **PASSED paths/blobs**; one inherited fragment exception explicitly reconciled with source-line links. Local link/diff/protected-path checks appended at final review. |

[Baseline machine receipt](2026-10-09-horde-1.7-wp0-baseline-results.json) and [authoring/hash receipt](2026-10-09-horde-1.7-wp0-authoring-validation.json) retain actual counts and log hashes. **Not run by WP0:** Windows Release, seven-stage Host runner/capture/package programme; Android native four-ABI/unsigned Shipping/build APKs; physical Android/Windows RT captures, lifecycle/device sessions, sustained FPS/power/thermal, controls/owner-feel/listening; Kit native import/art/device check and new source binary round trips. Existing historical/remote evidence does not close those current-run gaps. No device installation or data clearing.

## Priority and dependency handoff

1. Review one early-WP2 authoritative vertical-transform fixture. It reaches support/root height, camera, body, equipment sockets, physical light and event-time listener together, preserving the zero-offset dungeon. No full loader/rope/forest architecture is selected.
2. Define logical campaign/zone/checkpoint state above GPU lifetime early; later readiness/generation/rollback and durable save ownership must agree. First prove a cheap blockout and compare small resident forest against staged preparation under measured overlap/cost before expanding art.
3. Reuse accepted timing/input/combat/equipment/UI/music, then scope genuine remaining dodge protection/teaching/contact/readability/motion/run and power work. Do not automatically bundle them with height.
4. Prepared sources/reference/narrative/music prevent duplicate production, but rights/native-fit/pixels/offline-voice/listening and combined Android/Windows gates remain. Outdoor1–5 precede art;1.8 gate6 follows accepted1.7.

## Unresolved owner decisions

- Approve the **single vertical proof scope** in the [proposal](2026-10-09-horde-1.7-vertical-transform-proposal.md), or select an isolated dodge-invulnerability alternative in a separate task. Neither is implemented/bundled by this checkpoint.
- Confirm three-slot count and manual-save-at-safe-checkpoint versus other retention policy before WP9; decide the first usable recovery/healing method and safe checkpoint health policy, retaining three vitality/no speculative flask for now.
- Tune short dodge immunity placement/duration and its post-hit-guard interaction; coherent tutorial slowdown/input clock, warnings, skeleton durability and Keeper nonfatal repel/lighting rhythm; no fixed new timing/balance chosen.
- Select player-direction clip dispositions/run control and speed policy after rig/route proof, plus separately authored carry/descent/mantle and Kit staging/appearance/gesture needs.
- Resolve Kit/source public redistribution/Meshy entitlement and separate Hotstrike history issues; final narrow recording copy/casting/accent/rights/offline audio/listening gates remain. All recording flags stay false.
- Confirm real-scale rescue fit/stone movement/route grades/lookout shell from blockout; select residency and numeric outdoor budgets only after measurements. Engine/demo target split remains a proposal, not prerequisite or architecture approval.
- Owner release housekeeping for GitHub update discovery is separate; no tag/release/updater patch here.

## Stop condition

WP0 ends at the reviewed documentation checkpoint. No runtime defect repair, next implementation, full campaign/asset cycle, PR creation/merge, release or production work follows. Commit/push only the explicit documentation allowlist to `codex/horde-1.7-wp0-readiness`; preserve the primary checkout and PR16/main. The final Git commit and matching remote branch identify the checkpoint and are returned in the handoff; this log does not embed its own recursive commit identity.

## Final documentation review and bounded baseline finding

Final documentation review verified 15 documentation-only paths, 62 local links, 155 immutable source-section line links, and 173 external references. All checked local targets and immutable source line bounds resolve; `git diff --check` passes and the protected-path diff is empty. The full source manifest retains the single inherited ROADMAP fragment defect rather than claiming original fragments all pass. No source rollback, generated ABI, release evidence, packaging, asset or production-default edit.

Original failures: `horde_rt_final_held_torch_clearance_tests` (180.03s timeout), `horde_rt_static_gltf_asset_tests` (180.02s timeout), `horde_rt_static_texture_array_tool_tests`, `horde_rt_static_preparation_lod_policy_tests`, `horde_rt_static_preparation_colliding_lod_policy_tests`, and `horde_rt_dielectric_topology_tool_tests`. The GLTF timeout initially had no captured output; subsequent direct input inspection proved its `valid-multi.glb` was a pointer, as were failed PNG/GLB tool inputs. Matching binaries were already cached.

Documented correction 1: `git lfs checkout` of the 49 explicitly listed current-baseline static-GLTF/dielectric fixtures, static-array PNGs and sword LOD1 input, verified against pointer SHA256/size. No network fetch. Retry command `ctest --preset windows-x64-debug --timeout 180 -R '^horde_rt_(static_gltf_asset_tests|static_texture_array_tool_tests|static_preparation_lod_policy_tests|static_preparation_colliding_lod_policy_tests|dielectric_topology_tool_tests)$'`: **3/5 passed**, 18.30s. GLTF passed in0.29s; LOD-limit/dielectric passed. Two checks exposed additional pointer-only boulder/source-weapon textures.

Documented correction 2: restore11 exact cached baseline texture inputs, each SHA256/size verified; `ctest --preset windows-x64-debug --timeout 180 -R '^horde_rt_(static_texture_array_tool_tests|static_preparation_colliding_lod_policy_tests)$'`: **2/2 passed**, 10.63s. Inputs/results/log hashes remain in the machine receipt. These existing tool contracts create temporary test derivatives with Blender/KTX/glTF; they are not future asset production/admission or a Kit native-fit pass.

Final observed disposition is **143 distinct checks with passing results across the full run and targeted setup retries**, with one unresolved held-torch timeout. The whole matrix was not rerun and is not described as a fresh144/144 pass. No runtime or test source, assertion or timeout policy changed. Held-torch runtime inputs were already hydrated, so it was not repeated. The smallest separate investigation is flushed stage timing and measured sweep cost in `tests/FinalHeldTorchClearanceTests.cpp` under this exact Debug/MSVC setup; reproduce before proposing a test-harness/environment correction or genuine bounded kinematics repair, preserving real-geometry clearance and acceptance. This is a failed baseline check, not a diagnosed runtime defect, and is not bundled into the vertical proof.

Final primary preservation check: main remains `c3c15d3243492c8338e37ce8312b14e62e118a0b` with the same modified ASSET_LICENSES/PROJECT_MEMORY and untracked roster recorded above. PR16 and main were not mutated. Checkpoint includes only the explicit 15 documentation paths in the source manifest; ignored build/test logs and cached inputs are not staged.
