# Bass review: result and remaining work

## Useful result

A 74-second same-notes comparison and ten alternative full-context excerpts are ready. They use the actual Core renderer's existing Soft Upright voice, with no independent normalization and no changes to the musical events or existing audio.

- A/B listening file: `previews/bass-voice-alternative/Bass-Voice-Comparison-Classic-vs-Soft-Upright.mp3`
- Full alternative tour: `previews/The-Horde-Listening-Tour-Soft-Upright-Alternative.mp3`
- Exact-change evidence and chapter times: `previews/bass-voice-alternative/bass-voice-alternative-manifest.json`
- Detailed source audit: `validation/bass-audit/BASS-RENDERER-AUDIT.md`

The A/B alternates Classic then Soft Upright for main-theme bass, main-theme full context, Bellwether bass, and Bellwether full context. Each pair retains its existing bank gain. The Soft Upright recipe is naturally somewhat quieter; that difference has not been hidden by normalization.

## Diagnosis

The measured bass notes are correctly pitched. Classic Core bass is an unfiltered, unbandlimited sawtooth plus an octave-lower sine. The current app instead applies the recipe's 420 Hz main and 220 Hz sub low-pass filters, with a different sustain/release envelope. Core also ignores the score's beat-volume setting in these bass tests: changing it 100-fold leaves identical PCM. The generic Core envelope has a 13.9% discontinuity at its attack junction.

These are renderer differences, not evidence that the bass should be transposed. The separate melody-octave authoring correction is not a bass fix.

## What the alternative establishes

Only the cloned bass events' `bassTone` changes to `soft_upright`. Every other event field is hash-identical, and every reconstructed Classic full-context WAV matches the previously delivered section WAV byte for byte. No canonical configuration fix is claimed: Standard-profile normalization currently discards the raw bassTone setting, so this explicitly approved alternative uses a render-only event override.

For the first main-theme and Bellwether bass notes, energy above 3 kHz falls from approximately 1.34%/1.78% with Classic to 0.00026%/0.00055% with Soft Upright. These are spectral-energy measurements, not isolated alias-distortion values. Both are real Core voices; neither alternate is a render of the app's filtered chain.

All ten alternate context clips are unclipped and contain finite samples. The A/B MP3 decodes successfully and measures -3.1 dBFS true peak. The unnormalized, fixed-gain entity excerpt peaks at -2.38 dBFS in PCM and remains unclipped.

## What is still unverified

An actual audible Core-versus-app filtered-chain A/B remains blocked by the verified cloud-browser runtime restrictions. No blocked browser attempt was repeated or bypassed. The next faithful engine test should use unchanged notes/events through the actual app bass functions, recording filters, envelopes, mixer gains, and compressor behavior. Any shared-engine fix must preserve pitch/timing and pass mixing, bandlimiting, transient, loop-boundary, and listening tests.

This material is an alternate listening direction, not a production-mastered or app-parity-approved replacement.

## Provenance

Pocket Chordsmith source commit: `0c6975cadcd4aba0047ab94170a2adabc722517f`. Tomb B's comparison metadata was refreshed after the Tomb C octave declaration was corrected to its already-rendered value; all Tomb B bass/full events remain identical and no diagnostic audio was rewritten. See `validation/bass-audit/tomb-provenance-refresh.json`.
