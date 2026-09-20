[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Apk,
    [Parameter(Mandatory=$true)][string]$PublishedApk,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$Sdk = 'C:\Users\sam_s\AppData\Local\Android\Sdk',
    [string]$SpirvTools = 'C:\VulkanSDK\1.4.350.0\Bin'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
function Require([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }
function Hash-Bytes([byte[]]$bytes) {
    [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}
function Entry-Bytes($zip, [string]$name) {
    $entry = $zip.GetEntry($name)
    Require ($null -ne $entry) "Missing APK entry: $name"
    $inputStream = $entry.Open()
    $memory = [IO.MemoryStream]::new()
    try { $inputStream.CopyTo($memory); return ,$memory.ToArray() }
    finally { $memory.Dispose(); $inputStream.Dispose() }
}
Require (-not (Test-Path -LiteralPath $OutputDirectory)) 'Output directory already exists; preserve prior evidence.'
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$Apk = (Resolve-Path -LiteralPath $Apk).Path
$PublishedApk = (Resolve-Path -LiteralPath $PublishedApk).Path
Require ((Get-FileHash -LiteralPath $PublishedApk).Hash.ToLowerInvariant() -eq
    '52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48') 'Published 1.6.0 APK hash mismatch.'
$buildTools = Get-ChildItem -LiteralPath (Join-Path $Sdk 'build-tools') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$badging = (& (Join-Path $buildTools.FullName 'aapt2.exe') dump badging $Apk | Out-String)
Require ($LASTEXITCODE -eq 0 -and $badging.Contains("package: name='com.samfa12.hordelanternrt.baseline'") -and
    $badging.Contains("versionName='1.6.0-source-baseline'") -and
    -not $badging.Contains('application-debuggable')) 'Wrong package identity or debuggable baseline.'
$certificate = (& (Join-Path $buildTools.FullName 'apksigner.bat') verify --print-certs $Apk | Out-String)
Require ($LASTEXITCODE -eq 0 -and $certificate -match 'CN=Android Debug') 'Baseline must use the development certificate.'
$changed = (& git -C $repo diff 57c81b6 --name-only -- src/vulkan shaders assets cmake/HordeRtSources.cmake | Out-String).Trim()
Require ($LASTEXITCODE -eq 0 -and -not $changed) "Baseline renderer/shader/assets were changed: $changed"
$models = @(Get-ChildItem (Join-Path $repo 'android/app/build/intermediates/cxx') -Recurse -Filter build_model.json |
    ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json } |
    Where-Object { $_.variant.variantName -eq 'baseline' })
Require ($models.Count -eq 4) 'Expected exactly four baseline native build models.'
Require ((@($models.info.name | Sort-Object) -join ',') -eq 'arm64-v8a,armeabi-v7a,x86,x86_64') 'Unexpected native ABI set.'
foreach ($model in $models) {
    Require (-not $model.variant.isDebuggableEnabled -and $model.variant.optimizationTag -eq 'RelWithDebInfo') 'Wrong native build mode.'
    $cache = Get-Content -LiteralPath (Join-Path $model.cxxBuildFolder 'CMakeCache.txt') -Raw
    Require ($cache.Contains('HORDE_RT_DEBUG_CHECKPOINTS:BOOL=OFF') -and
        $cache.Contains('HORDE_RT_BASELINE_BENCHMARK_HARNESS:BOOL=ON') -and
        $cache.Contains('CMAKE_BUILD_TYPE:STRING=RelWithDebInfo')) 'Wrong native harness/checkpoint policy.'
}
$zip = [IO.Compression.ZipFile]::OpenRead($Apk)
$published = [IO.Compression.ZipFile]::OpenRead($PublishedApk)
try {
    $assets = @($published.Entries | Where-Object { $_.FullName.StartsWith('assets/') -and $_.Name } | ForEach-Object FullName | Sort-Object)
    $newAssets = @($zip.Entries | Where-Object { $_.FullName.StartsWith('assets/') -and $_.Name } | ForEach-Object FullName | Sort-Object)
    Require (($assets -join "`n") -ceq ($newAssets -join "`n")) 'APK asset inventory changed.'
    foreach ($name in $assets) { Require ((Hash-Bytes (Entry-Bytes $zip $name)) -ceq (Hash-Bytes (Entry-Bytes $published $name))) "Changed packaged asset $name" }
    $library = Entry-Bytes $zip 'lib/arm64-v8a/libhorde_rt_probe_android.so'
    $strippedFiles = @(Get-ChildItem -LiteralPath (Join-Path $repo 'android/app/build/intermediates/stripped_native_libs/baseline') -Recurse -Filter libhorde_rt_probe_android.so |
        Where-Object { $_.FullName -match '\\arm64-v8a\\' })
    Require ($strippedFiles.Count -eq 1) 'Expected exactly one stripped baseline ARM64 library.'
    $stripped = $strippedFiles[0].FullName
    Require ((Hash-Bytes $library) -ceq (Get-FileHash -LiteralPath $stripped).Hash.ToLowerInvariant()) 'Packaged ARM64 differs from stripped native output.'
    $hexLibrary = [Convert]::ToHexString($library)
    $shaderReports = @()
    foreach ($name in @('MinimalRayGenShader', 'MinimalLegacyRayGenShader')) {
        $header = Join-Path $repo "src/vulkan/raytracing/$name.inc"
        $words = [regex]::Matches((Get-Content -LiteralPath $header -Raw), '0x([0-9a-fA-F]{8})u')
        $bytes = [byte[]]::new($words.Count * 4)
        for ($i = 0; $i -lt $words.Count; ++$i) {
            [Buffer]::BlockCopy([BitConverter]::GetBytes([Convert]::ToUInt32($words[$i].Groups[1].Value,16)), 0, $bytes, $i*4, 4)
        }
        Require ($words.Count -gt 5 -and $hexLibrary.Contains([Convert]::ToHexString($bytes))) "Exact old SPIR-V absent from ARM64: $name"
        $spv = Join-Path $OutputDirectory "$name.spv"
        $dis = Join-Path $OutputDirectory "$name.spvasm"
        [IO.File]::WriteAllBytes($spv, $bytes)
        & (Join-Path $SpirvTools 'spirv-val.exe') --target-env vulkan1.2 $spv
        Require ($LASTEXITCODE -eq 0) "SPIR-V validation failed: $name"
        & (Join-Path $SpirvTools 'spirv-dis.exe') $spv -o $dis
        Require ($LASTEXITCODE -eq 0) "SPIR-V disassembly failed: $name"
        $assembly = Get-Content -LiteralPath $dis -Raw
        $shaderReports += [ordered]@{
            name=$name; words=$words.Count; sha256=(Hash-Bytes $bytes)
            headerSha256=(Get-FileHash -LiteralPath $header).Hash.ToLowerInvariant()
            atomics=[regex]::Matches($assembly, '\bOpAtomic\w+').Count
            rayQueryInitializations=[regex]::Matches($assembly, '\bOpRayQueryInitializeKHR\b').Count
            spirvVal='passed'; spirvDis='passed'
        }
    }
    $report = [ordered]@{
        schema=1; status='passed'; baseSource='57c81b635a6c10e2772283639026936adac80f8b'
        apkSha256=(Get-FileHash -LiteralPath $Apk).Hash.ToLowerInvariant()
        arm64Sha256=(Hash-Bytes $library); nativeMode='RelWithDebInfo'
        checkpoints='OFF'; developmentSigned=$true; debuggable=$false
        identicalPublishedAssetCount=$assets.Count; shaders=$shaderReports
        qualification='Rebuilt source-baseline harness, not the public APK or diagnostic-free Shipping policy.'
    }
    [IO.File]::WriteAllText((Join-Path $OutputDirectory 'package-proof.json'), ($report | ConvertTo-Json -Depth 8) + "`n")
    $report | ConvertTo-Json -Depth 8
}
finally { $zip.Dispose(); $published.Dispose() }
