# MSVC CI selection checkpoint — 2026-10-23

This checkpoint removes duplicated compiler-independent checks from the Windows
compiler lane while preserving its native and platform coverage. It changes only
the [Windows CTest selector](../../../tools/run-windows-host-ci-tests.ps1), the
[host workflow](../../../.github/workflows/shared-simulation-host.yml), the
[validation guide](../../AGENT_VALIDATION.md) and this record. It contains no game,
shader, asset, compiler or render-quality change.

## Retained hosted results

Both recorded runs used the previous selector with overlapping asset-policy and
sequential CTest processes. These terminal results precede this selection change.

| Run | Exact source | MSVC build step | MSVC selection step | Terminal result |
| --- | --- | ---: | ---: | --- |
| [39933824808](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39933824808) | `8937eed5fd9104527bd5c8b9566dd54d8334836b` | Passed, 606 s | Cancelled after 202 s | GCC, Clang, sanitizers and Vulkan CPU-host passed; Android Debug failed; MSVC cancelled |
| [39936408455](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39936408455) | `b08aef6542b961ba6e5bd7304ff520421937e680` | Passed, 618 s | Cancelled after 204 s | Five jobs passed, including Android Debug; MSVC cancelled |

The 606/618-second native builds dominate these budgets and remain unchanged.
The retained first-run MSVC log shows the native report UI test passed in 1.05 s;
it does not establish a report-form hang. Cancellation followed the start of
`horde_rt_music_asset_admission_tests`, without its result. Four provider-object
evidence wrappers passed in 23.17, 23.75, 26.19 and 23.65 s (96.76 s total) and
remain selected. Eight completed portable checks totalled 29.08 s. Later portable
checks and the overlapping asset-policy result were not reported before that
cancellation, so the total time saved by this change has not been measured.

The locally retained inputs are ignored reports, not additional committed files:

| Local input | SHA-256 |
| --- | --- |
| [prior-jobs.json](../../../reports/msvc-ci-budget-20261023/prior-jobs.json) | `5272879640bca244ab2d33764032564561191aa42f7bee8e5753788608eee1db` |
| [prior-msvc.log](../../../reports/msvc-ci-budget-20261023/prior-msvc.log) | `8318c7b986564369ca56e9c0037ffb899ba330090e02187fe0fc4b48bb2d799d` |
| [b08-terminal-jobs.json](../../../reports/msvc-ci-budget-20261023/b08-terminal-jobs.json) | `14e5582916808c237ee73e96bb00ba3012e32302f4413367467975615313aa3d` |

## Selection and existing alternate coverage

GCC and Clang keep their complete Vulkan-disabled CTest selections, without an
exclusion filter. Both selections passed in the two retained source runs above.
The following exact names are omitted only by the Windows CI helper; their CMake
registrations and local/full Windows validation remain available.

| Exact CTest name | Owning script (`tests/` unless prefixed) |
| --- | --- |
| `horde_rt_third_party_notice_admission_tests` | `ThirdPartyNoticeAdmissionTests.ps1` |
| `horde_rt_raygen_artifact_ownership_tests` | `RaygenArtifactOwnershipTests.ps1` |
| `horde_rt_player_manifest_agreement_tests` | `PlayerManifestAgreementTests.ps1` |
| `horde_rt_player_package_inventory_tests` | `PlayerPackageInventoryTests.ps1` |
| `horde_rt_forest_runtime_roster_tests` | `ForestRuntimeRosterTests.py` |
| `horde_rt_forest_texture_source_tests` | `ForestTextureSourceTests.py` |
| `horde_rt_forest_placement_source_tests` | `tools/build-forest-placement-contract.py` |
| `horde_rt_forest_root_placement_tests` | `tools/fit-forest-root-placements.py` |
| `horde_rt_music_asset_admission_tests` | `MusicAssetAdmissionTests.ps1` |
| `horde_rt_horde_162_asset_policy_tests` | `Horde162AssetPolicyTests.ps1` |
| `horde_rt_android_showcase_witness_policy_tests` | `AndroidShowcaseWitnessPolicyTests.ps1` |
| `horde_rt_pocket_audio_core_pin_tests` | `PocketAudioCorePinTests.ps1` |
| `horde_rt_region_page_build_policy_tests` | `test_region_page_build_policy.py` |
| `horde_rt_chapter_host_phase_analysis_tests` | `test_chapter_host_phases.py` |
| `horde_rt_corridor_detail_certificate_tests` | `test_corridor_detail_certificate.py` |
| `horde_rt_rescue_source_admission_tests` | `RescueBlockoutSourceAdmissionTests.py` |
| `horde_rt_water_contact_audio_tests` | `test_water_contact_audio.py` |
| `horde_rt_chapter_local_audio_tests` | `test_chapter_local_audio.py` |
| `horde_rt_keeper_death_audio_tests` | `test_keeper_death_audio.py` |
| `horde_rt_kit_audio_tests` | `test_kit_audio.py` |
| `horde_rt_chapter_texture_alias_payload_tests` | `test_chapter_texture_aliases.py` |

