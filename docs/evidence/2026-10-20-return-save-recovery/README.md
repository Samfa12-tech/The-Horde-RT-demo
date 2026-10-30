# Actual lower-return save recovery and repeat ascent

Fixed a demonstrated restore mismatch: loading a completed-forest lower-return
save re-armed the first-exterior latch, so repeat ascent silently lowered the
saved raised lantern. The accepted [equipment/traversal plan](../../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md)
requires restoration of the prior valid held configuration after traversal.
`CommitCampaignRestore` now recognises validated completed-forest revision-two
records as having already visited the exterior. Revision-one lower saves retain
the existing lowered-lantern first-reunion lesson. No format, health, healing,
charge, entitlement or story-completion policy changed.

Existing folder: `C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`.
Branch `codex/horde-1.7-wp2-vertical-proof`, draft PR 27 direction, local work only.
Base `966cd8e456fb17d975169d94afdab8cf038af53f`.

## Actual source and bounded method

The source is the **actual lower-return autosave produced by the prior R3 run**:
`reports/horde-campaign-fixtures-route-20261020-r3/slot-1-auto.hrd`, 162 bytes,
SHA-256 `031ced0a83a136861833ab4e9930ae6ea28c7c6e86d47c61c80ba24e8778472b`.
It is schema one/content two, LowerClaimed, two hearts, sword held, lantern High,
first ascent completed, completed forest and no pending lantern lesson.

The launcher verifies it against the committed prior receipt, then copies its
bytes into a new isolated working auto and an immutable manual witness. It does
not decode/re-encode or manufacture completion flags. Native restoration reads
the working auto through the normal worker, native Load controls, input rebase
and successful hardware-RT presentation barrier. Only ordinary context input
starts repeat ascent. New upper autosave/reload uses the current production
rules. No authoritative player pose, combat, completion injection or new reward.

**The original journey began from an explicit post-Keeper seed.** Natural guard/
Keeper victory, chest claiming and earlier speech remain unverified. Native
computer interaction was unavailable; this is scripted hidden HWND/shared-60-Hz/
BLAS-TLAS hardware-RT continuation, not a physical playthrough. The driver advances
four fixed ticks per rendered frame; this is not performance or listening proof.

## Retained failure, correction and acceptance

- **R1 failed honestly:** seven checks, one failure, 164 frames / 16.485 seconds.
  Lower recovery correctly restored High and two hearts. The real stow/ascent/
  pull-up/release completed, but the first safe exterior tick changed High to
  Low. The complete route result remains rejected. A separate CPU continuation
  from those same copied bytes reproduced two failed assertions out of ten.
- **R2 passes after the correction:** ten native checks, 226 frames / 35 seconds.
  All three original milestone PNGs were inspected: restored lower side,
  repeat-ascent upper release, and recovered upper auto. Completed evidence
  matches current simulation tick/epoch and successful RT swapchain presentation.
  This supplies functional held-state evidence, not full art/primary-arm admission.
- **Final R3 passes after admission hardening:** ten checks, 226 frames / 31.734
  seconds. Review found that direct native/CPU entry points had trusted caller
  provenance while the launcher checked the hash. Both entry points now compare
  every byte against the recorded source identity before restoring anything.
  The pin is admission-only and never loads or grants state. All three final
  PNGs are byte-identical to the inspected R2 images; [comparison](final-vs-runtime-captures.json)
  and fresh completed RT identities are retained.

| Acceptance item | Result |
| --- | --- |
| Actual lower auto → native Load → explicit Resume | Pass: fresh generation/input barrier, grounded safe side, modern rules, original two hearts and held configuration. |
| Story/Kit persistence | Pass: proof and all forest flags retained; no active speech, new lesson or completion grants; Kit remains grounded at its completed final waypoint. Its distant wait enum is allowed to change normally. |
| Repeat ascent | Pass: current Climb prompt/context edge, observed stow/ascent/pull-up, grounded upper release, one reward/rope deployment, original sword and High lantern retained. |
| Upper auto and native recovery | Pass: actual SafeExit content-two auto, canonical upper anchor/yaw/pitch, same health/equipment/story, then worker Load/fresh RT/Resume. |
| Quiet continuation | Pass: 120 ticks before ascent and after final reload without repeated save, healing or speech restart. |
| Natural victory/claim, visible death, real controls/lifecycle/listening | Still pending. No combat/death/phone/public action was attempted. |

