# Held-high source-baseline A/B recovery data

Twenty evidence/script/image files are retained byte-for-byte. SHA256SUMS.txt
binds their original bytes. Two PNGs use the repository's existing Git LFS policy;
they preserve the exact screenshots needed to reproduce the checked scene-region
comparison. APKs, other screenshots and build caches remain local-only.

Read [the exact-artifact report and limitations](../../ENGINEERING_1_6_1_LANTERN_ABBA_2026-09-20.md)
before interpreting the results. The 1.6.0 side is a narrow **source-baseline
harness**, not the unmodified public APK. Neither arithmetic nor matching one
frozen image proves complete glass correctness or a causal optimization.

- Baseline text is the native report read from its visible accessibility tree,
  not copied from private app files or retyped from screenshots.
- Candidate JSON/text and completion markers are the exact native exports.
- Whole-run context includes startup and warm-up. The rejected task-bring-to-front
  launch is recorded but excluded from every accepted timing window.
- `measured-baseline-package-proof.json` belongs to installed APK `5f9253…c4fee2`.
  `uninstalled-aligned-baseline-package-proof.json` belongs to later APK
  `4cbd08…5a3461`; **no device timing is assigned to that rebuilt artifact**.
- `analyse.ps1` checks workload/build/settings/zone/sample/identity contracts and
  reproduces descriptive statistics. `compare-presented.py` reads the two PNGs
  and checks the unobscured region below the changing HUD. It does not edit images.

For reproduction, copy this folder to scratch; fetch LFS objects if needed, run
the PowerShell analyzer (7.5+ for DateKind String), and run the Python comparison
with Pillow. New summary timestamps change the generated summary hash, not the
raw reports. No credentials, device serial or private app data are included.
