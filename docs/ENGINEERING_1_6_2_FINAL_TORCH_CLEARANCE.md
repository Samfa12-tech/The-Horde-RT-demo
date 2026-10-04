# 1.6.2 final rig torch clearance checkpoint

The shared target-only clearance tests missed an imported-arm reach regression. Low-roof carry lowered the fully extended anatomical grip outside the actual rig's admitted reach. A bounded horizontal retraction in `HeldItemKinematics.cpp`, before recalculating world-down clearance, fixes the final socket without relaxing the existing 15 mm grip tolerance. No scene, platform, shader, asset, combat timing or rig solver change was needed.

## Evidence and decision

- Inspected the actual `02-worst-bend.png`, associated OBJ and capture metadata. Its recorded camera is (4.2, -10), yaw 0, pitch -0.04, animation time 0, anatomical body, 960×540 hardware RT pipeline. The apparent nearby flame is not proof of intersection. That pose already has 89.0149 mm of main-envelope roof headroom.
- Production skinning commits the solved grip socket; the torch mesh, original flame socket, original light socket and their shadows use that same final frame. The defect was a target the admitted arm could not reach, not separate torch/light placement.
- Actual-rig prepatch sweep: **352 cases, 51 failures**. Forty-five poses exceeded the unchanged 15 mm grip tolerance (up to 73.781 mm in the recorded diagnostics); six accepted poses retained only 24.6851–27.6977 mm of clearance against the required 30 mm reserve. These positive headroom values do not establish solid geometry penetration.
- The patch retracts the complete hand/item frame along horizontal player-forward by `min(0.28 m, initialLowering × 0.45)`, then reevaluates the shared roof at that retracted grip and applies world-down lowering in the view basis. Rechecking the retracted position also covers retreat into an exit lintel. Open-space, legacy mount, dropped torch and reward-lantern ownership remain governed by their existing paths.

## Checks actually completed

`FinalHeldTorchClearanceTests.cpp` loads the actual admitted player rig, viewmodel and torch mesh. It reproduces the production CPU view-to-grounded-model conversion, calls the actual rig solve and production item/socket composition, and checks the final mesh vertices, conservative visible main-flame domain and light point against shared physical overhead footprints. It checks final strict grip agreement rather than only target placement. This CPU reconstruction duplicates the production coordinate conversion; future scene conversion changes must update this fixture. The optional exact capture cross-check establishes agreement for the recorded baseline pose, not every possible runtime state.

| Check | Observed result |
| --- | --- |
| Final actual-rig sweep | 1,475 cases, zero failures; minimum headroom 32.2371 mm; maximum left-grip error 0.0121731 mm |
| Sweep coverage | Three portal approaches, 13 positions, four yaws, three look angles, multiple gait phases, idle/attack/parry; seven attack/parry transition phases at two look extremes and five locomotion blends; recorded baseline pose |
| Optional actual capture comparison | Grounded model/world transform maximum error 0; all 15,855 exported viewmodel positions agree within 0.001073 mm |
| Existing target/continuity fixture | 7,272 portal cases plus 401 walk/look samples pass; maximum sampled hand step 28.2601 mm |
| Existing player animation/rig fixture | PASS, including IK, socket, import/reset, portrait visibility and delivery contracts |
| Existing shared gameplay fixture | PASS, including commands/events, capture seam persistence and retry |

The existing three fixtures passed in 11.67 seconds in an isolated standalone CPU build. Final actual-rig fixture compiled under MSVC 19.44.35227, C++20, `/W4 /WX /permissive-`, with `/wd4324` limited to the preexisting intentionally aligned GPU ABI padding diagnostic. It links the existing Debug static libraries and directly compiles the changed kinematics file, matching `/MTd`. No Vulkan device or GPU work was performed by this audit.

The visible flame check includes a 0.4 m local vertical extent above the flame socket and ±0.105 m lateral domain. The main shader flame maximum is approximately 0.3432 m; the remaining height is a conservative tip reserve. Procedural smoke/embers above that domain continue to use physical depth occlusion; the fixture does not claim all particles are contained within the main-flame envelope.

## Durable receipts

