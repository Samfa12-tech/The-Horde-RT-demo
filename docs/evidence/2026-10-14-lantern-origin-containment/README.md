# Claimed lantern origin containment prerequisite

Source parent: `ba2b8cc386489c138fff7ebae3ecd666db503609`. This checkpoint fixes a
reproduced side-wall crossing before any ordinary static-region eviction. It
does **not** admit a partial GPU owner. Normal static-region savings remain zero.

The real imported player rig and lantern reproduced glass outside the coloured
passage wall. The first failing native test retained Z=-16.806524 against the
wall at -16.8. At the north wall, the old zero-swing glass reached -13.5749
against -13.6. That zero-swing physical light was still inside; it was not proof
of light escape. Failed tests and captures are retained with the receipt.

The gameplay hand target now fits a conservative, orientation-aware glass and
four-area-light envelope inside the exact rectangular cover of the tomb's
walkable footprint union. The ring, swinging cage, flame and light follow that
one target. The cover bridges genuine aligned portals and cannot bridge an
empty gap or concave corner. Exterior carry, the camera, inputs, pendulum limits
and production shaders are unchanged.

The bound covers continuous swing and torsion, rather than only selected
angles. It includes the native glass bounds, .44 rigid scale, .097 ring-to-hinge
separation, 15 mm final Grip position tolerance and 6 mm ray spawn allowance.
The .02-radian **per-axis** Grip orientation tolerance is converted to the
conservative SO(3) chord `sqrt(3*(1-cos(.02)))`. The ordinary full-weight skin
solver restores hand orientation; native tests check the actual reward Grip,
not the detached torch. Asset bounds are pinned in the native test and receipt.
Unknown/nonfinite/unfittable states are not certified by this correction.

The initial circular bound broke the existing front-wall CPU pose assertion.
The retained failure led to the oriented bound; that original assertion passes
without modification. Native coverage now includes 324 actual-rig cases across
both side walls/front wall, High/Low carry, both aspects, pitch limits, diagonal
swing and torsion. Empty gaps, real portal joins, narrow fits, nonfinite input
and 520 continuous passage steps are checked separately. Original checkpoint171,
its nine negative cases, descent59 ticks and the full 180-second torch policy
remain intact.

## Inspected RT images and capture attribution

Eight matched side-wall captures pass, before/after on both hardware RT backends
with High and Low **lantern carry**. These are not Current/Higher shadow-tier
comparisons: all use the same recorded Higher shadows, High optical profile,
960x540 output, fixed camera(-16,-13.84), yaw -pi/2, pitch -.05 and 12 settling
frames. Exact shader, completed-frame and artifact identities are in the receipt
and archived manifests. Root opened the actual images. The corrected cage moves
clear of the wall with its hand; these frozen views do not certify moving owner
feel, sustained performance or phone acceptance.

The first captures failed with a constant18 `primaryTorchPixels` despite a zero
hand-torch mask. Source attribution established that both retained world Keeper
torch bodies use legacy custom index1, which the frozen shader counts as torch.
The production hand torch uses custom22. The capture gate now requires BOTH
hand-torch TLAS masks to be zero, and allows nonzero legacy pixels only with an
active world-torch alias. Eight positive/negative compile-time cases preserve
rejection of a visible hand torch or an unexplained count. Raw counts remain
recorded. No shader instruction/query budget, ABI or production mask changed.

The direct front-wall frozen fixtures132/133 still fail the primary-arm gate in
**both** baseline and candidate. All four remain rejected; they are not accepted
visual evidence. Their original native CPU assertion passes. This observed
limit and the actual uploaded meshes are retained rather than weakening the
visibility assertion. Full visual acceptance remains open.

The shared moving out-and-back/repeat-ascent/reset diagnostic succeeds on both
backends: 1241 rendered route frames and27 gallery captures each. Return-to-tomb,
second ascent and reset images were opened for this carry change; the complete
gallery/trace identities are retained. This starts after staged death/first
ascent with static resources resident and uses up to four simulation ticks per
render. It is neither a complete opening-to-Keeper owner playtest nor Shipping
performance/ordinary streaming evidence.

The before build is a deliberately mixed diagnostic baseline: current app
objects and libraries, with only `HeldItemKinematics.cpp` replaced by its exact
ba2b blob through an explicit object. It is not presented as a whole ba2b build.
The link derivation, object hashes and executable hashes are recorded.

Pipeline, High carry, before:

![Pipeline High carry before](gallery/Pipeline-before-high.png)

Pipeline, High carry, corrected:

![Pipeline High carry corrected](gallery/Pipeline-after-high.png)

QueryCompute, Low carry, before:

![QueryCompute Low carry before](gallery/Query-before-low.png)

QueryCompute, Low carry, corrected:

![QueryCompute Low carry corrected](gallery/Query-after-low.png)

## Validation and remaining gate

Affected Release tests pass8/8 plus the capture-ownership/controller test1/1.
Debug retained the first7/8 result with its registry-size failure; the corrected
registry/native selection passes2/2 and ownership/controller1/1. Other Debug
checks, including the original full torch test137.68s within180s, passed. Android
four-ABI Debug build/lint and actual closed-roster/ASTC package checks pass. No
phone install, capture or input occurred. Artifact hashes are intermediate,
uninstalled candidates. CI belongs to the exact subsequently pushed commit.

Audio/haptic manual revalidation required for this slice: **NO**. No playback,
gain, event-time listener/source, combat or haptic input changes. The larger
chapter's outstanding listening/device acceptance remains separate.

The ordinary streaming gate still requires retained opaque-shell and all
ray/interface/light contributor admission, collision/render-ready loading,
completed-work retirement and correct return reload. This hand envelope is one
prerequisite, not permission to retire geometry. Controlled optimized Release
pacing/memory measurements and physical Current/Higher plus saved Lower-to-
Current migration remain pending. The earlier desktop CPU-load explanation is
unconfirmed; these hidden Debug captures are not playable-FPS measurements.
