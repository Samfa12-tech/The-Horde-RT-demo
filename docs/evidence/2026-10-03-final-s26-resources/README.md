# Final S26 resource comparison record

Finite October 3, 2026 S26 comparison on `SM-S948B` (`R5GL219SZGK`). The device work is complete; this record archives the completed scheduled reports and bounded memory samples. No new benchmark was run for closeout.

## Scope and identities

This compared the existing dynamic mapping and immutable placement organization with unchanged Mobile quality, geometry, shaders, animation, music, and 75% render scale. Both artifacts used the open-aperture profile and `RayTracingPipeline`; neither used the wrapper's physical-control label. It does not test physical glass or a different optimization, and it does not certify sustained gameplay.

| Member | Source | Installed APK SHA-256 |
| --- | --- | --- |
| Control | `aa2000b03f32c7d80097ffe84f966e5298f300e6` | `5c55552866d359dd158ef26821a9c25a533c7048b7e1884ffe38c793aa68dcd3` |
| Candidate | `3d26ad64a3db1e1e1b7965587a72fd114189bdf6` | `5f05a13102a6bf2205cb9af38124139469dd7605a40e3e99217570d835b7c70e` |

The artifacts predate music commit `ec13876069b4049b524c5198734c0bbbabbd039c`, which changed the A/D WAVs. Treat these as pre-music-change resource measurements, not as the current music build or final release. The owner accepted the A/D music separately.

## Completed reports and admission

All six completed reports say `status=complete`, `presentedEveryFrame=true`, 75% scale, and `RayTracingPipeline`. In each report, expected, completed, CPU-accepted, and valid-GPU counts agree exactly; rejected, cancelled, outstanding, missing, pending, error, disabled, and unsupported rows are zero. Each completed frame has an RT duration and reached presentation. The earlier interrupted `final26-b-high-20261003` is incomplete and excluded from measurement and archive.

| Order / run | Member / workload | Rows accepted = valid GPU | Cycle median / p95 (ms) | GPU RT median / p95 (ms) | Context observed: battery C; thermal status; GPU power |
| --- | --- | ---: | ---: | ---: | --- |
| 1 `final26-a-route-20261003` | Control / showcase route | 1,838 | 58.1970 / 77.9315 | 47.8816 / 66.4972 | 27.2–34.1; 0; 0–1 |
| 2 retry `final26-b-high2-20261003` | Candidate / held-high | 600 | 93.6585 / 94.8007 | 92.0212 / 92.7630 | 31.4–36.9; 0–1; 0 |
| 3 `final26-b-route-20261003` | Candidate / showcase route | 1,838 | 67.8279 / 92.0503 | 57.8481 / 81.6040 | 33.4–35.8; 0; 0–6 |
| 4 `final26-a-high-20261003` | Control / held-high | 600 | 93.6380 / 100.7332 | 92.1011 / 99.3686 | 33.8–38.4; 0–1; 0–1 |
| 4a `final26-a-route2-20261003` | Warm control / showcase route | 1,838 | 70.1888 / 96.6234 | 59.0912 / 86.3764 | 34.2–36.2; 0; 0–9 |
| 5 `final26-b-live-20261003` | Candidate / lantern reveal sequence | 600 | 121.2535 / 129.6016 | 109.3299 / 117.6675 | 39.1–40.7; 1–2; 0–10 |

Thermal columns summarize the entire separately sampled startup, warm-up, and measurement context; they are not frame-aligned. The live reveal-sequence run began warm after extended phone music listening. Report it as standalone candidate workload data, not a paired gain. The interrupted first `final26-b-high-20261003` attempt remains incomplete and is not represented in the table.

## Comparison and resource findings

The heavy held-high pair is effectively unchanged: candidate cycle median 93.6585 ms versus control 93.6380 ms (+0.0205 ms, about +0.02%); candidate GPU median 92.0212 ms versus control 92.1011 ms (-0.0799 ms, about -0.09%). This is no meaningful speedup.

The first route comparison raised a greater-than-15% warning: candidate cycle median 67.8279 ms versus the original control's 58.1970 ms, and candidate GPU median 57.8481 ms versus 47.8816 ms. That pair is not matched: candidate GPU thermal power reached 6 versus 0–1, and the candidate route had active RAM sampling while the initial control route did not. Opening-only medians were 90.7014 ms candidate versus 75.9852 ms initial control. Skin and upload medians were similar (candidate total skin 8.6298 ms and dynamic upload 0.3200 ms; initial control 8.5105 ms and 0.3268 ms), so these CPU stages do not explain the route timing difference.

