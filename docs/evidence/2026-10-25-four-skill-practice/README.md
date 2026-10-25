# Four-skill practice and shared attack authority

Later exact-head CI found an eligibility/intent regression in the motion
retry/reward fixture. Its [narrow correction and retained failure](../2026-10-25-practice-eligibility-fix/README.md)
separate an inactive rule profile from a human Skip, without changing the scenario
or its gates. This local checkpoint's five passing tests were not full CI.

Base `51af5c26e14700574b2217d36324c547a6b5d9d4` passed all six hosted checks
in [40193192999](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40193192999).
This checkpoint implements the approved rule/teaching slice independently of the
still-unapproved in-world ring capacity. It does not implement the revival bridge.

## Shared rules and current defaults

An eligible held frontal guard now blocks an actual ordinary skeleton contact
fully. Rear hits, attacks during player attack/recovery, and Keeper red attacks
retain full damage. Timed parry, dodge protection, attack recovery and the accepted
guard-to-attack-to-held-guard animation retain their existing timing.

`EnemyAttackKind` is latched when an enemy windup actually starts. Its contact
pulse carries the same category, independent of a later attacker-token handoff.
The isolated special policy selects every third attempt per actor as GuardBreak;
it never creates a red skeleton attack. Yellow remains timed-parryable/dodgeable,
but breaks a held guard, requires physical release before another guard/parry,
and otherwise applies the existing full hit. No new stamina/stun system is added.

**New yellow attacks remain OFF in normal gameplay.** The explicit
`specialSkeletonAttacks1_7` configuration is only used in focused validation until
the enemy-anchored RT warning passes its approval/visual/cost gates. Ordinary
attacks are neutral rather than all being labelled yellow. Keeper red authority
is unchanged. Existing accessible text is not an admitted substitute for a ring.
No shader, ABI, material/primitive/owner capacity or quality default changed.

## Practice outcomes and exit ownership

The shared 60 Hz owner now records four real outcomes, accepting valid successes
out of order while guiding Block -> Parry -> Dodge -> Attack from Guard:

- eligible normal contact actually blocked before practice damage suppression;
- existing real timed-parry success;
- existing actual timed dodge-protected contact;
- a guard-origin accepted attack that hits, completes its legitimate recovery
  and returns to still-held guard. Whiff, release, interruption or a different
  attack identity cannot manufacture that last result.

All four outcomes reach **Ready**, still protected. The existing menu command
becomes **Finish practice** there and **Skip practice** earlier. An exit request
keeps protection through an existing windup/contact and commits only at a safe
attack boundary; pause cannot acknowledge it. Turning lessons off uses that same
boundary. No new Interact precedence or input route is introduced.

**Practice: no damage** remains visible alongside the current skill and current
attack warning. Actual parried-stagger success takes precedence over the next
skill; an ordinary Hit stagger still cannot claim a parry. Protection captions
clear immediately when protection ends, rather than fading stale immunity text.

If both guards fall before the lessons, the prompt offers explicit **Replay
opening practice** or Skip. It neither invents skills nor automatically revives
enemies. Replay retains its existing opening reset route. Ordinary Retry retains
genuinely learned skills. The existing standalone Keeper workshop remains a dodge
workshop, without granting the other three skills. Confirmed earned-save restore
ends practice immediately with no fabricated learned flags, including its paused
first frame; save schema, slot ownership and record semantics are unchanged.

## Checks and inspected output

Five affected native CTests pass: combat foundation (853 assertions, zero
failures), campaign checkpoints, combat timing, shared simulation gameplay and
Windows practice launch/prompt. The final additional actual earned-restore
assertion passes in the campaign target. Its existing 60-second deadline and all
other test deadlines are unchanged. Both skeleton survivor orders and the room
edge handoff remain covered.

The full Android suite passes **490 tests, zero failures/errors, six existing
skips**. Four-ABI Debug assembly and lint pass: zero errors, the same 83 warnings.
An introduced unused Skip resource warning was corrected by selecting the actual
Skip/Finish resource, without changing the words or native behavior.

Eight actual host Android widget cases cover 320x568/640x320 at 100/200% font.
The [portrait](host-portrait-font200.png) and
[landscape](host-landscape-font200.png) images are **widget fixtures on a black
background, not phone captures, RT combat or completed in-world warnings**.
Text is complete, noninteractive and within the viewport, without ellipsis.
The initial width assertion included a trailing wrap space: full advance
229.58203 px, visible extent 225.58203 px and glyph ink 226 px inside 226 px.
The corrected test verifies excluded characters are whitespace and retains the
same font, full text, viewport, ellipsis and rounding checks. This follows the
[Android Layout extent contract](https://developer.android.com/reference/android/text/Layout#getLineMax(int));
no runtime font/layout or acceptance tolerance was reduced to get a pass.

The existing hidden actual RT guard proof completes **202 presented/completed
frames**. All 202 pose records and all 60 PNG hashes match its retained control;
one fresh parry, full attack recovery, held return and release are preserved.
Representative attack/return frames were inspected. This is fixed-input,
simulation-timed **unpaced** animation evidence, not natural combat, damage,
measured FPS, human pacing, Windows foreground/DPI or phone acceptance.

Original 815-check/34-failure old-policy expectations, the actual success-cue
correction, both width failures, sandbox cache-lock failure and raw results are
retained under ignored `reports/combat-practice-20261025/`. No gates were deleted.
The central approved-cache route succeeded without cache/permission changes.

## Artifact, custody and remaining work

The preserved final Debug APK is `Horde-1.7-four-skill-practice-r5-debug.apk`,
206,293,691 bytes, SHA256
`84df18dd59e7b89bab38d4c1c123231b924aa09d40c8fd5482dd271cb5e22e32`.
Package `com.samfa12.hordelanternrt.debug`, code 10/name `1.6.2-debug`, four ABIs.
All 185 assets match frozen f27982cd byte-for-byte. Source masters, private scream
and quiet music duplicate are excluded. **Not installed**: the released phone
still has f27982cd. Owner campaign/settings, foreground and all 151 parked files
remain unchanged; no owned local build/renderer/transfer is running.

Rings still require the [explicit capacity decision](../../design/HORDE_1_7_ATTACK_RING_SEAM_2026_10_25.md).
New yellow gameplay admission waits for them. The [bridge plan](../../design/HORDE_1_7_COMBAT_PRACTICE_BRIDGE_2026_10_25.md)
still needs one-time event/ownership integration, the verified scream's game mix,
and an authored contact-preserving stand transition. Both rejected joins remain
disabled. Flask, forest depth and prior physical/foreground gates are not closed
by this checkpoint. No phone, merge, release or devlog publication occurred.

**Audio/haptic manual revalidation required: YES**. Normal full blocks now omit
player-damage feedback, and safe exit/optional guard break affect damage semantics.
No listening, phone haptic, balance or thumb-feel acceptance is claimed.

[Sanitized exact-source, tests, RT and artifact receipt](receipt.json).
