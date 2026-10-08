# Moving sword-contact evidence and owner disposition — 9 October 2026

Test-only source `5a2a466166babd9646eb7031124f009c11ed6942`, tree `54c994daa3c1e6d4c1adb23a96fc1c79c4928380`, adds strict opt-in modes to the existing excluded host witness. Production combat/pose/range/cone/parry and the installed `ac97da91` runtime are unchanged. At the owner's explicit decision, **keep current combat for 1.6.2 and document the forgiving hit-detection limitation**; further precision animation/contact correction is deferred from this goal.

## Bounded findings

Ordinary 60 Hz targets start 1.52 m away at 0° and −15°. Damage authority updates the player before moving the target that tick; the witness pairs post-update player pose with the control's prior target snapshot, rather than using a later, already damaged/dead pose. At pulse17 the target has walked approximately 165 mm, remains Walking, and is at range 1.35467 m. Both gates admit damage.

| Sample | Frontal exact blade/mesh gap | −15° exact blade/mesh gap |
| --- | --- | --- |
| Real damage pulse17 |215.684 mm|17.5033 mm|
| Counterfactual late downstroke19 |182.992 mm|23.6694 mm|
| Counterfactual bottom-hold21 |179.313 mm|0 mm|
| Counterfactual bottom-hold23 |206.105 mm|0 mm|

Late samples use an unhit control target; normal damage has already killed the actual target at17. Its control transitions from Walking to AttackWindup. Later intersections are **not new hit admissions**, presented-frame contact or a reason to delay all damage. The frontal gap stays positive, so one shared delay is unsupported. Selected triangle-centroid offsets add component context; they are not exact nearest-surface displacement vectors. Region labels remain nearest actual imported joint segments, not source skin-weight classifications.

The two modes together add only ten full-mesh queries, using the full 6,905-triangle blade and 9,402-triangle target. The unchanged static24-query experiment is not rerun. No runtime sweep, new collider engine, range extension, parry widening or timing/trajectory change is introduced.

## Checks and exact scope

The final explicit host target builds in3.303 seconds; `--moving-downstroke` completes six queries in1.266 seconds under a60-second bound. Unknown/extra arguments exit2 with no mesh-query stdout, preventing accidental expensive static runs. `--moving-pulse` retains its earlier four-query1.013-second evidence from source-file hash `ea58df95dc66b5d50febbb4e84329c1952cd79809fb49652a8c4e45a75f82431`, before the shared-helper refactor. It is not relabelled as execution of the final source. The final file hash is `698c824a2a0bdb6a64ef1f931133abbbd0e8a1e03a44022a0123a39190970976`; final witness EXE `33351efda8bdec675179c5c9f247d372e567830fc0b139bf27564eb886426964`.

Fixture/loading/motion/query-budget assertions pass; **exact contact agreement fails in the reported positive-gap cases**. This distinction remains explicit even though the investigation executable exits0. No phone, input latency, scanout or continuous collision pass follows. The target remains excluded from ordinary builds/CTest/CI execution; its source receives separate CI after push.

[Sanitized receipt](evidence/2026-10-09-moving-contact/receipt.json), [pulse rows](evidence/2026-10-09-moving-contact/pulse-rows.txt) and [late counterfactual rows](evidence/2026-10-09-moving-contact/late-counterfactual-rows.txt) retain exact artifacts and observations. Earlier rig agreement, static witnesses and rejected proxy/sweep findings retain their original scope. Audio/haptic acceptance and long sustained-performance deferral are separate owner decisions. No merge, signing, tagging or release.
