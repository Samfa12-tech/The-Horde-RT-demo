# Lateral player movement: one offline proof remains unadmitted

Basis `9be9c7f826edb807959d1c20a5e03530ed6c3292`. No production source, bindings,
original assets, gameplay, saves, settings or shader changed. This is an actual
remaining directional-art gap from accepted plan section 5.5a, not a new movement
mechanic or a closure of that section. The canonical remaining map retains it.

Current `PlayerRenderSlot::LoadAsset` binds `PlayerLocomotionClipSet()` containing
Idle/Walking. `PlayerAnimationState` selects Idle/Walk; strafe uses the unchanged
forward sampling clock, while backward motion has its admitted phase correction.
The earlier four-direction procedural dodge remains separate and unchanged.
Neither runtime branch supplies a dedicated lateral walk/run clip.

One offline crossover candidate was authored on the unchanged owned rig at the
existing 1.9 m/s speed, with a 2/3-second cycle, 1.2667 m declared stride and two
foot exchanges. Feet retain their forward heading, with a depth stagger during
crossing. Stance local velocity cancels the declared translating root for contact
measurement; swing endpoints share the stance velocity. No root motion was added
to gameplay and the saved scene is an in-place preview.

| Evidence | First take, right / left | One knee correction, right / left |
| --- | ---: | ---: |
| Required extra leg reach | 94.74 / 95.74 mm | 0 / 0 mm |
| Maximum adjacent stance sole drift | 7.42 / 7.15 mm | 5.06 / 4.56 mm |
| Minimum sole world Z | -7.79 / -6.90 mm | -5.74 / -5.51 mm |
| Geometry/weights and bind unchanged | Yes / Yes | Yes / Yes |

The correction lowers the authored pelvis 120 mm rather than stretching the
limbs; it does not alter collision or camera height. It removes the reach fault,
but the inspected crossing poses still look cramped. Zero required extension is
not natural-motion acceptance. There is no new numerical contact tolerance, and
the per-frame drift is not a full-stance maximum or a baseline improvement claim.
Real-speed translating motion and native RT joins were not reviewed. No clip was
exported or installed; this family is not ready for production integration.

Original offline Blender images, **not native RT captures**:

- [First crossing pose](initial-right-11.png).
- [Same phase with knee correction](knee-cleanup-right-11.png).

[Editable source and reproduction](../../../assets/models/player/source/lateral-gait-study-r1/README.md),
[receipt](receipt.json), [first full samples](initial-contact.json),
[corrected full samples](knee-cleanup-contact.json). Both left/right binds and
geometry/weight digests match the source. No game asset package was constructed.
The optional `game-dev` CLI was not installed; the existing repository Blender
5.2 workflow was used, with no installation or provider call.

The initial Blender invocation exited successfully and saved its source, contact
data and ten previews. OpenImageIO separately failed to open the optional global
thumbnail `C:/Users/sam_s/.thumbnails/large/blender_28968_f1d68e3a9d9690fa3f0352bce7628017.png.png`;
the precise cause was not reported. That path was not retried and no permission
was changed. The correction disables optional file preview in its factory-started
process, does not save preferences, and completes without that write attempt.
This thumbnail is not needed to inspect or reopen the authored source.

Next: use a deliberate lateral gait/reference with credible weight transfer and
non-cramped leg separation, preserving aim and the accepted motor. Do not enable
this crossover candidate, stack another gait variation now, or present the source
as a completed feature. Native import, actual feet/hands/body secondary views,
interruption and phone feel remain future gates for a credible candidate.
The already passing `9be9c7f8` runtime/CI remains the active implementation.

Raw local source and all twenty previews remain under
`reports/player-strafe-study-20261024/`. All 151 parked owner files remain
unchanged. No phone, foreground activation, audio change, merge or release.
Audio/haptic manual revalidation required for this offline study: **NO**.

The existing asset-policy CTest passes 1/1 in 181.17 seconds, including its closed
runtime/archive roster and source-exclusion fixtures. The saved Blender file
reopens with both scenes and four actions; each proof covers frames 1-41. Source
syntax, relative links, production-asset SHA256 and all 151 owner hashes pass.
No unchanged native/Android application was rebuilt just for this source proof.
