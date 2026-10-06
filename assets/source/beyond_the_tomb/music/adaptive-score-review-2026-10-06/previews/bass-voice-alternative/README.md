# Bass voice alternative

This is a separately labelled listening alternative using Core's existing `soft_upright` bass voice. It is not the current app's filtered bass renderer, a new synth, a canonical score edit, or an accepted final sound.

Start with `Bass-Voice-Comparison-Classic-vs-Soft-Upright.mp3` (74 seconds). The eight chapters compare:

1. Main theme bass only, Classic
2. Main theme bass only, Soft Upright
3. Main theme full context, Classic
4. Main theme full context, Soft Upright
5. Bellwether bass only, Classic
6. Bellwether bass only, Soft Upright
7. Bellwether full context, Classic
8. Bellwether full context, Soft Upright

The same existing bank gain is used for both members of each pair, including isolated stems. There is no independent normalization, EQ, or limiting. The Core recipes have different built-in waveforms and relative voice levels, so a natural level difference is part of this voice comparison. Only the montage has 10 ms editorial edge fades and 400 ms gaps.

The ten `.soft-upright-context.wav` files correspond to the ten chapters in the original listening tour, with unchanged section lengths. They are ready for assembling the same tour layout. The original section WAVs/FLACs and canonical scores remain untouched by this script.

## Exact change and proof

The Standard-profile normalizer currently ignores `raw.bassTone = 'soft_upright'` and still emits Classic bass events. Therefore the alternate makes the explicitly authorized render-only change on cloned timeline events: `event.bassTone = 'soft_upright'` for bass events only. It feeds those events into the actual, unchanged Core `renderPocketAudioEventBuffer`.

This is not a canonical configuration fix. The renderer is unchanged. Every other event property is identical, including MIDI, timing, duration, velocity, articulation, slide intent, pan, and profile data. All non-bass events are identical. `bass-voice-alternative-manifest.json` and the per-excerpt comparison files record matching event SHA256 values after excluding only bassTone. They also verify that each reconstructed Classic context WAV matches its previously delivered section WAV byte for byte.

The `soft_upright` voice uses Core's triangle-plus-sub-sine recipe instead of Classic's sawtooth-plus-sub-sine. Core still omits its recipe's low-pass filters and still has the same envelope and mixer limitations. An improved subjective result here would approve an alternate voice direction, not prove the underlying app/Core parity gap is fixed.

## Future renderer acceptance criteria

- Preserve identical note registers, starts, durations, velocities, and intended slides; no automatic transposition or composition rewrite
- Prove that Standard-profile bassTone selection survives normalization, with round-trip tests
- Honor the recipe's main/sub filters and the intended attack, sustain, and release behavior
- Apply the score's bass/beat mixer control; changing its value must change rendered bass gain
- Test oscillator bandlimiting and attack-junction continuity with measured and listening evidence
- Compare actual Core and actual app renders under recorded fixed gain and compressor settings; do not normalize individual stems to hide differences
- Recheck clipping, true peak, exact loop boundaries, all adaptive joins, and listening on the target playback devices
- Retain and label any audible recipe differences until they are deliberately accepted

The source-backed Classic bass/filter audit and the verified cloud browser blocker are documented separately under `validation/bass-audit/`. No further browser attempt was made for this alternate.
