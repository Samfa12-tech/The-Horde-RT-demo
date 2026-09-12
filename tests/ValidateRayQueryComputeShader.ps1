param(
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($VulkanSdk)) { $VulkanSdk = 'C:\VulkanSDK\1.4.350.0' }
$repo = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repo 'shaders/raytracing/minimal.comp'
if (-not (Test-Path -LiteralPath $source)) { throw 'Missing hardware ray-query compute entry point.' }
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repo 'build/rayquery-shader-contract'
}
[void](New-Item -ItemType Directory -Path $OutputDirectory -Force)
$compiler = Join-Path $VulkanSdk 'Bin/glslangValidator.exe'
$optimizer = Join-Path $VulkanSdk 'Bin/spirv-opt.exe'
$validator = Join-Path $VulkanSdk 'Bin/spirv-val.exe'
$disassembler = Join-Path $VulkanSdk 'Bin/spirv-dis.exe'
foreach ($tool in @($compiler, $optimizer, $validator, $disassembler)) {
    if (-not (Test-Path -LiteralPath $tool)) { throw "Missing shader tool: $tool" }
}

# Exercise the real compiler/artifacts: dropping hardware traversal, importing a
# ray-pipeline capability or leaving Shipping diagnostics must fail this test.
foreach ($instrumentation in 0..1) {
    foreach ($quality in 0..1) {
        foreach ($material in 0..1) {
            $key = "compute-i$instrumentation-q$quality-m$material"
            $raw = Join-Path $OutputDirectory "$key.raw.spv"
            $spirv = Join-Path $OutputDirectory "$key.spv"
            $assemblyPath = Join-Path $OutputDirectory "$key.spvasm"
            $compileOptimization = @()
            if ($material -eq 0) { $compileOptimization = @('-Os') }
            & $compiler -V --target-env vulkan1.2 -S comp @compileOptimization `
                "-DHORDE_RT_VARIANT_INSTRUMENTATION=$instrumentation" `
                "-DHORDE_RT_VARIANT_QUALITY=$quality" `
                "-DHORDE_RT_VARIANT_MATERIAL=$material" -o $raw $source
            if ($LASTEXITCODE -ne 0) { throw "Compute compilation failed: $key" }
            # Match the established raygen policies: inline OpaqueFast, retain
            # GenericDielectric helper boundaries to avoid duplicating its loops.
            if ($material -eq 0) {
                & $optimizer -O $raw -o $spirv
            } else {
                & $optimizer --eliminate-dead-functions --eliminate-dead-code-aggressive `
                    --simplify-instructions --eliminate-dead-branches --cfg-cleanup $raw -o $spirv
            }
            if ($LASTEXITCODE -ne 0) { throw "Compute optimization failed: $key" }
            & $validator --target-env vulkan1.2 $spirv
            if ($LASTEXITCODE -ne 0) { throw "Compute SPIR-V validation failed: $key" }
            & $disassembler $spirv -o $assemblyPath
            if ($LASTEXITCODE -ne 0) { throw "Compute disassembly failed: $key" }
            $assembly = Get-Content -LiteralPath $assemblyPath -Raw
            foreach ($required in @('OpEntryPoint GLCompute', 'OpCapability RayQueryKHR',
                                     'OpRayQueryInitializeKHR', 'OpRayQueryProceedKHR', 'OpImageWrite')) {
                if ($assembly -notmatch [regex]::Escape($required)) {
                    throw "Missing $required in $key"
                }
            }
            if ($assembly -match 'OpCapability RayTracingKHR|OpTraceRayKHR|GL_EXT_ray_tracing') {
                throw "Ray-pipeline requirement leaked into $key"
            }
            $atomics = [regex]::Matches($assembly, '\bOpAtomic\w+').Count
            $hasDiagnosticBinding = $assembly -match '\bBinding 22\b'
            if ($instrumentation -eq 0 -and ($atomics -ne 0 -or $hasDiagnosticBinding)) {
                throw "Shipping diagnostic overhead in $key"
            }
            if ($instrumentation -eq 1 -and ($atomics -eq 0 -or -not $hasDiagnosticBinding)) {
                throw "Diagnostic control lost its counters in $key"
            }
            $hash = (Get-FileHash -LiteralPath $spirv -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($material -eq 0) { $opaqueHash = $hash }
            elseif ($hash -eq $opaqueHash) { throw "Opaque/generic material policy did not specialize: $key" }
            Write-Output "PASS $key bytes=$((Get-Item -LiteralPath $spirv).Length) atomics=$atomics sha256=$hash"
        }
    }
}
