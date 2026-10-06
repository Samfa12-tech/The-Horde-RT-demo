# Soundtrack source and reference-audio archive

Archived at the owner's request on 2026-10-06 for The Horde's planning branch. This is a source/review archive, not runtime admission or a release. The source directory is outside the closed Android and Windows packaging allowlists. Existing game music and application code are unchanged.

## Contents

- 15 canonical family banks in `scores/` and 2 optional event banks in `event-scores/`, each with equivalent PCS1 in `share-codes/`.
- 11 optional current-editor register projections in `editor-compatible/`; every intentionally raised note is listed. Canonical scores retain the intended lower register. The app grid clamps these 11 banks; Core supports their canonical register.
- 136 lossless stereo 44.1 kHz PCM16 FLAC section renders in `section-audio/`, with cue timing, hashes, render metrics and musical event data.
- 17 full-review MP3 assemblies in `previews/full/`, plus the original listening tour and a separately labelled softer-bass tour/A-B experiment.
- Cue maps, authoring brief, register index, validation summaries and renderer gap notes. WAV outputs are represented by lossless FLAC rather than redundant archived PCM. Diagnostic-only WAVs and full-review WAVs are not archived.

## Credit and permission

The original **What the Dark Keeps** score was supplied by the owner, authorized for Horde use only on 2026-09-30. No named original composer was established; original authorship is not reassigned. Its exact original admission notice is preserved in `source-reference/Original-Use-and-Attribution.md`, and the supplied JSON SHA256 is `3ab832e08807d3745fca172b0bb6b95ccbbe37face4e60077027d6d35b55c9a0`. The later owner instruction authorizes this repository archive; it does not grant a general reuse licence to third parties. New companion scores and arrangements were authored by the OpenAI assistant for the owner. No exclusive legal copyright claim is made.

The original notice describes an earlier accepted native bank and its historic render process; it is not the manifest of this expanded review suite. No original native runtime assets were replaced here.

## Renderer and verification boundary

Reference rendering used Pocket Audio Core **0.2.0**, Pocket Chordsmith **v68**, at commit `0c6975cadcd4aba0047ab94170a2adabc722517f`; final canonical projects use schema **17**. The engine, editor and PCS implementation remain separately licensed/private-UNLICENSED and are not copied into this archive. Wrapper scripts only import external modules; use a separately authorized checkout through `POCKET_CHORDSMITH_ROOT` or the renderer's `--core-root` argument.

Canonical notes, voices and held durations were validated against actual Core timeline events. Actual app-code import/export was evaluated with DOM mocks for 28 files. Browser UI/audio parity, filtered live-render parity, final mastering, owner listening acceptance and native integration are not verified. Core classic-bass reference audio must not be confused with final production audio. The softer-upright experiment changes cloned render-event voice IDs only and is explicitly noncanonical.

`tools/` preserves authoring and reference-render wrappers. Canonical JSON/PCS1 and checked-in evidence supersede historical generation scripts: do not regenerate over canonical files in place. Work on a copy for new experiments. Several diagnostics expect regenerated WAV inputs; their absence in this storage-efficient archive is intentional. Run `python3 tools/verify-archive.py` to verify the archived JSON/PCS1 pairs, FLAC PCM hashes, full-track MP3 hashes and complete file inventory without any engine dependency. It requires Python 3 and ffmpeg.

`archive-sha256.json` covers every archived file except itself. Paths in evidence have been made archive-relative; musical source and audio bytes are unchanged. No credentials, account metadata, Library identifiers or third-party engine implementation are included.
