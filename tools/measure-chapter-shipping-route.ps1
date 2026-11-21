param([Parameter(Mandatory=$true)][string]$Output,[switch]$Query,
 [ValidateSet('Debug','Release')][string]$Configuration='Release',[switch]$Foreground,[string]$Executable,
 [string]$BellwetherResidentFixture)
$ErrorActionPreference='Stop'
$doorway=-not [string]::IsNullOrWhiteSpace($BellwetherResidentFixture)
if($doorway -and ($Configuration -ne 'Debug' -or $Foreground)) {
 throw 'The Bellwether fixture is a hidden Debug GPU and resource measurement.'
}
$deadlineSeconds=if($doorway){300}else{180}
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination=if([IO.Path]::IsPathRooted($Output)){$Output}else{Join-Path $root $Output}
foreach($path in @($destination,($destination+'.stdout.log'),($destination+'.stderr.log'),($destination+'.candidate.json'))){
 if(Test-Path -LiteralPath $path){throw 'Choose a fresh measurement directory and receipt prefix'}
}
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=if($Executable){(Resolve-Path -LiteralPath $Executable).Path}else{Join-Path $root ('build/chapter-shipping-measurement/'+$Configuration+'/HordeLanternRT.exe')}
$start.WorkingDirectory=$root;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($argument in @('--development-rescue-journey','--capture-showcase',$destination,
 '--development-checkpoint')){$start.ArgumentList.Add($argument)}
$fixtureHashes=$null
if($doorway) {
 $fixture=(Resolve-Path -LiteralPath $BellwetherResidentFixture).Path
 foreach($argument in @('bellwether-repeat-visit','--measure-bellwether-doorway','--capture-hidden',
  '--chapter-resident-fixture',$fixture)){$start.ArgumentList.Add($argument)}
 $fixtureHashes=@{}
 $rolePrefixes=@('')
 if(Test-Path -LiteralPath (Join-Path $fixture 'tavern-keeper') -PathType Container) {
  $rolePrefixes=@('tavern-keeper/','record-keeper/')
 }
 foreach($prefix in $rolePrefixes) {
  foreach($name in @('asset.manifest.json','resident.glb','base-color.windows.ktx2','normal.windows.ktx2','orm.windows.ktx2')) {
   $relative=$prefix+$name
   $path=Join-Path $fixture $relative
   if($name.EndsWith('.ktx2') -and !(Test-Path -LiteralPath $path -PathType Leaf)){continue}
   $fixtureHashes[$relative]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
  }
 }
}else{
 $start.ArgumentList.Add('chapter-kit-reunion')
 $start.ArgumentList.Add('--measure-chapter-shipping-route')
}
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
 'src/vulkan/raytracing/PresentableTinyRtSceneStaticGeometry.cpp','src/vulkan/raytracing/RtStaticMeshSlot.cpp',
 'src/vulkan/raytracing/PresentableTinyRtSceneRegionPages.cpp',
 'src/vulkan/raytracing/RtRegionGeometryPreparation.cpp','src/vulkan/raytracing/RtRegionPreparationRecovery.h',
 'src/vulkan/raytracing/RtRegionGeometrySource.cpp','src/world/streaming/RegionStreaming.cpp',
 'src/scene/CorridorStaticDetail.h','src/scene/CorridorStaticDetailFaces.h',
 'src/gameplay/simulation/GameSimulation.cpp','src/gameplay/simulation/StaticDetailLease.h',
 'src/gameplay/validation/ChapterRouteMeasurementClock.h','src/gameplay/validation/ChapterRouteHostPhases.h',
 'src/gameplay/validation/ChapterRoundTripProbe.h')){
 $sourceHashes[$source]=(Get-FileHash -LiteralPath (Join-Path $root $source) -Algorithm SHA256).Hash.ToLowerInvariant()
}
if($doorway) {
 foreach($source in @('src/gameplay/validation/BellwetherDoorwayProbe.h','src/gameplay/DevelopmentCheckpointSimulation.h',
  'src/gameplay/DevelopmentCheckpoints.h','src/gameplay/traversal/BellwetherTavernRoute.h')) {
  $sourceHashes[$source]=(Get-FileHash -LiteralPath (Join-Path $root $source) -Algorithm SHA256).Hash.ToLowerInvariant()
 }
}
$cachePath=Join-Path (Split-Path -Parent (Split-Path -Parent $start.FileName)) 'CMakeCache.txt'
$buildProfile=$null
if(Test-Path -LiteralPath $cachePath) {
 $policy=@{}
 foreach($line in [IO.File]::ReadLines($cachePath)) {
  if($line -match '^((?:CMAKE_(?:GENERATOR|BUILD_TYPE|CONFIGURATION_TYPES|HOME_DIRECTORY|CXX_COMPILER|CXX_FLAGS_RELEASE))|(?:HORDE_RT_(?:INSTRUMENTATION_OVERRIDE|DIELECTRIC_QUALITY_OVERRIDE|CHAPTER_MEASUREMENT|REGION_PAGE_PROTOTYPE_ROOT))):[^=]*=(.*)$') {
   $policy[$Matches[1]]=$Matches[2]
  }
 }
 $buildProfile=@{cachePath=$cachePath;cacheSha256=(Get-FileHash -LiteralPath $cachePath -Algorithm SHA256).Hash.ToLowerInvariant();configuration=$Configuration;policy=$policy}
}
$scope=if($doorway){'Debug CPU; hidden Bellwether doorway route with resident fixture; GPU and resource evidence only'}else{
 $Configuration+' CPU with Shipping High shaders; uncovered visible wall-clock scripted out/back and tomb backtrack after staged Keeper death; automatic static-detail lifetime; not owner/phone acceptance'
}
@{scope=$scope;
 executablePath=$start.FileName;buildProfile=$buildProfile;
 executableSha256=(Get-FileHash -LiteralPath $start.FileName -Algorithm SHA256).Hash.ToLowerInvariant();
 sources=$sourceHashes;residentFixtureHashes=$fixtureHashes;backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'};
 maximumSimulationTicksPerRender=4;simulationHz=60;deadlineSeconds=$deadlineSeconds;
 foregroundRequested=[bool]$Foreground;
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($destination+'.candidate.json') -Encoding utf8
$process=[Diagnostics.Process]::Start($start)
$outFile=[IO.FileStream]::new($destination+'.stdout.log',[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read,1,$true)
$errFile=[IO.FileStream]::new($destination+'.stderr.log',[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read,1,$true)
$out=$process.StandardOutput.BaseStream.CopyToAsync($outFile)
$err=$process.StandardError.BaseStream.CopyToAsync($errFile)
$timedOut=-not $process.WaitForExit($deadlineSeconds*1000)
if($timedOut){$process.Kill();$process.WaitForExit()}
try {$null=$out.GetAwaiter().GetResult();$null=$err.GetAwaiter().GetResult()}
finally {$outFile.Dispose();$errFile.Dispose()}
if($timedOut){throw "Scoped measurement exceeded its $deadlineSeconds second process deadline"}
Write-Output ("$Output measurement exit: "+$process.ExitCode)
if($process.ExitCode -ne 0){exit $process.ExitCode}
