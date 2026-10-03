# Current-source CI recovery

## Reconciliation

Owner resumed work with the quota guard removed. Local and remote engineering
heads both started at `f9eb5149a468a52070011108c0eb7586eb80a365`, a documentation-only
descendant of requested checkpoint `cb3a29e`. Four unrelated scratch paths remain
untouched. Fresh fetch identifies main as `bda1b99a62e1de883273dd21f267bcfc92bc5490`.
PR #15 was `CONFLICTING` / `DIRTY`, with no check runs at the current head.

Merged main **into engineering**, without rebasing, force-pushing or merging the
unfinished programme into main. The five conflicts were documentation only:

- AGENTS: retained the lean main guidance plus owner-authorised hardware RayQuery
  and direct lead implementation scope already reconciled on engineering.
- FUTURE_WORK: retained the detailed S24/S25 design, with the newer owner-approved
  inclusion of compatibility, music and reporting inside 1.6.1.
- AGENT_ENGINE_CONTRACTS: retained shared main contracts and accepted 1.6.1
  submission ownership, shader policy and world-body/viewmodel requirements.
- AGENT_VALIDATION: retained main guidance and the later targeted-development /
  full-final-candidate sequencing and exact-artifact boundaries.
- ANDROID_RT_DEVICE_COMPATIBILITY_RECORD: kept the September 20 date and all
  accumulated device evidence; main's tested-configuration wording is preserved.

Main's README links and future 1.7/1.8 roadmap documents are retained, without
authorising those future milestones. No runtime source changed in this merge.
Validation: reviewed main-versus-engineering differences, resolved conflict
markers, inspected staged documentation and checked whitespace/relative links.
This is merge review, not compiler, hardware or release acceptance.

## Current finding status

| Finding | Status |
| --- | --- |
| PR conflicts hide current compiler checks | Resolved: main reconciled in `d3a7225`; PR is MERGEABLE, description updated, and fresh push/PR jobs exist for `cd8614b` |
| Portable lane excludes actual skinned smoke/fixtures | Resolved coverage gap: Vulkan-enabled GCC 13.3 / SDK 1.3.275 CPU-host lane passes 8/8 on both fresh push and PR runs at `cd8614b`; local MSVC Shipping/Mobile also passes 8/8 |
| Portable suite current-source failure | Resolved in `c46e255`; fresh `adc4579` push and PR runs pass 43/43 portable tests and 8/8 focused Vulkan-host tests |
| Phase 2 generated-runtime admission | Open; repeatable single-thread artifact differs in vertex ordering/tangents and has not replaced the accepted runtime |
| Phase 1 measurement foundation | Accepted; not being restarted |
| Dedicated viewmodel, glass/backend parity, music/reporting and final matrix | Remain in scope and incomplete |

Audio/haptic manual revalidation required: **NO** for this documentation/CI work.

## Recovery gate closed — results verified 2026-09-23

Fresh push [35500684339](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/35500684339)
and PR integration [35500686564](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/35500686564)
both completed successfully at `adc4579763abe474a08cf8aa9172e0d622579a34`.
Each passes **43/43 portable tests and 8/8 Vulkan-enabled CPU-host player tests**
with GCC 13.3. PR #15 is `MERGEABLE` / `CLEAN`; its full-scope description was
updated. Main was not changed. These results close CI recovery, not the remaining
engineering programme or physical-device gates. Subsequent runtime-asset admission
will receive its own fresh CI run.

## Fresh evidence and first follow-up

- Push run [35500137514](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/35500137514)
  and PR run [35500139386](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/35500139386)
  are new runs for `cd8614bf83404bd342b9f84fb434254d39d58e1e`, not old-job reruns.
- Both focused player lanes pass all eight tests. The retained portable lane
  exposes one failure in `horde_rt_desktop_controller_input_tests`: it still looked
  for an inline UI pause assignment superseded by `MeasurementPausedByUi`.
  Actual tuning forwarding and UI pause propagation are intact in current source.
- Reproduced the exact RED locally. The repaired assertion requires the helper's
  settings/RT Lab/diagnostics/report logic, its actual assignment to
  `simulationPaused`, propagation to `simulationInput.paused`, tuning forwarding
  and reset. Runtime code was not changed to satisfy this test. External-build
  source lookup is fixed by an explicit CTest working directory.
- Final local MSVC targeted test passes 1/1 (1.41s). The broader CI result remains
  pending until a new run includes the fix. Logs stay under ignored `reports/`;
  the immutable GitHub run links retain remote evidence.
- Imported main roadmap files are byte-preserved, including intentional Markdown
  two-space line breaks reported by `git diff --check`; new/resolved edits pass
  the scoped whitespace check. No unrelated formatting churn was applied.
