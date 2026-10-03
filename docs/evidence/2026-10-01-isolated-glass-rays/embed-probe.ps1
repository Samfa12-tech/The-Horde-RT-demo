param([Parameter(Mandatory=$true)][string]$ModuleRoot)
$ErrorActionPreference = 'Stop'
$expected = 'C:/Dev/tmp/horde-glass-isolated-20261001/source'
if ((Get-Location).Path.Replace('\','/').TrimEnd('/') -cne $expected) {
    throw 'Only the isolated external investigation source may be modified.'
}
# Same pure include-generation functions as the previous retained ray probe.
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $expected 'tools/compile-raygen.ps1'), [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Generator parse failed.' }
foreach ($name in @('Get-RaygenSpirvWords', 'Get-RaygenIncludeText')) {
    $function = $ast.Find({ param($n)
        $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name
    }, $true)
    if ($null -eq $function) { throw "Missing generation function $name" }
    . ([scriptblock]::Create($function.Extent.Text))
}
$key = 'diagnostic_mobile_generic_dielectric'
$root = Join-Path $ModuleRoot $key
$stats = Get-Content (Join-Path $root 'raygen-stats.json') -Raw | ConvertFrom-Json
$spirv = Get-RaygenSpirvWords (Join-Path $root 'minimal.rgen.spv')
$include = Get-RaygenIncludeText -Key $key -DependencySha256 $stats.dependencySha256 -Words $spirv.Words
[IO.File]::WriteAllText((Join-Path $expected "src/vulkan/raytracing/variants/$key.inc"),
                      $include, [Text.UTF8Encoding]::new($false))
$sha = [Security.Cryptography.SHA256]::Create()
try { $includeHash = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($include))).Replace('-', '').ToLowerInvariant() }
finally { $sha.Dispose() }
$catalog = Get-Content (Join-Path $expected 'tools/raygen-variant-catalog.json') -Raw | ConvertFrom-Json
$row = @($catalog.variants | Where-Object key -CEQ $key)[0]
foreach ($field in @('words','bytes','instructions','branchOperations','loops','selectionMerges','functions','functionCalls','rayQueryInitializations','atomicInstructions','dependencySha256','dependencies')) {
    $row.$field = $stats.$field
}
$row.spirvSha256 = $stats.compiledSpirvSha256
$row.includeSha256 = $includeHash
$probeCatalog = Join-Path $ModuleRoot 'investigation-only-catalog.json'
[IO.File]::WriteAllText($probeCatalog, ($catalog | ConvertTo-Json -Depth 20), [Text.UTF8Encoding]::new($false))
& (Join-Path $expected 'tools/GenerateRtPipelineVariantCatalog.ps1') -Write -CatalogPath $probeCatalog
if ($LASTEXITCODE) { throw 'Probe adapter generation failed.' }
$row | Select-Object key,spirvSha256,includeSha256,words,atomicInstructions | ConvertTo-Json
