# RT Waterfall and Lich Mist Validation - 2026-08-23

## Outcome

The previous opaque blue waterfall wall was replaced with real, separated world-BLAS water geometry and physically based, bounded ray-query optics. The route now has three narrow roof-fed strands, a shallow catchment, a connected floor runnel, and a barred drain. The existing automatic drench keeps the released 0.70-second gutter and 1.15-second settled-lantern timing. The lich room gains a low, depth-clipped blue-grey ritual mist volume.

This is not a screen-space overlay, raster fallback, particle curtain, video, or imported texture. `vkCmdTraceRaysKHR`, recursion depth one, ray-query shading, strict ASTC, one-frame ownership, the existing TLAS slot count, and shared gameplay authority remain intact.

## Rendering design

- `SurfaceWater = 10` is appended to the CPU/shader material ABI. Existing IDs 0-9 and per-triangle world-surface metadata remain unchanged.
- The falling water is closed eight-facet geometry with real air gaps. Its thin depth prevents the opaque coloured-slab appearance while retaining an actual ray-traced surface and silhouette.
- Primary water hits use air-to-water IOR 1.333, Schlick Fresnel with F0 0.0204, exact near/far interface normals, near-neutral Beer-Lambert absorption, and geometry-bound animated flow normals. Water is excluded only from direct-light occlusion, matching thin clear glass.
- High uses one bounded transmission query plus one bounded reflection query. Mobile uses one bounded transmission query plus analytic environment reflection. Secondary water shading is query-free. Off skips water candidates on the primary query while preserving all geometry and gameplay contracts.
- Android exposes `RT water: High / Mobile / Off`, persists the choice, and defaults to Mobile. Windows gameplay and deterministic Windows capture use High.
- The catchment uses geometry-bound radial impact normals and light-aware highlights. The six-millimetre floor film flows toward the real drain along negative X.
- Lich mist intersects a final-room AABB, stops at primary-hit depth, takes six deterministic fixed samples, and front-to-back composites transmittance. Density is floor-weighted and staff-state-aware; it does not create a ceiling plume.

