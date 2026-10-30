$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$source = Get-Content (Join-Path $root 'src/platform/windows/DiagnosticWindow.cpp') -Raw

function Require([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

Require ($source -match 'createStatic\(kCombatTeachingPromptId,\s*"",\s*SS_OWNERDRAW\)') `
    'The lesson prompt must remain a native static text control with owner-drawn presentation.'
Require ($source -match 'item->CtlID\s*==\s*kCombatTeachingPromptId[\s\S]{0,4200}GetWindowTextA\(item->hwndItem,\s*caption') `
    'Owner-drawn lesson plaque must render the current accessible static-control caption.'
Require ($source -match 'NativeUiUsesHighContrast\(\)[\s\S]{0,250}GetSysColor\(COLOR_WINDOWTEXT\)' -and
         $source -match 'GetSysColorBrush\(COLOR_WINDOW\)') `
    'Lesson plaque must use native system colors in high-contrast mode.'
Require ($source -match 'CreateFontA\([\s\S]{0,260}FF_ROMAN,\s*"Georgia"' -and
         $source -match 'kCombatTeachingPromptFontProperty') `
    'Lesson plaque must use and release its DPI-scaled serif title font.'
Require ($source -match 'SetLayeredWindowAttributes\(prompt,\s*0u,\s*(?:context\.combatTeachingFadeOpacity|alpha),\s*LWA_ALPHA\)' -and
         $source -match 'context\.controlsEnabled\s*&&\s*!MeasurementPausedByUi\(context\)') `
    'The plaque must preserve its existing cue fade and modal/input eligibility guards.'
Require ($source -match 'MoveWindow\(prompt,\s*\(width\s*-\s*promptWidth\)\s*/\s*2,\s*ScaleForDpi\(window,\s*72\),\s*promptWidth,\s*ScaleForDpi\(window,\s*76\)') `
    'Lesson plaque must retain the centered top-of-gameplay placement.'

Write-Output 'Windows combat lesson presentation contract passed.'
