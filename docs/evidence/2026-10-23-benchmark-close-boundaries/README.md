# WIP: close intent across the opening-menu benchmark

This follows `3223f6ed96577f538e40fd9614af4f39851de7c2`. The complete benchmark
menu remains **unadmitted**. Three bounded, background-only Debug cases exercise
native HWND handling, real profile resources, completed RT presentation and
retirement. No foreground activation, screenshot, physical input, listening or
phone operation is used. The earlier cancellation/polling failures remain intact.

## Demonstrated defect and focused fix

The first WM_CLOSE injection during Queued Entry detachment reached the real
bridge. It set the transfer's close/cancel flags, returned through a fresh Entry,
retired the final owner and exited zero with custody unchanged. Nevertheless, the
successor's lifecycle receipt said `closeRequested=false`. The acceptance
assertion correctly failed: **Close intent missing from successor lifecycle**
(12.125 s). That red receipt is retained, not relabelled as a passing shutdown.

Source confirmed that the new benchmark owner imported `cancelPending` but omitted
`closeRequested`. It now OR-imports both into its existing lifecycle flags before
profile apply, preserving intent received directly during construction. The
stable transfer still controls deferred closing only after real fresh Entry
presentation. No fake restoration/completion proof, early surface destruction,
campaign reset or separate renderer owner was added.

## Final bounded checks

| Native case | Actual evidence |
| --- | --- |
| Detached WM_CLOSE | **10.953 s, pass.** Null context, matching Queued bridge, pending cancellation, truthful successor close flag, no profile apply. Fresh Entry presentation precedes final retirement and HWND destruction; exit zero. |
| WM_CLOSE after Standard apply | **11.828 s, pass.** Actual completed submission at 1920 x 1080 internal/output, DRS off. Original 1232 x 803 output / 924 x 602 internal and complete submitted-dimension tuple restored; exact window/style/menu bookkeeping acknowledged. Fresh Entry is then presented, retired and closed; exit zero. |
| Detached Esc regression | **8.625 s, pass.** Correct cancellation witness, no close flag or profile apply, fresh Entry and `BENCHMARK CANCELLED`; exact original window retained; exit zero. |

All three use the same final Debug executable. The staged settings, original
build-directory settings and owner campaign hashes remain unchanged, and each
isolated fixture still contains only its manifest. Reports remain local,
custom-unranked, campaign-free during Showcase, with no measurement-success claim.

Final Debug/Release compilation passes. The existing five focused Debug
retirement/menu/profile/Entry/launch contracts pass in 0.14 s. Startup guards
require isolated mode for close injection, reject simultaneous cancel/close
injections and retain Release's Debug-only rejection, all without starting a
renderer. Read-only review found the OR-import, deferred-close ordering, transfer
lifetime and no-activation guards consistent with these boundaries.

The preceding correction's exact [CI40024208451](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40024208451)
is terminal success at `3223f6ed`, attempt 1, all six jobs passed, updated
21:15:18 UTC (MSVC 483 s). This does not certify this later close-intent fix;
the new checkpoint needs its own exact-head CI result.

Reproduce each separately with a new label and the built Debug app:

```powershell
python tools/validate-windows-benchmark-detached-cancel.py new-close-label --action detached-close
python tools/validate-windows-benchmark-detached-cancel.py new-profile-label --action close-after-apply
python tools/validate-windows-benchmark-detached-cancel.py new-esc-label
```

The post-apply close selects the provisional Standard profile deliberately to
exercise real dimension changes and rollback, rather than unchanged Custom
settings. No full course or performance experiment was repeated. Timings above
are bounded test durations, not FPS or performance gains.

## Remaining acceptance

- Visible Windows layout and DPI, physical keyboard/controller interaction and
  Entry/Showcase transition listening remain **pending**. No activation retry or
  alternate route was used. All new native runs explicitly use nonactivating
  isolated startup with update/error prompts suppressed.
- Release compilation and early rejection are verified; **Release menu runtime
  interaction is not**. Its safe isolated menu adapter is intentionally Debug-only.
  Normal Release startup can activate and access ordinary campaign/settings state;
  it was not launched past that guard under the current restrictions.
- These are specific WM_CLOSE and Esc boundaries. Other message interleavings,
  physical Alt-F4/Exit behavior, error/fatal-retention paths and full natural use
  are not inferred from them. Earlier standalone Shipping course evidence is
  separate from this new menu ownership path.
- Android profile launch/adoption and phone acceptance remain separate. PR27 base
  conflicts are unchanged; this checkpoint grants no merge or release readiness.

[Exact sources, executable/receipt identities, retained failure, guards and CI](receipt.json).
Full local logs/custody files stay ignored under
`reports/benchmark-menu-entry-20261023/`. The original failed receipts are unchanged.
All 151 parked owner files remain excluded. No torch edit, transmission, merge or
release. Audio/haptic manual revalidation remains **YES** for the menu transition.
