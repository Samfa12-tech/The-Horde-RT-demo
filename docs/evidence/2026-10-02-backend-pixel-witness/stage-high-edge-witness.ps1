param([Parameter(Mandatory)][string]$OutputRoot, [switch]$ResumeCompletedPipeline)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if ((& git -C $repoRoot branch --show-current) -cne 'codex/horde-rtx-corrections') {
    throw 'The fixed-pixel observer is isolated and must never stage on engineering.'
}
if (Test-Path -LiteralPath $OutputRoot) {
    if (-not $ResumeCompletedPipeline) { throw 'Preserve completed artifacts; use a new staging directory.' }
} else { New-Item -ItemType Directory -Path $OutputRoot | Out-Null }
$budgetHash = (Get-FileHash (Join-Path $repoRoot 'tools/raygen-variant-budgets.json')).Hash
$priorPipeline = Get-Content (Join-Path $repoRoot 'tools/raygen-variant-catalog.json') -Raw | ConvertFrom-Json
$priorCompute = Get-Content (Join-Path $repoRoot 'tools/rayquery-variant-catalog.json') -Raw | ConvertFrom-Json
$VulkanSdk = 'C:/VulkanSDK/1.4.350.0'
$compilerScript = Join-Path $repoRoot 'tools/compile-raygen.ps1'
$computeScript = Join-Path $repoRoot 'tools/GenerateRayQueryComputeVariants.ps1'
foreach ($script in @($compilerScript, $computeScript)) {
    $tokens = $null; $errors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile($script, [ref]$tokens, [ref]$errors)
    if ($errors.Count) { throw 'Compiler functions could not be parsed.' }
    foreach ($definition in $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst]}, $false)) {
        $body = $definition.Extent.Text.Replace('$PSCommandPath',
            $(if ($script -ceq $compilerScript) { '$compilerScript' } else { '$computeScript' }))
        . ([scriptblock]::Create($body))
    }
}
$validator = Join-Path $VulkanSdk 'Bin/glslangValidator.exe'
$optimizer = Join-Path $VulkanSdk 'Bin/spirv-opt.exe'
$disassembler = Join-Path $VulkanSdk 'Bin/spirv-dis.exe'
$source = Join-Path $repoRoot 'shaders/raytracing/minimal.rgen'
$manifest = Get-RaygenVariantManifest
$definition = @($manifest.Variants | Where-Object name -CEQ 'diagnostic_high_generic_dielectric')
if ($definition.Count -ne 1) { throw 'Expected one High Diagnostic Generic variant.' }
if ($ResumeCompletedPipeline) {
    $statsPath = Join-Path $OutputRoot "$($definition[0].name)/raygen-stats.json"
    $stats = Get-Content -LiteralPath $statsPath -Raw | ConvertFrom-Json
    foreach ($dependency in $stats.dependencies) {
        if ((Get-RaygenDependencyHash -Path (Join-Path $repoRoot $dependency.path)) -cne $dependency.sha256) {
            throw 'Completed pipeline dependencies changed; retained artifact cannot be reused.'
        }
    }
    Write-Output 'Reusing completed pipeline compile/validation after wrapper metadata repair; dependencies unchanged.'
} else {
    Invoke-RaygenVariantCompilation -VariantDefinition $definition[0] -OutputRoot $OutputRoot -Manifest $manifest
}
$singleManifest = [pscustomobject]@{Sha256=$manifest.Sha256; Variants=@($definition[0])}
$freshPipeline = New-RaygenVariantCatalog -Manifest $singleManifest -OutputRoot $OutputRoot
$priorPipeline.variants = @($priorPipeline.variants | ForEach-Object {
    if ($_.key -ceq $definition[0].name) { $freshPipeline.variants[0] } else { $_ }
})
Copy-Item -LiteralPath (Join-Path $OutputRoot "$($definition[0].name)/$($definition[0].name).inc") `
    -Destination (Join-Path $repoRoot "src/vulkan/raytracing/variants/$($definition[0].name).inc")
Write-RaygenCatalogJson -Path (Join-Path $repoRoot 'tools/raygen-variant-catalog.json') -Catalog $priorPipeline
& (Join-Path $repoRoot 'tools/GenerateRtPipelineVariantCatalog.ps1') -Write

$sourcePath = Join-Path $repoRoot 'shaders/raytracing/minimal.comp'
$manifestPath = Join-Path $repoRoot 'tools/raygen-variants.json'
$compiler = Join-Path $VulkanSdk 'Bin/glslangValidator.exe'
$validator = Join-Path $VulkanSdk 'Bin/spirv-val.exe'
$computeManifest = Read-ComputeManifest -Path $manifestPath
$active = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$paths = [Collections.Generic.List[string]]::new()
Add-ComputeShaderDependencies -Path $sourcePath -ActivePaths $active -SeenPaths $seen -Dependencies $paths
$dependencies = @($paths | ForEach-Object {
    [ordered]@{path=Get-ComputeRelativePath -Path $_; sha256=Get-ComputeCanonicalFileSha256 -Path $_}
} | Sort-Object path)
$computeDefinition = @($computeManifest.Variants | Where-Object policyKey -CEQ $definition[0].name)
if ($computeDefinition.Count -ne 1) { throw 'Expected one matching compute variant.' }
$compiled = Invoke-ComputeVariantCompilation -Variant $computeDefinition[0] -RunRoot $OutputRoot `
    -Dependencies $dependencies -ManifestSha256 $computeManifest.Sha256
$priorCompute.variants = @($priorCompute.variants | ForEach-Object {
    if ($_.key -ceq $compiled.Row.key) { $compiled.Row } else { $_ }
})
Copy-Item -LiteralPath $compiled.IncludePath -Destination (Join-Path $repoRoot $compiled.Row.artifactPath)
Write-ComputeCanonicalText -Path (Join-Path $repoRoot 'tools/rayquery-variant-catalog.json') `
    -Text (($priorCompute | ConvertTo-Json -Depth 16) + "`n")
Write-ComputeCanonicalText -Path (Join-Path $repoRoot 'src/vulkan/raytracing/RtRayQueryVariantCatalog.generated.h') `
    -Text (Get-ComputeGeneratedHeader -Rows $priorCompute.variants)
if ((Get-FileHash (Join-Path $repoRoot 'tools/raygen-variant-budgets.json')).Hash -cne $budgetHash) {
    throw 'Frozen production budgets changed.'
}
Write-Output 'UNADMITTED OBSERVER: compiled two changed modules; retained all fourteen other artifacts/provenance, with no fresh-compilation claim.'
