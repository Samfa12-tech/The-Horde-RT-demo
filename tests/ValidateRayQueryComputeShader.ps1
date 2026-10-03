[CmdletBinding()]
param(
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$generator = Join-Path $repoRoot 'tools\GenerateRayQueryComputeVariants.ps1'
if (-not (Test-Path -LiteralPath $generator -PathType Leaf))
{
    throw 'Missing frozen hardware ray-query compute variant generator.'
}

$ownsOutputDirectory = [string]::IsNullOrWhiteSpace($OutputDirectory)
if ($ownsOutputDirectory)
{
    $ownedOutputParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
    $OutputDirectory = Join-Path $ownedOutputParent `
        ('horde-rt-rayquery-compute-check-' + [guid]::NewGuid().ToString('N'))
}

$catalogPath = Join-Path $repoRoot 'tools\rayquery-variant-catalog.json'
$expectedKeys = @(
    'rayquery_compute_diagnostic_high_generic_dielectric',
    'rayquery_compute_diagnostic_high_opaque_fast',
    'rayquery_compute_diagnostic_mobile_generic_dielectric',
    'rayquery_compute_diagnostic_mobile_opaque_fast',
    'rayquery_compute_shipping_high_generic_dielectric',
    'rayquery_compute_shipping_high_opaque_fast',
    'rayquery_compute_shipping_mobile_generic_dielectric',
    'rayquery_compute_shipping_mobile_opaque_fast')
$frozenPaths = @($expectedKeys | ForEach-Object {
    Join-Path $repoRoot "src\vulkan\raytracing\variants\$_.inc"
}) + @(
    $catalogPath,
    (Join-Path $repoRoot 'src\vulkan\raytracing\RtRayQueryVariantCatalog.generated.h'))
$beforeHashes = @{}
foreach ($path in $frozenPaths)
{
    if (-not (Test-Path -LiteralPath $path -PathType Leaf))
    {
        throw "Missing frozen hardware ray-query compute artifact: $path"
    }
    $beforeHashes[$path] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
}

try
{
    & $generator -Check -VulkanSdk $VulkanSdk -OutputDirectory $OutputDirectory
    if ($LASTEXITCODE -ne 0)
    {
        throw "Frozen hardware ray-query compute variant validation failed with exit code $LASTEXITCODE."
    }

    foreach ($path in $frozenPaths)
    {
        $afterHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ($afterHash -cne $beforeHashes[$path])
        {
            throw "Read-only compute variant validation modified a frozen artifact: $path"
        }
    }

    $catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
    $actualKeys = @($catalog.variants | ForEach-Object key)
    if (($actualKeys -join "`n") -cne ($expectedKeys -join "`n"))
    {
        throw 'Frozen compute catalog must contain the exact eight prefixed keys in canonical order.'
    }
    if ($catalog.target.stage -cne 'comp' -or
        $catalog.target.executionModel -cne 'GLCompute' -or
        $catalog.target.executionBackend -cne 'RayQueryCompute' -or
        $catalog.target.hardwareTraversal -cne 'RayQueryKHR')
    {
        throw 'Frozen compute catalog lost its hardware RayQuery GLCompute execution identity.'
    }
    foreach ($instrumentation in @('Shipping', 'Diagnostic'))
    {
        foreach ($quality in @('Mobile', 'High'))
        {
            $pair = @($catalog.variants | Where-Object {
                $_.instrumentation -ceq $instrumentation -and $_.quality -ceq $quality
            })
            $opaque = @($pair | Where-Object material -CEQ 'OpaqueFast')
            $generic = @($pair | Where-Object material -CEQ 'GenericDielectric')
            if ($pair.Count -ne 2 -or $opaque.Count -ne 1 -or $generic.Count -ne 1 -or
                $opaque[0].spirvSha256 -ceq $generic[0].spirvSha256)
            {
                throw "Opaque/generic compute material policy did not specialize: $instrumentation/$quality"
            }
        }
    }
    Write-Output 'Hardware RayQuery compute frozen shader conformance passed for all eight policy modes.'
}
finally
{
    if ($ownsOutputDirectory -and (Test-Path -LiteralPath $OutputDirectory -PathType Container))
    {
        $resolvedOwnedParent = (Resolve-Path -LiteralPath $ownedOutputParent).Path
        $resolvedOutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
        $resolvedOutputParent = Split-Path -Parent $resolvedOutputDirectory
        $resolvedOutputLeaf = Split-Path -Leaf $resolvedOutputDirectory
        if ($resolvedOutputDirectory.Equals(
                $resolvedOwnedParent, [StringComparison]::OrdinalIgnoreCase) -or
            -not $resolvedOutputParent.Equals(
                $resolvedOwnedParent, [StringComparison]::OrdinalIgnoreCase) -or
            -not $resolvedOutputLeaf.StartsWith(
                'horde-rt-rayquery-compute-check-', [StringComparison]::Ordinal))
        {
            throw "Refusing to recursively remove compute test output outside its exact temporary parent: $resolvedOutputDirectory"
        }
        Remove-Item -LiteralPath $resolvedOutputDirectory -Recurse -Force
    }
}
