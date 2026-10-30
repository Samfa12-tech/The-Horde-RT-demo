# Matched phone resident versus streamed comparison — 16 October 2026

**Baseline RT rendering dominates this measured workload.** Both backends complete resident and ordinary streamed out-and-back routes on the same APK. Resident exterior GPU durations are 142–149 ms; streamed views are 138–143 ms. The measured streaming-step CPU peak is 4.4–4.6 ms. Streamed runs free13.59MiB and restore exactly, with12.85MiB observed overlap above full residency and zero readiness waits. **This is not smooth-play, complete-chapter or production-page acceptance.**

## Exact candidate and controls

Private APK `a42a0651dc9282eaceb13e1627b19e5348688127352c18f1b36be9f0c0414808` (183,633,178B), based on867b817555bae2ff18c3700cc34ab7c43197674f plus the exact archived source hashes. It is not described as a pristine later-commit build. Same existing development signer/versionCode10; install and pulled-back bytes match. The169asset payloads and actual shader modules are unchanged. Raw phoneSM-S948B, Android17/API37, Adreno840, Vulkan1.4.295. Native Debug is optimizedO2; actual shaders Shipping/Mobile. The default production profile is unchanged and this independent-page profile remains private.

Saved settings are unchanged:40% scale,576×1188 internal,1440×2971 swapchain, Current shadows, water1/fire0, cap30, glassOff, mistOn, dust1. No resolution, shader budget, sample, capacity, lighting, material or geometry reduction. Frozen budget SHA-256 remains `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`. No AS/descriptor ownership redesign. Both modes execute the same normal wall-delta simulation/input route; only the explicit resident test supplies the renderer's existing allowEviction=false. This control is guarded by the private Debug chapter-timing profile and lasts only while that run is armed and unfinished.

The route walks from the admitted reunion to the landing, descends, backtracks through the certified passage, returns and ascends. Four fixed view holds last360simulation ticks; analysis discards the first/last60ticks and all phase-transition frames. No authoritative-pose injection, forced phases/readiness, or fixed-delta override. There are zero mid-run image readbacks. The unchanged120s route,30s arming and2s terminal drain limits remain enforced. Rendered camera yaw/pitch and resource/CPU tags belong to the committed submission; strict one-to-one completed-frame/display joins and adverse-counter checks are required.

## Matched views

Entries show median actual display interval /median GPU duration, in milliseconds. GPU duration is not playable FPS. Positions are stable within each held sample set. Lower and restored landing positions/angles match exactly between modes; near-target approach holds differ by at most5.2mm, with identical yaw/pitch. All pairs use identical saved settings and exact pipeline identities (see JSON).

| Backend /view | Resident display /GPU ms | Streamed display /GPU ms | GPU median change |
| --- | ---: | ---: | ---: |
| pipeline /landing-sweep | 162.50 /142.73 | 158.33 /142.63 | -0.07% |
| pipeline /lower-sweep | 75.00 /68.47 | 75.00 /68.35 | -0.17% |
| pipeline /passage-sweep | 75.00 /65.55 | 75.00 /66.40 | +1.29% |
| pipeline /repeat-ascent | 158.33 /142.19 | 158.33 /142.65 | +0.32% |
| query /landing-sweep | 166.67 /148.83 | 158.33 /139.46 | -6.30% |
| query /lower-sweep | 75.00 /66.48 | 75.00 /66.52 | +0.06% |
| query /passage-sweep | 75.00 /64.86 | 75.00 /65.29 | +0.67% |
| query /repeat-ascent | 162.50 /147.27 | 154.17 /138.24 | -6.14% |

Pipeline starts have matching framework thermal status0 and AP/BAT/SKIN differences0.2/1.0/0.5°C. Query starts both have framework status1, with differences0/0/0.3°C. Query ran streamed first, Pipeline resident first. Pipeline ended at status1; Query remained1. These are typed current-HAL samples, not clock/power control or thermal causality. Query's approximately6% lower exterior GPU durations in the streamed run are an observation from one pair, not an attributed streaming speedup or backend ranking. No matched view regresses by the15% investigation threshold. Longer replicates/thermal acceptance remain open.

## Whole-route pacing and real memory

| Backend /mode | Actual span | Frames/display joins | Median /p95 /max display ms | Overrun delta |
| --- | ---: | ---: | ---: | ---: |
| pipeline /resident | 67.17s | 647 /647 | 83.33 /162.50 /166.67 | 265 |
| pipeline /streamed | 69.07s | 667 /667 | 83.33 /162.50 /170.83 | 263 |
| query /resident | 67.46s | 652 /652 | 87.50 /166.67 /183.33 | 265 |
| query /streamed | 68.72s | 673 /673 | 87.50 /158.33 /170.83 | 269 |

All accepted collector pending counts are zero, setup accepted-ID baselines are zero, and every completed route frame has its own actual timestamp. The full route mix differs from a single held view; do not subtract cycle durations and label the difference loading time. Catch-up overruns and poor intervals remain visible; no deadline or assertion was weakened. These67–69s observations do not establish long-duration sustained/thermal smoothness, scanout, photons or touch latency.

