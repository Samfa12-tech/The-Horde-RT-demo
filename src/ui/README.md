# UI / Diagnostic Overlay

The shared UI layer is the diagnostic data/text model used behind the branded platform menus and unsupported-device surface. Normal play keeps technical output tucked away unless requested or startup fails.

It should show:

- Backend.
- RT mode.
- GPU identity.
- Vulkan API and driver details.
- Required extension yes/no values.
- Internal resolution.
- FPS / frame time.
- Unsupported reason when applicable.

Both platforms expose the capability/performance view on demand. A future developer overlay may add route/enemy/BLAS detail, but it must not replace the current branded player-facing HUD.
