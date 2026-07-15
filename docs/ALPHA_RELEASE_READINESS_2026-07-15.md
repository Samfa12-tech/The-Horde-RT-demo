# Initial showing alpha release readiness - 2026-07-15

## Decision

Prepare a bounded **Horde Lantern RT - Initial Showing Alpha 0.1.0** for Samfa12.com now. Market it as the native RT torch-corridor/material-gallery proof with one honest thin clear-glass skylight. Do not imply the planned framed mirror, stained-glass route, or final guided crypt slices are complete.

The Windows and Android alphas are public on separate itch channels. Itch is the canonical artifact host; Samfa12.com links there rather than hosting a second copy. Anonymous checks returned HTTP 200 for the itch project and purchase pages and exposed both versioned downloads. A rendered-browser audit of the live Samfa12.com `/games/` catalogue verified the Horde card, thumbnail, released status, itch download link, and GitHub source link.

## Features actioned

| Area | Android | Windows |
|---|---|---|
| Public identity | `Horde Lantern RT Alpha`, `0.1.0-alpha.1`, generated launcher icon | `HordeLanternRT.exe`, alpha title, icon and version resource, GUI subsystem |
| Start / pause | Branded entry overlay; Menu and Android Back pause/resume | Native pause overlay; Esc pause/resume; Windows menu bar |
| Actions | Resume, restart route, controls, settings, diagnostics, quit | Resume, restart route, controls, settings, diagnostics, quit |
| Settings | Persisted SFX enable/volume, look sensitivity, compact HUD, 50-100% RT render scale (100% default) | Persisted SFX enable, look sensitivity, fullscreen/windowed, 50-100% RT render scale (100% default) |
| Honest RT state | Polls native runtime state; active claim appears only after honest presentation | Status rail updates only after honest RT presentation; diagnostics refresh |
| Unsupported/error state | Automatically opens technical diagnostics; attack hidden | Explicit diagnostic/startup/render-error UI; no fallback |
| SFX | `SoundPool`, thirteen bounded FilmCow UI/combat/movement clips | WinMM asynchronous playback; movement cues never interrupt active combat/menu cues |
| Lifecycle | Portrait-first surface restart after Home/lock/resume; one native render loop | Resize/fullscreen swapchain recreation; input cleared on focus loss |
| Portable assets | Gradle-generated runtime assets | Executable-relative `assets/` first, source fallback only for development |

## Checks completed

