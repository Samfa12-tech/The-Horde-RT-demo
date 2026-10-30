# PR27 conflict diagnosis, read-only

Compared exact local objects:

- PR: `469e0cc9885b0bed57fd2c2eb5318264d322a42e`.
- Main: `e1b142c167d57530504af51b43fd89568da61bb9`.
- Merge base: `1c28d0563a897dc72d410bdb2b61723e66ed92af`.

`git diff/show/log/blame` and legacy three-argument `git merge-tree` produced two
textual conflicts. No fetch, index/ref update, checkout or actual merge occurred.
This is a fixed-object textual preview, not semantic/merged-build acceptance.

| File | Ownership and proposed resolution |
| --- | --- |
| `.gitattributes` | Main's `5b7f29cf6` adds the 1.6.2 GitHub publication-receipt byte-protection rule at the former EOF. PR27 independently appends later asset/LFS/evidence rules, beginning in `e18f17b4`. Retain every PR rule and insert main's exact publication rule before the appended tail, preserving ordering. |
| `PROJECT_DECISIONS.md` | Main's `5b7f29cf6` inserts the verified 1.6.2 publication closeout and marks the old hold historical. PR27 inserts newer 1.7 decisions at that location. Retain those newer decisions first, then main's full closeout and historical heading, preserving all supplements and version-specific supersession. |
| `docs/ROADMAP.md` | Changed on both sides but preview combines cleanly. Retain main's closeout corrections and PR27's dated WP0 appendix (`4a28ec7e`), including its historical source identity. |

Main changes 32 paths and PR27 6,937 relative to this base. These three are the
entire path intersection; no C++/Java/shader/reporting/test function conflicts
appear. Main's other 29 one-sided release/provenance paths must also survive any
future integration. Ownership above means introducing side/commit, not a guessed
human owner. Do not choose one whole side or discard owner work.

The proposed resolution is **not implemented**. Actual merge approval remains
absent; current/future head and main changes need comparison before integration.
Full private fixed-blob/marker evidence is retained in
`reports/pr27-conflicts-20261028/`.
