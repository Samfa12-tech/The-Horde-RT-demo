param([Parameter(Mandatory=$true)][string]$Output,
      [Parameter(Mandatory=$true)][uint32[]]$Triangles,
      [string]$Checkpoint='chapter-lantern-side-wall-high', [switch]$Query)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination=if([IO.Path]::IsPathRooted($Output)){$Output}else{Join-Path $root $Output}
if(Test-Path -LiteralPath $destination){throw 'Choose a fresh directory; previous evidence is preserved'}
if($Triangles.Count -eq 0){throw 'An explicit nonempty triangle list is required'}
[void][IO.Directory]::CreateDirectory($destination)
$executable=Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
$sources=@{}
foreach($path in @('src/vulkan/raytracing/RtStaticMeshSlot.h','src/vulkan/raytracing/RtStaticMeshSlot.cpp',
 'src/vulkan/raytracing/PresentableTinyRtScene.h','src/vulkan/raytracing/PresentableTinyRtScene.cpp',
 'src/vulkan/raytracing/PresentableTinyRtSceneStaticGeometry.cpp','src/platform/windows/DiagnosticWindow.cpp')){
 $sources[$path]=(Get-FileHash -LiteralPath (Join-Path $root $path)).Hash.ToLowerInvariant()
}
@{scope='Frozen Debug sparse GPU-owner transaction; not contributor eligibility or playable FPS';
 executableSha256=(Get-FileHash -LiteralPath $executable).Hash.ToLowerInvariant();sources=$sources;
 triangles=$Triangles;checkpoint=$Checkpoint;deadlineSeconds=180;
 backend=if($Query){'RayQueryCompute'}else{'RayTracingPipeline'}
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destination 'candidate.json') -Encoding utf8
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=$executable;$start.WorkingDirectory=$root;$start.UseShellExecute=$false
$start.CreateNoWindow=$true;$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($argument in @('--development-rescue-journey','--capture-showcase',$destination,
 '--development-checkpoint',$Checkpoint,'--validate-static-sparse-cutover',($Triangles -join ' '))){
 $start.ArgumentList.Add($argument)
}
if($Query){$start.ArgumentList.Add('--require-rayquery-compute')}
$process=[Diagnostics.Process]::Start($start)
$out=$process.StandardOutput.ReadToEndAsync();$err=$process.StandardError.ReadToEndAsync()
$timedOut=-not $process.WaitForExit(180000)
if($timedOut){$process.Kill();$process.WaitForExit()}
[IO.File]::WriteAllText($destination+'.stdout.log',$out.Result)
[IO.File]::WriteAllText($destination+'.stderr.log',$err.Result)
if($timedOut){throw 'Scoped capture exceeded its unchanged 180-second deadline'}
Write-Output ("$Output capture exit: "+$process.ExitCode)
if($process.ExitCode -ne 0){exit $process.ExitCode}
