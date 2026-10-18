# Playable crouch and one trail root

Continues in the existing `.worktrees/horde-1.7-wp2-vertical-proof` worktree and
`codex/horde-1.7-wp2-vertical-proof` branch from `1bf8921a565dd92f40ca2c8b4c6edf651ff0e2c9`.
The source/build identities and actual checks are in [validation.json](validation.json).
No new checkout, branch switch, owner cleanup, public push or release is involved.

Windows Debug and Release applications build; the same 15 selected native tests
pass in each configuration. All 68 focused Java tests pass, and Android Debug
assembles for arm64-v8a, armeabi-v7a, x86 and x86_64. The exact APK is
`b1f266f69adf6b92eda2c18fe7c02aa967cbb626d853ef4dd910ab1394f82211`
(183,853,322 bytes). These are affected checks, not a complete release gate.

Shared 60 Hz simulation now owns a bounded 1.8 m standing / 1.2 m crouching
profile with a 0.25 s transition. Standing requires full-height clearance;
release beneath a roof retains the safe crouched height until clear. Foot
support remains independent. Crouching caps actual movement at 1 m/s and
consumes run/dodge requests without activating them. Pauses clear intent while
preserving safe posture; rope traversal owns its existing separate pose and
consumes simultaneous/stale stance commands. Safe checkpoint reconstruction
restores standing with neutral controls; transient stance is not added to the
save schema, and a live crouch is not admitted as an arbitrary manual snapshot.

Android has a touch Crouch/Stand toggle and L1 hold; Windows has Ctrl and
XInput LB hold. Existing action bindings stay connected to the coherent input
mailbox. Native accepted posture drives the Android label and blocked-standing
message. Lifecycle, focus, controller takeover/release, pause and load boundaries
consume old input rather than replaying it. Small layout regression coverage
includes compact 320 px / 85% controls.

Camera, held-item solves, frame light offsets, fog/dust camera and event-time
listener height use the same posture drop. The existing indoor visibility
certificate checks the actual lowered camera/light; a pose leaving its valid
light prism restores the full owner rather than omitting geometry. No shader,
ray-tracing backend, render scale or quality policy changes. The shared body
palette lowers the hips and solves bounded leg IK while preserving animated
ankles and leg lengths. It keeps the separate rescue planted-pose path intact.

One original root is admitted near trail segment 3, fraction 0.40. It spans
1.10 m across the trail and rises 10 cm above surveyed ground. Its exact 20
triangles feed both the existing NativeForestBark RT owner and immutable STEP
support. It adds 60 vertices/indices: 4,080 native geometry bytes and 20 region
bytes, before allocator/AS/GPU overhead. Those are data sizes, not measured GPU
memory or performance. The old ten off-trail placements and 11,884 nonwalkable
triangles are unchanged; the combined owner has 11,904 triangles. Original
geometry provenance is recorded in `ASSET_LICENSES.md`. No obstacle requiring
crouch, jump/slide, general climbing, free physics or heavy exterior expansion
is introduced.

The tests exercise imported production terrain and real ordinary-input
simulation: slopes, thin roots, forward/reverse crossing, lowered clearance,
blocked standing, return to ground, pause, crouch speed, feet/held props/camera,
safe save restore and rope input ownership. The rig sweep covers 288 idle/walk
samples including maximum drop. Java checks cover coherent controller/touch
publication and lifecycle/layout regressions. Build/test outcomes are recorded
separately from the bounded phone receipt and physical acceptance.

Rejected results remain under ignored `reports/`, with hashes in the receipt.
Initial compile errors (missing geometry include, a namespace and a test roof
initializer) were corrected. The first root shape had a flared base contacting
the body before its eligible top; matching the ground-draped upper/lower outline
fixed ordinary crossing without changing limits or assertions. Seven walking
rig samples initially sank skin below the boots by 4.4–22.9 mm despite correct
ankle transforms; an upward crouch knee pole fixed the skin sweep, preserving
the rescue pole and checks. Compact stance placement initially overlapped the
lantern; controller takeover initially cleared a fresh L1 press. Both have
explicit regression coverage. An initial new-rig CTest command did not execute
the binary; direct execution exposed the seven real failures before correction.
An owned restricted-environment MSBuild hung and was stopped; authorized normal
local builds and final checks are recorded separately, with no owner process killed.

Physical full-route/ceiling feel, controller/keyboard ergonomics, skin/held-item
art acceptance, audio/haptics, saving UI/lifecycle, cloud CI/sanitizers and
sustained Android/Windows RTX performance remain open. No physical root walk or
new performance gain is inferred from these host tests or a stationary phone
crouch. Manual audio/haptic revalidation required: **YES**: listener/source
height and root support/footsteps changed. Reuse the existing pinned exterior
and two-torch gates before further heavy content. The next product checkpoint
should remain one bounded existing gap, with actual performance acceptance and
platform save entrypoints still on the 1.7 critical path.
