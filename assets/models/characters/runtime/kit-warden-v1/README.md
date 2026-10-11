# Kit Warden native runtime candidate

Derived from the exact owner-approved source in `../../source/kit-warden-v1/`.
Credit: Warden/Kit source created with Meshy; runtime processing by Samfa12/Codex.
Public Horde repository/game redistribution is owner-confirmed under the
applicable Meshy entitlement; this is not an MIT or CC0 asset declaration.

The 2,151,672-byte GLB SHA-256 is
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.
All 464 non-image buffer views retain their original bytes, including the
24-joint skin, 26,945 unique vertices, 77,904 indices and seven animation clips.
The four embedded images become 4x4 routing thumbnails. Actual texture pixels
are compiled separately; these thumbnails must never be used as runtime maps.
Material factors, UVs, normal-map strength, emission and animation nodes remain
unchanged. No remeshing, retargeting, decimation or provider generation occurred.

Reproduce from the repository root into a fresh directory:

```powershell
python tools/prepare-kit-runtime.py --source assets/models/characters/source/kit-warden-v1/briarhold-warden-1k.glb --output reports/kit-runtime-reproduction
pwsh -File tools/build-kit-textures.ps1 -InputDirectory reports/kit-runtime-reproduction/texture-inputs -OutputDirectory reports/kit-texture-reproduction
```

The recipe refuses to replace differing files. The texture compiler uses full
1K mip chains, RGBA8 on Windows, strict ASTC 4x4 normals and ASTC 6x6 other maps
on Android. ORM preserves the authored metallic/roughness channels and uses
AO=1 because the source has no occlusion map. Source masters stay separate.

Native static import, original/runtime indexed skin correspondence, all seven
clips at five phases, PBR tangent frames and original expanded-index mapping
are tested by `horde_rt_kit_source_intake_tests`. Khronos validation reports
zero errors and one inherited `NODE_SKINNED_MESH_NON_ROOT` warning. Native
source/runtime poses match; live RT placement still needs inspection.

This is a prepared candidate, not scene admission. It is not copied into game
packages yet. Texture upload, dedicated third-slot lifetime ownership, actual
Kit rendering, zone retirement and Windows/Android visual/performance checks
remain integration gates. The frozen 18-layer/three-character ceilings and
production shaders are unchanged.
