# Android death-screen Load entrypoint

The death card now offers Load checkpoint through the existing three-slot
campaign menu. Manual and latest-autosave records remain distinct; this adds no
storage format, slot overwrite policy, owner-data migration or automatic write.

A death-origin latch keeps the original recovery screen authoritative while
the chooser is open. Native Alive publication and Home/pause cannot clear it
before the load's camera/input and RT-presentation handoff. Back without a load,
matching precommit cancellation and failed Load return to the original death
card, with the failure message visible. A cancellation requested by Home keeps
its accepted serial until the matching owner outcome can return that card.

After logical commit, Back cannot restore the superseded dead simulation. It
keeps the paused Load result while the existing rebase/presentation obligation
finishes. Only explicit Resume, after current matching Load success/presentation
and native Alive with positive vitality, clears the death overlay. Resume again
consumes held controls and stale feedback and publishes the restored absolute
camera with neutral input before unpausing. No dead gameplay is resumed.

Retry and Restart retain their existing owner-Alive acknowledgement policy after
returning to the death card. Detached old Retry/Restart buttons cannot mutate an
in-flight death Load. Accepted Retry/Restart revoke the death-origin latch so
their existing recovery path can complete normally.

## Host reproduction and checks

The new real-Activity test first ran against unchanged production Java and failed
because the death card had no Load checkpoint action. That one-test failure,
its XML and original MainActivity hash remain under
`reports/2026-10-18-android-death-load/`.

The final affected host selection passes **82/82** tests in 26 seconds: decoder
7, real Activity fake-JNI campaign integration 26, death restart recovery 1,
controller integration 35, surface lifecycle 7 and touch cancellation 6.
It covers death-origin manual/auto navigation, precommit Back/cancel/failure,
native Alive arriving before handoff, Home both before and after commit, rejected
dead-player Resume, successful alive Resume, and detached/original Retry/Restart
behavior. Existing cancellation, exact-token/surface, stale-publication and
neutral-input checks remain in the same selection. The intermediate 79-test
selection also passed before the final lifecycle cases were added.

The exact command, final source hashes, baseline identity and copied XML hashes
are in [receipt.json](receipt.json). Tests execute production Activity methods
with immutable fake native publications; they perform no real save IO or Vulkan
rendering. This is host implementation evidence, not physical death recovery.

The Java implementation/test phase performed no phone action, Android assemble/
native build, Windows/CMake source change, owner save access, chapter-earned hook,
push or release. The later authorized build-only follow-up is recorded below.
Current-build physical death/Load/Back, postcommit Home, controller/touch focus,
actual RT handoff, camera/audio and alive Resume remain acceptance requirements.


## Android build-only follow-up

Current Debug and unsigned Release APKs assemble for arm64-v8a, armeabi-v7a,
x86 and x86_64, including the death-Load Java and extracted common load-barrier
alias. Fifteen selected affected input hashes are unchanged between build start
and completion; [android-build.json](android-build.json) records those exact
sources, artifact hashes/ABI contents, commands and log hashes. The working HEAD
was `0a9d7588` with the recorded local implementation changes.

The first combined offline run completed Debug but failed Release ARMv7 when
Ninja could not create a 321-character object path. That failure remains in the
original log. Reconfiguring only the existing ARMv7 Release build cache with
`CMAKE_OBJECT_PATH_MAX=260` produced a 249-character hashed object path. No source,
SDK/toolchain or signing configuration changed. Release then completed all four
ABIs in 2m39s. `HORDE_VALIDATION_UNSIGNED=1` bypassed release credential reads;
Debug retained its ordinary developer signing and Release is unsigned validation.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Debug | 183958042 | `e9ce289a8a527083aa1d6d2ca2976da5ba439768f8cc933b3a34e20f153701c8` |
| Unsigned Release | 169333341 | `3bc695b1a6c15252a0cac83a3a77e5ff596c6a504105796cbcd61d8810438749` |

APKs remain in their normal build-output locations; no large artifact was copied.
No app was installed, launched or foregrounded and no device was accessed. This
is compilation/package evidence. Earlier phone acceptance belongs to its original
APK/source and is not reassigned to these builds. Physical death-Load acceptance,
full-route, power-loss, audio/controller and performance gates remain separate.
