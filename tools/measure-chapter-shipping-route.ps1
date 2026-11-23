param([Parameter(Mandatory=$true)][string]$Output,[switch]$Query,
 [ValidateSet('Debug','Release')][string]$Configuration='Release',[switch]$Foreground,[string]$Executable,
 [string]$BellwetherResidentFixture,[string]$BellwetherCampaignDirectory,
 [string]$BellwetherPackageReceipt,[switch]$BellwetherWest)
$ErrorActionPreference='Stop'
$fromPackage=-not [string]::IsNullOrWhiteSpace($BellwetherPackageReceipt)
$externalRoster=-not [string]::IsNullOrWhiteSpace($BellwetherCampaignDirectory)
$residentRoundTrip=$fromPackage -or $externalRoster
$hasDoorwayFixture=-not [string]::IsNullOrWhiteSpace($BellwetherResidentFixture)
if(($residentRoundTrip -and $hasDoorwayFixture) -or ($fromPackage -and ($externalRoster -or $Executable))) {
 throw 'Choose one Bellwether measurement source. A packaged run uses the executable and assets in its ZIP.'
}
if($BellwetherWest -and !$residentRoundTrip) { throw 'The west route requires the combined Bellwether roster.' }
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
$playtestPackage=$null
$packageRoot=$null
if($fromPackage) {
 $package=Get-Content -LiteralPath $BellwetherPackageReceipt -Raw | ConvertFrom-Json
 if($package.sourceCommit -cnotmatch '^[0-9a-f]{40}$' -or $package.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
    $package.executableSha256 -cnotmatch '^[0-9a-f]{64}$' -or $package.residentFiles -ne 25) {
  throw 'Expected the private Windows package receipt with all five residents.'
 }
 $archive=(Resolve-Path -LiteralPath $package.archive).Path
 if((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $package.sha256) {
  throw 'Private Windows ZIP differs from its package receipt.'
 }
 $packageRoot=$destination+'.extracted-playtest'
 if(Test-Path -LiteralPath $packageRoot){throw 'Choose a fresh extraction destination.'}
 [IO.Compression.ZipFile]::ExtractToDirectory($archive,$packageRoot)
 $Executable=Join-Path $packageRoot 'HordeLanternRT.exe'
 if((Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash -ne $package.executableSha256 -or
    !(Test-Path -LiteralPath (Join-Path $packageRoot 'Bellwether.playtest') -PathType Leaf) -or
    (Test-Path -LiteralPath (Join-Path $packageRoot 'HordeLanternRT.settings.ini'))) {
  throw 'The extracted private executable, marker or fresh settings do not match the package contract.'
 }
 $playtestPackage=@{sourceCommit=$package.sourceCommit;archiveSha256=$package.sha256;
  executableSha256=$package.executableSha256;voiceFiles=$package.voiceFiles}
}
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=if($Executable){(Resolve-Path -LiteralPath $Executable).Path}else{Join-Path $root ('build/chapter-shipping-measurement/'+$Configuration+'/HordeLanternRT.exe')}
$start.WorkingDirectory=if($fromPackage){$packageRoot}else{$root}
$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($argument in @('--development-rescue-journey','--capture-showcase',$destination,
 '--development-checkpoint')){$start.ArgumentList.Add($argument)}
$fixtureHashes=$null
if($doorway) {
 $start.ArgumentList.Add('bellwether-repeat-visit')
 $start.ArgumentList.Add('--capture-hidden')
 if($residentRoundTrip) {
  $start.ArgumentList.Add($(if($BellwetherWest){'--measure-bellwether-west-roundtrip'}else{'--measure-bellwether-resident-roundtrip'}))
  if($fromPackage) {
   $fixture=Join-Path $packageRoot 'assets/private_bellwether'
   $start.ArgumentList.Add('--measure-bellwether-from-package')
  }else{
   $fixture=(Resolve-Path -LiteralPath $BellwetherCampaignDirectory).Path
   $start.ArgumentList.Add('--bellwether-campaign-dir')
  }
  $rolePrefixes=@('tavern-keeper/','record-keeper/','artificer/','salvager/')
  if(Test-Path -LiteralPath (Join-Path $fixture 'relic-dealer') -PathType Container) {
   $rolePrefixes+='relic-dealer/'
  }elseif($BellwetherWest){
   throw 'The west route requires the prepared relic dealer.'
  }
 }else{
  $fixture=(Resolve-Path -LiteralPath $BellwetherResidentFixture).Path
  $start.ArgumentList.Add('--measure-bellwether-doorway')
  $start.ArgumentList.Add('--chapter-resident-fixture')
  $rolePrefixes=@('')
  if(Test-Path -LiteralPath (Join-Path $fixture 'tavern-keeper') -PathType Container) {
   $rolePrefixes=@('tavern-keeper/','record-keeper/')
  }
 }
 if(!$fromPackage){$start.ArgumentList.Add($fixture)}
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
 if($BellwetherWest){
  $names+=@('stock-heads.manifest.json','stock-heads.glb')
  foreach($source in @('src/gameplay/traversal/BellwetherWestRoute.h','src/gameplay/traversal/BellwetherStock.h',
   'assets/models/world/source/bellwether-stock-v1/geometry.generated.h')){
   $sourceHashes[$source]=(Get-FileHash -LiteralPath (Join-Path $root $source) -Algorithm SHA256).Hash.ToLowerInvariant()
  }
 }
 foreach($name in $names) {
  $tavernAssetHashes[$name]=(Get-FileHash -LiteralPath (Join-Path $tavernRoot $name) -Algorithm SHA256).Hash.ToLowerInvariant()
 }
}
$cachePath=Join-Path (Split-Path -Parent (Split-Path -Parent $start.FileName)) 'CMakeCache.txt'
$buildProfile=$null
if(Test-Path -LiteralPath $cachePath) {
 $policy=@{}
 foreach($line in [IO.File]::ReadLines($cachePath)) {
  if($line -match '^((?:CMAKE_(?:GENERATOR|BUILD_TYPE|CONFIGURATION_TYPES|HOME_DIRECTORY|CXX_COMPILER|CXX_FLAGS_DEBUG|CXX_FLAGS_RELEASE))|(?:HORDE_RT_(?:INSTRUMENTATION_OVERRIDE|DIELECTRIC_QUALITY_OVERRIDE|CHAPTER_MEASUREMENT|REGION_PAGE_PROTOTYPE_ROOT))):[^=]*=(.*)$') {
   $policy[$Matches[1]]=$Matches[2]
  }
 }
 $buildProfile=@{cachePath=$cachePath;cacheSha256=(Get-FileHash -LiteralPath $cachePath -Algorithm SHA256).Hash.ToLowerInvariant();configuration=$Configuration;policy=$policy}
}
$scope=if($fromPackage){'Extracted private Debug playtest; hidden Bellwether round trip at ordinary Windows client size'}
elseif($BellwetherWest){'Debug configuration; hidden Bellwether west round trip; native timing and resource evidence'}
elseif($residentRoundTrip){'Debug configuration; hidden Bellwether resident round trip; native timing and resource evidence'}
elseif($doorway){'Debug CPU; hidden Bellwether doorway route with resident fixture; GPU and resource evidence only'}else{
 $Configuration+' CPU with Shipping High shaders; uncovered visible wall-clock scripted out/back and tomb backtrack after staged Keeper death; automatic static-detail lifetime; not owner/phone acceptance'
}
@{scope=$scope;
 executablePath=$start.FileName;buildProfile=$buildProfile;
 executableSha256=(Get-FileHash -LiteralPath $start.FileName -Algorithm SHA256).Hash.ToLowerInvariant();
 sources=$sourceHashes;residentFixtureHashes=$fixtureHashes;backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'};
 tavernAssetHashes=$tavernAssetHashes;playtestPackage=$playtestPackage;
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
