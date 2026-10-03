[CmdletBinding()]
param(
    [string]$ReceiptPath = (Join-Path $PSScriptRoot 'independent-scan\independent-apk-module-receipt.json')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

$root = [IO.Path]::GetFullPath($PSScriptRoot)
$out = [IO.Path]::GetFullPath((Split-Path -Parent $ReceiptPath))
if (-not $out.StartsWith(($root.TrimEnd('\') + '\'), [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Receipt output must remain under the external investigation root.'
}
if (Test-Path -LiteralPath $out) { throw "Refusing to reuse existing output directory: $out" }
$null = New-Item -ItemType Directory -Path $out
$extracted = Join-Path $out 'modules'
$null = New-Item -ItemType Directory -Path $extracted

$source = Join-Path $root 'source-eaf'
$raygenCatalogPath = Join-Path $source 'tools\raygen-variant-catalog.json'
$computeCatalogPath = Join-Path $source 'tools\rayquery-variant-catalog.json'
$raygen = Get-Content -LiteralPath $raygenCatalogPath -Raw | ConvertFrom-Json
$compute = Get-Content -LiteralPath $computeCatalogPath -Raw | ConvertFrom-Json
$variants = @(
    [pscustomobject]@{ apkKind='benchmark'; instrumentation='Shipping'; quality='Mobile'; apk=(Join-Path $root 'artifacts\benchmark\HordeLanternRT-opaque-retained-investigation-shipping-mobile-arm64.apk') },
    [pscustomobject]@{ apkKind='debug'; instrumentation='Diagnostic'; quality='Mobile'; apk=(Join-Path $root 'artifacts\debug\HordeLanternRT-opaque-retained-investigation-diagnostic-mobile-arm64.apk') }
)
$spirvVal = 'C:\VulkanSDK\1.4.350.0\Bin\spirv-val.exe'
$spirvDis = 'C:\VulkanSDK\1.4.350.0\Bin\spirv-dis.exe'
foreach ($tool in @($spirvVal,$spirvDis)) { if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Missing validator tool: $tool" } }

function Hash-Bytes([byte[]]$Bytes) {
    $h=[Security.Cryptography.SHA256]::Create()
    try { return ([Convert]::ToHexString($h.ComputeHash($Bytes))).ToLowerInvariant() }
    finally { $h.Dispose() }
}
function Read-ZipBytes([string]$Apk,[string]$Name) {
    $zip=[IO.Compression.ZipFile]::OpenRead($Apk)
    try {
        $entry=$zip.GetEntry($Name)
        if ($null -eq $entry) { throw "APK is missing $Name : $Apk" }
        $stream=$entry.Open()
        try { $memory=[IO.MemoryStream]::new(); try { $stream.CopyTo($memory); return ,$memory.ToArray() } finally { $memory.Dispose() } }
        finally { $stream.Dispose() }
    } finally { $zip.Dispose() }
}
function Get-AssetMap([string]$Apk) {
    $zip=[IO.Compression.ZipFile]::OpenRead($Apk)
    try {
        $map=[ordered]@{}
        foreach ($entry in $zip.Entries | Where-Object { $_.FullName.StartsWith('assets/',[StringComparison]::Ordinal) -and $_.Name.Length -gt 0 } | Sort-Object FullName) {
            $s=$entry.Open(); try { $m=[IO.MemoryStream]::new(); try { $s.CopyTo($m); $hash=Hash-Bytes $m.ToArray() } finally { $m.Dispose() } } finally { $s.Dispose() }
            $map[$entry.FullName]=[ordered]@{length=[long]$entry.Length;sha256=$hash}
        }
        return $map
    } finally { $zip.Dispose() }
}
function ConvertTo-CanonicalJsonValue($Value) {
    if ($null -eq $Value) { return $null }
    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        $ordered=[ordered]@{}
        foreach ($property in $Value.PSObject.Properties | Sort-Object Name -CaseSensitive) { $ordered[$property.Name]=ConvertTo-CanonicalJsonValue $property.Value }
        return $ordered
    }
    if ($Value -is [System.Collections.IDictionary]) {
        $ordered=[ordered]@{}
        foreach ($key in @($Value.Keys | Sort-Object -CaseSensitive)) { $ordered[[string]$key]=ConvertTo-CanonicalJsonValue $Value[$key] }
        return $ordered
    }
    if ($Value -is [System.Collections.IEnumerable] -and $Value -isnot [string]) {
        $items=@(); foreach ($item in $Value) { $items+=,(ConvertTo-CanonicalJsonValue $item) }; return ,$items
    }
    return $Value
}

$moduleRecords=@()
$apkRecords=@()
foreach ($variant in $variants) {
    if (-not (Test-Path -LiteralPath $variant.apk -PathType Leaf)) { throw "Missing sealed APK: $($variant.apk)" }
    $apkHash=(Get-FileHash -LiteralPath $variant.apk -Algorithm SHA256).Hash.ToLowerInvariant()
    $elfName='lib/arm64-v8a/libhorde_rt_probe_android.so'
    $elf=Read-ZipBytes $variant.apk $elfName
    if ($elf.Length -lt 64 -or $elf[0] -ne 0x7f -or $elf[1] -ne 0x45 -or $elf[2] -ne 0x4c -or $elf[3] -ne 0x46 -or $elf[4] -ne 2 -or $elf[5] -ne 1 -or [BitConverter]::ToUInt16($elf,18) -ne 0x00b7) { throw "Not an ARM64 little-endian ELF: $($variant.apk)" }
    $elfPath=Join-Path $extracted "$($variant.apkKind)-libhorde_rt_probe_android.so"
    [IO.File]::WriteAllBytes($elfPath,$elf)
    $elfHash=Hash-Bytes $elf
    $selectedRaygen=@($raygen.variants | Where-Object { $_.instrumentation -ceq $variant.instrumentation -and $_.quality -ceq $variant.quality })
    $selectedCompute=@($compute.variants | Where-Object { $_.instrumentation -ceq $variant.instrumentation -and $_.quality -ceq $variant.quality })
    if ($selectedRaygen.Count -ne 2 -or $selectedCompute.Count -ne 2) { throw 'Catalog selection did not resolve exactly two modules per backend.' }
    $candidateOffsets=@()
    for ($offset=0; $offset -le $elf.Length-20; $offset+=4) {
        if ([BitConverter]::ToUInt32($elf,$offset) -eq 0x07230203) {
            $candidateOffsets += $offset
        }
    }
    foreach ($offset in $candidateOffsets) {
        if ($offset+20 -gt $elf.Length) { continue }
        $model=$null; $cursor=5; $valid=$true
        while (($offset+$cursor*4) -lt $elf.Length -and $cursor -lt 4096) {
            $inst=[BitConverter]::ToUInt32($elf,$offset+$cursor*4); $wc=[int]($inst -shr 16); $op=[int]($inst -band 0xffff)
            if ($wc -le 0 -or $offset+($cursor+$wc)*4 -gt $elf.Length) { $valid=$false; break }
            if ($op -eq 15 -and $wc -ge 3) { $candidate=[BitConverter]::ToUInt32($elf,$offset+($cursor+1)*4); if ($candidate -eq 5 -or $candidate -eq 5313) { $model=[int]$candidate; break } }
            if ($op -eq 54) { break }
            $cursor += $wc
        }
        if (-not $valid -or $null -eq $model) { continue }
        $rows=if ($model -eq 5313) { $selectedRaygen } elseif ($model -eq 5) { $selectedCompute } else { @() }
        $matches=@()
        foreach ($row in $rows) {
            $bytes=[int]$row.words*4
            if ($offset+$bytes -gt $elf.Length) { continue }
            $module=New-Object byte[] $bytes; [Array]::Copy($elf,$offset,$module,0,$bytes)
            if ((Hash-Bytes $module) -ceq [string]$row.spirvSha256) { $matches+=,[pscustomobject]@{row=$row;bytes=$module} }
        }
        if ($matches.Count -ne 1) { throw "Relevant aligned SPIR-V at ELF offset $offset is unknown/ambiguous; model=$model matches=$($matches.Count)." }
        $row=$matches[0].row; $module=$matches[0].bytes
        $wordCount=[int]($module.Length/4); $cursor=5; $atomicCounts=[ordered]@{}; $binding22=$false; $queryInit=0; $hasQueryCap=$false; $lastOp=-1
        while ($cursor -lt $wordCount) {
            $inst=[BitConverter]::ToUInt32($module,$cursor*4); $wc=[int]($inst -shr 16); $op=[int]($inst -band 0xffff)
            if ($wc -le 0 -or $cursor+$wc -gt $wordCount) { throw "Malformed SPIR-V instructions in $($row.key)." }
            if ($op -eq 17 -and $wc -eq 2 -and [BitConverter]::ToUInt32($module,($cursor+1)*4) -eq 4472) { $hasQueryCap=$true }
            if ($op -eq 71 -and $wc -eq 4 -and [BitConverter]::ToUInt32($module,($cursor+2)*4) -eq 33 -and [BitConverter]::ToUInt32($module,($cursor+3)*4) -eq 22) { $binding22=$true }
            if ($op -eq 4473) { $queryInit++ }
            if (($op -ge 227 -and $op -le 242) -or $op -in @(318,319,5614,5615,6035)) { $k=[string]$op; if (-not $atomicCounts.Contains($k)) {$atomicCounts[$k]=0}; $atomicCounts[$k]++ }
            $lastOp=$op; $cursor+=$wc
        }
        if ($cursor -ne $wordCount -or $lastOp -ne 56 -or -not $hasQueryCap) { throw "Module shape/capability invalid for $($row.key)." }
        $atomics=0; foreach ($value in $atomicCounts.Values) { $atomics += [int]$value }
        if ($atomics -ne [int]$row.atomicInstructions) { throw "Atomic opcode count mismatch for $($row.key): actual $atomics catalog $($row.atomicInstructions)." }
        if ($queryInit -ne [int]$row.rayQueryInitializations -or $binding22 -ne [bool]$row.hasDiagnosticsBinding) { throw "Query/binding reflection mismatch for $($row.key)." }
        $modulePath=Join-Path $extracted "$($variant.apkKind)-$($row.key).spv"
        [IO.File]::WriteAllBytes($modulePath,$module)
        $valLog=Join-Path $extracted "$($variant.apkKind)-$($row.key)-spirv-val.log"
        & $spirvVal --target-env vulkan1.2 $modulePath *> $valLog
        if ($LASTEXITCODE -ne 0) { throw "spirv-val failed for $($row.key); see $valLog" }
        $disPath=Join-Path $extracted "$($variant.apkKind)-$($row.key).spvasm"
        $disLog=Join-Path $extracted "$($variant.apkKind)-$($row.key)-spirv-dis.log"
        & $spirvDis $modulePath -o $disPath *> $disLog
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $disPath -PathType Leaf)) { throw "spirv-dis failed for $($row.key); see $disLog" }
        $assembly=Get-Content -LiteralPath $disPath -Raw
        $imageReadCount=[regex]::Matches($assembly,'(?m)^\s*%\S+\s*=\s*OpImageRead\b').Count
        if ($variant.instrumentation -ceq 'Shipping' -and ($binding22 -or $atomics -ne 0 -or $imageReadCount -ne 0)) { throw "Shipping module has diagnostics instrumentation: $($row.key)" }
        $moduleRecords += [pscustomobject]@{ apkKind=$variant.apkKind;backend=$(if($model -eq 5313){'RayTracingPipeline'}else{'RayQueryCompute'});catalogKey=[string]$row.key;strategy=[string]$row.strategy;elfOffsetBytes=$offset;sha256=Hash-Bytes $module;bytes=$module.Length;words=$wordCount;spirvVal='passed Vulkan 1.2';spirvDis='passed';rayQueryInitializeCount=$queryInit;hasBinding22=$binding22;atomicOpcodeCounts=$atomicCounts;atomicInstructionCount=$atomics;opImageReadCount=$imageReadCount;moduleFile=$modulePath }
    }
    $found=@($moduleRecords | Where-Object apkKind -ceq $variant.apkKind)
    if ($found.Count -ne 4) { throw "Expected four exact selected modules in $($variant.apkKind) ELF; found $($found.Count)." }
    $apkRecords += [pscustomobject]@{kind=$variant.apkKind;path=$variant.apk;sha256=$apkHash;elfEntry=$elfName;elfSha256=$elfHash;elfBytes=$elf.Length;elfPath=$elfPath;selectedPipelineOpaqueKey=($selectedRaygen | Where-Object material -ceq 'OpaqueFast').key;selectedPipelineGenericKey=($selectedRaygen | Where-Object material -ceq 'GenericDielectric').key;selectedComputeOpaqueKey=($selectedCompute | Where-Object material -ceq 'OpaqueFast').key;selectedComputeGenericKey=($selectedCompute | Where-Object material -ceq 'GenericDielectric').key;modules=$found}
}

$controlApk='C:\Dev\tmp\horde-shipping-ab-20260930\active-strategy\HordeLanternRT-eaf-active-strategy-benchmark-arm64.apk'
if (-not (Test-Path -LiteralPath $controlApk -PathType Leaf)) { throw "Missing frozen control APK: $controlApk" }
$controlAssets=Get-AssetMap $controlApk
$assetComparisons=@()
foreach ($variant in $variants) {
    $candidateAssets=Get-AssetMap $variant.apk
    $names=@($controlAssets.Keys + $candidateAssets.Keys | Sort-Object -Unique)
    $changed=@()
    foreach ($name in $names) {
        $a=$controlAssets[$name]; $b=$candidateAssets[$name]
        if ($null -eq $a -or $null -eq $b -or $a.length -ne $b.length -or $a.sha256 -cne $b.sha256) { $changed+=,[pscustomobject]@{entry=$name;control=$a;candidate=$b} }
    }
    $assetComparisons += [pscustomobject]@{candidateKind=$variant.apkKind;controlApk=$controlApk;controlApkSha256=(Get-FileHash -LiteralPath $controlApk -Algorithm SHA256).Hash.ToLowerInvariant();controlAssetCount=$controlAssets.Count;candidateAssetCount=$candidateAssets.Count;byteIdenticalAssetCount=($names.Count-$changed.Count);changedEntryCount=$changed.Count;changedEntries=$changed;controlAssets=$controlAssets;candidateAssets=$candidateAssets}
}

$assetSemantics=@()
foreach ($variant in $variants) {
    foreach ($entryName in @('assets/ASSET_LICENSES.md','assets/models/player/runtime/clip-manifest.json','assets/models/player/viewmodel/runtime/asset.manifest.json')) {
        $leftBytes=Read-ZipBytes $controlApk $entryName; $rightBytes=Read-ZipBytes $variant.apk $entryName
        $leftText=[Text.Encoding]::UTF8.GetString($leftBytes); $rightText=[Text.Encoding]::UTF8.GetString($rightBytes)
        if ($entryName.EndsWith('.json',[StringComparison]::OrdinalIgnoreCase)) {
            $left=ConvertTo-Json -InputObject (ConvertTo-CanonicalJsonValue ($leftText|ConvertFrom-Json)) -Depth 100 -Compress
            $right=ConvertTo-Json -InputObject (ConvertTo-CanonicalJsonValue ($rightText|ConvertFrom-Json)) -Depth 100 -Compress
            $assetSemantics += [pscustomobject]@{candidateKind=$variant.apkKind;entry=$entryName;kind='canonical-json';semanticEqual=($left -ceq $right);controlCanonicalSha256=Hash-Bytes ([Text.UTF8Encoding]::new($false).GetBytes($left));candidateCanonicalSha256=Hash-Bytes ([Text.UTF8Encoding]::new($false).GetBytes($right))}
        } else {
            $leftLines=[regex]::Split($leftText,"\r\n|\n|\r"); $rightLines=[regex]::Split($rightText,"\r\n|\n|\r")
            $leftNormalized=[string]::Join("`n",$leftLines); $rightNormalized=[string]::Join("`n",$rightLines)
            $assetSemantics += [pscustomobject]@{candidateKind=$variant.apkKind;entry=$entryName;kind='line-normalized-markdown';semanticEqual=($leftNormalized -ceq $rightNormalized);controlCrlf=([regex]::Matches($leftText,"`r`n").Count);candidateCrlf=([regex]::Matches($rightText,"`r`n").Count);controlLfOnly=([regex]::Matches($leftText,'(?<!`r)`n').Count);candidateLfOnly=([regex]::Matches($rightText,'(?<!`r)`n').Count);controlNormalizedSha256=Hash-Bytes ([Text.UTF8Encoding]::new($false).GetBytes($leftNormalized));candidateNormalizedSha256=Hash-Bytes ([Text.UTF8Encoding]::new($false).GetBytes($rightNormalized))}
        }
    }
}

$abiFiles=@('src/vulkan/raytracing/RtSceneAbi.generated.h','shaders/raytracing/include/rt_scene_abi.generated.glsl')
$abiRecords=@()
foreach ($relative in $abiFiles) {
    $path=Join-Path $source $relative
    $headBlob=(git -C $source rev-parse "eafbf8262442a82e0835edf5cf5306d718e64633:$relative").Trim()
    $worktreeBlob=(git -C $source hash-object -- $relative).Trim()
    if ($headBlob -cne $worktreeBlob) { throw "ABI input differs from exact source base: $relative" }
    $abiRecords += [pscustomobject]@{path=$relative;headBlobSha1=$headBlob;worktreeBlobSha1=$worktreeBlob;identicalToSourceBase=$true;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$abiText=Get-Content -LiteralPath (Join-Path $source 'src\vulkan\raytracing\RtSceneAbi.generated.h')
$structStart=($abiText | Select-String 'struct alignas\(16\) RtDielectricDiagnostics').LineNumber
$structEnd=($abiText | Select-String '^};' | Where-Object LineNumber -gt $structStart | Select-Object -First 1).LineNumber
$counterFieldCount=@($abiText[($structStart-1)..($structEnd-2)] | Where-Object { $_ -match 'std::uint32_t\s+\w+' }).Count
if ($counterFieldCount -ne 41) { throw "Expected unchanged 41-field diagnostics roster; found $counterFieldCount." }

$receipt=[ordered]@{
    schema=1
    scope='independent exact module extraction and SPIR-V validation from sealed APK ELF; intentionally does not rerun or replace standard policy-parity containment'
    generatedUtc=[DateTime]::UtcNow.ToString('o')
    sourceRoot=$source
    sourceHead='eafbf8262442a82e0835edf5cf5306d718e64633'
    sourceDirty='experimental opaque raygen catalog/includes only; no production source or compute module edits'
    catalogs=[ordered]@{raygenPath=$raygenCatalogPath;raygenSha256=(Get-FileHash -LiteralPath $raygenCatalogPath -Algorithm SHA256).Hash.ToLowerInvariant();computePath=$computeCatalogPath;computeSha256=(Get-FileHash -LiteralPath $computeCatalogPath -Algorithm SHA256).Hash.ToLowerInvariant()}
    androidArtifacts=$apkRecords
    assetComparison=$assetComparisons
    assetSemanticComparison=$assetSemantics
    abiEvidence=[ordered]@{inputs=$abiRecords;diagnosticsCounterFieldCount=$counterFieldCount;interpretation='Generated native and GLSL ABI files are byte-identical to exact eaf source base; the diagnostic roster remains 41 fields.'}
    standardContainmentStatus='RED preserved: standard checker rejects compute/raygen strategy parity for rayquery_compute_diagnostic_mobile_opaque_fast; this independent scan makes no policy-admission claim'
    runtime='No device/ADB/runtime or performance claims; diagnostic static atomics are module contents, not runtime counter evidence.'
}
$json=$receipt | ConvertTo-Json -Depth 14
[IO.File]::WriteAllText($ReceiptPath,$json+"`n",[Text.UTF8Encoding]::new($false))
Get-FileHash -LiteralPath $ReceiptPath -Algorithm SHA256 | Select-Object Path,Hash
Write-Output "Receipt: $ReceiptPath"
Write-Output "Modules scanned: $($moduleRecords.Count); candidate assets compared: $($assetComparisons.Count)"
