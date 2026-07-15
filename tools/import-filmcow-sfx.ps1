param(
    [string]$SourceRoot = "C:\Users\sam_s\Documents\Possum Cafe\PossumCafeAndroid\Archive\FilmCow Recorded SFX",
    [string]$DestinationRoot = (Join-Path $PSScriptRoot "..\assets\audio\filmcow")
)

$ErrorActionPreference = "Stop"

$clips = [ordered]@{
    "ui_select.wav"     = "clicky button 1.wav"
    "ui_back.wav"       = "button pressed 1.wav"
    "menu_toggle.wav"   = "metal button 1.wav"
    "sword_swing_1.wav" = "woosh 1.wav"
    "sword_swing_2.wav" = "woosh 3.wav"
    "sword_hit_1.wav"   = "fencing hit 1.wav"
    "sword_hit_2.wav"   = "fencing hit 2.wav"
    "enemy_fall.wav"    = "body fall 1.wav"
    "player_step_1.wav" = "footstep dirt 3.wav"
    "player_step_2.wav" = "footstep dirt 4.wav"
    "skeleton_step_1.wav" = "footstep dirt 17.wav"
    "skeleton_step_2.wav" = "footstep dirt 18.wav"
    "skeleton_attack.wav" = "harpoon rattle.wav"
}

function Convert-Pcm24ToPcm16 {
    param([string]$InputPath, [string]$OutputPath)

    $reader = [System.IO.BinaryReader]::new([System.IO.File]::OpenRead($InputPath))
    try {
        if ([Text.Encoding]::ASCII.GetString($reader.ReadBytes(4)) -ne "RIFF") { throw "Not a RIFF WAV: $InputPath" }
        [void]$reader.ReadUInt32()
        if ([Text.Encoding]::ASCII.GetString($reader.ReadBytes(4)) -ne "WAVE") { throw "Not a WAVE file: $InputPath" }

        $format = $null
        $data = $null
        while ($reader.BaseStream.Position -lt $reader.BaseStream.Length) {
            $chunkId = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $chunkSize = $reader.ReadUInt32()
            $chunk = $reader.ReadBytes([int]$chunkSize)
            if (($chunkSize % 2) -ne 0 -and $reader.BaseStream.Position -lt $reader.BaseStream.Length) { [void]$reader.ReadByte() }
            if ($chunkId -eq "fmt ") { $format = $chunk }
            if ($chunkId -eq "data") { $data = $chunk }
        }

        if ($null -eq $format -or $null -eq $data) { throw "Missing fmt or data chunk: $InputPath" }
        $fmtReader = [System.IO.BinaryReader]::new([System.IO.MemoryStream]::new($format))
        try {
            $audioFormat = $fmtReader.ReadUInt16()
            $channels = $fmtReader.ReadUInt16()
            $sampleRate = $fmtReader.ReadUInt32()
            [void]$fmtReader.ReadUInt32()
            [void]$fmtReader.ReadUInt16()
            $bitsPerSample = $fmtReader.ReadUInt16()
        } finally {
            $fmtReader.Dispose()
        }
        if ($audioFormat -ne 1 -or $bitsPerSample -ne 24) { throw "Expected 24-bit integer PCM: $InputPath" }
        if (($data.Length % 3) -ne 0) { throw "Invalid 24-bit sample data length: $InputPath" }

        $sampleCount = [int]($data.Length / 3)
        $pcm16 = [byte[]]::new($sampleCount * 2)
        for ($i = 0; $i -lt $sampleCount; $i++) {
            $offset24 = $i * 3
            # Cast every byte before shifting. PowerShell otherwise preserves
            # the byte-sized operand and truncates both shifted high bytes,
            # which silently converted the original import to zero samples.
            $value = [int]$data[$offset24] -bor ([int]$data[$offset24 + 1] -shl 8) -bor ([int]$data[$offset24 + 2] -shl 16)
            if (($value -band 0x800000) -ne 0) { $value -= 0x1000000 }
            $sample = [int16]($value -shr 8)
            $bytes = [BitConverter]::GetBytes($sample)
            $pcm16[$i * 2] = $bytes[0]
            $pcm16[$i * 2 + 1] = $bytes[1]
        }

        $writer = [System.IO.BinaryWriter]::new([System.IO.File]::Create($OutputPath))
        try {
            $blockAlign = [uint16]($channels * 2)
            $byteRate = [uint32]($sampleRate * $blockAlign)
            $writer.Write([Text.Encoding]::ASCII.GetBytes("RIFF"))
            $writer.Write([uint32](36 + $pcm16.Length))
            $writer.Write([Text.Encoding]::ASCII.GetBytes("WAVEfmt "))
            $writer.Write([uint32]16)
            $writer.Write([uint16]1)
            $writer.Write([uint16]$channels)
            $writer.Write([uint32]$sampleRate)
            $writer.Write($byteRate)
            $writer.Write($blockAlign)
            $writer.Write([uint16]16)
            $writer.Write([Text.Encoding]::ASCII.GetBytes("data"))
            $writer.Write([uint32]$pcm16.Length)
            $writer.Write($pcm16)
        } finally {
            $writer.Dispose()
        }
    } finally {
        $reader.Dispose()
    }
}

New-Item -ItemType Directory -Force -Path $DestinationRoot | Out-Null
foreach ($entry in $clips.GetEnumerator()) {
    $source = Join-Path $SourceRoot $entry.Value
    $destination = Join-Path $DestinationRoot $entry.Key
    if (-not (Test-Path -LiteralPath $source)) { throw "Missing FilmCow source clip: $source" }
    Convert-Pcm24ToPcm16 -InputPath $source -OutputPath $destination
    Write-Host "$($entry.Value) -> $($entry.Key)"
}

Write-Host "Imported $($clips.Count) FilmCow clips as mono 48 kHz 16-bit PCM WAV files."
