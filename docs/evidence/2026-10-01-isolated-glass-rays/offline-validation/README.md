# Isolated glass path offline validation

Investigation-only offline corroboration for the temporary SM-S948B
Diagnostic/Mobile pipeline path capture. No renderer, asset, device, or source
changes are made here. The emitted summary is not a graphics acceptance result.

Run with Python 3 from any directory:

```powershell
python C:\Dev\tmp\horde-glass-isolated-20261001\offline-validation\analyze_paths.py
```

The script reads the existing `path-analysis.json`, `marker-analysis.json`, and
`path-counter-comparison.json` under `C:\Dev\tmp\horde-glass-isolated-20261001`.
It pins the full runtime GLB path and SHA-256 in the script and writes JSON to
stdout; `summary.json` is the captured run output.

It parses the `LanternGlass` mesh, matches the uploaded triangle vertices, and
performs double-precision Möller–Trumbore intersections over all 72 glass
triangles using each recorded float object-space ray origin/direction. It
checks nearest intersections and reports explicit determinant, barycentric,
TMin-comparison, and nearest/tie tolerances in `summary.json`. These calculations
corroborate the hardware-selected hits; they do **not** emulate GPU traversal,
reproduce BVH rounding, or establish unrecorded rays.

Result: 405/405 captured triangles match the pinned GLB exactly, and all 405
captured candidates are the nearest double-precision intersections with no
nearest tie at the documented threshold. The 80 reason-2 rows share
`EXXEX`: enter one pane, internally reflect once, exit it, enter a different
pane, then reach that pane's real exit at the fifth-interface budget boundary.
The component-group transitions are 3→4 (30), 4→1 (18), 5→0 (21), and 3→2
(11). Seventy-nine final segments connect opposing parallel faces separated
about 0.007 local GLB units; one is an adjacent-face path (triangles 12→13).
The separate closed-volume-budget row is `EXXXE`: entry, two internal TIRs,
exit, then a real entry into the next pane.

In the selected shader, `rt_dielectric_transport.glsl` checks the budget before
processing interface index 4; reason 2 increments the recovery counter but
sets transmitted radiance to zero. Thus these observed events are genuine
five-interface truncations, not duplicate-candidate recoveries. This capture
does not quantify final pixel contribution, settle the Mobile quality policy,
or establish compute/RTX behavior. Owning counter arrays match the comparison
receipt, but control/probe submission serials and simulation ticks differ.
The instrumented path image matches the marker image outside its reserved
record rows; the marker image itself differs from control at the 81 target
pixels. No image pass is claimed.