The optics follow primary references for dielectric Fresnel/refraction and bounded ray-query transparency: [NIST water refractive index](https://www.nist.gov/publications/index-refraction-liquid-water), [PBRT dielectric reflection and transmission](https://www.pbr-book.org/4ed/Reflection_Models/Specular_Reflection_and_Transmission), [Google Filament materials](https://google.github.io/filament/main/materials.html), [NVIDIA refraction](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-19-generic-refraction-simulation), and [Khronos ray-query transparency](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Advanced_Topics/Ray_Query_Reflections_and_Transparency.html). The downward/asymmetric normal transport follows the two-phase flow principle described in [Valve's water-flow presentation](https://advances.realtimerendering.com/s2010/Vlachos-Waterflow%28SIGGRAPH%202010%20Advanced%20RealTime%20Rendering%20Course%29.pdf), without adding flow-map textures.

Blender is not a runtime dependency. It remains useful for a future authored stone slot, drain surround, or splash mesh, but a Blender fluid bake would add frame assets and would not solve runtime camera-dependent RT reflection/refraction, lighting response, pause-time determinism, or phone cost. The current hybrid keeps geometry real and optics dynamic.

## Gameplay and input contracts

- The drench trigger is confined to X `[-2.48, -1.78]` at the terminal zig-zag exit and remains non-colliding. A continuous route test crosses it from X -1.6 to -2.8.
- Checkpoint name `lantern-drop` and lantern state timing are unchanged. The `skylight` checkpoint now faces the no-torch water/catchment/drain composition.
- A Debug-only Android replay defect found during this gate was repaired: an idle lich could kill the player between checkpoint timing and replay, leaving a menu pause latched. Authoritative fixed-step route replay now advances independently of that external pause state.
- Windows controller discovery scans all XInput users and WinMM devices. Live owner-assisted WinMM telemetry identified the connected Backbone One as VID `358A`, PID `0204`, with X/Y movement, Z/R right-stick look, RT button mask `0x0200`, LT mask `0x0100`, B/Circle mask `0x0002`, and the explicitly tested menu/start button at `0x0800`. The exact identity selects Z/R and independently edge-latches RT attack, LT parry, B/Circle dodge, and pause/resume. Mouse capture remains independent.
- Dodge is shared 60 Hz gameplay state rather than a platform-only camera displacement: it latches the left-stick direction, normalises diagonals, falls back to forward at neutral, travels 0.90 m over 0.20 seconds, has a 0.55-second cooldown, and uses the existing corridor collision resolver. Paused commands are consumed rather than buffered.

The waterfall now has a true looping world-space cue sourced from “Water Dripping” by DRAGON-STUDIO under the Pixabay Content License. Windows XAudio2 and Android MediaPlayer own platform playback while native simulation-derived gains attenuate and stereo-pan the fixed stream position. The runtime WAV is a mono 48 kHz PCM16 cyclic derivative with a 0.75-second crossfade; provenance and hashes are recorded in `assets/audio/pixabay/waterfall_loop.METADATA.md` and `ASSET_LICENSES.md`.

Because this candidate adds a positional looping cue and dodge changes listener position, `Audio/haptic manual revalidation required: YES`. The owner completed a broad Windows hands-on pass on the exact final candidate and judged it "perfect" after confirming the water/mist presentation, directional dodge, RT/LT/B routing, right-stick look, D-pad menu control, pause/resume, focus outline, and controller resolution adjustment. This is owner-reported overall perception; loop-seam quality and calibrated left/right pan were not separately scored.

## Fresh host verification

- Raygen source SHA-256: `88ff329918fec881131fa4f3a9bb50e58028ab64fac61fce32f7d06587ea9d4b`.
- Embedded raygen include SHA-256: `8e6977d43882e7cd625e3c5288107568e754c61c13dc88cc0c3796bd16f6090e`.
- Compiled SPIR-V: 174,520 bytes / 43,630 words / 9,578 instructions; SHA-256 `756f1f842aa282fef464d0f2f794f8da42f9e96951eb9a0feee318b1613cac41`.
- `tools/compile-raygen.ps1 -Check`: pass; embedded words match the freshly compiled shader.
- Windows Debug and Release builds: pass.
- Windows Debug and Release CTests: 13/13 each, including material/shader ABI, bounded water queries, deterministic mist bounds, route traversal, unchanged lantern states, and desktop controller-axis mapping.
- Android `assembleDebug` and `lintDebug`: pass across the configured build.
- Two complete Windows High capture sets from the exact final candidate, `build/water-final-20260823-1922-a/` and `build/water-final-20260823-1922-b/`, contain 13/13 byte-identical PNGs. Visual inspection found the old blue wall, black transmission column, and animated zig-zag coverage mask absent. The background remains visible/refracted through the separated strands; the rounded catchment connects to the drain runnel; the settled torch composition remains visible; lich/sword/roof sightlines remain readable.
- Fresh repository Host gate `reports/foundation-runs/run-20260823-192724/`: pass. It repeated shader-staleness and negative-safety checks, clean Debug/Release builds and 13/13 tests in each configuration, 13 fixed captures, clean Android Debug/Release builds and lint, validation-package/licence checks, and evidence hashing. Its artifacts are explicitly unpublishable.

The final rounded-catchment/drain/audio/controller candidate passed fresh 13/13 Debug and 13/13 Release CTests, including exact Backbone identity/masks, independent trigger edge latches, directional/collision-safe dodge, mailbox coherence, rounded catchment/drain connectivity, and positional-loop source contracts.

Owner playtest accepted the overall water/mist candidate and explicitly confirmed directional dodge, RT attack, LT parry, corrected right-stick look, D-pad navigation, pause/resume, the gold focus outline, and controller operation of the resolution slider. The initial right-stick failure occurred even though raw Z/R selection was correct; render-frame snapshot mirroring could overwrite continuous platform view intent before the 60 Hz simulation consumed it. The follow-up keeps camera input intent across non-tick render frames and applies controller look after current frame timing is known. An explicit failed pause test plus raw edge logging corrected the menu/start mapping from a button-sweep false association to the measured `0x0800` control.

The final controller-menu follow-up assigns D-pad up/down to focus navigation and left/right to five-percentage-point changes when the render-resolution slider is focused. Live D-pad edges drove the actual settings/rebuild route and persisted `renderScale=65` in the Debug candidate settings file, proving that controller adjustment reached more than the visual thumb. Left/right retains focus navigation when another menu control is selected. The owner subsequently judged the final candidate "perfect."

## Fresh Android evidence

Exact final candidate runs:

- focused 75% measurement: `reports/android-showcase-runs/run-20260823-192345/`;
- full replay/capture/Home-resume: `reports/android-showcase-runs/run-20260823-192439/`.

- Device: raw model code `SM-S948B`, Android 16/API 36, Adreno 840.
- Candidate provenance: dirty working tree rooted at commit `4ed6594bd47f725ddfea8cef5a021ef4b5e06e10`.
- Debug APK SHA-256: `e43ed30c11b82463b7a465488182c72d4d4e4fd6701e266c89a87f5f6a404e85`.
- Installed `base.apk` SHA-256: identical to the candidate.
- Embedded raygen SHA-256: `8e6977d43882e7cd625e3c5288107568e754c61c13dc88cc0c3796bd16f6090e`.
- Mobile RT water was the effective default: the preserved preferences had no `water_quality` override and the application default is Mobile.
- Strict ASTC, native ray-tracing pipeline, and honest RT swapchain presentation remained active.

Focused 75% results use the median of three 120-frame CPU-present-loop windows:

| Checkpoint | Windows (ms) | Median (ms) | Derived FPS | Reference band | Android thermal | GPU power level |
|---|---:|---:|---:|---|---:|---:|
| `lantern-drop` | 12.542 / 12.541 / 12.497 | 12.541 | 79.738 | 60 FPS or better | 0 | 0 |
| `skylight` | 11.363 / 10.855 / 11.424 | 11.363 | 88.005 | 60 FPS or better | 0 | 0 |
| `lich` | 20.448 / 20.457 / 20.457 | 20.457 | 48.883 | 30-50 FPS | 0 | 0 |

The focused run moved from current HAL AP/BAT/SKIN 28.2/27.5/29.2 C to 43.1/30.7/35.7 C while system thermal status and Samsung GPU thermal power level remained 0. Relative to the earlier matched water/mist run, `lantern-drop` is -3.69%, `skylight` is +2.20%, and `lich` is -0.60%; none crosses the 15% investigation threshold.

The deterministic replay reached 13/13 waypoints and ended in `finale`; all 13 scene-only captures reached 12 stable presented frames; the inspected `lantern-drop`, no-torch `skylight`, `lich`, and `finale-roof` images preserve transparency and sightlines; Home/resume produced a new honest RT presentation. Candidate and installed `base.apk` hashes match exactly. Both runs have no warnings or failures.

## Evidence qualification

- The Windows owner playtest closes the requested live water/mist and Backbone-control feel gate for this candidate.
- Static and automated phone capture inspection confirms Mobile-path composition and transparency, but is not a hands-on phone artistic judgment.
- The broad owner verdict covers the audible candidate as a whole; a separately calibrated loop-seam and stereo-direction listening score was not recorded.
- No haptic cue or routing changed.

No deployment or public package was produced.

## Publication follow-up - 2026-08-23

The sentence above remains the boundary of the original candidate-validation pass. The reviewed slice was subsequently committed, line-ending-safe on clean Windows checkouts, fast-forwarded to `main`, and published as Showcase Alpha `0.1.5-alpha.1` after fresh merged-source, exact package, and exact signed-device checks.

- Source: feature commit `2bda83a`; merged/published runtime source `172bb0a`.
- Windows: itch build `#1908330`, ZIP SHA-256 `631b9f01a4d348e18733c989ebacc9c32ce9005ac9498e49e8757a0a36411166`.
- Android: itch build `#1908331`, stable-key-signed APK SHA-256 `1e81238a6e1b0e934c50eb15e80fc8efd39c06f16ca8960c428b22e5f5d5a7f2`, byte-matched after installation on `SM-S948B`.

See `SHOWCASE_ALPHA_0_1_5_RELEASE_VALIDATION_2026-08-23.md` for the complete release and public-page evidence.

With the exact byte-matched signed release still installed on `SM-S948B`, the owner subsequently approved the waterfall audio and confirmed that haptics and pause/resume work correctly. This closes the change-triggered Android audio/haptic/lifecycle owner gate without claiming a calibrated pan or loop-seam score.
