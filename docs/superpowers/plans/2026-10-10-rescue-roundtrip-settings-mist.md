# Rescue round trip, Settings focus order and rope/mist investigation

Source correction: `5104be14b170a89b7f6b84b932fb40ca9add9f1f`; starting checkpoint `2f39f803a88a4825a3cf72886703635543d2d970`. Draft [PR27](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/27) remains the single development PR. [Receipt](../../evidence/2026-10-10-rescue-roundtrip-settings-mist/receipt.json) and [runs](../../evidence/2026-10-10-rescue-roundtrip-settings-mist/runs.jsonl) retain exact inputs, commands, timings and sanitized log identities. No new game/device launch occurred in this investigation.

## Owner observation on the preceding candidate

The owner played the Windows Debug rescue/practice candidate from `d40d75b349c146d9096a768b393705ccc2f9f0fc` (EXE `2e842b0b4fad29e0156c9d0fc4248739b1b67432d84eb808e225c22eece67521`) at checkpoint `2f39f803a88a4825a3cf72886703635543d2d970`. They climbed, dismounted, returned down without a crash, and reported attached equipment/light, restored sword and correct Graphics-panel controller navigation. The process exited normally (0). The final completed-frame report proves genuine **RayTracingPipeline / Diagnostic / High** presentation with the two exact selected shader hashes in the receipt. This is one scoped Windows owner round trip, not repeatability, Query, Android, sustained-performance or lifecycle certification.

The owner remains unsure whether the lantern stayed stashed after dismount. Do not convert uncertainty into a pass or a confirmed defect. The actual return hinge resumes ordinary left-hand ownership when `equipmentStowed` clears; no restoration behavior was changed here.

The Settings Down skip and rope/mist artifact remain historical failures on that exact executable. The owner clarified that the ground shadow is correct, while an additional strange mist shadow follows the pushed rope; they did not identify its light source. New Settings executable identities below do not inherit their old owner pass.

## Settings correction and host evidence

The shared Windows controller roster placed Graphics after Back although both native Settings layouts draw it on row two. Move only its roster entry immediately after Sensitivity; visibility filtering excludes hidden legacy Water/scale controls. Graphics spatial navigation, slider adjustment, Tab cycling, capture/menu guards and Android/controller gameplay mappings remain unchanged. Ordered roster and entry/pause visibility/layout assertions cover the defect.

Debug game/controller builds pass; selected checks pass **8/8**, including GDI graphics contracts. Release game/controller builds pass; selected checks pass **7/7**. Both newly built executables passed exact Diagnostic/Shipping High Pipeline+Query containment plus adversarial and registration controls. Two failed build attempts are retained: test-local name collision, then an incorrect build target spelling; corrected builds pass. No thresholds/deadlines were changed. Shader/native inputs of the full torch sweep are unchanged; its prior 165.18-second Debug / 14.82-second margin remains historical, not a new run. Android was not rebuilt or tested for this Windows-only roster correction.

- New Windows Debug EXE: `692c69e6618c1bf1ba4a66f361616fb9aca0f2129ec7ef00ffbc7b724b6eec70`.
- New Windows Release EXE: `d4056b229f4a3b1886f9c51fbaa9d61f294221d927e93d02e3b0d84ab9461973`.

## Light-source trace and bounded mist diagnosis

`activeSkyLight` already supplies a real directional moon, with direction `(-0.180027, 0.930140, -0.320048)` and roof/geometry-tested visibility. In the opened rescue/finale chamber it instead selects two authored aperture targets `(-34.35,2.76,-16.02)` / `(-33.05,2.76,-14.38)`. Rescue snapshot publication holds dawn reveal at zero; this does not remove the sky-aperture light. The physical lantern is an admitted active fire emitter and surface direct lighting calls the existing shadow-transmittance path. It is not intentionally shadowless. Its actual completed-frame light position/strength is recorded. These code paths do not establish which source created the owner's exact visible band; lantern-only/sky-only matched captures were not performed.

Shadows in lit mist can be physically legitimate. The current approximation nevertheless queries source visibility only at the ray's fog-segment midpoint, then reuses it across 2/6/8 density samples. A moving thin opaque rope can block that midpoint without blocking the samples whose lighting inherits its result.

The reproducible [CPU probe](../../evidence/2026-10-10-rescue-roundtrip-settings-mist/probe.cpp) uses the actual fixed-step solver (600 ticks, Ready) and `RescueRopeTriangleVertices` (176 triangles). It compares rope-only intersection at midpoint versus six existing sample locations across 9,153 synthetic view rays per source. Aperture source 0 has 954 rays with disagreement and 708 clear samples incorrectly classified dark by the midpoint; source 1 has 823/822; the held-fixed recorded lantern position has 439/355. For both sky targets there is an example with a blocked midpoint but **all six samples clear**. [Metrics and limitations](../../evidence/2026-10-10-rescue-roundtrip-settings-mist/mist-visibility-subset.json) preserve the exact grid and input identity. This is an analytic subset excluding other geometry/glass/density integration, not GPU image reproduction or proof of the exact screenshot source.

Reproduce without launching the game: configure the evidence directory's CMake project with `-DHORDE_SOURCE=<repository root>`, build its `rope_mist_probe` target in Release, then run it. The output records actual solved vertices and counts. Installed MSVC/CMake were used; no new dependencies or runtime assets were acquired.

## Remaining correction / acceptance

No mist shader correction is claimed. Next isolate sky and lantern contributions in matched RT captures, then prototype visibility at the density sample rather than reusing a midpoint. Preserve rope shadow/reflection/transmission masks and correct surface shadow. A straightforward per-sample implementation changes worst-case dynamic source queries from six per segment to 12/36/48 for the existing 2/6/8 density modes; these are query-count bounds, not measured frame costs. It must first satisfy every unchanged compiler/variant invariant and frozen artifact ceiling, then undergo matched GPU/image/performance checks. No temporary matrix for that new implementation was produced here, so neither feasibility within the caps nor acceptable GPU cost is established. Do not solve the band by excluding the rope, disabling shadows or lowering quality.

Audio/haptic manual revalidation required for this correction: **NO** (Windows focus ordering only; semantic feedback inputs unchanged). Existing milestone-specific owner/audio and Android/backend/lifecycle gaps remain pending. New Settings manual check: enter Settings, press Down from Sensitivity and confirm Graphics is next; continue through SFX/Music/Fullscreen/Back. No launch is automatic.
