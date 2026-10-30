# Submitted dimensions and provisional Windows launch - 23 October 2026

Base source: `78883df79fe49f0e7281857debd0e8633474d98e`.
This checkpoint records actual command/submission facts and adds an explicit,
temporary Windows fixed-size launch with acknowledged restoration. Seven bounded
native RT lifecycle cases pass. **No full measured course, Shipping profile,
mobile launch, official resolution, ranked fairness or leaderboard is admitted.**

## Actual facts and population checks

`PresentableTinyRtScene::RecordTraceAndCopy` records the real dispatch/storage
extent, swapchain-copy extent, packed surface transform and copy/blit choice
after those commands are recorded. Camera aspect derives from that actual image
and transform using the same float math as `rt_frame.glsl`; it is not a camera
setting or GPU readback of a shader variable. DRS=false describes this renderer's
fixed scene allocation: no adaptive-resolution controller is instantiated.
It is not a measured experiment with adaptive resolution enabled.

The optional tuple stays with its recorded slot through exact submission and
completion ownership, then with each expected benchmark row. Canonical admission
rejects malformed tuples. `BenchmarkSummary` checks **every measured row** against
the same actual tuple and metadata extents, rejecting intermediate size,
transform/aspect or DRS drift and mixed availability even when endpoints match.
Historical absence stays unavailable (`submittedDimensions:null` in the local
summary), never copied from requested settings. Reports remain custom/unranked.
The existing bounded row capacity and frame-time allocation policy are unchanged.

The original profile precondition incorrectly compared camera aspect with output
aspect. Real odd-size percentage rounding can make those ratios differ. A new
721x406-to-961x541 regression reproduces two failures out of 165 before correction.
Shared camera math now follows the internal image and transform. Provisional
specifications still require their exact sizes, identity transform and DRS off;
custom odd/rotated originals restore exactly. Independent read-only review found
no weakened standard admission or ownership defect.

## Explicit Windows launch and restoration

```text
HordeLanternRT.exe --benchmark-showcase <local-output-directory> --benchmark-profile windows-rt-provisional-v1
```

Ordinary benchmarks retain their custom path. This opt-in pins the previously
proposed High/Windows 1920x1080 internal/output contract and authored showcase
route. It rejects mutation flags, wrong platforms/compiled optics and undersized
monitors. It does not silently lower resolution or select a fallback renderer.
The monitor/display mode is not changed. Temporary borderless window sizing
removes decorations/menu, centres the exact target within the actual monitor,
and uses the existing full swapchain recreation path.

`WindowsBenchmarkProfileSession` owns Applying/Measuring/Restoring phases. Actual
RT presentation plus the exact owning completion acknowledges application and
restoration. It checks completed extents, projection, backend, compiled optics
and uploaded effects; glass comes from the live allocation under the same scene
epoch. A newer record cannot substitute for an older acknowledgement. Close,
restart, escape, external resize/DPI and focus-loss messages cancel instead of
destroying/restarting the active owner. Settings input is blocked while scoped.
No confirmed/pending graphics transaction is used or persisted.

Restoration verifies original style/exstyle, menu, placement, outer/client rect,
fullscreen bookkeeping and live graphics intent, then requires a fresh RT
completion. A failed restoration stays a failure. A device loss or externally
destroyed HWND cannot be certified restored. Full-course result/report data are
latched before resource restoration invalidates their live ledger; unattended
report UI mutation is deferred. A profile attempt/restoration attachment remains
local and custom-unranked. Existing unattended launch already opens no campaign
session; no hidden-fixture mode or campaign guard was changed.

## Checks actually performed

- Five focused MSVC Release evidence/profile CTests pass in **6.31 s**, including
  **175 profile checks, zero failures**. They cover malformed dimensions, eight
  transforms, completed-slot isolation, uniform actual rows, a single drifting
  row, missing observations, metadata disagreement and custom DRS disclosure.
- Final Debug Windows launch/lifecycle and frame-evidence coordinator tests pass
  **2/2 in 4.74 s**. The actual Windows RT app builds without new warnings/errors.
- Seven sequential native Debug RT validations use the same exact staged EXE,
  muted isolated INI files and no campaign session. Every process exits 0, every
  custody comparison passes and no process remains. The native runs leave the
  built source EXE/settings, production player/arms/Kit assets and checked owner
  campaign location unchanged.

