# Audio

The showcase uses a deliberately small FilmCow Recorded SFX subset:

- UI select, back, and menu toggle.
- Two sword swing variants.
- Two metal impact variants.
- One enemy fall.
- Two player footstep variants.
- Two skeleton footstep variants.
- One skeleton attack/rattle cue.
- Lich charge, impact, hurt, and fall cues.

User-selected Pixabay cues provide the waterfall torch extinguish, Gothic chest
unlock latch, chest-opening creak, and waterfall ambience. Runtime files are
mono 48 kHz PCM16 WAVs under `pixabay/`; exact source hashes, processing, and
licence evidence are recorded in the adjacent metadata files.

Runtime files are mono 48 kHz 16-bit PCM WAVs under `filmcow/`. They are derived from the local Possum Cafe archive by `tools/import-filmcow-sfx.ps1`; exact source names and licence terms are recorded in `ASSET_LICENSES.md`.

Voice work, music, and a larger mixer remain outside this update. Android uses `SoundPool` left/right gains; Windows uses XAudio2 per-voice matrices for centred and spatial cues with WinMM fallback. Audio failure must never hide or alter the native RT capability result.
