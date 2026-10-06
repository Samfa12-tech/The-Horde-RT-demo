# Actual editor-code compatibility check

Pinned Pocket Chordsmith main0c6975cadcd4aba0047ab94170a2adabc722517f full v68 HTML was evaluated in Node24 VM. Browser DOM elements were mocked; UI rendering, audio gain application and FX update were stubbed. The actual importProject, per-section sync/sanitization, musical helpers and exportProject ran unchanged. All17projects imported successfully. Each of A-H was selected/synchronized before re-export.

Six projects retain every compared musical field; eleven change only melodyOctaves as recorded in report.json. Comparison covers notes, holds, slides, instruments, octaves, mute, solo, pan, progressions, drum grid, bars, song sequence, BPM, key, scale, chord type/instrument, bass holds/notes. Null continuation-cell hold flags survived exactly. This does not validate visual editor behavior, audio synthesis or browser import.

Actual browser verification was attempted but blocked: Chromium process creation fails socket() Operation not permitted, including the supported escalation attempt; the managed cloud browser returns ERR_BLOCKED_BY_CLIENT for localhost. Neither restriction was bypassed. No filtered app PCM pilot was produced.

Source audit: full-app schedulePlanStep reads getSectionData and compact melodyOctaves, then melodyIndexToMidi. Import clamps melodyOctaves to[-1,1]. Preserving authored rich absolute-MIDI events via compactMirror:false keeps metadata at export but does not establish faithful live app playback. Do not silently lift octaves or call the flagged scores fully editor-ready.

Reproduce source-only check: node horde-soundtrack/tools/audit-editor-node-vm.cjs

Re-export files in this directory are diagnostic derivatives only. Canonical score files were not modified.

## Optional edition verification

Subsequent approved editor-compatibility edition adds11explicitly octave-adjusted files under editor-compatible/. All11passed the same actual app-code import/export with zero compared musical-field changes. Updated report covers28inputs total:17canonical plus11optional edition files. Canonical files remain untouched. This checks preservation of the disclosed octave-adjusted edition, not equality to the lower-register canonical composition.
