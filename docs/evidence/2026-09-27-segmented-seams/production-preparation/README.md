# Production admission preparation

The owner accepts wrist, inner-bicep/armpit, normal look-down/occlusion and mirror
appearance. Directional locomotion and visible feet/steeper look are future work
in `FUTURE_WORK.md`, not remaining1.6.1 acceptance gates.

## Reproducibility

Two clean Blender5.2 single-thread runs reproduce the exact accepted world, view
and paired manifest, all six comparisons passing:

```powershell
./tools/validate-player-regeneration.ps1 `
  -OutputDirectory C:/Dev/tmp/horde-accepted-pair-reproduction-20260927-a `
  -BlenderExecutable 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' `
  -AcceptedPairDirectory docs/evidence/2026-09-27-segmented-seams
```

Use a new output directory for another run. `regeneration.json` retains source
hashes and both outputs. World=`f2c3f62b...`, view=`6f06d77e...`, manifest=`556f8b4f...`.
The base-rig guard now reads the admitted view receipt's original `sourceWorldSha256`
instead of comparing the base rig against whichever derived world is installed.
The five-part manifest composition is idempotent, avoiding a duplicate body
remainder when the promoted manifest is used as input. No source asset changes.
Repeatability and matching the accepted reference are separate tool predicates;
both pass for retained output records, and an altered second-export hash rejects.

## Package preparation

Default Android asset staging and Windows/Android package inventories now include
the viewmodel GLB and its two-part manifest. No processing receipt enters shipping
packages. New `PlayerPackageInventoryTests.ps1` checks both staging and required
entry lists, with an optional actual-APK byte check; it is registered in CTest.

An ordinary ARM64 Debug build (no candidate overlay) succeeds in42s. Its APK hash
is `28ff8dc5d3e301514f5c088a8a392ec4c9bb0f87bfdb75cbad76b641f697bf2a`.
The actual APK contains the current source viewmodel and manifest byte-for-byte,
and no processing JSON. This is the old production asset pair plus staging
preparation, **not** a promoted modelled-player build; it was not installed.
The owner's accepted isolated APK `66de46e6...` remains installed/Home-backgrounded.

Fresh Windows configure and two focused CTests pass: player manifest agreement
and player package inventory. PowerShell parse/diff checks pass. No signing,
publication, comprehensive final-candidate matrix or new phone claim is implied.

Next: admit the accepted exact world/view pair, update its durable metadata and
five-region agreement tests, then switch normal application presentation to the
anatomical modelled route while preserving explicit diagnostic comparisons.
The production route has not switched in this preparation slice.

Audio/haptic manual revalidation required: **NO for preparation-only asset tooling
and package inventory changes**. The separately tracked anatomical-profile
feedback acceptance remains separate from visual approval.
