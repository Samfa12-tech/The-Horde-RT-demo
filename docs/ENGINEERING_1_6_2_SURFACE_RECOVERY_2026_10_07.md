# Android RT surface pre-rotation — 7 October 2026

## Reproduced failure and bounded correction

Exact intermediate source `359a57112fd198351edcc6c4b6eae52f28bcd2a6`,
Debug APK SHA-256
`e8f0dee22d0093c69be8b704a61cf651003a6764eb227f89fab6e3c12c3eb48d`,
reproduces failed landscape recovery on allocated SM-S948B / Android 16.
The image is upright and the owner-requested scrollbar is visible, but the
native scene recreates about four times per second and editors remain waiting
for RT. Surface generation 1 reports current transform `0x2`, supported mask
`0x1ff`, selected pre-transform `0x1`, oriented window/current/swapchain
2981×1440 and traced 1491×720. Rebuilding selects the same mismatched tuple.
Private cases 15/16 in `task-4/integrated-162-359a5711-20261007/` retain that
failure. Returning to original portrait lock 0 and Restore recovers; every
preference entry is unchanged and owned PID 13055 is stopped.

[Android's pre-rotation guidance](https://developer.android.com/games/optimize/vulkan-prerotation)
and [Khronos' surface rotation sample](https://docs.vulkan.org/samples/latest/samples/performance/surface_rotation/README.html)
require matching the current surface transform and using the natural image
extent, with the rendered view transformed accordingly. The earlier identity
policy made the image upright through composition but did not establish stable
accepted presentation on this device. Its host-only pass is preserved as
insufficient coverage, not relabelled as device recovery.

The correction selects a supported concrete current transform. Quarter-turns
exchange the oriented surface dimensions once to obtain the natural image
extent. A shared presentation transform controls primary-ray UVs and view
aspect in both genuine Pipeline and RayQueryCompute backends. Player lower-view
mask and vignette use the same upright view UV; image writes keep the original
storage pixel. World geometry, lighting, held poses and simulation are unchanged.
The eight concrete rotation/mirror forms have mathematical host coverage;
unresolved INHERIT and unsupported transforms fail clearly.

The existing 128-byte / 32-float push layout is retained. Its output-mode float
keeps released identity values 0/1; bit 0 remains the presentation-format R/B
correction and bits 1–3 encode the transform. Shader and capture normalization
both decode that low bit, avoiding a channel-swap error for rotated frames.
The native acquire/present recreation log now includes both result codes and
the active tuple. Strict SUBOPTIMAL recreation, current-output frame ownership,
exact Graphics ACK and the normal post-ACK confirmation timer are unchanged.

## Validation and shader cost

The revised surface policy test fails against the previous implementation
(`surface-prerotation-before-fix-20261007.log`), then policy and mathematical
transform tests pass 2/2 in 3.35 s. Tests cover all eight corner/interior inverse
mappings, mirror order, aspect, packed transform/BGRA independence and rejected
unknown/unsupported flags. They do not prove actual rotated phone presentation.

The first freeze attempt without absolute output arguments is an invocation
failure, not a shader failure. The subsequent eight-mode compile validates
SPIR-V but the original frozen byte budget rejects the added presentation code;
no artifacts publish from that failed freeze. A separately retained eight-mode
matrix measures the exact increase:

| Family (each Diagnostic/Shipping, High/Mobile) | Bytes | Words | Instructions | Branch operations | Selection merges |
| --- | ---: | ---: | ---: | ---: | ---: |
| Generic dielectric | +1280 | +320 | +85 | +12 | +5 |
| Opaque fast | +932 | +233 | +59 | +12 | +5 |

Loops, functions, calls, ray-query sites, atomic instructions and all exact
variant/diagnostic/inlining invariants remain unchanged. The reviewed budget
change raises only those five measured maxima to the exact matrix values, with
no extra headroom or validator change. The original rejected attempt remains
in `surface-prerotation-raygen-freeze-20261007-02.log`; exact comparisons are
private `surface-prerotation-budget-comparison-private.json`. This is a required
presentation capability cost, not a gameplay FPS measurement or an owner
quality/performance acceptance.

Full Windows Debug build and Android Debug assembly with all four native ABIs
pass (`surface-prerotation-native-build-20261007.log` and
`surface-prerotation-android-assemble-20261007.log`). Nine affected native
contracts pass in 16.03 s: surface policy, transform math, entry scene/handoff,
preview frame adapter, compiled cache, bundle contracts/lifetime and scene ABI.
The first freshness run stops on two stale canonical compatibility include
hash witnesses; the Compute freshness check passes in 38.91 s. After updating
only the exact normalized include witnesses, manifest and artifact freshness
pass 2/2 in 126.85 s, retaining compiler/source identity, raw SPIR-V pins,
negative fixtures and unchanged-worktree assertions. Both the failure and
corrected result are retained in `surface-prerotation-shader-freshness-20261007`
logs. All eight Pipeline and eight Compute variants are regenerated and valid.
The prior owner-scrollbar Java regression and lint evidence remains applicable;
this slice changes no Java interface or UI code.

Integrated immutable package, actual phone rotations, ACK/Restore and
surface/Home recovery remain required. Host and build passes do not close them.

This internal bounded review is not Eric's independent audit. Final candidate,
owner visual/audio/haptic decisions and sustained phone performance remain open.
