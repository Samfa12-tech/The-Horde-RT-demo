$ErrorActionPreference = 'Stop'
# Exercise the runner's actual guard without its ADB/install entrypoint.
$runner = Join-Path $PSScriptRoot '../tools/run-android-showcase-validation.ps1'
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($runner, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Android capture runner did not parse' }
$assignments = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.AssignmentStatementAst] -and
    $node.Left.Extent.Text -match '^\$combatCaptureExpectations(?:\[|$)'
}, $false))
if ($assignments.Count -ne 3) { throw 'Review changed combat expectation declarations' }
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
Write-Output "Android combat capture expectations: $cases cases passed."
