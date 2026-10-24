# Bounded campaign journey tests with same-run earned saves

The former single Debug registration exceeded its 30-second budget after gaining full forest recovery/return coverage; the retained 2626d273 evidence completed with 145 checks in about 37 seconds. This checkpoint keeps the same journey and assertions but gives the already fresh-loaded endpoint return its own registration. Every registration still has TIMEOUT=30. No runtime, gameplay, quality, workflow cap or permission change.

## Coverage and lifecycle

The producer still earns Keeper victory, reward, lower manual/auto, ascent, interrupted upper descent/recovery, the complete forest progression and endpoint publication through the original inputs. It retains exploration/no repeated save, stale readiness, headroom, original reset/facade reset and write-failure assertions. Critically, it ALSO opens, checks for 120 quiet ticks and destroys a fresh endpoint PlaySession while the original journey session remains alive. Splitting the heavy return must not drop that in-process lifecycle check.

The consumer is a different process, loading only the exact endpoint auto and lower manual bytes that this invocation's successful producer copied. It repeats fresh load/input rebasing/quiet checks, walks the entire reverse trail, descends and lands, and checks completed Kit/story, no record rewrite and untouched unselected slots. The original 120/2400/1800/60 tick bounds, walking inputs, tether and collision are unchanged.

All 118 original Check call expressions remain textually present after whitespace normalization; a bounded read-only audit also checked state/loop ownership. Original no-argument Release behavior still passes 145 checks. Registered producer passes 144 and consumer 15: the combined159 includes 11 deliberately repeated endpoint lifecycle checks, one repeated post-return slot check and two new handoff guards. None of the original assertions was skipped.

## Fixture ownership

A generated CTest include chooses a fresh 128-bit-shaped random hex nonce at EACH CTest invocation, including filtered/rerun selections. Setup/consumer/cleanup inherit that same nonce. Build/configuration-specific roots and create-only ownership prevent cross-run/configuration fallback; the receipt binds nonce, compiled build stamp, sizes and FNV integrity hashes to the actual earned record bytes. These hashes are integrity checks, not a security/authentication claim. Ready publication occurs only after all producer assertions succeed. Claim-once consumption rejects replay. Cleanup removes only known regular files inside its validated owned root and refuses unknown files/symlinks.

CTest FIXTURES_SETUP/REQUIRED/CLEANUP ensures consumer-only selection includes earning and cleanup, setup failure skips the consumer, and cleanup still executes. The existing explicit Vulkan-host roster adds the new return registration; portable/MSVC selections already include new native registrations. Build target and workflow structure/caps/permissions remain unchanged.

## Actual validation

- Final relevant Debug selection:8/8 pass,75.24s total; producer23.75s, return16.29s, cleanup0.02s, each under its unchanged30s deadline. The preliminary isolated Debug selection also passed3/3 (25.16s/19.39s), before the caller-config preservation correction.
- Final relevant Release selection:8/8 pass,10.36s total. Final consumer-only Release selection automatically adds producer/cleanup and passes3/3. Original unpartitioned Release entry point passes145/145.
-14 isolated guard cases pass using actual earned bytes: missing/stale setup, changed receipt/payload/build, cross-run receipt, cross-configuration access, valid consumer, replay, failure dependency/cleanup, foreign-file refusal and normal cleanup. The retained failed-setup log is an EXPECTED failure witness: earning fails on an occupied stale root, recovery is Not Run, cleanup succeeds. All owned handoff save roots were removed. No owner saves/settings used.

This reorganizes per-test budgets; it does not claim less total test work or a game performance improvement. Hosted exact-head CI is reported separately after the backup. Cold/loaded runner variance remains subject to the unchanged gates.

Casual/urgent context selection is still pending. Authored .75s data and the global .50s playback timing remain unchanged; retained timing data is not contextual behavior. Phone/foreground/release acceptance is untouched. Audio/haptic manual revalidation required: NO (test organization only). All151 parked owner files remain unchanged. No phone, merge, release or deployment.

Reproduction from the active repository root after building the affected targets:

```powershell
ctest --test-dir build/presets/windows-x64-debug -C Debug -R '^horde_rt_campaign_earned_return_tests$' -V
ctest --test-dir build/presets/windows-x64-release -C Release -R '^horde_rt_campaign_earned_return_tests$' -V
python docs/evidence/2026-10-24-campaign-journey-partition/check_fixture_guards.py
```

The optional guard driver needs both configured Debug and Release binaries. Its expected negative paths must not be mistaken for runtime/gameplay failures.

The failed-setup log copy removes only CTest trailing whitespace; receipt.json retains its original report hash. The original failed witness remains unchanged under reports.
