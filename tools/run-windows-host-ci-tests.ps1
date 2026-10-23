param([Parameter(Mandatory=$true)][string]$BuildDirectory,
      [string]$Configuration='Release', [string]$ExcludePattern='', [switch]$DryRun)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$ctest=(Get-Command ctest -ErrorAction Stop).Source
$common=@('--test-dir',$BuildDirectory,'--build-config',$Configuration,'--output-on-failure','--no-tests=error')
# These compiler-independent checks remain in both complete Linux CTest lanes.
# Keep exact names: native asset tests and provider-object evidence still belong
# to MSVC, and new native registrations must enter this selection by default.
$portable=@(
    'horde_rt_third_party_notice_admission_tests',
    'horde_rt_raygen_artifact_ownership_tests',
    'horde_rt_player_manifest_agreement_tests',
    'horde_rt_player_package_inventory_tests',
    'horde_rt_forest_runtime_roster_tests',
    'horde_rt_forest_texture_source_tests',
    'horde_rt_forest_placement_source_tests',
    'horde_rt_forest_root_placement_tests',
    'horde_rt_music_asset_admission_tests',
    'horde_rt_horde_162_asset_policy_tests',
    'horde_rt_android_showcase_witness_policy_tests',
    'horde_rt_pocket_audio_core_pin_tests',
    'horde_rt_region_page_build_policy_tests',
    'horde_rt_chapter_host_phase_analysis_tests',
    'horde_rt_corridor_detail_certificate_tests',
    'horde_rt_rescue_source_admission_tests',
    'horde_rt_water_contact_audio_tests',
    'horde_rt_chapter_local_audio_tests',
    'horde_rt_keeper_death_audio_tests',
    'horde_rt_kit_audio_tests',
    'horde_rt_chapter_texture_alias_payload_tests'
)
$listing=@('--test-dir',$BuildDirectory,'--build-config',$Configuration,'--show-only=json-v1')
$json=& $ctest @listing
if($LASTEXITCODE -ne 0){throw 'Could not inspect the complete Windows CTest roster.'}
$allNames=@(($json | ConvertFrom-Json).tests | ForEach-Object name)
foreach($name in $portable) {
    if(@($allNames | Where-Object {$_ -ceq $name}).Count -ne 1) {
        throw "Portable exclusion must occur exactly once in the complete roster: $name"
    }
}
$toolExcluded=@($allNames | Where-Object {$ExcludePattern -and $_ -cmatch $ExcludePattern})
$selected=@($allNames | Where-Object {$_ -cnotin $portable -and $_ -cnotin $toolExcluded})
if($selected.Count -eq 0){throw 'Remaining Windows CTest selection is empty.'}
# CTest supports ordinary groups, not PCRE (?:...) groups. Verify its actual
# regex result against the explicit partition before executing any fixture.
$portablePattern='^('+ (($portable | ForEach-Object {[regex]::Escape($_)}) -join '|') + ')$'
$combinedPattern=if($ExcludePattern){"($ExcludePattern)|$portablePattern"}else{$portablePattern}
$json=& $ctest @listing -E $combinedPattern
if($LASTEXITCODE -ne 0){throw 'Could not inspect the filtered Windows CTest selection.'}
$actual=@(($json | ConvertFrom-Json).tests | ForEach-Object name)
if($actual.Count -ne $selected.Count -or
   @(Compare-Object ($selected | Sort-Object -CaseSensitive) ($actual | Sort-Object -CaseSensitive) -CaseSensitive).Count) {
    throw 'CTest filtering changed the complete intended Windows selection.'
}
if($DryRun) {
    return [pscustomobject]@{
        RegisteredCount=$allNames.Count
        SelectedCount=$selected.Count
        PortableExcludedTests=$portable
        ToolExcludedTests=$toolExcluded
        SelectedTests=$selected
    }
}
Write-Output "Windows selection: $($selected.Count) tests; $($portable.Count) portable checks run in both Linux lanes; $($toolExcluded.Count) tool checks excluded."
# One test-running CTest process owns the sequential selection and LastTest.log.
& $ctest @common -E $combinedPattern
if($LASTEXITCODE -ne 0){throw "Windows CTest failure: exit=$LASTEXITCODE."}
