# Horde 1.6.2 review candidate - 5 October 2026

This is the historical 5 October graphics review checkpoint. The owner resumed
bounded tomb-demo work after the reset; current status, expanded scope and owner
gates are in the [active finish plan](ENGINEERING_1_6_2_TOMB_FINISH.md).
Merge and publication remain on hold. The original pause restriction applied
until that resume; the [exact pause checkpoint](ENGINEERING_1_6_2_PAUSE_2026_10_05.md)
retains completed checks, glass-apply timing, artifacts and the resume plan.

The final graphics candidate is frozen at runtime source
`1334cc9c58ec97940ac10d861f0397143ca7a9f4`, with both Windows configurations,
three four-ABI Android packages, exact hardware checks and all six runtime CI
jobs passing. See the [final graphics record](ENGINEERING_1_6_2_FINAL_GRAPHICS.md)
for defaults, migration/persistence, Mist On/Off and exact validation limits.
Later documentation backups do not relabel binaries. Independent final audit
and remaining owner/device gates precede release actions.

The [mirror comparison](ENGINEERING_1_6_2_MIRROR_COST.md) found no useful
repeatable saving. Retain the ordinary mobile mirror and the explicitly required
desktop/High mirror. Optional experiments are closed or deferred; no further
exploratory cohorts form part of this closeout.

The ordinary c112 mist is adopted after owner still-image approval and independent Mobile ABBA review. The accepted mist boundary is `2bdad1327eb1d4fedee9e9e223a3054536717749`; the subsequent Keeper-lighting runtime is `6468c3e47908901e8a904cf4a0eb1c7029c9767b` on draft [PR18](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/18). The accepted2bd mist boundary's [CI37250895699](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37250895699) passed all six individual jobs; all six jobs on prior backup25a79299 in [CI37247496966](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37247496966) passed. New Windows/Android artifacts, native checks and the accepted upward grate view are recorded below. The owner also approved moving mist after the two current Windows runs. The subsequent two-flank-torch correction passes exact native/device checks and the owner-approved lighting motion sequence. Its measured Shipping cost crosses the investigation threshold; performance tradeoff acceptance remains open.

This is an unpublished review candidate. Historical stopped/earlier current labels retain their own scope; read this record and the [second-pass ledger](ENGINEERING_1_6_2_SECOND_PASS.md) before reusing them. The owner has authorized a separate optional Lower indirect-lighting candidate and matched comparison while retaining Current as default. The separate prototype is now deferred after the balanced Shipping comparison fails to demonstrate useful benefit; Current remains default. Its inactive source backup preserves the experiment. Texture LOD remains separately unbenchmarked/deferred.

## Final graphics artifacts and evidence, 5 October

| Artifact at runtime 1334 | SHA256 |
| --- | --- |
| Windows Debug EXE | `fbfa3b08e86be05038dbc881953353bedf77146675d6560b445c8c63fe231759` |
| Windows Release EXE | `8b886d20c0b5b9fc116a0ce0e3890e292eef4d29fe2540fca8c066eece2ba5e0` |
| Windows Debug ZIP | `947f5beb5dda1e134671942aa50112f08ed51d4f0b002805f9cdf0d3d1a83aa3` |
| Windows Release ZIP | `53642c2fbc973b1e7f32686cb2820ae9f6e14aa3c75bbe02da5aa405b600bb90` |
| Android Debug APK | `cea6f9594696a7d97df85c2d6cbe7782b82f5a9c561f5a43bc93aa194bf1a11e` |
| Android unsigned Release APK | `567f0d2043721c37ad26d88c1756f22f90e127488b20a905d4aaee5acb1167be` |
| Isolated development-signed Shipping Benchmark APK | `dfb36907def5444327be2acfc6ee873db16594160a0f15d093b4ade7b02229db` |

