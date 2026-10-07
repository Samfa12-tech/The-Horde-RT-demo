# Player Rag torch socket and reachable roof correction

7 October 2026, PR18. This is the affected host validation for the correction
following `0e98a8bb`, developed on `cb1570a1`. It is not the final integrated
package, physical-device acceptance or sustained-performance evidence.

The final player item now composes its authored Rag `Grip`, rather than the
former ornate torch's socket. Keeper/reward assets retain their original socket
contracts. The horizontal carry target moves from -0.160 to -0.135 m so the
actual Rag mesh fits the existing portrait frame; carry depth remains 0.68 m.

Roof lowering and camera-side retreat belong to shared kinematics. The imported
rig's wrist-from-Grip offset and scaled arm-chain lengths are pinned against the
actual asset. A bounded four-iteration excess-reach solve retains the roof-cleared
pose when reach is adequate, fades its reach penalty with clearance demand,
limits individual proposals to 25 mm, and accepts only objective-decreasing
backtracked proposals. Overall retreat remains capped at 0.45 m. The same final
target supplies the arm, item, visible flame and physical light; there is no
separate renderer/light correction.

## Reproduction and affected checks

- Earlier candidates preserved failures: a total-distance objective activated a
  0.438558 m walking jump near a roof boundary; an excess-reach variant passed
  continuity but missed one original Grip witness by 21.65 mm. Its proposal
  trace exposed a Newton step increasing the objective, followed by a return
  step. Descent backtracking corrected the witness, but the 120 mm proposal cap
  still produced a 40.729 mm sampled jump. None is hidden or relabelled a pass.
- Final original two reach witnesses pass without changing their inputs or
  the existing 15 mm Grip tolerance.
- `horde_rt_held_item_clearance_tests` passes (3.98 s), including 9,696 portal
  cases and the fixed walking/look continuity guard below 40 mm per sampled
  step. The incorrectly named earlier CTest invocation found no tests and is
  explicitly an invocation gap.
- `horde_rt_final_held_torch_clearance_tests` passes (168.00 s): 3,287 actual
  final-rig poses, zero failures, maximum Grip error 0.0118211 m, worst measured
  headroom 0.0323658 m. Its 1,812 roof poses also skin the actual 15,855-vertex
  viewmodel. Tests use the actual Rag vertices and Flame/Light sockets, rather
  than the old ornate fixture. These are strict geometric checks, not RT frames.
- Player animation, held-item sockets (including imported sword swing/parry
  overhead cases), and preview frame adapter checks pass: 3/3, 14.16 s.

External local receipts: `task-4/rag-bounded-step-witness-20261007-01.log`,
`rag-bounded-step-continuity-20261007-01.log`,
`rag-bounded-step-full-20261007-01.log`,
`rag-bounded-step-full-stdout-20261007-01.log`, and
`rag-socket-affected-tests-20261007-01.log`. Failed intermediate attempts and
the bounded candidate/proposal diagnostics remain alongside them.

## Remaining gates and cost

The solver can add up to 20 backtracking roof evaluations across four iterations.
This is a bounded CPU cost, not a measured phone saving. Measure it in the
integrated simulation/frame trace before accepting its runtime cost. Physical
moving hands/flame/light, dropped/drenched states, body stow and reward transfer,
shadows/reflections, owner appearance/feel and affected Android/Windows package
checks remain required. The Rag source/runtime provenance and atlas preservation
record are unchanged; no additional asset licence is invented here.
