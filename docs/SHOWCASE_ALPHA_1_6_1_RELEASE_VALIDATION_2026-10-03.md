# Showcase Alpha1.6.1 — signed, frozen, unpublished candidate

Package source checkpoint: `0c7db23beb5af1fd83b9a4d7cdf950386f6e0eea`.
Package version1.6.1; Android versionCode 9. October3,2026.
Runtime implementation remains `3d26ad64a3db1e1e1b7965587a72fd114189bdf6`;
current music/assets are from `ec13876069b4049b524c5198734c0bbbabbd039c`.
The only subsequent Gradle change is the reviewed music-manifest admission pin;
current-package metadata is committed at the package source checkpoint.

Owner explicitly confirmed signing backup/recovery and authorised production
signing and artifact freezing after green CI. This does not authorise merge,
release/tag creation or publication. Owner-only checklist boxes are not marked
by Codex. No signing secret or recovery material is included in this evidence.

## Immutable artifacts

Local directory: `releases/candidates/1.6.1-freeze-20261003-0c7db23/`.
Do not rebuild, rezip, resign, append files inside the ZIP or regenerate the
manifest after freezing. A later authorised publisher must consume these bytes.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Horde-Lantern-RT-Alpha-1.6.1-Windows-x64.zip | 116,163,128 | `94cb88d8cab9fded13013b60f4ad50ae26390063d096115c4374ed83ce562787` |
| Horde-Lantern-RT-Alpha-1.6.1-Android.apk | 108,261,409 | `bc5c7ce3c755c16ec39e2c16fa9eae01c31983c7393e645f881c5bcc9e6a346c` |
| SHA256SUMS.txt | 220 | `efff07ebe93454c3be6f76c1736f3c8869feb13474b8854f8d80991a24a1666e` |

The [canonical provenance](../release-provenance/horde-lantern-rt-alpha-1.6.1.json)
pins the source, files, sizes, hashes and documentation. Artifacts remain local,
ignored and unpublished; committing a receipt is not remote binary backup.

## Package construction and current checks

No unchanged game binary was rebuilt. A new Windows stage was constructed from
the hash-verified current-player/music validation ZIP `d2984666...3df58b12`:
all69 assets are byte-identical to current source and the admitted executable
is unchanged (`11e0a59b032cee25296d99cbf74d531871d98abbfe784823cc52f9b3b84dba45`).
Only committed README/release notes/licences accompany it. Stale block-arm/glass
README wording was corrected; generated reports and the validation-only marker
are excluded. The original unpublishable ZIP remains untouched and is not a
release artifact. Final ZIP has73 entries, no source/high/processing/platform-
wrong textures, and exact source/music/held-item/attribution admission passes.
Windows file/product version1.6.1, embedded PerMonitorV2 manifest and all in-app
credit markers pass. This is a portable Windows package, not Authenticode signing.

