# Immutable native music bank (not playback acceptance)

Engineering base199697bf52b811e7f45ab18476093485166feff0 with authorised bank
edits, not a pristine-base artifact. Canonical score, sixteen admitted WAVs,
Core534a6e6 pin, simulation/SFX transport and accepted player/RT work unchanged.

## Change and evidence

- `MusicPcmAssetBank` owns exact20,160,000B decoded immutable PCM. Sixteen bounded
  reads before playback, Core-only decoding, no partial spans on failure, no
  ready-bank reload/move/copy. Audio must stop/join before bank destruction.
- Actual-WAV tests verify all slots/frames/None, stable borrowed addresses,
  real nonzero finite Core output with no further reads, and six injected late
  failures atG-tail after thirteen successful clips, with clean retry. They use
  nonmodal failures and correctly sized output storage.
- Fresh MSVC Debug6/6 in13.64s and Release6/6 in8.99s: resolver, stream, real WAV,
  bank, closed asset admission and Core contracts. Real Windows RT executables
  compile/link both. Not a new RT capture or Windows output/listening pass.
- Focused external MSVC ASan bank test exit0/no sanitizer diagnostics. Final
  `/fsanitize=address /Zi /EHsc`, Debug linker `/debug /INCREMENTAL:NO`, installed
  MSVC ASan runtime on processPATH. First build's LNK4300 incremental-link warning
  and passing test retained separately; corrected final build has no warnings.
- Android Debug builds all four ABIs. Actual universal APK113,350,863B SHA-256
  `450d2cf6a331bc482302a81a27339f812d6f7a44c29772e6f5b151d04dfb3f2d`.
  Fresh package verification identifies that exact APK/native entry, exact17
  music entries, current licence and52 unchanged render/SFX assets againstC11.
  Sources excluded. Actual ARM64 native3,719,384B SHA-256
  `5b122656de753abaac94f7ceeff872e3d964bbeb2c17dea95cca78f59827619d`.
  Four actual Diagnostic/Mobile pipeline/compute modules freshly val/dis PASS;
  all four identities equalC11. Not Shipping/High, RT device or output acceptance.

No phone actions, installation, platform reader/output, consumed-clock, seam/drift,
mix/lifecycle/listening or performance claim. Audio/haptic manual revalidation:NO
while unwired; YES for audible integration. Independent volume and backend work
remain open, as do physical glass, Mobile architecture/parity and final gates.

## Retained scope

Manifest binds selected logs, receipt, verification method and this description.
No APK/native/test binaries or duplicate WAVs are archived. Exact external paths
in the method identify historical artifacts; omitted bytes remain receipt claims.
Rebuilt APKs require new hashes. ASan executable SHA-256
`811f858e13f521c5ec918afbe71d7e7c568c7a43a3b2654786d3f4cccc8e4252`
identifies the retained external executable, not an archived portable binary.
Bank source is identified by the containing reviewed commit. Current-source
compiler CI is required separately; older199697b passes do not validate new bank.
No publication, signing recovery or licence changes authorised.
