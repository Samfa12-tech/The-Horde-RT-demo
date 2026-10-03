[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourceRoot,
    [Parameter(Mandatory = $true)][string]$RecipeResultsPath
)

$ErrorActionPreference = 'Stop'
$expectedRoot = 'C:\Dev\tmp\horde-opaque-retained-profile-20260930\source-eaf'
$SourceRoot = [IO.Path]::GetFullPath($SourceRoot)
if (-not $SourceRoot.Equals($expectedRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing unexpected source worktree: $SourceRoot"
}
$SourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
$RecipeResultsPath = (Resolve-Path -LiteralPath $RecipeResultsPath).Path
$recipe = 'C:\Dev\tmp\horde-opaque-retained-profile-20260930\compile-retained-investigation.ps1'
$catalogPath = Join-Path $SourceRoot 'tools\raygen-variant-catalog.json'
$headerPath = Join-Path $SourceRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h'
$adapterPath = Join-Path $SourceRoot 'tools\GenerateRtPipelineVariantCatalog.ps1'
$provenanceRoot = 'C:\Dev\tmp\horde-opaque-retained-profile-20260930\package-provenance'
if (Test-Path -LiteralPath $provenanceRoot) { throw "Refusing pre-existing provenance directory: $provenanceRoot" }
if ((& git -C $SourceRoot rev-parse HEAD).Trim() -cne 'eafbf8262442a82e0835edf5cf5306d718e64633') {
    throw 'Source worktree is not at the authorized eaf checkpoint.'
}
if ((& git -C $SourceRoot status --porcelain).Count -ne 0) { throw 'Source worktree must be clean before experimental staging.' }
foreach ($path in @($catalogPath, $headerPath, $adapterPath, $recipe)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required input is missing: $path" }
}

function Get-Sha256([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function Get-FileSha256([string]$Path) { return Get-Sha256 ([IO.File]::ReadAllBytes($Path)) }
function Get-CanonicalTextSha256([string]$Text) { return Get-Sha256 ([Text.UTF8Encoding]::new($false, $true).GetBytes($Text)) }
function Get-SpirvMetrics([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 20 -or ($bytes.Length % 4) -ne 0) { throw "Invalid SPIR-V module: $Path" }
    $disassembly = [IO.Path]::ChangeExtension($Path, '.spvasm')
    if (-not (Test-Path -LiteralPath $disassembly -PathType Leaf)) { throw "SPIR-V disassembly is missing: $disassembly" }
    $text = [IO.File]::ReadAllText($disassembly)
    $opcodePattern = '(?m)^\s*(?:%\S+\s*=\s*)?Op\w+'
    $atomicPattern = '\bOpAtomic(?:Load|Store|Exchange|CompareExchange|CompareExchangeWeak|IIncrement|IDecrement|IAdd|ISub|SMin|UMin|SMax|UMax|And|Or|Xor|FMin|FMax|FAdd|FMinEXT|FMaxEXT|FAddEXT)\b'
    return [pscustomobject]@{
        Bytes = $bytes.Length
        Words = [int]($bytes.Length / 4)
        Sha256 = Get-Sha256 $bytes
        Instructions = ([regex]::Matches($text, $opcodePattern)).Count
        BranchOperations = ([regex]::Matches($text, '\bOp(?:Branch|BranchConditional|Switch)\b')).Count
        Loops = ([regex]::Matches($text, '\bOpLoopMerge\b')).Count
        SelectionMerges = ([regex]::Matches($text, '\bOpSelectionMerge\b')).Count
        Functions = ([regex]::Matches($text, '\bOpFunction\b')).Count
        FunctionCalls = ([regex]::Matches($text, '\bOpFunctionCall\b')).Count
        RayQueryInitializations = ([regex]::Matches($text, '\bOpRayQueryInitializeKHR\b')).Count
        AtomicInstructions = ([regex]::Matches($text, $atomicPattern)).Count
        HasDiagnosticsBinding = [regex]::IsMatch($text, '(?m)OpDecorate\s+%\S+\s+Binding\s+22\b')
    }
}
function Write-RaygenInclude([string]$Path, [string]$Key, [string]$DependencyHash, [string]$SpirvPath) {
    $bytes = [IO.File]::ReadAllBytes($SpirvPath)
    $wordCount = [int]($bytes.Length / 4)
    $lines = [Collections.Generic.List[string]]::new()
    $lines.Add("// Raygen variant key: $Key")
    $lines.Add("// Raygen dependency SHA-256: $DependencyHash")
    for ($first = 0; $first -lt $wordCount; $first += 8) {
        $last = [Math]::Min($first + 7, $wordCount - 1)
        $wordTokens = for ($index = $first; $index -le $last; ++$index) {
            '0x{0:x8}u' -f [BitConverter]::ToUInt32($bytes, $index * 4)
        }
        $lines.Add('    ' + (($wordTokens -join ', ') + ','))
    }
    $text = [string]::Join("`n", $lines) + "`n"
    [IO.File]::WriteAllText($Path, $text, [Text.UTF8Encoding]::new($false))
    return $text
}

$receipt = Get-Content -LiteralPath $RecipeResultsPath -Raw | ConvertFrom-Json
if ($receipt.schema -ne 1 -or $receipt.sourceRoot -cne $SourceRoot -or
    $receipt.sourceHead -cne 'eafbf8262442a82e0835edf5cf5306d718e64633' -or
    $receipt.recipeSha256 -cne (Get-FileSha256 $recipe)) {
    throw 'Investigation recipe receipt does not match the authorized source or current recipe bytes.'
}
$recipeRows = @($receipt.variants)
if ($recipeRows.Count -ne 2) { throw 'Investigation receipt must contain exactly the two OpaqueFast variants.' }
$keys = @('shipping_mobile_opaque_fast', 'diagnostic_mobile_opaque_fast')
foreach ($key in $keys) {
    if (@($recipeRows | Where-Object key -CEQ $key).Count -ne 1) { throw "Investigation receipt is missing or duplicates $key." }
}

New-Item -ItemType Directory -Path $provenanceRoot | Out-Null
$originalCatalogCopy = Join-Path $provenanceRoot 'original-frozen-raygen-variant-catalog.json'
$originalHeaderCopy = Join-Path $provenanceRoot 'original-frozen-RtPipelineVariantCatalog.generated.h'
Copy-Item -LiteralPath $catalogPath -Destination $originalCatalogCopy
Copy-Item -LiteralPath $headerPath -Destination $originalHeaderCopy
$originalCatalog = Get-Content -LiteralPath $originalCatalogCopy -Raw | ConvertFrom-Json
$catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
$originalRows = @($originalCatalog.variants)
$rows = @($catalog.variants)
if ($rows.Count -ne 8 -or $originalRows.Count -ne 8) { throw 'Both original and experimental catalogs must contain eight variants.' }
$inputBytesByKey = @{}
$includeHashes = @{}
$dependencyHashes = @{}
$variantEvidence = @()

foreach ($key in $keys) {
    $variant = @($rows | Where-Object key -CEQ $key)
    $oldVariant = @($originalRows | Where-Object key -CEQ $key)
    $recipeRow = @($recipeRows | Where-Object key -CEQ $key)
    if ($variant.Count -ne 1 -or $oldVariant.Count -ne 1 -or $recipeRow.Count -ne 1) { throw "Cannot resolve unique catalog row for $key." }
    $variant = $variant[0]
    $oldVariant = $oldVariant[0]
    $recipeRow = $recipeRow[0]
    $modulePath = Join-Path (Split-Path -Parent $RecipeResultsPath) (Join-Path "$key-retained" 'minimal.rgen.spv')
    if (-not (Test-Path -LiteralPath $modulePath -PathType Leaf)) { throw "Recipe module is missing: $modulePath" }
    $module = Get-SpirvMetrics $modulePath
    if ($module.Sha256 -cne $recipeRow.retainedModuleSha256 -or $module.Bytes -ne $recipeRow.retainedBytes -or
        $module.Words -ne $recipeRow.retainedWords -or $module.AtomicInstructions -ne $recipeRow.atomicInstructions -or
        $module.HasDiagnosticsBinding -ne $recipeRow.hasDiagnosticsBinding) {
        throw "Recipe receipt does not match actual module bytes or module metadata: $key"
    }
    if ($key -ceq 'shipping_mobile_opaque_fast' -and
        ($module.AtomicInstructions -ne 0 -or $module.HasDiagnosticsBinding -or
         [regex]::IsMatch([IO.File]::ReadAllText([IO.Path]::ChangeExtension($modulePath, '.spvasm')), '\bOpImageRead\b'))) {
        throw 'Shipping OpaqueFast investigation module has diagnostics/atomic/image-read contamination.'
    }
    if ($key -ceq 'diagnostic_mobile_opaque_fast' -and
        ($module.AtomicInstructions -ne 30 -or -not $module.HasDiagnosticsBinding)) {
        throw 'Diagnostic OpaqueFast experiment differs from the reviewed actual 30-site/binding22 result.'
    }

    $includeDependencyIdentity = [string]::Join('|', @(
        'opaque-retained-investigation-v1'
        "key=$key"
        "normalPreprocessedSha256=$($recipeRow.preprocessedSourceSha256)"
        "normalCompilerModuleSha256=$($recipeRow.controlModuleSha256)"
        "investigationRecipeSha256=$($receipt.recipeSha256)"
        "retainedSpirvSha256=$($module.Sha256)"
    ))
    $dependencyHash = Get-CanonicalTextSha256 ($includeDependencyIdentity + "`n")
    $includePath = Join-Path $SourceRoot ([string]::Join([IO.Path]::DirectorySeparatorChar, [string[]]$variant.artifactPath.Split('/')))
    $includeText = Write-RaygenInclude -Path $includePath -Key $key -DependencyHash $dependencyHash -SpirvPath $modulePath
    $includeCanonicalHash = Get-CanonicalTextSha256 $includeText

    $variant.strategy = 'OpaqueRetainedInvestigation'
    $variant.shippingAllowed = $false
    $variant.dependencySha256 = $dependencyHash
    $variant.includeSha256 = $includeCanonicalHash
    $variant.spirvSha256 = $module.Sha256
    $variant.bytes = $module.Bytes
    $variant.words = $module.Words
    $variant.instructions = $module.Instructions
    $variant.branchOperations = $module.BranchOperations
    $variant.loops = $module.Loops
    $variant.selectionMerges = $module.SelectionMerges
    $variant.functions = $module.Functions
    $variant.functionCalls = $module.FunctionCalls
    $variant.rayQueryInitializations = $module.RayQueryInitializations
    $variant.atomicInstructions = $module.AtomicInstructions
    $variant.hasDiagnosticsBinding = $module.HasDiagnosticsBinding
    $variant.driverSafeFullyInlined = $module.Functions -eq 1 -and $module.FunctionCalls -eq 0
    $variant.boundedGenericFunctionsRetained = $module.Functions -gt 1 -and $module.FunctionCalls -gt 0 -and $module.RayQueryInitializations -le 3
    $variant.compiler = [ordered]@{
        glslangCompileArguments = @('-V', '--target-env', 'vulkan1.2', '-S', 'rgen', '-o', '<output>', '<exact-preprocessed-source>')
        spirvOptArguments = @('--eliminate-dead-functions', '--eliminate-dead-code-aggressive', '--simplify-instructions', '--eliminate-dead-branches', '--cfg-cleanup', '<input>', '-o', '<output>')
        spirvValArguments = @('--target-env', 'vulkan1.2', '<input>')
        spirvDisArguments = @('<input>', '-o', '<output>')
    }

    $variantEvidence += [pscustomobject]@{
        key = $key
        originalDependencySha256 = $oldVariant.dependencySha256
        preprocessedSourceSha256 = $recipeRow.preprocessedSourceSha256
        controlModuleSha256 = $recipeRow.controlModuleSha256
        retainedModuleSha256 = $module.Sha256
        retainedBytes = $module.Bytes
        retainedWords = $module.Words
        retainedAtomicInstructions = $module.AtomicInstructions
        retainedHasBinding22 = $module.HasDiagnosticsBinding
        includeSha256 = $includeCanonicalHash
        investigationDependencySha256 = $dependencyHash
    }
}

# Preserve every row not explicitly covered by this OpaqueFast profile.
foreach ($key in @($originalRows | ForEach-Object key | Where-Object { $_ -notin $keys })) {
    $before = ($originalRows | Where-Object key -CEQ $key | ConvertTo-Json -Depth 16 -Compress)
    $after = ($rows | Where-Object key -CEQ $key | ConvertTo-Json -Depth 16 -Compress)
    if ($before -cne $after) { throw "Unexpected catalog mutation outside the two OpaqueFast rows: $key" }
}
$catalog.generator = [ordered]@{
    path = $recipe
    interface = 'opaque-retained-investigation-v1'
    sha256 = $receipt.recipeSha256
}
$catalogText = (($catalog | ConvertTo-Json -Depth 20) -replace "`r`n", "`n") + "`n"
[IO.File]::WriteAllText($catalogPath, $catalogText, [Text.UTF8Encoding]::new($false))

& $adapterPath -Write -CatalogPath $catalogPath -OutputPath $headerPath
if (-not $?) { throw 'Existing catalog adapter rejected experimental catalog while generating its C++ adapter.' }
& $adapterPath -Check -CatalogPath $catalogPath -OutputPath $headerPath
if (-not $?) { throw 'Existing catalog adapter failed its exact experimental-catalog check.' }

$preparationReceipt = [ordered]@{
    schema = 1
    classification = 'nonproduction experimental RT raygen package; no promotion or performance claim'
    sourceRoot = $SourceRoot
    sourceHead = 'eafbf8262442a82e0835edf5cf5306d718e64633'
    externalCompilerRecipe = $recipe
    externalCompilerRecipeSha256 = $receipt.recipeSha256
    catalogPath = $catalogPath
    catalogSha256 = Get-FileSha256 $catalogPath
    generatedAdapterPath = $headerPath
    generatedAdapterSha256 = Get-FileSha256 $headerPath
    originalFrozenCatalog = $originalCatalogCopy
    originalFrozenCatalogSha256 = Get-FileSha256 $originalCatalogCopy
    originalFrozenAdapter = $originalHeaderCopy
    originalFrozenAdapterSha256 = Get-FileSha256 $originalHeaderCopy
    adapterCommands = @(
        'tools/GenerateRtPipelineVariantCatalog.ps1 -Write -CatalogPath tools/raygen-variant-catalog.json -OutputPath src/vulkan/raytracing/RtPipelineVariantCatalog.generated.h'
        'tools/GenerateRtPipelineVariantCatalog.ps1 -Check -CatalogPath tools/raygen-variant-catalog.json -OutputPath src/vulkan/raytracing/RtPipelineVariantCatalog.generated.h'
    )
    adapterCheck = 'passed; unchanged adapter validates root schema, actual include words/hashes, and emitted C++ header'
    variantEvidence = $variantEvidence
    interpretation = @(
        'The frozen production compiler/manifest/budget gates were not edited or weakened; the experimental compiler recipe is distinct and external.'
        'The experimental catalog retains wire-compatible frozen schema/status because the unchanged adapter requires it; its generator identity and strategy label explicitly identify this nonproduction investigation.'
        'The Diagnostic retained module has 30 static atomic instructions and binding 22; the normal Diagnostic/Mobile control has 5. No runtime counter equivalence is asserted.'
    )
}
$receiptPath = Join-Path $provenanceRoot 'experimental-bundle-preparation.json'
[IO.File]::WriteAllText($receiptPath, (($preparationReceipt | ConvertTo-Json -Depth 16) + "`n"), [Text.UTF8Encoding]::new($false))
Write-Output "Experimental bundle receipt: $receiptPath"
Write-Output "Catalog SHA-256: $($preparationReceipt.catalogSha256)"
Write-Output "Generated adapter SHA-256: $($preparationReceipt.generatedAdapterSha256)"
