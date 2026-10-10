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

The presentation layer in `menu/` adds one quiet room bed and a bounded,
actual-lantern-turn metal-creak cue. The owner-saved balance is Room 7%/Chain 21%,
under the existing SFX/focus/Play-fade envelope. The rejected flame hiss is
removed entirely. `asset.manifest.json` records the licensed local FilmCow
room foley and owner-supplied Irhouen source, hashes and processing. Originals
stay private. Final revised-package and Windows listening remain separate;
gameplay SFX/music handover are unchanged.

The owner-requested Irhouen `Metal Creaks 189729` replaces the initial Hammy01
chain. Its authored one-second event is cropped from 4.1–5.1 s and edge-faded;
room bytes and the saved 7%/21% mix remain unchanged. Replacement listening
is pending on its own exact build.

## Kit chapter speech (1.7 development)

`kit/runtime/` contains the thirteen approved English cuts with owner-confirmed
public game/repository distribution rights. They preserve the approved mono
PCM16 24 kHz samples and measured timing; source masters and private provider
account records remain outside the repository. Public provenance names Mureka
and pins the approved cut hashes. The closed manifest, reproducible
read-only-source admission recipe and ASSET_LICENSES entry identify the exact
cuts. Windows uses its generation-owned XAudio2 speech voice; Android reuses
its finite SoundPool speech mapping. Both use persisted independent Voice gain
and the shared subtitle/skip/fallback owner. `forest.wait` is admitted and mapped
but remains catalog-only until a separate gameplay trigger is accepted.
