param([Parameter(Mandatory=$true)][string]$Destination,[switch]$ValidateOnly)
$ErrorActionPreference='Stop'
$target=[IO.Path]::GetFullPath($Destination)
if(Test-Path -LiteralPath $target){throw 'Refuse to overwrite curated evidence'}
$prefix='C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass\docs\evidence\'
if(-not $target.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Target must remain within named engineering evidence directory'}
if(-not $ValidateOnly){New-Item -ItemType Directory -Path $target | Out-Null}
$manifest=[Collections.Generic.List[object]]::new()
function Retain([string]$relative,[switch]$Compress){
    $source=Join-Path $PSScriptRoot $relative
    if(-not (Test-Path -LiteralPath $source -PathType Leaf)){throw "Missing exact allowed evidence: $relative"}
    if([IO.Path]::GetExtension($source) -notin @('.json','.jsonl','.txt','.log','.md','.ps1','.py','.patch')){throw 'Binary/application media cannot be curated by this allowlist'}
    if($ValidateOnly){return}
    $outputRelative=if($Compress){"$relative.gz"}else{$relative}
    $output=Join-Path $target $outputRelative
    New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force | Out-Null
    if($Compress){
        $inputStream=[IO.File]::OpenRead($source)
        $outputStream=[IO.File]::Create($output)
        $zip=[IO.Compression.GZipStream]::new($outputStream,[IO.Compression.CompressionLevel]::Optimal)
        try{$inputStream.CopyTo($zip)}finally{$zip.Dispose();$outputStream.Dispose();$inputStream.Dispose()}
        # Verify retained report bytes by decompression, without editing originals.
        $compressed=[IO.File]::OpenRead($output)
        $decoder=[IO.Compression.GZipStream]::new($compressed,[IO.Compression.CompressionMode]::Decompress)
        $sha=[Security.Cryptography.SHA256]::Create()
        try{$decodedHash=[Convert]::ToHexString($sha.ComputeHash($decoder)).ToLowerInvariant()}finally{$sha.Dispose();$decoder.Dispose();$compressed.Dispose()}
        if($decodedHash -cne (Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()){throw 'Retained report roundtrip mismatch'}
    }else{Copy-Item -LiteralPath $source -Destination $output}
    $manifest.Add([ordered]@{path=$outputRelative.Replace('\','/');sha256=(Get-FileHash -LiteralPath $output).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $output).Length;sourceSha256=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant();sourceBytes=(Get-Item -LiteralPath $source).Length;compressed=[bool]$Compress})
}
$runs=@('b0-route','b0-high','a1-route','a1-high','a1-live','b1-route','b1-high','b1-live','b2-route','b2-high','b2-live','a2-route','a2-high','a2-live','active-strategy-route','opening-gpu-stages-v2-route','opening-gpu-stages-v3-route')
foreach($run in $runs){
    $id="shipping-$run-20260930"
    $root="phone/$id"
    foreach($file in @('trial.json','analysis.json','context-samples.jsonl','launch.log','pull.log')){Retain "$root/$file"}
    foreach($file in @('result.json','benchmark.txt')){Retain "$root/$id/$file"}
    Retain "$root/$id/benchmark.json" -Compress
}
Retain 'phone/shipping-opening-gpu-stages-v3-route-20260930/stage-analysis.json'
Retain 'phone/shipping-opening-gpu-stages-v3-route-20260930/joined-stage-rows.json' -Compress
foreach($install in @('a1-20260930','b1-20260930','b2-20260930','a2-20260930','active-strategy-20260930','opening-gpu-stage-v2-20260930','opening-gpu-stage-v3-20260930')){
    foreach($file in @('receipt.json','install.log','pull.log')){Retain "installs/$install/$file"}
}
foreach($file in @('PROTOCOL.md','ab-summary.json','run-shipping-trial.ps1','run-pair-block.ps1','install-pair-member.ps1','install-followup-artifact.ps1','analyse-trial.ps1','analyse-opening-stages.ps1','run-opening-stage-probe.ps1','curate-evidence.ps1','baseline/arm64-containment.json','baseline/gradle-build.log','candidate/apk-badging.txt','candidate/containment.json','candidate/build-02.log','candidate/development-certificate.txt','asset-provenance/compare-apk-assets-and-shaders.ps1','asset-provenance/apk-asset-and-shader-comparison.json','active-strategy/artifact.json','active-strategy/containment.json','active-strategy/certificate.txt','active-strategy/badging.txt','active-strategy-android-build.log','active-strategy-green-build.log','active-strategy-release-build.log','active-strategy-release-ctest.log','18616f4-push-ci.log','18616f4-pr-ci.log','opening-stage-proposal/PROPOSAL.md','opening-stage-proposal/REVIEW.md','opening-stage-proposal/REVIEW_DISPOSITION.md','opening-stage-proposal/artifact-v2.json','opening-stage-proposal/artifact-v3.json','opening-stage-proposal/containment-v2.json','opening-stage-proposal/containment-v3.json','opening-stage-proposal/investigation-v2.patch','opening-stage-proposal/investigation-v3.patch','opening-stage-proposal/build-v2.log','opening-stage-proposal/build-v3.log','opening-stage-proposal/host-ctest.log')){Retain $file}
# Never curate broad logcat/media directories. Keep only the bounded probe marker lines.
foreach($log in @('phone-stage-logcat.txt','phone-stage-logcat-v3.txt')){
    if($ValidateOnly){if(-not (Test-Path (Join-Path $PSScriptRoot "opening-stage-proposal/$log"))){throw 'Missing bounded probe log'};continue}
    $lines=@(Get-Content (Join-Path $PSScriptRoot "opening-stage-proposal/$log") | Where-Object {$_ -match 'OPENING_GPU_STAGE'})
    $out=Join-Path $target "opening-stage-proposal/$log"
    [IO.File]::WriteAllLines($out,$lines,[Text.UTF8Encoding]::new($false))
    $manifest.Add([ordered]@{path="opening-stage-proposal/$log";sha256=(Get-FileHash -LiteralPath $out).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $out).Length;filteredToProbeMarkers=$true})
}
if($ValidateOnly){Write-Output 'Exact evidence allowlist preflight PASS'}else{
    $manifest | ConvertTo-Json -Depth 5 > (Join-Path $target 'SHA256SUMS.json')
    Write-Output "Curated $($manifest.Count) exact allowed files, preserving complete reports through verified gzip roundtrips."
}