The [full admission record](ENGINEERING_1_6_2_FINAL_GRAPHICS.md#validation-boundary)
binds 5,823 before/after source files, 85-member Windows packages, actual PE/ELF
shader inspection and four-ABI Android alignment/notices/policy. Local checks:
12 affected CTests plus 3 renderer/player integration checks, both Release
shader fixtures, 154 Java tests/24 classes and lint 0 errors/49 warnings.
[Runtime CI 37271761916](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37271761916)
passes all six jobs.

Both Windows backends admit 78 Showcase and 28 preview images. All 26 On and 26
restored-On views match prior 646 RGBA exactly; actual Off completed uploads,
shared camera/fire/geometry and physical 24/19 ownership join. The owner accepts
the optional Off still images, noting little visible difference. No new motion
or timing acceptance is inferred. Windows receipt
`c64c0156c230a40376a6be8eb121cceececef8276f7daafac49466442c93a777`.

Both new Debug phone backends pass 13 routes/seven captures/strict Home at
saved 50%, with actual APK pullbacks and the original 1,420-byte preferences
unchanged. Completed MistOn and fire/shadow controls join physical 24/19.
Phone receipt `76211a9987b5235fb4cf323eaa43f46e5c9c9209d4f6e7a8845f4fd947ed2237`.
The isolated validation app verifies the staged 50%/GlassOff/MistOn tuple,
explicit restoration, MistOff background rollback and confirmed Off/On cold
restarts. Settings receipt
`ff03e377b5185ecb0329df652c34c0357df344e70a0a9eb5d151b20539413a2d`.
No owner Debug settings write or data clearing occurs; the fresh-install
key-absence rule is tested without clearing the existing phone app.

Current mobile 50%/Mobile-water/Mobile-fire/GlassOff/Current/MistOn/cap 30 is the
owner's deliberate default choice, preserving valid saved/custom/legacy tuples.
Desktop 100%/High-water/High-fire/GlassOn/Current/MistOn/cap 30 and mirrors remain.
This does not measure savings or sustained 30 FPS. The substantial Keeper cost,
Windows listening/report interaction, new quality cost, full four-light/early
reward overlap, final independent audit, owner signing safeguards and other
device dispositions remain explicit. These packages remain unpublished and
unsigned for production. Earlier artifacts below retain their own scopes.

## Keeper lighting artifacts and evidence, 5 October

Runtime content commit `6468c3e47908901e8a904cf4a0eb1c7029c9767b` is pushed and remotely verified. The [lighting record](ENGINEERING_1_6_2_KEEPER_TORCH_LIGHTING.md) describes reveal/death timing, shared authored sockets, physical occlusion and the explicit four-light budget. Current builds preserve genuine Pipeline and BLAS/TLAS-backed RayQueryCompute rendering. The later shader-fixture word-pin correction and Windows test diagnostics change tests only; they do not relabel these binaries.

| Artifact | SHA256 |
| --- | --- |
| Windows Debug EXE | `5aabd64664d12b9168f28b55830fe82eb6c58c60788fe7e2123e21a3e0466a41` |
| Windows Release EXE | `869003917fd96dce205ebf8a153a72962be5a1b055d1c70b31e2d7ef601c7f4e` |
| Windows Debug ZIP | `cb2553b6a34e8f8af7194177372062587bde94f309e49ce125252aa659c4cf4d` |
| Windows Release ZIP | `4694559239f35e277ca252f8ddf3f26d6cd48803933f38fd100ecdf98cc6f0ab` |
| Android Debug APK | `bd37e97496db92c7c375ab7674282dbd15b16b9e3e9a9ec94645db0c60e38295` |
| Android unsigned Release APK | `78a6b42d99e47cafb193364158a302ed1e5d3b2bbed6b28008aefc45dc144e9a` |
| Separate Shipping Benchmark APK, minimum50 | `e42bc8cae3867ba93e5fb572b563cefa093438eac00c5698b170bfd04271994b` |

Each Windows archive closes85 exact files; only the executable and the documented reused-torch attribution paragraph differ from the previous frozen package. All three APKs build all four ABIs and pass ELF/16KiB/compiler-policy/notice admission. Actual packaged ARM64 payloads equal stripped libraries and contain the four expected SDK-validated modules; both Shipping packages have zero diagnostic atomics and no binding22. Runtime/held-asset package contracts pass. The source inventory is a current snapshot, rather than a before-build attestation of every compile input. Earlier packages remain intact.

Six affected renderer/player CTests pass24.85s. The shader Manifest fixture passes32.88s; Artifact passes92.29s after two additional exact compatibility word pins, preserving the initial87.44s failure. Existing budgets and all guards remain unchanged. Both native backends pass13/13 static checkpoints with synchronization validation and no Vulkan/SYNC error markers: Pipeline66.105s and Compute65.968s. All26 PNG hashes join canonical last-presented completed records, physical TLAS24/BLAS19 and actual selected shader payloads. Combat records contain IDs3/4 at the admitted sockets; finished-death reward records contain only the unchanged lantern ID. Native admission receipt `9da9e01ca4721e7d0cdab16490d9d0902f849e0b7ab59fa2d3b612d47438cb1e`. These static observations do not establish the reveal/death motion, early reward overlap, four simultaneous real lights, owner appearance or Shipping cost.

Allocated SM-S948B/Android16 passed both new Debug backends at saved50%: each13 route waypoints, seven named captures and strict Home/Resume. Both exact installed-APK pullbacks match Debugbd37; saved preferences remain byte-identical1420 bytes with SHA256 `3118148f509e127df6c607f23533bc6782ed6b504dc08aec2cff49160c68140a`. No settings were written or data cleared. Actual completed phone combat records contain IDs3/4 and physical TLAS24/BLAS19. The unsigned Release fails SDK certificate verification as expected; no signing invoked. Phone admission receipt `11ce90149829db6943dc7c1a3e9e5c7bd8531623d4c044b39042bb40cd1a3d58` joins both13-waypoint routes, all14 captures and post-resume completed serials2174/2193. The [ordinary Shipping comparison](ENGINEERING_1_6_2_KEEPER_TORCH_COST.md) now retains four complete native reports with an A2 observer qualification. Hot-phone pooled GPU cost rises62.4 to72.1ms, and the finale subset61.9 to129.9ms; cost tradeoff acceptance remains open. The owner gave positive live-phone flank-torch appearance feedback; reveal/combat/death/extinguishing motion acceptance remains open. Prior2bd moving-mist approval remains at its original scope.

Runtime646 CI has a retained discrepancy: [PR run37256204897](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37256204897) passes all six jobs, while [push job111593710285](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37256202062/job/111593710285) builds successfully but fails one of88 MSVC host tests on a2000ms Windows note-control message timeout. The failing edit cannot be identified from its original broad phase label. One original local Release test and one diagnostics-only revision pass; neither explains the CI timeout. Test-only per-edit labels and UI-thread state diagnostics preserve deadlines, privacy guards and assertions. No production dialog fix or infrastructure-cause claim is made. All six individual jobs on test/documentation backup `e6f8eae9869e6ee4adc7fc1db5db5c9691ac557a` pass in [PR run37258601408](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37258601408). This later pass does not explain or erase the retained646 push failure.

## Implemented scope

| Slice | Current result | Acceptance limit / next action |
| --- | --- | --- |
| Five source-audit corrections | Presentation ownership on both platforms; explicit AS scratch and SBT checks; bundled cgltf notice; transactional output-only Windows resizing; credential-token boundaries | Keep the owner's accepted older-driver shutdown-proof limitation in [A1 decision](ENGINEERING_1_6_2_A1_COMPATIBILITY_DECISION.md). No inferred unsupported-device certification. |
| Graphics / preview / UI | Shared persisted/requested/effective settings; genuine RT preview; direct mobile choices, row scrollbars, live presented-FPS, clear Use / Keep / Restore flow; independent glass, shadows and Low fire | Owner accepted saved50% and one-line Interact. New shadow/fire appearance and sustained cost remain open. Lower and Current each use one sample; no fewer-query saving is claimed. |
| Material / resolution foundations | Shared authored normal strength/UV scale, tangent/import/ABI checks and AO separation; efficient fixed-size transactions | DRS and temporal geometry remain bounded CPU foundations, without runtime history or reconstruction. Required moving-light material appearance gates remain explicit. |
| Scene / torch / reveal / audio | Approved full-width stairs and cave-in; coherent roof clearance for torch/hands/flame/light/shadow; retained entry wall grate/vines with upper masonry; closed entry skylight, barred separate waterfall skylight, untouched waterfall hole/vines; Keeper presentation hold | Owner accepted collapse/flame, torch lowering, vines/waterfall grate, sound/haptics and Keeper intro hold/return. Owner approved the native partial upward entry-enclosure view; Windows changed-audio listening remains open. No enemy relocation or1.7 work. |
| Recovery / safety | Native Alive acknowledgment and ordered menu reset/retry; descriptor-layout bounds from the shared27-entry Diagnostic roster; persistent mapped-buffer reader retains resource ownership; exact-once Windows context retirement | The owner's Debug array assertion and private duplicate-map capture failure are retained negatives. Failed Windows retirement now retains the whole context; the separate shared-initialization failed-idle limitation remains open. |
| Reporting | Local typed statistics with explicit consent and privacy boundaries | Benchmark Send remains unavailable pending separately authorized compatible endpoint admission. No private reports or identifiers are committed. |

## Integrated ordinary-mist artifacts, 5 October

Current main-source Debug and Release builds include the owner-approved c112 mist, declared C152 camera-framing expectation and exact scoped software notices. All85 files in each new Windows archive match its staged roster. The inherited package README now identifies the unpublished1.6.2/code10 candidate. Frozen earlier83-file archives remain intact.

| Artifact | SHA256 |
| --- | --- |
| Windows Debug EXE | `72b9a0f8ab6586c08059dd6a214176e66ad72526d4f8821d17bdf65e807b6a10` |
| Windows Debug ZIP | `435444950ada230d60967f5e193d06bafdd6625eaafcab8f154033032814c3ab` |
| Windows Release EXE | `da94ae61cdda63f5d7ac1bfdf5da42ddda6d9addaa86250451d40c33a84a1e6f` |
| Windows Release ZIP | `57d41fcbe9daee624bfe174746aba218e29ef156fb0e098992b02d7f9fad7bf1` |

Actual four-module PE extraction per executable passed SDK validation/disassembly. Shipping modules contain no diagnostic atomics or binding22. Fresh Pipeline and Compute source banks/catalogs match all eight modes each. The development staging/framing CTest and six affected provider/input/ABI/fire/player CTests pass. Two current C152 captures, genuine Pipeline and Compute, exit0 with synchronization validation and no Vulkan/SYNC markers; actual manifest records legal camera pose, dedicated ownership and the explicit cropping expectation. The owner approved the rendered partial upper enclosure. Full rim visibility is outside the current camera range and is not claimed. Earlier failed C152 capture remains an incomplete negative.

Both current Windows Keeper/retry/reward motion runs completed: Pipeline76.928s and Compute93.781s, each28 captures, exit0, synchronization validation enabled and zero Vulkan/SYNC markers. The owner approved the moving mist. Automated runs were muted. An earlier run opened on the isolated validation desktop and failed the unchanged30-second focus guard without captures; a read-only desktop probe established the cause. The replacements used the owner's Default desktop with the same executable, harness and180-second deadline. No desktop/security settings changed.

| Android artifact, all four ABIs built offline | SHA256 |
| --- | --- |
| Debug APK | `d58f9f21743f5f68991f58905abfb239d20cfed029fd0d703cc1e46c780f4db6` |
| Unsigned Release APK | `e3a2254b5bfe70ddaf0c8f5a868f94997b71d16649c6df677efe1b858ea561fd` |
| Separate development-signed Shipping Benchmark APK, minimum50 | `21d1b3ba9dd469c0aa5a01d898c24b19964f399033aead2d92bd740fe23e44da` |

All three APKs passed runtime/held-asset and exact software/cgltf notice checks. Actual packaged ARM64 modules match the stripped library and pass SDK validation; all four ELF architectures and16KiB/policy checks pass. Debug seal receipt `e95564b256be7a753ad6b270c43c69089e6e88e6ca08b86ce7cfa672f692caf7`, unsigned Release `306f387e1c7ae14ccbe77f54f66afeedf46989de56278264f134a60cb1dda631`, and Benchmark `e875d291300b5e05c7480064f2dd2864e3a7a514f261e1cab80f6be96b83ce05` retain their distinct input/compile-policy scopes. The current-source snapshots are not proof of a before-build inventory of every compile input. The unsigned Release fails signature verification as expected; no production signing material was read or used.

Allocated SM-S948B/Android16 passed both current Debug backends at saved50%: each13/13 route waypoints, seven named feature/Keeper/held-lantern captures and strict Home/Resume with a fresh completed owning RT frame. Actual installed APK pullbacks are retained and match Debugd58; saved preferences remain byte-identical1420 bytes before/after both runs. No settings were written or data cleared. The collection source context was2bd with documentation-only changes; the binary is not relabelled with a later documentation commit. This is development correctness/lifecycle evidence, not Shipping performance or another-device certification. The dark Keeper captures support the owner's new flank-torch request, rather than proving acceptance of the pending lighting change.

New fire/shadow/material choice cost and owner Windows audio/report checks remain open. Older Android and Windows cohorts below keep their exact artifact identities.

Independent admission verified56 Windows captures/2,207 owning frame joins and14 phone PNG/state joins, both13/13 routes and lifecycle ownership. Selected actual EXE and sealed ARM64 module byte ranges match the admitted strategy pairs. Windows receipt `e4f56c9216ed33f84c2d66e20216dc3fa94308482f267b2c9f6eb17589777962`, phone receipt `079be9eb959d89764b10987650d5a2f366a5306395369de5740f21a9049d4e03`; final index `842519df1743b954b7dc360ebfeca70daa5c8e6fd6b88b9709644b7f043114cd`. This admission excludes the subsequent flank lights and establishes no sustained FPS or listening pass.

## Previous frozen artifacts

All files below are separate frozen local review outputs, not release packages. Windows artifacts were built from3f9b; Android artifacts remain the separately identified89fec builds because the retirement change affects Windows and host checks only. Both Windows archives contain83 exact files and passed closed asset/notice/module SDK admission; receipt SHA256 `af045f7c69ac34f920bc5aede4b35c2d5aded64d0ee564981b6d130533bad1d4`. The inherited release README was corrected only in these fresh review outputs; frozen1.6.1 artifacts are intact.

| Artifact | SHA256 |
| --- | --- |
| Windows Debug EXE | `befa7687ac4b53cce403e2fa15fb23ff157b8db28cf9f87b223fcfebf0439e1e` |
| Windows Release EXE | `0ecdc824ddf507289167bc2e6f054ded4211b5cbe3b68c8cb7462b7b38b6f644` |
| Windows Debug ZIP | `aebc366e13efe87e56ce308e46e5bc1049f61397d3193a251e21dea8196098c1` |
| Windows Release ZIP | `a5ef09b27564880a41e840efde304ccbdc22328d46972b4bab29630d9a65389e` |
| Android Debug APK | `c66bcde2b5cad882bb3899f5050a39b2ad2a90ee09c31b3e28ae6e4fd97e101e` |
| Isolated Android Shipping benchmark APK, minimum50 | `8da32be3de5fb1d1f0cad351eab21f34d6d4bc2920c77a1737be2a3304fbd54e` |

Windows Debug/Release builds passed. The actual-header retirement-owner CTest passed1/1 with29 lifecycle assertions; the earlier reader actual-method CTest retains its separate89fec evidence. Two fresh3f9b Debug entry-panel captures, one per genuine RT backend, exited0 with synchronization validation and zero Vulkan/SYNC markers. They exercise successful native retirement without injecting a failed graphics-idle result. Android Debug/Shipping benchmark rebuilt all four ABIs offline at89fec; both passed asset/held-package and ARM64 stripped-versus-packaged SDK validation. The benchmark's four-ABI/source/compiler seal passed2,416 checks with240 designated current sources; receipt `110f2c05e2ec69cc2ad8f5bffdd6e2062b278fc13ae4a59735f8243f74b54f6d`.

Independent Windows evidence admission passed611 checks. All271 selected source/build/capture/module/stage/ZIP files, totaling1,015,328,720 bytes, remained unchanged; receipt `44146c5b6905344383b05dfd00395847931eb14c590e03dba96cab758971bded`. Actual four-module payloads per EXE and complete83-member ZIP roundtrips match. Documentation30453433011f3678032bacc7c87b6f2695c149f4 also passed all six individual jobs in CI37197584606. Neither audit nor CI substitutes for missing owner/device gates.

The allocated **SM-S948B/Android16** now passed two new Debug APKc66bcde2 runs at the owner's saved50%: genuine Pipeline and RayQueryCompute, each with13/13 deterministic route waypoints, entry/Keeper/finale captures and strict Home/Resume with a fresh completed owning frame after surface recreation. The recorded installed APK hash matchesc66bcde2 in both runs; a separate raw installed-APK pullback was not retained in those packets. Saved preferences are byte-identical before, after Pipeline and after Compute:50%, Mobile water/fire, cap30, GlassOn, Current shadows, no pending draft. Independent admission passed248 checks with326 selected files unchanged, including actual packaged-versus-stripped Debug libraries and selected ARM64 SDK modules; receipt `ae21b5f3a6ac78ab82e7dcfb5f0c83d7370f2c832e26ab461272930e975bee41`. The APK remains the89fec build; c76 identifies the unchanged runner/source context, not a rebuilt APK. This is correctness/lifecycle evidence, without a sustained performance, new subjective acceptance or other-device claim. The isolated Shipping8da32be3 APK is now admitted as the exact installed control for a new mist cost comparison; that comparison is now independently admitted; its descriptive results and adoption scope are recorded below.

Historical runtime03a49's four Windows Keeper motion/retry runs with70 captures retain their old hashes. Unchanged Java code's149 tests/24 classes and lint0 errors/49 warnings likewise remain dated evidence. Initial broad CTest116/125 and scoped successful follow-ups are separate runs; the PowerShell5 version fixture's Restricted-policy block remains, without a security-policy change. Current-main scoped MIT software documents are reconciled; asset, Pocket Audio Core and mixed-package exclusions remain unchanged. Existing frozen review packages predate those root notices and have not been silently altered.

## Decisions and measured limits

- **Selective opacity: retain NO-GO.** The existing1.6.1 experiment already tested the same mechanism with32 equality captures and9,752 Shipping frames, finding no repeatable saving. The redundant isolated proposal was restored exactly; no new candidate was installed. Reopen only for materially different contrary evidence.
- **Spatial FSR: defer production; retain Linear.** Six native static cases/192 audited timestamp rows passed. At75%, mean post interval was0.010123 ms Linear /0.060966 ms FSR; at50%,0.009581 /0.056959 ms. Two private FSR images allocate4,915,200 bytes. This is a static encoded-downsample comparison, excluding RT/presentation and proving neither mobile benefit nor moving-image AA.
- **Mist: ordinary c112 adopted after owner still approval and Mobile cost review.** Exactly 24 guarded ordinary source/bank/catalog/budget overlays preserve density, extinction and march budgets, with actual emitter strength applied once and at most four interval-midpoint source visibility calls per active mist pixel; GenericDielectric may traverse multiple bounded interfaces. Four matched-quality ABBA courses each admitted 1,838 measured rows. Pooled GPU means were 57.117353 ms control / 56.1244265 ms candidate, with different ordered thermal conditions; no causal saving, sustained 30 FPS or separate lantern-heavy claim follows. Independent dataset receipt `c20cf7e4c34bb6ae38cf5039c5ac95eb172611b86d2143117fca384d21a30a0e` admits identity, counts, weighted means, privacy and preferences. [Full adoption and cost record](ENGINEERING_1_6_2_MIST_ADOPTION.md) keeps actual APK hashes, per-case temperatures, source pins and remaining exact-candidate/motion gates. Private Compute sky and blocked real-fire probes retain their narrower path scope; Pipeline probe creation timeout remains unexplained. The admitted2bd gameplay had one fire; the later Keeper flank-light slice requires new native overlap evidence. No private probe was copied into production.

- **Diffuse-direction bounce: defer Lower adoption after the separate matched experiment.** Current remains default. All7,352 owning rows join modes and unchanged controls; pooled GPU62.107672/61.934068ms differs0.28% with unequal thermal context and final Current faster than either Lower. Useful benefit is unproved. The inactive35-file source archive preserves the2bd prototype; no new cohort or ordinary adoption follows. **Texture LOD remains separately unbenchmarked/deferred**, lacking world/Lich mip chains and complete secondary footprint ownership. [Separate decisions](ENGINEERING_1_6_2_OPTIONAL_TRANSPORT_DECISIONS.md).
- **DLSS/SGSR/detail normals/general LOD:** deferred on the documented input/history/licence/resource/measurement gaps. Parallax remains deferred by contract. See [temporal decisions](ENGINEERING_1_6_2_TEMPORAL_DECISIONS.md).

One audited unchanged Shipping75% control course completed1,838 accepted CPU/GPU frames. Whole-cycle median64.942291 ms/p95 88.431562; owning AS+shading+transfer GPU interval54.128827 ms/p95 77.725623. USB battery38.4 to40.6 C, external thermal status2 to2; aggregate report thermal remains unknown. This warming ordered control establishes neither a causal improvement nor sustained30 FPS. No default scale or quality was lowered. Owner Debug saved50% remains intact.

## Review and release gates

1. Keeper torches now have exact builds/device checks and owner-approved reveal/death-completion/extinguish motion. Resolve their measured phone cost tradeoff; early reward overlap and four simultaneous real lights remain distinct gaps. The mirror comparison retains desktop/high-quality mirrors and Current transport.
2. Preserve the accepted native partial upper-entry view; new fire/shadow/material choices still need appearance and measured cost. Preserve all approvals at their exact scope.
3. Admit the exact final APKs on allocated SM-S948B with scoped affected checks; S24 is deferred and S25 remains unverified. Connection alone never grants a new allocation.
4. Complete independent assistant review, refresh current CI and freeze the final artifact/evidence identity. Windows listening, statistics/clipboard/picker and any newly changed audio remain explicit owner gates.
5. Obtain explicit release approval. No merge, production signing, tag, publication, deployment, paid generation, credentials/security change or data clearing is authorized.

Independent review identified and cleared the bounded correction for a Windows failure-path ownership gap. The exact-once heap owner detaches host callbacks and joins host audio before native retirement; failure retains the complete context without member destruction or retries. CPU fixtures and current CI cover ordering/retention, and both native success paths pass. No hardware failure was reproduced. The accepted older-driver WSI proof limitation and separate shared-initialization failed-idle cleanup limitation remain explicit.

### Current owner motion and phone quality feedback

The exact646 Debug5a four-window sequence passes70 captures and2,627 owning
frames across Pipeline and hardware RayQueryCompute; the owner approves the
flank ignition, combat/death persistence and chest-light transition. Admission
`9b89b796f1b9d4723c62d5c3434aa8e076ec70e9688ba58289ffd1ab95eb407e`. Muted readbacks prove no listening or sustained timing result.
Owner separately says the phone looks fine at50% internal resolution; this is
a useful accepted visual reference, without changing defaults or proving
sustained performance. Existing comparisons retain75% and all matched settings.
The ordinary stone still image is acceptable as an experiment, but the owner
prefers the mirror and requires its retention for high graphics and desktop.
The completed mirror comparison now defers a mobile fallback because useful
repeatable benefit is unproved. The owner's later explicit default selection
is recorded separately above; earlier cost comparisons keep their original
75% settings. No upscaler or33% phone experiment follows.
