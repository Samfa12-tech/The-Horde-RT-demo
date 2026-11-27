. (Join-Path $PSScriptRoot 'horde-1.7-kit-visual-asset-policy.ps1')
. (Join-Path $PSScriptRoot 'horde-1.7-kit-asset-policy.ps1')
. (Join-Path $PSScriptRoot 'horde-1.7-tomb-asset-policy.ps1')

# Runtime-only 1.6.2 audio/environment/world admission. This does not certify device,
# listening, signing or release acceptance and works with Debug ZIP/APK files.
function Get-Horde162AssetSpecification {
    @(
        [pscustomobject]@{ Path='audio/menu/menu_room.wav'; Bytes=384044; Sha256='9aa9a9631f7dbaf724ecd46bfbbea17142af20497299a83db08055751c7cc361'; Platform='Both'; Kind='Wave'; Channels=1; Frames=192000 },
        [pscustomobject]@{ Path='audio/menu/menu_chain.wav'; Bytes=96044; Sha256='cb53cefe152ea118fdee6cf6c9beedd3e516a483041a7832981489a7eb04697f'; Platform='Both'; Kind='Wave'; Channels=1; Frames=48000 },
        [pscustomobject]@{ Path='audio/menu/asset.manifest.json'; Bytes=4668; Sha256='4a99707ffccde3acdf1ad1758f4f189e24bd00ecc5c999b2fd7e34d077a62289'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-sword-scabbard/asset.manifest.json'; Bytes=758; Sha256='75e030f75e5dea9baa1732ce7efed67a1dd40e79c7203a2319d3f0e08044319e'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-sword-scabbard/processing-receipt.json'; Bytes=10052; Sha256='3d257b3cd2cfcd4f169ae962ef86e28d8b252351a1fbccd29a06d03d35e56541'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb'; Bytes=15128; Sha256='d23c2b0711f53ce7608e68945ee7c38dd4eb1dc06ad5e33e58acc26c5bc0eaf5'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='audio/filmcow/equipment/sword_draw.wav'; Bytes=41972; Sha256='a70dcbcf7d627f8ebec641a33c144f64682771c715f0bfc819fd2f8d981713ce'; Platform='Both'; Kind='Wave'; Channels=1; Frames=20964 },
        [pscustomobject]@{ Path='audio/filmcow/equipment/sword_sheath.wav'; Bytes=67558; Sha256='0f38fe0560c91aa1b97de84a01787d70d490832f727684ca402e9dfcf4ff2aaf'; Platform='Both'; Kind='Wave'; Channels=1; Frames=33757 },
        [pscustomobject]@{ Path='audio/filmcow/equipment/asset.manifest.json'; Bytes=1503; Sha256='b84eeef9402cebc0140996e25a819d8b47a1032b1eb5691fc95a926d09daa28d'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='audio/pixabay/chest_unlock.wav'; Bytes=36908; Sha256='a1dae01adb6534a2e1504f0d5bc41abff0305e619264dff0339577d4d5b2d808'; Platform='Both'; Kind='Wave'; Channels=1; Frames=18432 },
        [pscustomobject]@{ Path='audio/pixabay/chest_open.wav'; Bytes=479276; Sha256='788b1c31c504c8eea9e5316c87859013aba6a49f205e0e4a66b181aafbdfb4a6'; Platform='Both'; Kind='Wave'; Channels=1; Frames=239616 },
        [pscustomobject]@{ Path='audio/pixabay/torch_extinguish.wav'; Bytes=100268; Sha256='ef2e85d4219cce2e5789d6326755909f59e302bd26bc2dbd8f3e39c270614ab8'; Platform='Both'; Kind='Wave'; Channels=1; Frames=50112 },
        [pscustomobject]@{ Path='audio/pixabay/keeper_i_sense_you.wav'; Bytes=248684; Sha256='b476d7c672cbfce30a38c02f31978d0024f16c658ce2fd8cb716cbf91a994418'; Platform='Both'; Kind='Wave'; Channels=1; Frames=124320 },
        [pscustomobject]@{ Path='audio/pixabay/keeper_come_closer.wav'; Bytes=275418; Sha256='7a79e79aa4ebee4920d11f28f9ca140ac1e22b3c5cec2352ee159e0ba117a719'; Platform='Both'; Kind='Wave'; Channels=1; Frames=137687 },
        [pscustomobject]@{ Path='audio/pixabay/skeleton_idle_rattle.wav'; Bytes=120044; Sha256='8ac138628c744ab3a3df130abb415b11803145063932b4e6956414d010b504ea'; Platform='Both'; Kind='Wave'; Channels=1; Frames=60000 },
        [pscustomobject]@{ Path='audio/pixabay/skeleton_falling_bones.wav'; Bytes=193964; Sha256='66ee82dcab110f14ff7685eb60fa3b0caed32765c6b07f8b654433cd5c97efc9'; Platform='Both'; Kind='Wave'; Channels=1; Frames=96960 },
        [pscustomobject]@{ Path='audio/pixabay/waterfall_loop.wav'; Bytes=2047132; Sha256='de7711f0e6ef9cf0bdd3d04ba7a1b713ce09ef18169bd994182b09ab2017ee62'; Platform='Windows'; Kind='Wave'; Channels=1; Frames=1023527 },
        [pscustomobject]@{ Path='audio/pixabay/waterfall_core_loop.wav'; Bytes=2208044; Sha256='5bb82801f0cff75c56f07993fad128bf1dee2a9876d032c808b8033a135fbad3'; Platform='Android'; Kind='Wave'; Channels=2; Frames=552000 },
        [pscustomobject]@{ Path='textures/environment/runtime/night-storm.windows.ktx2'; Bytes=699532; Sha256='a5e274bfafae55efa4a80840208a39fc7f24c6cf5fe69c744c9c2cd7d60420b0'; Platform='Windows'; Kind='Ktx'; Format=43 },
        [pscustomobject]@{ Path='textures/environment/runtime/night-storm.android.ktx2'; Bytes=80128; Sha256='f833b77867e7b3c01659f5e9ef38834e4a0e5b967cd077023157eb682feb7cec'; Platform='Android'; Kind='Ktx'; Format=166 },
        [pscustomobject]@{ Path='models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb'; Bytes=652980; Sha256='c67471b522c92f354d9a5880dea6e48b4f22b5095207841c35c95b5cfc75c4d4'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/collapsed-entry/asset.manifest.json'; Bytes=742; Sha256='952e1920fbc60646154d0424556c2935a18e7cf882866a9fe9f4f45ef38be623'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb'; Bytes=1402952; Sha256='db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-tree-pair-v1/horde-upright-alder-v1-lod1.glb'; Bytes=1436640; Sha256='f545d7e08370b5e451597c3fd903d8d93b6703d11c75039181a04f7f5312e0bb'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-tree-pair-v1/asset.manifest.json'; Bytes=896; Sha256='b244efaa09eb6851c276b0377d0069396812728a1644f14c4df400dfa0d310ab'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-deadwood-v1/asset.manifest.json'; Bytes=914; Sha256='3c2dbb174242634356c9e6efbf686e0f39d4e28571cff24cdff38c35b495a820'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-deadwood-v1/horde-dead-snag-lod2.glb'; Bytes=3730464; Sha256='71905eaa0bf27baa40ccecd4d4a1d8722d43c9954dff94e3ccc242d639d410dd'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-deadwood-v1/horde-fallen-log.glb'; Bytes=1873468; Sha256='5f4f1fcd6ecdb69715f09cabbe2fac8e716851b87eacad7ec07ec702f715ecef'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-deadwood-v1/horde-gnarled-stump.glb'; Bytes=1425904; Sha256='291381fef23ae05de5382214c778b97cce0969ccc0ff8876879a340ecc6d86d2'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-deadwood-v1/horde-root-tangle.glb'; Bytes=1876496; Sha256='a964f9af66778ddeda2dc3e03d9984fc66f7a36c0a20bf18f8b9b67cc12db5b6'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-rocks-v1/asset.manifest.json'; Bytes=854; Sha256='252add83e55b0e2e0e38c4d395e5422b77047fcc456e0bade1ac6dec33d2a072'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-rocks-v1/horde-rock-low_boulder.glb'; Bytes=704076; Sha256='1bdc6f650410a1f352ad8a314fa0f0b1a978ab226720c97a3e6dde6fd94df262'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-rocks-v1/horde-rock-split_shelf.glb'; Bytes=763376; Sha256='00cc649340942f503cb83a3f5f91c5c5d9509bffe0851f972a4fd2c6625b3aa5'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-rocks-v1/horde-rock-upright_crag.glb'; Bytes=703880; Sha256='2e3e0ca2a3d25c91d12dfa46c3ba6fb639dbda54af2a551c0a46c79b9c186d4a'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-road-clue-v1/asset.manifest.json'; Bytes=960; Sha256='d69c31f7fe62864dcdcb76ed054dc2316fe6e63f2b40c45b9cffc9e54b68b0d5'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-road-clue-v1/horde-road-kerb.glb'; Bytes=2596; Sha256='7bf6a0bafb61d0c32c25ca8f7e730db5d17a72582b1d12097a381be2d29632bc'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-road-clue-v1/horde-road-waystone.glb'; Bytes=6252; Sha256='7229e9920b0d5892a5f480722fa894a16a667c86059d29967d2a031e3f6edb1b'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/asset.manifest.json'; Bytes=709; Sha256='238f141fb0c2e4aacfafd3af0df80118d11b4447067e53ee24e5e15136464481'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/bellwether-tavern-v1.glb'; Bytes=4804676; Sha256='a1f6f1b45096efbef1eb8c0a6406718098471f567d882235e6dbe5a028f77dd0'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/stock-heads.manifest.json'; Bytes=669; Sha256='a1fcc25fbf9c5821c22eb9da2559eb93018d0e5aba6c25b8b1055459c979234f'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/stock-heads.glb'; Bytes=62920; Sha256='529841e0f8e4e065a8dc20c69aa4f06d172a6df8e8cbc2b514935eb6dba9af09'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/privy-rag.manifest.json'; Bytes=661; Sha256='053a136c1c0f0ec97056fdc9c1ba7985a3a5ed35d6af0c9209e54f0505f7974c'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/privy-rag.glb'; Bytes=7592; Sha256='ba755185839b5dad0f4ef0423049f70f92b7bdb932f2c12ea965d482c814b776'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/road-silver.manifest.json'; Bytes=674; Sha256='cd36eb7dbfcb033ac5ab345134fd3f8b97462b4b14b9c02c858775a441d6b3ab'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/road-silver.glb'; Bytes=40152; Sha256='6e7bdce26c3eeddfdb7a1c78278eec32e0b83ef2e6b87a7bad35a124de4c35fa'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/garden-crops.manifest.json'; Bytes=691; Sha256='4312b85a97302f39935d10d3b2cac1f4ceba7412344ddca82293610cfa6bb9b3'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/garden-crops.glb'; Bytes=282228; Sha256='ea55200b061adcab443b7859cd7874f3552c94b908bef808509f371b8287cf8e'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/south-door.manifest.json'; Bytes=704; Sha256='ec9ea1c3e1cf79e03813781e33b6196cd89cc5c8363eeea5a2d85b663022f550'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/bellwether-south-door.glb'; Bytes=29400; Sha256='65e78893ef1a5cf5ebd572ffa1dc67507526948fa3fce7a73796b2eaac239fb9'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/mill-bench-drive.manifest.json'; Bytes=662; Sha256='fa51c54444ee2209d30d461f5a94f0aa9a13cc0fb4bfc2d39ed2a6086ffd05f5'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/mill-bench-drive.glb'; Bytes=40608; Sha256='d336e1c7ed136aebc09e21a32b22c6af51a36fb5779da8608ce3b1f814d48f71'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/mill-waterwheel.manifest.json'; Bytes=671; Sha256='a69e877ec88197ef4d5cd6308f56ce028d023041fcd3b1ad5e9c76ac51d9cc20'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/mill-waterwheel.glb'; Bytes=222196; Sha256='848c723d3b04de91da33aad231b61c8ea6700f9d977cc8e12fdd9fa47c557145'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/window.manifest.json'; Bytes=1008; Sha256='4fc919a3851ba6f890c8c8d518abe5975a46cd0e1fc4384a4346be3edf34540f'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/bellwether-window.glb'; Bytes=2752; Sha256='f9bddea2b07d7fd3480f3cb27735767e4a8b15984f145899a41d21c9067802fc'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/tavern-furniture-lod0.manifest.json'; Bytes=712; Sha256='7c70dcf4dbc40b8b48bbf9970f5f6b3d6950f799eb63db5f4fe818c06d2136c1'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/tavern-furniture-lod0.glb'; Bytes=128644; Sha256='c6f5d8ea4165dc9c8df2219debe6be88c99cc8d9c239f83c36a8dfba6ee418d0'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/tavern-furniture-lod1.manifest.json'; Bytes=709; Sha256='0d22c5e9eec9c725d05ee406cace94a807a11618149370610050e738bb3f8d8a'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/tavern-furniture-lod1.glb'; Bytes=29564; Sha256='46ca11aaec5fd9d50fca1954347a29d5ffa0ebe0d0c108cf91ca177d1599d4b5'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/tavern-furniture-lod2.manifest.json'; Bytes=709; Sha256='f208ac879df8364d23ef1c4f8c93772fd15a15301fc27c0057ee3a556db2f188'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/tavern-furniture-lod2.glb'; Bytes=16100; Sha256='c4f19634e08be2a9959f0a6064062a01b244fe4ab2c076dac0d110b817da65a3'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/wayside-archive-cover.manifest.json'; Bytes=700; Sha256='02e853a61e774afa7bc98cf84625182cb0e51fe10714f6ef3782fe0cfd11d170'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/wayside-archive-cover.glb'; Bytes=3552; Sha256='9fe9834e587b9c3f37093db8c7f8bc3ea8279f9c3edaedbdfa7ce9c003818e91'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/wayside-road-register.manifest.json'; Bytes=702; Sha256='7c1413cf76f6b8cdc4836a8f047093b4225826a73d78647d2dbb4b463bf01725'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-tavern-v1/wayside-road-register.glb'; Bytes=7376; Sha256='e2a7714c5cce4ecac3daf95bfdf577c41b26bcdd4b8e8ce3bf06c852bb5c6ea5'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-loose-road-stone-v1/asset.manifest.json'; Bytes=615; Sha256='5855f77a780dec504377874aad8f11ea5e2fcbb261ec94cb9136836d51818508'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/bellwether-loose-road-stone-v1/loose-road-stone.glb'; Bytes=7136; Sha256='9b127782e788cef0b8c5a911ba74939250b9df37b298e30a567c1452f3a194d3'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/alder-far.glb'; Bytes=1142888; Sha256='c3ad6a0c0d0fddb3cc936d530cae2d7a345e851a7f80edd702147d2d15a1eb23'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/alder-middle.glb'; Bytes=1200292; Sha256='1d79ab1901272a0633f50f9da60eddabd7685d9b957e1a3dcc97175faaf8bba3'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/asset.manifest.json'; Bytes=1037; Sha256='40d688bf4813ec6ae465ed10d24aa00e46758dc89467107eca68c224a072ba61'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/boundary-panel.glb'; Bytes=1104816; Sha256='a8a7cc53cc4d4277ebd8ff6d43deeffc5ba5b039de9ead781d1ac033ef86df82'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/pine-far.glb'; Bytes=1110020; Sha256='e12ba8af5855a8c33a88057611708e74dc20c03bf5dcbb9ef974933ce9c2b4b4'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/pine-middle.glb'; Bytes=1172280; Sha256='74b83325e9a80bdf2fcc8f7a413a08f430ee6e2f8965d0afeb4587f49742008f'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/runtime-roster.json'; Bytes=1667; Sha256='a84554a71e8f51a3eb67eba2186c90ceb2da6425020f68bef6ecbf389264d6d5'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-surroundings-v1/woodland-low-fern.glb'; Bytes=1161988; Sha256='1e5509f5b28a44565a394e4686b21e1b9076462dd6eb2496b99bcefde5cc95b6'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-understory-v1/asset.manifest.json'; Bytes=860; Sha256='cbc759a594b2e80092aa0c3daf87c52fd9c9ac77b08120e1fa908359d67241ee'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/world/runtime/forest-understory-v1/horde-fern-open-clump.glb'; Bytes=206396; Sha256='0c86c7f7d1072ca18bda70080de645d5975bb9bb1634d2a74bd63943543e652b'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-understory-v1/horde-fern-small-clump.glb'; Bytes=166268; Sha256='9e170ceae6443c3a68b3a7a300ca76343e422e1ee304a8f58452da046fe588f9'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/forest-tree-pair-v1/runtime-roster.json'; Bytes=2013; Sha256='6c68e1c9d5a6bcb42a8cabe40e378fd446e0fb262ecb788ab242349815829df3'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb'; Bytes=267672; Sha256='476cb05a8f11f1af917a0cfa29610cc03f23522478a366f6cf2cf49b906db74d'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/props/runtime/player-rag-torch/asset.manifest.json'; Bytes=830; Sha256='5316b2072008da441024cda1153f725f5afcd7d366f3a5b89dece98ec74617aa'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='textures/props/runtime/bellwether-roof-v1/base-color.windows.ktx2'; Bytes=5592908; Sha256='af5a5d48916958a48f16bbd8208fde59eac89991d8b756b6c7bf633192419947'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/bellwether-roof-v1/normal.windows.ktx2'; Bytes=5592908; Sha256='39a2802fa7bd4ca493a7a0f6d219bb8e875bcab10970989b7d0686b174555a3f'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/bellwether-roof-v1/orm.windows.ktx2'; Bytes=5592908; Sha256='b82343ae790eb7e9605e375674f3232b10ba3d9581c99e5c7804ece6cf783f88'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/bellwether-roof-v1/base-color.android.ktx2'; Bytes=626752; Sha256='c5d5695c11e0ddb631b17d6a22402c6ed37eab462339b5f0be510aea50a02b74'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/bellwether-roof-v1/normal.android.ktx2'; Bytes=1398592; Sha256='1ed37de3f934772c8fc75a9382dceabc8b37bdbfadd5fab7af6958e017f34311'; Platform='Android'; Kind='PropsKtx'; Format=157; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/bellwether-roof-v1/orm.android.ktx2'; Bytes=626752; Sha256='2d1d19720779ca85bd24df0219d1f478523baf1c1995f5d0cac5a9a03bec4e1e'; Platform='Android'; Kind='PropsKtx'; Format=165; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/asset.manifest.json'; Bytes=15047; Sha256='11ff79f5aab2f1302060ca7b5a99ce7271591f63000aee55684762416d178f5d'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='textures/props/runtime/base-color.android.ktx2'; Bytes=11273648; Sha256='5e21700c1f781c913e5dcf0ca05ff79e01db57eac39514ef3ed19a2b961eeb44'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=18 },
        [pscustomobject]@{ Path='textures/props/runtime/normal.android.ktx2'; Bytes=20972384; Sha256='defa8b9dbce7130cc8ea601a2868a486a4f11882851d62b25e60aa0742bcfa30'; Platform='Android'; Kind='PropsKtx'; Format=157; Layers=15 },
        [pscustomobject]@{ Path='textures/props/runtime/orm.android.ktx2'; Bytes=9394784; Sha256='97bd3e4d4f687beec09cd080c9753d794c7a38d3ce5209cefc208cafd2f32ec2'; Platform='Android'; Kind='PropsKtx'; Format=165; Layers=15 },
        [pscustomobject]@{ Path='textures/props/runtime/emissive.android.ktx2'; Bytes=626752; Sha256='da5ee0baa4e6b84280a1b9ae077954c4658a4e7fff1da03212d220d282f9542a'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/base-color.windows.ktx2'; Bytes=100663776; Sha256='05b7fef7b69ac1f2f42f863594663f4bafa0f4f346334cbfc01a1efabb7456cf'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=18 },
        [pscustomobject]@{ Path='textures/props/runtime/normal.windows.ktx2'; Bytes=83886564; Sha256='6ecc469a0bfde110a82a49857f7dd786823c9cebfe50da22963d54149af2a4e3'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=15 },
        [pscustomobject]@{ Path='textures/props/runtime/orm.windows.ktx2'; Bytes=83886564; Sha256='b18098e68c838f6b3ca2a4578fb851ba73bd22e84b89bf546b3e6559ba2f01c7'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=15 },
        [pscustomobject]@{ Path='textures/props/runtime/emissive.windows.ktx2'; Bytes=5592908; Sha256='c642d3972a14bf477d88f2ecb34ffc177ad4dc6308dfe8f507af6033c292dd56'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=1 }
    )
}

function Get-Horde162ManifestSpecification {
    @(
        [pscustomobject]@{ Path='audio/pixabay/keeper-asset.manifest.json'; Platform='Both' },
        [pscustomobject]@{ Path='audio/pixabay/waterfall-core.manifest.json'; Platform='Android' },
        [pscustomobject]@{ Path='textures/environment/runtime/asset.manifest.json'; Platform='Both' }
    )
}

function Join-Horde162Path {
    param([string]$Root, [string]$Relative)
    [IO.Path]::Combine([IO.Path]::GetFullPath($Root), ($Relative.Replace('/', [IO.Path]::DirectorySeparatorChar)))
}

function Get-Horde162Sha256 {
    param([string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-Horde162Wave {
    param([string]$Path, [int]$Channels, [long]$Frames)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 44 -or [Text.Encoding]::ASCII.GetString($bytes,0,4) -cne 'RIFF' -or
        [BitConverter]::ToUInt32($bytes,4) -ne $bytes.Length-8 -or [Text.Encoding]::ASCII.GetString($bytes,8,4) -cne 'WAVE') {
        throw "1.6.2 WAV RIFF header mismatch: $Path"
    }
    $formatCount=0; $dataCount=0; $offset=12L
    while ($offset -lt $bytes.Length) {
        if ($offset+8 -gt $bytes.Length) { throw "1.6.2 WAV truncated chunk: $Path" }
        $id=[Text.Encoding]::ASCII.GetString($bytes,[int]$offset,4)
        $size=[long][BitConverter]::ToUInt32($bytes,[int]$offset+4)
        $body=$offset+8
        if ($body+$size -gt $bytes.Length) { throw "1.6.2 WAV invalid chunk size: $Path" }
        if ($id -ceq 'fmt ') {
            ++$formatCount
            if ($size -ne 16 -or [BitConverter]::ToUInt16($bytes,[int]$body) -ne 1 -or
                [BitConverter]::ToUInt16($bytes,[int]$body+2) -ne $Channels -or
                [BitConverter]::ToUInt32($bytes,[int]$body+4) -ne 48000 -or
                [BitConverter]::ToUInt32($bytes,[int]$body+8) -ne 48000*$Channels*2 -or
                [BitConverter]::ToUInt16($bytes,[int]$body+12) -ne $Channels*2 -or
                [BitConverter]::ToUInt16($bytes,[int]$body+14) -ne 16) { throw "1.6.2 WAV must be PCM16 LE 48 kHz with $Channels channels: $Path" }
        }
        if ($id -ceq 'data') { ++$dataCount; if ($size -ne $Frames*$Channels*2) { throw "1.6.2 WAV frame count mismatch: $Path" } }
        $offset=$body+$size+($size%2)
    }
    if ($offset -ne $bytes.Length -or $formatCount -ne 1 -or $dataCount -ne 1) { throw "1.6.2 WAV requires exactly one format/data chunk: $Path" }
}

function Assert-Horde162EnvironmentKtx {
    param([string]$Path, [uint32]$Format)
    $bytes=[IO.File]::ReadAllBytes($Path)
    $signature=[byte[]]@(0xab,0x4b,0x54,0x58,0x20,0x32,0x30,0xbb,0x0d,0x0a,0x1a,0x0a)
    if ($bytes.Length -lt 80 -or [Convert]::ToBase64String($bytes,0,12) -cne [Convert]::ToBase64String($signature) -or
        [BitConverter]::ToUInt32($bytes,12) -ne $Format -or [BitConverter]::ToUInt32($bytes,20) -ne 512 -or
        [BitConverter]::ToUInt32($bytes,24) -ne 256 -or [BitConverter]::ToUInt32($bytes,28) -ne 0 -or
        [BitConverter]::ToUInt32($bytes,32) -ne 0 -or [BitConverter]::ToUInt32($bytes,36) -ne 1 -or
        [BitConverter]::ToUInt32($bytes,40) -ne 10 -or [BitConverter]::ToUInt32($bytes,44) -ne 0) {
        throw "1.6.2 environment must be the admitted 512x256, ten-mip, native-format KTX2: $Path"
    }
}

function Assert-Horde162Assets {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot)
    $assets=Join-Horde162Path $RepositoryRoot 'assets'
    foreach ($spec in @(Get-Horde162AssetSpecification; Get-Horde17TombAssetSpecification; Get-Horde17KitAssetSpecification; Get-Horde17KitVisualAssetSpecification)) {
        $path=Join-Horde162Path $assets $spec.Path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "1.6.2 runtime asset missing: $($spec.Path)" }
        if ((Get-Item -LiteralPath $path).Length -ne $spec.Bytes -or (Get-Horde162Sha256 $path) -cne $spec.Sha256) { throw "1.6.2 runtime byte/hash mismatch: $($spec.Path)" }
        if ($spec.Kind -ceq 'Wave') { Assert-Horde162Wave $path $spec.Channels $spec.Frames }
        elseif ($spec.Kind -ceq 'KitWave') { Assert-Horde17KitWave $path $spec.Frames }
        elseif ($spec.Kind -ceq 'Ktx') { Assert-Horde162EnvironmentKtx $path $spec.Format }
        elseif ($spec.Kind -ceq 'PropsKtx') {
            $bytes=[byte[]]::new(80); $stream=[IO.File]::OpenRead($path)
            try { $read=$stream.Read($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
            if ($read -ne 80 -or [BitConverter]::ToString($bytes,0,12) -cne 'AB-4B-54-58-20-32-30-BB-0D-0A-1A-0A' -or
                [BitConverter]::ToUInt32($bytes,12) -ne $spec.Format -or [BitConverter]::ToUInt32($bytes,20) -ne 1024 -or
                [BitConverter]::ToUInt32($bytes,24) -ne 1024 -or [BitConverter]::ToUInt32($bytes,28) -ne 0 -or
                [BitConverter]::ToUInt32($bytes,32) -ne $spec.Layers -or [BitConverter]::ToUInt32($bytes,36) -ne 1 -or
                [BitConverter]::ToUInt32($bytes,40) -ne 11 -or [BitConverter]::ToUInt32($bytes,44) -ne 0) {
                throw "1.6.2 props KTX must match the admitted 1K/full-mip/layer/native-format profile: $path"
            }
        }
        elseif ($spec.Kind -ceq 'Glb') {
            $bytes=[IO.File]::ReadAllBytes($path)
            if ($bytes.Length -lt 20 -or [Text.Encoding]::ASCII.GetString($bytes,0,4) -cne 'glTF' -or
                [BitConverter]::ToUInt32($bytes,4) -ne 2 -or [BitConverter]::ToUInt32($bytes,8) -ne $bytes.Length -or
                [Text.Encoding]::ASCII.GetString($bytes,16,4) -cne 'JSON') { throw "1.6.2 runtime GLB header/length mismatch: $path" }
        }
        elseif ($spec.Kind -ceq 'Json') { $null=Get-Content -LiteralPath $path -Raw | ConvertFrom-Json }
        else { throw "1.6.2 runtime asset has an unknown admission type: $($spec.Path)" }
    }
    $keeper=Get-Content -LiteralPath (Join-Horde162Path $assets 'audio/pixabay/keeper-asset.manifest.json') -Raw | ConvertFrom-Json
    if ($keeper.schema -ne 1 -or $keeper.target -cne '1.6.2' -or $keeper.license -cne 'Pixabay Content License; no CC0 claim' -or
        $keeper.licenseTerms -cne 'https://pixabay.com/service/terms/' -or @($keeper.assets).Count -ne 4) { throw '1.6.2 keeper manifest admission mismatch.' }
    $keeperPaths=@('keeper_i_sense_you.wav','keeper_come_closer.wav','skeleton_idle_rattle.wav','skeleton_falling_bones.wav')
    foreach ($name in $keeperPaths) {
        $entries=@($keeper.assets | Where-Object { $_.runtimePath -ceq $name })
        $spec=@(@(Get-Horde162AssetSpecification; Get-Horde17TombAssetSpecification; Get-Horde17KitAssetSpecification; Get-Horde17KitVisualAssetSpecification) | Where-Object { $_.Path -ceq "audio/pixabay/$name" })[0]
        if ($entries.Count -ne 1 -or $entries[0].sha256 -cne $spec.Sha256 -or $entries[0].bytes -ne $spec.Bytes -or
            $entries[0].sampleRate -ne 48000 -or $entries[0].channels -ne 1 -or $entries[0].bitsPerSample -ne 16 -or
            $entries[0].frames -ne $spec.Frames) { throw "1.6.2 keeper manifest runtime mismatch: $name" }
    }
    $core=Get-Content -LiteralPath (Join-Horde162Path $assets 'audio/pixabay/waterfall-core.manifest.json') -Raw | ConvertFrom-Json
    $coreSpec=@(@(Get-Horde162AssetSpecification; Get-Horde17TombAssetSpecification; Get-Horde17KitAssetSpecification; Get-Horde17KitVisualAssetSpecification) | Where-Object { $_.Path -ceq 'audio/pixabay/waterfall_core_loop.wav' })[0]
    $fullSpec=@(@(Get-Horde162AssetSpecification; Get-Horde17TombAssetSpecification; Get-Horde17KitAssetSpecification; Get-Horde17KitVisualAssetSpecification) | Where-Object { $_.Path -ceq 'audio/pixabay/waterfall_loop.wav' })[0]
    if ($core.schema -ne 1 -or $core.runtimePath -cne 'waterfall_core_loop.wav' -or $core.sha256 -cne $coreSpec.Sha256 -or
        $core.bytes -ne $coreSpec.Bytes -or $core.frames -ne $coreSpec.Frames -or $core.channels -ne 2 -or
        $core.sampleRate -ne 48000 -or $core.bitsPerSample -ne 16 -or $core.source -cne 'waterfall_loop.wav' -or
        $core.sourceSha256 -cne $fullSpec.Sha256 -or $core.sourceFrames -ne $fullSpec.Frames) { throw '1.6.2 waterfall manifest admission mismatch.' }
    $environment=Get-Content -LiteralPath (Join-Horde162Path $assets 'textures/environment/runtime/asset.manifest.json') -Raw | ConvertFrom-Json
    if ($environment.schema -ne 1 -or $environment.rights -cne 'Project-owned generated asset.' -or @($environment.runtime).Count -ne 2) { throw '1.6.2 environment manifest admission mismatch.' }
    foreach ($platform in @('windows','android')) {
        $entry=@($environment.runtime | Where-Object { $_.platform -ceq $platform })
        $spec=@(@(Get-Horde162AssetSpecification; Get-Horde17TombAssetSpecification; Get-Horde17KitAssetSpecification; Get-Horde17KitVisualAssetSpecification) | Where-Object { $_.Path -ceq "textures/environment/runtime/night-storm.$platform.ktx2" })[0]
        $format=if($platform -ceq 'windows'){'R8G8B8A8_SRGB'}else{'ASTC_6x6_SRGB_BLOCK'}
        if ($entry.Count -ne 1 -or $entry[0].path -cne "night-storm.$platform.ktx2" -or $entry[0].sha256 -cne $spec.Sha256 -or
            $entry[0].bytes -ne $spec.Bytes -or $entry[0].format -cne $format -or @($entry[0].dimensions).Count -ne 2 -or
            $entry[0].dimensions[0] -ne 512 -or $entry[0].dimensions[1] -ne 256 -or $entry[0].mipLevels -ne 10) { throw "1.6.2 environment manifest runtime mismatch: $platform" }
    }
    return [pscustomobject]@{ Keeper=$keeper; Waterfall=$core; Environment=$environment }
}

function Get-Horde162RuntimeFiles {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot, [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    $null=Assert-Horde162Assets $RepositoryRoot
    @(@(Get-Horde162AssetSpecification; Get-Horde17TombAssetSpecification; Get-Horde17KitAssetSpecification; Get-Horde17KitVisualAssetSpecification) | Where-Object { $_.Platform -ceq 'Both' -or $_.Platform -ceq $Platform } | ForEach-Object Path)
    @(Get-Horde162ManifestSpecification | Where-Object { $_.Platform -ceq 'Both' -or $_.Platform -ceq $Platform } | ForEach-Object Path)
}

function Assert-Horde162StagedAssets {
    param([string]$RepositoryRoot, [string]$AssetRoot, [ValidateSet('Windows','Android')][string]$Platform)
    $expected=@(Get-Horde162RuntimeFiles $RepositoryRoot $Platform)
    $observed=@(foreach($root in @('audio/menu','audio/filmcow/equipment','audio/pixabay','audio/kit','textures/environment','textures/props','models/world','models/characters','textures/characters','models/props/runtime/player-rag-torch','models/props/runtime/player-sword-scabbard','models/props/source')) {
        $path=Join-Horde162Path $AssetRoot $root
        if (Test-Path -LiteralPath $path -PathType Container) {
            foreach($item in Get-ChildItem -LiteralPath $path -Recurse -Force) {
                if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw '1.6.2 staged assets contain a reparse point.' }
                if (-not $item.PSIsContainer) { $item.FullName.Substring([IO.Path]::GetFullPath($AssetRoot).Length+1).Replace('\','/') }
            }
        }
    })
    if ([string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive)) -cne
        [string]::Join("`n",[string[]]@($observed | Sort-Object -CaseSensitive))) { throw "1.6.2 $Platform staged inventory is not the closed runtime roster." }
    foreach($relative in $expected) {
        $source=Join-Horde162Path $RepositoryRoot "assets/$relative"; $staged=Join-Horde162Path $AssetRoot $relative
        if ((Get-Item -LiteralPath $source).Length -ne (Get-Item -LiteralPath $staged).Length -or
            (Get-Horde162Sha256 $source) -cne (Get-Horde162Sha256 $staged)) { throw "1.6.2 staged byte/hash mismatch: $relative" }
    }
}

function Copy-Horde162RuntimeAssets {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot, [Parameter(Mandatory=$true)][string]$AssetRoot,
          [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    foreach($relative in Get-Horde162RuntimeFiles $RepositoryRoot $Platform) {
        $source=Join-Horde162Path $RepositoryRoot "assets/$relative"; $target=Join-Horde162Path $AssetRoot $relative
        if (Test-Path -LiteralPath $target) {
            if ((Get-Horde162Sha256 $target) -cne (Get-Horde162Sha256 $source)) { throw "Refusing to overwrite a different staged asset: $relative" }
        } else {
            $null=New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
            Copy-Item -LiteralPath $source -Destination $target
        }
    }
    Assert-Horde162StagedAssets $RepositoryRoot $AssetRoot $Platform
}

function Assert-Horde162Package {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot, [Parameter(Mandatory=$true)][string]$ArchivePath,
          [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    $expected=@(Get-Horde162RuntimeFiles $RepositoryRoot $Platform | ForEach-Object { "assets/$_" })
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
    try {
        foreach ($entry in $zip.Entries) {
            if ($entry.FullName.Contains('\') -or $entry.FullName.StartsWith('/', [StringComparison]::Ordinal) -or
                $entry.FullName -match '(^|/)(\.|\.\.)(/|$)') { throw '1.6.2 package contains a noncanonical or traversing entry path.' }
        }
        $entries=@($zip.Entries | Where-Object { $_.FullName -match '(?i)^assets/(audio/menu|audio/filmcow/equipment|audio/pixabay|audio/kit|textures/environment|textures/props|textures/characters|models/characters|models/world|models/props/runtime/player-rag-torch|models/props/runtime/player-sword-scabbard|models/props/source)(/|$)' })
        # ZIP directory entries are metadata, distinguished by their trailing
        # slash, not by byte length (a zero-byte foreign file is still a file).
        foreach($entry in $entries) {
            if ($entry.FullName.EndsWith('/', [StringComparison]::Ordinal)) {
                if ($entry.Length -ne 0) { throw '1.6.2 package has a malformed nonempty directory entry.' }
                if ($entry.FullName.Contains('\') -or @($expected | Where-Object {
                    $_.StartsWith($entry.FullName, [StringComparison]::Ordinal)
                }).Count -eq 0) { throw '1.6.2 package directory is outside the closed runtime roster.' }
            }
        }
        $names=@($entries | ForEach-Object FullName)
        $unique=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        foreach($name in $names) { if (-not $unique.Add($name)) { throw '1.6.2 package has duplicate runtime entry names.' } }
        $entries=@($entries | Where-Object { -not $_.FullName.EndsWith('/', [StringComparison]::Ordinal) })
        $names=@($entries | ForEach-Object FullName)
        if ([string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive)) -cne
            [string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive))) { throw "1.6.2 $Platform package inventory is not the closed runtime roster (missing, source, foreign or other-platform entry)." }
        foreach($entry in $entries) {
            $source=Join-Horde162Path $RepositoryRoot $entry.FullName
            if ($entry.Length -ne (Get-Item -LiteralPath $source).Length) { throw "1.6.2 package byte/hash mismatch: $($entry.FullName)" }
            $sha=[Security.Cryptography.SHA256]::Create(); $stream=$entry.Open()
            try { $actual=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
            finally { $stream.Dispose(); $sha.Dispose() }
            if ($actual -cne (Get-Horde162Sha256 $source)) { throw "1.6.2 package byte/hash mismatch: $($entry.FullName)" }
        }
    } finally { $zip.Dispose() }
}
