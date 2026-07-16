# Audio

The showcase alpha uses a deliberately small FilmCow Recorded SFX subset:

- UI select, back, and menu toggle.
- Two sword swing variants.
- Two metal impact variants.
- One enemy fall.
- Two player footstep variants.
- Two skeleton footstep variants.
- One skeleton attack/rattle cue.
- Lich charge, impact, hurt, and fall cues.

Runtime files are mono 48 kHz 16-bit PCM WAVs under `filmcow/`. They are derived from the local Possum Cafe archive by `tools/import-filmcow-sfx.ps1`; exact source names and licence terms are recorded in `ASSET_LICENSES.md`.

Long ambience, voice work, music, and a larger mixer remain out of this alpha. Android uses `SoundPool`; Windows uses bounded asynchronous WinMM playback and allows movement cues only when they will not interrupt a combat/menu cue. Audio failure must never hide or alter the native RT capability result.
