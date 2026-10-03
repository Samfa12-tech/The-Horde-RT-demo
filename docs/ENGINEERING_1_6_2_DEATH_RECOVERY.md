# Android death recovery correction

An idle validation run died during combat. A fresh process entered at vitality3 after2seconds and showed a skeleton/death screen after30seconds; this is not an immediate fresh-launch death finding. Those idle runs are rejected as matched performance workloads.

Restart Route had a separate, confirmed ordering bug, also present in the released baseline: Java published reset then resume; Android's pause synchronization acknowledged the reset without applying it, so the owner saw no pending reset and retained vitality0/Dead.

Ordinary menu synchronization now retains explicit reset/retry while discarding combat, interaction and held-pose edges. Genuine accepted surface stop/start retains the default discard-all barrier. A separate discard floor prevents a later menu publication from overwriting a still-unobserved lifecycle barrier. Final world-command admission shares the publisher mutex with accepted stop/start, checks current surface identity and executes at most128 zero-delta commands. Remaining commands defer every unlocked simulation branch to the next frame. Normal ticks and GPU work run outside that lock; JNI never mutates simulation directly.

The death menu now waits for the existing native Alive acknowledgment before resuming. It does not manufacture full health or accept repeated restart taps while recovery is pending.

Validation before device rebuild:

- Real combat-dead simulation plus coherent mailbox regression covers reset then resume before owner consumption, exactly-once application, retry while paused, stale competing edges, stop overriding reset, newer reset after stop and coalesced reset/retry priority. Gameplay and unchanged lantern lifecycle-discard fixtures pass.
- Independent source review found and closed a stop/admission race before freezing the native change. No publisher relock, reverse lock order or driver work under the admission lock was found.
- The production Java restart method is exercised under Robolectric: one reset request, retained paused/dead UI, no optimistic vitality/unpause and repeated-tap suppression. Focused test passes.
- Character source contracts pass after updating the synchronization/deferred-replay witnesses. An unrelated stale Windows resize failure witness now requires the actual typed ResourceFailure and failed acknowledgment. Initial failures are retained; assertions were not weakened.

Fresh APK admission and physical death/restart/retry/Home-resume validation remain required. Earlier APK, presentation, performance and owner appearance evidence retains its exact historical identity; it does not certify these new native bytes. Original validation settings75/Mobile/Mobile/GlassOn/30 were restored and visibly confirmed before stopping the validation process. Production data was untouched.
