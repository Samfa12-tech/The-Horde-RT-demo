param([Parameter(Mandatory=$true)][string]$Output,[switch]$Query,
 [ValidateSet('Debug','Release')][string]$Configuration='Release',[switch]$Foreground,[string]$Executable,
 [string]$BellwetherResidentFixture,[string]$BellwetherCampaignDirectory)
$ErrorActionPreference='Stop'
$residentRoundTrip=-not [string]::IsNullOrWhiteSpace($BellwetherCampaignDirectory)
$hasDoorwayFixture=-not [string]::IsNullOrWhiteSpace($BellwetherResidentFixture)
if($residentRoundTrip -and $hasDoorwayFixture) { throw 'Choose one Bellwether measurement fixture.' }
$doorway=$residentRoundTrip -or $hasDoorwayFixture
if($doorway -and ($Configuration -ne 'Debug' -or $Foreground)) {
 throw 'The Bellwether fixture is a hidden Debug GPU and resource measurement.'
}
$deadlineSeconds=if($residentRoundTrip){420}elseif($doorway){300}else{180}
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
 $start.ArgumentList.Add('bellwether-repeat-visit')
 $start.ArgumentList.Add('--capture-hidden')
 if($residentRoundTrip) {
  $fixture=(Resolve-Path -LiteralPath $BellwetherCampaignDirectory).Path
  $start.ArgumentList.Add('--measure-bellwether-resident-roundtrip')
  $start.ArgumentList.Add('--bellwether-campaign-dir')
  $rolePrefixes=@('tavern-keeper/','record-keeper/','artificer/','salvager/')
 }else{
  $fixture=(Resolve-Path -LiteralPath $BellwetherResidentFixture).Path
  $start.ArgumentList.Add('--measure-bellwether-doorway')
  $start.ArgumentList.Add('--chapter-resident-fixture')
  $rolePrefixes=@('')
  if(Test-Path -LiteralPath (Join-Path $fixture 'tavern-keeper') -PathType Container) {
   $rolePrefixes=@('tavern-keeper/','record-keeper/')
  }
 }
 $start.ArgumentList.Add($fixture)
 $fixtureHashes=@{}
 foreach($prefix in $rolePrefixes) {
  foreach($name in @('asset.manifest.json','resident.glb','base-color.windows.ktx2','normal.windows.ktx2','orm.windows.ktx2')) {
   $relative=$prefix+$name
   $path=Join-Path $fixture $relative
   if(!$residentRoundTrip -and $name.EndsWith('.ktx2') -and !(Test-Path -LiteralPath $path -PathType Leaf)){continue}
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
$tavernAssetHashes=$null
if($doorway) {
 foreach($source in @('src/gameplay/validation/BellwetherDoorwayProbe.h','src/gameplay/DevelopmentCheckpointSimulation.h',
  'src/gameplay/DevelopmentCheckpoints.h','src/gameplay/traversal/BellwetherTavernRoute.h',
  'src/gameplay/traversal/BellwetherEastRoute.h','src/vulkan/raytracing/BellwetherResidents.h',
  'src/gameplay/traversal/BellwetherTavernDoor.h')) {
  $sourceHashes[$source]=(Get-FileHash -LiteralPath (Join-Path $root $source) -Algorithm SHA256).Hash.ToLowerInvariant()
 }
 $tavernAssetHashes=@{}
 $tavernRoot=Join-Path $assetRoot 'models/world/runtime/bellwether-tavern-v1'
 $names=@('asset.manifest.json','bellwether-tavern-v1.glb','south-door.manifest.json','bellwether-south-door.glb')
 $drive=@('mill-bench-drive.manifest.json','mill-bench-drive.glb')
 if($drive | Where-Object {Test-Path -LiteralPath (Join-Path $tavernRoot $_)}) { $names+=$drive }
 $wheel=@('mill-waterwheel.manifest.json','mill-waterwheel.glb')
 if($wheel | Where-Object {Test-Path -LiteralPath (Join-Path $tavernRoot $_)}) { $names+=$wheel }
 foreach($name in $names) {
  $tavernAssetHashes[$name]=(Get-FileHash -LiteralPath (Join-Path $tavernRoot $name) -Algorithm SHA256).Hash.ToLowerInvariant()
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
$scope=if($residentRoundTrip){'Debug CPU; hidden Bellwether resident round trip; GPU and resource evidence only'}
elseif($doorway){'Debug CPU; hidden Bellwether doorway route with resident fixture; GPU and resource evidence only'}else{
 $Configuration+' CPU with Shipping High shaders; uncovered visible wall-clock scripted out/back and tomb backtrack after staged Keeper death; automatic static-detail lifetime; not owner/phone acceptance'
}
@{scope=$scope;
 executablePath=$start.FileName;buildProfile=$buildProfile;
 executableSha256=(Get-FileHash -LiteralPath $start.FileName -Algorithm SHA256).Hash.ToLowerInvariant();
 sources=$sourceHashes;residentFixtureHashes=$fixtureHashes;backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'};
 tavernAssetHashes=$tavernAssetHashes;
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
