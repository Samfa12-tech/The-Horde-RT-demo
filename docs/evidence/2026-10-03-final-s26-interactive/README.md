# Current-bank S26 Shipping-path interaction admission

October 3, 2026; exact **SM-S948B**. This is a bounded interaction/package check,
not another performance experiment or subjective player acceptance. Runtime
`3d26ad64a3db1e1e1b7965587a72fd114189bdf6`, current score/assets from
`ec13876069b4049b524c5198734c0bbbabbd039c`. The separate completed
[resource comparison](../2026-10-03-final-s26-resources/README.md) retains its older
unchanged music bank and cannot be relabelled this artifact.

## Exact artifacts and scope

- Debug package `com.samfa12.hordelanternrt.debug`, version `1.6.1-debug` / code 9,
  with **Shipping/Mobile shaders**, native/Java Debug shell and assertions.
  APK SHA-256 `6eab75f8d829c8756a5c45f222f77104954b4010d24f7a9d479893d6e480f85a`;
  installed/pulled bytes match. This is not non-debuggable Shipping performance.
- Unsigned production-ID `1.6.1` / code 9 APK SHA-256
  `79ceaa7a9ba4faa10410ee3e174fdc5ca58906983cc2b1342acc242e4fbe14e9`;
  **not installed, signed or published**. APK 16 KiB zip alignment passes.
- One fresh four-ABI Debug/unsigned Release build with explicit validation-only
  signing suppression and CLI Shipping/Mobile overrides succeeded in 2m1s.
  Exact current 17-entry music manifest/WAV admission passed for both APKs;
  no source score or editor/synth code is packaged. Runtime PCM remains
  20,160,000 bytes. No unrelated bank, material, sampling or resolution cut.
- Extracted actual ARM64 ELF hashes: Debug
  `cf65cc3cc0aaa8e38a9ab0b3e22624aaf83461445e81292c3b2c05e034e01f74`, Release
  `b836454af1d4a0d51b7958d816b14d31073a08d83ecb45505188ccdef344cd71`.
  Both actual four-module bundles pass `spirv-val` / disassembly with no binding
  22 and zero diagnostic atomics. Release ELF is byte-identical to the resource
  benchmark ELF, not evidence that the APK/music bank is unchanged.

## Finite device checks

| Check | Actual result / limit |
| --- | --- |
| Entry and ordinary scene | Native RT active; textured hands and two skeletons visible in the current build. This is image/presentation evidence, not sustained gameplay pacing. |
| Live held lantern | Existing player high-lantern checkpoint 142, followed by live movement/look, sword inputs and high/low controls. A 7.888s screen recording and selected frames show lit open-aperture lantern, hands/sword and changing poses/view. It is not proof that every cooldown-filtered input produced its full animation or a new owner acceptance. |
| Render scale in heavy live scene | 75→100→75→50→75, UI values checked; same process and surface generation, four `output_only=1` results. Native idle+resize times 86.620, 131.290, 86.248, 46.314ms respectively. Each reaches presentation; final image retains the held lantern/hands and vitality 3. These times exclude touch/UI/compositor latency and are not an A/B FPS gain. |
| Home/resume | Same process 11942; generation 3 cancelled, generation 5 requested, started and honestly presented. Initial starting UI retained; readiness came about **14.6s** after the request. Vitality 3, lower-lantern state and native RT active survive. No instant-resume or external audio-focus claim. |
| Restore normal configuration | Shipping overrides were command-line only. Normal current-bank Diagnostic Debug APK `315b1c9f4ba99f397f1aa638fd0b19f251a7dd1877c3d9fff6680587feda3f04` restored without clearing data, saved scale 75% or volume choices. Separate production 1.6.0 installation untouched. |

The owner already accepted A and D as “both perfect”; no new audition is required
for the unchanged playback code here. Distinct external audio-focus interruption,
phone footstep-balance/subjective cuff follow-up and current final S24 artifact
acceptance remain separate. S25 is unverified. A short screen recording's encoded
frame rate is **not** gameplay or display FPS.

## Retained evidence

`SHA256SUMS.txt` binds the allowlisted raw images/UI, actual module receipts, scale
log and game-only lifecycle/music subset. Full local APKs/ELFs/video remain under
`C:/Dev/tmp/horde-final-s26-interactive-20261003`; the video hash is in the receipt,
not an uploaded screen recording. No launcher/private-app images, app lists,
credentials or unrelated logs are archived. The initial finale checkpoint/UI and
failed pre-readiness/missing-directory attempts were not used as successful live
evidence. These observations do not reopen accepted player or glass work.

Audio/haptic manual revalidation required: **NO** for these renderer/resource
checks; the separately outstanding interruption/changed-footstep checks remain
open. No merge, release, signing recovery or publication.
