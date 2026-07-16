# Lich placeholder texture derivatives v01

Generated from the embedded images in `assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb` by `tools/prepare-lich-textures.ps1`.

The two embedded PNG payloads are compressed differently but decode to identical pixels. Consequently, the source labelled emissive cannot be used as a whole-surface emission map. The tool derives a conservative violet selection for likely staff-crystal and eye/gem texels and retains `emissive-mask-preview.png` for audit. This colour threshold is deterministic placeholder art, not proof of the intended Meshy material regions.

Runtime formats:

- `base-color-2048-rgba8.ktx2`: raw RGBA8 sRGB KTX2 used by Windows.
- `emissive-2048-rgba8.ktx2`: raw RGBA8 sRGB KTX2 derived emission used by Windows.
- Raw `.rgba` payloads are temporary generation intermediates; the uncompressed KTX2 payload is used directly by the UV audit.
- `base-color-2048-astc6x6.ktx2`: strict `VK_FORMAT_ASTC_6x6_SRGB_BLOCK` Android derivative.
- `emissive-2048-astc6x6.ktx2`: strict `VK_FORMAT_ASTC_6x6_SRGB_BLOCK` Android derivative.

Audited hashes:

| File | Bytes | SHA-256 |
|---|---:|---|
| `base-color-2048-rgba8.ktx2` | 16,777,484 | `EB1EED6D5990C47FB98C7489A5BA7B5B47B8BBBD41AFD73A42457A668B82B168` |
| `emissive-2048-rgba8.ktx2` | 16,777,484 | `AA22C00F6E89D74D93B689E8B3C5E50101031D0595B04451A3EBCF15D559DA98` |
| `base-color-2048-astc6x6.ktx2` | 1,871,648 | `DC2CFB44F7929C4D85F3A85B3785296DA9DB607F04B727D64EAFC85B47283E46` |
| `emissive-2048-astc6x6.ktx2` | 1,871,648 | `679E84D1B7797414AEAAFE3268EBAC756F734E3546E399B94EC5BEC69D354F13` |
| `emissive-mask-preview.png` | 213,482 | `528D4427FE59787B014DC1566FE27B2A35D437F407056B7044EC232EE268901B` |

The script pins the audited GLB hash, verifies decoded image equality, discovers `ktx` from `PATH` (or accepts `-KtxPath`), validates both KTX2 files, and rejects any ASTC output whose VkFormat header is not 166. Re-audit and revise the script for a new source model instead of bypassing those checks.
