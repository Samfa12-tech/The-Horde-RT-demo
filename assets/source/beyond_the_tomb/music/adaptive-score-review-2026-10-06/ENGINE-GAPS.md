# Renderer/editor and integration gaps

These are observed compatibility limits and future requirements, not completed engine work. See `README.md`, `tools/RENDERING.md`, `validation/bass-audit/BASS-REPORT.md`, `validation/bass-audit/BASS-RENDERER-AUDIT.md` and `manifests/cue-manifest.json` for evidence and detailed proposals.

1. Expand editor melody register support without clamping canonical notes. Eleven optional octave-projected banks are supplied only for editing in today's grid.
2. Preserve compact held-note and mix semantics during schema migration. Canonical schema17 banks explicitly identify compact mirrors; generic migration placeholders alone do not preserve this intent.
3. Restore or compare Core/app bass filters, envelope and beat-volume behavior. Current Core ignores those intended filters/mixer settings; the softer-upright preview is an experiment, not a fix.
4. Establish actual browser/live-render parity for timbre, envelopes and FX. Node VM import/export checks do not test browser audio.
5. Implement actual section-end transitions rather than bar quantization; audible stingers and return actions; independent stem/layer mixing and dialogue ducking.
6. Define persistent cue state, one-time reveals, death/cancellation, pause/resume and save/reload behavior; then validate adaptive joins and intelligibility on target devices.
7. Admit assets through the native platform streaming, voice-count, memory, loudness and provenance gates before runtime use. Full-track MP3s are review conveniences, not sample-accurate loops.

No engine code, native integration, accepted game-bank replacement or production release is part of this archive.
