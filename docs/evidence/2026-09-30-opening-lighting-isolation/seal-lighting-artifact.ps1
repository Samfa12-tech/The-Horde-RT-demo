param([Parameter(Mandatory=$true)][ValidateSet('shadow','volume')][string]$Mode)
$ErrorActionPreference='Stop'
$source=Join-Path $PSScriptRoot 'opening-lighting-source'
$destination=Join-Path $PSScriptRoot "opening-lighting-$Mode"
$expectedMode=if($Mode -ceq 'shadow'){1}else{2}
$header=Join-Path $source 'shaders/raytracing/include/rt_lighting_investigation.glsl'
if((Get-Content $header -Raw) -notmatch "(?m)^#define HORDE_LOCAL_LIGHTING_ISOLATION_MODE $expectedMode$"){
    throw 'Wrong isolation source mode'
}
if((git -C $source rev-parse HEAD).Trim() -cne '7a095c7836058f2129f1c3b08b1e8482671cbc72'){
    throw 'Wrong source base'
}
if(Test-Path (Join-Path $destination 'artifact.json')){throw 'Never overwrite an immutable artifact receipt'}
$apk=& (Join-Path $source 'tools/resolve-android-apk.ps1') -AndroidRoot (Join-Path $source 'android') -Variant benchmark
$retainedApk=Join-Path $destination "HordeLanternRT-nonshipping-fire-$Mode-isolation-arm64.apk"
Copy-Item -LiteralPath $apk -Destination $retainedApk
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead($retainedApk)
try{
    $entry=$zip.GetEntry('lib/arm64-v8a/libhorde_rt_probe_android.so')
    if($null -eq $entry){throw 'Missing exact ARM64 native entry'}
    $native=Join-Path $destination 'libhorde_rt_probe_android.so'
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$native,$false)
}finally{$zip.Dispose()}
$containment=& (Join-Path $source 'tools/InspectRtPipelineBundleContainment.ps1') -TargetPath $native -TargetPlatform Android -Instrumentation Shipping -Quality Mobile
$containment | Out-File -LiteralPath (Join-Path $destination 'containment.json') -Encoding utf8
Copy-Item -LiteralPath (Join-Path $source 'tools/raygen-variant-catalog.json') -Destination (Join-Path $destination 'raygen-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $source 'tools/rayquery-variant-catalog.json') -Destination (Join-Path $destination 'rayquery-variant-catalog.json')
Copy-Item -LiteralPath $header -Destination (Join-Path $destination 'rt_lighting_investigation.glsl')
git -C $source diff -- shaders/raytracing/include/rt_frame.glsl shaders/raytracing/include/rt_fire.glsl shaders/raytracing/include/rt_lighting.glsl | Out-File -LiteralPath (Join-Path $destination 'investigation.patch') -Encoding utf8
$artifact=[ordered]@{
    baseCommit='7a095c7836058f2129f1c3b08b1e8482671cbc72';sourceDirty=$true
    mode=$Mode;nonphysicalWorkloadIsolation=$true;publishable=$false
    apkPath=$retainedApk;apkSha256=(Get-FileHash $retainedApk).Hash.ToLowerInvariant()
    nativeSha256=(Get-FileHash $native).Hash.ToLowerInvariant()
    patchSha256=(Get-FileHash (Join-Path $destination 'investigation.patch')).Hash.ToLowerInvariant()
    headerSha256=(Get-FileHash $header).Hash.ToLowerInvariant()
    purpose='Opening cost attribution only; never a physical/quality-preserving candidate'
}
$artifact | ConvertTo-Json -Depth 4 | Out-File -LiteralPath (Join-Path $destination 'artifact.json') -Encoding utf8
$artifact | ConvertTo-Json -Depth 4
