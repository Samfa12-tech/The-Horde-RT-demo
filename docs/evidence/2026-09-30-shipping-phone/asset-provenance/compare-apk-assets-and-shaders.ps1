[CmdletBinding()]
param(
    [string]$BaselineApk = 'C:\Dev\tmp\horde-shipping-ab-20260930\baseline\HordeLanternRT-03c-shipping-mobile-benchmark-arm64.apk',
    [string]$CandidateApk = 'C:\Dev\tmp\horde-shipping-ab-20260930\candidate\HordeLanternRT-d63-shipping-mobile-benchmark-arm64.apk',
    [string]$BaselineRepoRoot = 'C:\Dev\tmp\horde-shipping-ab-20260930\baseline-source',
    [string]$CandidateRepoRoot = 'C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass',
    [string]$ReceiptPath = (Join-Path $PSScriptRoot 'apk-asset-and-shader-comparison.json')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Get-Sha256([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([Convert]::ToHexString($sha.ComputeHash($Bytes))).ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Get-ZipEntryMap([string]$ApkPath, [string]$Prefix) {
    $zip = [IO.Compression.ZipFile]::OpenRead($ApkPath)
    try {
        $map = [System.Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
        foreach ($entry in $zip.Entries | Where-Object {
            $_.FullName.StartsWith($Prefix, [StringComparison]::Ordinal) -and
            -not [string]::IsNullOrEmpty($_.Name)
        }) {
            $stream = $entry.Open()
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $hash = ([Convert]::ToHexString($sha.ComputeHash($stream))).ToLowerInvariant() }
            finally { $sha.Dispose(); $stream.Dispose() }
            $map[$entry.FullName] = [ordered]@{ length = [long]$entry.Length; sha256 = $hash }
        }
        return $map
    }
    finally { $zip.Dispose() }
}

function Read-ZipEntryText([string]$ApkPath, [string]$EntryName) {
    $zip = [IO.Compression.ZipFile]::OpenRead($ApkPath)
    try {
        $entry = $zip.GetEntry($EntryName)
        if ($null -eq $entry) { throw "Missing expected ZIP entry: $EntryName" }
        $stream = $entry.Open()
        $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::UTF8, $true)
        try { return $reader.ReadToEnd() }
        finally { $reader.Dispose(); $stream.Dispose() }
    }
    finally { $zip.Dispose() }
}

function Read-ZipEntryBytes([string]$ApkPath, [string]$EntryName) {
    $zip = [IO.Compression.ZipFile]::OpenRead($ApkPath)
    try {
        $entry = $zip.GetEntry($EntryName)
        if ($null -eq $entry) { throw "Missing expected ZIP entry: $EntryName" }
        $stream = $entry.Open()
        try {
            $memory = [IO.MemoryStream]::new()
            try { $stream.CopyTo($memory); return ,$memory.ToArray() }
            finally { $memory.Dispose() }
        }
        finally { $stream.Dispose() }
    }
    finally { $zip.Dispose() }
}

function ConvertTo-CanonicalJsonValue($Value) {
    if ($null -eq $Value) { return $null }
    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        $ordered = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties | Sort-Object Name -CaseSensitive) {
            $ordered[$property.Name] = ConvertTo-CanonicalJsonValue $property.Value
        }
        return $ordered
    }
    if ($Value -is [System.Collections.IDictionary]) {
        $ordered = [ordered]@{}
        foreach ($key in @($Value.Keys | Sort-Object -CaseSensitive)) {
            $ordered[[string]$key] = ConvertTo-CanonicalJsonValue $Value[$key]
        }
        return $ordered
    }
    if ($Value -is [System.Collections.IEnumerable] -and $Value -isnot [string]) {
        $items = @()
        foreach ($item in $Value) { $items += ,(ConvertTo-CanonicalJsonValue $item) }
        return ,$items
    }
    return $Value
}