These registrations are in `CMakeLists.txt` at the notice/player admission,
forest-source, audio/policy and final Python-policy groups. Core pin admission
uses `cmake -P`; region-page admission uses `project(RegionPolicy NONE)`. Neither
compiles a native target. The other checks inspect source, manifests, encoded
assets, provenance or synthetic archive/policy fixtures.

Five post-check steps retain both Linux copies and now skip Windows: gauntlet
chirality/UV winding, sleeve seam transfers, body partition/head mask, segmented
seam plans and Android combat capture expectations. The Gradle APK resolver keeps
its Windows run, including host filesystem/path behavior.

Every native C++ executable remains selected. This includes asset/source-named
native tests, Windows OS/UI/report fixtures and all four
`horde_rt_provider_*_fixture_evidence` wrappers. Those wrappers execute the built
provider fixture and inspect its actual compiler-produced object bytes, so Linux
coverage does not substitute for MSVC evidence. Windows presentation contracts
also remain. The existing 15 SDK/Android/tool exclusions are unchanged.

The helper first discovers the unfiltered roster and requires each of the 21
portable names exactly once. It partitions that roster using exact portable
names plus the existing tool filter, rejects an empty remainder, and compares
CTest's actual regex result with the intended selection. New registrations enter
by default. A single test-running CTest process owns the sequential run and
`LastTest.log`; the asset-policy overlap is removed. `--no-tests=error`, the
15-minute cap, workflow permissions, compiler matrix, build commands and settings
are preserved.

## Local validation of the selection change

Validation used base `b08aef6542b961ba6e5bd7304ff520421937e680` plus the two modified
selector/workflow files. Their normalized Git blob identities at validation were
`e6343ac0d874f4b11e5546ffa6dbe612a754e02d` (selector) and
`365b17c414a9109e8d68e725e0384956a0502ccc` (workflow).

- PowerShell parser: passed.
- `-DryRun` against the existing `build/presets/windows-x64-debug` configuration:
  231 registered, 195 selected, 21 portable and 15 tool exclusions; all 181 native
  executable tests, all four provider-object wrappers and the native report UI
  fixture retained. This local configuration enables Vulkan targets; its counts
  are not the Vulkan-disabled hosted lane's expected counts.
- An ephemeral synthetic CTest harness passed 8/8 cases: complete partition with
  a future native test; missing portable registration; duplicate portable
  registration; empty remainder; filtered-roster mismatch; listing failure;
  exactly one successful test-running owner with fail-on-empty; execution-failure
  propagation. Dry runs executed no fixtures. This was recorded in the local
  task output; no separate harness report was retained under `reports/`.
- Exact full-file workflow comparison passed: only the intended comments, step
  rename and five Windows-skip conditions changed. Other commands, matrix,
  timeout, settings, permissions and jobs were preserved.
- `git diff --check`: passed.

No full native suite or broad build was rerun for this selection-only change.
These checks validate roster preservation and helper behavior, not hosted
completion or performance. The changed candidate has no terminal CI result at
this checkpoint. Lead review, backup, commit/push and terminal changed-head CI
remain outstanding. No network or phone operation was performed for this work.

Audio/haptic manual revalidation required: **NO** — no gameplay, event transport,
playback or haptic semantics changed.
