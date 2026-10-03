$ErrorActionPreference = 'Stop'
$taskRepo = 'C:/Users/sam_s/.codex/worktrees/horde-whistle-listening/the Horde RT Demo'
$taskBaseline = 'C:/Dev/tmp/horde-fast-resize-20261001/HordeLanternRT-fast-resize-final-debug-arm64.apk'
$taskReceipt = 'C:/Dev/tmp/horde-music-instrumentation-20261001/whistle-bank/listening-manifest-checkout.json'
if (Test-Path -LiteralPath $taskReceipt) { throw 'Receipt exists; do not repeat completed processing.' }
if ((git -C $taskRepo rev-parse HEAD).Trim() -cne '97f07e2a5791346f2b4563221909a87ab5948086') { throw 'Unexpected candidate HEAD.' }
if ((Get-FileHash -LiteralPath $taskBaseline).Hash.ToLowerInvariant() -cne 'c11ff703794d74dbfb75dc7a0186af0f6c5c416e8383755a5cd8f84310daf0fe') { throw 'Unexpected baseline APK.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$taskZip = [IO.Compression.ZipFile]::OpenRead($taskBaseline)
$taskReports = @()
try {
    foreach ($taskPath in @('assets/models/player/runtime/clip-manifest.json', 'assets/models/player/viewmodel/runtime/asset.manifest.json')) {
        $taskEntry = $taskZip.GetEntry($taskPath)
        $taskInput = $taskEntry.Open()
        $taskMemory = [IO.MemoryStream]::new()
        try { $taskInput.CopyTo($taskMemory); $taskBytes = $taskMemory.ToArray() }
        finally { $taskInput.Dispose(); $taskMemory.Dispose() }
        $taskProcess = [Diagnostics.Process]::new()
        $taskProcess.StartInfo.FileName = 'git'
        $taskProcess.StartInfo.Arguments = 'show HEAD:' + $taskPath
        $taskProcess.StartInfo.WorkingDirectory = $taskRepo
        $taskProcess.StartInfo.UseShellExecute = $false
        $taskProcess.StartInfo.CreateNoWindow = $true
        $taskProcess.StartInfo.RedirectStandardOutput = $true
        [void]$taskProcess.Start()
        $taskGitText = $taskProcess.StandardOutput.ReadToEnd()
        $taskProcess.WaitForExit()
        if ($taskProcess.ExitCode -ne 0) { throw 'Git blob read failed.' }
        $taskProcess.Dispose()
        $taskOldText = [Text.Encoding]::UTF8.GetString($taskBytes)
        $taskFile = Join-Path $taskRepo $taskPath
        $taskCheckoutText = [IO.File]::ReadAllText($taskFile)
        # Require exact canonical source equality, not a weaker parsed-JSON test.
        if ($taskOldText.Replace("`r`n", "`n") -cne $taskGitText -or
            $taskCheckoutText.Replace("`r`n", "`n") -cne $taskGitText) {
            throw "Non-newline difference at $taskPath; no output written."
        }
        $taskBeforeHash = (Get-FileHash -LiteralPath $taskFile).Hash.ToLowerInvariant()
        # Restore accepted artifact bytes only after proving unchanged source.
        [IO.File]::WriteAllBytes($taskFile, $taskBytes)
        $taskReports += [pscustomobject]@{
            path = $taskPath; beforeSha256 = $taskBeforeHash
            acceptedBytes = $taskBytes.Length; restoredSha256 = (Get-FileHash -LiteralPath $taskFile).Hash.ToLowerInvariant()
            canonicalGitSourceUnchanged = $true; change = 'Checkout-only line-ending restoration from accepted C11 APK'
        }
    }
} finally { $taskZip.Dispose() }
$taskStatus = @(git -C $taskRepo status --porcelain)
$taskAllowed = @(' M assets/models/player/runtime/clip-manifest.json', ' M assets/models/player/viewmodel/runtime/asset.manifest.json')
if (@($taskStatus | Where-Object { $_ -cnotin $taskAllowed }).Count -ne 0) { throw 'Unrelated checkout change.' }
git -C $taskRepo diff --exit-code -- assets/models/player/runtime/clip-manifest.json assets/models/player/viewmodel/runtime/asset.manifest.json
if ($LASTEXITCODE -ne 0) { throw 'Canonical Git diff differs despite normalized equality.' }
[pscustomobject]@{ sourceHead = '97f07e2a5791346f2b4563221909a87ab5948086'; sourceWorkingTreeDirty = ($taskStatus.Count -gt 0); canonicalSourceDiffEmpty = $true;
    retryReason = 'First restoration wrote validated bytes, then rejected Git status reporting checkout-only newline changes. Normalized exact blob comparison and git diff both pass.'; manifests = $taskReports } |
    ConvertTo-Json -Depth 5 | Out-File -LiteralPath $taskReceipt -Encoding utf8
Write-Output "Accepted manifest checkout bytes restored; canonical Git source unchanged. Receipt: $taskReceipt"
