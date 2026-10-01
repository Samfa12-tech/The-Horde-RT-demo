param([Parameter(Mandatory)][string]$OutputRoot, [switch]$ReuseCompletedMatrix)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if ((& git -C $repoRoot branch --show-current) -cne 'codex/horde-high-row43-current') {
    throw 'This unadmitted experiment may only stage on its isolated investigation branch.'
}
$budgetPath = Join-Path $repoRoot 'tools/raygen-variant-budgets.json'
$budgetHash = (Get-FileHash -LiteralPath $budgetPath).Hash
$compilerScript = Join-Path $repoRoot 'tools/compile-raygen.ps1'
$VulkanSdk = 'C:/VulkanSDK/1.4.350.0'
$validator = Join-Path $VulkanSdk 'Bin/glslangValidator.exe'
$optimizer = Join-Path $VulkanSdk 'Bin/spirv-opt.exe'
$disassembler = Join-Path $VulkanSdk 'Bin/spirv-dis.exe'
$source = Join-Path $repoRoot 'shaders/raytracing/minimal.rgen'
if (-not $ReuseCompletedMatrix) { & $compilerScript -Matrix -OutputDirectory $OutputRoot }
# Reuse the existing compiler's catalog/publication functions without claiming
# its failed frozen cost gate passed. No production assertion/budget is edited.
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($compilerScript,[ref]$tokens,[ref]$parseErrors)
if ($parseErrors.Count) { throw 'Compiler script could not be parsed.' }
foreach ($definition in $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst]},$false)) {
    $body = $definition.Extent.Text
    if ($definition.Name -ceq 'New-RaygenVariantCatalog') {
        # In an AST-loaded function PSCommandPath has no file binding. Keep its
        # metadata hash pointed at the actual compiler, not this wrapper.
        $body = $body.Replace('$PSCommandPath','$compilerScript')
    }
    . ([scriptblock]::Create($body))
}
$manifest = Get-RaygenVariantManifest
if ($ReuseCompletedMatrix) {
    if (-not [IO.Path]::IsPathRooted($OutputRoot)) { throw 'Reuse requires an absolute external output path.' }
    $OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
    if ($OutputRoot.Equals($repoRoot,[StringComparison]::OrdinalIgnoreCase) -or
        $OutputRoot.StartsWith($repoRoot.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Candidate matrix must remain outside the repository.'
    }
    foreach ($variant in $manifest.Variants) {
        $root = Join-Path $OutputRoot $variant.name
        $stats = Get-Content (Join-Path $root 'raygen-stats.json') -Raw | ConvertFrom-Json
        if ($stats.manifestSha256 -cne $manifest.Sha256 -or
            $stats.compiledSpirvSha256 -cne (Get-RaygenFileSha256 -Path (Join-Path $root 'minimal.rgen.spv'))) {
            throw 'Completed candidate artifact identity changed.'
        }
        foreach ($dependency in $stats.dependencies) {
            if ($dependency.sha256 -cne (Get-RaygenDependencyHash -Path (Join-Path $repoRoot $dependency.path))) {
                throw 'Completed matrix no longer matches source; do not silently reuse it.'
            }
        }
    }
}
$catalog = New-RaygenVariantCatalog -Manifest $manifest -OutputRoot $OutputRoot
$catalog.generator.sha256 = Get-RaygenDependencyHash -Path $compilerScript
$temporaryCatalog = Join-Path $OutputRoot 'unadmitted-raygen-catalog.json'
Write-RaygenCatalogJson -Path $temporaryCatalog -Catalog $catalog
Publish-RaygenFrozenCatalogBundle -StagingRoot $OutputRoot -Manifest $manifest -ExpectedCatalogPath $temporaryCatalog `
    -ArtifactRoot (Join-Path $repoRoot 'src/vulkan/raytracing/variants') `
    -CatalogFile (Join-Path $repoRoot 'tools/raygen-variant-catalog.json')
& (Join-Path $repoRoot 'tools/GenerateRtPipelineVariantCatalog.ps1') -Write
& (Join-Path $repoRoot 'tools/GenerateRayQueryComputeVariants.ps1') -Write -OutputDirectory (Join-Path $OutputRoot 'compute')
if ((Get-FileHash -LiteralPath $budgetPath).Hash -cne $budgetHash) { throw 'Frozen budgets changed.' }
Write-Output 'INVESTIGATION ONLY: actual module identities staged; production frozen cost admission remains FAILED.'