function Get-TextNewlineCounts([string]$Text) {
    return [ordered]@{
        crlf = [regex]::Matches($Text, "`r`n").Count
        lfOnly = [regex]::Matches($Text, "(?<!`r)`n").Count
        crOnly = [regex]::Matches($Text, "`r(?!`n)").Count
    }
}

function Get-VariantKey([string]$Repo, [string]$Backend, [string]$ModuleSha256, [string[]]$ExpectedKeys) {
    $catalogName = if ($Backend -ceq 'RayQueryCompute') {
        'rayquery-variant-catalog.json'
    }
    elseif ($Backend -ceq 'RayTracingPipeline') {
        'raygen-variant-catalog.json'
    }
    else { throw "Unknown embedded SPIR-V backend: $Backend" }
    $catalogPath = Join-Path $Repo (Join-Path 'tools' $catalogName)
    $catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
    $rows = @($catalog.variants | Where-Object {
        $_.spirvSha256 -ceq $ModuleSha256 -and $ExpectedKeys -ccontains [string]$_.key
    })
    if ($rows.Count -ne 1) { throw "Expected one catalog row for $Backend module $ModuleSha256; found $($rows.Count)." }
    return [string]$rows[0].key
}

function Get-NormalizedShaderInstructions([string]$AssemblyPath) {
    $debugMetadataOps = @(
        'OpSourceContinued', 'OpSource', 'OpSourceExtension', 'OpName',
        'OpMemberName', 'OpString', 'OpLine', 'OpNoLine', 'OpModuleProcessed'
    )
    $idMap = [System.Collections.Generic.Dictionary[string,int]]::new([StringComparer]::Ordinal)
    $instructions = [System.Collections.Generic.List[string]]::new()
    $imageReadCount = 0
    $reader = [IO.StreamReader]::new($AssemblyPath)
    try {
        while (($line = $reader.ReadLine()) -ne $null) {
            $trimmed = $line.Trim()
            if (-not $trimmed -or $trimmed.StartsWith(';', [StringComparison]::Ordinal)) { continue }
            if ($trimmed -notmatch '^(?:%[0-9]+\s*=\s*)?(?<operation>Op[A-Za-z0-9]+)\b') {
                throw "Unrecognized SPIR-V disassembly line: $trimmed"
            }
            $operation = $Matches.operation
            if ($operation -ceq 'OpImageRead') { ++$imageReadCount }
            if ($debugMetadataOps -ccontains $operation) { continue }

            $builder = [Text.StringBuilder]::new()
            $cursor = 0
            foreach ($match in [regex]::Matches($trimmed, '%[0-9]+')) {
                [void]$builder.Append($trimmed.Substring($cursor, $match.Index - $cursor))
                $identifier = $match.Value
                if (-not $idMap.ContainsKey($identifier)) { $idMap.Add($identifier, $idMap.Count) }
                [void]$builder.Append('%v').Append($idMap[$identifier])
                $cursor = $match.Index + $match.Length
            }
            [void]$builder.Append($trimmed.Substring($cursor))
            $instructions.Add($builder.ToString())
        }
    }
    finally { $reader.Dispose() }
    $normalizedText = [string]::Join("`n", $instructions)
    $normalizedBytes = [Text.UTF8Encoding]::new($false).GetBytes($normalizedText)
    return [ordered]@{
        instructions = $instructions.ToArray()
        instructionCount = $instructions.Count
        normalizedSha256 = Get-Sha256 $normalizedBytes
        opImageReadCount = $imageReadCount
    }
}

$baseline = [IO.Path]::GetFullPath($BaselineApk)
$candidate = [IO.Path]::GetFullPath($CandidateApk)
$baselineRepo = [IO.Path]::GetFullPath($BaselineRepoRoot)
$candidateRepo = [IO.Path]::GetFullPath($CandidateRepoRoot)
foreach ($requiredFile in @(
    $baseline,
    $candidate,
    (Join-Path $baselineRepo 'tools\InspectRtPipelineBundleContainment.ps1'),
    (Join-Path $candidateRepo 'tools\InspectRtPipelineBundleContainment.ps1')
)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) { throw "Required file not found: $requiredFile" }
}
$spirvDisCommand = Get-Command spirv-dis -ErrorAction Stop
$spirvValCommand = Get-Command spirv-val -ErrorAction Stop

