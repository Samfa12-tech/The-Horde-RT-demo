# September 20 recovery evidence

These 26 selected reports are preserved byte-for-byte for GitHub recovery.
`SHA256SUMS.txt` records their hashes. They contain benchmark/renderer data, not
credentials, APKs, raw assets, signing material or device serial numbers.

- `phone/`: five complete 600-frame native reports, their completion markers,
  two intentionally invalid Home-interruption markers, and three thermal traces.
- `windows/`: two published-1.6.0 and two clean-candidate route benchmark reports.
- `parity/`: six capture manifests and the independently reproduced 15-pair
  pixel-comparison result. PNGs are not included; hashes and comparison results
  are retained, but reproducing the pixel check requires recapturing the images
  or recovering the original local `reports/parity-20260920` directory.

Interpretation and exact source/artifact identities:

- [Lantern workload and physical-phone evidence](../../ENGINEERING_1_6_1_LANTERN_BENCHMARK_2026-09-20.md)
- [Windows route A/B and its limitations](../../ENGINEERING_1_6_1_CLEAN_SHIPPING_COMPARISON_2026-09-20.md)
- [Recovery handoff and remaining work](../../ENGINEERING_1_6_1_HANDOFF.md)

The five complete phone measurements belong to APK
`0f30a72535a532f770d59f817b23934a2152f087588de95e8982698c60d80a95`, not the later
cleanup APK. The fixed Home marker belongs to cleanup APK
`faf42c587379238796bfae987382386ea31028a643a9a28f6081827aadfc0eb1`.
Windows candidate route data belongs to clean source `62ed1f1` and EXE
`d0911c2917e848363eab8833619928869d67c397439f53378061ce433d07a0df`.

Complete reports do not prove glass correctness, a speedup, matched thermals,
final release readiness or owner acceptance. The image parity manifests retain
nonzero Diagnostic glass findings; Shipping's unavailable counters are not zeros
to use as physical-correctness evidence. See the linked records before reuse.

Large local build artifacts, screenshots and scratch investigations are not a
GitHub backup. The source, tests, detailed findings and this structured evidence
are sufficient to resume engineering without silently inventing lost device runs.
