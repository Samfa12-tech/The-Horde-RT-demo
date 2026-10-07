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

## Immutable integrated device checkpoint

Source `868691fc11e7ecf52b0e6ff99d7bab31e2b63997`, tree
`4b63a195b88cab23edec98f165f08992bfee87a0`, is pushed to existing PR18.
Post-commit Debug assembly passes. The sealed package in private
`task-4/integrated-162-868691fc-20261007/` passes closed asset admission, all
four ABI payload checks, fullUser/0x480 manifest inspection and 16 KiB alignment:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Android Debug APK | 138462724 | `c865152bab855517fe48822c19fbda90bc242b4008e55b788c1f09d4d69ea93f` |
| Windows Debug executable | 11321856 | `4d86cd6d0fe628164b0bf97bb194eb1478c94ecda7973d80309b354b075beed9` |
| Packaged arm64 native library | 4359888 | `72176cb1832aef6aa61783a024a153f93524afca7c67ccab11ebccd089a77cc0` |

Installed phone base APK pullback matches the seal. Allocated device is
SM-S948B / Android 16; existing font scale 1.7 and override density 560 remain.
ADB-requested quarter-turns now select the current `0x2` or `0x8`, natural
swapchain 1440×2981 and traced 720×1491. Native controls stay in the landscape
window coordinate system. Actual screenshots show upright geometry, labels,
scrollbar and actions. The original portrait traced tuple remains 720×1490;
quarter-turn dimensions exchange axes without reducing traced work.

Pipeline cases 09–21 cover ready portrait, both landscape directions, upside-down
portrait and return to original portrait. Each rotation has one transition
recreation, not a repeated loop. Landscape Use serial 6 reaches an exact native
current-presented ACK in 155.078 ms, observed by UI in 172.994 ms; Keep and Restore
are available in the normal confirmation state. Restore serial 7 reaches exact
ACK in 167.548 ms, observed in 292.067 ms. No Keep/save is selected. The first
trial also exercises normal automatic rollback about 15 seconds after the
acknowledged trial, with its restore exact ACK retained.

Required genuine RayQueryCompute is independently confirmed in its capability
report and completed-frame shader identities. Its actual Entry scene is upright
in landscape; its authored preview is upright and ready in reverse landscape.
Use serial 3 reaches exact ACK in 346.954 ms, UI observation 590.601 ms; Restore
serial 4 reaches exact ACK in 197.848 ms, UI observation 290.917 ms. Both preserve
the normal timer and unchanged requested/effective/saved safeguards.

Home/resume on both backends intentionally leaves the preview and returns to
confirmed Settings, as `onPause` requires. A new surface generation 3 presents
the RT Entry scene, and accepted completed-frame reports establish fresh scene
epochs (Pipeline 9, Compute 8). Actual resumed output is portrait; this is not
evidence of a landscape Home/resume or physical owner rotation gesture. Pipeline
capability readbacks 23/24 advance accepted submission 4265→6665 in epoch 9.
Compute Home and later readbacks advance 3129→3249 in epoch 8. Periodic capability
reports are separate from the exact per-request ACK; they are not joined by
assuming identical timestamps.

Two private-helper expectations were wrong: editors are intentionally disabled
while Keep/Restore confirmation is active, and Home intentionally closes preview
to Settings. Their original captures/errors remain, followed by correctly
targeted confirmation and resume checks. An early Compute preview read has
`no-accepted-completed-frame` while presentation is already true; the later
settled read supplies accepted completed-frame evidence. This transient is
retained and is not relabelled as a completed frame or persistence failure.

Every XML preference entry is identical before installation and after both
backend blocks, including inactive metadata. Original `lock 0` is restored.
Owned phone PIDs 23157 and 29766 are stopped with receipts; production app/data
are untouched. These bounded checks establish recovery and acknowledgement,
not sustained gameplay FPS, power/thermal benefit or owner touch/audio approval.

The exact Windows executable passes Pipeline landscape and Compute portrait
Entry captures. Each completes six RT poses, no-Use Graphics Back, saved Graphics
preservation and actual Play handoff at epoch 7. Both owned processes exit with
zero synchronization-validation errors. Actual compositions were inspected.
All 12 aggregate source CI checks pass: push `37614824777`, PR `37614831521`.
Documentation after this runtime seal has its own CI. This remains an intermediate
inspection checkpoint, not the final review candidate or independent audit.