Android signing consumed the retained, aligned unsigned Release APK
`79ceaa7a9ba4faa10410ee3e174fdc5ca58906983cc2b1342acc242e4fbe14e9`.
`apksigner` verifies v2/v3 with one signer and the established certificate:
`8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.
Every original APK entry remains byte-identical after signing, including the
production manifest, DEX, native libraries, textures and17-entry music bank.
Exact production ID `com.samfa12.hordelanternrt`,1.6.1/code9, non-debuggable,
sensorPortrait and16KiB ZIP alignment pass. All four exact packaged native ELF
files have three LOAD segments each, all aligned at0x4000. No shared C++ runtime,
source/high/processing assets or Windows-only textures; all11 credit markers pass.
The first handoff guard rejected its relative keystore filename before signing;
resolving it beside the existing handoff fixed that bounded packaging issue.
No key was replaced or copied, and process-scoped secret variables were cleared.

Actual extracted Windows Shipping/High and signed Android Shipping/Mobile
SPIR-V: four modules per binary, both hardware-RT backends, `spirv-val` and
disassembly pass; zero diagnostic atomics and no readback binding22. Compiler/
SPIR-V checks are not physical-device presentation or pixel-parity proof.
See the [finite freeze evidence](evidence/2026-10-03-signed-freeze/README.md).

The actual canonical publication preflight now passes with origin/GitHub checks,
no fixture/skip flags: exact artifacts, source-time generated VERSION/map/
CMake/Gradle identity, documentation and expected tag/release absence. The initial
failure from stale literal-version assumptions is preserved. A bounded tooling
repair retains historical1.6.0/code8 guards and passes focused fixture/CTest1/1.
This is structural admission, not publication authorisation or a device test.

## Test and device matrix — retain each evidence scope

| Lane | Actual result / boundary |
| --- | --- |
| Exact package-source push37084760313 and PR37084764357 | Both completed success, all six lanes, source0c7db23. Earlier93e6400 runs37083785643/37083790750 are also green. |
| GCC / Clang portable hosts | Current six-lane compiler CI success; previous inspected current-source rosters59/59 each. No physical Vulkan device. |
| MSVC portable host | CI success; inspected current-source roster65/65. Separate broader Windows tooling checks retain their own results. |
| Focused Vulkan CPU-host |17/17; actual player/skinning/semantic/resource fixtures. No hardware RT presentation. |
| Selected Clang ASan/UBSan |15 finite named fixture targets pass; not an unrestricted full sanitizer suite. |
| Android | Four-ABI build/Java contracts/lint lane success; final exact APK package/ELF/SPIR-V checks above. |
| Windows RTX live | Retained exact executable/current69 assets pass Pipeline smoke and foreground required Compute1838/1838 completed presented CPU/GPU rows, counter/cancel/restart/completion. Final archive changes documentation only; no new performance/parity claim. [Live receipt](evidence/2026-10-03-windows-live-closeout/README.md). |
| SM-S948B | Final signed APK installs over observed1.6.0/code8 without data clearing; installed pullback hash matches. Saved settings, modelled hands/two enemies, strict ASTC, honest RT presentation and same-PID Home/resume pass.76% is the retained setting, not a75% performance run. [Signed-device receipt](evidence/2026-10-03-signed-s26/README.md); earlier [interaction receipt](evidence/2026-10-03-final-s26-interactive/README.md) retains heavy scene/resize and owner checks. |
| S24 | Owner-deferred: **working but not fully tested**; retained development images only, not this signed artifact. |
| S25 | Exact device unverified. |
| Audio/haptic manual revalidation required | **NO**: packaging/signature/documentation only; gameplay events, gains, assets and playback bytes are unchanged from owner-accepted work. |

Initial foundation failures and targeted repair evidence remain in the
[integration matrix](evidence/2026-10-02-final-integration/README.md); do not
invent a new single-run97/97 pass. All329 commits through package source are in
the [commit inventory](evidence/2026-10-03-signed-freeze/commits-through-0c7db23.txt),
including integration ancestry, not329 independent feature patches.

## Accepted scope and open gates

The [finding-by-finding disposition](ENGINEERING_1_6_1_FINDING_STATUS.md) owns the
audit reconciliation. Modelled world/viewmodel ownership, grips/cuff/look-down,
music/volumes/Windows focus, consent-based relay reports and bounded resource
ownership are accepted. Mobile omits pane geometry; High physical glass/fixes
remain intact, with unresolved High contact/near-edge defects and numerical
parity explicitly deferred. No tolerance was loosened or fake RT introduced.

Performance is owner-accepted as-is: sustained30FPS at75% is **not achieved**.
Retained S26 held-high resource pair93.6585 vs93.6380ms shows no meaningful gain;
standalone warm live reveal121.2535ms is not comparable to a cold/other workload.
No further optimisation is queued; full graphics-options menu is planned1.6.2.
RAM pressure checks do not close unavailable GPU bandwidth/cache/stall counters.
Hotstrike redistribution remains owner-tracked, explicitly nonblocking by owner
policy; no licence/asset/history/distribution workaround is made here.

The final signed-device gate now passes by a new owner-authorised S26 smoke,
not by inference from earlier Debug evidence. Initial ready11.353s and observed
resume12.730–17.317s are not instant readiness. A first unpaused interval reached
gameplay death while inspection continued; controlled paused-resume returns with
vitality3. UI automation could not idle during diagnostics; logs/image/presentation
are separate evidence, not a full diagnostics UI pass. No performance matrix,
email or accepted listening test was repeated. The exact Android compatibility
record is updated; Horde is stopped and phone released.

Merge and publication remain withheld. At a separately authorised launch, publish
a non-draft GitHub Release `v1.6.1` as well as the chosen distribution channels:
installed1.6.0 selects it in the passing fixture, but itch alone cannot trigger
the existing updater. The action opens a download page, not an automatic install.
The real future update prompt remains post-publication evidence, not a preflight
pass. Signed-update smoke is now separately proven; explicit publication
authority still must not be inferred from a structural provenance preflight.
