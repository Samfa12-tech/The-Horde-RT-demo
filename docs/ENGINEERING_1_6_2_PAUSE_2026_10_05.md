# Horde 1.6.2 pause checkpoint - 5 October 2026

The owner has changed the priority: tighten the candidate after the weekly reset
and further review. Merge, production signing, tagging and publication are on
hold. No additional optimization experiment or long run starts during this pause.
The existing draft [PR18](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/18)
and branch `codex/horde-1.6.2-engine-readiness` retain the work.

## Exact completed boundary

Runtime source is `1334cc9c58ec97940ac10d861f0397143ca7a9f4`, based on released
main `1df058b77baacaab76dc112f578deb77ddb791e9`. Its
[CI run37271761916](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37271761916)
passes all six jobs. Later documentation backups do not relabel these artifacts;
their newly triggered CI status is separate.

- Windows Debug and Release builds, three all-four-ABI Android packages,
  12 affected native CTests, three renderer/player integration checks, both
  Release shader fixtures, 154 Java tests in 24 classes and lint 0 errors/49 warnings pass.
- The pre/post-build source seal covers 5,823 tracked files/3,652,319,441 bytes.
  Windows 85-member packages, Android 16KiB/notices/compiler policy, actual PE/ARM64
  SDK modules and Shipping zero atomics/no binding22 pass. Release remains unsigned.
- Both Windows backends pass real Keeper Mist On/Off/restored-On 13-pose banks
  and 14-image/10-transaction previews: 78 Showcase plus 28 preview images,
  eight successful synchronization-enabled launches with no error markers.
  All 26 On and 26 restored-On RGBA views match prior 646 exactly. Owner accepts
  optional Off still appearance, noting little visible difference in these views.
- Both SM-S948B/Android16 Debug backends pass 13 route points, seven captures
  and strict same-process Home/resume at the owner's saved 50%. Actual installed
  APK pullbacks match. Original 1,420-byte saved preferences are byte-identical.
- The isolated Shipping validation app passes staged reset 50%/Mobile water/fire/
  GlassOff/Current shadows/MistOn/cap 30 through a current real RT frame,
  explicit Restore, MistOff Home rollback and confirmed Off/On cold-restart
  persistence. Its original confirmed 75%/Mobile/GlassOn/Current/MistOn/cap 30
  tuple is restored. No owner Debug setting is written; no app data is cleared.

Both validation apps are stopped. No local build, application, GPU or device operation
is in flight. Unrelated dirty work and frozen 1.6.1 artifacts remain untouched.
Raw reports, screenshots, preferences and private identifiers stay outside Git.

## Phone glass/apply timing issue

A glass change recreates the admitted RT scene. In the observed phone check,
recreation took long enough that the initial 8x500ms first-ACK observer missed
completion. The ordinary 15-second confirmation timer starts only after exact
native acknowledgement; it was not extended or weakened. A later bounded 120s
observer verified actual 50%/GlassOff/MistOn and successful Restore. There is no
concrete new native ACK/lifetime defect established by the read-only review.
This does not close the product concern about slow glass application. Before
changing code, distinguish driver pipeline creation, scene/resource rebuild,
UI accessibility churn and actual ACK timing with bounded evidence.

Owned-UI traversal and stale-target failures, a tap that did not stage Off,
private wrapper argument-name/menu-case errors and a Git working-directory
preflight failure remain preserved negatives. Subsequent checks verified actual
requested/effective values and resumed from actual state. None is relabelled
as an earlier pass, and no renderer authority check was weakened.

## Artifacts and evidence

The [candidate artifact table](ENGINEERING_1_6_2_REVIEW_CANDIDATE.md#final-graphics-artifacts-and-evidence-5-october)
contains all seven exact artifact hashes. Primary Debug APK:
`cea6f9594696a7d97df85c2d6cbe7782b82f5a9c561f5a43bc93aa194bf1a11e`.
Separate development-signed Shipping validation APK:
`dfb36907def5444327be2acfc6ee873db16594160a0f15d093b4ade7b02229db`.

| Closed receipt | SHA256 |
| --- | --- |
| Windows capture admission | `c64c0156c230a40376a6be8eb121cceececef8276f7daafac49466442c93a777` |
| Phone replay/capture/lifecycle | `76211a9987b5235fb4cf323eaa43f46e5c9c9209d4f6e7a8845f4fd947ed2237` |
| Phone settings/rollback/persistence | `ff03e377b5185ecb0329df652c34c0357df344e70a0a9eb5d151b20539413a2d` |

The [final graphics record](ENGINEERING_1_6_2_FINAL_GRAPHICS.md) and
[execution ledger](ENGINEERING_1_6_2_SECOND_PASS.md) retain exact scope/limits.
Desktop/high and ordinary mobile mirrors remain: the closed stone comparison
found no useful repeatable benefit. Current indirect transport remains; Lower,
FSR/vendor upscalers, LOD/detail normals and parallax retain their deferrals.

## Outstanding validation and resume plan

The affected final-preset correctness and Mist persistence checks above are
complete. Fresh-install key absence is CPU/Java evidence; the existing phone was
not cleared for a fresh-install test. No matched sustained cost/30FPS claim is
established for the final 50%/GlassOff/MistOn tuple or MistOff. The significant
Keeper-flank cost, quality-choice cost, early reward overlap/four simultaneous
real lights, Windows changed-audio listening and report clipboard/picker/manual
interaction, final independent review and other-device dispositions remain
open at their recorded scopes. S24 is deferred; S25 is unverified. Older-driver
WSI shutdown proof and separate failed-initialization cleanup limits remain.

1. After the reset and further owner direction, read this checkpoint, the
   candidate record and durable local checkpoint first. Refresh the pushed
   branch/CI state and review the immutable 1334 candidate without replaying
   completed checks merely because the thread resumed.
2. Agree the bounded tightening scope. Investigate glass-apply latency first
   if selected; establish the actual slow stage before any reusable fix. Do not
   reopen rejected optimization experiments without materially new evidence.
3. Validate only affected changes and remaining owner/device gates. Preserve
   saved settings and exact artifact identities; a runtime edit requires new
   packages/evidence. Phone allocation must be confirmed for future use.
4. Perform final independent review before any release action. Merging,
   production signing, tagging and publication require later authorization
   and owner-only signing/backup safeguards.
