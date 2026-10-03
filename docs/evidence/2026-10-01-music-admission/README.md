# Runtime music admission (not playback acceptance)

Owner-supplied; authorised for Horde use only. Engineering base
`0db0d0b2b47e6c208911dab8b8129793ed7918cc`; authorised edits over that base,
not a pristine-base artifact. Accepted player/RT/gameplay/music score and
sixteen prototype WAVs are unchanged. No source-tool application code is copied.

## Actual checks

- Canonical ZIP rehashed; exact JSON72,237B/PCS1 93,846B imported verbatim.
  Parsed JSON/PCS1 agreement independently confirmed. Eighteen source/runtime
  file hashes agree with the reviewed manifest and original rendering receipt.
- Manifest pin `1126f9f537efb607b11bd492e1c79d6e8b94814567ce06b654e03b0d915c9ff3`;
  sixteen stereo48kHz PCM16 body/tail WAVs total20,160,704 bytes, decoded
  immutable PCM20,160,000. C/G are one-shots; historical loop filenames become
  body without altering samples, cuts, tails, gain or effect rate.
- Lead MSVC Debug5/5 in8.26s and Release5/5 in7.26s: resolver, stream,
  actual-runtime WAV decode, asset admission and generic Core PCM contracts.
  Asset fixture is14/14 (three positives/eleven negatives), including equal-count
  duplicate ZIP rejection. Fixtures are independent copies, never source hardlinks.
  Linux-compatible temp/path handling was corrected during lead review; fresh
  compiler CI is required after pushing the checkpoint.
- Real Windows RT executables compile/link Debug and Release. Windows stage/ZIP
  music contract is tested, not a full new release ZIP, RT capture or listening run.
- Android `prepareRuntimeAssets` passes; Debug builds all four ABIs. Actual
  resolver-selected universal APK109,517,702B SHA-256
  `e7f7c92e898ca7ea9dd56f0eaadce780168151bddeac55114c3ca169a223d44f`.
  Its exact17 music entries match source; canonical/metadata files are excluded.
  Current ASSET_LICENSES is packaged byte-exact. All52 prior render/SFX assets
  match accepted DebugC11; the old53 count includes the attribution file, which
  intentionally changes. The verifier's initial incorrect53 expectation is
  retained separately and fixed, not disguised as an asset regression.
- Actual ARM64 native SHA-256
  `13b951d26230da67b1d82e41ccbcfd21a783ab3e3eb1bfbfb25d66c0778f0763`;
  all four packaged Diagnostic/Mobile pipeline/compute SPIR-V modules freshly
  validate/disassemble and match C11 identities exactly. Not Shipping/High proof.

No phone install or PCM output backend. No native consumed-clock/seam/drift,
SFX mix, lifecycle/listening, device presentation or performance acceptance.
The phone is unavailable per owner; this independent work requires no phone.
Audio/haptic manual revalidation:NO while unwired; YES for later audible mixing.
Physical glass, Mobile performance/backend parity and final release gates remain open.

## Retained scope

`manifest.json` binds selected logs/receipt/method; no APK/native binaries or
duplicate WAVs are archived here. Runtime assets and canonical source have their
own closed manifest. The historical verification method uses exact externalC11
and worktree paths; it does not make the omitted APK bytes portable evidence.
The source/runtime/archive policy and negative fixtures run from repository tools.
Rebuilt artifacts require fresh hashes; do not assign this APK's identity to them.
No publication, signing recovery or general source/score relicensing authorised.
