# Final metadata freeze and verified publication

Owner explicitly confirmed publication green for go on October3,2026, after
signing/recovery and exact signed S26 update smoke. Normal main integration and
scoped cleanup are authorised after verified publication. No phone operation,
new listening test or binary rebuild is required for this metadata-only change.

Final package source: `a397757249871b6b64fe5b77fc14f24e8cfcbb2b`.
`metadata-admission.json` independently compares the two73-entry Windows ZIPs:
only README, release notes and licence-status text differ. Executable/all69
current-source assets and signed Android are unchanged. The three final frozen
files are read-only, with sizes/hashes pinned by canonical release provenance.
The [initial freeze](../2026-10-03-signed-freeze/README.md) remains historical,
unchanged and is not selected for publication.

Selected local directory:
`releases/candidates/1.6.1-metadata-freeze-20261003-a397757/`.
The guarded Butler publisher accepts this explicit directory without overwriting
another freeze, and refuses to delete a pre-existing staging directory.

## Completed finite checks

- Exact945f990 push37089806774/PR37089810005: both success, all six compiler,
  Vulkan CPU-host, selected sanitizer and Android lanes. `ci-945f990-*.json`.
  The b4c6404 stale portable README assertion failure and corrected local1/1
  run are preserved; old-job reruns are not used as current evidence.
- Actual canonical preflight passes with source/artifact/origin/GitHub checks,
  no fixture or skip flags. `preflight-final-metadata.json`.
  Fresh `postpublication-preflight.json` passes origin tag/GitHub release
  present-matched and exact artifacts after publication; no repeat upload.
- Butler public Windows2055201/Android2055202 ready at1.6.1; `itch-status.txt`.
- Actual public-channel downloads: all73 Windows payload entries byte-identical
  to frozen ZIP; exact signed APK108,261,409 bytes/SHA-256bc5c7ce3...a346c.
  `itch-downloaded-payloads.json`; no claim about Butler's recompressed ZIP hash.
- GitHub release402284385 is public/non-draft/prerelease, tag/targeta397757,
  published2026-10-03T02:33:39Z. Three uploaded assets have exact canonical
  sizes/digests. `github-release.json`; public manifest download hash verified
  in `downloaded-artifact-SHA256SUMS.txt`.
- The unchanged1.6.0 shared updater parses actual public GitHub API metadata and
  selects1.6.1;1.6.1 is up to date. `public-releases-response.json` and
  `live-update-selection.txt`. No old-install platform popup/network/UI test.
- Published-line guard0a6bd27: focused Release version/policy CTest3/3 passes
  (`published-policy-tests.txt`). An initial selection used a preset with no
  compiled C++ executable (Not Run), retained separately; the actual existing
  validated build directory passes. No unchanged binary rebuild or secret access.

Next unfinished step: fresh released-docs/guard closeout CI, normal PR15 merge,
preservation-safe main sync and scoped recoverable worktree cleanup. Do not
repeat publication, signed phone smoke, accepted listening or old benchmarks.
The owner's live itch description and draft devlog remain preserved; attempted
bounded description edits did not persist and are not claimed as published.

`SHA256SUMS.txt` binds the raw receipts and complete commit inventory through
guard0a6bd27. Later documentation/integration ancestry is visible on PR15/main;
the inventory includes parallel/main integration commits, not one feature per
entry. This README is the mutable explanatory index, not a hashed raw receipt.

Audio/haptic manual revalidation required: **NO** — exact accepted runtime,
music and sound assets are unchanged. S24 final coverage remains owner-deferred
working but not fully tested; S25 unverified; measured performance and remaining
High glass defects retain their explicit owner disposition.
