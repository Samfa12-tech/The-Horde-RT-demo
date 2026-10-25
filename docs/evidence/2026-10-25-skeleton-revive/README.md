# Reverse-death pose proof: direct idle join rejected

Base source `3546471815607f5487773b4f196864673cd46c7b` passes all six hosted
jobs in [run 40189229304](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40189229304).
This separate Debug-only fixture adds a finite hidden immutable-snapshot preview.
Production simulation, animation bindings, original GLB and normal rendering are
unchanged. No gameplay revival, healing, targetability or wave is implemented.

Two targeted Debug app builds and two 150-frame actual RT captures pass the
presented/completed-submission, primary-arm and unchanged application/event gates.
The first view was cropped and remains retained privately; one farther oblique
follow-up shows both corpses and floor. It samples the original nonlooping
2.966666698-second Dead clip backwards, preserving roots/facing and health zero,
then shows the direct Idle control. Five authored seconds at 30 samples/s are
rendered **unpaced**; this is an animation proof, not measured FPS or phone play.

The reverse rise looks plausible for an undead actor, but the direct join fails
visibly at frame119/3.9667s ([Dead0](dead-zero.png)) to frame120/4s
([Idle0](idle-join.png)). Both feet, hands and torso jump into the idle stance.
Native ankle displacement of roughly 0.656/0.629 m corroborates that visible snap;
it is not a newly invented numeric rejection threshold. The inherited corpse
skin minimum is about 5.25 mm below local floor, and Dead0/Idle0 are about 13.92/
10.81 mm above; those source values alone do not reject the rise.

**Direct reverse-Dead-to-Idle is unadmitted.** No blend candidate is stacked onto
this retained pass. The later [single blend proof](../2026-10-25-skeleton-revive-blend/README.md)
implements the [narrow seam](NEXT_BLEND_SEAM.md), but rejects the visible foot
slide. Both failures remain; an authored contact-preserving transition is the
next requirement. Do not disguise the join with root/yaw snapping or enable it.

The follow-up used a reports-only executable/settings copy; assets resolve from
the same compiled Debug source root. Capture mode skips campaign construction.
`LOCALAPPDATA` alone would not redirect the normal known-folder campaign path.
Owner settings/campaign hashes, all 151 parked owner files and foreground custody
verified unchanged. There was no phone or foreground activation, and no owned
build/capture/transfer remains. The source asset's existing attribution and
distribution restrictions remain in [ASSET_LICENSES.md](../../../ASSET_LICENSES.md).

[Sanitized exact source/executable/native witness receipt](receipt.json).
The silent private MP4, full raw frames, build logs and private custody maps stay
under ignored `reports/skeleton-revive-20261025/`; no Library/public clip upload.

For explicit reproducibility only, use the retained isolated wrapper with
`HORDE_SKELETON_REVIVE_CAPTURE=1` and existing `--capture-showcase <directory>
--development-checkpoint player-viewmodel-forward --capture-hidden` arguments.
The Debug selector rejects visible/focused windows, competing capture selectors,
nonproduction body and nonempty events; it has a 150-frame/90-second bound.
It is absent from the normal path and does not prove natural encounter ownership.

Future integration must retain actor IDs/corpse roots and room engagement; do not
call `SwordCombat.Reset` to revive. One-time scream/rise/live state, fair attack
enablement, Replay/Skip, death/Retry and terminal save/load remain separate gates.
Audio/haptic revalidation: **NO** for this silent pose-only proof; the future
gameplay bridge has its own audio/feedback acceptance.
