# Rescue exterior handoff and graphics navigation correction — 10 October 2026

The unchanged owner retry reproduced the renderer stop immediately after the climb reached the exterior. Its actual error was a **2.245027 m right palm socket miss**, against the unchanged **15 mm** limit. The hand target was below the player model while its reachable socket remained above. This is an identified rig preparation rejection, not evidence of a Vulkan device loss.

Source candidate: `d40d75b349c146d9096a768b393705ccc2f9f0fc`, continuing `codex/horde-1.7-wp2-vertical-proof` / draft PR27 from `2463dbbe1e18594f9b8c7af3a3c5ce90fc7c6b9b`. No checkout reset, new executor, merge or release occurred. Primary unrelated tracked/untracked work and other worktrees were left intact.

## Reproduction and correction

The old executable was source `53347791b0bd1331219651b01ea3012bafcab26d`, SHA-256 `79e1e8634717bb563928611553019506288a78ce0df2cd1032d7a164e45a6a75`. The owner-authorized retry used `--development-rescue-journey --development-keeper-practice`; this existing practice start is a recorded setup difference from the first run, avoiding the waterfall torch sequence without changing the production Keeper gate. It exited **1** after dialog dismissal. [Sanitized exact error and private raw-log identities](../../evidence/2026-10-10-rescue-exterior-handoff-fix/unchanged-candidate-reproduction.json) retain the failure separately from later passes.

`SwordOverheadLowering` treated tomb roof footprints as overhead even when their entire volume was below the exterior support (2.05 m). The rope pose override hid the impossible ordinary sword target until equipment ownership returned. The fix excludes a roof only when `roof.topY <= playerSupportWorldY`. Above-foot roofs, wall retraction, ordinary floor behavior and all tolerances remain active.

The actual imported-rig regression now exercises the **first restored UpperSafe frame**, four yaws × three pitches, and restored LowerSafe after descent. It failed in **13 exterior poses before the fix**; final result is **16,455 cases / 955 sampled poses / zero failures**, with the same socket tolerance and rope reach bounds. No renderer guard was relaxed. Ordinary GUI failures now persist their actual frame error, support, rescue phase and equipment ownership in `reports/windows_render_failure.txt` before showing the error dialog.

Graphics-menu controller/keyboard arrows now use current native control rectangles, preferring aligned rows/columns and nearby direction-cone neighbors. They do not wrap across unrelated rows. Disabled/hidden controls are excluded, slider adjustment takes precedence, Tab retains cyclic navigation, and non-graphics menus/gameplay mappings are unchanged. Named rectangle negatives and wiring checks retain the intermediate failures: one new expected-neighbor assertion was corrected to the actual nearby diagonal; the Tab guard was made independent of checkout newline style. Independent read-only review found no further implementation issue.

## Available host/package validation

- Windows Debug and Release native builds pass. **40/40 Debug**, **39/39 Release** affected tests pass; the configuration-independent Windows GDI graphics contract ran once in Debug. Selection was checked against registrations and uses fail-on-empty.
- The original full torch test remains unmodified: 3,287 poses / 1,812 roof-entry skinned poses / 3,178 plane / 57,970 disk samples. Fresh Debug **165.18 s / 14.82 s margin** and Release **16.21 s / 163.79 s margin** pass; the old 169.73 s Debug / 10.27 s margin is historical only. Both focused invocations retain `--timeout 180`.
- Offline Android Debug and unsigned Release native builds execute for arm64-v8a, armeabi-v7a, x86 and x86_64. Unchanged Debug/Release lint tasks and 269 Java tests across 40 suites are **UP-TO-DATE cached results**, not freshly executed Java tests.
- Fresh eight-key Pipeline catalogue compilation, generated adapter and eight-key RayQuery checks pass. Shaders, ABI, budgets, packages/catalogues and assets are unchanged; no regeneration was required.
- Actual newly built Windows Diagnostic/High and Shipping/High executables contain exactly their selected Pipeline/Query pairs. Positive and negative containment/registration tests pass. Android shader payload inspection covers **ARM64 only** for Diagnostic/Mobile and Shipping/Mobile. All four ABI libraries in both APKs separately match stripped bytes and ELF architecture/16 KiB alignment; this is not all-ABI shader inspection.

The [receipt](../../evidence/2026-10-10-rescue-exterior-handoff-fix/receipt.json) and [complete pass/failure ledger](../../evidence/2026-10-10-rescue-exterior-handoff-fix/runs.jsonl) provide exact commands, source/diff identities, exit statuses, timings, sanitized logs and hashes. The cancelled sandbox MSBuild invocation is retained as setup failure; the host-toolchain rerun and final builds are separate evidence. It is not counted as a failed gameplay assertion or used to explain the owner crash.

Exact built artifacts (SHA-256):

| Output | SHA-256 |
| --- | --- |
| windows_debug | `2e842b0b4fad29e0156c9d0fc4248739b1b67432d84eb808e225c22eece67521` |
| windows_release | `0569afe7323b10d4301331f57731e3fd2e5cf9081232b86a1ddc9122fa2c06a7` |
| android_debug | `248921259683b92c9fa1f71fcaf88f21fa751e36405a3150729e0dc6c965f005` |
| android_release_unsigned | `873fadbd480711d3e7a84bd0cdcff8f31b6fff31f1b36b3c69ccdfcefd732ac4` |

[Immutable source correction](https://github.com/Samfa12-tech/The-Horde-RT-demo/commit/d40d75b349c146d9096a768b393705ccc2f9f0fc)

## Owner acceptance and next playtest

The owner's old-candidate feedback remains scoped: rope deployment/player bump and climb speed positive; L3 sprint confirmed; graphics caption fit good; controller focus order frustrating; rope/mist shadow artifact not noticed on that run. Hand cadence remains tuning feedback and is unchanged. These observations do not transfer to the new candidate.

**HOST/PACKAGE-READY for this correction. DEVICE-VERIFIED: NO. OWNER AUDIO/HAPTIC: PENDING.** New candidate Windows/Android backend presentation, visual coherence, repeated traversal, lifecycle and listening remain open. WP3 combined workload/budget acceptance, actual outdoor mist/fireflies, general player falling physics, final animation clips and Kit voice/subtitles remain outside this correction.

Exact Windows Debug launch from the worktree root, only when the owner is ready:

```powershell
& .\build\presets\windows-x64-debug\Debug\HordeLanternRT.exe --development-rescue-journey --development-keeper-practice
```

1. Defeat the practice Keeper and collect the lantern. Let the rope unfurl, approach the visible prompt and use **left-click/A** to climb.
2. At the top, allow rope release and equipment restoration; look up/down and around. Verify the sword hand and lantern/light remain attached and the game continues. Descend, then repeat the round trip.
3. Pause into Graphics in a window. Try controller directions across the displayed rows/columns and the resolution slider; ensure focus stays sensible and text remains readable. Return to play and check focus/pause recovery.
4. Note any roof clipping or rope/mist shadow issue. Listening/haptic and other backend/device/lifecycle cases remain separate checks.

No new game/device launch or installation occurred during the correction validation.
