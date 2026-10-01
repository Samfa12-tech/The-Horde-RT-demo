# October 2 report-relay integration — finite evidence

This is the current report-delivery continuation record, not a new renderer,
player, music or performance investigation. Preserve the original engineering
checkout, pending owner phone checks and unrelated raw S24 scratch evidence.
No main merge, game release or publication.

## Accepted checkpoints

- Public native envelope/foreground HTTPS owner: `cf156c2`, verified pushed on
  both `codex/horde-mobile-lantern-profile` and the engineering branch.
- Private shared relay: reviewed `fd104d7`, durable notes `0743e98`, pushed to
  its private branch. Existing mailbox, sender, bindings and provider retained.
  Deployed Worker `1ca58068-e876-4016-be6f-b1ac4d2035ed`.
- Current `cf156c2` push [36911249219](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36911249219)
  and PR [36911254051](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36911254051)
  both succeed. All ten actual job logs inspected: each GCC56, Clang56, MSVC58,
  focused Vulkan CPU-host15, Android52, zero failures/errors/skips; four Android
  native ABIs, lint and package guards pass. These are not new capture/UI results.
  Merge `39b7d73f6b79d583a9a2e7b06fcf0a9f4d7cf7dc` has exact main/head parents
  and the identical head tree `c7602884ea0bc4aeb722a25f5be029a1968267dc`.
- Live health/verification page200, invalid schema400 and real native PNG wire
  fixture with intentionally invalid token403. Invalid-token admission is not
  an email or successful Turnstile acceptance. Worker50/50 tests, including real
  workerd PNG/SQLite fixtures, pass. Tooling audit0.

## Vulkan readback exception safety

`CaptureStorageImage` previously allocated its output vector after mapping the
readback. Allocation failure could strand mapped memory and the temporary buffer.
CPU storage is now allocated before acquiring Vulkan resources; successful copy
is followed by unmap/destruction before moving the owned pixels into the result.
No shader, geometry, animation, AS, material or gameplay change.

Fresh Vulkan-enabled Windows Debug application builds. Affected submission,
scene-initialisation and resource-inventory CTests pass3/3. One fresh RTX5050
Laptop Debug/Diagnostic/High/RayTracingPipeline capture completes and honestly
presents the normal RGB, scene-only RT image with no overlays after12 settling
frames. `player-viewmodel-forward` PNG SHA256
`3afe620967f40985737f82cd119edc48424d482add83d542ebfad8bf09bd8189`;
960x540. This tests readback/presentation, not new phone, performance, glass or
player acceptance. Full local receipts are under
`C:/Dev/tmp/horde-report-submission-20261002/` (`capture-windows-debug.log`,
`capture-windows-debug-tests.log`, `rtx-capture/capture-manifest.json`).

## Consented Android game-image ownership

Readback happens only on the existing render owner, after this paused frame has
successfully presented and lifecycle pause is acknowledged. JNI transfers owned
RGB-normalised pixels/context through one generation-tagged ticket. Edit,
lifecycle interruption, render teardown and stale generations cannot publish a
different capture. No OS screenshot, UI overlay, arbitrary file, save or log.
Source readback is capped before allocation at8,388,608 pixels (32MiB RGBA;
GPU readback plus CPU source copy can transiently total64MiB). The disclosed
proportional report thumbnail is at most768 long/432 short, not a gameplay
render-resolution change;1080x2235 becomes371x768. Schema admission1280/720
and PNG512KiB remain unchanged. Attachment-cap failure is honest, not a silently
resized/omitted image.

Lossless RGB8 PNG Sub prediction emits only IHDR/IDAT/IEND with CRCs. The actual
RT noise fixture rejected960x540 at512KiB; unfiltered768x432 also exceeded the
cap. Sub-filtered768x432 is460809 bytes, SHA256
`462677a63e112389df28843ec25be7bf2abd21f57b97f8316d36ba9c2a7f1790`.
Actual workerd validates its deflate/pixels and614883-byte envelope, with outbound
requests denied (no verification/email). Smaller640x360 measured333010 bytes,
not selected as a second silent shrink. Initial Windows Node ESM path spelling
failed before the harness ran; corrected file URL yields the recorded pass.

Current native source: Debug3/3 affected reporting tests pass (the accidentally
broad selector also ran the unchanged release-updater preflight, passing4/4 total),
Release3/3 pass. New two-axis fixed-point bilinear gradient golden assertions
also pass both configurations. Android four-ABI Debug native build passes after
readback exception/pause-lock fixes and thumbnail-bound changes. Focused Java
PNG5/5, verification7/7 and submission16/16 pass, including before-dispatch and
in-flight interruption preserving the same frozen report. Receipts under the
same local directory: `capture-android-native-final.log`, `capture-native-*-final-tests.log`,
`report-helpers-final.log`, `png-sub-fixture.log`, `png-workerd-admission-final.log`.
These are host/compiler/local-runtime facts; JNI lifecycle races and real phone
readback/WebView acceptance are not claimed from these tests.

## Next unfinished step

Finish Android form/lifecycle review and focused activity tests, then build/lint
the complete candidate before device acceptance. Windows remote transport is a
separate bounded implementation slice. Existing Turnstile hostname admission needs the requested
owner dashboard sign-in; do not repeat the request or copy/broaden credentials.
Then send only the approved single labelled fixture email and verify the
attachment/delivery. No email has been sent yet. Windows remote UI/transport and
actual device/lifecycle acceptance remain open; local JSON remains fallback.

Audio/haptic manual revalidation required:NO (reporting/readback only; feedback,
playback, source events, assets, music and haptics unchanged).
