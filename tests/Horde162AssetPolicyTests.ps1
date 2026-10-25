$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $repo 'tools/horde-1.6.2-asset-policy.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$scratch=Join-Path $temp ('Horde162AssetPolicy-'+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $scratch
$script:checks=0
function Require([bool]$Value,[string]$Message) { if(-not $Value){throw $Message} }
function Assert-CollapseGradleInventory([string]$Text) {
    $names=@([regex]::Matches($Text, "(?m)^\s*include '([^']+)'\s*$") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -like 'models/world/*' })
    $expected=@('models/world/runtime/collapsed-entry/asset.manifest.json','models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb',
        'models/world/runtime/forest-tree-pair-v1/asset.manifest.json','models/world/runtime/forest-tree-pair-v1/runtime-roster.json',
        'models/world/runtime/forest-rocks-v1/asset.manifest.json',
        'models/world/runtime/forest-deadwood-v1/asset.manifest.json',
        'models/world/runtime/forest-deadwood-v1/horde-fallen-log.glb',
        'models/world/runtime/forest-deadwood-v1/horde-root-tangle.glb',
        'models/world/runtime/forest-deadwood-v1/horde-gnarled-stump.glb',
        'models/world/runtime/forest-deadwood-v1/horde-dead-snag-lod2.glb',
        'models/world/runtime/forest-rocks-v1/horde-rock-low_boulder.glb',
        'models/world/runtime/forest-rocks-v1/horde-rock-split_shelf.glb',
        'models/world/runtime/forest-rocks-v1/horde-rock-upright_crag.glb',
        'models/world/runtime/forest-road-clue-v1/asset.manifest.json',
        'models/world/runtime/forest-road-clue-v1/horde-road-kerb.glb',
        'models/world/runtime/forest-road-clue-v1/horde-road-waystone.glb',
        'models/world/runtime/bellwether-shell-v4/asset.manifest.json',
        'models/world/runtime/bellwether-shell-v4/bellwether-shell-v4.glb',
        'models/world/runtime/forest-understory-v1/asset.manifest.json',
        'models/world/runtime/forest-understory-v1/horde-fern-open-clump.glb',
        'models/world/runtime/forest-understory-v1/horde-fern-small-clump.glb',
        'models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb','models/world/runtime/forest-tree-pair-v1/horde-upright-alder-v1-lod1.glb') + @(Get-Horde17TombAssetSpecification | Where-Object {$_.Path -like 'models/world/*'} | ForEach-Object Path)
    Require ([string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive)) -ceq [string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive))) 'Gradle world assets must match the exact collapse, forest and tomb runtime rosters.'
}
function Assert-PropsGradleInventory([string]$Text) {
    $names=@([regex]::Matches($Text, "(?m)^\s*include '([^']+)'\s*$") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -like 'textures/props/*' })
    $expected=@('textures/props/runtime/asset.manifest.json','textures/props/runtime/base-color.android.ktx2',
                'textures/props/runtime/normal.android.ktx2','textures/props/runtime/orm.android.ktx2','textures/props/runtime/emissive.android.ktx2',
                'textures/props/runtime/chapter-terrain/base-color.android.ktx2',
                'textures/props/runtime/chapter-terrain/normal.android.ktx2',
                'textures/props/runtime/chapter-terrain/orm.android.ktx2')
    Require ([string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive)) -ceq [string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive))) 'Gradle props assets must be the exact eight-file Android runtime roster.'
}
function Assert-RagTorchGradleInventory([string]$Text) {
    $names=@([regex]::Matches($Text, "(?m)^\s*include '([^']+)'\s*$") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -like 'models/props/runtime/player-rag-torch/*' })
    $expected=@('models/props/runtime/player-rag-torch/asset.manifest.json','models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb')
    Require ([string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive)) -ceq [string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive))) 'Gradle player Rag torch assets must be the exact two-file Android runtime roster.'
}
function Assert-EquipmentGradleInventory([string]$Text) {
    $names=@([regex]::Matches($Text, "(?m)^\s*include '([^']+)'\s*$") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -like 'audio/filmcow/equipment/*' })
    $expected=@('audio/filmcow/equipment/asset.manifest.json','audio/filmcow/equipment/sword_draw.wav','audio/filmcow/equipment/sword_sheath.wav')
    Require ([string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive)) -ceq [string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive))) 'Gradle equipment audio must be the exact three-file runtime roster.'
}
function Assert-MenuGradleInventory([string]$Text) {
    $names=@([regex]::Matches($Text, "(?m)^\s*include '([^']+)'\s*$") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -like 'audio/menu/*' })
    $expected=@('audio/menu/asset.manifest.json','audio/menu/menu_room.wav','audio/menu/menu_chain.wav')
    Require ([string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive)) -ceq [string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive))) 'Gradle menu ambience must be the exact three-file runtime roster.'
}
function Assert-ScabbardGradleInventory([string]$Text) {
    $names=@([regex]::Matches($Text, "(?m)^\s*include '([^']+)'\s*$") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -like 'models/props/runtime/player-sword-scabbard/*' })
    $expected=@('models/props/runtime/player-sword-scabbard/asset.manifest.json','models/props/runtime/player-sword-scabbard/processing-receipt.json','models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb')
    Require ([string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive)) -ceq [string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive))) 'Gradle scabbard must be the exact three-file runtime roster.'
}
function Assert-EquipmentAndroidStaging([string]$Gradle, [string]$Activity) {
    # Admission to the APK alone does not make assets visible to native file IO.
    # Check the actual packaged paths against startup staging before writeReports
    # publishes the files root used by the Showcase renderer.
    $required=@([regex]::Matches($Gradle, "(?m)^\s*include '([^']+)'\s*$") |
        ForEach-Object { $_.Groups[1].Value } |
        Where-Object { $_ -like 'models/props/runtime/player-rag-torch/*' -or
                       $_ -like 'models/props/runtime/player-sword-scabbard/*' -or
                       $_ -like 'models/world/runtime/chapter-entry-return-v1/*' -or
                       $_ -like 'models/world/runtime/chapter-terrain-v1/*' -or
                       $_ -like 'textures/props/runtime/chapter-terrain/*.android.ktx2' -or
                       $_ -like 'models/characters/runtime/kit-warden-v1/*' -or
                       $_ -like 'textures/characters/runtime/kit-warden-v1/*.android.ktx2' })
    Require ($required.Count -eq 18) 'Android equipment, entry and terrain staging require the closed eighteen-file package roster.'
    $startup=@([regex]::Matches($Activity,
        '(?ms)^    StartupDiagnostics prepareInitialDiagnostics\(\)\s*\{(.*?)^    \}'))
    $helper=@([regex]::Matches($Activity,
        '(?ms)^    private boolean\[\] stageRuntimeAssetCache\(\)\s*\{(.*?)^    \}'))
    Require ($startup.Count -eq 1 -and $helper.Count -eq 1) 'Android equipment staging requires unique startup and runtime-cache methods.'
    $body=$startup[0].Groups[1].Value
    $publication=[regex]::Match($body, 'ProbeBridge[.]writeReports\(filesRoot\)')
    $invocations=@([regex]::Matches($body, 'stageRuntimeAssetCache\(\)'))
    Require ($publication.Success -and $invocations.Count -eq 1 -and
             $invocations[0].Index -lt $publication.Index) 'Android runtime-cache staging must run once before native report-root publication.'
    $staged=@([regex]::Matches($helper[0].Groups[1].Value,
        'stageAsset\("([^"]+)",\s*"([^"]+)"\)'))
    $allStaged=@([regex]::Matches($Activity, 'stageAsset\("([^"]+)",\s*"([^"]+)"\)'))
    foreach($path in $required) {
        $matches=@($staged | Where-Object { $_.Groups[1].Value -ceq $path })
        $allMatches=@($allStaged | Where-Object { $_.Groups[1].Value -ceq $path })
        Require ($matches.Count -eq 1 -and $allMatches.Count -eq 1 -and
                 $matches[0].Groups[2].Value -ceq $path) "Android equipment asset must be staged once at its exact native path: $path"
    }
}
function Assert-CollapsePackageInventory([string]$Text) {
    $errors=$null; $ast=[Management.Automation.Language.Parser]::ParseInput($Text,[ref]$null,[ref]$errors)
    Require ($errors.Count -eq 0) 'Package inventory script failed parsing.'
    $strings=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.StringConstantExpressionAst]},$true) | ForEach-Object Value)
    foreach ($name in @('assets/models/world/runtime/collapsed-entry/asset.manifest.json','assets/models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb')) {
        Require (@($strings | Where-Object { $_ -ceq $name }).Count -eq 2) 'Package inventory must require the collapse pair in both platform archives.'
    }
    foreach ($name in @('assets/models/world/runtime/forest-tree-pair-v1/asset.manifest.json',
                       'assets/models/world/runtime/forest-tree-pair-v1/runtime-roster.json',
                       'assets/models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb',
                       'assets/models/world/runtime/forest-tree-pair-v1/horde-upright-alder-v1-lod1.glb')) {
        Require (@($strings | Where-Object { $_ -ceq $name }).Count -eq 2) 'Package inventory must require the exact forest tree-pair roster in both platform archives.'
    }
    $commands=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.CommandAst]},$true) | ForEach-Object { $_.GetCommandName() })
    Require ($commands -contains 'Copy-Horde162RuntimeAssets' -and @($commands | Where-Object { $_ -ceq 'Assert-Horde162Package' }).Count -ge 2) 'Package inventory must use closed staging and admission.'
    $parameters=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.CommandParameterAst]},$true) | ForEach-Object ParameterName)
    Require (@($parameters | Where-Object { $_ -ceq 'RequireHorde162World' }).Count -eq 1) 'Current package must request the new world contract switch.'
}
function Assert-RagTorchPackageInventory([string]$Text) {
    $errors=$null; $ast=[Management.Automation.Language.Parser]::ParseInput($Text,[ref]$null,[ref]$errors)
    Require ($errors.Count -eq 0) 'Package inventory script failed parsing.'
    $strings=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.StringConstantExpressionAst]},$true) | ForEach-Object Value)
    foreach ($name in @('assets/models/props/runtime/player-rag-torch/asset.manifest.json','assets/models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb')) {
        Require (@($strings | Where-Object { $_ -ceq $name }).Count -eq 2) 'Package inventory must require the player Rag torch pair in both platform archives.'
    }
}
function Assert-EquipmentAndScabbardPackageInventory([string]$Text) {
    $errors=$null; $ast=[Management.Automation.Language.Parser]::ParseInput($Text,[ref]$null,[ref]$errors)
    Require ($errors.Count -eq 0) 'Package inventory script failed parsing.'
    $strings=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.StringConstantExpressionAst]},$true) | ForEach-Object Value)
    foreach ($name in @('assets/audio/filmcow/equipment/asset.manifest.json','assets/audio/filmcow/equipment/sword_draw.wav','assets/audio/filmcow/equipment/sword_sheath.wav',
                       'assets/models/props/runtime/player-sword-scabbard/asset.manifest.json','assets/models/props/runtime/player-sword-scabbard/processing-receipt.json','assets/models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb')) {
        Require (@($strings | Where-Object { $_ -ceq $name }).Count -eq 2) 'Package inventory must require every equipment/scabbard runtime file in both platform archives.'
    }
}
function Expect-Failure([scriptblock]$Action,[string]$Message) {
    $observed=''; try { & $Action | Out-Null } catch { $observed=$_.Exception.Message }
    Require (-not [string]::IsNullOrWhiteSpace($observed)) 'Negative asset fixture unexpectedly passed.'
    Require ($observed -like "*$Message*") "Negative fixture failed for an unrelated reason: $observed"
    ++$script:checks
}
function New-FixtureArchive([string]$Name,[string]$Platform,[string]$Omit='', [string]$Extra='', [string]$Duplicate='', [string]$Corrupt='', [string[]]$Directories=@(), [string]$EmptyFile='') {
    $path=Join-Path $scratch $Name
    # Compress the large canonical native atlases once per platform. ZIP Update
    # retains unchanged compressed entries; each negative mutates only its target.
    $base=Join-Path $scratch "$Platform-canonical-base.zip"
    if (-not (Test-Path -LiteralPath $base)) {
        $seed=[IO.Compression.ZipFile]::Open($base,[IO.Compression.ZipArchiveMode]::Create)
        try {
            foreach ($relative in Get-Horde162RuntimeFiles $repo $Platform) {
                $null=[IO.Compression.ZipFileExtensions]::CreateEntryFromFile($seed,(Join-Horde162Path $repo "assets/$relative"),"assets/$relative",[IO.Compression.CompressionLevel]::Fastest)
            }
        } finally { $seed.Dispose() }
    }
    Copy-Item -LiteralPath $base -Destination $path
    $zip=[IO.Compression.ZipFile]::Open($path,[IO.Compression.ZipArchiveMode]::Update)
    try {
        if ($Omit) { $zip.GetEntry($Omit).Delete() }
        if ($Duplicate) { $null=[IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,(Join-Horde162Path $repo $Duplicate),$Duplicate,[IO.Compression.CompressionLevel]::Fastest) }
        if ($Corrupt) {
            $zip.GetEntry($Corrupt).Delete()
            $bytes=[IO.File]::ReadAllBytes((Join-Horde162Path $repo $Corrupt));$bytes[$bytes.Length-1]=$bytes[$bytes.Length-1] -bxor 1
            $entry=$zip.CreateEntry($Corrupt,[IO.Compression.CompressionLevel]::Fastest);$stream=$entry.Open()
            try{$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
        }
        if($Extra){
            $entry=$zip.CreateEntry($Extra);$stream=$entry.Open()
            try{$bytes=[Text.Encoding]::ASCII.GetBytes('synthetic foreign source, never admitted');$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
        }
        foreach($directory in $Directories){$null=$zip.CreateEntry($directory)}
        if($EmptyFile){$null=$zip.CreateEntry($EmptyFile)}
        # A Debug candidate may contain unrelated native/signing metadata. The
        # asset validator intentionally does not mistake this for Release proof.
        $entry=$zip.CreateEntry('META-INF/debug-fixture.txt');$stream=$entry.Open()
        try{$stream.WriteByte(1)}finally{$stream.Dispose()}
    }finally{$zip.Dispose()}
    return $path
}
function New-HeldFixtureArchive([string]$Name,[string[]]$Omit=@(), [string]$Duplicate='', [string]$Corrupt='') {
    $contract=Join-Path $repo 'tools/test-held-item-package-contract.ps1'
    $ast=[Management.Automation.Language.Parser]::ParseFile($contract,[ref]$null,[ref]$null)
    $assignments=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.AssignmentStatementAst] -and
        $node.Left -is [Management.Automation.Language.VariableExpressionAst] -and $node.Left.VariablePath.UserPath -ceq 'requiredEntries'},$true))
    $names=@($assignments | ForEach-Object { $_.Right.FindAll({param($node) $node -is [Management.Automation.Language.ExpandableStringExpressionAst]},$true) } | ForEach-Object { $_.Value.Replace('$AssetPrefix','assets') })
    $path=Join-Path $scratch $Name; $zip=[IO.Compression.ZipFile]::Open($path,[IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($name in ($names+@('assets/ASSET_LICENSES.md','ASSET_LICENSES.md'))) {
            if ($Omit -ccontains $name) { continue }
            $source=if($name -like '*ASSET_LICENSES.md'){Join-Path $repo 'ASSET_LICENSES.md'}else{Join-Horde162Path $repo $name}
            $bytes=[IO.File]::ReadAllBytes($source)
            if($name -ceq $Corrupt){$bytes[$bytes.Length-1]=$bytes[$bytes.Length-1] -bxor 1}
            $copies=if($name -ceq $Duplicate){2}else{1}
            for($copy=0;$copy -lt $copies;++$copy){$entry=$zip.CreateEntry($name);$stream=$entry.Open();try{$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}}
        }
    } finally {$zip.Dispose()}
    return $path
}
try {
    $gradle=Get-Content (Join-Path $repo 'android/app/build.gradle') -Raw
    Assert-CollapseGradleInventory $gradle; ++$script:checks
    Expect-Failure {Assert-CollapseGradleInventory ($gradle.Replace("        include 'models/world/runtime/collapsed-entry/asset.manifest.json'",''))} 'exact collapse, forest and tomb runtime rosters'
    Expect-Failure {Assert-CollapseGradleInventory ($gradle.Replace("        include 'models/world/runtime/forest-tree-pair-v1/runtime-roster.json'",''))} 'exact collapse, forest and tomb runtime rosters'
    Expect-Failure {Assert-CollapseGradleInventory ($gradle+"`ninclude 'models/world/source/*.blend'`n")} 'exact collapse, forest and tomb runtime rosters'
    Assert-PropsGradleInventory $gradle; ++$script:checks
    Expect-Failure {Assert-PropsGradleInventory ($gradle.Replace('textures/props/runtime/base-color.android.ktx2','textures/props/runtime/*.android.ktx2'))} 'exact eight-file Android runtime roster'
    Expect-Failure {Assert-PropsGradleInventory ($gradle+"`ninclude 'textures/props/runtime/base-color.windows.ktx2'`n")} 'exact eight-file Android runtime roster'
    Assert-RagTorchGradleInventory $gradle; ++$script:checks
    Expect-Failure {Assert-RagTorchGradleInventory ($gradle.Replace("        include 'models/props/runtime/player-rag-torch/asset.manifest.json'",''))} 'exact two-file Android runtime roster'
    Expect-Failure {Assert-RagTorchGradleInventory ($gradle+"`ninclude 'models/props/runtime/player-rag-torch/*.glb'`n")} 'exact two-file Android runtime roster'
    Assert-EquipmentGradleInventory $gradle; ++$script:checks
    Expect-Failure {Assert-EquipmentGradleInventory ($gradle.Replace("        include 'audio/filmcow/equipment/sword_draw.wav'",''))} 'exact three-file runtime roster'
    Expect-Failure {Assert-EquipmentGradleInventory ($gradle+"`ninclude 'audio/filmcow/equipment/*.wav'`n")} 'exact three-file runtime roster'
    Assert-MenuGradleInventory $gradle; ++$script:checks
    Expect-Failure {Assert-MenuGradleInventory ($gradle.Replace("        include 'audio/menu/menu_chain.wav'",''))} 'exact three-file runtime roster'
    Expect-Failure {Assert-MenuGradleInventory ($gradle+"`ninclude 'audio/menu/*.wav'`n")} 'exact three-file runtime roster'
    Assert-ScabbardGradleInventory $gradle; ++$script:checks
    Expect-Failure {Assert-ScabbardGradleInventory ($gradle.Replace("        include 'models/props/runtime/player-sword-scabbard/processing-receipt.json'",''))} 'exact three-file runtime roster'
    Expect-Failure {Assert-ScabbardGradleInventory ($gradle+"`ninclude 'models/props/runtime/player-sword-scabbard/*.glb'`n")} 'exact three-file runtime roster'
    $activity=Get-Content (Join-Path $repo 'android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java') -Raw
    Assert-EquipmentAndroidStaging $gradle $activity; ++$script:checks
    foreach($path in @('models/props/runtime/player-rag-torch/asset.manifest.json',
                       'models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb',
                       'models/props/runtime/player-sword-scabbard/asset.manifest.json',
                       'models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb',
                       'models/props/runtime/player-sword-scabbard/processing-receipt.json',
                       'models/world/runtime/chapter-entry-return-v1/asset.manifest.json',
                       'models/world/runtime/chapter-entry-return-v1/collapsed-entry-lod0.runtime.glb',
                       'models/world/runtime/chapter-terrain-v1/asset.manifest.json',
                       'models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb',
                       'textures/props/runtime/chapter-terrain/base-color.android.ktx2',
                       'textures/props/runtime/chapter-terrain/normal.android.ktx2',
                       'textures/props/runtime/chapter-terrain/orm.android.ktx2',
                       'models/characters/runtime/kit-warden-v1/asset.manifest.json',
                       'models/characters/runtime/kit-warden-v1/kit-warden-turn.runtime.glb',
                       'textures/characters/runtime/kit-warden-v1/base-color.android.ktx2',
                       'textures/characters/runtime/kit-warden-v1/normal.android.ktx2',
                       'textures/characters/runtime/kit-warden-v1/orm.android.ktx2',
                       'textures/characters/runtime/kit-warden-v1/emissive.android.ktx2')) {
        $call='stageAsset\("'+[regex]::Escape($path)+'",\s*"'+[regex]::Escape($path)+'"\)'
        Expect-Failure {Assert-EquipmentAndroidStaging $gradle ([regex]::Replace($activity,$call,'true'))} 'exact native path'
        Expect-Failure {Assert-EquipmentAndroidStaging $gradle ([regex]::Replace($activity,$call,'stageAsset("'+$path+'", "wrong-native-path")'))} 'exact native path'
    }
    $startupCall='final boolean[] staged = stageRuntimeAssetCache();'
    $publicationCall='written=ProbeBridge.writeReports(filesRoot);'
    Expect-Failure {Assert-EquipmentAndroidStaging $gradle ($activity.Replace($startupCall,'final boolean[] staged = new boolean[4];'))} 'once before native report-root publication'
    $late=$activity.Replace($startupCall,'final boolean[] staged = new boolean[4];').Replace($publicationCall,$publicationCall+' stageRuntimeAssetCache();')
    Expect-Failure {Assert-EquipmentAndroidStaging $gradle $late} 'once before native report-root publication'
    Expect-Failure {Assert-EquipmentAndroidStaging $gradle ($activity.Replace($startupCall,$startupCall+' stageRuntimeAssetCache();'))} 'once before native report-root publication'
    $duplicate='stageAsset("models/props/runtime/player-rag-torch/asset.manifest.json", "models/props/runtime/player-rag-torch/asset.manifest.json");'
    Expect-Failure {Assert-EquipmentAndroidStaging $gradle ($activity.Replace($startupCall,$startupCall+$duplicate))} 'exact native path'
    Expect-Failure {Assert-EquipmentAndroidStaging ($gradle+"`ninclude 'models/characters/runtime/kit-warden-v1/unadmitted.glb'`n") $activity} 'closed eighteen-file package roster'
    foreach ($scriptPath in @('tools/package-alpha.ps1','tools/run-foundation-validation.ps1')) {
        $text=Get-Content (Join-Path $repo $scriptPath) -Raw
        Assert-CollapsePackageInventory $text; ++$script:checks
        Expect-Failure {Assert-CollapsePackageInventory ($text.Replace('assets/models/world/runtime/collapsed-entry/asset.manifest.json','assets/models/world/runtime/collapsed-entry/omitted.json'))} 'both platform archives'
        Expect-Failure {Assert-CollapsePackageInventory ($text.Replace('Copy-Horde162RuntimeAssets','Copy-UnvalidatedWorld'))} 'closed staging and admission'
        Expect-Failure {Assert-CollapsePackageInventory ($text.Replace('-RequireHorde162World',''))} 'new world contract switch'
    }
    $packageText=Get-Content (Join-Path $repo 'tools/package-alpha.ps1') -Raw
    Assert-RagTorchPackageInventory $packageText; ++$script:checks
    Expect-Failure {Assert-RagTorchPackageInventory ($packageText.Replace('assets/models/props/runtime/player-rag-torch/asset.manifest.json','assets/models/props/runtime/player-rag-torch/omitted.json'))} 'both platform archives'
    Expect-Failure {Assert-RagTorchPackageInventory ($packageText.Replace('assets/models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb','assets/models/props/runtime/player-rag-torch/omitted.glb'))} 'both platform archives'
    $packageAst=[Management.Automation.Language.Parser]::ParseInput($packageText,[ref]$null,[ref]$null)
    $packageStrings=@($packageAst.FindAll({param($node) $node -is [Management.Automation.Language.StringConstantExpressionAst]},$true) | ForEach-Object Value)
    foreach($name in @('asset.manifest.json','menu_room.wav','menu_chain.wav')) {
        Require (@($packageStrings | Where-Object {$_ -ceq ('assets/audio/menu/'+$name)}).Count -eq 2) 'Both platform archives must explicitly require each menu ambience asset.'
    }
    ++$script:checks
    Assert-EquipmentAndScabbardPackageInventory $packageText; ++$script:checks
    Expect-Failure {Assert-EquipmentAndScabbardPackageInventory ($packageText.Replace('assets/audio/filmcow/equipment/sword_draw.wav','assets/audio/filmcow/equipment/omitted.wav'))} 'both platform archives'
    Expect-Failure {Assert-EquipmentAndScabbardPackageInventory ($packageText.Replace('assets/models/props/runtime/player-sword-scabbard/processing-receipt.json','assets/models/props/runtime/player-sword-scabbard/omitted.json'))} 'both platform archives'
    $null=Assert-Horde162Assets $repo
    ++$script:checks
    $heldContract=Join-Path $repo 'tools/test-held-item-package-contract.ps1'
    $heldArchive=New-HeldFixtureArchive 'held-world-positive.zip'
    & $heldContract -AndroidApkPath $heldArchive -WindowsZipPath $heldArchive -RequireHorde162World
    ++$script:checks
    $legacyArchive=New-HeldFixtureArchive 'legacy-held-positive.zip' -Omit @('assets/models/world/runtime/collapsed-entry/asset.manifest.json','assets/models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb')
    & $heldContract -AndroidApkPath $legacyArchive -WindowsZipPath $legacyArchive
    ++$script:checks
    Expect-Failure {& $heldContract -AndroidApkPath $legacyArchive -RequireHorde162World} 'missing exact entry'
    foreach ($name in @('asset.manifest.json','collapsed-entry-lod0.runtime.glb')) {
        $path="assets/models/world/runtime/collapsed-entry/$name"
        $heldArchive=New-HeldFixtureArchive ('held-missing-'+$name+'.zip') -Omit $path
        Expect-Failure {& $heldContract -AndroidApkPath $heldArchive -RequireHorde162World} 'missing exact entry'
        $heldArchive=New-HeldFixtureArchive ('held-duplicate-'+$name+'.zip') -Duplicate $path
        Expect-Failure {& $heldContract -AndroidApkPath $heldArchive -RequireHorde162World} 'exactly one'
        $heldArchive=New-HeldFixtureArchive ('held-corrupt-'+$name+'.zip') -Corrupt $path
        Expect-Failure {& $heldContract -AndroidApkPath $heldArchive -RequireHorde162World} 'byte/hash mismatch'
    }
    foreach ($name in @('asset.manifest.json','collapsed-entry-lod0.runtime.glb')) {
        $path="assets/models/world/runtime/chapter-entry-return-v1/$name"
        $heldArchive=New-HeldFixtureArchive ('chapter-missing-'+$name+'.zip') -Omit $path
        Expect-Failure {& $heldContract -AndroidApkPath $heldArchive} 'missing exact entry'
        $heldArchive=New-HeldFixtureArchive ('chapter-duplicate-'+$name+'.zip') -Duplicate $path
        Expect-Failure {& $heldContract -AndroidApkPath $heldArchive} 'exactly one'
        $heldArchive=New-HeldFixtureArchive ('chapter-corrupt-'+$name+'.zip') -Corrupt $path
        Expect-Failure {& $heldContract -AndroidApkPath $heldArchive} 'byte/hash mismatch'
    }
    $windows=@(Get-Horde162RuntimeFiles $repo Windows);$android=@(Get-Horde162RuntimeFiles $repo Android)
    $baseWindows=@(Get-Horde162AssetSpecification | Where-Object {$_.Platform -ceq 'Both' -or $_.Platform -ceq 'Windows'})
    $baseAndroid=@(Get-Horde162AssetSpecification | Where-Object {$_.Platform -ceq 'Both' -or $_.Platform -ceq 'Android'})
    Require ($baseWindows.Count + @(Get-Horde162ManifestSpecification | Where-Object {$_.Platform -ceq 'Both' -or $_.Platform -ceq 'Windows'}).Count -eq 50 -and
             $baseAndroid.Count + @(Get-Horde162ManifestSpecification | Where-Object {$_.Platform -ceq 'Both' -or $_.Platform -ceq 'Android'}).Count -eq 51) 'Current shared asset roster includes five exact prepared deadwood and three exact road-clue payloads.'
    Require ($windows.Count -eq 107 -and $android.Count -eq 108) 'Current closed roster includes exact tomb, terrain, tree-pair, Kit, grave-marker, understory, town, deadwood and road-clue runtime assets; no editable sources.'
    $deadwoodFiles=@('models/world/runtime/forest-deadwood-v1/asset.manifest.json',
        'models/world/runtime/forest-deadwood-v1/horde-fallen-log.glb',
        'models/world/runtime/forest-deadwood-v1/horde-root-tangle.glb',
        'models/world/runtime/forest-deadwood-v1/horde-gnarled-stump.glb',
        'models/world/runtime/forest-deadwood-v1/horde-dead-snag-lod2.glb')
    Require (@($windows | Where-Object {$_ -like 'models/world/runtime/forest-deadwood-*/*'}).Count -eq 5 -and
             @($android | Where-Object {$_ -like 'models/world/runtime/forest-deadwood-*/*'}).Count -eq 5) 'Both platforms admit only five exact deadwood payloads.'
    foreach($piece in $deadwoodFiles) {
        Require (@($windows | Where-Object {$_ -ceq $piece}).Count -eq 1 -and
                 @($android | Where-Object {$_ -ceq $piece}).Count -eq 1) 'Each exact deadwood payload is admitted once on both platforms.'
    }
    $rockFiles=@('models/world/runtime/forest-rocks-v1/asset.manifest.json',
        'models/world/runtime/forest-rocks-v1/horde-rock-low_boulder.glb',
        'models/world/runtime/forest-rocks-v1/horde-rock-split_shelf.glb',
        'models/world/runtime/forest-rocks-v1/horde-rock-upright_crag.glb')
    Require (@($windows | Where-Object {$_ -like 'models/world/runtime/forest-rocks-*/*'}).Count -eq 4 -and
             @($android | Where-Object {$_ -like 'models/world/runtime/forest-rocks-*/*'}).Count -eq 4) 'Both platforms admit only four final rock payloads.'
    foreach($rock in $rockFiles) {
        Require (@($windows | Where-Object {$_ -ceq $rock}).Count -eq 1 -and
                 @($android | Where-Object {$_ -ceq $rock}).Count -eq 1) 'Each exact rock payload is admitted once.'
    }
    $townFiles=@('models/world/runtime/bellwether-shell-v4/asset.manifest.json',
        'models/world/runtime/bellwether-shell-v4/bellwether-shell-v4.glb')
    Require (@($windows | Where-Object {$_ -like 'models/world/runtime/bellwether-shell-*/*'}).Count -eq 2 -and
             @($android | Where-Object {$_ -like 'models/world/runtime/bellwether-shell-*/*'}).Count -eq 2) 'Both platforms admit only the two final closed town payloads.'
    foreach($town in $townFiles) {
        Require (@($windows | Where-Object {$_ -ceq $town}).Count -eq 1 -and
                 @($android | Where-Object {$_ -ceq $town}).Count -eq 1) 'Each exact town payload is admitted once on both platforms.'
    }
    $understory=@('models/world/runtime/forest-understory-v1/asset.manifest.json',
        'models/world/runtime/forest-understory-v1/horde-fern-open-clump.glb',
        'models/world/runtime/forest-understory-v1/horde-fern-small-clump.glb')
    Require (@($windows | Where-Object {$_ -like 'models/world/runtime/forest-understory-v1/*'}).Count -eq 3 -and
             @($android | Where-Object {$_ -like 'models/world/runtime/forest-understory-v1/*'}).Count -eq 3) 'Both platforms admit only the three final understory files.'
    foreach($clump in $understory) {
        Require (@($windows | Where-Object {$_ -ceq $clump}).Count -eq 1 -and
                 @($android | Where-Object {$_ -ceq $clump}).Count -eq 1) 'Each exact understory payload is admitted once on both platforms.'
    }
    $graveMarkers=@('models/world/runtime/grave-markers-r03/grave-marker-round.manifest.json',
        'models/world/runtime/grave-markers-r03/grave-marker-round.runtime.glb',
        'models/world/runtime/grave-markers-r03/grave-marker-damaged.manifest.json',
        'models/world/runtime/grave-markers-r03/grave-marker-damaged.runtime.glb')
    Require (@($windows | Where-Object {$_ -like 'models/world/runtime/grave-markers-*/*'}).Count -eq 4 -and
             @($android | Where-Object {$_ -like 'models/world/runtime/grave-markers-*/*'}).Count -eq 4) 'Both platforms admit only the four final grave-marker files.'
    foreach($marker in $graveMarkers) {
        Require (@($windows | Where-Object {$_ -ceq $marker}).Count -eq 1 -and
                 @($android | Where-Object {$_ -ceq $marker}).Count -eq 1) 'Each exact grave-marker path must be admitted once on both platforms.'
    }
    Require (@(Get-Horde17KitAssetSpecification).Count -eq 14) 'Kit admission must retain its exact thirteen-cut/one-manifest roster.'
    foreach ($kit in Get-Horde17KitAssetSpecification) {
        Require ($windows -ccontains $kit.Path -and $android -ccontains $kit.Path) 'Both platforms must admit every approved Kit runtime entry.'
        Require (@($packageStrings | Where-Object { $_ -ceq ('assets/'+$kit.Path) }).Count -eq 2) 'Both package inventories must explicitly require every Kit runtime entry.'
    }
    $kitSpec=@(Get-Horde17KitAssetSpecification | Where-Object {$_.Kind -ceq 'KitWave'})[0]
    $kitWave=Join-Horde162Path $repo ('assets/'+$kitSpec.Path)
    Assert-Horde17KitWave $kitWave $kitSpec.Frames
    Expect-Failure {Assert-Horde17KitWave $kitWave 0} 'Kit WAV requires'
    Expect-Failure {Assert-Horde17KitWave $kitWave 156001} 'Kit WAV requires'
    $badKitWave=Join-Path $scratch 'kit-wrong-rate.wav'
    $kitBytes=[IO.File]::ReadAllBytes($kitWave)
    [BitConverter]::GetBytes([uint32]48000).CopyTo($kitBytes,24)
    [IO.File]::WriteAllBytes($badKitWave,$kitBytes)
    Expect-Failure {Assert-Horde17KitWave $badKitWave $kitSpec.Frames} 'Kit WAV requires'
    Require ($windows -ccontains 'audio/pixabay/waterfall_loop.wav' -and $windows -cnotcontains 'audio/pixabay/waterfall_core_loop.wav') 'Windows must preserve accepted full waterfall only.'
    Require ($android -ccontains 'audio/pixabay/waterfall_core_loop.wav' -and $android -cnotcontains 'audio/pixabay/waterfall_loop.wav') 'Android must use admitted Core loop only.'
    ++$script:checks
    foreach($platform in @('Windows','Android')) {
        $archive=New-FixtureArchive "$platform-positive.zip" $platform
        Assert-Horde162Package $repo $archive $platform
        $stage=Join-Path $scratch "$platform-stage/assets"
        Copy-Horde162RuntimeAssets $repo $stage $platform
        Assert-Horde162StagedAssets $repo $stage $platform
        ++$script:checks
    }
    # Every menu file is required and hash-pinned on each platform. Neither
    # original collections nor an unreviewed fifth clip may enter a package.
    foreach($platform in @('Windows','Android')) {
        foreach($name in @('asset.manifest.json','menu_room.wav','menu_chain.wav')) {
            $entry="assets/audio/menu/$name"
            $archive=New-FixtureArchive "$platform-menu-missing-$name.zip" $platform -Omit $entry
            Expect-Failure {Assert-Horde162Package $repo $archive $platform} 'closed runtime roster'
            $archive=New-FixtureArchive "$platform-menu-corrupt-$name.zip" $platform -Corrupt $entry
            Expect-Failure {Assert-Horde162Package $repo $archive $platform} 'byte/hash mismatch'
            $archive=New-FixtureArchive "$platform-menu-duplicate-$name.zip" $platform -Duplicate $entry
            Expect-Failure {Assert-Horde162Package $repo $archive $platform} 'duplicate runtime entry'
        }
        $archive=New-FixtureArchive "$platform-menu-foreign.zip" $platform -Extra 'assets/audio/menu/original-chain.mp3'
        Expect-Failure {Assert-Horde162Package $repo $archive $platform} 'closed runtime roster'
        $archive=New-FixtureArchive "$platform-menu-rejected-flame.zip" $platform -Extra 'assets/audio/menu/menu_flame.wav'
        Expect-Failure {Assert-Horde162Package $repo $archive $platform} 'closed runtime roster'
    }
    $directories=@('assets/audio/pixabay/','assets/textures/environment/','assets/textures/environment/runtime/')
    $archive=New-FixtureArchive 'explicit-directory-entries.zip' Windows -Directories $directories
    Assert-Horde162Package $repo $archive Windows
    ++$script:checks
    $archive=Join-Path $scratch 'actual-compress-archive.zip'
    Compress-Archive -Path (Join-Path $scratch 'Windows-stage/*') -DestinationPath $archive
    Assert-Horde162Package $repo $archive Windows
    ++$script:checks
    $archive=New-FixtureArchive 'directory-plus-foreign-file.zip' Windows -Directories $directories -Extra 'assets/textures/environment/source/foreign.png'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'closed runtime roster'
    $archive=New-FixtureArchive 'nonempty-directory.zip' Windows -Extra 'assets/textures/environment/runtime/'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'malformed nonempty directory'
    $kitArchive=New-FixtureArchive 'foreign-kit-master.zip' Windows -EmptyFile 'assets/audio/kit/master.wav'
    Expect-Failure {Assert-Horde162Package $repo $kitArchive Windows} 'closed runtime roster'
    $kitArchive=New-FixtureArchive 'duplicate-original-kit-model.zip' Windows -EmptyFile 'assets/models/characters/runtime/kit-warden-v1/kit-warden-lod0.runtime.glb'
    Expect-Failure {Assert-Horde162Package $repo $kitArchive Windows} 'closed runtime roster'
    $kitArchive=New-FixtureArchive 'kit-promotion-receipt.zip' Android -EmptyFile 'assets/models/characters/runtime/kit-warden-v1/turn-runtime-promotion.json'
    Expect-Failure {Assert-Horde162Package $repo $kitArchive Android} 'closed runtime roster'
    $archive=New-FixtureArchive 'zero-byte-foreign-file.zip' Windows -EmptyFile 'assets/audio/pixabay/foreign.wav'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'closed runtime roster'
    $archive=New-FixtureArchive 'foreign-empty-directory.zip' Windows -Directories @('assets/textures/environment/source/')
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'closed runtime roster'
    foreach($case in @(
        @{Name='missing-cue';Platform='Windows';Omit='assets/audio/pixabay/keeper_i_sense_you.wav'},
        @{Name='missing-kit-grate';Platform='Windows';Omit='assets/audio/kit/runtime/prologue.kit_grate.wav'},
        @{Name='missing-kit-model';Platform='Windows';Omit='assets/models/characters/runtime/kit-warden-v1/kit-warden-turn.runtime.glb'},
        @{Name='missing-kit-normal';Platform='Android';Omit='assets/textures/characters/runtime/kit-warden-v1/normal.android.ktx2'},
        @{Name='kit-private-source';Platform='Windows';Extra='assets/models/characters/source/private.blend'},
        @{Name='kit-private-texture';Platform='Android';Extra='assets/textures/characters/source/master.png'},
        @{Name='missing-kit-manifest';Platform='Android';Omit='assets/audio/kit/runtime/asset.manifest.json'},
        @{Name='missing-death-cue';Platform='Windows';Omit='assets/audio/pixabay/keeper_death.wav'},
        @{Name='missing-death-core';Platform='Android';Omit='assets/audio/pixabay/keeper_death_core.wav'},
        @{Name='missing-death-manifest';Platform='Android';Omit='assets/audio/pixabay/keeper-death.manifest.json'},
        @{Name='missing-core-manifest';Platform='Android';Omit='assets/audio/pixabay/waterfall-core.manifest.json'},
        @{Name='missing-collapse-glb';Platform='Android';Omit='assets/models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb'},
        @{Name='missing-collapse-manifest';Platform='Windows';Omit='assets/models/world/runtime/collapsed-entry/asset.manifest.json'},
        @{Name='missing-rag-torch-glb';Platform='Android';Omit='assets/models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb'},
        @{Name='missing-rag-torch-manifest';Platform='Windows';Omit='assets/models/props/runtime/player-rag-torch/asset.manifest.json'},
        @{Name='missing-props-array';Platform='Android';Omit='assets/textures/props/runtime/normal.android.ktx2'},
        @{Name='missing-props-manifest';Platform='Windows';Omit='assets/textures/props/runtime/asset.manifest.json'},
        @{Name='missing-sword-draw';Platform='Android';Omit='assets/audio/filmcow/equipment/sword_draw.wav'},
        @{Name='missing-sword-sheath';Platform='Windows';Omit='assets/audio/filmcow/equipment/sword_sheath.wav'},
        @{Name='missing-equipment-manifest';Platform='Windows';Omit='assets/audio/filmcow/equipment/asset.manifest.json'},
        @{Name='missing-scabbard-glb';Platform='Android';Omit='assets/models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb'},
        @{Name='missing-scabbard-manifest';Platform='Windows';Omit='assets/models/props/runtime/player-sword-scabbard/asset.manifest.json'},
        @{Name='missing-scabbard-receipt';Platform='Android';Omit='assets/models/props/runtime/player-sword-scabbard/processing-receipt.json'},
        @{Name='foreign-source';Platform='Windows';Extra='assets/textures/environment/source/foreign-original.png'},
        @{Name='foreign-cue';Platform='Android';Extra='assets/audio/pixabay/unreviewed.wav'},
        @{Name='windows-extra-core';Platform='Windows';Extra='assets/audio/pixabay/waterfall_core_loop.wav'},
        @{Name='android-extra-full-loop';Platform='Android';Extra='assets/audio/pixabay/waterfall_loop.wav'},
        @{Name='other-platform-env';Platform='Windows';Extra='assets/textures/environment/runtime/night-storm.android.ktx2'},
        @{Name='collapse-source';Platform='Android';Extra='assets/models/world/source/private.blend'},
        @{Name='collapse-foreign-runtime';Platform='Windows';Extra='assets/models/world/runtime/collapsed-entry/unapproved.glb'},
        @{Name='rag-torch-processing-receipt';Platform='Windows';Extra='assets/models/props/source/rag-torch-v01/processing-receipt.json'},
        @{Name='rag-torch-runtime-processing-receipt';Platform='Android';Extra='assets/models/props/runtime/player-rag-torch/processing-receipt.json'},
        @{Name='rag-torch-authoring-png';Platform='Android';Extra='assets/textures/props/source/rag-torch-v01/base-color.png'},
        @{Name='rag-torch-source-zip';Platform='Windows';Extra='assets/models/props/source/rag_torch_v01_blender_package.zip'},
        @{Name='props-source';Platform='Windows';Extra='assets/textures/props/source/private.png'},
        @{Name='scabbard-source';Platform='Android';Extra='assets/models/props/source/player-sword-scabbard/source.blend'},
        @{Name='scabbard-foreign-runtime';Platform='Windows';Extra='assets/models/props/runtime/player-sword-scabbard/unapproved.glb'},
        @{Name='equipment-raw-clip';Platform='Android';Extra='assets/audio/filmcow/equipment/dagger unsheath 1.wav'},
        @{Name='equipment-extra-clip';Platform='Windows';Extra='assets/audio/filmcow/equipment/unapproved.wav'},
        @{Name='props-extra-array';Platform='Android';Extra='assets/textures/props/runtime/unapproved.android.ktx2'},
        @{Name='props-wrong-platform';Platform='Windows';Extra='assets/textures/props/runtime/normal.android.ktx2'}
    )) {
        $archiveArguments=@{Name=$case.Name+'.zip';Platform=$case.Platform}
        if($case.ContainsKey('Omit')){$archiveArguments.Omit=$case.Omit}
        if($case.ContainsKey('Extra')){$archiveArguments.Extra=$case.Extra}
        $archive=New-FixtureArchive @archiveArguments
        Expect-Failure {Assert-Horde162Package $repo $archive $case.Platform} 'closed runtime roster'
    }
    foreach ($extra in @('assets\models\world\runtime\collapsed-entry\foreign.glb',
                         'assets/other/../../models/world/runtime/collapsed-entry/foreign.glb',
                         '/assets/models/world/runtime/collapsed-entry/foreign.glb')) {
        $archive=New-FixtureArchive ('invalid-path-'+$script:checks+'.zip') Android -Extra $extra
        Expect-Failure {Assert-Horde162Package $repo $archive Android} 'noncanonical or traversing entry path'
    }
    $archive=New-FixtureArchive 'duplicate.zip' Windows -Duplicate 'assets/audio/pixabay/keeper_i_sense_you.wav'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'duplicate runtime entry'
    $archive=New-FixtureArchive 'duplicate-props-array.zip' Android -Duplicate 'assets/textures/props/runtime/normal.android.ktx2'
    Expect-Failure {Assert-Horde162Package $repo $archive Android} 'duplicate runtime entry'
    foreach($case in @(
        @{Name='corrupt-cue';Platform='Windows';Path='assets/audio/pixabay/keeper_come_closer.wav'},
        @{Name='corrupt-env';Platform='Android';Path='assets/textures/environment/runtime/night-storm.android.ktx2'},
        @{Name='corrupt-manifest';Platform='Windows';Path='assets/audio/pixabay/keeper-asset.manifest.json'},
        @{Name='corrupt-collapse-glb';Platform='Windows';Path='assets/models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb'},
        @{Name='corrupt-collapse-manifest';Platform='Android';Path='assets/models/world/runtime/collapsed-entry/asset.manifest.json'},
        @{Name='corrupt-rag-torch-glb';Platform='Windows';Path='assets/models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb'},
        @{Name='corrupt-rag-torch-manifest';Platform='Android';Path='assets/models/props/runtime/player-rag-torch/asset.manifest.json'},
        @{Name='corrupt-props-array';Platform='Android';Path='assets/textures/props/runtime/base-color.android.ktx2'},
        @{Name='corrupt-props-manifest';Platform='Windows';Path='assets/textures/props/runtime/asset.manifest.json'},
        @{Name='corrupt-sword-draw';Platform='Windows';Path='assets/audio/filmcow/equipment/sword_draw.wav'},
        @{Name='corrupt-sword-sheath';Platform='Android';Path='assets/audio/filmcow/equipment/sword_sheath.wav'},
        @{Name='corrupt-equipment-manifest';Platform='Android';Path='assets/audio/filmcow/equipment/asset.manifest.json'},
        @{Name='corrupt-kit-rope';Platform='Android';Path='assets/audio/kit/runtime/rescue.rope.wav'},
        @{Name='corrupt-kit-visual';Platform='Android';Path='assets/models/characters/runtime/kit-warden-v1/kit-warden-turn.runtime.glb'},
        @{Name='corrupt-kit-map';Platform='Windows';Path='assets/textures/characters/runtime/kit-warden-v1/base-color.windows.ktx2'},
        @{Name='corrupt-scabbard-glb';Platform='Windows';Path='assets/models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb'},
        @{Name='corrupt-scabbard-receipt';Platform='Android';Path='assets/models/props/runtime/player-sword-scabbard/processing-receipt.json'},
        @{Name='corrupt-scabbard-manifest';Platform='Android';Path='assets/models/props/runtime/player-sword-scabbard/asset.manifest.json'}
    )) {
        $archive=New-FixtureArchive ($case.Name+'.zip') $case.Platform -Corrupt $case.Path
        Expect-Failure {Assert-Horde162Package $repo $archive $case.Platform} 'byte/hash mismatch'
    }
    $fixtureRepo=Join-Path $scratch 'manifest-fixture'
    foreach($relative in (@(Get-Horde162AssetSpecification | ForEach-Object Path)+@(Get-Horde162ManifestSpecification | ForEach-Object Path)+@(Get-Horde17TombAssetSpecification | ForEach-Object Path)+@(Get-Horde17KitAssetSpecification | ForEach-Object Path)+@(Get-Horde17KitVisualAssetSpecification | ForEach-Object Path))) {
        $source=Join-Horde162Path $repo "assets/$relative";$target=Join-Horde162Path $fixtureRepo "assets/$relative"
        $null=New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
        Copy-Item -LiteralPath $source -Destination $target
    }
    $manifestPath=Join-Horde162Path $fixtureRepo 'assets/audio/pixabay/keeper-asset.manifest.json'
    $manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.assets[0].channels=2
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath
    Expect-Failure {Assert-Horde162Assets $fixtureRepo} 'keeper manifest runtime mismatch'
    Copy-Item -LiteralPath (Join-Horde162Path $repo 'assets/audio/pixabay/keeper-asset.manifest.json') -Destination $manifestPath
    foreach ($name in @('asset.manifest.json','collapsed-entry-lod0.runtime.glb')) {
        $relative="assets/models/world/runtime/collapsed-entry/$name"; $path=Join-Horde162Path $fixtureRepo $relative
        $bytes=[IO.File]::ReadAllBytes($path); $bytes[$bytes.Length-1]=$bytes[$bytes.Length-1] -bxor 1
        [IO.File]::WriteAllBytes($path,$bytes)
        Expect-Failure {Assert-Horde162Assets $fixtureRepo} 'runtime byte/hash mismatch'
        Copy-Item -LiteralPath (Join-Horde162Path $repo $relative) -Destination $path
    }
    foreach ($relative in @('assets/textures/props/runtime/asset.manifest.json','assets/textures/props/runtime/emissive.android.ktx2')) {
        $path=Join-Horde162Path $fixtureRepo $relative;$bytes=[IO.File]::ReadAllBytes($path);$bytes[$bytes.Length-1]=$bytes[$bytes.Length-1] -bxor 1
        [IO.File]::WriteAllBytes($path,$bytes)
        Expect-Failure {Assert-Horde162Assets $fixtureRepo} 'runtime byte/hash mismatch'
        Copy-Item -LiteralPath (Join-Horde162Path $repo $relative) -Destination $path
    }
    $propsForeign=Join-Horde162Path (Join-Path $scratch 'Windows-stage/assets') 'textures/props/source/private.png'
    $null=New-Item -ItemType Directory -Path (Split-Path -Parent $propsForeign) -Force
    [IO.File]::WriteAllText($propsForeign,'synthetic authoring texture')
    Expect-Failure {Assert-Horde162StagedAssets $repo (Join-Path $scratch 'Windows-stage/assets') Windows} 'closed runtime roster'
    Remove-Item -LiteralPath $propsForeign
    $worldForeign=Join-Horde162Path (Join-Path $scratch 'Android-stage/assets') 'models/world/source/private.blend'
    $null=New-Item -ItemType Directory -Path (Split-Path -Parent $worldForeign) -Force
    [IO.File]::WriteAllText($worldForeign,'synthetic authoring content')
    Expect-Failure {Assert-Horde162StagedAssets $repo (Join-Path $scratch 'Android-stage/assets') Android} 'closed runtime roster'
    $stage=Join-Path $scratch 'Windows-stage/assets'
    $foreign=Join-Horde162Path $stage 'textures/environment/source/unreviewed.png'
    $null=New-Item -ItemType Directory -Path (Split-Path -Parent $foreign) -Force
    [IO.File]::WriteAllText($foreign,'synthetic source')
    Expect-Failure {Assert-Horde162StagedAssets $repo $stage Windows} 'closed runtime roster'
    Write-Output "PASS: $script:checks 1.6.2 admission/staging/archive cases; no build, release packaging, signing or device action."
} finally {
    $resolved=[IO.Path]::GetFullPath($scratch)
    $prefix=$temp.TrimEnd([IO.Path]::DirectorySeparatorChar)+[IO.Path]::DirectorySeparatorChar
    if(-not $resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture cleanup target escaped its verified temp root.'}
    if(Test-Path -LiteralPath $resolved){Remove-Item -LiteralPath $resolved -Recurse -Force}
}
