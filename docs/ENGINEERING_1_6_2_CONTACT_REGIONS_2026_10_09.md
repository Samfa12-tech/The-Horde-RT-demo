# Bounded sword contact-region witness — 9 October 2026

Test-only source `f0cd8cfcc41b613ebe68a8eb7fe04510821f965c`, tree `57c8927d2fb71a81d7d312087ecd1447d8170a34`, adds `horde_rt_sword_target_contact_region_witness` as an explicit **EXCLUDE_FROM_ALL** Windows host target. It reuses the actual imported poses, sword mesh and anatomical player/Grip helpers; no runtime combat, render, parry, range/cone or hit-timing behavior changes. This is a bounded investigation, not a selected collision engine.

The current gameplay gate uses root XZ range 1.72 m, forward-dot 0.52 and nearest living eligible target. The witness uses ordinary immediate-attack and idle snapshots 16–23, drawn sword, high torch, aspect 1 and 1.28 m root range. Undamaged control supplies the target's Attack Windup pose. The fully engaged actual-rig agreement, preceding-tick bracket and rejected coarse-capsule result retain their [earlier exact scope](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#bounded-preceding-tick-contact-probe-8-october-follow-up); unchanged closed negative experiments are not rerun.

A single local Windows x 64 Debug build/run finishes in 10.006 s, exit 0, within a 60 s run limit. Exactly 24 full-mesh queries use 6,905 blade triangles and 9,402 target triangles; BVH lower-bound pruning leads to 1,125,143 exact leaf triangle-pair tests. The query returns nearest blade/target triangle IDs. Region labels identify the nearest actual animated imported joint segment to the target triangle's centroid; they are geometric labels, **not source skin-weight classifications**. Runtime asset API does not expose source weights; tied zero-distance witnesses are not enumerated.

| Case | Gap at admitted attack pulse 17 | Closest sampled contact 16–23 |
| --- | --- | --- |
| Frontal 1.28 m attack |275.347 mm|36.2886 mm at 23; no intersection|
| −15°1.28 m attack |234.658 mm|Intersection at 19,20,23|
| Frontal 1.28 m idle control |No attack/damage pulse|192.863 mm at 23; no intersection|

Both attack cases are range/cone eligible and admit damage at tick 17 despite a positive visible-mesh gap there. The selected nearest target triangle often lies near the posed forearm/hand segments. These discrete fixtures show bearing-dependent downstroke contact after the current pulse in one case; they do not prove continuous contact, moving-guard contact or exact presented-frame timing. They do not justify one uniform pulse delay, extending reach/cone, widening parry or adopting a compact collider/sweep. Calibration remains open.

[Sanitized receipt](evidence/2026-10-09-contact-regions/receipt.json) and [all24 rows](evidence/2026-10-09-contact-regions/rows.txt) retain exact sample and hash evidence. Witness source SHA-256 `9c6046bc19fa83f459e2e4b9a5d5823e5c6ed0f77854d31b88102979cfe4df23`; executable `497bb4baea6729d6dbdda505d4c8e46ee2fb5fd6d82e565953d6a2990a4165b1`; raw run log `88d36c26e1515f9ef80885d51e69f233d5afb2af97f63f583471d63610e00fe0`. Two initial namespace build failures and the corrected build remain privately recorded.

Explicit local invocation: `cmake --build build/presets/windows-x64-debug --config Debug --target horde_rt_sword_target_contact_region_witness`, followed by its executable once with a bounded 60 s timeout. Ordinary builds/CTest/CI **do not execute this expensive witness**. Source CI separately passes 12/12 aggregate jobs: [push37829424238](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37829424238), [PR37829429505](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37829429505). That CI result does not replace the explicit local witness evidence or close moving-contact/owner acceptance.
