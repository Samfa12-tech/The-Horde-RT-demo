# Exact runtime derivative and platform maps; masters and processing receipts stay outside packages.
function Get-Horde17KitVisualAssetSpecification {
    @(
        [pscustomobject]@{ Path='models/characters/runtime/kit-warden-v1/asset.manifest.json'; Bytes=754; Sha256='0218c5f3f122c1be51af356e4b63689c38da9efebb4d03fb06d716a3055b5c57'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/characters/runtime/kit-warden-v1/kit-warden-lod0.runtime.glb'; Bytes=2151672; Sha256='1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/base-color.windows.ktx2'; Bytes=5592908; Sha256='4597dd3294dfab20870b8aa7be6ecad8795e18ebe5715020e387ead852334d17'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/normal.windows.ktx2'; Bytes=5592908; Sha256='bba3cc54df1272ac33384a05dc2c7662425452f695788f98dc4b1d4ee0cfc73f'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/orm.windows.ktx2'; Bytes=5592908; Sha256='b7f66cde46ea516af41f140b216b091ee394120adad2fca9571365016c57df80'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/emissive.windows.ktx2'; Bytes=5592908; Sha256='e5d8010e6aad869f53c9ea3edb1bccae61255a1145f6e2d0ab3055272d48475c'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/base-color.android.ktx2'; Bytes=626752; Sha256='d5bfe40003ae53b601f0086f19751f536b3fed59ef31722b8f20458d40cd8abc'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/normal.android.ktx2'; Bytes=1398592; Sha256='8d4f27c08f1e3067285d873a9d07f757e2a45c3027b53a66c503c612f0322831'; Platform='Android'; Kind='PropsKtx'; Format=157; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/orm.android.ktx2'; Bytes=626752; Sha256='3f141a1f319ff5b8c410248e2d05b2853e724fec8e20b7fc0a0732c77de90c29'; Platform='Android'; Kind='PropsKtx'; Format=165; Layers=1 },
        [pscustomobject]@{ Path='textures/characters/runtime/kit-warden-v1/emissive.android.ktx2'; Bytes=626752; Sha256='216824b64c3a354d4dc5a613a4afe6b1c8ad47dccb8c88ecad3f85f6e29e7e83'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=1 }
    )
}