| Native case | Actual applied internal/output | Actual restoration | Result |
| --- | --- | --- | --- |
| Apply/restore, original 100% | 1920x1080 / 1920x1080 | 1232x803 / 1232x803 | Pass |
| Cancel during apply | No applied acknowledgement claimed | 1232x803 / 1232x803 | Pass |
| Cancel during measurement, original 75% | 1920x1080 / 1920x1080 | 924x602 / 1232x803 | Pass |
| Restart request | 1920x1080 / 1920x1080 | 1232x803 / 1232x803 | Pass; no restart across scope |
| Alt+F4 and close request | 1920x1080 / 1920x1080 | 1232x803 / 1232x803 | Pass; restore before close |
| Stale acknowledgement | 1920x1080 / 1920x1080 | 1232x803 / 1232x803 | Wrong generation rejected, current owner accepted |
| Apply/restore, original 75% | 1920x1080 / 1920x1080 | 924x602 / 1232x803 | Pass |

Each acknowledged application is followed by three additional actual RT frames.
Every applied tuple has aspect 1.77777779, transform 0, DRS off and no blit. The
75% original uses actual internal-derived aspect 1.53488374 and a linear blit;
its slightly different output ratio is honestly `aspectPreserved:false` and is
restored rather than reinterpreted as a standard. Exact window/menu/settings
restoration passes in all seven cases. The primary `final-apply100` applied/restored
storage-image pair was visually inspected: visible RT scene, correct recorded
image sizes and expected changed horizontal field of view. All capture files
are hashed; no image-quality or performance gain claimed.

These are explicit bounded lifecycle validations, selected by
`--validate-benchmark-profile apply-restore|cancel-apply|cancel-measure|restart-measure|close-measure|stale-owner`.
Final review found that the temporary keyboard guard swallowed Alt+F4. It now
requests the same deferred close/restoration; the final exact-binary close case
verifies its flags before also testing WM_CLOSE. All seven final cases were
rechecked after this correction.

The stale case alters only the acknowledgement generation presented to the
protocol, not a real submitted GPU packet. Their receipts intentionally retain
`measurementAndRestorationPassed:false`; no timing benchmark was completed.

## Retained failures and limits

- Initial native setup exited 2 before RT because the test runner incorrectly
  passed an empty `--campaign-test-root`; that flag activates the separate
  guarded hidden harness. The corrected invocation omits it and uses the
  existing unattended no-campaign path. The guard was not bypassed or weakened.
- The first native production-path attempt could not apply the decorated
  1920x1080 client, returned failure, and **acknowledged original RT restoration**
  with custody intact. Its rejection remains. The single bounded borderless
  window correction then passed on the actual 1920x1080 monitor at 125% DPI.
- The first native build exposed nonmovable scope state in a move-reset render
  context. A lazily allocated session now keeps a stable borrowed address and
  adds no session allocation to ordinary launches/context reset. The failed build remains.
- Preceding exact source `78883df7` [CI 39959398522](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39959398522)
  passes all six jobs. MSVC: 13m31s, configure 30s, build 606s, selection 136s,
  **150/150**, CTest 134.73s; report UI and APK resolver 10/10 pass; annotations
  empty. Earlier explicit 15-minute cancellations remain; no cap or test gate
  changes. This result does not validate the newer source in this checkpoint.

Next bounded benchmark milestone: one complete provisional Windows measured
course using the intended Shipping bank, verifying its final local report and
acknowledged rollback. The current result is a Debug lifecycle proof. Android
fixed-output launch, rotation/background recovery and practicality remain open;
the phone was not used. Profile/version/backend cohorts, sustainable performance,
owner comfort, full natural journey, Debug route deadline, PR conflicts and
deferred forest/smoke/flask decisions remain separate. No email/upload, backend
change, public website, merge, release or deployment occurred.

`receipt.json` and `788-terminal-receipt.json` retain compact sanitized aggregate
results, exact source/binary/pipeline identities and raw-log/capture hashes.
Raw publications, custody details and PNGs stay ignored under
`reports/benchmark-launch-20261023/`; none are shipped or uploaded by this work.
