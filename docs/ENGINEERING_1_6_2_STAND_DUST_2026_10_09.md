# Keeper stands and packaged Dust Low checkpoint — 9 October 2026

Integrated source `ac97da914e0578b4dc04f0428fa9ee49922783f9`, tree `781e2c23bc6fc6be3167b1c4e0a7058d86cacfd7`. The accepted torch-water material correction is retained. This is an immutable Debug inspection checkpoint on draft PR18, not a release or Eric's completed audit.

## Physical stands, preserved audio

The two existing Keeper-room torch stands now share their rendered 0.16 m base half-extent and authored world centres with player and Keeper collision. The player uses the existing rectangle resolver; Keeper orbit, charge and recovery use that same resolver before their existing chest collision. No reach, hit timing, damage, lantern-light lifetime or rendering dimensions change. Player sliding, normal route/backtracking and chest stand-off remain usable in host regressions.

Before the fix, the delegated focused regressions fail both player crossings and real Keeper movement. Those failures are retained in agent tool output, not invented as separate log files. Current integration independently passes their tests. Keeper coverage includes a 60-second rear-prop recovery fixture and a room target grid using actual walking/orbit/charge/recovery simulation.

Integration review then reproduces four acoustic assertions: the low floor bases were accidentally treated as full-height walls by the existing XZ wall approximation. Source `ac97da91` restores that approximation's original five masonry rectangles while movement retains all seven obstacles. Ear-height sound across each base and stand-centred cues stay unobstructed; existing arch/bend obstruction and chest sound checks still pass. No menu/audio asset or mix changes; owner audio/haptics remain approved.

## Fresh/reset Dust Low

At the owner's explicit request, fresh and staged-reset defaults use **Dust Low** on Android and desktop. Android remains **50%**, Mobile water/fire, Glass Off, Current shadows, cap30 and Mist On; desktop remains 100%, High water/fire, Glass On, Current shadows, cap30 and Mist On. 33%/40% remain experimental choices.

Saved Off/Low/Standard and custom tuples take precedence. Historical baseline, genuine pending-only legacy migrations and older schema migration behavior retain their recorded Off policy. Fresh Back/Restore without Use keeps the fresh tuple authoritative. Reset does not silently save; exact native Use/Keep/Restore acknowledgement rules remain intact. Tests cover saved Dust values, fresh no-preview restore, interrupted reset and preserving other preferences. No device preferences are cleared to test defaults.

## Exact packages and validation

| Artifact | SHA-256 |
| --- | --- |
| Debug APK, 138,462,962 bytes | `90a80a31d88f7f758fdb9587c7917a0a61f1c1257f237df0debb58e66df40593` |
| Windows Debug EXE, 11,572,736 bytes | `29608d38a259045aecab318ef9f9d6f45d63ad6e0a30a4202102b8dcfcbf629a` |

- Seven affected host suites pass in 24.74 seconds: graphics settings, development checkpoint legality/assets, simulation gameplay, route, Keeper gameplay, spatial audio and character/render-slot contracts. Windows build passes.
- Current Android results contain **256 tests / 39 classes, zero failures/errors**; lint and Debug assembly of all four ABIs pass. The acoustic-only follow-up reuses unchanged Java results and rebuilds native payloads.
- Closed package asset admission, ZIP 16 KiB alignment, fullUser orientation and retained controller configuration manifest pass. All 94 APK asset entries are byte-identical to the accepted water APK; shaders/assets are unchanged by this slice.
- Exact installed-base pullback on **SM-S948B / Android 16** matches the sealed APK; settings and menu mix remain byte-identical. Normal launch uses no Debug checkpoint/effect overrides. The foreground portrait entry is visually inspected, then handed to the owner for ordinary stand testing. This is not a physical collision or controller pass.
- Source CI: [push37847101238](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37847101238) / [PR37847110165](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37847110165), pending at publication. Previous documentation head `58895deb` passed 12/12 separately; it does not certify this source.

[Sanitized receipt](evidence/2026-10-09-stand-dust/receipt.json) records exact four-ABI payload hashes, checks, preserved preferences and evidence class. Prior seals remain untouched. The normal-launch harness retains its corrected package/activity-name and Android 16 foreground-field refusals; these were diagnostic guard errors, not Vulkan or persistence failures.

## Remaining disposition

Owner waterfall appearance, audio/haptics, smaller Dust Low appearance, ordinary 33% play, touch/BB-51 menu and recorded recovery checks remain accepted under their exact recorded scope. New physical stand/player/Keeper collision acceptance is pending. Long sustained phone pacing/thermal/power/memory work is owner-deferred; existing measured gaps remain and sustained 30 FPS is not certified. No resolution/effect reduction hides collision or Dust cost.

Moving combat contact/downstroke calibration, remaining moving equipment/body/shadow/reflection and screen/lifecycle integration, a final immutable review candidate with current aggregate CI, and Eric's independent audit remain separate work. Minor walking torch-arm wiggle, dust shafts, seamless music handover and a temporal/vendor renderer rewrite remain deferred. No merge, production signing, tagging or release.
