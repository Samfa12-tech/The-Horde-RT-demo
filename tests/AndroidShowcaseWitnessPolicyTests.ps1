$ErrorActionPreference = 'Stop'
$runner = Join-Path $PSScriptRoot '../tools/run-android-showcase-validation.ps1'
$tokens=$null; $errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($runner,[ref]$tokens,[ref]$errors)
if($errors.Count){throw ($errors | Out-String)}
foreach($name in @('Get-ExpectedShowcaseInstanceCapacity','New-ScopedLogcatArguments','Start-ScopedLogWindow','Get-ScopedLogcat')){
    $definition=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $name},$false))
    if($definition.Count -ne 1){throw "Expected one runner function: $name"}
    . ([scriptblock]::Create($definition[0].Extent.Text))
}
function Check([bool]$condition,[string]$message){if(-not $condition){throw $message}}
function Reject([scriptblock]$action){$failed=$false;try{& $action}catch{$failed=$true};Check $failed 'Invalid witness policy must fail closed.'}
$repository=Split-Path -Parent $PSScriptRoot
Check ((Get-ExpectedShowcaseInstanceCapacity $repository) -eq 21) 'Current actual generated instance capacity remains 21; material capacity32 is separate.'
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('horde-witness-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $fixture 'src/vulkan/raytracing') -Force | Out-Null
try{
    $abiPath=Join-Path $fixture 'src/vulkan/raytracing/RtSceneAbi.def'
    '{"schema":1,"capacities":{"instanceMetadata":21}}' | Set-Content -LiteralPath $abiPath
    Check ((Get-ExpectedShowcaseInstanceCapacity $fixture) -eq 21) 'Released baseline checkout keeps its21-instance assertion.'
    '{"schema":1,"capacities":{"instanceMetadata":32}}' | Set-Content -LiteralPath $abiPath
    Check ((Get-ExpectedShowcaseInstanceCapacity $fixture) -eq 32) 'Changed ABI fixture derives32 without weakening exact expected count.'
    '{"schema":1,"capacities":{"instanceMetadata":0}}' | Set-Content -LiteralPath $abiPath
    Reject {Get-ExpectedShowcaseInstanceCapacity $fixture}
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
Write-Output 'PASS: Android witness ABI/log scope contracts; all transport injected, no ADB/build/device action.'
