# Flask timing r2: separate unadmitted source derivative

This editable Blend is the original r1 scene with only the complete-use return
edge aligned to53/60seconds (0.883333s),6.667ms earlier. The existing0.36s sword
restore finishes at1.243333s, then holds to the unchanged1.25s drink endpoint.
No healing, equipment event or runtime readiness is granted by this scene.

Original body/viewmodel/sword/flask geometry, bind matrices, left-arm source pose,
material inputs and the complete cancellation sample rows are unchanged. The
original r1 Blend and its0.89s profile remain intact. The saved r2 playback and
292 discrete head-contact observations pass; these selected-region source
checks are not native IK, full-body contact or first-person/RT acceptance.

The existing [authorer](../flask-use-study-r1/author.py) takes the explicit
`--restore-start-tick 53` option for this derivative. Without it, regenerated
legacy complete/cancellation pose rows still match the retained r1 rows exactly.
All destinations must be new; do not overwrite either saved source to rerun.
From the repository root, for example:

```powershell
blender --background --python-exit-code 1 --python assets/models/player/source/flask-use-study-r1/author.py -- --restore-start-tick 53 --output-directory reports/flask-r2-new --editable-output reports/flask-r2-new.blend
blender --background --python-exit-code 1 --python assets/models/player/source/flask-use-study-r1/check-contact.py -- --sweep --blend reports/flask-r2-new.blend --intent reports/flask-r2-new/effective-intent.json --motion-receipt reports/flask-r2-new/source-motion.json --output reports/flask-r2-contact-new.json
```

The source timing corresponds to the separately tested utility-clock candidate;
it does not establish the real GameSimulation caller ordering or shared native
hand/item transform solve. Normal flask stays OFF and no capacity changes or
production clips/bindings are included. See the [evidence and remaining gates](../../../../../docs/evidence/2026-10-26-flask-utility-preparation/README.md).

Existing asset provenance/rights remain in `ASSET_LICENSES.md`; no paid generation
or new third-party input is used. This source folder is outside runtime/package
inventories. Workbench previews are authoring views, not captured gameplay.
