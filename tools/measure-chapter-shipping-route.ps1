param([Parameter(Mandatory=$true)][string]$Output,[switch]$Query,
 [ValidateSet('Debug','Release')][string]$Configuration='Release',[switch]$Foreground)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination=if([IO.Path]::IsPathRooted($Output)){$Output}else{Join-Path $root $Output}
foreach($path in @($destination,($destination+'.stdout.log'),($destination+'.stderr.log'),($destination+'.candidate.json'))){
 if(Test-Path -LiteralPath $path){throw 'Choose a fresh measurement directory and receipt prefix'}
}
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=Join-Path $root ('build/chapter-shipping-measurement/'+$Configuration+'/HordeLanternRT.exe')
$start.WorkingDirectory=$root;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($argument in @('--development-rescue-journey','--capture-showcase',$destination,
 '--development-checkpoint','chapter-kit-reunion','--measure-chapter-shipping-route')){$start.ArgumentList.Add($argument)}
if($Query){$start.ArgumentList.Add('--require-rayquery-compute')}
if($Foreground){$start.ArgumentList.Add('--measure-chapter-foreground')}
$assetRoot=Join-Path (Split-Path -Parent $start.FileName) 'assets'
if(-not (Test-Path -LiteralPath $assetRoot)){$assetRoot=Join-Path $root 'assets'}
foreach($model in @('models/enemies/meshy/skeleton_biped_merged_animations_v01.glb',
 'models/enemies/meshy/lich_placeholder_merged_animations_v01.glb')) {
 $bytes=[IO.File]::ReadAllBytes((Join-Path $assetRoot $model))
 if($bytes.Length -lt 200 -or [Text.Encoding]::ASCII.GetString($bytes,0,4) -ne 'glTF') {
  throw 'Measurement requires actual GLB payloads before launch, not missing assets or LFS pointers'
 }
}
$sourceHashes=@{}
foreach($source in @('src/platform/windows/DiagnosticWindow.cpp','src/platform/windows/WindowsChapterShippingMeasurement.inl',
 'src/platform/windows/WindowsChapterShippingLaunch.h','src/platform/windows/WindowsAssetRoot.h',
 'src/platform/windows/WindowsPresentInterval.h','CMakeLists.txt',
 'src/vulkan/raytracing/PresentableTinyRtScene.h','src/vulkan/raytracing/PresentableTinyRtScene.cpp',
 'src/gameplay/validation/ChapterRouteMeasurementClock.h',
 'src/gameplay/validation/ChapterRoundTripProbe.h')){
 $sourceHashes[$source]=(Get-FileHash -LiteralPath (Join-Path $root $source) -Algorithm SHA256).Hash.ToLowerInvariant()
}
@{scope=($Configuration+' CPU with Shipping High shaders; uncovered visible wall-clock scripted round-trip after staged Keeper death; zero static-region residency saving; not owner/phone acceptance');
 executableSha256=(Get-FileHash -LiteralPath $start.FileName -Algorithm SHA256).Hash.ToLowerInvariant();
 sources=$sourceHashes;backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'};
 maximumSimulationTicksPerRender=4;simulationHz=60;deadlineSeconds=180;
 foregroundRequested=[bool]$Foreground;
 configure='cmake -S . -B build/chapter-shipping-measurement -G "Visual Studio 17 2022" -A x64 -DHORDE_RT_INSTRUMENTATION_OVERRIDE=Shipping -DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=High -DHORDE_RT_CHAPTER_MEASUREMENT=ON'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath ($destination+'.candidate.json') -Encoding utf8
$process=[Diagnostics.Process]::Start($start)
$out=$process.StandardOutput.ReadToEndAsync();$err=$process.StandardError.ReadToEndAsync()
$timedOut=-not $process.WaitForExit(180000)
if($timedOut){$process.Kill();$process.WaitForExit()}
[IO.File]::WriteAllText($destination+'.stdout.log',$out.Result)
[IO.File]::WriteAllText($destination+'.stderr.log',$err.Result)
if($timedOut){throw 'Scoped measurement exceeded its180-second process deadline'}
Write-Output ("$Output measurement exit: "+$process.ExitCode)
if($process.ExitCode -ne 0){exit $process.ExitCode}