| Backend | Full →retired →restored device-local B | Observed peak B | Saving /peak above full |
| --- | ---: | ---: | ---: |
| pipeline | 160,831,232 →146,581,568 →160,831,232 | 174,307,392 | 13.59 /12.85MiB |
| query | 160,830,720 →146,581,056 →160,830,720 | 174,306,880 | 13.59 /12.85MiB |

Resident allocation sizes stay constant throughout. Each streamed run owns exactly one new unload/restore and zero readiness waits. The certified84,219-triangle forest-detail V/I/private-primitive/BLAS owner retires; coarse world/terrain/distant landmarks, shared textures/materials, collision, actors/held equipment and uncertain shadow/reflection/transmission contributors stay resident. CPU sources remain cached: this does not prove package I/O or CPU-cache eviction. Memory is scene-owned allocation inventory and an observed owning-frame peak, not OS/process RAM or an exhaustive GPU heap high-water. The13.59MiB saving and12.85MiB overlap are separate from the earlier Windows figures.

## Transition observations

| Backend | Streaming-step CPU median /p95 /max ms | Unload ±1s display max | Reload ±1s display max |
| --- | ---: | ---: | ---: |
| pipeline | 0.0107 /0.0396 /4.6004 | 83.33ms | 91.67ms |
| query | 0.0105 /0.0307 /4.4101 | 87.50ms | 100.00ms |

The CPU counter surrounds AdvanceOrdinaryStaticDetail: bounded preparation, allocation/copy, AS submit/nonblocking poll and planning within that call. It excludes earlier completed-owner retirement, later commit/recording and standalone AS GPU execution. It is not total loading cost. The actual presentation intervals include queue/render work. Pipeline's streamed worst170.83ms frame occurs during descent at15.75s, outside unload37.06s/reload47.53s; resident worst166.67ms. Query resident worst183.33ms versus streamed170.83ms. Transition windows include ordinary camera/scene work and do not isolate upload, traversal or filtering. The measured4.4–4.6ms preparation peak is real; it is not a guarantee that broader page coverage will be hitch-free.

## Reproduced measurement failure and correction

The first APKd599e213…ffba70 passed its Pipeline pair, but Query completed677route frames and then failed the unchanged2s terminal drain. Six unresolved IDs belonged to boot/setup measurement4, serials7–13; all677route frames in measurement6 had actual timestamps. The collector correctly rejected pending IDs. No run was rescued by deleting rows or ignoring the guard. The corrected APK attaches timing IDs only after a chapter-only run arms, with explicit regression tests; general all-presentation validation is unchanged. All four corrected runs have zero setup IDs and satisfy the original terminal guard. The failed packet remains in the archive. A still-capture preparation that rejected motion, and the owner-interrupted phone-disconnection attempt, are preserved and excluded from acceptance.

## Checks and disposition

- Native affected Release3/3 and Debug3/3 pass (motion scenario, authored-terrain journey, timing policy). Full routes at1/3/4/6 ticks and focused/matched modes at3/5/6 ticks retain ordinary input, readiness and lifecycle rules; resident-counter tests reject borrowed/nonmonotonic/unpaired history.
- Strict timing analyzer15/15 pass, including distinct comparison identity and missing/adverse timing failures. A focused read-only review found no remaining consequential issue in private mode scoping or submitted-frame ownership.
- Private candidate: all four Android ABIs assemble/lint pass. Final default Debug: all four ABIs assemble/lint and286Java tests in45suites pass, zero failures/errors/skips, private profile off. Default APKb49c4985…9fd040 is not installed.
- Final unsigned default Android Release: all four ABIs and Release lint pass, using only the existing scoped short generated native staging path. No signing/publication/install. Exact hash is in unsigned-release-receipt.json. Initial build-target/compiler-scope failures remain in the raw packet.
- Owner-approved Debug updates use the same signer/version and preserve preference bytes; the release app is untouched. All corrected run post-settings equal the original bytes. Horde Debug is stopped. Audio/haptic manual revalidation required: NO for this measurement-only change; full chapter owner acceptance remains separately pending.

## Recommendation and remaining gates

Keep the private page path experimental and preserve its working memory benefit. Next profile the existing baseline RT workload from the exact lower/restored held views (especially the exterior) before changing streaming architecture or quality. These results identify substantial resident rendering cost but do not attribute it to a particular shader, material, light or tree family. Address measured bottlenecks without reducing accepted quality; do not start asset replacement or a broad rewrite from these data. Retain the12.85MiB overlap as an explicit memory-policy consideration.

Complete-chapter routes (the previous longer120s routes failed), broader useful page coverage, package-provider/cache lifetime, actual-transition lifecycle/pressure, full matched secondary-ray/collision visuals and long-duration smoothness remain open. Current/Higher phone checks and saved Lower→Current migration remain pending. Mist28/Easy Tree/canopy/campaign saves/later village gameplay stay deferred. No production adoption, merge or release.

[Comparison data](comparison-summary.json), [candidate/source identity](candidate.json), [archive receipt](receipt.json), [raw/source/check packet](matched-phone-source-and-runs.zip), [preserved failed timing IDs](preserved-timing-failure.json). The pre-run and menu-covered post-run captures were opened locally; they do not establish a new visual/secondary-ray acceptance gallery. Existing unchanged-asset visual receipts retain their own artifact identities.
