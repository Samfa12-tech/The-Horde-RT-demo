[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourceRoot,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$VulkanSdk = 'C:\VulkanSDK\1.4.350.0'
)

$ErrorActionPreference = 'Stop'
$SourceRoot = [IO.Path]::GetFullPath($SourceRoot)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $OutputDirectory) {
    throw "Refusing to reuse investigation output directory: $OutputDirectory"
}
$compiler = Join-Path $SourceRoot 'tools\compile-raygen.ps1'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Production baseline compiler is missing: $compiler"
}
$bin = Join-Path $VulkanSdk 'Bin'
$glslang = Join-Path $bin 'glslangValidator.exe'
$optimizer = Join-Path $bin 'spirv-opt.exe'
$validator = Join-Path $bin 'spirv-val.exe'
$disassembler = Join-Path $bin 'spirv-dis.exe'
foreach ($tool in @($glslang, $optimizer, $validator, $disassembler)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Shader tool is missing: $tool" }
}
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null

function Get-TextSha256([string]$Text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text)))).Replace('-', '').ToLowerInvariant()
    } finally { $sha.Dispose() }
}
function Get-FileSha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Get-ToolVersion([string]$Tool) {
    $lines = @(& $Tool --version 2>&1 | ForEach-Object { ([string]$_).Trim() } | Where-Object { $_ })
    if ($LASTEXITCODE -ne 0 -or $lines.Count -eq 0) { throw "Could not read tool version: $Tool" }
    return ($lines -join ' ')
}
function Invoke-Variant([string]$Key) {
    if ($Key -notin @('shipping_mobile_opaque_fast', 'diagnostic_mobile_opaque_fast')) {
        throw "Unsupported experiment variant: $Key"
    }
    $controlRoot = Join-Path $OutputDirectory "$Key-control"
    $retainedRoot = Join-Path $OutputDirectory "$Key-retained"
    $controlCompileLog = Join-Path $OutputDirectory "$Key-control-compile.log"
    & $compiler -Variant $Key -OutputDirectory $controlRoot -VulkanSdk $VulkanSdk *> $controlCompileLog
    if (-not $?) { throw "Normal frozen compiler control failed for $Key." }
    $controlVariantRoot = Join-Path $controlRoot $Key
    $preprocessed = Join-Path $controlVariantRoot 'minimal.rgen.preprocessed'
    $preprocessedHash = Get-FileSha256 $preprocessed
    $retainedModuleRoot = New-Item -ItemType Directory -Path $retainedRoot
    $rawModule = Join-Path $retainedRoot 'minimal.rgen.raw.spv'
    $module = Join-Path $retainedRoot 'minimal.rgen.spv'
    $assembly = Join-Path $retainedRoot 'minimal.rgen.spvasm'
    $glslangLog = Join-Path $retainedRoot 'glslang.log'
    $optimizerLog = Join-Path $retainedRoot 'spirv-opt.log'
    $validationLog = Join-Path $retainedRoot 'spirv-val.log'
    $disassemblyLog = Join-Path $retainedRoot 'spirv-dis.log'

    & $glslang -V --target-env vulkan1.2 -S rgen -o $rawModule $preprocessed *> $glslangLog
    if ($LASTEXITCODE -ne 0) { throw "Retained glslang compile failed for $Key (exit $LASTEXITCODE)." }
    & $optimizer --eliminate-dead-functions --eliminate-dead-code-aggressive `
        --simplify-instructions --eliminate-dead-branches --cfg-cleanup $rawModule -o $module *> $optimizerLog
    if ($LASTEXITCODE -ne 0) { throw "Retained SPIR-V optimization failed for $Key (exit $LASTEXITCODE)." }
    & $validator --target-env vulkan1.2 $module *> $validationLog
    if ($LASTEXITCODE -ne 0) { throw "SPIR-V validation failed for $Key (exit $LASTEXITCODE)." }
    & $disassembler $module -o $assembly *> $disassemblyLog
    if ($LASTEXITCODE -ne 0) { throw "SPIR-V disassembly failed for $Key (exit $LASTEXITCODE)." }

    $text = [IO.File]::ReadAllText($assembly)
    $moduleBytes = [IO.File]::ReadAllBytes($module)
    if (($moduleBytes.Length % 4) -ne 0) { throw "Retained module is not word-aligned: $Key" }
    $wordCount = [int]($moduleBytes.Length / 4)
    $opcodePattern = '(?m)^\s*(?:%\S+\s*=\s*)?Op\w+'
    $atomicPattern = '\bOpAtomic(?:Load|Store|Exchange|CompareExchange|CompareExchangeWeak|IIncrement|IDecrement|IAdd|ISub|SMin|UMin|SMax|UMax|And|Or|Xor|FMin|FMax|FAdd|FMinEXT|FMaxEXT|FAddEXT)\b'
    $hasBinding22 = [regex]::IsMatch($text, '(?m)OpDecorate\s+%\S+\s+Binding\s+22\b')
    $stats = [ordered]@{
        key = $Key
        preprocessedSource = $preprocessed
        preprocessedSourceSha256 = $preprocessedHash
        sourceIsExactOrdinaryCompilerPreprocess = $true
        controlModuleSha256 = Get-FileSha256 (Join-Path $controlVariantRoot 'minimal.rgen.spv')
        retainedRawSha256 = Get-FileSha256 $rawModule
        retainedModuleSha256 = Get-FileSha256 $module
        retainedBytes = $moduleBytes.Length
        retainedWords = $wordCount
        instructions = ([regex]::Matches($text, $opcodePattern)).Count
        branchOperations = ([regex]::Matches($text, '\bOp(?:Branch|BranchConditional|Switch)\b')).Count
        loops = ([regex]::Matches($text, '\bOpLoopMerge\b')).Count
        selectionMerges = ([regex]::Matches($text, '\bOpSelectionMerge\b')).Count
        functions = ([regex]::Matches($text, '\bOpFunction\b')).Count
        functionCalls = ([regex]::Matches($text, '\bOpFunctionCall\b')).Count
        rayQueryInitializations = ([regex]::Matches($text, '\bOpRayQueryInitializeKHR\b')).Count
        atomicInstructions = ([regex]::Matches($text, $atomicPattern)).Count
        hasDiagnosticsBinding = $hasBinding22
        spirvVal = 'passed (Vulkan 1.2)'
        spirvDis = 'passed'
        glslangCompileArguments = @('-V', '--target-env', 'vulkan1.2', '-S', 'rgen', '-o', '<output>', '<exact-preprocessed-source>')
        spirvOptArguments = @('--eliminate-dead-functions', '--eliminate-dead-code-aggressive', '--simplify-instructions', '--eliminate-dead-branches', '--cfg-cleanup', '<input>', '-o', '<output>')
    }
    return [pscustomobject]$stats
}

$results = @(
    Invoke-Variant 'shipping_mobile_opaque_fast'
    Invoke-Variant 'diagnostic_mobile_opaque_fast'
)
$record = [ordered]@{
    schema = 1
    classification = 'nonproduction compiler-treatment investigation; no renderer or shader-source changes'
    sourceRoot = $SourceRoot
    sourceHead = (& git -C $SourceRoot rev-parse HEAD).Trim()
    recipe = $PSCommandPath
    recipeSha256 = Get-FileSha256 $PSCommandPath
    toolchain = [ordered]@{
        targetEnvironment = 'vulkan1.2'
        glslangValidator = Get-ToolVersion $glslang
        spirvOpt = Get-ToolVersion $optimizer
        spirvVal = Get-ToolVersion $validator
        spirvDis = Get-ToolVersion $disassembler
    }
    compilerTreatment = [ordered]@{
        baseline = 'exact normal compile-raygen.ps1 output for same variant and byte-identical preprocessed input'
        experiment = 'glslang without -Os followed by the five retained-function/dead-code passes recorded per variant'
        causalScope = 'combined compiler-treatment group; does not isolate glslang flag from SPIR-V optimizer pass changes'
    }
    variants = $results
}
$resultPath = Join-Path $OutputDirectory 'retained-investigation-recipe-results.json'
[IO.File]::WriteAllText($resultPath, (($record | ConvertTo-Json -Depth 12) + "`n"), [Text.UTF8Encoding]::new($false))
Write-Output "Investigation recipe receipt: $resultPath"
$results | Select-Object key, preprocessedSourceSha256, controlModuleSha256, retainedModuleSha256, retainedBytes, retainedWords, functions, functionCalls, rayQueryInitializations, atomicInstructions, hasDiagnosticsBinding | Format-Table -AutoSize
