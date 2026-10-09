# 1.6.2 ordinary experimental resolution choices — 9 October 2026

The owner requests ordinary Graphics choices for manual 33% and 40% testing rather than extending automatic sustained telemetry now. Source `406777388e01cb702fff4475e2aa18ddb14a0353`, tree `823ad707f8d7c2afe15011fef5c50d1fcd564800`, exposes those discrete values on Android and Windows, marked **Experimental**. Both use real lower-resolution RT dispatch through the existing shared renderer and transaction. Output/native UI resolution stays unchanged. This is not an upscaler or a sustained-performance result.

Fresh/reset Android remains **50% / Mobile water and fire / Glass Off / Current shadows / cap30 / Mist On / Dust Off**. Desktop remains 100% / High water and fire / Glass On / Current / cap30 / Mist On / Dust Off. Saved/custom preferences remain authoritative. The ordinary minimum admission is now 33, admitting only 33/40 below 50; corrupt gap values fall back 50, and the established upper clamp stays 100. The explicit legacy min 50 variant is retained. Android’s standard 50–100 slider remains; the experimental buttons and Return to 50 use existing draft controls. Windows maps the discrete choices before 50–100 and preserves controller steps across the gap.

**Use → exact native acknowledgement → Keep/save or Restore**, including the normal 15-second visible confirmation, is preserved. Merely selecting a choice edits the draft. Ordinary benchmark/timing/motion adapters stay Off; isolated measurement admission remains strict. Combat, shaders, assets, audio, lifecycle and gameplay rules are unchanged.

## Exact build and checks

[Sanitized receipt](evidence/2026-10-09-experimental-scales/receipt.json) records:

- Android Debug APK SHA-256 `51fbde779f52e972d9520a8dccfa6b7c1b2526d324c9cc2a7dd6681d7c89f577`, 138,462,962 bytes. Its installed pulled base matches.
- Windows Debug EXE SHA-256 `cd789e49b2b0b710cd9566b9edce3f5529ed898718859b2c81f324b9d3a8118e`, 11,572,224 bytes.
- 252 Java tests/38 suites, zero failures/errors/skips; lint and four-ABI assembly pass. All ELF PT_LOAD alignments/offsets and ZIP payload alignment pass 16 KiB checks. All 94 runtime assets match the sealed earlier ordinary Debug artifact byte-for-byte.
- Seven affected host suites pass: shared settings and native extents, min33/legacy50 admission, preview transactions, Windows persistence, desktop controller input, and Windows glass/ACK contracts. The actual CMake admission matrix passes. Tests cover default preservation, invalid gaps, retained 33/40 saves, native extents for both backends, Windows trackbar/controller mapping and exact ACK/Keep/Restore.

Runtime [push 37835135971](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37835135971) and [PR 37835142352](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37835142352) each pass six jobs: **12/12 aggregate**. Later documentation CI is separate.

## Limited normal-phone check and owner handoff

The intended SM-S948B / Android 16 receives the exact ordinary `.debug` APK. Installation preserves both saved-settings and accepted menu-mix files byte-for-byte. Native Graphics shows `Resolution: 33% (Experimental)` and `Resolution: 40% (Experimental)`.

Each choice is selected and Used. Keep becomes enabled after the existing exact presented-frame acknowledgement, with `Keep or Restore: 15 s`; Restore is then selected without Keep. Final Details observes Requested/Saved/Effective 50%, 720×1490 internal/1440×2980 output on real Pipeline. The owner's existing custom 50% / Mobile / Glass On / Current / Mist On / Dust Low / cap30 tuple is unchanged, pending is false and the accepted menu mix stays byte-identical. Pending trial metadata changes normally; do not describe the whole post-preview preference file as byte-identical. Native lower extents are covered by host tests and earlier isolated evidence; this UI-only check does not independently capture their dimensions or close a new physical Compute gate.

The automatic owned session stops; the normal build is reopened at entry for owner manual play. No new Windows visual, physical controller, audio/haptic, sustained motion/thermal, power/memory or Keeper/reward pass follows.

## Retained corrections and next decision

Three new Java admission tests first fail as expected under the old ordinary min 50 policy. An initial runner used the wrong working directory and matched no tests. A host command initially named a nonexistent Windows target. Two new native fixture assertions incorrectly expected 101 to clamp 50; they are corrected to the established 100 upper clamp and rerun successfully. These harness/fixture failures remain recorded, rather than being labelled persistence failures.

The first phone check correctly refuses Details because that button is disabled during confirmation; no touch occurs. The corrected check completes both ACK/Restore paths and final native 50 verification, then refuses an offscreen Settings BACK target. The owned app stops and entry is reopened, with preferences checked independently. Neither guard indicates a product regression.

Keep 33/40 as optional experiments. The [earlier moving comparison](ENGINEERING_1_6_2_MOTION_PRESENTATION_2026_10_09.md) and limited owner 33% appearance acceptance retain their original artifacts and scope. The owner can now test ordinary movement, combat, thin geometry and Keeper/reward manually, including after warming. Sustained 30 FPS and a final accepted profile remain open; they do not block this menu checkpoint. Any reconstruction/upscaling decision follows that feedback. No temporal/vendor implementation, default change or release is authorised by this checkpoint.
