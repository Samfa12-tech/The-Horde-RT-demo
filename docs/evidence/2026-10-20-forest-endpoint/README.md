# Forest lookout recovery checkpoint — 20 October 2026

The approved 1.7 plan requires a durable forest endpoint (section 12.2). Earlier
saves exposed only LowerClaimed and SafeExit. Reaching F04 completed the route
and story, but loading still returned to the summit and reconstructed Kit at the
reunion. This local slice adds **Forest lookout** to the existing save system.
[Exact source hashes, artifacts and results](receipt.json).

The new boundary is earned near F04 only after the existing victory/lantern/
ascent guards, Lookout residency, grounded standing player, grounded completed
Kit route, presented fresh-raise proof and all four forest story lines. The final
line and continuation queue must be finished. The canonical player anchor uses
F04 X13.3/Z36.2, yaw2.51/pitch−0.15; Kit uses its existing F04+1m X endpoint.
Both elevations and standing clearance come from current native terrain.

Restore remains an offside shared-simulation transaction, with fresh identities,
neutral input rebase and the existing exact RT presentation barrier. Kit starts
idle at the lookout; completed reunion/trail speech does not replay. The endpoint
latch survives exterior exploration without downgrade or repeated autosaves.
An actually completed descent can later earn the lower safe side while retaining
forest completion and Kit at the lookout. Android and Windows name the actual
checkpoint; no new menu page or save architecture is introduced.

Format 1/schema 1 and 142-byte payload / 162-byte file stay fixed. Revision 1
records keep their validation, interpretation and golden bytes, with no eager
rewrite. Content revision 2 represents an admitted completed forest, including
later lower/summit safe-side returns; incomplete story facts are rejected. It
begins only after endpoint admission, rather than speech flags alone (Kit may
still be catching up). Older executables reject revision 2 as unsupported;
unknown revisions remain unsupported. No owner records were migrated or edited.

| Validation | Actual result |
| --- | --- |
| Campaign/dialogue/barrier/menu | 10/10 Release CTest suites pass. Includes 966 checkpoint checks, 2283 codec checks, 145 play-session checks, 77 session checks and 45 hidden Windows menu checks. [Run](forest-endpoint-native-tests.log), [details](forest-endpoint-native-tests-detail.log). |
| Earned host journey | Existing ordinary-input opening/guards/Keeper/claim/ascent journey now continues through fresh raise, real trail movement and F04 speech/Kit completion, one real disk autosave, exploration without downgrade, fresh reload without rewrite, and return/descent preserving completed story. No checkpoint seed, authoritative pose or direct quest mutation earns that journey. CPU readiness/presentation injection, damage disabled and teaching disabled remain explicit limits. |
| Storage failure | Separate injected endpoint auto/manual failures retain prior valid bytes and other slots; explicit manual retry succeeds independently of the failed auto. This is host fault injection, not a phone disk/power-loss claim. |
| Restore rejection | Separate player-only and Kit-only blocked headroom, wrong ground height, pending/stale readiness, active speech/incomplete flags, crouch and stale input/callback cases reject without mutating the live session. Actual triangle support ID/surface replaces hard-coded summit identity. |
| Traversal regression | 2/2 rescue/imported-terrain journey suites pass. [Run](forest-endpoint-traversal-tests.log), [details](forest-endpoint-traversal-tests-detail.log). |
| Android Java | All 372 tests / 51 suites pass, zero skipped. New endpoint catalogue admission/label and paused-load presentation-barrier checks included. [Build/test log](forest-endpoint-java.log). |
| Builds | Windows Release application and all four Android Debug ABIs build. APK SHA c7a0b2fb2a445a044ad5b56604e98761b0b5016e71dd69cbb483dc6961b50b9c, 186149109 bytes. No installation or application launch. [Windows](forest-endpoint-native-build-2.log), [Android](forest-endpoint-android-build.log). |

Initial native compilation caught incorrect `.x/.y` indexing of a test's
`std::array<float,3>` point. The correction uses `[0]/[1]`; the
[failed build](forest-endpoint-native-build.log) remains preserved. No production
assertion was weakened. Independent bounded production review found no further
actionable defect. This is a selected host lane, not a full repository pass;
the previously recorded unrelated inventory/source-pin failures remain separate.

## Milestone map and next acceptance

- Implemented systems awaiting broader acceptance: combat teaching, reward/night,
  rope out/back, saves/death Load, movement/crouch/root stepping, captions/VO,
  native controls and pause policy. Forest endpoint is now implemented locally;
  its new exact APK has no physical acceptance.
- Missing player-facing integration remains: the approved optional player-opened
  lookout objective/Continue exploring/Return to menu panel; fuller Kit speaking/
  lantern reaction acting; deliberate opening composition and small floor debris.
  Regional geography still needs approved-map comparison; playable town is 1.8.
- Physical gates remain distinct: natural earned checkpoints/death Load and full
  traversal, visible target/contact and first-time teaching, controller/comfort,
  current-candidate RT scene/Kit placement, audio and sustained warm performance.
  No new heavy outdoor asset or performance loop was started.

The bounded next acceptance is an isolated exact-APK Forest lookout load through
the existing resource/input/presentation barrier, inspection of grounded player/
Kit and completed captions, explicit Resume, and return-side persistence. The
next small missing host-side feature is the already-approved player-opened
lookout objective panel. Neither requires inventing another broad branch.

The corrected earlier graphics APK remains installed on SM-S948B. The owner's
latest explicit **Mist Off** is saved there; all other preferences, 12 campaign
files/locks and original rotation/font/density custody are preserved in the
[separate phone receipt](../2026-10-20-graphics-followup/README.md). No new forest
save was written to that phone. Controller hardware was absent.

Audio/haptic manual revalidation required: **YES, bounded to the new restore
destination**. Listen for absent completed VO replay and appropriate resumed
lookout ambience after load. No audio asset/gain/backend or combat haptic change.
No listening acceptance is inferred from host tests.

Banked optimization remains the exact-zero-mist exit (about 18% lower GPU time in
the cleared tomb); it is not a Keeper two-light or sustained 30 FPS pass. Rejected
quality trials remain rejected; the renewed Keeper arm attempt had zero timed
frames and remains inconclusive. No Keeper preparation/arming/timing or the
previously denied blind Resume, turn and Swing action was retried. No public push,
merge, release, owner process termination, heavy asset edit, spending or Reef
operation occurred. Work stays in the existing WP2 worktree and draft PR27.
