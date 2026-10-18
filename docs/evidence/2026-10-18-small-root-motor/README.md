# Bounded small-root motor and body clearance

Continued in the existing `horde-1.7-wp2-vertical-proof` worktree/branch from
`ad5857ed` (Android Back confirmation), with no branch switch, new worktree,
owner cleanup, public push or release. Exact source/build/check identities are
recorded in [validation.json](validation.json).

An immutable, explicitly admitted STEP geometry lane now supplies conservative
footprint support for small roots. Bare terrain queries and tree grounding stay
unchanged. Ordinary props remain unsteppable; legacy collision and held-item
queries still treat STEP faces as solid. All lanes share the existing bounded
65,536-triangle intake. No visible asset or renderer admission is added here.

The shared simulation opts into the motor only for current gameplay with
STEP-bearing terrain outside the authored rope landing. It retains the existing
continuous nine-probe ground sweep, caps rise/descent at 18 cm, sweeps thin roots
between endpoints, retains stepped support through stationary refresh/pause,
and returns to actual ground beyond the root. Separate body queries reject
ordinary props, shaft/trunks, higher STEP faces and inadequate headroom. Ground
continuity is required even when a root spans a crack. Frozen historical
checkpoint behavior is preserved.

The terrain body query clips upward walkable faces above the local floor slope
band; steep, vertical and downward faces retain the full body band. This avoids
mistaking sloping ground beneath the feet for a wall while retaining higher
contact. It is a bounded fixed-height cylinder/footprint approximation, not a
full capsule/solid-interior solver, gravity, free jump or general climbing.
STEP support uses a conservative face-height maximum; it is not an exact foot IK
contact solver. The shorter 1.2 m fixture profile proves clearance separation only.
**Player crouch input, posture/camera/hand/body animation and runtime root asset
admission are still unfinished.** No obstacle requiring crouch is shipped.

The production fixture imports the actual `chapter-terrain-v1` manifest/GLB,
attaches a disposable 12 cm root at (-23.5,-3.4), restores a real safe-exit state,
publishes current terrain readiness and walks ordinary 60 Hz input along the
forest route onto/off the root in both directions. It also tests exact 18/18.1 cm
limits, an untagged identical prop, a taller root, stale foot height, low ceiling,
shorter body clearance, held-prop solidity, thin cracked ground and retired
generation rejection. The 10 root faces retain 680 additional CPU bytes in this
fixture; that is not a runtime scene, GPU budget or performance measurement.

Failures are retained under ignored `reports/`. The first live fixture used
the wrong absolute camera for the required neutral-after-load publication;
correcting the fixture allowed movement. The next run exposed a real regression:
an entire broad sloping triangle was rejected because a distant vertex exceeded
the local floor band. Local height clipping corrected it; tests now separately
prove nearby slope clearance and rejection when the contacted part exceeds the
band. Assertions were expanded, not removed. The final source/check roster and
results are in the receipt; older 30-check geometry runs predate body clearance.

The first broad CTest invocation ran inside the restricted subprocess sandbox.
Its real-file campaign session test failed in both configurations. The exact
unchanged binaries then passed all 58 checks in the authorized normal local
environment; no storage/assertion change or repeated-until-green loop was used.
Keep the failed invocations distinct from those isolated passes. The native
storage error was not printed, so its precise underlying sandbox mechanism is
not established. Other selected tests retain their own observed results.

No physical RT, visible root, crouch feel, new sustained timing or cloud CI pass
is claimed. CI/sanitizer target rosters are wired, with execution still separate.
Audio/haptic manual revalidation required: YES when STEP geometry is admitted to
the playable scene, because support height affects event-time listener/source
positions. This host fixture does not substitute for that physical acceptance.

Next bounded motor work: shared crouch posture with collision-height transitions,
blocked standing, coherent Android/Windows controls and a grounded walking pose;
keep low-ceiling sword clearance and save/load input rebasing coherent. Add an
actual authored root only with matching RT geometry/provenance and route checks.
