# RT Lighting Refinement - 2026-07-12

## Torch lighting

- Camera-held props use TLAS mask `0x02`; corridor and skeleton geometry use `0x01`.
- Primary and reflection rays still see both masks.
- Direct-light visibility rays use `0x01`, so the held torch and sword no longer cast distracting solid self-shadows onto nearby floors and walls.
- Torch visibility samples a small two-point flame area with a deterministic interleaved sample per pixel. This preserves one shadow ray per pixel while breaking up perfectly pin-sharp point-light edges at the phone RT resolution.

## Second-room roof light

- The original solid ceiling now ends before room two.
- Room two has four real stone roof slabs with three physical gaps between them.
- The directional sun/moon light uses the existing ray-query visibility test against those slabs. Surfaces receive the light only when the shadow ray genuinely passes through a gap.
- The earlier experimental screen-space/analytic shaft overlay was rejected and removed because it looked artificial and violated the RT-first visual intent.
- No fake volumetric shafts ship in the phone path.
- Visible dust shafts remain a future option only if implemented as actual participating-media scattering and proven within budget, likely desktop-first.

## Validation

- Android and Windows diagnostic targets compile with the revised geometry and shader.
- Fresh Android install reached successful RT swapchain presentation.
- With the volumetric experiment removed, the phone path adds no extra full-screen roof visibility ray beyond the existing direct-light shadow query.
- Repeated development runs heated the phone enough to vary sustained timings. The last clean first-room sample before removal of the fake overlay was 16.667 ms median and 16.667 ms p95 over 126 intervals. A room-two run under accumulated heat averaged roughly 20-21 ms internally. Re-run a cold room-two benchmark before treating this as final thermal evidence.

## Asset packaging

The five Poly Haven materials remain visually settled enough for this slice. Runtime data is still three raw 512 x 512 x 5-layer arrays. Gradle now generates a runtime-only Android asset set, reducing the debug APK from 93,855,324 to 46,793,811 bytes. GPU compression remains open work; source JPGs stay in the repo but are not packaged.
