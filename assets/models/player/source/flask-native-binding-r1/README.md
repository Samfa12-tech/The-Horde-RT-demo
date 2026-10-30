# Flask native binding candidate r1

Source-only and unadmitted. This folder contains one append-only body GLB,
one separate corrected editable Blend, the exporter and an exact receipt.
It changes no production asset, clip binding, simulation caller or Android package.

The body retains its original geometry, node/bind/scale data, binary prefix and
Idle/Walking clips. `FlaskUse` adds 76 keys over 1.25 s. `FlaskCancel` adds 35 keys
over 0.566666667 s and starts only from Use at 0.60 s. It is not a generic
arbitrary-phase cancellation. Each clip has 78 channels for the 26-joint rig;
only `RightArm`, `RightForeArm` and `RightHand` change locally. Head, neck, spine,
RightShoulder and every left joint retain exact native Idle values.

The original r2 source used Bezier quaternion-component curves with sign flips.
The separate Blend corrects right-arm quaternion hemisphere continuity and sets
those TRS curves to LINEAR. Authored key poses remain exactly unchanged. Original
r1/r2 Blends remain intact. The native GLB already chose continuous shortest-path
keys, so its bytes did not change during this source interpolation correction.
Failed original-source subframe evidence remains in the report folder.

Actual CPU loader checks cover 220 keys/midpoints. With both IK pose weights zero,
solved Grips equal named node transforms exactly. The original unchanged
viewmodel accepts the shared solved palette at both checked endpoints. This
does not test the later nonzero left-light solve, rendering, contact or feel.

Transforms use Horde model space, +Y up/+Z forward, column-major matrices.
Translations are metres; raw joint bases retain the original centimetre scale.
Nodes are RightArm=20, RightForeArm=19, RightHand=18, RightGrip=17.
Rigidize joint bases for item sockets. The flask's native local Grip is
translation `[0, 0.065, 0]`; compose the carried root from the solved RightGrip
and its inverse. The independently retained source flask Bezier root drifts
26.77 mm from the corrected arm at Use midpoints (22.18 mm for fixed Cancel),
despite sub-micrometre key agreement. It must not become a separate carried
transform authority. Native hand/flask contact and arbitrary cancellation remain open.

Reproduce into a new report child with the installed Blender 5.2:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --python-exit-code 1 --python assets/models/player/source/flask-native-binding-r1/export_native.py -- --output reports/flask-native-binding-20261026/reproduce-new --correct-native-interpolation --body-only
```

Existing output folders are refused. Blend save bytes are pinned, not claimed
identical across saves. The receipt binds input/output hashes and measurements.
Existing player/viewmodel/sword provenance and user/service rights remain
unchanged; no new third-party or paid input is used. See `ASSET_LICENSES.md`.
