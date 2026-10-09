# Bounded torch-water colour correction — 9 October 2026

The broad opaque yellow waterfall/catchment is corrected in runtime `8edca4bb7e91f014cba50a8bc7dcd29d79a3c5c9`, tree `0b61f9e861c75e1ff19ac66389e8ba1c19b09629`. The owner approves the observed waterfall appearance. This is a material checkpoint, not the final integrated review/release candidate.

## Cause and smallest retained change

Current source confirmed the reported `shadeThinWater` path: it added the shared fire emitter's full opaque diffuse response (gain 2.65, white base) outside the Fresnel reflection/transmission mixture. Capture-only baseline `683c77e1717d2f08fd37932c0115229b8b42f535` keeps that material unchanged and supplies four fixed ordinary torch-equipped cameras. Both phone water modes reproduce saturated yellow streams and a bright opaque-looking catchment.

Isolated zero-diffuse source `576045c2da4a80524aa9cab0b5cefafc8bf1790d` removes those bands but makes the dark waterfall too faint. The retained refinement confines fire diffuse to the **existing entrained-air fraction**: 0.004 for the film and 0.010–0.048 for the falling stream. Its local input albedo is this fraction; the shared opaque fire helper remains byte-for-byte source-identical. Mobile analytic specular and High traced fire reflection remain owned by their existing paths. The existing bubble coefficient, absorption/refraction, terminal opaque scenery shading, Fresnel mix and warm local highlights are unchanged. No global torch colour, exposure, scale or mist compensation is introduced.

The same water reflection decision is reused. The water sky-interface expression now uses RGB transmittance in both compiled routes: the legacy helper returns replicated scalar visibility, so this removes a duplicate equivalent branch without changing transport. All frozen shader budgets remain unchanged.

## Exact packages and checks

| Subject | SHA-256 |
| --- | --- |
| Before Debug APK | `f44fbd758e46eaad9cd5850327b1cfd63c154921d161258d6729a9533d933fb3` |
| Retained Debug APK, 138,462,962 bytes | `fbdc1b70684566b63ecf8f66e97b5c7778f38d3540e1ef132d1110560296c6d5` |
| Before Windows EXE | `8bb4b5c1fefeabfd33d069370ae82a87254bc022e7742a58692a1c30486e9620` |
| Retained Windows EXE | `c400317e975ef218a3141e494a34fd1b840267bc54f81c8c701aa29262a38fbe` |

- All eight Pipeline variants pass compilation, SPIR-V validation, frozen metrics and catalog publication; all eight RayQueryCompute variants pass generation/validation and catalog publication. Compatibility embeds and runtime catalog are regenerated. Functions and ray-query initialization sites remain unchanged in every corresponding Pipeline variant.
- Three affected Vulkan-enabled host fixtures pass: character/render-slot shader contracts, variant provider and bundle contracts (11.23 seconds total). Windows build passes.
- 255 Android unit tests in 39 classes, lint and all four ABI Debug builds pass. Package asset policy and ZIP 16 KiB alignment pass; orientation/fullUser and retained controller configuration contract are preserved. The APK's assets, Java dex and manifest are byte-identical to the before package. Exact installed-base pullback matches on **SM-S948B / Android 16**.
- Device install and each capture cohort preserve main settings and menu mix byte-for-byte. Capture-owned apps and Windows processes stop. Phone saved choices are not replaced by the temporary 50% capture override.

## Before/after images and scope

Each pair uses the same authored near, far, oblique and catchment camera, fixed animation time 0, input, actual emitter sockets/strength/colour, mist, shadow/dust modes and other effects. The phone keeps its saved Dust Low; the Windows frozen fixture keeps Dust Off. The phone uses actual 50% traced extent 720×1490 with native 1440×2980 output/UI and exposure 0.92; fire remains Mobile for **both** water choices. Windows uses 100%, 960×540, exposure 0.62 and High water/fire on the **NVIDIA GeForce RTX 5050 Laptop GPU**. Exposure is fixed within each platform pair, not made equal between platforms.

There are **16 phone pairs** (four cameras × Mobile/High water × both real RT backends), plus **eight Windows High-water pairs**. Both phone backends report successful RT presentation and exact owning completed-frame evidence. Windows captures complete with synchronization validation enabled and zero VUID/error markers. Inspection covers all pairs: transparent background detail/refraction, warm torch-lit stone through the streams, readable thin warm streams and catchment, and bounded view-dependent reflections replace the opaque yellow bands. These are frozen images, not moving/scanout or full-route acceptance.

[Phone Pipeline / Mobile](evidence/2026-10-09-torch-water/phone-pipeline-mobile.png) · [Pipeline / High](evidence/2026-10-09-torch-water/phone-pipeline-high.png) · [Compute / Mobile](evidence/2026-10-09-torch-water/phone-compute-mobile.png) · [Compute / High](evidence/2026-10-09-torch-water/phone-compute-high.png)

[Windows Pipeline](evidence/2026-10-09-torch-water/windows-pipeline-high.png) · [Windows Compute](evidence/2026-10-09-torch-water/windows-compute-high.png)

[Sanitized receipt](evidence/2026-10-09-torch-water/receipt.json) retains source/package identities, original native image hashes, display-derivative hashes, exact settings and owning-frame observations. Display sheets crop only native-system bars from the phone and uniformly scale each panel; original full images remain private local evidence.

## Cost, retained failures and limits

Resources and actual selected emitter/fire/shadow/mist/dust payloads match in every phone pair. No resource or ray-query site is added; frozen code-size/work metrics pass. Individual submission-12 whole RT command-buffer observations range as follows across these four cameras (milliseconds):

| Backend / water | Before | After |
| --- | --- | --- |
| Pipeline / Mobile | 35.624–47.717 | 35.469–47.396 |
| Pipeline / High | 39.191–65.763 | 38.924–65.631 |
| Compute / Mobile | 33.588–46.681 | 34.538–46.105 |
| Compute / High | 37.843–65.241 | 37.544–66.190 |

These are owning completed-frame GPU durations, **not displayed FPS or a controlled causal performance comparison**. Sequential order/thermal state is uncontrolled and each view supplies one observation after 12 stable frames. No performance gain or certified sustained cost is claimed. The owner has deferred the long sustained phone programme; existing measured gaps remain recorded.

Rejected setups remain preserved: initial wrapper/bubble forms exceeded frozen size budgets and were not admitted; budgets were not enlarged. One initial before Pipeline phone image showed stale legacy UI despite a retained ready report; it is rejected and all eight views are recaptured using a fresh validation-owned generated report. Zero-diffuse Windows Compute cold initialization exceeded the original 45-second diagnostic process deadline, producing no image/pass. Final captures retain identical native readiness/validation criteria under a separately recorded bounded 90-second process observation deadline and complete; normal Graphics acknowledgements and 15-second confirmation are unchanged. No persistence failure is inferred from these harness issues.

Audio/haptic manual revalidation required: **NO** (material/capture-only change); owner audio/haptics are independently approved on 9 October. Moving waterfall/contact/body/secondary-view integration, final aggregate CI, the immutable review candidate and Eric's audit remain separate. No merge, signing or release.
