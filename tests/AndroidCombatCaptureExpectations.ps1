$ErrorActionPreference = 'Stop'
# Exercise the runner's actual guard without its ADB/install entrypoint.
$runner = Join-Path $PSScriptRoot '../tools/run-android-showcase-validation.ps1'
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($runner, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Android capture runner did not parse' }
$zoneMap = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.AssignmentStatementAst] -and
    $node.Left.Extent.Text -ceq '$checkpointZones'
}, $false))
if ($zoneMap.Count -ne 1) { throw 'Expected one explicit capture zone map' }
. ([scriptblock]::Create($zoneMap[0].Extent.Text))
foreach ($name in @('glass-transport', 'glass-fire-transport', 'glass-tinted-transport',
        'glass-millimetre-closed', 'glass-edge-fresnel')) {
    if ($checkpointZones[$name] -cne 'skylight-chamber') {
        throw "$name must match the real shared-simulation staged zone"
    }
}
$assignments = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.AssignmentStatementAst] -and
    $node.Left.Extent.Text -match '^\$combatCaptureExpectations(?:\[|$)'
}, $false))
if ($assignments.Count -ne 4) { throw 'Review changed combat expectation declarations' }
if (-not ($assignments | Where-Object { $_.Extent.Text -match "player-viewmodel-lantern-low-parry" })) {
    throw 'Expected an explicit low-lantern parry combat expectation'
}
foreach ($assignment in $assignments) { . ([scriptblock]::Create($assignment.Extent.Text)) }
$guards = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.IfStatementAst] -and
    $node.Extent.Text.StartsWith('if ($combatCaptureExpectations.ContainsKey($Checkpoint))')
}, $true))
if ($guards.Count -ne 1) { throw 'Expected one actual capture combat guard' }
$guard = [scriptblock]::Create($guards[0].Extent.Text)
$cases = 0
foreach ($Checkpoint in @('player-body-downward-cut', 'player-body-upward-slice',
                           'player-viewmodel-downward-cut', 'player-viewmodel-upward-slice')) {
    $upward = $Checkpoint.EndsWith('upward-slice')
    # Independently specified late-active values, also tested against native staging.
    $action = if ($upward) { 'upward-active' } else { 'swing-active' }
    $walkTime = if ($upward) { 0.6167 } else { 0.5833 }
    $actionTime = if ($upward) { 0.1667 } else { 0.4033 }
    $edges = if ($upward) { 2 } else { 1 }
    foreach ($mutation in @('valid', 'zero-time', 'early-phase', 'wrong-action', 'lost-edge', 'lost-event')) {
        $state = [pscustomobject]@{ animationTime = $walkTime; playerCombat = [pscustomobject]@{
            action = $action; actionTime = $actionTime; lastConsumedAttackSequence = $edges } }
        $escapedName = [regex]::Escape($Checkpoint)
        $log = "HORDE_COMBO_STAGE checkpoint=$Checkpoint staged=1 consumed_attack_edges=$edges player_swing_events=$edges enemy_hit_events=0 action=$action action_time=$actionTime events_cleared=1"
        switch ($mutation) {
            'zero-time' { $state.animationTime = 0.0 }
            'early-phase' { $state.playerCombat.actionTime = 0.08 }
            'wrong-action' { $state.playerCombat.action = 'idle' }
            'lost-edge' { $state.playerCombat.lastConsumedAttackSequence = 0 }
            'lost-event' { $log = $log.Replace("player_swing_events=$edges", 'player_swing_events=0') }
        }
        $failures = [Collections.Generic.List[string]]::new()
        . $guard
        if (($failures.Count -eq 0) -ne ($mutation -eq 'valid')) {
            throw "$Checkpoint/$mutation produced unexpected guard result: $failures"
        }
        ++$cases
    }
}

$Checkpoint = 'player-viewmodel-lantern-low-parry'
$expectedCombat = $combatCaptureExpectations[$Checkpoint]
$state = [pscustomobject]@{ animationTime = 0.15; playerCombat = [pscustomobject]@{
    action = 'parry-active'; actionTime = 0.11; lastConsumedParrySequence = 1 } }
$escapedName = [regex]::Escape($Checkpoint)
$log = "HORDE_PARRY_STAGE checkpoint=$Checkpoint staged=1 consumed_parry_edges=1 parry_success_events=0 player_damaged_events=0 player_killed_events=0 enemy_hit_events=0 action=parry-active action_time=0.1100 events_cleared=1"
foreach ($mutation in @('valid', 'wrong-phase', 'wrong-time', 'missing-parry-edge',
                        'parry-success-event', 'player-damage-event', 'player-killed-event',
                        'stage-failed', 'missing-log', 'enemy-hit-event')) {
    $state = [pscustomobject]@{ animationTime = 0.15; playerCombat = [pscustomobject]@{
        action = 'parry-active'; actionTime = 0.11; lastConsumedParrySequence = 1 } }
    $log = "HORDE_PARRY_STAGE checkpoint=$Checkpoint staged=1 consumed_parry_edges=1 parry_success_events=0 player_damaged_events=0 player_killed_events=0 enemy_hit_events=0 action=parry-active action_time=0.1100 events_cleared=1"
    switch ($mutation) {
        'wrong-phase' { $state.playerCombat.action = 'parry-startup' }
        'wrong-time' { $state.playerCombat.actionTime = 0.04 }
        'missing-parry-edge' { $state.playerCombat.lastConsumedParrySequence = 0 }
        'parry-success-event' { $log = $log.Replace('parry_success_events=0', 'parry_success_events=1') }
        'player-damage-event' { $log = $log.Replace('player_damaged_events=0', 'player_damaged_events=1') }
        'player-killed-event' { $log = $log.Replace('player_killed_events=0', 'player_killed_events=1') }
        'stage-failed' { $log = $log.Replace('staged=1', 'staged=0') }
        'missing-log' { $log = '' }
        'enemy-hit-event' { $log = $log.Replace('enemy_hit_events=0', 'enemy_hit_events=1') }
    }
    $failures = [Collections.Generic.List[string]]::new()
    . $guard
    if (($failures.Count -eq 0) -ne ($mutation -eq 'valid')) {
        throw "$Checkpoint/$mutation produced unexpected guard result: $failures"
    }
    ++$cases
}
Write-Output "Android combat capture expectations: $cases cases passed."

$ownershipFunction = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -ceq 'Test-CheckpointPlayerOwnership'
}, $false))
if ($ownershipFunction.Count -ne 1) { throw 'Expected the shared capture/benchmark ownership guard' }
. ([scriptblock]::Create($ownershipFunction[0].Extent.Text))
$ownershipCases = 0
foreach ($checkpointName in @('opening', 'lantern-held-high',
        'player-viewmodel-lantern-low-parry', 'lantern-glass-production')) {
    foreach ($owned in @($true, $false)) {
        foreach ($inspectionFlags in @($true, $false)) {
            $state = [pscustomobject]@{
                dedicatedPlayerPrimaryOwnership = $owned
                rtLab = [pscustomobject]@{
                    productionRewardPropsVisible = $inspectionFlags
                    productionLanternGlassOnly = $inspectionFlags
                }
            }
            $expected = if ($checkpointName -ceq 'lantern-glass-production') {
                $inspectionFlags -and -not $owned
            } else { $owned }
            if ((Test-CheckpointPlayerOwnership $checkpointName $state) -ne $expected) {
                throw "Unexpected player ownership result: $checkpointName/$owned/$inspectionFlags"
            }
            ++$ownershipCases
        }
    }
}
Write-Output "Android gameplay versus isolated-glass ownership: $ownershipCases cases passed."