The added warm control route narrows the comparison but does not establish a gain. Candidate route cycle median / p95 was 67.8279 / 92.0503 ms versus warm control 70.1888 / 96.6234 ms; GPU median was 57.8481 versus 59.0912 ms. Opening-only cycle medians were 90.7014 versus 95.2757 ms, and GPU medians 79.9928 versus 84.5273 ms. Those differences are below 15%, but the run order was interrupted rather than interleaved and candidate and control thermal ranges differ. Do not interpret the lower candidate medians as a proven improvement. The long pre-run music listening preceded only the standalone final reveal-sequence run.

CPU skin and dynamic-upload medians for the warm route control were 8.6412 and 0.3214 ms; candidate route values were 8.6298 and 0.3200 ms. The live reveal sequence measured 8.5479 ms skin and 0.3412 ms upload median. Held-high rows report near-zero skin/upload work because that frozen heavy scene does not exercise the route's animated skin path. These CPU values are separate from GPU RT duration.

Four complete RAM sidecars each contain three scheduled samples. They retained process/native/graphics memory, thread counts, system memory pressure, and bounded `meminfo`, `procStatus`, pressure, and global-memory snapshots. PSS / RSS / native-heap PSS / Android `Graphics` allocation ranges in KiB, plus observed process thread counts, were:

| Sidecar | Process PSS | Process RSS | Native-heap PSS | Android `Graphics` allocation | Threads | PSI `some/full` avg10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Candidate held-high | 556,847–567,119 | 657,320–667,484 | 206,522–215,946 | 300,768–301,024 | 29 | 0 / 0 |
| Candidate route | 498,618–547,158 | 600,712–648,424 | 145,913–192,553 | 302,704 | 34–35 | 0 / 0 |
| Control held-high | 545,983–546,845 | 648,912–649,696 | 197,739–200,271 | 300,768–301,024 | 30 | 0 / 0 |
| Warm control route | 538,471–548,664 | 641,928–652,120 | 187,927–196,871 | 302,704–303,216 | 35 | 0–0.01 / 0–0.01 |

These samples are before/after or scheduled boundary observations, not frame-aligned memory measurements. Their 0.59–0.76 s collection times are observer work. Pressure totals are system-wide and cannot be attributed to the game. The Android `Graphics` allocation and thread counts are coarse process/system observations; they are distinct from hardware GPU memory accounting and traffic. No GPU memory, bandwidth, cache, or stall counters were collected or inventoried by these trials; they remain open evidence gaps. The old 128-byte staged-record estimate in sidecar metadata is analytical only and is not an allocation or measured traffic in this candidate.

All route, held-high, and reveal-sequence medians are above the 33.333 ms 30-FPS reference. These are short workload comparisons, not a 30-FPS pass, sustained-performance claim, or proof that RAM is or is not the bottleneck. No quality or resolution change was used.

## Archive and remaining evidence limits

`raw/` contains the allowlisted byte-exact `trial.json`, `context-samples.jsonl`, and `result.json` for each completed report; each full `benchmark.json` is retained as deterministic `benchmark.json.gz`. The four named RAM sidecars retain their exact receipt, samples, and only the three per-sample memory/pressure snapshot sets. `SHA256SUMS.txt` binds each exact raw input and archived byte sequence, plus the README summary and archiver script hashes. The archiver checks complete run identity, frame admission/presentation/GPU validity, sidecar completeness, and gzip byte-for-byte round trips.

The interrupted `final26-b-high-20261003`, including its partial output, is intentionally excluded. The archive also excludes installed APKs, ADB diagnostic dumps, SurfaceFlinger app lists, and unrelated logs.

The separate current-bank Shipping-path Debug-shell motion/lifecycle/Settings
checks are now [complete](../2026-10-03-final-s26-interactive/README.md); their new
APKs are not these benchmark artifacts. Subjective and external-focus boundaries
remain separate. S24 final Shipping and exact S25 remain unavailable/unverified.
This report does not authorize merge, release, or publication.
