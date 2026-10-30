# Reuse decoded fixture inputs, retain every verdict

This follows the separately validated [object-reuse checkpoint](PROVIDER_REUSE.md)
`d21a2635`. Five added lines in `RtPipelineVariantFixtureEvidence.ps1` retain
decoded include bytes for known variant keys within one short-lived process.
Includes remain immutable during that fixture invocation. Failed reads are not
cached; successful returns retain the prior PowerShell byte enumeration behavior.
No compiler object or verdict is shared across the four variant processes.

The same complete predicate still scans the actual object, a metadata-contaminated
copy and a word-stream-contaminated copy. Neither negative mutates cached source
bytes. AST/text comparison confirms all seven other functions and the complete
main catalog/hash/word-count/negative-control sequence are unchanged. Caching is
limited to the decoder helper; no result predicate or test exclusion changes.

Source analysis bounds decoding at eight distinct includes instead of up to 14
calls: two selected, six forbidden, then those six forbidden again for the
word-contamination negative. Those counts are analytical, not instrumented counters.
The cache retains eight byte arrays until process exit; test-process peak memory
was not measured, and no game-memory saving is claimed.

All four existing evidence wrappers pass against the **same Release objects**
used in the preceding object-reuse test cohort, without another build:

| Variant | Before cache | Cache |
| --- | ---: | ---: |
| Shipping/Mobile | 31.58 s | 22.74 s |
| Shipping/High | 32.39 s | 21.63 s |
| Diagnostic/Mobile | 30.15 s | 22.88 s |
| Diagnostic/High | 30.88 s | 22.50 s |
| Sum | 125.00 s | 89.75 s |

This is one serial local observation, not a controlled whole-CI speed guarantee.
The earlier unchanged-parser control cohort totalled 112.69 s, demonstrating
local variation before this change. No repeated full pipeline, deadline increase,
coverage deletion, renderer/performance change or phone operation was used.
PowerShell syntax parsing passes. [Receipt](parser-cache-receipt.json) retains
object identities, script hashes, unmodified-predicate proof and raw-log hashes.

## Exact preceding hosted result

[Run 39951083061](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39951083061)
at `719b86aa` is terminal: **all six jobs pass**. MSVC took 14m04s, with 27 s
configure, 615 s build, 153 s selection, 149/149 CTest results, both native report
UI fixtures and 10/10 APK resolver cases. Annotations are explicitly empty;
there was no cancellation, timeout or supersession. Only 56 s remained below
the unchanged 15-minute cap. This pass does not validate the newer deduplication
commits or establish robust cold-build headroom.

The [compact hosted summary](719-terminal-summary.json) points to the retained
full phase receipt. No retry of that green run was requested. The next normal
checkpoint push supplies its own exact-head CI; it must be read separately.