$baselineAssets = Get-ZipEntryMap $baseline 'assets/'
$candidateAssets = Get-ZipEntryMap $candidate 'assets/'
$allAssetNames = @($baselineAssets.Keys + $candidateAssets.Keys | Sort-Object -Unique -CaseSensitive)
$assetEntries = @(
    foreach ($name in $allAssetNames) {
        $left = $baselineAssets[$name]
        $right = $candidateAssets[$name]
        [ordered]@{
            name = $name
            baseline = $left
            candidate = $right
            byteIdentical = ($null -ne $left -and $null -ne $right -and
                $left.length -eq $right.length -and $left.sha256 -ceq $right.sha256)
        }
    }
)

$textEvidence = @()
foreach ($name in @(
    'assets/ASSET_LICENSES.md',
    'assets/models/player/runtime/clip-manifest.json',
    'assets/models/player/viewmodel/runtime/asset.manifest.json'
)) {
    $leftText = Read-ZipEntryText $baseline $name
    $rightText = Read-ZipEntryText $candidate $name
    $entry = [ordered]@{
        name = $name
        baselineSha256 = $baselineAssets[$name].sha256
        candidateSha256 = $candidateAssets[$name].sha256
        baselineLength = $baselineAssets[$name].length
        candidateLength = $candidateAssets[$name].length
        baselineNewlines = Get-TextNewlineCounts $leftText
        candidateNewlines = Get-TextNewlineCounts $rightText
    }
    if ($name.EndsWith('.json', [StringComparison]::OrdinalIgnoreCase)) {
        $leftObject = $leftText | ConvertFrom-Json
        $rightObject = $rightText | ConvertFrom-Json
        $leftCanonical = ConvertTo-Json -InputObject (ConvertTo-CanonicalJsonValue $leftObject) -Depth 100 -Compress
        $rightCanonical = ConvertTo-Json -InputObject (ConvertTo-CanonicalJsonValue $rightObject) -Depth 100 -Compress
        $entry.kind = 'json'
        $entry.semanticEqual = ($leftCanonical -ceq $rightCanonical)
        $entry.baselineObject = $leftObject
        $entry.candidateObject = $rightObject
        $entry.canonicalBaseline = $leftCanonical
        $entry.canonicalCandidate = $rightCanonical
    }
    else {
        $leftLines = [System.Collections.Generic.List[string]]::new()
        $reader = [IO.StringReader]::new($leftText)
        try { while (($line = $reader.ReadLine()) -ne $null) { $leftLines.Add($line) } }
        finally { $reader.Dispose() }
        $rightLines = [System.Collections.Generic.List[string]]::new()
        $reader = [IO.StringReader]::new($rightText)
        try { while (($line = $reader.ReadLine()) -ne $null) { $rightLines.Add($line) } }
        finally { $reader.Dispose() }
        $lineDiff = @()
        $lineCount = [Math]::Max($leftLines.Count, $rightLines.Count)
        for ($index = 0; $index -lt $lineCount; ++$index) {
            $leftLine = if ($index -lt $leftLines.Count) { $leftLines[$index] } else { $null }
            $rightLine = if ($index -lt $rightLines.Count) { $rightLines[$index] } else { $null }
            if (-not [string]::Equals($leftLine, $rightLine, [StringComparison]::Ordinal)) {
                $lineDiff += [ordered]@{ line = $index + 1; baseline = $leftLine; candidate = $rightLine }
            }
        }
        $entry.kind = 'markdown'
        $entry.lineContentEqual = ($lineDiff.Count -eq 0)
        $entry.changedLines = $lineDiff
    }
    $textEvidence += $entry
}

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-shipping-ab-asset-evidence-' + [guid]::NewGuid().ToString('N'))
$tempParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
New-Item -ItemType Directory -Path $tempRoot | Out-Null
try {
    $moduleEvidence = @()
    $normalizedModuleData = @()
    foreach ($entry in @(
        [ordered]@{ label = 'baseline'; apk = $baseline; repo = $baselineRepo },
        [ordered]@{ label = 'candidate'; apk = $candidate; repo = $candidateRepo }
    )) {
        $libraryEntry = 'lib/arm64-v8a/libhorde_rt_probe_android.so'
        $libraryBytes = Read-ZipEntryBytes $entry.apk $libraryEntry
        $libraryPath = Join-Path $tempRoot ($entry.label + '-libhorde_rt_probe_android.so')
        [IO.File]::WriteAllBytes($libraryPath, $libraryBytes)
        $containmentScript = Join-Path $entry.repo 'tools\InspectRtPipelineBundleContainment.ps1'
        $scanText = & $containmentScript -TargetPath $libraryPath -TargetPlatform Android `
            -Instrumentation Shipping -Quality Mobile | Out-String
            $scan = $scanText | ConvertFrom-Json
        if ($scan.targetSha256 -cne (Get-Sha256 $libraryBytes)) { throw "Containment hash mismatch for $($entry.label) packaged library." }
        $inspectedModules = @()
        foreach ($module in $scan.modules) {
            $offset = [int]$module.offset
            $byteCount = [int]$module.words * 4
            if ($offset -lt 0 -or $byteCount -le 20 -or $offset + $byteCount -gt $libraryBytes.Length) {
                throw "Invalid embedded SPIR-V range in $($entry.label) library."
            }
            $moduleBytes = New-Object byte[] $byteCount
            [Array]::Copy($libraryBytes, $offset, $moduleBytes, 0, $byteCount)
            $modulePath = Join-Path $tempRoot ("$($entry.label)-$($module.executionModel)-$($module.sha256).spv")
            $semanticKey = Get-VariantKey $entry.repo $module.backend $module.sha256 @($scan.semanticKeys)
            [IO.File]::WriteAllBytes($modulePath, $moduleBytes)
            & $spirvValCommand.Source --target-env vulkan1.2 $modulePath
            if ($LASTEXITCODE -ne 0) { throw "spirv-val failed for $($entry.label) module $($module.sha256)." }
            $assemblyPath = "$modulePath.spvasm"
            & $spirvDisCommand.Source --raw-id $modulePath -o $assemblyPath
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $assemblyPath -PathType Leaf)) {
                throw "spirv-dis failed for $($entry.label) module $($module.sha256)."
            }
            $normalized = Get-NormalizedShaderInstructions $assemblyPath
            $normalizedModuleData += [PSCustomObject]@{
                label = $entry.label
                semanticKey = $semanticKey
                instructions = $normalized.instructions
            }
            $inspectedModules += [ordered]@{
                semanticKey = $semanticKey
                backend = $module.backend
                executionModel = $module.executionModel
                sha256 = $module.sha256
                words = $module.words
                rayQueryInitializations = $module.rayQueryInitializations
                binding22 = $module.binding22
                atomicInstructions = $module.atomicInstructions
                opImageReadCount = $normalized.opImageReadCount
                normalizedInstructionCount = $normalized.instructionCount
                normalizedInstructionSha256 = $normalized.normalizedSha256
                spirvVal = 'passed'
                spirvDis = 'passed'
            }
        }
        if ($inspectedModules.Count -ne 4) { throw "Expected four packaged Shipping modules in $($entry.label); saw $($inspectedModules.Count)." }
        $moduleEvidence += [ordered]@{
            label = $entry.label
            apkSha256 = (Get-FileHash -LiteralPath $entry.apk -Algorithm SHA256).Hash.ToLowerInvariant()
            libraryEntry = $libraryEntry
            librarySha256 = $scan.targetSha256
            libraryBytes = $scan.targetBytes
            semanticKeys = @($scan.semanticKeys)
            modules = $inspectedModules
        }
    }

    $instructionComparisons = @()
    $baselineNormalized = @($normalizedModuleData | Where-Object { $_.label -ceq 'baseline' })
    $candidateNormalized = @($normalizedModuleData | Where-Object { $_.label -ceq 'candidate' })
    foreach ($candidateModule in $candidateNormalized) {
        $baselineMatches = @($baselineNormalized | Where-Object { $_.semanticKey -ceq $candidateModule.semanticKey })
        if ($baselineMatches.Count -ne 1) { throw "No unique baseline SPIR-V for $($candidateModule.semanticKey)." }
        $leftInstructions = $baselineMatches[0].instructions
        $rightInstructions = $candidateModule.instructions
        $firstDifference = $null
        $sharedCount = [Math]::Min($leftInstructions.Count, $rightInstructions.Count)
        for ($index = 0; $index -lt $sharedCount; ++$index) {
            if (-not [string]::Equals($leftInstructions[$index], $rightInstructions[$index], [StringComparison]::Ordinal)) {
                $firstDifference = [ordered]@{
                    zeroBasedInstructionIndex = $index
                    baseline = $leftInstructions[$index]
                    candidate = $rightInstructions[$index]
                }
                break
            }
        }
        if ($null -eq $firstDifference -and $leftInstructions.Count -ne $rightInstructions.Count) {
            $firstDifference = [ordered]@{
                zeroBasedInstructionIndex = $sharedCount
                baseline = if ($sharedCount -lt $leftInstructions.Count) { $leftInstructions[$sharedCount] } else { $null }
                candidate = if ($sharedCount -lt $rightInstructions.Count) { $rightInstructions[$sharedCount] } else { $null }
            }
        }
        $instructionComparisons += [ordered]@{
            semanticKey = $candidateModule.semanticKey
            baselineInstructionCount = $leftInstructions.Count
            candidateInstructionCount = $rightInstructions.Count
            normalizedInstructionsEqual = ($null -eq $firstDifference)
            firstDifference = $firstDifference
        }
    }

    $changedAssets = @($assetEntries | Where-Object { -not $_.byteIdentical })
    $receipt = [ordered]@{
        schema = 1
        generatedUtc = [DateTime]::UtcNow.ToString('o')
        inputs = [ordered]@{
            baselineApk = $baseline
            baselineApkSha256 = (Get-FileHash -LiteralPath $baseline -Algorithm SHA256).Hash.ToLowerInvariant()
            candidateApk = $candidate
            candidateApkSha256 = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash.ToLowerInvariant()
            baselineRepoRootForInspector = $baselineRepo
            candidateRepoRootForInspector = $candidateRepo
            spirvVal = $spirvValCommand.Source
            spirvDis = $spirvDisCommand.Source
        }
        assetSummary = [ordered]@{
            baselineCount = $baselineAssets.Count
            candidateCount = $candidateAssets.Count
            identicalCount = @($assetEntries | Where-Object { $_.byteIdentical }).Count
            changedCount = $changedAssets.Count
        }
        assets = $assetEntries
        changedTextAssetSemantics = $textEvidence
        packagedShippingModules = $moduleEvidence
        normalizedShaderInstructionComparisons = $instructionComparisons
    }
    $receiptJson = ConvertTo-Json -InputObject $receipt -Depth 100
    $receiptFullPath = [IO.Path]::GetFullPath($ReceiptPath)
    $receiptDirectory = Split-Path -Parent $receiptFullPath
    if (-not (Test-Path -LiteralPath $receiptDirectory -PathType Container)) {
        New-Item -ItemType Directory -Path $receiptDirectory | Out-Null
    }
    [IO.File]::WriteAllText($receiptFullPath, $receiptJson + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    Write-Output $receiptFullPath
}
finally {
    $resolvedTemp = [IO.Path]::GetFullPath($tempRoot)
    if (-not $resolvedTemp.StartsWith($tempParent, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove temporary path outside OS temp: $resolvedTemp"
    }
    if (Test-Path -LiteralPath $resolvedTemp) { Remove-Item -LiteralPath $resolvedTemp -Recurse -Force }
}
