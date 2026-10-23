# Single Kit lower-detail review

**Bounded study complete; candidate remains UNADMITTED. Production Kit retained.**
The [candidate and provenance](candidate/README.md) were committed separately as
`33f8978c`. The rejected world-body experiment remains closed; the player world
body and first-person arms are unchanged. No phone operation or performance claim.

## Geometry and material risk

| Measure | Original | Candidate |
| --- | ---: | ---: |
| Vertices | 26,945 | 21,539 (−20.06%) |
| Triangles | 25,968 | 19,922 (−23.28%) |
| Native 64-byte vertex payload | 1,724,480 B | 1,378,496 B |
| Native 32-bit index payload | 311,616 B | 239,064 B |
| Audit GLB file | 2,151,672 B | 3,736,968 B |
| Welded boundary edges / components | 17 / 1 | 17 / 1 |
| Welded nonmanifold edges / Euler | 16 / −6 | 18 / −4 |
| Geometry versus averaged-normal disagreements | 0 | 14 |

The audit file grows because the original BIN prefix is retained. Lower runtime
geometry bytes are not a shipping/download reduction. In the matching loaded RT
resource inventory, host-visible allocation falls by 418,816 B and device-local
allocation by 809,088 B; counts remain 52 buffers, 63 allocations, 20 BLAS and
28 TLAS instances. This inventory is not a peak-memory or isolated BLAS-byte study.

The source's 16 nonmanifold edges are inherited, **not newly rejected defects**.
Both new four-incident edges also exist under exact float positions, on right
thigh around (−0.214, 0.760, 0.046) m and front torso around
(−0.016, 1.126, 0.137) m; they are not solely rounding/weld artifacts. The 14 new
normal disagreements are finite facets on unprotected torso/limbs, with angles
90.89–123.21° and areas 1.40e−5–6.73e−5 m². No new zero-area, below-baseline-minimum
sliver or zero-UV finding. Face, hands and boots are outside these new findings.

These counts do not prove a visible crack or materially unsafe behavior by
themselves. They identify new retriangulation/shading risks requiring review,
distinguished from source defects. [Mapped witnesses](audit/material-risk.json)
and [preservation/topology receipt](audit/static-inspection.json) retain source
rows, skin identities and neighborhoods. No repair, normal rewrite, facet removal
or additional optimizer invocation occurred.

## Native rig, acting and grounding

All 457 nongeometry accessor byte regions, rig, seven clips, materials, surviving
attribute rows, 11,428 protected rows and 11,075 protected source triangles pass
exact preservation. The additive candidate-only intake command checks 70 clip
poses (seven clips × five phases × neutral/acting), plus 15 idle/walk blends with
head/chest motion: **85 poses, 1,830,815 vertex poses**, exact raw and grounded
PBR bytes against each mapped source row. This verifies floor/contact preservation
at those samples, not continuous artistic deformation everywhere.

Review found that the initial 15 blend checks compared grounded output only,
although their diagnostic claimed raw parity too. That narrower log is retained.
The [focused refinement](native/raw-blend-refinement.json) adds raw vertex/tangent
comparison for those blends and re-passes all 85 poses, original intake and the
expected wrong-map rejection. No timing or RT runtime changed; those measurements
were not repeated to repair a test label.

The original intake still enforces its production counts/materials and passes
all 35 existing clip samples. All 663 original worker contracts pass. A deliberately
wrong temporary map fails at its first changed row, as intended; its retained
failure is a gate sanity check, not a production failure. [Native receipt](native/native-receipt.json).
No existing gate or default asset/worker setting changes.

The explicit experiment command is:

```text
horde_rt_kit_source_intake_tests --compare-candidate <candidate.glb> <source-row-map.u32le>
```

## Actual matched Windows RT

Both isolated packages use the same existing Debug executable, SHA-256
`c3c931924d01fd62997b752a48516b82dbcb51263b19bfe1130a7ae8fdca3d71`, built from
the tested `749e5ffe` runtime source. Only Kit's explicit package copy differs.
Current RayTracingPipeline Diagnostic/High shader identities, saved settings,
camera/acting/motion snapshots and successful presentation identities match.
No private shader substitution or exposure/lighting adjustment.

The existing hidden lookout fixture passes **54/54 checks per cohort**, including
34 completed render calls. It uses copies of the canonical seeded ForestEndpoint
test save; source save/manifest and copied settings hashes remain unchanged.
It does not establish naturally earned progression or physical controls.

| View | Original | Candidate | Scope |
| --- | --- | --- | --- |
| Front | [image](rt/control-windows-kit-neutral.png) | [image](rt/candidate-windows-kit-neutral.png) | 3 m, 1232×803 output/internal, 100% |
| Profile | [image](rt/control-windows-kit-motion-turn-30.png) | [image](rt/candidate-windows-kit-motion-turn-30.png) | 3 m, same dimensions, 87.19° relative facing |
| Close reunion | [image](rt/control-close-reunion.png) | [image](rt/candidate-close-reunion.png) | About 1 m, 960×540 output/internal, 100% |

All six captured PNGs were visually inspected. No obvious gross silhouette loss or
boot separation is apparent in these views. Small torso/limb shading and shadow
differences remain: front changes 3,750 pixels, profile 2,409 and close-up 10,130.
Those are unmasked image differences, **not** measured contour error or a visual
equivalence pass. The hood is dark even in the close-up; fine-face acceptance,
the provisional one-output-pixel contour tolerance, localized facet safety and
reflection quality are not established. The other 24 acting/motion states have
matched receipts/images retained locally; they are not an independently reviewed
continuous motion movie. [RT comparison](rt/comparison.json), [build/custody/image receipt](receipt.json).

## One CPU component bracket

Release, serial control/candidate/closing control; 30 warmups and 240 samples each:

| Median | Opening control | Candidate | Closing control |
| --- | ---: | ---: | ---: |
| Kit pose preparation | 1.84105 ms | 1.46170 ms | 1.85265 ms |
| Combined Kit/player preparation | 3.54475 ms | 3.15875 ms | 3.54430 ms |
| Unchanged player world preparation | 1.21365 ms | 1.20545 ms | 1.20745 ms |
| Unchanged first-person skin | 0.47200 ms | 0.47310 ms | 0.47460 ms |

Approximately **20.6% Kit / 10.9% combined** reduction versus the faster control
is observed in this bounded desktop CPU workload. Original control output digests
match; all three first-person digests match. Dispatch/join bookkeeping is inside
the critical timer; no worker is selected. Clock/thermal equivalence was not traced.
This excludes owner setup, GPU upload, AS execution, RT dispatch and frame pacing.
The RT fixture explicitly reports `performanceEvidence=false`; its retained frame
stage/GPU observations are not an admitted frame-time benchmark. **No overall FPS,
RT-dispatch or Android gain is claimed.**

## Disposition and next step

Keep this single artifact available for review, with production original selected.
The promising CPU result does not resolve its localized material risks or missing
visual/device acceptance. Do not begin another automatic reduction/repair loop.
If pursued later, resolve those mapped regions through an expressly scoped asset
review before a phone comparison; retain the original as control.

Return next to player-facing 1.7 acceptance: compact HUD thumb reach and continuous
portrait/landscape rotation need an owner-offered phone, while recovery flask rules
and deferred shared forest work remain separate decisions. The parked chimney
prototype, PR27 conflicts and existing Debug route gate are unchanged. All owned
native render/build processes have completed; no phone or transfer is active.
