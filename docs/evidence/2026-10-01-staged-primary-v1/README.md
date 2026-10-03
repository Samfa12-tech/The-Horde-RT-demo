# Staged primary v1 — first candidate evidence

Normal control anchor: `7ab8607e3f3297444dc17fcac9344b29b8d8ccce`.
The candidate was built in the dirty engineering worktree before its first
reviewed commit. `source-artifact-receipt.json` preserves the runtime source
fingerprint and exact Windows binary identities, not a retroactive clean-build
claim. Capture and shader manifests preserve their individual hashes. Source
fingerprints exclude unrelated scratch files; nothing there was removed.

## Results and limits

- Diagnostic/Mobile and Shipping/Mobile-policy RTX5050: 20 control/candidate
  native image pairs each. All 13 standard captures pass unchanged RGB3 /
  fraction over1 <=0.001; all seven focused PNG pairs are byte-identical.
- The comparator **overall results FAIL** on timing: +74.498% Diagnostic and
  +71.656% Shipping-policy capture-loop medians. These are fresh-process Windows
  capture-loop observations, not matched warm Release GPU or phone comparisons.
- Shipping-policy captures use the Debug native automation shell; Release
  correctly refused capture automation (exit2). Candidate Diagnostic/Shipping
  standard images are identical; that comparator's timing gate fails +3.657%.
- Actual three-page RTX allocation: 66,355,200 bytes, memory flags1 DEVICE_LOCAL.
  132,710,400 nominal read+write bytes/frame is arithmetic, not DRAM traffic.
- Actual Android benchmark APK `fe116182…887d62a`, ARM64 native library
  `bfadc512…a6b1093`, and Windows Release EXE `b8173849…90ea52` are in the module
  inspection receipt. Both native binaries contain the eight expected modules
  exactly once; SPIR-V validation/disassembly pass, zero atomics/no binding22.
  This is an isolated development-signed investigation APK, not normal Shipping
  admission. The closed production four-module inspector was not weakened.
- Fresh affected Debug/Release tests 3/3 in each control/candidate configuration;
  CPU fake-resource owner fault injection 1/1 in both Diagnostic/Mobile trees.
  Memory-sidecar 34 assertions and shader-generator 16 fixtures pass. Fake Vulkan
  or ADB fixtures are explicitly not physical-device acceptance.

External immutable artifacts/raw logs/PNGs are retained under
`C:/Dev/tmp/horde-staged-rt-20261001/`; compact receipts and comparison results
are committed here. The shader tool's later external-output-root safety guard
changed the generator hash, not shader transformation. Earlier manifests retain
their original generator identity; no unchanged modules were rebuilt merely to
replace metadata.

Known phone physical failures/recoveries and backend parity remain open, not
cleared by zero-event frozen RTX fixtures. High image checks, per-pass timestamps,
warm SM-S948B timings and memory-counter discovery remain required. No phone was
connected or operated for this evidence. No promotion, merge or publication.

One durable run matrix and the next unfinished step live in
[the experiment record](../../ENGINEERING_1_6_1_STAGED_MOBILE_RT_2026-10-01.md).
Audio/haptic manual revalidation: NO; rendering organisation changed, not semantic
inputs or playback. Separate music listening gates remain open.
