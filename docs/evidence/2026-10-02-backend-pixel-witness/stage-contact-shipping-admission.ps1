param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if ((& git -C $repoRoot branch --show-current) -cne 'codex/horde-rtx-corrections') {
    throw 'Compile-only contact admission is restricted to the isolated branch.'
}
if (Test-Path -LiteralPath $OutputRoot) { throw 'Preserve completed results; use a new output root.' }
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
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
$budgetPath = Join-Path $repoRoot 'tools/raygen-variant-budgets.json'
$budgetHash = (Get-FileHash -LiteralPath $budgetPath).Hash
$budgets = Get-Content -LiteralPath $budgetPath -Raw | ConvertFrom-Json
$manifest = Get-RaygenVariantManifest
$definition = @($manifest.Variants | Where-Object name -CEQ 'shipping_high_generic_dielectric')
if ($definition.Count -ne 1) { throw 'Expected one High Shipping Generic policy.' }
$validator = Join-Path $VulkanSdk 'Bin/glslangValidator.exe'
$optimizer = Join-Path $VulkanSdk 'Bin/spirv-opt.exe'
$disassembler = Join-Path $VulkanSdk 'Bin/spirv-dis.exe'
$source = Join-Path $repoRoot 'shaders/raytracing/experimental/contact_shipping.rgen'
Invoke-RaygenVariantCompilation -VariantDefinition $definition[0] -OutputRoot $OutputRoot -Manifest $manifest
$stats = Get-Content -LiteralPath (Join-Path $OutputRoot 'shipping_high_generic_dielectric/raygen-stats.json') -Raw | ConvertFrom-Json
if ($stats.atomicInstructions -ne 0 -or $stats.hasDiagnosticsBinding) {
    throw 'Diagnostic atomics/readback binding leaked into the Shipping probe.'
}
$assembly = Get-Content -LiteralPath (Join-Path $OutputRoot 'shipping_high_generic_dielectric/minimal.rgen.spvasm') -Raw
if ($assembly -match '\b(?:backendWitness\w*|contactMathWitness|investigationInterfaces|investigationContact\w*)\b') {
    throw 'Fixed-pixel observer leaked into the Shipping probe.'
}
$sourcePath = Join-Path $repoRoot 'shaders/raytracing/experimental/contact_shipping.comp'
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
if ($computeDefinition.Count -ne 1) { throw 'Expected one matching compute policy.' }
$compiled = Invoke-ComputeVariantCompilation -Variant $computeDefinition[0] -RunRoot $OutputRoot `
    -Dependencies $dependencies -ManifestSha256 $computeManifest.Sha256
$computeAssembly = Get-Content -LiteralPath (Join-Path $OutputRoot "$($computeDefinition[0].key)/minimal.comp.spvasm") -Raw
if ($computeAssembly -match '\b(?:backendWitness\w*|contactMathWitness|investigationInterfaces|investigationContact\w*)\b') {
    throw 'Fixed-pixel observer leaked into the compute Shipping probe.'
}
$failures = @()
$limits = @($budgets.budgets | Where-Object key -CEQ $definition[0].name)
if ($limits.Count -ne 1) { throw 'Frozen pipeline cost limits are missing.' }
foreach ($metric in $budgets.metrics) {
    if ($metric -notin $stats.PSObject.Properties.Name -or $metric -notin $limits[0].max.PSObject.Properties.Name) {
        throw "Required cost metric is absent: $metric"
    }
    if ($stats.$metric -gt $limits[0].max.$metric) {
        $failures += [ordered]@{metric=$metric; actual=$stats.$metric; maximum=$limits[0].max.$metric}
    }
}
if ((Get-FileHash -LiteralPath $budgetPath).Hash -cne $budgetHash) { throw 'Frozen budgets changed.' }
[ordered]@{
    schema=1; candidateSource=(& git -C $repoRoot rev-parse HEAD).Trim(); investigationOnly=$true
    embedded=$false; hardwareExecution=$false; performanceEvidence=$false
    budgetSha256=$budgetHash.ToLowerInvariant(); pipelineStats=$stats; computeCatalogRow=$compiled.Row
    pipelineCostAdmitted=($failures.Count -eq 0); pipelineCostFailures=$failures
    nextStep=$(if ($failures.Count) {'STOP: no native/image run or production promotion; preserve cost failure.'}
               else {'Native/image acceptance remains required; this compilation is not promotion.'})
} | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $OutputRoot 'admission.json')
Write-Output "COMPILE-ONLY: two modules compiled/validated, no embedded artifact/catalog/budget writes; cost failures=$($failures.Count)."
