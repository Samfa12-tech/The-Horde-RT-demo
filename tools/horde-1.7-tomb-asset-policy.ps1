# Exact 1.7 tomb additions; historical 1.6.2 artifact pins remain unchanged.
# This admits file identities, not owner listening or physical-device acceptance.
function Get-Horde17TombAssetSpecification {
    @(
        [pscustomobject]@{ Path='audio/pixabay/water_wet_step_1.wav'; Bytes=21164; Sha256='663ebe1e5fb38945e0e0e25037efe95d86964c03a4d6d1c642464593ec2d3263'; Platform='Windows'; Kind='Wave'; Channels=1; Frames=10560 },
        [pscustomobject]@{ Path='audio/pixabay/water_wet_step_1_core.wav'; Bytes=42284; Sha256='bae5e1c700b7b3fc841662ca92869d6b074680fe513a8d04a4ef46e2deb7a944'; Platform='Android'; Kind='Wave'; Channels=2; Frames=10560 },
        [pscustomobject]@{ Path='audio/pixabay/water_wet_step_2.wav'; Bytes=23084; Sha256='cd62db9512d7ff60ad9f0160c1eb3d98d6cf82130a30da6971bc0d053fe358b2'; Platform='Windows'; Kind='Wave'; Channels=1; Frames=11520 },
        [pscustomobject]@{ Path='audio/pixabay/water_wet_step_2_core.wav'; Bytes=46124; Sha256='785201e135581c30cb243b77428bd32d07b113e2d1f3416eab8d85018ee89159'; Platform='Android'; Kind='Wave'; Channels=2; Frames=11520 },
        [pscustomobject]@{ Path='audio/pixabay/water_stream_contact.wav'; Bytes=33644; Sha256='2cee1adcb1c944d19af0d2a5c8b81999908f6482bf2fcdd486d0457627ed8cfa'; Platform='Windows'; Kind='Wave'; Channels=1; Frames=16800 },
        [pscustomobject]@{ Path='audio/pixabay/water_stream_contact_core.wav'; Bytes=67244; Sha256='2e242637d10db7b978efaa755eee93d037477985e2106ce462063e7af18c3461'; Platform='Android'; Kind='Wave'; Channels=2; Frames=16800 },
        [pscustomobject]@{ Path='audio/pixabay/water-contact.manifest.json'; Bytes=2562; Sha256='c01bf5a6dfb32c640f82e7989c51d834a1918960de99c18660fafbe6702bf20b'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/tomb-dressing-v01/tomb-niche-rect/asset.manifest.json'; Bytes=721; Sha256='cad866e741683d054a944afe7bb082e8b435cad6d0e27c85252ee8e1ee082abc'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/tomb-dressing-v01/tomb-niche-rect/candidate-receipt.json'; Bytes=940; Sha256='025ab5c64df9878d2d54bafcb1ade958aea77db429647987ce0883ce58839659'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/tomb-dressing-v01/tomb-niche-rect/tomb-niche-rect-lod0.runtime.glb'; Bytes=86016; Sha256='aa4b6cb8b072448534c4555a941851946bd6e9295af56b4b3ad035dd714e3ad9'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/tomb-dressing-v01/tomb-niche-arched/asset.manifest.json'; Bytes=723; Sha256='3f67e0906b49664cb17de8657c4be71a87b3bbd806358ac615df956e88e63c04'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/tomb-dressing-v01/tomb-niche-arched/candidate-receipt.json'; Bytes=943; Sha256='32785f6edd88f07a0e1adcab35aa23fa8f2ba75f7b708b5348949980e068c4d5'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/tomb-dressing-v01/tomb-niche-arched/tomb-niche-arched-lod0.runtime.glb'; Bytes=99316; Sha256='18810291df9ee3a630ca69dd813a29362b707f664982557e7dd7d37b8c8b5794'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/femur/tomb-femur-lod0.runtime.glb'; Bytes=10580; Sha256='bdf24bb2b71290b3acf843ea48c2eb280e7d75300463ae0096e0da4796c4c608'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/humerus/tomb-humerus-lod0.runtime.glb'; Bytes=8564; Sha256='f39dc48c4b0faf1668e748f8927f219dbd3e4d1c90e6e90c71ec9f8a97c192af'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/skull-jaw/tomb-skull-jaw-lod0.runtime.glb'; Bytes=127212; Sha256='07d00188f183de5fd1ea1d24051e7e3c993953f02aaf85fcc9349d4b38105c4f'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/derivation-receipt.json'; Bytes=3465; Sha256='390b57a453a0cb883bdd01938b0edb0dcae35e117a74eb324e4387b64022d1b8'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_1.glb'; Bytes=34992; Sha256='5e454e5aa302e3d081c9857e00a9c8d005c5454a5a7e04d6f212fdbf56c20823'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_2.glb'; Bytes=38456; Sha256='01d24d1c097267b87bafe4a7545d0cdde50292f2fe7af12a70022c590a82d125'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_3.glb'; Bytes=30440; Sha256='1a7967a9ae3a82a1e470067b9a344804aa5f40b36b7ba13b401538c6b8fa30a3'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_displaced_lid.glb'; Bytes=67240; Sha256='e93203b35e8e1979ba16a67f31044076b20127fa8bfbc60b96512d2a5e777962'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_offering_bowl.glb'; Bytes=45880; Sha256='6e193a9b36656bffa8c972dab0958884208c0a4f38b72333a4677ad2ceab302b'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_urn_broken_base.glb'; Bytes=87384; Sha256='c0ed7c0261a125385f821109927567c4fc8f1ff0799d0c3c479d4149e09ad594'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_urn_rim_shard.glb'; Bytes=56588; Sha256='63a76503741ae51f9230e7b23201037bb1e4402d348bbd27323000a82605f1ad'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/derivation-receipt.json'; Bytes=5134; Sha256='6c138994f57a3b3e10bf3113d591d6a47f47493b63618acc20932e9d79ee2f2f'; Platform='Both'; Kind='Json' }
    )
}
