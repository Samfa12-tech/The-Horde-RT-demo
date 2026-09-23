[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ("horde-rt-abi-generator-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $lfDefinitionPath = Join-Path $temporaryRoot "lf.def"
    $crlfDefinitionPath = Join-Path $temporaryRoot "crlf.def"
    $portableCpuPath = Join-Path $temporaryRoot "portable.generated.h"
    $portableGlslPath = Join-Path $temporaryRoot "portable.generated.glsl"
    $definitionText = [IO.File]::ReadAllText(
        (Join-Path $repoRoot "src\vulkan\raytracing\RtSceneAbi.def")).Replace("`r`n", "`n")
    [IO.File]::WriteAllText(
        $lfDefinitionPath, $definitionText, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText(
        $crlfDefinitionPath, $definitionText.Replace("`n", "`r`n"),
        [Text.UTF8Encoding]::new($false))
    & (Join-Path $repoRoot "tools\generate-rt-scene-abi.ps1") `
        -DefinitionPath $lfDefinitionPath `
        -CpuOutputPath $portableCpuPath `
        -GlslOutputPath $portableGlslPath
    & (Join-Path $repoRoot "tools\generate-rt-scene-abi.ps1") `
        -DefinitionPath $crlfDefinitionPath `
        -CpuOutputPath $portableCpuPath `
        -GlslOutputPath $portableGlslPath `
        -Check

    & (Join-Path $repoRoot "tools\generate-rt-scene-abi.ps1") -Check
    $generatedGlsl = Get-Content -LiteralPath (Join-Path $repoRoot "shaders\raytracing\include\rt_scene_abi.generated.glsl") -Raw
    if ($generatedGlsl -notmatch 'binding = 11\) readonly buffer RtInstanceMetadataBuffer' -or
        $generatedGlsl -notmatch '#if !defined\(HORDE_RT_VARIANT_INSTRUMENTATION\) \|\| HORDE_RT_VARIANT_INSTRUMENTATION == 1' -or
        $generatedGlsl -notmatch 'binding = 22\) restrict buffer RtDielectricDiagnosticsBuffer' -or
        $generatedGlsl -notmatch '\} rtDielectricDiagnostics;\s*#endif') {
        throw "Dielectric diagnostics must retain binding 22 for compatibility/Diagnostic shaders and omit it in Shipping variants."
    }
    if ($generatedGlsl -notmatch 'uint primaryUnclosedVolumeCount;' -or
        $generatedGlsl -notmatch 'uint shadowUnclosedVolumeCount;' -or
        $generatedGlsl -notmatch 'uint primaryCertifiedClosedVolumeRecoveryCount;' -or
        $generatedGlsl -notmatch 'uint shadowCertifiedClosedVolumeRecoveryCount;' -or
        $generatedGlsl -notmatch 'uint certifiedClosedVolumeRecoveryReasonMask;' -or
        $generatedGlsl -notmatch 'kRtMaterialFlagCertifiedClosedVolume = 1024u;') {
        throw "Dielectric diagnostics must append independent primary and shadow unclosed-volume counters."
    }

    $definition = Get-Content -LiteralPath (Join-Path $repoRoot "src\vulkan\raytracing\RtSceneAbi.def") -Raw | ConvertFrom-Json
    if ($definition.schema -ne 1 -or $definition.bindings.dielectricDiagnostics -ne 22) {
        throw "RT scene ABI schema/binding-22 contract changed."
    }
    $diagnosticRecord = @($definition.records | Where-Object name -eq 'RtDielectricDiagnostics')
    if ($diagnosticRecord.Count -ne 1 -or $diagnosticRecord[0].size -ne 176 -or
        @($diagnosticRecord[0].fields).Count -ne 41) {
        throw "RtDielectricDiagnostics must remain a 176-byte record with 41 uint fields."
    }
    $definition | Add-Member -Force -NotePropertyName records -NotePropertyValue @(
        [ordered]@{
            name = "StaticRtVertex"; alignment = 16; size = 65
            fields = @(
                [ordered]@{ name = "position"; type = "float4"; offset = 0 },
                [ordered]@{ name = "normal"; type = "float4"; offset = 16 },
                [ordered]@{ name = "tangent"; type = "float4"; offset = 32 },
                [ordered]@{ name = "uv0"; type = "float4"; offset = 48 }
            )
        },
        [ordered]@{
            name = "RtInstanceMetadata"; alignment = 16; size = 32
            fields = @(
                [ordered]@{ name = "primitiveBase"; type = "uint"; offset = 0 },
                [ordered]@{ name = "primitiveCount"; type = "uint"; offset = 4 },
                [ordered]@{ name = "stableObjectId"; type = "uint"; offset = 8 },
                [ordered]@{ name = "flags"; type = "uint"; offset = 12 },
                [ordered]@{ name = "emitterIndex"; type = "uint"; offset = 16 },
                [ordered]@{ name = "assetIndex"; type = "uint"; offset = 20 },
                [ordered]@{ name = "reserved0"; type = "uint"; offset = 24 },
                [ordered]@{ name = "reserved1"; type = "uint"; offset = 28 }
            )
        },
        [ordered]@{
            name = "RtPrimitiveMetadata"; alignment = 16; size = 16
            fields = @(
                [ordered]@{ name = "vertexOffset"; type = "uint"; offset = 0 },
                [ordered]@{ name = "indexOffset"; type = "uint"; offset = 4 },
                [ordered]@{ name = "indexCount"; type = "uint"; offset = 8 },
                [ordered]@{ name = "materialIndex"; type = "uint"; offset = 12 }
            )
        },
        [ordered]@{
            name = "RtMaterialGpu"; alignment = 16; size = 112
            fields = @(
                [ordered]@{ name = "baseColorFactor"; type = "float4"; offset = 0 },
                [ordered]@{ name = "emissiveFactorStrength"; type = "float4"; offset = 16 },
                [ordered]@{ name = "metallicRoughnessOcclusionTransmission"; type = "float4"; offset = 32 },
                [ordered]@{ name = "iorThicknessAttenuationDistance"; type = "float4"; offset = 48 },
                [ordered]@{ name = "attenuationColor"; type = "float4"; offset = 64 },
                [ordered]@{ name = "textureLayers"; type = "uint4"; offset = 80 },
                [ordered]@{ name = "materialFlags"; type = "uint4"; offset = 96 }
            )
        }
    )
    $definitionPath = Join-Path $temporaryRoot "invalid-size.def"
    $definition | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $definitionPath -Encoding utf8
    $rejected = $false
    try {
        & (Join-Path $repoRoot "tools\generate-rt-scene-abi.ps1") `
            -DefinitionPath $definitionPath `
            -CpuOutputPath (Join-Path $temporaryRoot "invalid.generated.h") `
            -GlslOutputPath (Join-Path $temporaryRoot "invalid.generated.glsl")
    }
    catch {
        $expected = "RT scene ABI record 'StaticRtVertex' declares size 65 but its fields occupy 64 bytes."
        if ($_.Exception.Message -ne $expected) { throw "Expected '$expected', got '$($_.Exception.Message)'" }
        $rejected = $true
    }
    if (-not $rejected) { throw "ABI generator accepted a record size that disagrees with its declared fields." }

    $validDefinitionPath = Join-Path $temporaryRoot "valid.def"
    $validCpuPath = Join-Path $temporaryRoot "valid.generated.h"
    $validGlslPath = Join-Path $temporaryRoot "valid.generated.glsl"
    Copy-Item -LiteralPath (Join-Path $repoRoot "src\vulkan\raytracing\RtSceneAbi.def") `
        -Destination $validDefinitionPath
    & (Join-Path $repoRoot "tools\generate-rt-scene-abi.ps1") `
        -DefinitionPath $validDefinitionPath `
        -CpuOutputPath $validCpuPath `
        -GlslOutputPath $validGlslPath
    Add-Content -LiteralPath $validCpuPath -Value "// deliberate stale output"
    $staleRejected = $false
    try {
        & (Join-Path $repoRoot "tools\generate-rt-scene-abi.ps1") `
            -DefinitionPath $validDefinitionPath `
            -CpuOutputPath $validCpuPath `
            -GlslOutputPath $validGlslPath `
            -Check
    }
    catch {
        $expected = "RT scene ABI generated output is stale: $validCpuPath"
        if ($_.Exception.Message -ne $expected) { throw "Expected '$expected', got '$($_.Exception.Message)'" }
        $staleRejected = $true
    }
    if (-not $staleRejected) { throw "ABI generator freshness check accepted a deliberately stale CPU output." }
    Write-Output "RT scene ABI generator layout, freshness, and negative-staleness contracts passed"
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
