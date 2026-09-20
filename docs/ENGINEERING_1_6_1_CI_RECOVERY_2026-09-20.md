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
| PR conflicts hide current compiler checks | Documentation reconciliation prepared; remote mergeability and fresh jobs still to verify |
| Portable lane excludes actual skinned smoke/fixtures | Focused CI coverage being added; not yet a passing job |
| Phase 2 generated-runtime admission | Open; repeatable single-thread artifact differs in vertex ordering/tangents and has not replaced the accepted runtime |
| Phase 1 measurement foundation | Accepted; not being restarted |
| Dedicated viewmodel, glass/backend parity, music/reporting and final matrix | Remain in scope and incomplete |

Audio/haptic manual revalidation required: **NO** for this documentation/CI work.
