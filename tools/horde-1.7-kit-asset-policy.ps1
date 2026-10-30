# Exact approved 1.7 Kit speech; no changes to historical 1.6.2 pins.
function Get-Horde17KitAssetSpecification {
    @(
        [pscustomobject]@{ Path='audio/kit/runtime/prologue.kit_grate.wav'; Bytes=310558; Sha256='2af6b1b650afb8395ced3ba2ec95beea7875a214fed26fcb5f967fcf9c1038a8'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=155257 },
        [pscustomobject]@{ Path='audio/kit/runtime/rescue.found.wav'; Bytes=116706; Sha256='4e01035e42315c13012b358ba3db3438ea6b88a030c9fafaa56fc6aa78e55f9c'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=58331 },
        [pscustomobject]@{ Path='audio/kit/runtime/rescue.rope.wav'; Bytes=125944; Sha256='7713bf78793f580d2a418126fd521b01ebcb309bedc3cd8b10c13b91f112a0b6'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=62950 },
        [pscustomobject]@{ Path='audio/kit/runtime/reunion.question.wav'; Bytes=149212; Sha256='061f23a012cd37d8cc8158ba19df85f9fa16edff479c53aaec1fd677b400a927'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=74584 },
        [pscustomobject]@{ Path='audio/kit/runtime/reunion.hint.wav'; Bytes=102750; Sha256='c5c9a012a562e900697bfdca5cd4b7dda7cb579b08d25db10bfaca5ea6fd7191'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=51353 },
        [pscustomobject]@{ Path='audio/kit/runtime/reunion.proof.wav'; Bytes=124898; Sha256='f9c2db3ce4fc592d994e73ea1f82761c807b4e178012d413652cbcb70c0394b2'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=62427 },
        [pscustomobject]@{ Path='audio/kit/runtime/reunion.first_piece.wav'; Bytes=160058; Sha256='8456da25e25588900c87ddfc3bba962eba6b232410e54cf284fe27b4515366d9'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=80007 },
        [pscustomobject]@{ Path='audio/kit/runtime/reunion.depart.wav'; Bytes=211648; Sha256='d94158155041ab556a73d15241480b41b4305aa3c2933e8c5eced47fc980f8b4'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=105802 },
        [pscustomobject]@{ Path='audio/kit/runtime/forest.night.wav'; Bytes=117006; Sha256='b063e4daa2497bb3f94d1eea388028309c835086433c8c8cfa33edd8596d84bc'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=58481 },
        [pscustomobject]@{ Path='audio/kit/runtime/forest.waystone.wav'; Bytes=142994; Sha256='c00c2d6956ef6fd6c93c71092d01f2c03b125644c211c7a700a8909a40ad795a'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=71475 },
        [pscustomobject]@{ Path='audio/kit/runtime/forest.clue.wav'; Bytes=202438; Sha256='cc783ba0141f6b47f67d77e197e2b60a38c69c50b79ac5b2db0d00657c47970b'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=101197 },
        [pscustomobject]@{ Path='audio/kit/runtime/forest.wait.wav'; Bytes=76326; Sha256='8fb4e10dc2a596eba85a4c9ec07e111e42a336f018e824f6e942839883bdf950'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=38141 },
        [pscustomobject]@{ Path='audio/kit/runtime/forest.village.wav'; Bytes=104386; Sha256='33b6b9d34dea2f6eda32ac44ceeddacfd51e3ab98dbdddafde3b65cee57d047d'; Platform='Both'; Kind='KitWave'; Channels=1; Frames=52171 },
        [pscustomobject]@{ Path='audio/kit/runtime/asset.manifest.json'; Bytes=8006; Sha256='eca3594ea3cea9821f511a256a37ca61d53ca6623bff3154affb85583b988f69'; Platform='Both'; Kind='Json' }
    )
}

function Assert-Horde17KitWave {
    param([string]$Path, [long]$Frames)
    $bytes=[IO.File]::ReadAllBytes($Path)
    if ($Frames -lt 1 -or $Frames -gt 156000 -or $bytes.Length -ne 44+2*$Frames -or
        [Text.Encoding]::ASCII.GetString($bytes,0,4) -cne 'RIFF' -or
        [BitConverter]::ToUInt32($bytes,4) -ne $bytes.Length-8 -or
        [Text.Encoding]::ASCII.GetString($bytes,8,8) -cne 'WAVEfmt ' -or
        [BitConverter]::ToUInt32($bytes,16) -ne 16 -or
        [BitConverter]::ToUInt16($bytes,20) -ne 1 -or
        [BitConverter]::ToUInt16($bytes,22) -ne 1 -or
        [BitConverter]::ToUInt32($bytes,24) -ne 24000 -or
        [BitConverter]::ToUInt32($bytes,28) -ne 48000 -or
        [BitConverter]::ToUInt16($bytes,32) -ne 2 -or
        [BitConverter]::ToUInt16($bytes,34) -ne 16 -or
        [Text.Encoding]::ASCII.GetString($bytes,36,4) -cne 'data' -or
        [BitConverter]::ToUInt32($bytes,40) -ne 2*$Frames) {
        throw "Kit WAV requires the admitted clean mono PCM16 24 kHz cut: $Path"
    }
}