Workspace-only receipts are one directory above this checkout; they contain no private report images or Gmail identifiers:

- `final-torch-prepatch-evidence.log`: complete expected-failing 51-case receipt, SHA-256 `73682cfd571fe797b54dcc7b487e0892c99b01e619d946e6228f2c3f20c26d86`.
- `final-torch-prepatch-held-kinematics.cpp`: temporary inverse of precisely the retraction patch, SHA-256 `b3f3a2874a8744f4d76993043ed47525c7596e45d2379af404867736abca1429`. The shipping runtime source was never reverted to produce this receipt.
- `final-torch-evidence.log`: patched full sweep and optional OBJ comparison; `final-torch-existing-evidence.log` and the standalone `Testing/Temporary/LastTest.log`: affected existing checks; `final-torch-build.log`: compilation receipts.
- Linked CPU libraries: `horde_rt_probe_core.lib` SHA-256 `eae401bdd01581567ae9b01e272438d6e9d0fe5c54fe58d6bff94c959e43c259`; `horde_gameplay_simulation.lib` SHA-256 `a287c807f83c40ff368c2124bc269754eabcd663bf6150b1c2cf5ac133cad21d`.
- Recorded OBJ SHA-256 `c6be4101a2eddb343a41eb18838e059eb0b8bd16cbf5e311b5d3949d5f0bc3c7`; PNG SHA-256 `47a539bc53168350f4fd1932e8624c96441245f5c68d44eb3e1185e5ea72b0e6`. Capture files remain local diagnostic evidence and are not required for CI.

Default fixture invocation is `<fixture> <repository-root>`. The optional diagnostic mode is `<fixture> <repository-root> --capture-obj <local-obj-path>`; a requested missing/mismatching capture fails. `--original-sweep` selects the original 352-case input set, primarily for the preserved inverse-patch negative receipt.

For native CMake admission under Vulkan targets and `BUILD_TESTING`: create `horde_rt_final_held_torch_clearance_tests` from the new test source, link `horde_rt_probe_core`, and register CTest with `${CMAKE_CURRENT_SOURCE_DIR}` as its sole argument. Do not register a capture-file dependency or the deliberately failing inverse-patch binary.

## Exact next action and acceptance limits

Source is stable for lead integration. Lead registers the fixture and rebuilds the native app, then captures the corrected shared pose through the actual RT path after the separately owned renderer-mode slice is ready. GPU visual acceptance and owner handheld motion/torch acceptance remain open. No new postpatch device or RT presentation claim is made here.

The Android camera APK completed at UTC 07:48:32, before this kinematics patch at 07:49:44. The Android owner verified all cached kinematics objects predate the patch; that sealed APK (hash prefix `071ca97a`, source `3500099`) and earlier captures are explicitly **prepatch**, not evidence for this correction. Phone ownership/allocation is controlled by lead; lead's latest message reallocates the SM-S948B Android 16 device, but this agent has no device/GPU authorization and has performed no device actions.

Later Android build follow-up: the Android owner sealed a new APK containing kinematics source SHA-256 `3a61fd092318f859ab97faf6465b28a1e5688a37d2e01badaf7a654dcfbbde09`. Incremental assembly passed for all four ABIs in 41 seconds; all 294 recorded runtime inputs were identical before/after assembly, and native ELF architecture/16 KiB alignment plus packaged asset/notice admission passed. APK SHA-256 is `b4e063afed5ea46be1467870a0d96adbb1a7e731e6bda8f978c2d4401f45e8ee`; workspace-only receipt is `candidate-android-torch-receipt.json`. This is packaged build evidence, with postpatch RT presentation and owner motion acceptance still pending.

Separate source-witness follow-up requested by the graphics owner: Windows output resize was extracted unchanged into `ApplyPendingOutputResize`, called by the normal render loop. The character smoke resize predicate now scopes its order checks to this helper, verifies the main-loop call precedes rendering, and retains evidence completion → epoch recreation → timer reset → output resize, prior-tuple rollback, failed acknowledgement and no scene destruction/reinitialization. Android resize checks remain unchanged. Only the predicate was edited; compilation/CTest are delegated to the lead's current native build allocation.