- [x] Android debug build and lint pass after the UI/audio pass.
- [x] Exact debug APK installed on connected `SM-S948B`.
- [x] Device retained its user accessibility font scale of `1.7`.
- [x] Android selected strict ASTC material arrays.
- [x] Android capability report selected `RayTracingPipeline`.
- [x] Android report recorded `RT scene presented: yes` only after swapchain presentation.
- [x] Live Android alpha entry, scene HUD, pause/menu, and settings surfaces captured on the phone.
- [x] Android SoundPool loaded all thirteen clips and live menu/combat input exercised playback without a load/play failure.
- [x] Windows Release target compiles as `HordeLanternRT.exe`.
- [x] Windows assets resolve from an executable-relative packaged tree.
- [x] Windows uses the static MSVC runtime for a portable zip; Vulkan remains a driver prerequisite.
- [x] Windows live RT presentation verified at 100% and 75%; internal dispatch changed from `982x628` to `737x471` while warm BGRA colour remained correct.
- [x] Windows pause/settings controls visually verified above the Vulkan swapchain; render-resolution slider captured at 100%.
- [x] Rear wall, deep arch, skeleton-behind-arch staging, clear skylight transmission route, layered held-torch flame, footsteps, and attack cue compile on both targets.
- [x] Candidate APK audit contains four ABI libraries, strict ASTC assets, the skeleton, and exactly thirteen FilmCow WAVs; staged Meshy torch GLBs are excluded.
- [x] Android 16 compatibility gate passes: the APK is 16 KiB zip-aligned, every release ELF `LOAD` segment is `0x4000` aligned, and no r26 `libc++_shared.so` is packaged.
- [x] Fresh Android diagnostics display `1440x2980`, `78.01 fps / 12.82 ms`, and honest RT presentation after the initial 30-frame sample.
- [x] Candidate package hashes regenerated in `releases/candidates/SHA256SUMS.txt`.
- [x] Butler Windows channel published and verified after the documentation refresh: `samfa12/the-horde:windows-x64`, upload `#18339908`, build `#1798649`, user version `0.1.0-alpha.1`.
- [x] Fresh packaged Windows report records live `982x628`, `163.24 fps / 6.13 ms`, honest RT presentation, and no stale `not measured yet` diagnostics.
- [x] Butler Android preflight safely refuses debug/unsigned or missing stable-key candidates.
- [x] Stable Samfa12 Android release keystore created outside the repository with a 4096-bit RSA key; release APK verifies with APK Signature Scheme v2 and certificate SHA-256 `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.
- [x] Exact release-signed APK installed on `SM-S948B`; portrait rotation `0`, font scale `1.7`, strict ASTC, 100% `1440x2980` RT dispatch, honest swapchain presentation, and all thirteen SoundPool clips reconfirmed.
- [x] Butler Android channel published and verified: `samfa12/the-horde:android`, upload `#18341739`, build `#1798652`, user version `0.1.0-alpha.1`.
- [x] FilmCow source, custom royalty-free terms, restrictions, exact source clips, and conversions are recorded in `ASSET_LICENSES.md`.
- [x] Android/Windows controls, requirements, known scope, and site copy are written.

## Connected-phone final gate

- [x] Install the exact current debug-signed preview candidate after the compact-menu revision; repeat once with the stable-key-signed artifact before upload.
- [x] Confirm the portrait main menu, Android Back behavior, and settings layout at font scale `1.7`.
- [x] Exercise movement, 360 look, Swing, menu pause/resume, restart route, diagnostics, and live SoundPool use.
- [x] Exercise Home -> resume without a black surface or duplicate process; the PID remained unchanged.
- [x] Confirm strict ASTC runtime assets and honest RT swapchain presentation again from the current candidate.
- [x] Capture final menu, scene, settings, diagnostics, rear-wall, and lifecycle evidence.
- [x] Verify the render slider at 100%, 75%, and 50% with matching internal diagnostics and intact warm colour.
- [x] Record a sustained hot-phone pass at 75%: 21 120-frame windows, 10.933 ms median / 15.050 ms p95 at thermal status 3 (about 91.5 FPS median).
- [x] Restore the persisted default to 100% after testing. On this device, 75% is the recommended sustained-play setting; hot 100% performance remains route/view dependent and can fall below 50 FPS.
- [x] Install and sanity-check the exact stable-key-signed APK before Android upload.

## Windows final gate

- [x] Launch from a freshly extracted candidate zip using its packaged asset tree; report recorded `RayTracingPipeline` and honest `RT scene presented: yes` at `982x628`.
- [x] Visually confirm child HUD/pause/settings controls remain above the Vulkan swapchain after the `WS_CLIPCHILDREN` revision.
- [x] Verify the saved render-resolution slider recreates the RT target and honestly re-presents at 75%.
- [x] Exercise Resume, Restart, Settings, Diagnostics, Quit, F1/F2, fullscreen/maximize, minimum resize, swing input/SFX, and look sensitivity on the freshly extracted candidate.
- [x] Confirm `RT scene presented: yes` in the packaged `reports/` output.
- [x] Capture the final entry, live scene, settings, diagnostics, credits, fullscreen, and maximized/minimum-window states at the machine's active 125% Windows scaling.
- [ ] Repeat the already-DPI-aware candidate at explicit 100% and 150% Windows scaling in a later compatibility sweep; this is not a blocker for the 125%-verified alpha refresh.

## Resolved high-risk blockers

