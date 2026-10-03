$ErrorActionPreference = 'Stop'
$reportRoot = 'C:/Users/sam_s/.codex/worktrees/horde-mobile-lantern-profile/the Horde RT Demo'
$reportScratch = 'C:/Dev/tmp/horde-report-export-20261001'
$reportFile = "$reportScratch/windows-real-export.json"
if (Test-Path -LiteralPath $reportFile) { throw 'Exact smoke destination already exists; do not overwrite or rerun silently' }
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class ReportNativeSmoke {
  public delegate bool EnumCallback(IntPtr hwnd, IntPtr argument);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumCallback callback, IntPtr argument);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumCallback callback, IntPtr argument);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr window, StringBuilder name, int count);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr window, int id);
  [DllImport("user32.dll")] public static extern int GetDlgCtrlID(IntPtr window);
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
  public static IntPtr FindChildId(IntPtr parent, int id) {
    IntPtr found=GetDlgItem(parent,id);
    if(found!=IntPtr.Zero) return found;
    EnumChildWindows(parent,(w,a)=>{if(GetDlgCtrlID(w)==id){found=w;return false;}return true;},IntPtr.Zero);
    return found;
  }
  public static string[] Inventory(IntPtr parent) {
    var rows=new List<string>();
    EnumChildWindows(parent,(w,a)=>{rows.Add(GetDlgCtrlID(w)+":"+Class(w));return true;},IntPtr.Zero);
    return rows.ToArray(); // No directory filenames or owner window content.
  }
}
'@
function Wait-ReportWindow([uint32]$ReportProcessId, [string]$Class) {
    $reportDeadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        $reportWindow = [ReportNativeSmoke]::Find($ReportProcessId, $Class)
        if ($reportWindow -ne [IntPtr]::Zero) { return $reportWindow }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $reportDeadline)
    throw "Own process window absent: $Class"
}
function Send-ReportControl([IntPtr]$Window, [uint32]$Message, [int]$Value) {
    $reportResult = [UIntPtr]::Zero
    $reportSent = [ReportNativeSmoke]::Send($Window,$Message,[IntPtr]$Value,[IntPtr]::Zero,2,2000,[ref]$reportResult)
    if ($reportSent -eq [IntPtr]::Zero) { throw 'Own native control did not respond' }
    return $reportResult.ToUInt64()
}
function Set-ReportText([IntPtr]$Window, [string]$Text) {
    $reportResult = [UIntPtr]::Zero
    if ([ReportNativeSmoke]::SendText($Window,12,[IntPtr]::Zero,$Text,2,2000,[ref]$reportResult) -eq [IntPtr]::Zero) {
        throw 'Own edit control did not accept text'
    }
}
$reportProcess = $null
$reportForm = [IntPtr]::Zero
$reportMain = [IntPtr]::Zero
$reportPicker = [IntPtr]::Zero
try {
    $reportProcess = Start-Process -FilePath "$reportRoot/build/presets/windows-x64-debug/Debug/HordeLanternRT.exe" `
        -WorkingDirectory $reportRoot -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput "$reportScratch/windows-real-stdout.log" `
        -RedirectStandardError "$reportScratch/windows-real-stderr.log"
    $reportMain = Wait-ReportWindow $reportProcess.Id 'HordeRtDiagnosticWindowClass'
    # The initial menu remains paused. Wait for the owner thread, not another app.
    $reportReadyDeadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        $reportMenu = [ReportNativeSmoke]::GetDlgItem($reportMain,164)
        if ($reportMenu -ne [IntPtr]::Zero -and [ReportNativeSmoke]::IsWindowEnabled($reportMenu)) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $reportReadyDeadline)
    if ($reportMenu -eq [IntPtr]::Zero) { throw 'Report menu entry absent' }
    $null = [ReportNativeSmoke]::PostMessage($reportMain,273,[IntPtr]2026,[IntPtr]::Zero)
    $reportForm = Wait-ReportWindow $reportProcess.Id 'HordeLanternPlaytestReportDialog'
    if ((Send-ReportControl ([ReportNativeSmoke]::GetDlgItem($reportForm,107)) 240 0) -ne 0 -or
        (Send-ReportControl ([ReportNativeSmoke]::GetDlgItem($reportForm,108)) 240 0) -ne 0) { throw 'New form consent not off' }
    if (-not [ReportNativeSmoke]::IsWindowEnabled([ReportNativeSmoke]::GetDlgItem($reportForm,108))) {
        throw 'Live renderer context unavailable; do not guess it'
    }
    $null = Send-ReportControl ([ReportNativeSmoke]::GetDlgItem($reportForm,102)) 334 1
    $null = Send-ReportControl ([ReportNativeSmoke]::GetDlgItem($reportForm,104)) 334 2
    Set-ReportText ([ReportNativeSmoke]::GetDlgItem($reportForm,106)) "Windows local export smoke.`r`nParry, then open the report menu."
    $null = Send-ReportControl ([ReportNativeSmoke]::GetDlgItem($reportForm,107)) 241 1
    $null = Send-ReportControl ([ReportNativeSmoke]::GetDlgItem($reportForm,108)) 241 1
    $null = [ReportNativeSmoke]::PostMessage($reportForm,273,[IntPtr]111,[IntPtr]::Zero)
    $reportPicker = Wait-ReportWindow $reportProcess.Id '#32770'
    # A common-dialog HWND is observable before its filename controls exist.
    # Wait for that actual control instead of declaring initial construction done.
    $reportPickerDeadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        $reportFileEdit = [ReportNativeSmoke]::FindChildId($reportPicker,1152)
        if ($reportFileEdit -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $reportPickerDeadline)
    if ($reportFileEdit -eq [IntPtr]::Zero) {
        [ReportNativeSmoke]::Inventory($reportPicker)
        throw 'Filename edit not identified; no guessed destination action'
    }
    Set-ReportText $reportFileEdit $reportFile
    $null = [ReportNativeSmoke]::PostMessage($reportPicker,273,[IntPtr]1,[IntPtr]::Zero)
    $reportSaveDeadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        if ((Test-Path -LiteralPath $reportFile) -and (Get-Item -LiteralPath $reportFile).Length -gt 0) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $reportSaveDeadline)
    $reportBytes = [IO.File]::ReadAllBytes($reportFile)
    $reportUtf8 = [Text.UTF8Encoding]::new($false,$true)
    $reportJson = $reportUtf8.GetString($reportBytes) | ConvertFrom-Json
    if ($reportBytes.Length -gt 16384 -or $reportBytes[0] -ne 123 -or
        $reportJson.note -ne "Windows local export smoke.`r`nParry, then open the report menu." -or
        $reportJson.context.platform -ne 'Windows' -or $reportJson.context.backend -ne 'RayTracingPipeline' -or
        $reportJson.context.internalExtent[0] -le 0 -or $reportJson.context.internalExtent[1] -le 0) { throw 'Real saved payload contract failed' }
    $reportJson | ConvertTo-Json -Compress -Depth 6
    Get-FileHash -LiteralPath $reportFile | Select-Object Hash | ConvertTo-Json -Compress
    'PASS: own real Windows application entry, default consent, explicit context and native picker/file write'
} finally {
    if ($reportPicker -ne [IntPtr]::Zero) { $null = [ReportNativeSmoke]::PostMessage($reportPicker,273,[IntPtr]2,[IntPtr]::Zero) }
    if ($reportForm -ne [IntPtr]::Zero) { $null = [ReportNativeSmoke]::PostMessage($reportForm,16,[IntPtr]::Zero,[IntPtr]::Zero) }
    if ($reportMain -ne [IntPtr]::Zero) { $null = [ReportNativeSmoke]::PostMessage($reportMain,16,[IntPtr]::Zero,[IntPtr]::Zero) }
    if ($reportProcess -ne $null -and -not $reportProcess.WaitForExit(5000)) {
        # Only the exact process this smoke created, never owner applications.
        Stop-Process -Id $reportProcess.Id
        'Own smoke process required cleanup termination; not a clean exit pass'
    }
}
