# Hosted checkout and fixture diagnosis - 22 October 2026

Base `93b49b018c1876b388582a7ba751023c20532285`, existing WP2 worktree and
PR 27. This checkpoint changes CI hydration, text encoding and exact-content
fixtures/receipts. Runtime C++, shaders, mesh/PCM payloads, tree positions,
quality settings, saves and all resource ceilings are unchanged.

## Verified causes and bounded corrections

| Failure | Evidence and correction |
| --- | --- |
| Ambience/foliage hashes and byte counts | Hosted failures read three **130-byte Git LFS pointers**, whose hashes exactly match Git pointer blobs rather than the unchanged PCM manifests. All four CI fetch/checkout pairs now include only the two missing runtime WAV roots. An isolated remote LFS cache fetch retrieves all three exact payloads (277,898 bytes total), and hashes/sizes match. No working payload is replaced. [Identities](audio-payloads.json), [scope](workflow-scope.json), [fetch](lfs-fetch.txt). |
| Windows locale text read | With Python UTF-8 mode explicitly disabled, the existing licence read fails on cp1252 byte 0x9d at 8249. All eight text reads in this test now specify UTF-8; binary hash, finite-PCM and headroom assertions stay intact. [Original failure](audio-cp1252-baseline.txt), [4/4 pass](audio-cp1252-test.txt). This locally reproduced encoding defect is separate from the hosted pointer-hash failures. |
| Exact generated water bounds | Git's prior HEAD attributes plus autocrlf=true produce 686 bytes/13 CRLF versus 673 bytes/13 LF from native output. One file-specific eol=lf rule makes checkout bytes exactly equal to the native generator, with unchanged numeric contents. [Filter reproduction](water-checkout.json) uses HEAD attributes for the before case and current attributes for after. It is local reproduction, not a retained raw hosted checkout. No tolerance or geometry change. |
| Forest root receipt and asset pin | The committed r23 terrain is 18,441,928 bytes, SHA c636a679...b537774. Its older policy pin and root receipt still named fb21e177.... Only the identity is refreshed; all 64 placement rows and all grounding measurements regenerate identically. All 46 policy rows now match committed identities (45/46 matched before). No source-era authoring tool is rewritten to pretend it used r23. [Provenance](content-provenance.json), [strict root check](root-test.txt), [placement header](placement-test.txt). |
| Android startup staging fixture | Production moved the 91-file runtime cache into stageRuntimeAssetCache(), called before writeReports. The test now scopes the unique helper and startup, requires one startup invocation before root publication, and retains exactly-once/exact-destination checks for all 18 required files. Existing removal/wrong-destination negatives now cover all 18; missing/late/duplicate helper calls, duplicate path staging and a 19th roster entry are rejected. Production Java is unchanged in this commit. |
| Native resource inventory | Current terrain has 37,846 triangles/19,486 vertices, within unchanged 39,618/21,134 caps. Accepted terrain commits 11b768dd/48662cc1 and debris 2c9fe54a left the integration fixture at 34,050 triangles and omitted 72 chip triangles. The 11,604-index discrepancy is exactly (3,796+72)*3. Correct those content expectations only; retain 32-material/32-primitive ceilings, actual Mobile 31/High 32 primitives, 30 imported+2 world materials and 18/15/15/1 texture layers. |
| Skinned smoke abort | Telemetry 411d634d added nested pose/vertex scopes, but the injected clock still had two samples. Its third .at() read aborts inside noexcept, exactly matching current CI. Supply six monotonic samples; retain outer 27 ns, one invocation and cached-same-tick no-read behavior, and additionally assert pose 7 ns/vertices 16 ns, one each. No runtime timing or skin behavior change. |

## Actual checks and retained failures

- [Four affected Release CTest registrations pass](selected-ctest.txt), including
  **199 admission/staging/archive cases**, strict root regeneration, regional
  water and local-audio provenance. The closed package fixtures are temporary
  synthetic test archives, not release artifacts.
- [Fresh native build](native-build.txt), then actual
  [resource-inventory](inventory-test.txt) and [skinned-player smoke](skinned-test.txt)
  executable runs both exit 0. These are CPU/Vulkan-contract fixtures; no GPU AS,
  RT presentation, gameplay performance, phone or listening pass is claimed.
- [Original c2bbc714 failure excerpts](hosted-baseline-excerpts.txt) are retained
  from [run 39851917764](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39851917764).
  [Exact 93b49b01 Vulkan-host excerpts](current-head-vulkan-excerpts.txt) from
  [run 39856202161](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39856202161)
  confirm only inventory and clock failures remain there; the previous eight
  render-slot source assertions pass in that job after the preceding checkpoint.
- At inspection, that run's MSVC lane remained in progress; GCC/Clang/Android
  were failed and the finite sanitizer lane passed. Corrected hosted reruns are
  pending the backup push. Local passes do not claim full hosted CI green.
- The separate [API24 compatibility checkpoint](../2026-10-22-survey-api24/README.md)
  addresses the real Android API 26 at min 24 lint defect. It is not an asset or
  shader workaround.

PR conflicts and the inherited Debug route 30 s timeout stay open. Next use the
fresh hosted results to identify any residual failure before safe PR conflict
resolution. Natural journey/recovery, actual endpoint listening, physical touch
feel, sustained warmed-phone performance and unspecified flask policy remain
separate acceptance gaps. No phone, public release, merge or owner-save action.
