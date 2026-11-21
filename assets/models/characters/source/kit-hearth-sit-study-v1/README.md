# Kit’s hearth sit

`kit-hearth-sit.blend` is an editable 83-frame lowering and settling take on the original 24-joint [Kit Warden rig](../kit-warden-v1/README.md). It uses the exact B01 chair, floor and side table from the [tavern authoring receipt](../../../world/source/bellwether-tavern-v1/authoring-receipt.json). The east-facing stance holds the left boot slightly ahead of the right. That is the contact target for the later turn and protective walk.

The seated pose finishes 4.0 mm above the seat in the sampled hip/thigh mesh, with a 270 mm pelvis drop. The seated measurement is also the smallest sampled clearance during lowering. Maximum sampled sole travel is 10.4 mm. Front, side and oblique Workbench frames were inspected against the authored chair. See [contact samples](contact.json) for the bounds and exact inputs.

The take covers the movement from planted stance to sitting. The earlier turn and the transition to conversation still need staging. The sampled contacts do not measure every clothing or hand triangle. This is Blender authoring evidence. The take has no native export or game binding yet.

Regenerate from the repository root into a new directory:

```powershell
blender --background --disable-autoexec --python-exit-code 1 --python tools/author-kit-hearth-sit.py -- build/kit-hearth-sit-rebuild
```

Editable study SHA-256 `74833c7727fbf319b9f0701b8b62d20d422df5ad018f82838e7aa19f5856006c`. Warden/Kit created with Meshy and supplied by Samfa12. Motion and furniture authored by Samfa12/Codex. The source entitlement remains recorded in [ASSET_LICENSES.md](../../../../../ASSET_LICENSES.md).
