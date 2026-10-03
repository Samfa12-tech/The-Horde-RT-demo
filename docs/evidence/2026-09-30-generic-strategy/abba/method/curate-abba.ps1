param([switch]$Finalize)
$ErrorActionPreference='Stop'
$sourceRoot='C:/Dev/tmp/horde-generic-route-profile-20260930'
$repo='C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.6.1-engineering-pass'
$target=Join-Path $repo 'docs/evidence/2026-09-30-generic-strategy/abba'
$ids=@('generic-route-control-a1-20260930','generic-route-profile-b1-20260930','generic-route-profile-b2-20260930','generic-route-control-a2-20260930')
$comparison=Get-Content "$sourceRoot/comparison.json" -Raw | ConvertFrom-Json
if($comparison.fullRouteRows -ne 7352 -or $comparison.openingRows -ne 640 -or
   $comparison.optimizationAcceptance -cne 'none' -or $comparison.causalConclusion -cne 'none') {throw 'Wrong frozen comparison'}
if($Finalize) {
    if(Test-Path "$target/SHA256SUMS.json") {throw 'Never overwrite frozen manifest'}
    $files=@(Get-ChildItem -LiteralPath $target -Recurse -File | Sort-Object FullName)
    if(-not (Test-Path "$target/README.md")) {throw 'Lead README required before freezing'}
    $rows=foreach($file in $files) {
        $relative=[IO.Path]::GetRelativePath($target,$file.FullName).Replace('\','/')
        $row=[ordered]@{path=$relative;bytes=$file.Length;sha256=(Get-FileHash $file.FullName).Hash.ToLowerInvariant()}
        if($relative.EndsWith('.gz')) {
            $original=Join-Path $sourceRoot ($relative -replace '^trials/','phone/' -replace '/benchmark.json.gz$',('/'+($relative.Split('/')[1])+'/benchmark.json'))
            if(-not (Test-Path $original)) {throw "Missing compressed source $original"}
            $row['compressed']=$true
            $row['sourceBytes']=(Get-Item $original).Length
            $row['sourceSha256']=(Get-FileHash $original).Hash.ToLowerInvariant()
        }
        $row
    }
    [ordered]@{schema=1;sourceCommit=$comparison.sourceCommit;evidenceClass='investigation-only-whole-strategy';files=@($rows)} |
        ConvertTo-Json -Depth 8 | Set-Content "$target/SHA256SUMS.json" -Encoding utf8NoBOM
    Write-Output "Frozen $($rows.Count) evidence records"
    return
}
if(Test-Path $target) {throw 'New evidence target must not exist'}
New-Item -ItemType Directory $target | Out-Null
function CopyExact([string]$inputRelative,[string]$outputRelative) {
    $inputFile=Join-Path $sourceRoot $inputRelative
    $outputFile=Join-Path $target $outputRelative
    if(-not (Test-Path $inputFile -PathType Leaf) -or (Test-Path $outputFile)) {throw 'Missing input or occupied output'}
    New-Item -ItemType Directory ([IO.Path]::GetDirectoryName($outputFile)) -Force | Out-Null
    Copy-Item -LiteralPath $inputFile -Destination $outputFile
    if((Get-FileHash $inputFile).Hash -cne (Get-FileHash $outputFile).Hash) {throw 'Byte copy changed'}
}
CopyExact 'comparison.json' 'comparison.json'
CopyExact 'curate-abba.ps1' 'method/curate-abba.ps1'
foreach($id in $ids) {
    $analysis=Get-Content "$sourceRoot/phone/$id/analysis.json" -Raw | ConvertFrom-Json
    if($analysis.integrity -cne 'PASS' -or $analysis.runId -cne $id) {throw "Non-admitted trial $id"}
    foreach($file in @('analysis.json','trial.json','context-samples.jsonl','launch.log','pull.log')) {
        CopyExact "phone/$id/$file" "trials/$id/$file"
    }
    CopyExact "phone/$id/$id/result.json" "trials/$id/result.json"
    foreach($file in @('receipt.json','install.log','pull.log')) {CopyExact "installs/$id/$file" "installs/$id/$file"}
    $input=[IO.File]::OpenRead("$sourceRoot/phone/$id/$id/benchmark.json")
    $output=[IO.File]::Create("$target/trials/$id/benchmark.json.gz")
    $gzip=[IO.Compression.GZipStream]::new($output,[IO.Compression.CompressionLevel]::Optimal,$true)
    try {$input.CopyTo($gzip)} finally {$gzip.Dispose();$output.Dispose();$input.Dispose()}
}
foreach($file in @('push-run.log','push-run.json','push-jobs-api.json','pr-run.log','pr-run.json','pr-jobs-api.json','ci-evidence-summary.json')) {
    CopyExact "current-head-ci/$file" "ci/$file"
}
Write-Output 'Curated complete ABBA and current-head CI; add lead README then freeze manifest'
