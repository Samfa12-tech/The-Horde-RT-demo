param([ValidateSet(640,1280)][int]$Width=640,[ValidateRange(4,240)][int]$Frames=24)
$ErrorActionPreference='Stop'
$labExe=Join-Path $PSScriptRoot 'horde_hybrid_lab.exe'
if (!(Test-Path -LiteralPath $labExe)) { throw 'Extract the complete Windows lab ZIP before running.' }
$labRun=Join-Path $PSScriptRoot ('results\run-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
& $labExe $labRun $Width ([int]($Width*9/16)) $Frames
if ($LASTEXITCODE -ne 0) { throw 'The Vulkan comparison failed. Read the diagnostic above.' }
Start-Process (Join-Path $labRun 'index.html') -WindowStyle Hidden
