# Warmed Android route comparison recovery data

Fifteen files, about 2.8 MB, retained byte-for-byte with SHA-256 inventory.
See [the interpretation and exact artifact identities](../../ENGINEERING_1_6_1_ANDROID_RELEASE_ABBA_2026-09-20.md)
before using these numbers. This is not lantern, final-release or causal speedup
evidence. No APK, screenshot, app-private data, credential or device serial is
included. Process IDs in context samples are transient test-process observations.

- `baseline-a*.txt`: native 1.6.0 text exported from the visible report accessibility
  tree, not retyped from a screenshot; precision is three decimal places.
- `route-b*/`: exact native candidate JSON/text and final export markers.
- Context files and `run-windows.json`: observed times; thermal ranges cover full
  run windows including warm-up, not measured-phase-only windows.
- `candidate-package-containment.json`: fresh clean-source packaged ARM64 proof,
  including external validation/disassembly and Shipping/Mobile module hashes.
- `analyse.ps1` and `analysis-summary.json`: reviewed integrity/arithmetic checks
  and their successful result. A success is not proof of thermal equivalence.

To reproduce arithmetic, copy this folder to scratch and run `analyse.ps1` with
PowerShell supporting `ConvertFrom-Json -DateKind String` (7.5 or newer). The
script writes a new summary with a fresh generation timestamp, so do not expect
that newly generated summary's byte hash to match the historical result. Raw
reports must remain untouched. JSON dates retain UTC; unavailable thermal values
remain unavailable rather than becoming zero.

The stable APK hash is `52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48`.
Candidate source is `167ce8b09374bd522d7e8b2493693bc50c00931c`, APK
`02112a48aea45431f52270ab9ee82d68091497aed0816dfcc359ba4068ee24bc`.
Both ran the original two-lap 1,838-measured-frame route at 76%; this must not be
combined with the separate 100% lantern dataset.
