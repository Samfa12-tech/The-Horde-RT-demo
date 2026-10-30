$Version='native-motion'
$ErrorActionPreference='Stop'
$root=$PWD.Path
$output=Join-Path $root "reports/backward-first-person-20261023/$Version"
if(Test-Path -LiteralPath $output){throw 'Never overwrite a capture directory.'}
$exe=Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=$exe;$start.WorkingDirectory=$root;$start.UseShellExecute=$false
$start.CreateNoWindow=$true;$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true

foreach($arg in @('--development-rescue-journey','--capture-showcase',$output,'--development-checkpoint','chapter-stone-portal-front','--capture-hidden')){$start.ArgumentList.Add($arg)}
$start.Environment['HORDE_LOCAL_BACKWARD_FIRST_PERSON']='1'
$hashes=@{}
foreach($path in @('src/gameplay/animation/PlayerAnimationState.h','src/gameplay/animation/PlayerAnimationState.cpp','src/gameplay/simulation/GameSimulation.h','src/gameplay/simulation/GameSimulation.cpp','src/vulkan/raytracing/PlayerRenderSlot.cpp','src/platform/windows/DiagnosticWindow.cpp','reports/backward-first-person-20261023/native-probe.inl')){$hashes[$path]=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $path)).Hash}
$receipt=@{version=$Version;startedUtc=[DateTime]::UtcNow.ToString('o');executableSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash;sources=$hashes;arguments=@($start.ArgumentList);scope='fresh production configuration at portal, fixed motor inputs rendered each tick; isolated dynamic first-person, no physical controls, natural route, pacing or phone acceptance'}
$p=[Diagnostics.Process]::Start($start);$stdout=$p.StandardOutput.ReadToEndAsync();$stderr=$p.StandardError.ReadToEndAsync();$timer=[Diagnostics.Stopwatch]::StartNew();$timedOut=$false
while(-not $p.WaitForExit(1000)){if($timer.Elapsed.TotalSeconds -gt 180){$timedOut=$true;$p.Kill();$p.WaitForExit();break}}
$receipt.pid=$p.Id;$receipt.exitCode=$p.ExitCode;$receipt.timedOut=$timedOut;$receipt.elapsedSeconds=$timer.Elapsed.TotalSeconds;$receipt.finishedUtc=[DateTime]::UtcNow.ToString('o')
[IO.File]::WriteAllText($output+'.stdout.txt',$stdout.Result);[IO.File]::WriteAllText($output+'.stderr.txt',$stderr.Result)
$receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($output+'.process.json') -Encoding utf8
Write-Output "$Version exit=$($p.ExitCode) timeout=$timedOut seconds=$($timer.Elapsed.TotalSeconds)"
$code=$p.ExitCode;$p.Dispose();if($timedOut){throw 'Local diagnostic exceeded unchanged180s ceiling.'};exit $code
