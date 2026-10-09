# Shared sword and player Rag clearance — 7 October 2026

The sword response uses the imported production blade bounds relative to its
actual Grip: long axis −0.135..0.91526 m, edge ±0.112 m and flat ±0.025 m.
Conservative samples include the cross-section and half-step vertical pad. Roof
footprints receive the existing smooth anticipation policy; whole-footprint
broad-phase rejection avoids sample checks against distant roofs. This is held
equipment clearance, not a new combat hit sweep or camera/crouch mechanic.

Lowering moves the shared right-hand target in world space. A camera-side grip
retreat capped at 0.30 m keeps the imported arm reachable. The same resolved
hand/item frame drives rigid RT geometry, arm IK, shadows and reflections.
The player Rag Flame is 0.565 m above Grip; its complete envelope is 0.965 m
including unchanged engine fire and animated-tip reserve. The original Keeper
torch's 0.925 m contract remains separate. Anatomical torch retreat now permits
up to 0.45 m under a low roof, correcting the left-hand reach failures observed
with the longer Rag socket. Open poses and authored drench/drop/reward ownership
remain unchanged. Actual moving appearance still requires inspection.

The clearance test imports the actual Rag mesh/sockets and checks 9,696 portal
samples, including body, engine fire and rigid socket composition. It passes;
maximum carry response is 0.798740 m and walk/look hand step 0.0307841 m. The
already-lowered Rag residual is 0.0887203 m; its bound accounts explicitly for
the measured 0.040 m Flame-height increase rather than retaining the former
asset's envelope assertion.

The actual production `PlayerRenderSlot` test resolves all 60/60 anatomical
poses across both lintels, three pitches and ten idle/swing/upward-slice/parry
phases. Maximum left/right socket error is below 11 micrometres, within the
unchanged 15 mm contract; composed Grip orientation error is zero. Every imported
blade vertex clears the tested roofs after the final rig solve. Five-millimetre
approach samples change lowering/hand position by at most 2.764/2.962 mm.
The open baseline is a valid standing point 0.70 m north of the skylight centre,
placing the carry inside the aperture with no roof anticipation. A player at
the bare room centre still has a small, correct anticipation response; it is
not a valid zero-response fixture.

Windows Debug app builds and all seven affected suites pass in
`post-reset-foundation-native-tests-20261007-03.log`. Focused rig build/test and
direct counters are in `held-item-socket-*-20261007-07.log`. Earlier compile,
stale-fixture and strict reach failures are retained. These host tests do not
certify every moving RT frame, physical phone cost or owner visual acceptance.
Android packages, swing/drench/reward motion, world body/shadows/reflections,
phone work/pacing and owner feedback remain open.
