[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$SkipAndroidBuild,
    [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repoRoot "android"
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot "build\asset-math-compiler-contract\$($Configuration.ToLowerInvariant())"
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($Arguments -join ' ')"
    }
}

function Get-CommandExecutable {
    param([Parameter(Mandatory = $true)][string]$Command)

    $match = [regex]::Match($Command, '^(?:"([^"]+)"|(\S+))')
    if (-not $match.Success) {
        throw "Could not parse compiler executable from compile command."
    }
    if ($match.Groups[1].Success) { return $match.Groups[1].Value }
    return $match.Groups[2].Value
}

function Get-ObjectPath {
    param(
        [Parameter(Mandatory = $true)]$CompileEntry,
        [Parameter(Mandatory = $true)][string]$SourceLeaf
    )

    $match = [regex]::Match($CompileEntry.command, '(?:^|\s)-o\s+(?:"([^"]+)"|(\S+))\s+-c(?:\s|$)')
    if (-not $match.Success) {
        throw "Could not parse the $SourceLeaf object path from the real compile command."
    }
    $relativeOrAbsolute = $(if ($match.Groups[1].Success) { $match.Groups[1].Value } else { $match.Groups[2].Value })
    if ([IO.Path]::IsPathRooted($relativeOrAbsolute)) {
        return [IO.Path]::GetFullPath($relativeOrAbsolute)
    }
    return [IO.Path]::GetFullPath((Join-Path $CompileEntry.directory $relativeOrAbsolute))
}

function Get-AuditedSymbol {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyString()][string[]]$AssemblyLines,
        [Parameter(Mandatory = $true)][string]$ExactOrContainedName,
        [Parameter(Mandatory = $true)][bool]$Exact
    )

    $matches = [Collections.Generic.List[object]]::new()
    $currentName = $null
    $currentLines = $null
    foreach ($line in $AssemblyLines) {
        $header = [regex]::Match($line, '^\s*[0-9a-fA-F]+\s+<(.+)>:\s*$')
        if ($header.Success) {
            if ($null -ne $currentLines) {
                $matches.Add([pscustomobject]@{ name = $currentName; lines = @($currentLines) })
            }
            $candidateName = $header.Groups[1].Value
            $wanted = $(if ($Exact) { $candidateName -ceq $ExactOrContainedName } else { $candidateName.Contains($ExactOrContainedName) })
            if ($wanted) {
                $currentName = $candidateName
                $currentLines = [Collections.Generic.List[string]]::new()
                $currentLines.Add($line)
            } else {
                $currentName = $null
                $currentLines = $null
            }
        } elseif ($null -ne $currentLines) {
            $currentLines.Add($line)
        }
    }
    if ($null -ne $currentLines) {
        $matches.Add([pscustomobject]@{ name = $currentName; lines = @($currentLines) })
    }
    if ($matches.Count -ne 1) {
        throw "Expected one audited symbol matching '$ExactOrContainedName', found $($matches.Count)."
    }
    return $matches[0]
}

function Get-Mnemonics {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string[]]$SymbolLines)

    $mnemonics = [Collections.Generic.List[string]]::new()
    foreach ($line in $SymbolLines) {
        $instruction = [regex]::Match($line, '^\s*[0-9a-fA-F]+:\s+([a-zA-Z][a-zA-Z0-9.]*)\b')
        if ($instruction.Success) {
            $mnemonics.Add($instruction.Groups[1].Value.ToLowerInvariant())
        }
    }
    return @($mnemonics)
}

if (-not $SkipAndroidBuild) {
    Push-Location $androidRoot
    try {
        $gradle = Join-Path $androidRoot "gradlew.bat"
        Invoke-CheckedNative $gradle ":app:buildCMake$Configuration[arm64-v8a]" "--console=plain"
    } finally {
        Pop-Location
    }
}

