# Anatomical body mounting candidate (Phase 3 still open)

Prepared on `30e2679`. This is an explicit, unaccepted development profile,
not a production asset replacement or completion of the arm programme.

## Cause and bounded change

The preceding five-region experiment revealed the camera behind the torso.
The native mesh's face is near model Z=0; the current 1.8 m character does not
need a speculative scale change. `AnatomicalBody` grounds the body under the
player's yaw-relative origin rather than the old forward view-relative shoulder
plane. World body and viewmodel still consume the same solved pose, transform,
gameplay grips and independently owned GPU geometry.

Moving only the body exceeded the measured fixed-grip reach. Therefore the
immutable simulation configuration selects a bounded shared held-pose profile:
sword open depth .65 m; reward open depth .70 m (.69 m ring after the existing
inset); both hand paths raised .10 m. The torch retains its original .68 m depth.
The existing wall-response endpoints, combat timing, sword/cage lateral
separation, lantern pendulum, grip orientation, asset scale .44, skin weights,
rig, roll, topology and maximum chain stretch are unchanged. The targets drive
the actual items, arm IK and held lighting; there is no renderer-only prop offset.

The default profile remains byte-compatible with the existing route. Windows
requires Debug `--anatomical-player-mount` plus a modelled development capture.
Android requires `-PhordeAnatomicalPlayerMount=true`, an explicit paired-manifest
viewmodel candidate, and the existing Debug build guard. The renderer rejects
anatomical mounting without the dedicated viewmodel and body remainder. Profile
identity is published in simulation snapshots and capture/device reports. Android
also reports the actual mask/primitive-filter ownership predicate; its runner
checks that predicate and the requested profile rather than trusting a route name.

## Evidence

- Five selected Vulkan-enabled Debug tests passed: player animation, held-item
  sockets, simulation gameplay, actual skinned smoke and character slot smoke.
  After retaining the original torch depth, the affected three tests passed again.
- The attack/cage sweep covers both profiles. Simulation tests cover default
  behaviour, configured shared targets, reset, retry and checkpoint/profile
  retention. No deterministic golden was replaced.
- Eleven existing native portrait checkpoints passed. After the torch-depth
  refinement, grips/downward-cut/look-up were recaptured successfully. The first
  closer-torch candidate was not installed. Low/down, high/up and low/parry have
  zero separation at all 198 matched world/view sleeve boundary edges, and exact
  gameplay grip contact remains within the existing tolerance.
- A fresh legacy low/down capture is byte-identical to prior PNG
  `6682ff63384c0e147fad79944434ea249577e74a64c02daf6bb8bcb40ac668ba`.
- The incompatible four-region anatomical capture rejects with the intended
  diagnostic instead of falling back. Native shader source/variants did not change
  in this profile slice.
- Android ARM64 Debug builds pass. Gradle policy tests reject malformed profile
  selection, missing candidate and missing paired manifest, and retain the
  pre-existing receipt/hash/traversal checks. No paid asset work or licence change.

## Acceptance boundaries and next action

Native checks are not continuous live-motion or subjective phone acceptance.
The nearer lantern changes its apparent framing despite retaining the owner's
12% physical reduction; this must be reviewed honestly, not called accepted.
Armpit and wrist surface appearance, mirror motion, live attack/parry/carry,
and owner acceptance remain open. Natural occlusion and shadowing are valid.

Torso/legs looking-down acceptance is also still open. Current camera pitch
limits permit only about 20 degrees downward centre look; the lower image ray
is about 52 degrees down. A body correctly beneath the eye cannot make its feet
visible in that range. The next bounded change should separate deeper anatomical
camera look from the existing bounded held-pose pitch: look down at the physical
body without dragging the held lantern/arms through the floor. Preserve legacy
limits, existing pose checkpoints and the actual RT visibility contract.

Audio/haptic manual revalidation required: **YES for this candidate** because
shared held-item/light positions change and can affect positional feedback.
This does not reopen unrelated audio checks on the unchanged production route.
