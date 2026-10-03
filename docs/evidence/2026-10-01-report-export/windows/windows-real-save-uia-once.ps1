$ErrorActionPreference = 'Stop'
$reportRoot = 'C:/Users/sam_s/.codex/worktrees/horde-mobile-lantern-profile/the Horde RT Demo'
$reportScratch = 'C:/Dev/tmp/horde-report-export-20261001'
$reportFile = "$reportScratch/windows-real-export-uia-once.json"
if (Test-Path -LiteralPath $reportFile) { throw 'Exact one-shot destination already exists; refusing overwrite' }
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class ReportNativeUiaSmoke {
  public delegate bool EnumCallback(IntPtr hwnd, IntPtr argument);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumCallback callback, IntPtr argument);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr window, StringBuilder name, int count);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr window, int id);
  [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr window);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr value, IntPtr argument);
  [DllImport("user32.dll", EntryPoint="SendMessageTimeoutW")] public static extern IntPtr Send(IntPtr window, uint message, IntPtr value, IntPtr argument, uint flags, uint timeout, out UIntPtr result);
  [DllImport("user32.dll", EntryPoint="SendMessageTimeoutW", CharSet=CharSet.Unicode)] public static extern IntPtr SendText(IntPtr window, uint message, IntPtr value, string argument, uint flags, uint timeout, out UIntPtr result);
  public static string Class(IntPtr window) { var b=new StringBuilder(100); GetClassName(window,b,100); return b.ToString(); }
  public static IntPtr Find(uint process, string cls) {
    IntPtr found=IntPtr.Zero;
    EnumWindows((w,a)=>{uint p; GetWindowThreadProcessId(w,out p); if(p==process && Class(w)==cls){found=w;return false;} return true;},IntPtr.Zero);
    return found;
  }
}
'@
function Wait-ReportWindow([uint32]$ReportProcessId, [string]$Class) {
    $deadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        $window = [ReportNativeUiaSmoke]::Find($ReportProcessId, $Class)
        if ($window -ne [IntPtr]::Zero) { return $window }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Own process window absent: $Class"
}
function Send-ReportControl([IntPtr]$Window, [uint32]$Message, [int]$Value) {
    $result = [UIntPtr]::Zero
    if ([ReportNativeUiaSmoke]::Send($Window,$Message,[IntPtr]$Value,[IntPtr]::Zero,2,2000,[ref]$result) -eq [IntPtr]::Zero) {
        throw 'Own native control did not respond'
    }
    return $result.ToUInt64()
}
function Set-ReportText([IntPtr]$Window, [string]$Text) {
    $result = [UIntPtr]::Zero
    if ([ReportNativeUiaSmoke]::SendText($Window,12,[IntPtr]::Zero,$Text,2,2000,[ref]$result) -eq [IntPtr]::Zero) {
        throw 'Own edit control did not accept text'
    }
}
function Find-UniqueFilenameEdit([IntPtr]$Picker, [uint32]$OwnerPid) {
    $root = [System.Windows.Automation.AutomationElement]::FromHandle($Picker)
    if ($root.Current.ProcessId -ne $OwnerPid) { throw 'UIA picker root is not the owned process' }
    $all = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition)
    $matches = [System.Collections.Generic.List[object]]::new()
    $inventory = [System.Collections.Generic.List[string]]::new()
    for ($i = 0; $i -lt $all.Count; $i++) {
        $element = $all.Item($i)
        $current = $element.Current
        if ($current.ProcessId -ne $OwnerPid) { continue }
        if ($current.ControlType -ne [System.Windows.Automation.ControlType]::Edit) { continue }
        $label = $current.Name
        $normalized = $label.Trim().TrimEnd(':').Trim()
        $isFilenameLabel = $normalized -match '^(?i:file name|filename)$'
        $inventory.Add("Edit AutomationId=$($current.AutomationId); Class=$($current.ClassName); Focusable=$($current.IsKeyboardFocusable); Label=" +
            $(if ($isFilenameLabel) { $normalized } else { '<redacted>' }))
        if ($isFilenameLabel -and $current.IsKeyboardFocusable -and -not $current.IsOffscreen) {
            $matches.Add($element)
        }
    }
    $inventory | ForEach-Object { $_ }
    if ($matches.Count -ne 1) { throw "Expected one accessible filename edit, found $($matches.Count); no save attempted" }
    return $matches[0]
}
$exe = "$reportRoot/build/presets/windows-x64-debug/Debug/HordeLanternRT.exe"
Get-FileHash -LiteralPath $exe | Select-Object Path,Hash | ConvertTo-Json -Compress
$process = $null
$form = [IntPtr]::Zero
$main = [IntPtr]::Zero
$picker = [IntPtr]::Zero
try {
    $process = Start-Process -FilePath $exe -WorkingDirectory $reportRoot -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput "$reportScratch/windows-real-uia-once.stdout.log" `
        -RedirectStandardError "$reportScratch/windows-real-uia-once.stderr.log"
    $main = Wait-ReportWindow $process.Id 'HordeRtDiagnosticWindowClass'
    $deadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        $menu = [ReportNativeUiaSmoke]::GetDlgItem($main,164)
        if ($menu -ne [IntPtr]::Zero -and [ReportNativeUiaSmoke]::IsWindowEnabled($menu)) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($menu -eq [IntPtr]::Zero) { throw 'Report menu entry absent' }
    $null = [ReportNativeUiaSmoke]::PostMessage($main,273,[IntPtr]2026,[IntPtr]::Zero)
    $form = Wait-ReportWindow $process.Id 'HordeLanternPlaytestReportDialog'
    if ((Send-ReportControl ([ReportNativeUiaSmoke]::GetDlgItem($form,107)) 240 0) -ne 0 -or
        (Send-ReportControl ([ReportNativeUiaSmoke]::GetDlgItem($form,108)) 240 0) -ne 0) { throw 'New form consent not off' }
    if (-not [ReportNativeUiaSmoke]::IsWindowEnabled([ReportNativeUiaSmoke]::GetDlgItem($form,108))) {
        throw 'Live context unavailable; note-only is available but this context-on smoke must not guess fields'
    }
    $null = Send-ReportControl ([ReportNativeUiaSmoke]::GetDlgItem($form,102)) 334 1
    $null = Send-ReportControl ([ReportNativeUiaSmoke]::GetDlgItem($form,104)) 334 2
    Set-ReportText ([ReportNativeUiaSmoke]::GetDlgItem($form,106)) "Windows local export smoke.`r`nParry, then open the report menu."
    $null = Send-ReportControl ([ReportNativeUiaSmoke]::GetDlgItem($form,107)) 241 1
    $null = Send-ReportControl ([ReportNativeUiaSmoke]::GetDlgItem($form,108)) 241 1
    $null = [ReportNativeUiaSmoke]::PostMessage($form,273,[IntPtr]111,[IntPtr]::Zero)
    $picker = Wait-ReportWindow $process.Id '#32770'
    $filenameEdit = Find-UniqueFilenameEdit $picker $process.Id
    $pattern = $filenameEdit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
    $pattern.SetValue($reportFile)
    $written = $filenameEdit.GetCurrentPropertyValue([System.Windows.Automation.ValuePattern]::ValueProperty)
    if ($written -ne $reportFile) { throw 'Filename value did not match the exact fresh target; no save attempted' }
    $null = [ReportNativeUiaSmoke]::PostMessage($picker,273,[IntPtr]1,[IntPtr]::Zero)
    $saveDeadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        if ((Test-Path -LiteralPath $reportFile) -and (Get-Item -LiteralPath $reportFile).Length -gt 0) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $saveDeadline)
    if (-not (Test-Path -LiteralPath $reportFile)) { throw 'Native save did not create the exact requested target' }
    $bytes = [IO.File]::ReadAllBytes($reportFile)
    $utf8 = [Text.UTF8Encoding]::new($false,$true)
    $jsonText = $utf8.GetString($bytes)
    $json = $jsonText | ConvertFrom-Json
    if ($bytes.Length -gt 16384 -or $bytes.Length -lt 2 -or
        $bytes[0] -ne 123 -or $json.schemaVersion -ne 1 -or
        $json.note -ne "Windows local export smoke.`r`nParry, then open the report menu." -or
        $json.context.platform -ne 'Windows' -or $json.context.version -ne '1.6.1' -or
        $json.context.build -ne '1.6.1' -or $json.context.rtPresented -ne $false) { throw 'Real Windows saved payload contract failed' }
    $hash = (Get-FileHash -LiteralPath $reportFile -Algorithm SHA256).Hash
    [PSCustomObject]@{ path=$reportFile; bytes=$bytes.Length; reportId=$json.reportId; hash=$hash; backend=$json.context.backend; quality=$json.context.quality; renderScale=$json.context.renderScale; extent=$json.context.internalExtent; rtPresented=$json.context.rtPresented } | ConvertTo-Json -Compress -Depth 5
    'PASS: owned Windows app, consent defaults off, explicit context consent, accessible native filename control and one local UTF-8 JSON save'
} finally {
    if ($picker -ne [IntPtr]::Zero) { $null = [ReportNativeUiaSmoke]::PostMessage($picker,273,[IntPtr]2,[IntPtr]::Zero) }
    if ($form -ne [IntPtr]::Zero) { $null = [ReportNativeUiaSmoke]::PostMessage($form,16,[IntPtr]::Zero,[IntPtr]::Zero) }
    if ($main -ne [IntPtr]::Zero) { $null = [ReportNativeUiaSmoke]::PostMessage($main,16,[IntPtr]::Zero,[IntPtr]::Zero) }
    if ($process -ne $null -and -not $process.WaitForExit(5000)) {
        Stop-Process -Id $process.Id
        'Own smoke process required cleanup termination; not a clean exit pass'
    } elseif ($process -ne $null) {
        "Own smoke process exited cleanly: $($process.ExitCode)"
    }
}
