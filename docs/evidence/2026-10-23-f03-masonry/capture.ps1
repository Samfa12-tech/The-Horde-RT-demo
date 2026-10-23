param([Parameter(Mandatory=$true)][ValidateSet('forward','reverse','lookout')][string]$View)
$ErrorActionPreference='Stop'
$root=(Get-Location).Path
$taskDir=Join-Path $root 'reports/f03-masonry-20261023'
$dest=Join-Path $taskDir $View
if(Test-Path -LiteralPath $dest){throw 'Refusing to overwrite an existing capture'}
if(@(Get-Process -Name HordeLanternRT -ErrorAction SilentlyContinue).Count){throw 'Existing Horde owner could conflict; no launch'}
$exe=Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
$settings=Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.settings.ini'
$before=(Get-FileHash -LiteralPath $settings -Algorithm SHA256).Hash
if($before -ne 'ed609b868fd2951698c46bb17b98ffbad647bf30ca7cfacfa5d64f5342fd57c3'){throw 'Settings differ before capture'}
if(Test-Path -LiteralPath 'C:/Users/sam_s/AppData/Local/HordeLanternRT/campaign-v1'){throw 'Unexpected owner campaign directory; inspect custody first'}
$checkpoint=if($View -eq 'lookout'){'chapter-forest-lookout'}else{'chapter-forest-clue-'+$View}
$arguments=@('--development-rescue-journey','--capture-showcase',('"'+$dest+'"'),'--capture-hidden','--development-checkpoint',$checkpoint)
$started=[DateTime]::UtcNow
$proc=Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskDir ($View+'.stdout.log')) -RedirectStandardError (Join-Path $taskDir ($View+'.stderr.log'))
$completed=$proc.WaitForExit(50000)
$record=@{view=$View;pid=$proc.Id;completed=$completed;baseCommit=(git rev-parse HEAD);dirtyFiles=@(git diff --name-only);exeSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;settingsBefore=$before;settingsAfter=(Get-FileHash -LiteralPath $settings -Algorithm SHA256).Hash;startedUtc=$started.ToString('o');endedUtc=[DateTime]::UtcNow.ToString('o');arguments=$arguments;scope='Diagnostic seeded accepted-death ascent/walk then frozen RT view; no campaign PlaySession or audio playback; not natural victory, listening, performance or phone acceptance'}
if($completed){$record.exitCode=$proc.ExitCode}
$record.ownerCampaignRemainsAbsent= -not (Test-Path -LiteralPath 'C:/Users/sam_s/AppData/Local/HordeLanternRT/campaign-v1')
$record|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $taskDir ($View+'.process.json')) -Encoding utf8
$record|ConvertTo-Json -Depth 5
if(-not $completed){throw 'Capture remains running; inspect recorded PID before another launch'}
if($proc.ExitCode -ne 0 -or $record.settingsAfter -ne $before -or -not $record.ownerCampaignRemainsAbsent){throw 'Capture or custody failed'}


