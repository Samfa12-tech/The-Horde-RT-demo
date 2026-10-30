param([Parameter(Mandatory=$true)][string]$Output,[switch]$Query,[string]$Executable)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination=if([IO.Path]::IsPathRooted($Output)){$Output}else{Join-Path $root $Output}
if(Test-Path -LiteralPath $destination){throw 'Choose a fresh directory; previous evidence is preserved'}
[void][IO.Directory]::CreateDirectory($destination)
$executable=if($Executable){(Resolve-Path -LiteralPath $Executable).Path}else{
 Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'}
$sources=@{}
foreach($path in @('src/scene/TombWallRecess.h','src/scene/TombDressing.h',
 'src/scene/TombDressing.cpp','src/gameplay/DevelopmentCheckpoints.h',
 'src/vulkan/raytracing/PresentableTinyRtScene.cpp')){
 $sources[$path]=(Get-FileHash -LiteralPath (Join-Path $root $path)).Hash.ToLowerInvariant()
}
@{scope='Frozen Debug native niche regression frames; not Shipping performance or visual acceptance';
 executableSha256=(Get-FileHash -LiteralPath $executable).Hash.ToLowerInvariant();sources=$sources;
 backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'};deadlineSeconds=180
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destination 'candidate.json') -Encoding utf8
foreach($checkpoint in @('chapter-niche-rect','chapter-niche-arch','chapter-niche-east')){
 $capture=Join-Path $destination $checkpoint
 $start=[Diagnostics.ProcessStartInfo]::new()
 $start.FileName=$executable;$start.WorkingDirectory=$root;$start.UseShellExecute=$false
 $start.CreateNoWindow=$true;$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
 $start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 foreach($argument in @('--development-rescue-journey','--capture-showcase',$capture,
  '--development-checkpoint',$checkpoint)){$start.ArgumentList.Add($argument)}
 if($Query){$start.ArgumentList.Add('--require-rayquery-compute')}
 $process=[Diagnostics.Process]::Start($start)
 $out=$process.StandardOutput.ReadToEndAsync();$err=$process.StandardError.ReadToEndAsync()
 $timedOut=-not $process.WaitForExit(180000)
 if($timedOut){$process.Kill();$process.WaitForExit()}
 [IO.File]::WriteAllText($capture+'.stdout.log',$out.Result)
 [IO.File]::WriteAllText($capture+'.stderr.log',$err.Result)
 if($timedOut){throw "$checkpoint exceeded the unchanged 180-second capture deadline"}
 Write-Output ("$checkpoint capture exit: "+$process.ExitCode)
 if($process.ExitCode -ne 0){exit $process.ExitCode}
}
