# Chapter clarity checkpoint — 30 October 2026 (AEDT)

This bounded source change follows Sam's chapter playtest; it is not a fresh
Windows/phone gameplay pass. The phone was released before this work.

## Confirmed fixes

- Android's controller footer repeated generic combat instructions forever,
  independently of the timed teaching owner. It now displays contextual actions
  only. Combat teaching, its Settings controls and protected practice remain.
- Kit's reunion already required a fresh manual, visibly presented lantern
  raise. The shared snapshot now exposes a reminder only while that proof is
  pending, the player is at the upper safe phase and the reward lantern is low.
  Android publishes this through the existing contextual state, retaining both
  the lantern and return-rope actions at the landing. Touch says to tap the icon,
  controller says Y, and Android mouse/keyboard says to select the icon (Android
  has no E binding). On touch the reminder clears the visible action buttons
  vertically using their current sizes/insets. Windows uses the existing active-input glyph selector for
  E/Y. The reminder survives the spoken line ending, clears on raising/proof,
  and is hidden through existing pause/menu/death/capture gates. It grants no
  quest progress or input itself.
- Controls help no longer describes the superseded half-damage guard rule:
  normal attacks fully block, yellow breaks held guard but permits parry/dodge,
  red requires avoidance. The accepted guard/attack/return animation is unchanged.

## Findings that remain open

- Missing rings are an admission gap: `RtAttackWarningOptions.enabled` and
  `SimulationConfig::specialSkeletonAttacks1_7` still default OFF. Their source
  assets, shared owner and adapter exist, but normal platform callers do not
  enable the in-world cue. This patch does not enable unreadable yellow attacks
  or call text a replacement for the ring. Next: coordinated isolated actual RT
  ring/attacker timeline, cancellation, dark/light, both-aspect and lifecycle
  review; then admit the cue and matching special attacks together.
- A protected practice Ready prompt deliberately persists until explicit
  Finish/Skip at a safe boundary. That differs from the removed generic footer;
  no automatic loss of protection was introduced to hide teaching text.
- Kit dialogue uses route proximity and an ordered continuation queue. The last
  `ForestVillage` line is "There. Chimney smoke." near route point7/F04
  (13.3,36.2,-1.15 in world X/Z/Y). It records a dialogue flag, not an explicit
  demo completion. Do not claim an enforced endpoint or natural pacing pass.
  Smallest proposal: an earned, dismissible chapter-complete card at the existing
  endpoint, with Continue exploring / Return to menu, preserving the same terrain
  and saves; agree that behavior before expanding it into another chapter.
- Fresh Windows menu→Play, journey/save-reload and visual placement remain
  unrun in this block pending a coordinated foreground window with Reef.
  Parent coordination through `send_message_to_thread` returned "thread not
  found"; no application launch/approval retry was attempted.

## Local supplied voice inventory (read-only)

Verified 29 Salvager +19 Relic Dealer lines, unique stable IDs and exact spoken
text against the recorded English bank at `d37f16381311c6e620f0c4a9d01c1cce83260dc8`.
All 96 MP3/WAV hashes match their manifests; all48 MP3s decode and match the WAV
PCM byte-for-byte (24kHz mono PCM16). Originals were not altered or imported.
Voices are Wise Animation Mentor and Versatile Audiobook Actor.

Twenty-seven lines remain `REVIEW_DESIGN_DEPENDENCY` (20/7); the other21 remain
editorial candidates. Casting approval is not complete word/listening/runtime
acceptance. Manifest durations exceed actual decoded content by48.667–64.667ms;
derive corrected runtime durations during intake rather than modifying originals.
The canonical bank is retained in the historical commit, not a current runtime
catalogue. Next integration step: editorial/listening reconciliation of the21
non-design-dependent candidates, source/licence intake, then stable-ID runtime
binding only where the corresponding approved actor/dialogue exists. No new NPC
behavior or provisional design was admitted by receiving these files.

## Validation

- Android focused Robolectric:44 controller integration +16 teaching preferences
  +6 teaching UI scheduling tests, all pass. Includes both orientation values,
  input-specific objective wording, rope coexistence, clear-on-close and no
  accidental action dispatch. This is UI policy evidence, not physical layout.
- Native shared `horde_rt_chapter_dialogue_tests`: pass, including the objective
  surviving subtitle end and rejecting rope/lower/raised/completed/wrong-item
  states, plus existing fresh-visible-raise/save/dialogue contracts.
- Windows Release application build: pass; executable not launched.
- Final Android `assembleDebug` and `lintDebug`: pass across arm64-v8a,
  armeabi-v7a, x86 and x86_64; lint0 errors/92 warnings. Final focused tests above
  were rerun after the touch placement adjustment and pass. Exact local package
  identity is recorded in `reports/chapter-clarity-20261030`. No device installation,
  live report submission, security change, merge or publication.

Raw audio validation details are local in
`reports/chapter-clarity-20261030/voice-inventory.json`; no audio bytes or private
owner save material are included in this evidence receipt.
