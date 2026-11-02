# Chimney smoke and benchmark discoverability

Development work following owner acceptance of Kit's basin guidance. No release,
ordinary feature admission, Android performance or owner smoke acceptance implied.

- `a2ccf862` makes fresh/reset Windows particles Standard. Existing explicit
  preferences and Android Low defaults are preserved; graphics default and
  persistence tests pass.
- The three smoke origins match actual B01, B03 and B06 cap corners imported
  from the unchanged Bellwether v4 GLB. A native-import test verifies them.
  Untracked v5 chimney experiments and all other parked owner assets are untouched.
- Reuses existing lit, world-space analytic particles, clipped by primary RT
  depth. No shader change, emissive smoke, dynamic light, texture, mesh or AS
  update. Low permits at most 9 puffs; Standard 15; Off performs no smoke work.
  Distance is bounded to 180 m with a final 25 m fade. Absolute simulation time
  drives bounded rise/drift and fade. Existing four candidates/tile and 64-mote
  upload remain fixed, with smoke below slot48 and combat retaining priority.
  This is a source budget, not measured GPU cost. Existing particle limitations
  (no secondary/reflected particles) remain.
- Windows front menu now says More / Benchmark. Benchmark is first in More,
  remains visible when temporarily ineligible (disabled safely), and receives
  initial focus when eligible. Existing chooser, explicit Run, Back/cancel,
  renderer ownership, settings restoration and reporting consent are retained.
- Nine focused host suites pass: indoor dust, Bellwether import, desktop input,
  benchmark menu ownership, graphics defaults/persistence, benchmark launch,
  RT scene ABI and raygen artifact ownership. Release Windows build succeeds.
  Smoke tests cover cap matching, quality/off, deterministic recreation,
  capacity/tile accounting, invalid time and distance. Menu source/native owner
  contracts cover ordering, safe Back and cancellation; they do not certify
  physical controller feel or mouse use.

## Actual RT image check

The local Standard-particles capture shows the small pale plume above the
middle roof; the matched Off control has no plume there.
Images are retained under ignored `reports/chimney-smoke-menu-20261102/endpoint-{standard,off}/232-chapter-demo-endpoint.png`; no new LFS image payload is added.
Both use the authored demo endpoint and the existing hidden native RT capture,
not a concept render or injected postprocess. The scene remains intentionally a
distant closed town shell. Capture receipt records binary/shader/image hashes.
Both exited0 and preserved foreground, settings and campaign bytes. Debug native
capture used its current diagnostic High shader pair; this is not final Shipping
or phone evidence. No shader files were changed. Absolute-time animation has host
coverage; a frozen still does not establish subjective moving-smoke acceptance.

The existing debug build tree initially rejected a mixed CMake3.22/3.31 module
invocation. Rebuilding with its original Visual Studio CMake3.31 completed.
The first new Off test incorrectly expected an output reset even though Off is
specified to leave bytes untouched; the fixture now starts with a known empty
frame and still checks byte-neutral Off behavior. No runtime assertion relaxed.

Mouse/controller physical feel for the new entry remains an owner check. Existing
visual-order navigation and native benchmark cancellation tests pass. No owner
window was closed, no foreground input sent and no phone accessed during this slice.
Final hosted checks are reported by exact development commit at closeout.

