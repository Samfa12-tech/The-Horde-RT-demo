# First-bend grate floor repair — 8 October 2026

The owner-reported lower opening is confirmed in real Pipeline and Compute images: rays pass under the three recess walls because the pocket has no floor. The visible strip is the environment/sky response, not authored exterior content. [Original RT image](evidence/2026-10-08-grate-floor/before.png) → [closed stone floor](evidence/2026-10-08-grate-floor/after.png). The owner saw the phone tests and accepts the appearance: “I saw the grate in the tests - it's a great (geddit?) fix!”

One ordinary closed mossy-stone base spans x=2.03..3.12, y=-1.13..-0.95, z=-8.80..-8.33. Its top meets the corridor floor, and its side/back ends embed 2 cm into the retained walls. The front top has no coplanar floor overlap. All six faces use the existing outward-wound world-box path: 12 triangles / 480 bytes of authored vertex/index/surface-code payload in the existing world BLAS. This is not a measured driver-allocation or zero-cost claim. There is no extra asset, instance, material, light, shader path, collider or walkable area. Bars, sprigs and the upper light well remain unchanged; Kit and the exterior scene are not added.

## Exact candidate

| Artifact | Identity |
| --- | --- |
| Runtime source | `7c9b8779f1eb0af9d20e4f1d326d92f2d8a3f412` |
| Runtime tree | `6b099c8c2b95d7cad3e5971c13a1c8b94ba2e551` |
| Android Debug APK, 138,462,962 bytes | `c7074c790d30563fb920415f9e879a5720f2af062096d5b69adb83a785d9420d` |
| Windows Debug EXE | `b8b4803ff3f285bf1e385acabf5190f8a791b9f2bdb5638bd6b788d0185a6923` |
| Test-only correction | `5301e4a7bcc8cbe280d6277ea183deb3d1178d92`; accepted APK remains byte-identical |

The private seal is `task-4/integrated-grate-7c9b8779-20261008`. [Sanitized receipt](evidence/2026-10-08-grate-floor/receipt.json) joins original image hashes, immutable source/package identity, current settings preservation, completed RT captures and the owner's limited visual approval. All 94 packaged assets match the accepted ddd5 menu build byte-for-byte. Four actual native ELF load layouts and stored ZIP offsets pass 16 KiB alignment; exact package/held-asset admission passes. Menu mix, defaults and saved settings remain unchanged.

## Checks and evidence limits

- The new geometry fixture first fails against the absent base. After closure, 24 footprint rays and three nearby downward sight lines hit stone; an underside light ray is blocked, while an upward ray above the floor stays open. Existing shared collision blocks entry through the grate and permits movement along the corridor. Five affected host fixtures pass in 19.20 s, including route traversal, gameplay, animation and Debug staging.
- Windows RTX 5050 Laptop GPU: four legal views (lower centre, left, right and upward), before and after, on each real RT backend. All eight exact candidate captures exit cleanly with synchronization validation enabled and zero validation error markers. Each backend's upward image is byte-identical before/after. Lower images replace the sky strip with normally lit stone. These static level views explicitly permit cropped arms but still require dedicated primary ownership; they do not close equipment/motion gates.
- SM-S948B / Android 16: exact installed-base pullback matches. Both real backends pass four frozen native-display views at 50%, 12 stable presented frames per view, 13-waypoint route replay, and completed-owning-frame Home/resume checks. Internal extent is 720×1490 with 1440×2980 swapchain; display capture is 1440×3120. The agent inspects lower/oblique phone views and the owner approves the observed grate appearance. Phone log checks are not installed Vulkan validation-layer evidence. Saved main preferences, 7/21 menu mix and free rotation are unchanged; owned apps are stopped.
- Full Android suite after the fixture correction: 241 tests / 38 classes, zero failures/errors/skips; lint and four-ABI build pass. Original runtime CI [push 37765586185](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37765586185) and [PR 37765594978](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37765594978) retain the Android failure: the old parry fixture expected newly valid pose 161 to be unknown. Test-only 5301e4a7 moves that rejection boundary to 164. Corrected-source CI [push 37766904731](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37766904731) / [PR 37766909226](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37766909226) has 6/12 aggregate jobs passed, 6 pending and 0 failures at the receipt timestamp. Documentation CI is separate; no green claim is copied from ddd5 or from device results.

The first old C147 baseline attempt retains its arm-visibility assertion failure and PNG. It is not an admitted successful capture. The eight successful before views use only the added inspection metadata/empty-base regression; their original world geometry is unchanged and their exact patch/executable hashes remain in the receipt.

Keep this bounded floor repair. Owner appearance acceptance and affected correctness checks are complete. Sustained 50/40/33 phone quality/cost, remaining controller/combat/equipment secondary-view and audio-focus checks, final integrated candidate and Eric's independent audit remain open in the [active plan](ENGINEERING_1_6_2_TOMB_FINISH.md). No release, merge, signing or publication follows.
