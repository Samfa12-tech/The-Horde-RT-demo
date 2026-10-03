[CmdletBinding()]
param(
    [string]$OutputRoot = 'C:\Dev\tmp\horde-first-blocker-20260930\ray-flag-audit\results-final'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

$controlApk = 'C:\Dev\tmp\horde-shipping-ab-20260930\active-strategy\HordeLanternRT-eaf-active-strategy-benchmark-arm64.apk'
$candidateApk = 'C:\Dev\tmp\horde-first-blocker-20260930\artifacts\benchmark\candidate-benchmark.apk'
$controlContainmentPath = 'C:\Dev\tmp\horde-shipping-ab-20260930\active-strategy\containment.json'
$candidateContainmentPath = 'C:\Dev\tmp\horde-first-blocker-20260930\artifacts\benchmark\containment.json'
$comparisonPath = 'C:\Dev\tmp\horde-first-blocker-20260930\artifacts\benchmark\asset-shader-comparison.json'
$sdk = 'C:\VulkanSDK\1.4.350.0\Bin'
$spirvVal = Join-Path $sdk 'spirv-val.exe'
$spirvDis = Join-Path $sdk 'spirv-dis.exe'

function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}
function Get-Sha256([byte[]]$bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([Convert]::ToHexString($sha.ComputeHash($bytes))).ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function Get-FileSha256([string]$path) {
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Get-LibraryBytes([string]$apkPath) {
    $zip = [IO.Compression.ZipFile]::OpenRead($apkPath)
    try {
        $entry = $zip.GetEntry('lib/arm64-v8a/libhorde_rt_probe_android.so')
        Assert-True ($null -ne $entry) "Missing packaged ARM64 ELF in $apkPath"
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
function Get-ModuleBytes([byte[]]$library, $moduleRow) {
    $offset = [int]$moduleRow.offset
    $length = [int]$moduleRow.words * 4
    Assert-True ($offset -ge 0 -and $length -gt 20 -and $offset + $length -le $library.Length) 'SPIR-V module range is outside its packaged ELF.'
    $bytes = New-Object byte[] $length
    [Array]::Copy($library, $offset, $bytes, 0, $length)
    Assert-True ((Get-Sha256 $bytes) -ceq [string]$moduleRow.sha256) 'Extracted module SHA does not match the exact containment receipt.'
    return ,$bytes
}
function Get-DisassemblyInfo([string]$path) {
    $lines = @(Get-Content -LiteralPath $path)
    $constants = [System.Collections.Generic.Dictionary[string,long]]::new([StringComparer]::Ordinal)
    foreach ($line in $lines) {
        if ($line -match '^\s*(?<id>%\d+)\s*=\s*OpConstant\s+%\d+\s+(?<value>-?(?:0x[0-9A-Fa-f]+|\d+))\s*$') {
            $value = if ($Matches.value.StartsWith('0x', [StringComparison]::OrdinalIgnoreCase)) {
                [Convert]::ToInt64($Matches.value.Substring(2), 16)
            } else { [long]::Parse($Matches.value, [Globalization.CultureInfo]::InvariantCulture) }
            $constants[$Matches.id] = $value
        }
    }
    $flagSites = @()
    foreach ($line in $lines) {
        if ($line -match '^\s*OpRayQueryInitializeKHR\b') {
            $operands = @([regex]::Matches($line, '%\d+') | ForEach-Object Value)
            Assert-True ($operands.Count -eq 8) "Unexpected OpRayQueryInitializeKHR operand count: $line"
            $flagId = [string]$operands[2]
            $flagValue = $null
            if ($constants.ContainsKey($flagId)) { $flagValue = $constants[$flagId] }
            $flagSites += [pscustomobject]@{ operandId = $flagId; numericValue = $flagValue; originalLine = $line.Trim() }
        }
    }
    return [pscustomobject]@{ lines = $lines; flagSites = $flagSites }
}
function Get-NormalizedInstructions([string[]]$lines) {
    $debugOps = @('OpSourceContinued','OpSource','OpSourceExtension','OpName','OpMemberName','OpString','OpLine','OpNoLine','OpModuleProcessed')
    $idMap = [System.Collections.Generic.Dictionary[string,int]]::new([StringComparer]::Ordinal)
    $instructions = [System.Collections.Generic.List[string]]::new()
    foreach ($line in $lines) {
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith(';', [StringComparison]::Ordinal)) { continue }
        if ($trimmed -notmatch '^(?:%\d+\s*=\s*)?(?<op>Op[A-Za-z0-9]+)\b') { throw "Unrecognized SPIR-V disassembly line: $trimmed" }
        if ($debugOps -ccontains $Matches.op) { continue }
        $builder = [Text.StringBuilder]::new(); $cursor = 0
        foreach ($match in [regex]::Matches($trimmed, '%\d+')) {
            [void]$builder.Append($trimmed.Substring($cursor, $match.Index - $cursor))
            if (-not $idMap.ContainsKey($match.Value)) { $idMap.Add($match.Value, $idMap.Count) }
            [void]$builder.Append('%v').Append($idMap[$match.Value])
            $cursor = $match.Index + $match.Length
        }
        [void]$builder.Append($trimmed.Substring($cursor))
        $instructions.Add($builder.ToString())
    }
    return ,$instructions.ToArray()
}
function Get-FlagCounts($sites) {
    $counts = [ordered]@{}
    foreach ($group in @($sites | Group-Object { if ($null -eq $_.numericValue) { 'unresolved' } else { [string]$_.numericValue } } | Sort-Object Name)) {
        $counts[$group.Name] = $group.Count
    }
    return $counts
}

Assert-True (-not (Test-Path -LiteralPath $OutputRoot)) "Refusing to overwrite audit output: $OutputRoot"
foreach ($path in @($controlApk,$candidateApk,$controlContainmentPath,$candidateContainmentPath,$comparisonPath,$spirvVal,$spirvDis)) {
    Assert-True (Test-Path -LiteralPath $path -PathType Leaf) "Required input is missing: $path"
}
Assert-True ((Get-FileSha256 $controlApk) -ceq 'ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae') 'Control APK identity changed.'
Assert-True ((Get-FileSha256 $candidateApk) -ceq '0383edcad0ffb0ab9e116ff5df7f8e20bbdb4cdca1f06a30986709257e42387e') 'Candidate APK identity changed.'
$controlContainment = Get-Content -LiteralPath $controlContainmentPath -Raw | ConvertFrom-Json
$candidateContainment = Get-Content -LiteralPath $candidateContainmentPath -Raw | ConvertFrom-Json
$comparison = Get-Content -LiteralPath $comparisonPath -Raw | ConvertFrom-Json
$controlLibrary = Get-LibraryBytes $controlApk
$candidateLibrary = Get-LibraryBytes $candidateApk
Assert-True ((Get-Sha256 $controlLibrary) -ceq $controlContainment.targetSha256) 'Control ELF does not match containment evidence.'
Assert-True ((Get-Sha256 $candidateLibrary) -ceq $candidateContainment.targetSha256) 'Candidate ELF does not match containment evidence.'

New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$moduleRoot = Join-Path $OutputRoot 'modules'
New-Item -ItemType Directory -Path $moduleRoot | Out-Null
$controlModules = @{}
$candidateModules = @{}
foreach ($label in @('control','candidate')) {
    $library = if ($label -ceq 'control') { $controlLibrary } else { $candidateLibrary }
    $containment = if ($label -ceq 'control') { $controlContainment } else { $candidateContainment }
    $moduleEvidence = @($comparison.packagedShippingModules | Where-Object { $_.label -ceq $(if ($label -ceq 'control') {'baseline'} else {'candidate'}) }).modules
    foreach ($evidence in $moduleEvidence) {
        $row = @($containment.modules | Where-Object { $_.sha256 -ceq $evidence.sha256 })
        Assert-True ($row.Count -eq 1) "No unique contained range for $label/$($evidence.semanticKey)."
        $module = Get-ModuleBytes $library $row[0]
        $modulePath = Join-Path $moduleRoot "$label-$($evidence.semanticKey).spv"
        $assemblyPath = "$modulePath.spvasm"
        [IO.File]::WriteAllBytes($modulePath, $module)
        & $spirvVal --target-env vulkan1.2 $modulePath
        Assert-True ($LASTEXITCODE -eq 0) "spirv-val failed for $label/$($evidence.semanticKey)."
        & $spirvDis --raw-id $modulePath -o $assemblyPath
        Assert-True ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $assemblyPath -PathType Leaf)) "spirv-dis failed for $label/$($evidence.semanticKey)."
        $info = Get-DisassemblyInfo $assemblyPath
        Assert-True ($info.flagSites.Count -gt 0) "No ray-query initialize sites found for $label/$($evidence.semanticKey)."
        $record = [pscustomobject]@{semanticKey=$evidence.semanticKey;sha256=$evidence.sha256;words=$evidence.words;modulePath=$modulePath;assemblyPath=$assemblyPath;flagSites=$info.flagSites;normalized=(Get-NormalizedInstructions $info.lines)}
        if ($label -ceq 'control') { $controlModules[$evidence.semanticKey] = $record } else { $candidateModules[$evidence.semanticKey] = $record }
    }
}

$flag = [ordered]@{gl_RayFlagsNoOpaqueEXT=2;gl_RayFlagsTerminateOnFirstHitEXT=4;binaryBlockerCombined=6}
$spirvHeader = 'C:\VulkanSDK\1.4.350.0\Include\spirv-headers\spirv.h'
$headerText = Get-Content -LiteralPath $spirvHeader -Raw
Assert-True ($headerText -match 'SpvRayFlagsNoOpaqueKHRMask\s*=\s*0x00000002' -and $headerText -match 'SpvRayFlagsTerminateOnFirstHitKHRMask\s*=\s*0x00000004') 'SPIR-V header ray flag masks do not match the expected aliases.'

$modulePairs = @()
foreach ($key in @('rayquery_compute_shipping_mobile_opaque_fast','shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_generic_dielectric','shipping_mobile_generic_dielectric')) {
    $control = $controlModules[$key]; $candidate = $candidateModules[$key]
    Assert-True ($null -ne $control -and $null -ne $candidate) "Missing module pair $key."
    $controlWords = [IO.File]::ReadAllBytes($control.modulePath); $candidateWords = [IO.File]::ReadAllBytes($candidate.modulePath)
    $controlWordCount = [int]($controlWords.Length / 4); $candidateWordCount = [int]($candidateWords.Length / 4)
    $wordDiffs = [System.Collections.Generic.List[object]]::new()
    $limit = [Math]::Min($controlWordCount, $candidateWordCount)
    for ($i=0; $i -lt $limit; ++$i) {
        $left = [BitConverter]::ToUInt32($controlWords, $i*4); $right = [BitConverter]::ToUInt32($candidateWords, $i*4)
        if ($left -ne $right) { $wordDiffs.Add([pscustomobject]@{wordIndex=$i;control=('0x{0:X8}' -f $left);candidate=('0x{0:X8}' -f $right)}) }
    }
    if ($controlWordCount -ne $candidateWordCount) { $wordDiffs.Add([pscustomobject]@{wordIndex=$limit;controlWordCount=$controlWordCount;candidateWordCount=$candidateWordCount}) }
    $leftInstructions = @($control.normalized); $rightInstructions = @($candidate.normalized)
    $instructionDiffs = [System.Collections.Generic.List[object]]::new()
    $instructionLimit = [Math]::Min($leftInstructions.Count, $rightInstructions.Count)
    for ($i=0; $i -lt $instructionLimit; ++$i) {
        if ($leftInstructions[$i] -cne $rightInstructions[$i]) { $instructionDiffs.Add([pscustomobject]@{instructionIndex=$i;control=$leftInstructions[$i];candidate=$rightInstructions[$i]}) }
    }
    if ($leftInstructions.Count -ne $rightInstructions.Count) { $instructionDiffs.Add([pscustomobject]@{instructionIndex=$instructionLimit;controlCount=$leftInstructions.Count;candidateCount=$rightInstructions.Count}) }
    $flagChanges = [System.Collections.Generic.List[object]]::new()
    $siteLimit = [Math]::Min($control.flagSites.Count,$candidate.flagSites.Count)
    for ($i=0; $i -lt $siteLimit; ++$i) {
        $old=$control.flagSites[$i]; $new=$candidate.flagSites[$i]
        if ($old.numericValue -ne $new.numericValue) { $flagChanges.Add([pscustomobject]@{site=$i;control=$old.numericValue;candidate=$new.numericValue;controlLine=$old.originalLine;candidateLine=$new.originalLine}) }
    }
    $isOpaque = $key -like '*opaque_fast'
    if (-not $isOpaque) {
        Assert-True ($wordDiffs.Count -eq 0 -and $instructionDiffs.Count -eq 0 -and $flagChanges.Count -eq 0) "Generic module changed: $key"
    } else {
        Assert-True ($flagChanges.Count -gt 0 -and @($flagChanges | Where-Object { $_.control -ne 2 -or $_.candidate -ne 6 }).Count -eq 0) "OpaqueFast changes include flags other than NoOpaque(2) to NoOpaque|TerminateOnFirstHit(6): $key"
        Assert-True ($control.flagSites.Count -eq $candidate.flagSites.Count) "OpaqueFast query-init site counts differ: $key"
        for($i=0;$i -lt $siteLimit;$i++){if($i -notin @($flagChanges|ForEach-Object site)){Assert-True ($control.flagSites[$i].numericValue -eq $candidate.flagSites[$i].numericValue) "Nearest/other ray flags changed at site ${i}: $key"}}
        $queryInitDiffCount = 0
        $constantDiffCount = 0
        $constantDiff = $null
        foreach ($difference in $instructionDiffs) {
            $leftTokens = @($difference.control -split '\s+')
            $rightTokens = @($difference.candidate -split '\s+')
            if ($leftTokens[0] -ceq 'OpRayQueryInitializeKHR' -and $rightTokens[0] -ceq 'OpRayQueryInitializeKHR') {
                Assert-True ($leftTokens.Count -eq $rightTokens.Count -and $leftTokens.Count -eq 9) "Ray-query initialization shape changed: $key"
                for ($token=0; $token -lt $leftTokens.Count; ++$token) {
                    if ($token -ne 3) { Assert-True ($leftTokens[$token] -ceq $rightTokens[$token]) "Non-flag ray-query operand changed in $key at normalized instruction $($difference.instructionIndex)." }
                }
                ++$queryInitDiffCount
            } elseif ($difference.control -match '^%v\d+ = OpConstant %v\d+ 2$' -and $difference.candidate -match '^%v\d+ = OpConstant %v\d+ 6$') {
                $leftParts = @($difference.control -split '\s+'); $rightParts = @($difference.candidate -split '\s+')
                Assert-True ($leftParts[0] -ceq $rightParts[0] -and $leftParts[1] -ceq $rightParts[1] -and $leftParts[2] -ceq $rightParts[2]) "Ray-flag constant changed beyond literal 2 to 6 in $key."
                $constantDiff = [pscustomobject]@{normalizedId=$leftParts[0];typeId=$leftParts[2];control=$difference.control;candidate=$difference.candidate}
                ++$constantDiffCount
            } else { throw "Unexpected normalized instruction change in $key at $($difference.instructionIndex): $($difference.control) => $($difference.candidate)" }
        }
        Assert-True ($queryInitDiffCount -eq $flagChanges.Count -and $constantDiffCount -le 1) "OpaqueFast instruction differences do not correspond exactly to changed flag sites and an optional 2-to-6 constant: $key; queryDiffs=$queryInitDiffCount flagSites=$($flagChanges.Count) constantDiffs=$constantDiffCount allDiffs=$($instructionDiffs.Count)"
        if ($null -ne $constantDiff) {
            $idPattern = [regex]::Escape($constantDiff.normalizedId)
            foreach ($side in @(@{name='control';instructions=$leftInstructions},@{name='candidate';instructions=$rightInstructions})) {
                for ($instructionIndex=0; $instructionIndex -lt $side.instructions.Count; ++$instructionIndex) {
                    $instruction = $side.instructions[$instructionIndex]
                    if ($instruction -match '^%v\d+ = OpConstant\b') { continue }
                    if ([regex]::IsMatch($instruction,"(?<![A-Za-z0-9])$idPattern(?!\d)")) {
                        $tokens = @($instruction -split '\s+')
                        Assert-True ($tokens[0] -ceq 'OpRayQueryInitializeKHR' -and $tokens.Count -eq 9 -and $tokens[3] -ceq $constantDiff.normalizedId) "Changed ray-flag constant has a non-binary-query consumer in $key ($($side.name)): $instruction"
                    }
                }
            }
        }
    }
    $modulePairs += [pscustomobject]@{semanticKey=$key;controlSha256=$control.sha256;candidateSha256=$candidate.sha256;controlWords=$controlWordCount;candidateWords=$candidateWordCount;rawWordDiffCount=$wordDiffs.Count;rawWordDiffsFirst32=@($wordDiffs|Select-Object -First 32);normalizedInstructionCountControl=$leftInstructions.Count;normalizedInstructionCountCandidate=$rightInstructions.Count;normalizedInstructionDiffCount=$instructionDiffs.Count;normalizedInstructionDiffs=$instructionDiffs;changedFlagConstantDefinitionCount=$constantDiffCount;controlRayQueryInitializeCount=$control.flagSites.Count;candidateRayQueryInitializeCount=$candidate.flagSites.Count;controlFlagCounts=Get-FlagCounts $control.flagSites;candidateFlagCounts=Get-FlagCounts $candidate.flagSites;changedFlagSites=$flagChanges;byteIdentical=($wordDiffs.Count -eq 0 -and $controlWords.Length -eq $candidateWords.Length)}
}
$opaquePairs=@($modulePairs|Where-Object semanticKey -like '*opaque_fast')
$genericPairs=@($modulePairs|Where-Object semanticKey -like '*generic_dielectric')
Assert-True (@($genericPairs|Where-Object {-not $_.byteIdentical}).Count -eq 0) 'Generic modules are not byte-identical.'
Assert-True (@($opaquePairs|Where-Object {$_.normalizedInstructionDiffCount -lt $_.changedFlagSites.Count -or $_.normalizedInstructionDiffCount -gt ($_.changedFlagSites.Count + 1)}).Count -eq 0) 'OpaqueFast instruction differences exceed the intended changed flag sites plus an optional flag constant.'
$changedSitesEach = [int]$opaquePairs[0].changedFlagSites.Count
$initializeSitesEach = [int]$opaquePairs[0].controlRayQueryInitializeCount
Assert-True (@($opaquePairs|Where-Object {$_.changedFlagSites.Count -ne $changedSitesEach -or $_.controlRayQueryInitializeCount -ne $initializeSitesEach -or $_.changedFlagConstantDefinitionCount -ne 0}).Count -eq 0) 'OpaqueFast pipeline/compute flag evidence differs or a constant definition changed.'
$unchangedSitesEach = $initializeSitesEach - $changedSitesEach

$receipt=[ordered]@{schema=1;generatedUtc=[DateTime]::UtcNow.ToString('o');controlApk=[ordered]@{path=$controlApk;sha256=Get-FileSha256 $controlApk};candidateApk=[ordered]@{path=$candidateApk;sha256=Get-FileSha256 $candidateApk};controlElfSha256=(Get-Sha256 $controlLibrary);candidateElfSha256=(Get-Sha256 $candidateLibrary);spirvTools=[ordered]@{validator=$spirvVal;disassembler=$spirvDis;spirvHeader=$spirvHeader;noOpaqueMask='0x00000002';terminateOnFirstHitMask='0x00000004'};flags=$flag;modulePairs=$modulePairs;conclusion="Both GenericDielectric modules are byte-identical. Each OpaqueFast module changes only the binary-blocker ray-query flag operand at $changedSitesEach static initialize sites (2 to 6); the other $unchangedSitesEach of $initializeSitesEach sites retain their numeric flags and all other normalized instructions are unchanged. No constant definition changed. Raw word differences equal those $changedSitesEach query-flag operand differences."}
$receiptPath=Join-Path $OutputRoot 'ray-query-flag-audit.json'
Assert-True (-not (Test-Path -LiteralPath $receiptPath)) "Refusing to overwrite $receiptPath"
[IO.File]::WriteAllText($receiptPath,(ConvertTo-Json -InputObject $receipt -Depth 20)+"`n",[Text.UTF8Encoding]::new($false))
Get-Content -LiteralPath $receiptPath -Raw
