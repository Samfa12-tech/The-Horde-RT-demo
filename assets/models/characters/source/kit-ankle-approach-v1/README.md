# Kit’s Bellwether road injury

`kit-ankle-approach.blend` contains two editable 60 Hz actions on the original 24-joint Kit Warden rig. `Kit|ankle_assess` is a 1.6-second right-ankle roll, balance catch and weight test that ends with Kit carrying weight on his left leg. `Kit|protective_walk` is a seamless 52-interval cycle driven by 0.76 m of accepted movement. The healthy left boot has a longer support phase than the injured right boot. The player and Kit continue through the existing road motor.

The [contact samples](contact.json) measure model-space joints and boot soles against a flat authoring plane. The authoring recipe checks that planted boots stay within 6 mm of the plane and that forward slip stays below 2 mm in the gait’s planted intervals. The shared terrain motor places the actor root. Individual boot contact across the game’s uneven ground still needs playtesting.

The [export receipt](native/export-receipt.json) records the append-only export onto the pinned hearth payload. The game uses the [terrain-fitted assessment](../../../world/source/bellwether-loose-road-stone-v1/README.md) and this study’s original protective gait. It reuses Kit’s existing mesh, skin, earlier animations and PBR maps. Source licence and motion provenance are recorded in [ASSET_LICENSES.md](../../../../../ASSET_LICENSES.md).

To rebuild from the repository root in fresh directories:

```powershell
blender --background --disable-autoexec --python-exit-code 1 --python tools/author-kit-ankle-approach.py -- build/kit-ankle-rebuild
blender --background --disable-autoexec assets/models/characters/source/kit-hearth-sit-study-v1/kit-hearth-sit.blend --python-exit-code 1 --python assets/models/characters/source/kit-step-turn-proof-v1/export_native.py -- --hearth-sit --output build/kit-hearth-base
blender --background --disable-autoexec assets/models/characters/source/kit-ankle-approach-v1/kit-ankle-approach.blend --python-exit-code 1 --python assets/models/characters/source/kit-step-turn-proof-v1/export_native.py -- --ankle-approach build/kit-hearth-base/kit-warden-turn.runtime.glb --road-fit assets/models/world/source/bellwether-loose-road-stone-v1/bellwether-loose-stone.blend --output build/kit-ankle-native
```