$compileDatabaseCandidates = Get-ChildItem -LiteralPath (Join-Path $androidRoot "app\.cxx\$Configuration") `
    -Filter compile_commands.json -File -Recurse | Sort-Object LastWriteTimeUtc -Descending
if ($compileDatabaseCandidates.Count -eq 0) {
    throw "No normal Gradle/CMake $Configuration ARM64 compile database exists. Run without -SkipAndroidBuild."
}

$selected = $null
foreach ($candidate in $compileDatabaseCandidates) {
    $entries = @(Get-Content -LiteralPath $candidate.FullName -Raw | ConvertFrom-Json)
    $gltfEntries = @($entries | Where-Object { $_.file -like '*\src\scene\assets\GltfDocument.cpp' -and $_.command -match '--target=aarch64' })
    $staticEntries = @($entries | Where-Object { $_.file -like '*\src\scene\assets\StaticMeshAsset.cpp' -and $_.command -match '--target=aarch64' })
    if ($gltfEntries.Count -eq 1 -and $staticEntries.Count -eq 1) {
        $selected = [pscustomobject]@{
            path = $candidate.FullName
            gltf = $gltfEntries[0]
            staticMesh = $staticEntries[0]
        }
        break
    }
}
if ($null -eq $selected) {
    throw "No normal Gradle/CMake ARM64 compile database contains exactly one instance of both audited production sources."
}

$compilerPath = Get-CommandExecutable $selected.gltf.command
if ($compilerPath -cne (Get-CommandExecutable $selected.staticMesh.command)) {
    throw "The audited production sources were not compiled by the same compiler."
}
$objdumpPath = Join-Path (Split-Path -Parent $compilerPath) "llvm-objdump.exe"
if (-not (Test-Path -LiteralPath $objdumpPath -PathType Leaf)) {
    throw "Could not find llvm-objdump beside the real Android compiler: $objdumpPath"
}

$gltfObject = Get-ObjectPath $selected.gltf "GltfDocument.cpp"
$staticObject = Get-ObjectPath $selected.staticMesh "StaticMeshAsset.cpp"
foreach ($objectPath in @($gltfObject, $staticObject)) {
    if (-not (Test-Path -LiteralPath $objectPath -PathType Leaf)) {
        throw "Normal Gradle/CMake object is missing: $objectPath"
    }
}

$compilerVersion = (& $compilerPath --version) -join "`n"
if ($LASTEXITCODE -ne 0) { throw "Could not query Android compiler version." }
$gltfAssemblyPath = Join-Path $OutputDirectory "GltfDocument.arm64.disassembly.txt"
$staticAssemblyPath = Join-Path $OutputDirectory "StaticMeshAsset.arm64.disassembly.txt"
$gltfAssembly = @(& $objdumpPath --disassemble --demangle --no-show-raw-insn $gltfObject)
if ($LASTEXITCODE -ne 0) { throw "llvm-objdump failed for $gltfObject" }
$staticAssembly = @(& $objdumpPath --disassemble --demangle --no-show-raw-insn $staticObject)
if ($LASTEXITCODE -ne 0) { throw "llvm-objdump failed for $staticObject" }
$gltfAssembly | Set-Content -LiteralPath $gltfAssemblyPath -Encoding utf8
$staticAssembly | Set-Content -LiteralPath $staticAssemblyPath -Encoding utf8

