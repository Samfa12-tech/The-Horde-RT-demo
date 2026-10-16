# Mobile region-source admission correction — 16 October 2026

This closes the reproduced chapter-start admission failure in the private region-page candidate. It does not adopt that candidate into production or close full phone streaming/performance acceptance. Completed tomb, materials, niches, actors, rope and world work are preserved.

## Cause and bounded correction

The approved experimental APK `c0545439b3850acb8d1c6c1c86355408c15a97eeb5fcd9098fb66780beda0b41` presented ordinary root startup, then failed the chapter rebuild with `Invalid or unowned source primitive boundary`. This was reproduced on the host with actual native lantern assets for High/Glass Off and Mobile/Glass On or Off; High/Glass On remained the passing control. Optical profiles legitimately remove pane primitive references while retaining their canonical backing index storage. The strict geometry partition requires complete ownership of its input index stream.

`RtRegionGeometrySource` now constructs a bounded canonical view of referenced index ranges when backing storage contains gaps. It preserves source-global triangle identities, active attributes/winding, role-specific vertices, transforms, materials and instance records. Contiguous input keeps the original fast path. Unreferenced storage never becomes selectable or rendered. Invalid/overlapping ranges, duplicate or foreign-owner selections and inadequate allowance still fail without replacing accepted output owners. Temporary capacities are charged against the existing preparation allowance. `GeometryPartition.cpp` and its ownership checks are byte-identical; no object-specific rendering branch, AS/descriptor change, extra capacity or quality reduction is used.

## Exact candidate and validation

The owner-approved corrected Debug APK is **`6db07c3889ccd440f462e9affcc059cfd5784e26bbcc7645fcefd48f28ab1e30`**, 183,540,186 bytes. It was built from `482ebaff21e06f8c70222e1d8d487fade4bb340e` plus the source/test patch pinned by [receipt.json](receipt.json). It is not described as a pristine future commit build. All 169 packaged assets, including shaders, are byte-identical to the failed candidate. Signer and versionCode10 match the existing development app. The prior installed ground build was identified and backed up once before replacement. The separate release app was untouched; no uninstall, downgrade or data clearing occurred.

- Native regression: actual High/Mobile × Glass On/Off × internal/trailing unused storage (eight cases), mixed static/body/viewmodel roles, exact attributes and identities, negative selection/capacity checks and output-owner preservation.
- Release partition/source CTests: **2/2 pass**. Debug: **2/2 pass**. Pre-fix reproduction failures remain in [host-repro-tests-r02.log](host-repro-tests-r02.log).
- Android private Debug build: all four ABIs plus lint pass. Removing the private property restores the default profile and removes its guard from all four ABIs. The resulting default APK is `7602d3ef36f7bd493fd5d80bd5521fd2ecee2eff3be812d9531fa9f119bbd032`, not installed and **not byte-identical to the prior default**, because the native adapter changed.
- Production shader inputs, frozen budgets, descriptors, 26 metadata / 28 TLAS / 32 primitive/material / 18 texture ceilings and the original torch/180-second policy are unchanged. Earlier Windows Release pacing belongs to its original executable, not this rebuilt native candidate.

The [build receipt](fixed-candidate.json) retains its pre-install snapshot wording; [device-result.json](device-result.json) records the subsequent approved installation and completed check. The [archive](native-device-receipts.zip) contains actual source bytes, build logs and device frame/capability states. Its hash and exclusions are in the receipt. APKs, preference XML, full window/activity dumps and unfiltered process logs remain private.

## Bounded physical evidence

Raw device model **SM-S948B**, Android **17 / API37**, Adreno840, Vulkan1.4.295. Saved scale was retained at **40%**, internal576×1188, swapchain1440×2971, Current shadows, Diagnostic/Mobile private profile. No scale override or saved shadow migration was applied.

The corrected APK was installed with data preserved and pulled back byte-for-byte. Foreground ownership was checked before captures or input. Normal chapter startup presented. Pipeline and QueryCompute then each reached `capture-ready` at the same frozen `chapter-landing-north` view, with 12 stable frames and actual completed RT presentation identities. Their RGB images are pixel-identical. All five images below were opened for inspection, including the original failure and post-resume image.

| Frozen sample | Pipeline | QueryCompute |
| --- | ---: | ---: |
| Device-local allocation | 166,119,424B | 166,118,912B |
| Host-visible allocation | 49,445,056B | 49,444,544B |
| Buffers / allocations | 56 / 67 | 54 / 65 |
| BLAS / TLAS / instances | 21 / 1 / 28 | 21 / 1 / 28 |
| Pipelines / SBT / descriptor sets | 2 / 2 / 1 | 2 / 0 / 1 |
| Last owning GPU sample | 54.724322ms | 54.184322ms |

These are single frozen Diagnostic samples, **not playable FPS, sustained performance, memory savings or a mobile budget pass**. No full out-and-back route ran here.

One Query Home/resume check backgrounded the owned app and reconstructed it: surface generation1→3, scene epoch3→5, measurement generation6→10 and a fresh matching completed presentation. Capture request generation stayed2, correctly treated as request identity rather than a resource epoch. This is bounded checkpoint recovery, not a full lifecycle matrix. At completion the original preference bytes were unchanged and the Debug app was force-stopped.

### Inspected gallery

| Failed chapter admission | Corrected normal tomb start |
| --- | --- |
| ![Original startup failure](chapter-start-failure.png) | ![Corrected tomb start](fixed-pipeline-start.png) |

| Pipeline landing | QueryCompute landing | QueryCompute after Home/resume |
| --- | --- | --- |
| ![Pipeline](pipeline-landing.png) | ![QueryCompute](query-landing.png) | ![Query resumed](query-resume.png) |

## Remaining gate and next check

Next validate the automatic ordinary-play forest-detail retirement and reload on a representative phone out-and-back route, with exact memory/peak/restoration and sustained CPU/presentation/transition timing. Preserve coarse world, shared resources and uncertain secondary-ray contributors. No Windows memory saving or frame pacing is transferred to the phone. Full motion, collision/shadow/reflection continuity, Current/Higher and saved Lower→Current migration, orientation and sustained thermal/owner acceptance remain pending. The independent-page profile remains experimental; package I/O/CPU cache lifetime and wider useful coverage remain open. Earlier Windows startup and early Pipeline stalls are unresolved. No merge or release.
