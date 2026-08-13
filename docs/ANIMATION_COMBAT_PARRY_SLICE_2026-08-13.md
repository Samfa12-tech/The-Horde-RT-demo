# Animation-Owned Combat and Timed Parry - 2026-08-13

## Scope

This development milestone replaces loose combat timers with fixed-step action phases and adds one phone-oriented skeleton parry. It preserves the published Alpha 0.1.3 packages and does not add a held guard, automatic counterattack, bonus damage, dodge, third enemy, new asset, or lich parry.

## Gameplay authority

`SwordCombat` now publishes bounded `PlayerCombatAction`, `EnemyCombatAction`, `CombatReaction`, and `PlayerCombatSnapshot` values. Action time is phase-local.

| Action | Wind-up/startup | Active | Recovery |
|---|---:|---:|---:|
| Player swing | 0.18 s | 0.16 s | 0.22 s |
| Skeleton strike | 1.12 s | 0.18 s | 1.50 s |
| Failed parry | 0.04 s | 0.22 s | 0.24 s |

Sword and skeleton contacts each resolve once as the action enters its active phase. The player contact uses the existing 1.72 m range and 0.52 frontal-cone dot threshold, selects only the nearest valid skeleton, and now gates lich hits as well. The lich retains three health, its two-second accepted-hit lockout, recoil, cries, death animation, roof opening, dawn reveal, and finale.

Parry uses its own monotonic input command. A skeleton strike succeeds as a parry only on the real active-window damage attempt while in normal hit range and inside the player's frontal cone. Success suppresses damage, emits one ordered `PlayerParrySucceeded` event from Player to the stable skeleton ID, and places that attacker in an 800 ms stationary stagger. The staggered entity retains the attack token. Successful recovery ends on the next fixed tick; the resulting riposte is an ordinary swing with no target assistance or damage bonus. Early, late, rear-facing, unavailable, spammed, and death-state commands do not reduce damage or buffer an action. Lich staff damage is unchanged and not parryable.

## Platform and feedback

- Android publishes `requestParry()` through the existing coherent JNI mailbox and adds a 104 x 72 dp `PARRY` button beside `SWING`. Both controls autosize their single-line labels and share the existing HUD/menu/death/automation visibility rules.
- Windows publishes the same independent command on non-repeating `Q` input.
- Success reuses the existing `sword_hit_2` metal-impact cue. Android adds a distinct 34 ms amplitude-235 vibration, with platform fallbacks when amplitude control is unavailable.
- Developer state exposes the last consumed parry sequence plus readable player and token-holder action phases.

## Renderer

The existing sword and right arm move across the view during parry; success adds a small bounded procedural jolt. Skeleton attack phases map to their existing Attack clip offsets, while stagger freezes at the contact pose and adds bounded whole-instance lean/recoil.

The former parallel `CharacterRenderPlan` authority was removed. One cached `CharacterFramePlan` now drives both CPU skin/refit and TLAS routing. Matching skeleton poses still share one BLAS and divergent poses still use no more than two buckets. The renderer remains at nine BLAS and nineteen physical TLAS slots; the lich route remains singular and masked slots remain valid.

## Host validation

- Windows Debug incremental build and all 12 CTests: pass.
- Windows Release incremental build and all 12 CTests: pass.
- Android final `assembleDebug lintDebug`, including arm64-v8a, armeabi-v7a, x86, and x86_64: pass.
- Embedded raygen SPIR-V staleness check: pass; shader and embedded words are unchanged.
- Fresh Host foundation run: `reports/foundation-runs/run-20260813-204944`, pass. It includes clean Debug/Release Windows builds and tests, clean Android Debug/Release builds and release lint, safety gates, packaging/licence checks, and evidence hashes.
- Deterministic comparison against `run-20260811-223230/captures/windows-idle-rerun`: all 13 captures pixel-identical. Matched median timing was 6.05105 ms baseline versus 6.0618 ms candidate, a 0.178% change.
- A final isolated post-cleanup capture repeated the pixel-identical 13/13 result at 6.0481 ms matched median, 0.049% below the same baseline. An intervening capture made concurrently with the Android compiler was rejected for timing use rather than misreported as a renderer regression.

These results prove deterministic simulation, renderer contracts, platform compilation, and unchanged deterministic imagery. They do not prove phone thermals, physical button reach, parry forgiveness, perceived telegraph/contact, clang/haptic distinction, or combat feel.

## Device gate

Pending exact-candidate `SM-S948B` validation. The required run is the standard six-checkpoint 75% gate below 20 ms at thermal status 0-3, with 100% reported separately, all 13 captures, replay, and Home/resume. The phone must remain awake/unlocked during automation. Hands-on parry feel remains an owner judgment after the exact APK is installed.