$symbolDefinitions = @(
    [pscustomobject]@{ key = "cgltf_node_transform_local"; assembly = $gltfAssembly; query = "cgltf_node_transform_local"; exact = $true; requireSub = $true },
    [pscustomobject]@{ key = "cgltf_node_transform_world"; assembly = $gltfAssembly; query = "cgltf_node_transform_world"; exact = $true; requireSub = $false },
    [pscustomobject]@{ key = "StaticMeshAsset::TransformPoint"; assembly = $staticAssembly; query = "::TransformPoint("; exact = $false; requireSub = $false }
)
$contractedPattern = '^(?:fmadd|fmsub|fnmadd|fnmsub|fmla|fmls)(?:\.[a-z0-9]+)?$'
$symbolEvidence = [Collections.Generic.List[object]]::new()
$failures = [Collections.Generic.List[string]]::new()
foreach ($definition in $symbolDefinitions) {
    $symbol = Get-AuditedSymbol $definition.assembly $definition.query $definition.exact
    $mnemonics = @(Get-Mnemonics $symbol.lines)
    $contracted = @($mnemonics | Where-Object { $_ -match $contractedPattern })
    $multiplyCount = @($mnemonics | Where-Object { $_ -match '^fmul(?:\.|$)' }).Count
    $addCount = @($mnemonics | Where-Object { $_ -match '^fadd(?:\.|$)' }).Count
    $subtractCount = @($mnemonics | Where-Object { $_ -match '^fsub(?:\.|$)' }).Count
    if ($contracted.Count -ne 0) {
        $failures.Add("$($definition.key) contains contracted instructions: $($contracted -join ', ')")
    }
    if ($multiplyCount -eq 0 -or $addCount -eq 0) {
        $failures.Add("$($definition.key) does not prove separate multiply/add instructions.")
    }
    if ($definition.requireSub -and $subtractCount -eq 0) {
        $failures.Add("$($definition.key) does not prove a separate subtract instruction.")
    }
    $symbolEvidence.Add([ordered]@{
        logicalName = $definition.key
        objectSymbol = $symbol.name
        instructionCount = $mnemonics.Count
        separateMultiplyCount = $multiplyCount
        separateAddCount = $addCount
        separateSubtractCount = $subtractCount
        contractedInstructions = @($contracted)
    })
}

$requiredFlags = @("-fno-fast-math", "-ffp-contract=off")
foreach ($entry in @($selected.gltf, $selected.staticMesh)) {
    foreach ($requiredFlag in $requiredFlags) {
        if ($entry.command -notmatch "(?<!\S)$([regex]::Escape($requiredFlag))(?!\S)") {
            $failures.Add("$([IO.Path]::GetFileName($entry.file)) compile command is missing $requiredFlag.")
        }
    }
    if ($entry.command -match '(?<!\S)-ffast-math(?!\S)' -or
        $entry.command -match '(?<!\S)-ffp-contract=(?:fast|on)(?!\S)') {
        $failures.Add("$([IO.Path]::GetFileName($entry.file)) compile command enables contraction or fast math.")
    }
}

$evidence = [ordered]@{
    schemaVersion = 1
    passed = ($failures.Count -eq 0)
    configuration = $Configuration
    compileDatabase = $selected.path
    compilerPath = $compilerPath
    compilerVersion = $compilerVersion
    objdumpPath = $objdumpPath
    sources = @(
        [ordered]@{
            source = $selected.gltf.file
            object = $gltfObject
            objectSha256 = (Get-FileHash -LiteralPath $gltfObject -Algorithm SHA256).Hash.ToLowerInvariant()
            compileCommand = $selected.gltf.command
            disassembly = $gltfAssemblyPath
        },
        [ordered]@{
            source = $selected.staticMesh.file
            object = $staticObject
            objectSha256 = (Get-FileHash -LiteralPath $staticObject -Algorithm SHA256).Hash.ToLowerInvariant()
            compileCommand = $selected.staticMesh.command
            disassembly = $staticAssemblyPath
        }
    )
    symbols = @($symbolEvidence)
    failures = @($failures)
}
$evidencePath = Join-Path $OutputDirectory "asset-math-compiler-contract.json"
$evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $evidencePath -Encoding utf8

if ($failures.Count -ne 0) {
    throw "Strict ARM64 asset-math compiler contract failed. Evidence: $evidencePath`n$($failures -join "`n")"
}

Write-Host "Strict ARM64 asset-math compiler contract passed."
Write-Host "Evidence: $evidencePath"
