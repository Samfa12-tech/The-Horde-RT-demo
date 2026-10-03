# Geometric glass-shadow absorption, September 30

Source baseline: pushed `cbc135ffb7b98167a20a40a72b3e39b96cf36d34`.
Phase4 remains **open**. This slice repairs the selected shadow path's absorption,
not the accepted player assets/animation/ownership or all dielectric transport.
Previous corner, duplicate-candidate and triangle-position fixes remain accepted.

## Change and bounded contract

The selected GenericDielectric helper still queries actual scene geometry once
per finite light segment. It collects up to4 Mobile/8 High distinct oriented
boundaries, consumes them nearest-first independently of BVH visitation order,
and pairs2/4 volume identities. These physical limits are unchanged.

- Beer-Lambert uses measured entry/exit or finite-endpoint distances, not authored
  half-thickness. A leading exit integrates origin-to-exit distance.
- Transmission remains applied at each crossed interface. Thin sheets use their
  full authored RGB tint, not a12% blend.
- Only exact equal-distance, same-instance/material/orientation/thinness
  duplicates merge; no epsilon removes nearby physical surfaces.
- The real query uses1um minimum and the actual finite endpoint. Previously,
  its1.5mm minimum could skip thin glass and real near-field opaque blockers.
- Overflow/invalid stacks fail closed and keep meaningful existing diagnostics;
  the8% scalar recovery light is gone. No primary Fresnel/IOR/refraction/TIR,
  material, geometry, resource ownership, masks or CPU/GLSL ABI changes.

This is geometric absorption on a **straight visibility segment**, not a full
refractive-caustic solver. A boundary-free segment wholly inside an unknown
initial medium still returns unity; that admission gap is explicitly **open**.
The fixture probe demonstrates it, not a passing physical case.

## Exact segment evidence

Investigation-only native-storage records call the same selected shader helper.
Two-pixel IEEE-float encoding avoids screenshot colour-management loss.
`probe/shadow-segments.patch` is source-only and not a Shipping feature.
Temporary module-footprint allowances were restored; physical limits never changed.

| Segment | Geometric interior distance | RTX pipeline | RTX compute | SM-S948B pipeline |
| --- | ---: | --- | --- | --- |
| normal | 0.200000m | pass | pass | pass |
| oblique45 degrees | 0.282843m | pass | pass | pass |
| inside origin, exit | 0.100000m | pass | pass | pass |
| finite endpoint inside | 0.100000m | pass | pass | pass |
| entry about0.100mm from origin | 0.200000m | pass | pass | pass |
|1mm closed pane | 0.001000m | pass | pass | pass |

All18 measured RGB results match the analytical transmission/absorption formula
within6.22e-8 absolute error (test allowance2e-5). Actual boundary distances,
orientations and material metadata are retained. This is not phone compute parity.
The seventh probe, both endpoints inside/no crossing, is deliberately not counted.
`checks/decode-shadow.py` reads retained native PNG/RGBA bytes without editing them.

## Exact phone and build state

SM-S948B / Android16 / Adreno840 / driver2150932499:
ordinary Debug APK`3de01bb86cc26c3c930ebe7af0f1fe760a2701de7f9c2031f81d3a794d323043`
is installed and pulled back identically. Run085838 completes7 captures plus
Home/resume,75%/1080x2235, strict ASTC and native pipeline RT presentation.
Probe APK`fc7ad292…8b6b8a0` separately completes2 captures/Home-resume;
raw storage records are retained. Run091047 restores the ordinary3de01bb8 APK,
checks isolated/tinted captures and Home/resume. Phone returned Home.
Stable and owner-accepted candidate packages/data were not cleared or replaced.

Unsigned Shipping ARM64 APK`214cee5b3f586353e5d861bf4d02a457d379a2986057fe8e5b031eea8e9cd946`
builds and passes actual packaged/stripped module containment, disassembly and
SPIR-V validation; it is not installed or published.
Fresh rebuild reproduces the ordinary Debug APK bytes.
No S24/S25, sustained performance, live-motion or subjective glass pass is claimed.

## Images: keep correctness changes separate from parity

All42 focused RTX captures complete with presentation proof and matched manifests:
Mobile/High x baseline/new pipeline/new compute x7 named views. All transport and
shadow overflow/unclosed counters remain0 on RTX. Phone isolated overflow1,
80 certified budget recoveries and grazing recoveries remain open and unchanged.
Finite-endpoint and implicit-origin counts now expose real previously unintegrated
segments; do not interpret new counts as newly manufactured errors.

- Correctness AB:14/14 RTX pairs fail unchanged max3/fraction0.001 tolerance.
  Ten have sparse changes; tinted/fire change about12% of opaque receivers too.
  All7 phone screenshot pairs fail; these are ADB AB images, not raw backend proof.
- Pipeline/compute:2/14 pass (isolated at both qualities);12/14 fail, peaks5–85.
  Sparse pixel/recovery/TIR differences remain open. Shipping/Diagnostic parity is
  a separate gate and is not established by these comparisons.
- Standard13:12 byte-identical; finale changes2418/518400 pixels>1, max67,
  failing both existing limits. No tolerance was loosened or old pixels restored.

The finale peak(577,534), [67,31,4]->[0,0,0], was separately investigated:
native light queries hit opaque world-body geometry at0.918797/0.899032mm,
inside the old1.5mm exclusion. CPU-upload triangle witnesses match within0.43um
and0.43um; the third26.809mm hit matches within2.34um. The nearest admitted
primary receiver is a BodyRemainderPrimaryVisible cuff surface, not the viewmodel
hand farther along that camera ray. Two light directions have geometric-normal
dot products-0.0450/-0.2979 and origins biased about70um inward: actual opaque
cuff exits block them. Thus this investigated peak is consistent with corrected
near-field self-occlusion, not a new player/proxy tuning change. This does **not**
classify every changed pixel. Full raw records, uploaded meshes, hashes and
read-only witnesses are in `probe/near-field/`; CPU exports are not GPU readback.

## Tests, shader footprint and remaining gates

Fresh MSVC Mobile/High builds, focused host3/3, CPU math/topology2/2, Android
ARM64 Debug/unsigned Release, full artifact/manifest negative suites,16 pipeline/
compute compilations/validations and regenerated adapters pass. The new source
guard fails against the old selected helper and passes restored candidate source.
Post-probe restoration/freshness checks pass; no investigation route remains.

All Shipping modules contain0 diagnostic atomics/no Binding22. Actual ARM64 APK
scans validate both pipeline and compute pairs. Generic Shipping55015->56553 words
(+2.80%); Diagnostic56960->58760 (+3.16%). Reviewed footprint ceilings and
Diagnostic static atomic shape32->41 are rebased explicitly; Opaque footprints
are unchanged, but raw SPIR-V hashes change. Physical4/8 and2/4 limits stay fixed.
No performance improvement is claimed from these correctness builds.

Current-source CI for the preceding cbc135f checkpoint is green:
push36639470345 / PR36639474785,45 portable and11 Vulkan-host tests. That is
baseline evidence, not CI for this new source until its own pushed run completes.

Next: close/reconcile remaining genuine interface-budget and contact recovery
cases; determine initial-medium reachability; validate live glass/failure counts;
then matched Shipping performance and separate backend parity. Preserve music,
consent reporting, justified resource/pacing work and the final candidate matrix.
Owner licensing/signing/publication boundaries remain unchanged.
Audio/haptic manual revalidation required: **NO**, RT-only transport/shadows.
