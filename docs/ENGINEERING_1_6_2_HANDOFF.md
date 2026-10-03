# Horde 1.6.2 execution checkpoint

Read this first after compaction. Current source and explicit owner instructions govern.

## Identity and workspace
- Implementation branch: `codex/horde-1.6.2-engine-readiness`.
- Isolated worktree: `C:/Users/sam_s/Documents/Codex/2026-10-03/task-4/source`.
- Baseline main: `1df058b77baacaab76dc112f578deb77ddb791e9`; fetched origin main matched.
- Frozen released package source: `a397757249871b6b64fe5b77fc14f24e8cfcbb2b`.
- Planning source: `34f78b4163ee6c698bd546c864345e1845aaf2cb` (PR16).
- Imported only docs/ROADMAP, IMPLEMENTATION_GOAL, UI_REFRESH, COLLAPSED_ENTRY, LICH_REVEAL and CAMPAIGN_DESIGN from planning source. Runtime, AGENTS and historical evidence remain main's.
- Original checkout's modified ASSET_LICENSES/PROJECT_MEMORY and untracked tool/audio work remain untouched.
- No other local or remote 1.6.2 implementation branch/PR found at intake; PR16 is planning only.

## Current next action
Integrate/review Phase1 audits, finish A4 Windows output-only resize, compile targeted host checks. Do not start new graphics implementation before the five audit corrections are checked. Audits/design for later phases may proceed independently.

## Slice ledger
| Slice | Owner | State | Next action/dependency |
|---|---|---|---|
| P0 baseline/intake | lead | implementing | Complete current controls/assets inventory and exact Windows baseline capture |
| A1 present semaphore | audit_sync | implementing | Review per-image lifetime patch; targeted platform builds and lifecycle checks |
| A2 scratch/SBT | audit_alignment | implementing | Review padded aligned allocation and all build/update paths; boundary/fault tests |
| A3 notices | audit_notice_report | implementing | Bundle exact cgltf licence in Windows/APK; package-byte tests; lead integrates Windows credits |
| A4 Windows resize | lead | not started | Switch compatible scale transactions to ResizeOutput after A1 releases platform files |
| A5 report boundaries | audit_notice_report | implementing | Bearer token boundary fixtures; no live submissions |
| P2 graphics/preview | graphics_design (audit) | not started | Concrete shared configuration/isolated preview design; then implementation |
| P3 material/temporal/shadows | material_audit (audit) | not started | Exact normal/tangent/tiling audit and bounded feasibility decisions |
| P4 audio/reveal | audio_reveal_audit (audit) | not started | Verify owner sound inventory, waveform/loop facts and reveal state interfaces |
| P4 scenes/UI/fire/clearance | lead | not started | Actual screenshots inspected; native baseline + layout/contact sheet before gated composition |
| P5 integrated candidate | lead | not started | Combined reviewed branch, exact evidence and current CI |

## Evidence and decisions
- The three authorized private screenshot attachments were retrieved from fresh message metadata and visually inspected. Raw reports/images remain outside the repository in temporary storage; no private identifiers/links are recorded here.
- Screenshot observations: rear cap is flat masonry behind spawn; torch top is near the lintel/low overhead brick on approach; shaft has an abrupt rectangular sky opening above pale vertical structures. Exact scene anchors must be resolved from code/runtime.
- Attachment downloader failed creating its local directory (Access denied); direct temporary download succeeded. This intake blocker is resolved.
- SDK discovery: VS2022 BuildTools CMake 3.31.6, Vulkan SDK1.4.350.0, Android SDK/NDK26.1.10909125, JDK21 installed. Local environment helper outside repository: `../environment.ps1`.
- Initial sandboxed CMake failed in existing MSBuildTemp with UnauthorizedAccessException. Elevated standard configure is running; this is an environment failure, not a compiler regression.
- Git LFS status can write common `.git/lfs/tmp` outside writable root. Use scoped read-only diffs or authorized elevated Git for integration; do not disable LFS and accidentally stage runtime asset rewrites.
- Rejected scope expansion: mandatory maintenance1 capability changes for A1. Use bounded per-image ownership and existing drain, disclose formal unextended present-completion limitation and physical validation gap.

## Device and acceptance ownership
- Phone is with Sam and NOT allocated. Do not install/test/clear data. Continue host work.
- No existing Horde/Blender/Vulkan GPU task observed during intake. Reserve RTX use only for bounded coordinated lead runs; child agents do CPU/source work.
- Required owner gates: collapse layout/Form before final gated composition, final scene/UI/fire/keeper motion acceptance, applicable changed-audio/haptic listening.
- Audio/haptic manual revalidation required: NO for Phase1 safety/notice/report/resize changes; YES when cue/event/playback/reveal audio changes are integrated.
- S24 final-release coverage deferred; S25 unverified; historical S26/RTX evidence does not validate this candidate.
- No paid assets, credential/security changes, production signing, merge, tags or publication authorized.

## Validation commands
Run from isolated source. Load `../environment.ps1` for local SDK paths. Use targeted checks during slices, broaden once candidate integrated.
- `cmake --preset windows-x64-debug`; `cmake --build --preset windows-x64-debug --parallel 2`.
- `ctest --preset windows-x64-debug -R <affected-selection> --output-on-failure`.
- Release equivalents for combined candidate.
- Shader changes: `tools/compile-raygen.ps1` appropriate current matrix/catalog options, staleness + Shipping inspection.
- Android build/contracts/lint without install: `android/gradlew.bat :app:testDebugUnitTest :app:lintDebug :app:assembleDebug --console=plain` with `HORDE_VALIDATION_UNSIGNED=1`.
- CI `.github/workflows/shared-simulation-host.yml` currently required six checks; candidate results must be fresh, not copied from1.6.1.

## Phase1 checkpoint — host verification
- Five source findings revalidated against current main and implemented; A1 formal unextended shutdown present-completion caveat remains explicitly open.
- Full Windows Debug configure/build PASS (`../debug-build.log`); initial SDK restore verified six pinned WebView2 entries.
- Affected CTest subset PASS7/7 (`../audit-ctest.log`): address-layout, notice admission, player package inventory, report text, native report UI, GPU resource mocks, scene resource inventory/output-resize transactions. No missing test is counted as a pass.
- Separate notice admission13 cases PASS and Android `:app:prepareRuntimeAssets` PASS; actual APK/ZIP byte admission remains for candidate packaging.
- A4 output-only resize retains BLAS/assets/descriptors unrelated to output, restores saved selection on allocation failure, logs `idle_and_resize_ms` independently. Live repeated resizing still pending.
- Phase2/3/4 source audits complete: see GRAPHICS_DESIGN, MATERIAL_AUDIT and AUDIO_REVEAL_AUDIT. Their proposals are not runtime/device passes.
- Next action: pin this audit slice, use its exact Windows Debug binary for pre-polish RTX views/sync checks, then implement shared graphics/config, compact preview, material normal/scale/AO and deterministic keeper reveal with separate file ownership.
