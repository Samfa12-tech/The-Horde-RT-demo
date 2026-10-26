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
health, parry/guard animation, input authority or rendering timer change. No
adapter consumes it yet. Normal ring visibility, flask and yellow policy stay
OFF. Reduced-motion transport, actual ring placement/style/tint and RT evidence
remain next gates, alongside the separate flask native hand/use integration.
No native game launch, security change, Computer Use, phone or publication.
