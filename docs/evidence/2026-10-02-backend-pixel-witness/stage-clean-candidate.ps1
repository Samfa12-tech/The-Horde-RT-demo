param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if ((& git -C $repoRoot branch --show-current) -cne 'codex/horde-rtx-corrections') {
    throw 'Unadmitted clean artifacts may only stage on the isolated corrections branch.'
}
$budgetPath = Join-Path $repoRoot 'tools/raygen-variant-budgets.json'
$budgetHash = (Get-FileHash -LiteralPath $budgetPath).Hash
$compilerScript = Join-Path $repoRoot 'tools/compile-raygen.ps1'
$VulkanSdk = 'C:/VulkanSDK/1.4.350.0'
$validator = Join-Path $VulkanSdk 'Bin/glslangValidator.exe'
$optimizer = Join-Path $VulkanSdk 'Bin/spirv-opt.exe'
$disassembler = Join-Path $VulkanSdk 'Bin/spirv-dis.exe'
$source = Join-Path $repoRoot 'shaders/raytracing/minimal.rgen'
& $compilerScript -Matrix -OutputDirectory $OutputRoot
# Reuse the real compiler publication path for an explicitly unadmitted
# investigation, never change the frozen ceilings or stage on engineering.
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($compilerScript,[ref]$tokens,[ref]$parseErrors)
if ($parseErrors.Count) { throw 'Compiler script could not be parsed.' }
foreach ($definition in $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst]},$false)) {
    $body = $definition.Extent.Text
    if ($definition.Name -ceq 'New-RaygenVariantCatalog') {
        $body = $body.Replace('$PSCommandPath','$compilerScript')
    }
    . ([scriptblock]::Create($body))
}
$manifest = Get-RaygenVariantManifest
$catalog = New-RaygenVariantCatalog -Manifest $manifest -OutputRoot $OutputRoot
$catalog.generator.sha256 = Get-RaygenDependencyHash -Path $compilerScript
$temporaryCatalog = Join-Path $OutputRoot 'unadmitted-clean-candidate-catalog.json'
Write-RaygenCatalogJson -Path $temporaryCatalog -Catalog $catalog
Publish-RaygenFrozenCatalogBundle -StagingRoot $OutputRoot -Manifest $manifest -ExpectedCatalogPath $temporaryCatalog `
    -ArtifactRoot (Join-Path $repoRoot 'src/vulkan/raytracing/variants') `
    -CatalogFile (Join-Path $repoRoot 'tools/raygen-variant-catalog.json')
& (Join-Path $repoRoot 'tools/GenerateRtPipelineVariantCatalog.ps1') -Write
& (Join-Path $repoRoot 'tools/GenerateRayQueryComputeVariants.ps1') -Write -OutputDirectory (Join-Path $OutputRoot 'compute')
if ((Get-FileHash -LiteralPath $budgetPath).Hash -cne $budgetHash) { throw 'Frozen budgets changed.' }
Write-Output 'INVESTIGATION ONLY: clean candidate staged; no frozen-cost admission or production promotion.'
