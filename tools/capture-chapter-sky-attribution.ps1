param(
 [Parameter(Mandatory=$true)][string]$Output,
 [ValidateSet('chapter-landing-north','chapter-landing-west','chapter-forest-second-bend','chapter-kit-reunion')]
 [string]$Checkpoint='chapter-landing-north',
 [ValidateSet('Production','Directional','Off')][string]$Mode='Production',
 [switch]$Query
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination=if([IO.Path]::IsPathRooted($Output)){$Output}else{Join-Path $root $Output}
foreach($path in @($destination,($destination+'.stdout.log'),($destination+'.stderr.log'),($destination+'.candidate.json'))){
 if(Test-Path -LiteralPath $path){throw 'Choose a fresh capture directory and receipt prefix'}
}
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
$start.WorkingDirectory=$root;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($argument in @('--development-rescue-journey','--capture-showcase',$destination,
 '--development-checkpoint',$Checkpoint)){$start.ArgumentList.Add($argument)}
if($Mode -ne 'Production') {
 $start.ArgumentList.Add('--capture-sky-attribution');$start.ArgumentList.Add($Mode.ToLowerInvariant())
}
if($Query){$start.ArgumentList.Add('--require-rayquery-compute')}
$sourceHashes=@{}
foreach($source in @('src/platform/windows/DiagnosticWindow.cpp','src/platform/windows/WindowsSkyAttributionLaunch.h',
 'src/vulkan/raytracing/PresentableTinyRtScene.h','src/vulkan/raytracing/PresentableTinyRtScene.cpp',
 'shaders/raytracing/include/rt_lighting.glsl')){
 $sourceHashes[$source]=(Get-FileHash -LiteralPath (Join-Path $root $source) -Algorithm SHA256).Hash.ToLowerInvariant()
}
@{scope='Frozen Debug light attribution; no geometry, material, quality or settings mutation';
 executableSha256=(Get-FileHash -LiteralPath $start.FileName -Algorithm SHA256).Hash.ToLowerInvariant();
 sources=$sourceHashes;backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'};
 checkpoint=$Checkpoint;mode=$Mode;deadlineSeconds=180
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath ($destination+'.candidate.json') -Encoding utf8
$process=[Diagnostics.Process]::Start($start)
$out=$process.StandardOutput.ReadToEndAsync();$err=$process.StandardError.ReadToEndAsync()
$timedOut=-not $process.WaitForExit(180000)
if($timedOut){$process.Kill();$process.WaitForExit()}
[IO.File]::WriteAllText($destination+'.stdout.log',$out.Result)
[IO.File]::WriteAllText($destination+'.stderr.log',$err.Result)
if($timedOut){throw 'Scoped capture exceeded its unchanged 180-second deadline'}
Write-Output ("$Output capture exit: "+$process.ExitCode)
if($process.ExitCode -ne 0){exit $process.ExitCode}
