# Bellwether loose road stone

This editable Blender study pairs the existing Kit ankle take with the actual chapter terrain at the lower-road station. It contains the preserved flat study action and a separate `Kit|ankle_assess_road_fit` action. Both refer to the original 24-joint Kit rig. The road fit places the boot on the sloping surface and uses the stone as temporary support under the right toe.

The 42-triangle stone starts moving at frame 9, matching the existing scrape at tick 8. It travels 284 mm across the ground and settles at frame 32. The exported GLB holds the unplaced mesh with a single opaque material. [Road contact measurements](road-contact.json) supply the world transforms over the 97-frame take in native metres and axes. Matrix columns are ordered local X, Y and Z. The Blender scene also contains only the nearby road triangles needed to inspect the contact.

At frame 9 the flat take's lowest left boot sample was 28.7 mm below the road and the right was 49.9 mm above it. The fitted action places those lowest samples 2.9 mm and 1.5 mm above their respective support surfaces. The largest fitted-versus-intended clearance error across all frames was 0.52 mm. The contact measurements cover the lowest weighted boot vertices and the stone's rotated footprint.

The game still plays the earlier ankle clip. Runtime integration needs the matching road-fit take and moving stone on the same simulation clock. The mesh has passed native source import and glTF validation. Scene lighting, hardware RT presentation, and phone performance remain open for this pair.

Rebuild from the repository root into a new folder:

```powershell
blender --background --disable-autoexec --python-exit-code 1 --python tools/author-bellwether-loose-stone.py -- build/loose-stone-rebuild
```

The recipe pins the [Kit source](../../../characters/source/kit-ankle-approach-v1/README.md) and chapter terrain by hash. Stone geometry, material and road fit are original Samfa12/Codex work. [Asset provenance](../../../../../ASSET_LICENSES.md)