The new upper record is 162 bytes with SHA-256
`7e03e25e7de99214a3229c5ab3b415104e273dd3a67502504820156010bf781f`.
The original R3 source and manual witness remain byte-identical. No other slot
record was created. [Route summary](route-summary.json) records decoded CRC-checked
source/upper facts and CSV-derived state; [final native receipt](r3-windows-route-validation.json)
retains exact checks and RT identities. Rejected R1 and runtime-pass R2 remain
beside it. Original CSVs/images/save bytes remain in the three isolated `reports/horde-campaign-fixtures-route-return-20261020-r*`
folders, identified by [raw hashes](raw-evidence-sha256.json), without committing
large images or duplicate runtime binaries.

## Checks and exact-build limits

- The dedicated actual-record CPU check changes from **10 checks/two failures**
  before to **10/zero** after, including the final exact-byte admission build.
  It reads the actual copied record and admits real
  imported bare terrain for CPU continuation; it cannot establish GPU readiness.
- Debug chapter dialogue and campaign checkpoint CTests pass (including 1,026
  checkpoint checks). The full Debug campaign CTest **times out at its unchanged
  30-second gate** on the runtime-corrected build before admission-only hardening.
  One later isolated diagnostic run of that same executable
  completes all **145 checks/zero failures in 39.609 seconds**. That is a behavior
  pass outside CTest, **not a pass of its time gate**. No contention cause or
  regression origin is asserted; no extra loop was added to its normal argc-one
  path. The original timeout is retained in [affected-host-final.log](affected-host-final.log).
- The final admission build's **Release campaign CTest passes in 4.34 seconds**, under
  its unchanged gate. Debug/Release results stay distinct. No timeout or assertion
  was weakened. Do not rerun these passing paths merely to accumulate evidence.
- Windows Debug/Release and offline Android Shipping/Mobile Debug validation
  builds pass; all four ABI lanes rebuilt. No device install or release signing.
- Six invalid-mode/root/reuse/manifest/copy cases reject with exit two, before a
  window; every existing fixture byte stays unchanged. See [guards](guard-tests.json).
- Two additional final-build guards reject matching, valid-CRC completed lower
  records with a different held pose at both native and CPU entry points. The
  unchanged claimed hash and matching copies cannot substitute another source;
  [exact-copy guards](exact-copy-guard.json) retain the rejected record identity.
- All three owned native processes exited. Final R3 retained 534 foreground samples; its
  owner was never shown or observed foreground. The foreground API returned null,
  so no particular external window is certified. The launcher never kills or
  automatically retries a pending game process.
- Custody records preserve owner INI, both canonical dirty files, original R3
  source and absence of the real LocalAppData campaign directory. Owner untracked
  assets remain untouched. No phone, Reef, public push or release operation.

[Source freeze](source-freeze.json) pins the built raw working bytes; Git may
normalise source line endings. [Artifacts](artifacts.json) pins Debug EXE
`110514db24cd2d5b6141755033840cf86d4de26f8e83a58e48a8f506be54f77f`,
Release EXE and four-ABI APK. No EXE/APK/save duplicate is committed.

Reproduce only after checking for a conflicting owner process, using a new suffix:
`python tools/validate-windows-route.py <new-suffix> --recover-lower-return`.
The original R3 file must still match its committed receipt. CPU reproduction:
`horde_rt_campaign_play_session_tests.exe --check-actual-return-recovery <isolated-fixture-directory>`.

Timestamp clarification: Windows logs use AUS Eastern daylight time (UTC+11) and
show 21 October; these runs occurred on **20 October UTC**. The preceding route
README's sentence saying 21 October UTC was incorrect. Original receipts remain
unchanged.

## Meaningful remaining 1.7 work

| Requires implementation/content choice | Existing implementation still awaiting acceptance |
| --- | --- |
| First recovery feature: six owner policy choices, then a bounded implementation. No healing API, flask/refill state or charge save fields exist. | Natural victory/claim/full route, known-state visible death Load, physical controls and saved-game/lifecycle interruption. |
| Selected foliage wind/distance LOD and planned participating effects need final scope/budget disposition; retain rejected pine LOD2 and mobile Mist Off. | Sustained warm Keeper/two lights/exterior pacing, combined workload and exact-candidate platform matrix. |
| Suitable grit/stone creak material, stationary Kit pivots and final opening/outdoor composition remain unresolved content/quality work. | Actual Windows/Android listening, output cleanup, haptics, final art, fonts/DPI/orientation/accessibility/controller coverage. |
| No new system is required by this checkpoint. | Debug campaign suite's existing 30-second timing gate remains unresolved. |

The tested post-Keeper route/recovery cases are now parked until a relevant
change or failure. The highest-impact implementation choice remains the
[six first-recovery decisions](../../design/HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md):
method/availability, capacity/amount, refill/rest, use/interruption, death/retry,
and migration/entitlement. Owner acceptance or one of the bounded existing content
gaps is a useful next milestone; another identical passing route run is not.