- [x] **Skeleton provenance/licence:** original Free Stylized Skeleton by Hotstrike Studio; user-created texture, rig, and animation processing through Meshy. Hotstrike modification/distribution terms and a conservative Meshy CC BY 4.0 attribution path are recorded in `ASSET_LICENSES.md`.
- [x] **Android signing:** Horde-specific stable Samfa12 keystore created outside Git and used through the secure environment-only packaging workflow.

The owner must still make an independent backup of the JKS and both passwords. `SIGNING_KEY_BACKUP_REQUIRED.txt` is stored beside the keystore as a recovery reminder; losing this key prevents compatible Android updates.

Stable signing workflow for future rebuilds:

1. Use the existing Horde-specific keystore; do not create a new identity for an update.
2. Back up the JKS and both passwords independently outside the repository.
3. Run `tools/package-signed-alpha.ps1 -KeyStorePath '<outside-repo path>'`; passwords are prompted securely and removed from the process environment afterward.
4. Install the exact signed candidate on the target phone before every Android publication.

## Itch publication and Samfa12.com link checklist

- [x] Treat itch as the canonical artifact host; no direct Samfa12.com artifact upload is required.
- [x] Use only the final signed Android APK; never upload the `UNSIGNED-DO-NOT-PUBLISH` artifact.
- [x] Upload the Windows x64 zip after clean extraction validation (`samfa12/the-horde:windows-x64`, build `#1798649`).
- [x] Publish supported/tested device wording, controls, honest unsupported behavior, alpha scope, and download labels.
- [x] Final candidate SHA-256 values regenerated in `SHA256SUMS.txt`.
- [x] Bundle `ASSET_LICENSES.md` in the Windows ZIP and link the public source repository containing it.
- [x] Run the guarded Butler workflow for the Windows package; the extracted zip is published to `windows-x64`.
- [x] Guarded Butler workflow published the non-debug, hash-matched APK to `android`: upload `#18341739`, build `#1798652`.
- [x] Public project and purchase pages return HTTP 200 and expose `the-horde-windows-x64.zip` and the signed Android APK, both version `0.1.0-alpha.1`.
- [x] The itch page links the public GitHub repository.
- [x] Render and anonymously verify the Horde card and itch/GitHub links on Samfa12.com `/games/`; the catalogue is data-driven and is not expected to appear as literal text in the static HTML or sitemap.

## Remaining post-publication checks

- [x] Build the next Windows candidate with an embedded Per-Monitor V2 manifest, DPI-scaled overlay geometry/fonts, minimum window sizing, and an in-app Credits & Licences dialog; packaging now extracts and verifies the manifest and credit markers.
- [x] Build the next Android candidate with an in-app Credits & Licences panel and a side-by-side `.debug` application ID; release resources retain `com.samfa12.hordelanternrt`, portrait orientation, and `0.1.0-alpha.1`.
- [x] Validate the next-candidate UI changes live on Windows and the side-by-side Android debug app; the unsigned Android release remains unpublishable.
- [x] Complete the extended Windows interaction sweep at the active 125% scale; explicit 100%/150% compatibility repeats remain a non-blocking later check.
- [x] Add direct Poly Haven, FilmCow, Hotstrike Studio, Meshy, CC BY 4.0, and full-provenance credits to the live itch description.
- [x] Mark both Code and Graphics in itch's AI disclosure and add the exact Meshy/OpenAI assistance description to the public page.
- [x] Post a public permission request to Hotstrike Studio asking whether the processed GLB may remain in the public GitHub source repository; request post `16578566` is awaiting a reply.
- [ ] Resolve public-source redistribution of the Hotstrike-derived skeleton GLB. The asset may ship inside a finished game, but the current public Git/LFS history exposes the raw derivative; removing it from history requires an owner-approved rewrite/force-push or explicit permission from Hotstrike Studio.
- [ ] Create tag `v0.1.0-alpha.1` only after the documentation commit is pushed and these remaining public-page/Windows checks are closed.
