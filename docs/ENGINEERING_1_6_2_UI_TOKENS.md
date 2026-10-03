# Native UI prototype tokens

The Android and Windows 1.6.2 prototype uses charcoal `#151719`, slate `#24272A`, parchment `#F2E9D8`, brass `#CFA96A`, iron boundary `#777E84` and disabled text `#AEB0AC`. Android secondary text is `#C9C4B8`, with `#D96F65` reserved as the danger token. These are source prototype choices, pending owner visual acceptance through the real RT scene.

Native controls retain their platform input/accessibility semantics. Buttons use a brass boundary, an inset/changed backing on press, an opaque parchment focus boundary and opaque disabled text. There are no custom animations or imported icon assets. Labels and focus remain opaque when the user changes backing opacity.

Android keeps its default action slots, pointer ownership and action timing. Paused Interface / HUD settings provide Comfortable/Compact presets, bounded 85–110% control scale, 55–95% backing opacity (70% default), stronger backing (95%) and optional routine RT status. Applying layout happens while input is gated. Reset Interface clears only those five presentation preferences and restores Show HUD; graphics, audio, consent and RT Lab remain intact. The native paused cluster preview is noninteractive. Minimum targets are 48dp and menu labels wrap within a scroll panel with safe-area padding.

Vitality uses three fixed segments plus readable life text at the top left. Successful routine RT status is optional and compact; its visible chip opens diagnostics. Loading, unsupported-device and error diagnostics remain discoverable. Diagnostics and benchmark output retain their detailed presentation.

Host fixtures cover preset/scale combinations at font scales 1.0, 1.3, 1.7 and 2.0, safe-area separation, stable contextual slots, long menu label wrapping, explicit focus and opaque disabled labels, and scoped preference migration/reset. Physical font rendering, touch, composited contrast against torch/dark/finale backgrounds and overlay cost remain owner/device acceptance gates. No measured cost or accepted theme is claimed by these host fixtures.
