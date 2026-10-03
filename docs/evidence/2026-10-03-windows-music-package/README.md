# Windows 1.6.1 music-package admission

Bounded local Windows Shipping/High package for the 1.6.1/code-9 candidate. This is an unpublishable validation ZIP, not a release or a device/performance result.

## Exact inputs and package

- Source HEAD: `ec13876069b4049b524c5198734c0bbbabbd039c`.
- Resource-runtime change: `3d26ad64a3db1e1e1b7965587a72fd114189bdf6` (ancestor of HEAD).
- Version contract: `1.6.1`, version code `9`; the existing release-version mutability guard passed.
- Music manifest: `assets/audio/music/what-the-dark-keeps/asset.manifest.json`, SHA-256 `672f2321b2fb429cffe9d61119103a1aa3565f355391a162497f741eec9d6e29`.
- Updated A/D audio input hashes and byte sizes are recorded in [SHA256SUMS.txt](SHA256SUMS.txt).
- Final ZIP: `C:\Dev\tmp\horde-final-windows-music-package-20261003\Horde-Lantern-RT-Alpha-1.6.1-Windows-x64-SHIPPING-HIGH-CURRENT-PLAYER-ASSETS-UNPUBLISHABLE.zip`; 116,168,306 bytes; SHA-256 `d2984666b292862c13cf1b8161ad2c085495bafb5c0d844f39538a7a3df58b12`.
- The first ZIP (`...-SHIPPING-HIGH-UNPUBLISHABLE.zip`, SHA-256 `d71101981a6035d3b5b4afd90cc9740f6288fb984783d5fa98e431c750074418`) is preserved as superseded evidence, not the final candidate package.
- Executable embedded in the ZIP: 3,448,320 bytes; SHA-256 `11e0a59b032cee25296d99cbf74d531871d98abbfe784823cc52f9b3b84dba45`.

## Admission evidence

- `tools/music-asset-policy.ps1` admitted the exact 17-entry manifest-plus-16-WAV runtime roster; every packaged music entry matched its source byte length and SHA-256. The canonical source directory/documents are absent from the ZIP.
- The existing Windows held-item package function passed against the ZIP: required runtime asset entries, GLB/WAV/KTX2 signatures, hydrated payloads, no source/high/processing files, and exact Meshy/CC BY 4.0 attribution in `ASSET_LICENSES.md`.
- ZIP inventory: 78 entries, including 17 music entries; no source, high, diagnostic, `.blend`, or `.glb.processing.json` entries.
- `tools/InspectRtPipelineBundleContainment.ps1` inspected the exact executable hash above as Windows / Shipping / High. It selected the four `shipping_high_*` / `rayquery_compute_shipping_high_*` variants; SPIR-V validation and disassembly passed, and the embedded modules report no binding 22 and no atomic instructions. Its JSON receipt is at `C:\Dev\tmp\horde-final-windows-music-package-20261003\pipeline-containment.json`.
- Incremental `horde_rt_diagnostic_window` Release build on `build/foundation-validation/20261002-183151` reported the target already built. The exact executable therefore remains the existing Release output whose source tree includes the resource commit; it was not rebuilt for this packaging task. Build output is at `C:\Dev\tmp\horde-final-windows-music-package-20261003\windows-release-build.log`.
- The stage reused non-music assets from `reports/final-integration-20261002/run-20261002-183151/artifacts/windows-stage-repair`; the final stage additionally refreshed only the world-body and viewmodel GLBs to the current receipt-pinned source files. All other entries are unchanged from the first ZIP.
- Persisted the existing pipeline-containment JSON and incremental-build log alongside a generated per-entry hash roster for all 78 final ZIP entries. [ARCHIVE-RECEIPT.json](ARCHIVE-RECEIPT.json) binds the archived raw files, roster, README, this one-time archive script, and the preserved original [SHA256SUMS.txt](SHA256SUMS.txt). This is artifact inspection/archival only, not a new build or game run.

## Scope and limits

The initial four-byte size difference was not harmless padding alone. In both files the prior-stage GLB had a JSON chunk four bytes longer and the same-size BIN chunk; decoded JSON had three changed values in accessor 7's Y/Z position bounds, and the BIN payload had 70,507 changed bytes. Accessor 7 is the second primitive's `POSITION` data (world body / viewmodel gauntlets), matching the cuff-fit asset update. The old-stage SHA-256 values exactly match each processing receipt's `previousAcceptedRuntimeSha256`; current source SHA-256 values exactly match `runtimeSha256` in each current processing receipt, and the viewmodel receipt's `pairedWorldSha256` matches the current world GLB. The asset manifests themselves describe budgets, roles, sockets and primitive semantics but do not contain file hashes. The final ZIP now contains the current source hashes for both GLBs, verified against those processing receipts, and was rerun through the Windows held-item package guard. This is the newer engineering runtime; the receipts still state that exact-device validation of the cuff change remains open.

This work did not run the game, perform another listening session, run broad host or Android validation, or test a physical Windows device. It establishes package/version/licence/music admission and static Shipping/High bundle containment only. The owner’s “A/D both perfect” report is upstream evidence, not a claim that this packaging task repeated subjective acceptance. The reused foundation stage had unrelated failures in its prior full validation; this package does not supersede or relabel that result.
