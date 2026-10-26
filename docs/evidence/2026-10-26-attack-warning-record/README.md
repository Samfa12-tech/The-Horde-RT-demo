# Authoritative attack-warning snapshot

A stateless `AttackWarningPresentation` is rebuilt at the end of
`GameSimulation::RefreshSnapshot`, after rescue/capture overrides, from the actual
combat token, attack kind and phase clocks. It carries source, kind, Windup or
Contact, phase elapsed/duration and an attack-relative contact boundary. It uses
existing skeleton1.12/0.18second and Keeper1.20/0.18second constants, not a UI,
wall-time, teaching or audio timer. Contact describes an authoritative phase,
not a successful hit, parry window or one-tick damage pulse.

Normal attacks, interrupted/dead/invalid sources, neutral recovery and ineligible
paused/dead/legacy states yield an empty record. Resuming derives the retained
phase rather than restarting it. Exact frozen windup endpoints remain Windup;
time alone never promotes them to Contact. Malformed clocks are rejected.
Keeper discharge uses the existing real discriminator. Skeleton identity must
match the singular attacker token; a handoff cannot keep the old actor's clock.

Focused Debug build and four existing CTest profiles pass: combat foundation,
revival integration, simulation timing and simulation gameplay (26.36seconds in
total). Direct boundary/invalid/handoff tests and the existing live snapshot loop
cover teaching disabled, actual windup/contact/recovery, zero-tick pause/resume
and real lethal damage. No deadline/assertion was removed or weakened.

The record is transient derived data, with no save schema, event ID, combat,
health, parry/guard animation, input authority or rendering timer change.

## Opt-in frame presentation

The shared frame adapter now consumes the record only through an explicit
`RtAttackWarningOptions` argument. Every established caller retains the default
OFF behavior; this is not normal-play admission or a platform UI toggle. The
adapter rejects stale, duplicate, missing or ineligible sources, resolves the
actual actor identity rather than the exterior render-roster override, and
anchors an opaque annulus above that actor's grounded root. Camera-facing
orientation includes the same published walking bob, crouch and rescue offsets
as the renderer, without moving the enemy anchor.

The provisional yellow circle contracts through actual Windup; Contact changes
its shape only when the authoritative phase changes. The red ellipse also has
a bounded pulse. Reduced motion uses static circle/ellipse geometry throughout
Windup, with a discrete Contact change. Neither variant creates a second clock,
claims to mark a successful hit, or equates closure with the defensive window.
Tint, chest height, size, overlap, motion and occlusion remain visual candidates.

Frame-local base colour and emission use a reusable static-PBR material override
after material aliases are packed, resolving the actual owner/primitive/row.
It rejects shared geometry/rows, transparent coverage, unavailable storage and
nonfinite factors before any write; immutable source materials, textures and
other GPU fields remain unchanged. The existing material upload carries these
factors, with no new shader branch, descriptor, light or capacity increase.

Focused Debug adapter, ABI and resource-inventory tests passed (28.59 seconds).
The subsequent camera-bob correction passed the adapter test (3.94 seconds).
Release adapter and ABI tests also passed (1.95 seconds).
These are CPU/native contracts, not hardware RT presentation, GPU allocation or
frame-rate evidence. Ring and flask remain OFF in normal play; yellow policy
remains OFF until the actual readable cue and other admission gates pass. No
native game launch, security change, Computer Use, phone or publication occurred.
