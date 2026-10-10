$ErrorActionPreference = 'Stop'
$runner = Join-Path $PSScriptRoot '../tools/run-android-showcase-validation.ps1'
$tokens=$null; $errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($runner,[ref]$tokens,[ref]$errors)
if($errors.Count){throw ($errors | Out-String)}
foreach($name in @('Get-ExpectedShowcaseInstanceCapacity','New-ScopedLogcatArguments','Start-ScopedLogWindow','Get-ScopedLogcat',
    'Get-ShowcaseBackendIntentArguments','Get-ShowcaseInstallArguments','Assert-ShowcaseComputeUserZero','Assert-ShowcaseExecutionBackend',
    'Register-ShowcaseBackendEvidence','Get-ShowcaseState','Get-ShowcaseCapability','Start-AutomationSession',
    'Send-AutomationIntent','Invoke-HomeResumeLifecycleCheck','Wait-ForLogPattern',
    'Get-HomeResumePresentedGeneration','Assert-HomeResumeCompletedFrame','Invoke-BoundedHomeResumeRead','Wait-HomeResumeCompletedFrame')){
    $definition=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $name},$false))
    if($definition.Count -ne 1){throw "Expected one runner function: $name"}
    . ([scriptblock]::Create($definition[0].Extent.Text))
}
$script:witnessChecks=0
function Check([bool]$condition,[string]$message){if(-not $condition){throw $message};$script:witnessChecks++}
function Reject([scriptblock]$action){$failed=$false;try{& $action}catch{$failed=$true};Check $failed 'Invalid witness policy must fail closed.'}
$repository=Split-Path -Parent $PSScriptRoot
Check ((Get-ExpectedShowcaseInstanceCapacity $repository) -eq 28) 'Current physical TLAS capacity28 appends Keeper character27/metadata25 while preserving player torch22, Keeper torches23/24, scabbard25, and water-contact26.'
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('horde-witness-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $fixture 'src/vulkan/raytracing') -Force | Out-Null
try{
    $abiPath=Join-Path $fixture 'src/vulkan/raytracing/RtSceneAbi.def'
    '{"schema":1,"capacities":{"instanceMetadata":21,"tlasInstances":21}}' | Set-Content -LiteralPath $abiPath
    Check ((Get-ExpectedShowcaseInstanceCapacity $fixture) -eq 21) 'Equal metadata/TLAS fixtures retain their exact physical count.'
    '{"schema":1,"capacities":{"instanceMetadata":22,"tlasInstances":32}}' | Set-Content -LiteralPath $abiPath
    Check ((Get-ExpectedShowcaseInstanceCapacity $fixture) -eq 32) 'Aliased metadata22 derives actual physical32 rather than guessing metadata capacity.'
    foreach($invalid in @(
        '{"schema":1,"capacities":{"instanceMetadata":22}}',
        '{"schema":1,"capacities":{"instanceMetadata":0,"tlasInstances":24}}',
        '{"schema":1,"capacities":{"instanceMetadata":22,"tlasInstances":21}}',
        '{"schema":1,"capacities":{"instanceMetadata":22,"tlasInstances":257}}',
        '{"schema":1,"capacities":{"instanceMetadata":22,"tlasInstances":24.5}}',
        '{"schema":1,"capacities":{"instanceMetadata":22,"tlasInstances":"24"}}'
    )) {
        $invalid | Set-Content -LiteralPath $abiPath
        Reject {Get-ExpectedShowcaseInstanceCapacity $fixture}
    }
}finally{Remove-Item -LiteralPath $fixture -Recurse -Force}
$arguments=@(New-ScopedLogcatArguments '1791000000.123456789' '4242')
Check ($arguments -contains '--pid=4242' -and $arguments -contains '-T' -and
    $arguments -contains '1791000000.123456789' -and $arguments -contains 'crash' -and
    $arguments -notcontains '-c' -and $arguments -notcontains 'all') 'Only app-PID/time-bounded main/crash logs are requested; no global clear.'
Reject {New-ScopedLogcatArguments '1791000000' '4242'}
Reject {New-ScopedLogcatArguments '1791000000.123456789' '4242 5000'}

# Invoke only extracted functions with an injected local transport. The actual
# runner entrypoint, ADB, Gradle, installation and device are never called.
$packageName='synthetic.debug'
$observedLogProcessIds=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$runLogStart='';$operationLogStart='';$lastLogProcessId=''
$fixtureProcessId='4242';$fixtureTimestamp='1791000000.123456789'
$observedArguments=[Collections.Generic.List[string[]]]::new()
function Invoke-AdbText {
    param([string[]]$Arguments,[switch]$AllowFailure)
    if($Arguments[0] -ceq 'shell' -and $Arguments[1] -ceq 'date'){return $fixtureTimestamp}
    if($Arguments[0] -ceq 'shell' -and $Arguments[1] -ceq 'pidof'){return $fixtureProcessId}
    $observedArguments.Add($Arguments)
    return ($Arguments -join '|')
}
Start-ScopedLogWindow -NewRun
$first=Get-ScopedLogcat
Check ($first.Contains('--pid=4242') -and $first.Contains($fixtureTimestamp)) 'Startup evidence uses the actual observed app process.'
$fixtureTimestamp='1791000001.987654321';Start-ScopedLogWindow
$second=Get-ScopedLogcat
Check ($second.Contains($fixtureTimestamp) -and $runLogStart -ceq '1791000000.123456789') 'New intent excludes stale completions while retaining whole-run start.'
$fixtureProcessId='5000';$null=Get-ScopedLogcat
$full=Get-ScopedLogcat -WholeRun
Check ($full.Contains('--pid=4242') -and $full.Contains('--pid=5000') -and
    -not $full.Contains('1791000001.987654321')) 'Whole-run crash evidence retains each observed app PID and original time bound.'
$fixtureProcessId='5000 6000';Reject {Get-ScopedLogcat}
$fixtureProcessId='';$lastLogProcessId='';Check ((Get-ScopedLogcat) -ceq '') 'Startup without a PID never broadens to other apps.'
$source=Get-Content -LiteralPath $runner -Raw
Check ($source -notmatch 'Invoke-AdbText\s+@\("logcat",\s*"-c"\)' -and $source -notmatch '"shell",\s*"pm",\s*"clear"') 'Runner never clears device logs or app data.'
# Backend intent/evidence fixtures likewise use only extracted functions and an
# injected transport. No runner entrypoint, installation or device is executed.
Check (@(Get-ShowcaseBackendIntentArguments $false).Count -eq 0) 'Default Pipeline intent flags stay unchanged.'
$computeArguments=@(Get-ShowcaseBackendIntentArguments $true)
Check (($computeArguments -join '|') -ceq '--user|0|--ez|horde_require_rayquery_compute|true') 'Explicit Compute carries its Debug requirement and user scope.'
Check ((@(Get-ShowcaseInstallArguments $false 'synthetic.apk') -join '|') -ceq 'install|-r|-t|synthetic.apk') 'Default install behavior and preserved app data remain unchanged.'
Check ((@(Get-ShowcaseInstallArguments $true 'synthetic.apk') -join '|') -ceq 'install|--user|0|-r|-t|synthetic.apk') 'Compute install uses only verified user0 and the development APK.'
Assert-ShowcaseComputeUserZero $true '0'
Assert-ShowcaseComputeUserZero $false '10'
foreach($user in @('1','10','','Unknown command')) { Reject { Assert-ShowcaseComputeUserZero $true $user } }
$pipelineBundle=[pscustomobject]@{
    opaqueFast=[pscustomobject]@{key='diagnostic_mobile_opaque_fast';sha256=('a'*64)}
    genericDielectric=[pscustomobject]@{key='diagnostic_mobile_generic_dielectric';sha256=('b'*64)}
}
$computeBundle=[pscustomobject]@{
    opaqueFast=[pscustomobject]@{key='rayquery_compute_diagnostic_mobile_opaque_fast';sha256=('c'*64)}
    genericDielectric=[pscustomobject]@{key='rayquery_compute_diagnostic_mobile_generic_dielectric';sha256=('d'*64)}
}
function New-HomeCapability([long]$Epoch, [bool]$Completed) {
    $pipeline=[pscustomobject]@{executionBackend='RayQueryCompute';opaqueFast=$computeBundle.opaqueFast;genericDielectric=$computeBundle.genericDielectric}
    $frame=[pscustomobject]@{
        identity=[pscustomobject]@{sceneEpoch=$Epoch;measurementGeneration=6;recordAttemptSerial=1;recordSerial=1;submissionSerial=1;completionSerial=1;frameSlot=0;simulationTick=27}
        pipeline=$pipeline
        dispatch=[pscustomobject]@{sceneReady=$true;rtDispatchRecorded=$true;swapchainCopyRecorded=$true}
        presentation=[pscustomobject]@{presented=$true;outcome='presented';finalIdleCompletion=$false;lastSuccessfulPresentSubmissionSerial=1}
        stages=[pscustomobject]@{status='valid'}
        dielectric=[pscustomobject]@{available=$true;compiled=$true;status='valid';completedSubmissionSerial=1}
        gpu=[pscustomobject]@{status='valid';completedSubmissionSerial=1}
    }
    return [pscustomobject]@{rtMode='RayTracingPipeline';executionBackend='RayQueryCompute';rtScene=[pscustomobject]@{presented=$true}
        rtFrameEvidence=[pscustomobject]@{sceneEpoch=$Epoch;measurementGeneration=6;observerAvailable=$true;running=$true;presented=$true
            completedFrameStatus=$(if($Completed){'available'}else{'pending'});completedFrame=$(if($Completed){$frame}else{$null})}}
}
Assert-ShowcaseExecutionBackend 'RayTracingPipeline' 'RayTracingPipeline' $true $pipelineBundle
Assert-ShowcaseExecutionBackend 'RayQueryCompute' 'RayQueryCompute' $true $computeBundle
foreach($backend in @('RayTracingPipeline','RayQuery','Unsupported','Raster','')) {
    Reject { Assert-ShowcaseExecutionBackend 'RayQueryCompute' $backend $true $computeBundle }
}
Reject { Assert-ShowcaseExecutionBackend 'RayQueryCompute' 'RayQueryCompute' $false $computeBundle }
Reject { Assert-ShowcaseExecutionBackend 'RayQueryCompute' 'RayQueryCompute' 'true' $computeBundle }
Reject { Assert-ShowcaseExecutionBackend 'RayQueryCompute' 'RayQueryCompute' $true $pipelineBundle }
Reject { Assert-ShowcaseExecutionBackend 'RayTracingPipeline' 'RayTracingPipeline' $true $computeBundle }
$mixedBundle=[pscustomobject]@{opaqueFast=$computeBundle.opaqueFast;genericDielectric=$pipelineBundle.genericDielectric}
Reject { Assert-ShowcaseExecutionBackend 'RayQueryCompute' 'RayQueryCompute' $true $mixedBundle }

$backendFixture=Join-Path ([IO.Path]::GetTempPath()) ('horde-backend-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $backendFixture | Out-Null
try {
    $outputDirectory=$backendFixture; $activityName='synthetic.debug/synthetic.Activity'
    $gpuTimingArgument='true';$fixtureProcessId='4242';$RequireRayQueryCompute=$true
    $requestedExecutionBackend='RayQueryCompute'
    $backendSelectionEvidence=[ordered]@{requested=$requestedExecutionBackend;effective=$null;status='Pending';observations=[Collections.Generic.List[object]]::new()}
    $lifecycleEvidence=[ordered]@{homeResumePassed=$false;honestPresentationAfterResume=$false;log=$null}
    $fixtureState=[pscustomobject]@{executionBackend='RayQueryCompute';presented=$true;selectedRtPipelineBundle=$computeBundle}
    # Max supported rtMode deliberately differs: only actual executionBackend
    # and RT-produced presentation can certify the selected Compute route.
    $beforeHomeCapability=New-HomeCapability 10 $true
    $beforeHomeCapability.rtFrameEvidence.completedFrame.identity.submissionSerial=900
    $beforeHomeCapability.rtFrameEvidence.completedFrame.identity.completionSerial=900
    $fixtureCapability=New-HomeCapability 12 $false
    $freshHomeCapability=New-HomeCapability 12 $true
    $preHomeLog="10-04 12:00:00.100 4242 4243 I HordeRtProbeBridge: HORDE_SURFACE_PRESENTED generation=1`nRT frame reached Android swapchain presentation"
    $postHomeLog="10-04 12:00:01.100 4242 4243 I HordeRtProbeBridge: HORDE_SURFACE_REQUEST generation=3`n10-04 12:00:02.100 4242 4243 I HordeRtProbeBridge: HORDE_SURFACE_PRESENTED generation=3`nRT frame reached Android swapchain presentation"
    function Save-PrivateFile {
        param([string]$RemotePath,[string]$Destination)
        $value=if($RemotePath.EndsWith('vulkan_capability_report.json')){
            if($Destination.EndsWith('lifecycle-pre-home-capability.json')){$beforeHomeCapability}else{$fixtureCapability}
        }else{$fixtureState}
        $value | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $Destination
    }
    function Register-SelectedRtPipelineBundle {param($Bundle,[string]$Context); Check ($Bundle.opaqueFast.sha256 -ceq ('c'*64)) 'Native state continues selected-bundle registration.'}
    function Start-Sleep {param([int]$Milliseconds)} # Lifecycle waits are injected, too.
    $script:resumeReads=0
    function Get-ScopedLogcat {param([switch]$WholeRun);$script:resumeReads++;if($script:resumeReads -gt 2){return "$preHomeLog`n$postHomeLog"};return $preHomeLog}
    $script:owningCapabilityReads=0
    $script:owningReadTimeouts=[Collections.Generic.List[int]]::new()
    function Invoke-BoundedHomeResumeRead {
        param([string[]]$Arguments,[int]$TimeoutMilliseconds)
        $script:owningReadTimeouts.Add($TimeoutMilliseconds)
        if($Arguments[0] -ceq 'shell' -and $Arguments[1] -ceq 'pidof'){return '4242'}
        if($Arguments[0] -ceq 'logcat'){return $postHomeLog}
        $script:owningCapabilityReads++
        $value=if($script:owningCapabilityReads -eq 1){$fixtureCapability}else{$freshHomeCapability}
        return ($value | ConvertTo-Json -Depth 12)
    }
    $observedArguments.Clear()
    Start-AutomationSession -RequestedScale 75
    Send-AutomationIntent -Checkpoint 'opening' -RequestedScale 75
    Send-AutomationIntent -RequestedScale 75 -Replay
    Send-AutomationIntent -Checkpoint 'opening' -RequestedScale 75 -CaptureOnly
    Invoke-HomeResumeLifecycleCheck
    $starts=@($observedArguments | Where-Object { $_[0] -ceq 'shell' -and $_[1] -ceq 'am' -and $_[2] -ceq 'start' })
    Check ($starts.Count -eq 5) 'Startup, checkpoint, replay, capture and resume each dispatch one intent.'
    foreach($intent in $starts) {
        $joined=$intent -join '|'
        Check ($joined.Contains('--ez|horde_require_rayquery_compute|true') -and $joined.Contains('--user|0')) 'Every Compute launch retains exact backend requirement and user scope.'
    }
    Check ($lifecycleEvidence.homeResumePassed -and $lifecycleEvidence.honestPresentationAfterResume -and
        $lifecycleEvidence.effectiveExecutionBackend -ceq 'RayQueryCompute') 'Resume acceptance requires a fresh matching capability report.'
    Check ($lifecycleEvidence.completedOwningFrameAfterResume -and $lifecycleEvidence.completedOwningFrameStatus -ceq 'Verified' -and
        $lifecycleEvidence.completedOwningFrame.surfaceGeneration -eq 3 -and $script:owningCapabilityReads -eq 2) 'Real collector retains initial pending proof and waits for one fresh completed owning frame.'
    $initialSaved=Get-Content (Join-Path $outputDirectory 'lifecycle-home-resume-capability.json') -Raw | ConvertFrom-Json
    Check ($initialSaved.rtFrameEvidence.completedFrameStatus -ceq 'pending' -and
        (Test-Path (Join-Path $outputDirectory 'lifecycle-owning-attempt-001-capability.json')) -and
        (Test-Path (Join-Path $outputDirectory 'lifecycle-owning-attempt-002-capability.json'))) 'Initial post-Home pending packet and each subsequent attempt are preserved separately.'
    Check (($script:owningReadTimeouts | Where-Object {$_ -lt 1 -or $_ -gt 5000}).Count -eq 0) 'Every collector transport read has a bounded remaining deadline.'
    $proof=Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '4242' $postHomeLog 'RayQueryCompute'
    Check ($proof.identity.completionSerial -eq 1) 'Fresh-epoch completion1 is valid despite older-epoch completion900; no invented cross-epoch serial floor.'
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $fixtureCapability 1 '4242' '4242' $postHomeLog 'RayQueryCompute'}
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '5000' $postHomeLog 'RayQueryCompute'}
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 3 '4242' '4242' $postHomeLog 'RayQueryCompute'}
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '4242' ($postHomeLog.Replace('4242','5000')) 'RayQueryCompute'}
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '4242' ($postHomeLog.Replace('REQUEST generation=3','REQUEST generation=4')) 'RayQueryCompute'}
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '4242' ($postHomeLog + "`n10-04 12:00:03.100 4242 4243 I HordeRtProbeBridge: HORDE_SURFACE_REQUEST generation=5") 'RayQueryCompute'}
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '4242' ($postHomeLog + "`n10-04 12:00:03.100 4242 4243 I HordeRtProbeBridge: HORDE_SURFACE_CANCEL generation=3") 'RayQueryCompute'}
    $cancelBeforePresent=$postHomeLog.Replace('HORDE_SURFACE_PRESENTED generation=3',"HORDE_SURFACE_CANCEL generation=3`n10-04 12:00:02.900 4242 4243 I HordeRtProbeBridge: HORDE_SURFACE_PRESENTED generation=3")
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $freshHomeCapability 1 '4242' '4242' $cancelBeforePresent 'RayQueryCompute'}
    $olderInitial=$fixtureCapability | ConvertTo-Json -Depth 12 | ConvertFrom-Json
    $olderCompleted=$freshHomeCapability | ConvertTo-Json -Depth 12 | ConvertFrom-Json
    $olderInitial.rtFrameEvidence.sceneEpoch=9
    $olderCompleted.rtFrameEvidence.sceneEpoch=9
    $olderCompleted.rtFrameEvidence.completedFrame.identity.sceneEpoch=9
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $olderInitial $olderCompleted 1 '4242' '4242' $postHomeLog 'RayQueryCompute'}
    $olderScope=$freshHomeCapability | ConvertTo-Json -Depth 12 | ConvertFrom-Json
    $olderScope.rtFrameEvidence.measurementGeneration=5
    $olderScope.rtFrameEvidence.completedFrame.identity.measurementGeneration=5
    Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $olderScope 1 '4242' '4242' $postHomeLog 'RayQueryCompute'}
    foreach($mutation in @(
        {param($c) $c.rtFrameEvidence.sceneEpoch=10},
        {param($c) $c.rtFrameEvidence.completedFrame.identity.measurementGeneration=5},
        {param($c) $c.rtFrameEvidence.completedFrame.identity.completionSerial=0},
        {param($c) $c.rtFrameEvidence.completedFrame.presentation.outcome='presented-needs-recreate'},
        {param($c) $c.rtFrameEvidence.completedFrame.dispatch.rtDispatchRecorded=$false},
        {param($c) $c.rtFrameEvidence.completedFrame.dielectric.completedSubmissionSerial=2},
        {param($c) $c.rtFrameEvidence.completedFrame.gpu.completedSubmissionSerial=2},
        {param($c) $c.rtFrameEvidence.completedFrame.pipeline.opaqueFast.sha256=('e'*64)},
        {param($c) $c.rtFrameEvidence.completedFrame.pipeline.genericDielectric.key='rayquery_compute_diagnostic_high_generic_dielectric'},
        {param($c) $c.rtFrameEvidence.completedFrame.pipeline.executionBackend='RayTracingPipeline'})) {
        $changed=$freshHomeCapability | ConvertTo-Json -Depth 12 | ConvertFrom-Json
        & $mutation $changed
        Reject {Assert-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability $changed 1 '4242' '4242' $postHomeLog 'RayQueryCompute'}
    }
    $script:owningCapabilityReads=0
    function Invoke-BoundedHomeResumeRead {param([string[]]$Arguments,[int]$TimeoutMilliseconds);throw 'Injected read timeout; no device action.'}
    Reject {Wait-HomeResumeCompletedFrame $beforeHomeCapability $fixtureCapability 1 '4242' '1791000001.123456789' 'RayQueryCompute' $outputDirectory -BudgetMilliseconds 1}
    Check ($lifecycleEvidence.homeResumePassed -and $lifecycleEvidence.honestPresentationAfterResume) 'Owning-frame negative evidence never rewrites legacy first-present booleans.'
    $null=Get-ShowcaseState -Destination (Join-Path $backendFixture 'synthetic-state.json')
    Check ($backendSelectionEvidence.status -ceq 'Accepted' -and $backendSelectionEvidence.effective -ceq 'RayQueryCompute') 'Native state metadata separates requested and accepted effective backend.'
    $fixtureState.executionBackend='RayTracingPipeline'
    Reject { Get-ShowcaseState -Destination (Join-Path $backendFixture 'wrong-state.json') }
    Check ($backendSelectionEvidence.status -ceq 'Rejected' -and $backendSelectionEvidence.effective -ceq 'RayTracingPipeline') 'Rejected fallback metadata reports the actual wrong backend honestly.'
    $fixtureCapability.executionBackend='Unsupported';$fixtureCapability.rtScene.presented=$false
    Reject { Get-ShowcaseCapability -Destination (Join-Path $backendFixture 'unsupported-capability.json') }
    Check ($backendSelectionEvidence.status -ceq 'Rejected' -and $backendSelectionEvidence.effective -ceq 'Unsupported') 'Unsupported capability cannot manufacture success or a selected Compute backend.'
    function Get-ScopedLogcat {param([switch]$WholeRun);return 'Required hardware RayQuery compute backend is unavailable; no alternate backend will be selected.'}
    Reject { Wait-ForLogPattern -Pattern 'never' -Description 'synthetic unsupported' }
    $outerTry=@($ast.EndBlock.Statements | Where-Object { $_ -is [Management.Automation.Language.TryStatementAst] })
    Check ($outerTry.Count -eq 1 -and $outerTry[0].CatchClauses.Count -eq 1) 'One outer failure handler owns rejected-backend metadata.'
    $failureBody=$outerTry[0].CatchClauses[0].Body.Extent.Text
    $failureHandler=[scriptblock]::Create($failureBody.Substring(1,$failureBody.Length-2))
    Reject { try { Get-ShowcaseCapability -Destination (Join-Path $backendFixture 'rejected-capability.json') } catch { . $failureHandler } }
    $persisted=Get-Content -LiteralPath (Join-Path $backendFixture 'backend-selection.json') -Raw | ConvertFrom-Json
    Check ($persisted.requested -ceq 'RayQueryCompute' -and $persisted.effective -ceq 'Unsupported' -and
        $persisted.status -ceq 'Rejected' -and $persisted.failureReason.Contains('actually RT-presented')) 'Actual rejection handler persists requested/effective/status without a success claim.'
    $RequireRayQueryCompute=$false;$observedArguments.Clear()
    Start-AutomationSession -RequestedScale 75
    Check (-not (($observedArguments[0] -join '|').Contains('horde_require_rayquery_compute')) -and
        -not (($observedArguments[0] -join '|').Contains('--user'))) 'Default Pipeline startup gains no Compute/user flags.'
} finally {
    $resolvedFixture=[IO.Path]::GetFullPath($backendFixture)
    $temporaryPrefix=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    if(-not $resolvedFixture.StartsWith($temporaryPrefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Unsafe backend fixture cleanup target.'}
    Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
}
foreach($functionName in @('Start-AutomationSession','Send-AutomationIntent','Invoke-HomeResumeLifecycleCheck')) {
    $definition=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $functionName},$false))[0]
    $flagCommands=@($definition.FindAll({param($node) $node -is [Management.Automation.Language.CommandAst] -and $node.GetCommandName() -ceq 'Get-ShowcaseBackendIntentArguments'},$true))
    Check ($flagCommands.Count -eq 1) 'Every current intent builder uses the backend flag policy exactly once.'
}
Check ($source.Contains("'startup-capability.json'") -and $source.Contains("'lifecycle-home-resume-capability.json'") -and
    $source.Contains('$capability = Get-ShowcaseCapability -Destination $capabilityPath')) 'Startup, lifecycle and final capability evidence all validate actual backend.'
Check ($source.Contains("'backend-selection.json'") -and $source.Contains("status = 'Pending'") -and
    $source.Contains("'NotVerified'")) 'Failures retain honest requested/effective backend selection metadata.'
$stagedIf=@($ast.EndBlock.Statements | Where-Object {
    $_ -is [Management.Automation.Language.IfStatementAst] -and $_.Extent.Text.Contains('$RequireRayQueryCompute -and $StagedPrimaryInvestigation')
})
Check ($stagedIf.Count -eq 1) 'Compute/staged incompatibility rejects before the device entrypoint.'
$RequireRayQueryCompute=$true;$StagedPrimaryInvestigation=$true
Reject { . ([scriptblock]::Create($stagedIf[0].Extent.Text)) }
$RequireRayQueryCompute=$false
. ([scriptblock]::Create($stagedIf[0].Extent.Text))
Write-Output "PASS: Android witness ABI/log/backend contracts; $script:witnessChecks assertions, injected transport only; no ADB/build/device action."
