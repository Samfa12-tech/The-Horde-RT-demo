param([Parameter(Mandatory=$true)][string]$ModuleRoot)
$ErrorActionPreference = 'Stop'
# Reuse only the repository's two pure generation functions. This temporary
# single-module capture is not a frozen production catalog or validation pass.
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path (Get-Location) 'tools/compile-raygen.ps1'), [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Generator parse failed.' }
foreach ($name in @('Get-RaygenSpirvWords', 'Get-RaygenIncludeText')) {
    $function = $ast.Find({ param($n)
        $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name
    }, $true)
    if ($null -eq $function) { throw "Missing generation function $name" }
    . ([scriptblock]::Create($function.Extent.Text))
}
$key = 'diagnostic_high_generic_dielectric'
$root = Join-Path $ModuleRoot $key
$stats = Get-Content (Join-Path $root 'raygen-stats.json') -Raw | ConvertFrom-Json
$spirv = Get-RaygenSpirvWords (Join-Path $root 'minimal.rgen.spv')
$include = Get-RaygenIncludeText -Key $key -DependencySha256 $stats.dependencySha256 -Words $spirv.Words
$includePath = Join-Path (Get-Location) "src/vulkan/raytracing/variants/$key.inc"
[IO.File]::WriteAllText($includePath, $include, [Text.UTF8Encoding]::new($false))
$sha = [Security.Cryptography.SHA256]::Create()
try { $includeHash = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($include))).Replace('-', '').ToLowerInvariant() }
finally { $sha.Dispose() }
$catalog = Get-Content tools/raygen-variant-catalog.json -Raw | ConvertFrom-Json
$row = @($catalog.variants | Where-Object key -EQ $key)[0]
foreach ($field in @('words','bytes','instructions','branchOperations','loops','selectionMerges','functions','functionCalls','rayQueryInitializations','atomicInstructions','dependencySha256','dependencies')) {
    $row.$field = $stats.$field
}
$row.spirvSha256 = $stats.compiledSpirvSha256
$row.includeSha256 = $includeHash
$probeCatalog = Join-Path $ModuleRoot 'investigation-only-catalog.json'
[IO.File]::WriteAllText($probeCatalog, ($catalog | ConvertTo-Json -Depth 20), [Text.UTF8Encoding]::new($false))
& ./tools/GenerateRtPipelineVariantCatalog.ps1 -Write -CatalogPath $probeCatalog
if ($LASTEXITCODE) { throw 'Probe catalog adapter failed.' }
$row | Select-Object key,spirvSha256,includeSha256,words,atomicInstructions | ConvertTo-Json
