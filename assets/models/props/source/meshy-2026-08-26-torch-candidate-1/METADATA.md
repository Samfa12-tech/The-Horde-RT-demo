# Medieval Hand Torch Candidate 1

- Created: 2026-08-26 with the authenticated project Meshy account.
- Preview task: `01a03b9d-02b1-7b7b-86ee-5e8c1f47af51` (`meshy-7`, standard, Ultra, geometry only, GLB); 25 credits.
- Refine task: `01a03ba0-23a8-7bd4-92bf-3f0493705c61` (`meshy-7`, PBR, 2K, remove lighting, GLB); 10 credits.
- Geometry prompt: `Isolated realistic medieval hand torch for a first-person Gothic ruin game. Tapered dark hardwood shaft, forged iron collar and restrained cage, tightly wrapped charred fuel cloth/rope at the top, believable hand grip and proportions, slightly asymmetrical wear but a clean readable silhouette. No hand, no arm, no character, no floor, no wall bracket, no flame, no smoke, no sparks, no glow, no light, no text, no logo, no background. Bottom grip origin and a separate named flame socket region at the centre of the fuel head. Game-ready closed geometry.`
- Texture prompt: `Neutral high-quality PBR medieval torch: dark worn hardwood with fibre and grip wear, blackened forged iron with soot and varied roughness, charred cloth/rope fuel head with dry cracked carbon detail. No baked orange light, no painted flame, no glow, no directional shadow, no words or symbols.`
- Source hashes: preview `01bbeaec5fc9db0a357cb3f746b29b6fd23d8a0412b7a994fa3419d241a4bbcd`; refined PBR `c6ee218b6536ad13640b592ad990c521e7cc91943da8155b416c6294becc7ec4`.
- Review: geometry accepted after the required six neutral views: tapered shaft, collar/cage/head, closed readable silhouette, and no flame/glow/beam/hand/bracket. Meshy's pale clean PBR was rejected. A deterministic region-preserving neutral PBR grade darkens dielectric regions to worn hardwood/char, grades metallic regions to blackened iron, raises soot/fibre roughness, and retains a black emissive map; it paints no light.
- Processing: reviewed Blender collapse remesh, metre normalization, +Y up/+Z forward, tangents, exact `Grip`, `Flame`, and `Light`; 4,999 runtime triangles. Runtime GLB SHA-256 `b1a2078c78d54dae548b89e20168c9922c7dfe33851d47f3d98e7ff1e7c14b8d`.
- Evidence: `.superpowers/sdd/2026-08-26-fire-pbr-reward-lantern-player-upgrade/evidence/task-3/torch-candidate-1-neutral/`, `torch-source-pbr/`, and `torch-runtime-pbr/`.
- Licence: the current account plan could not be independently proved. Distributed conservatively under Meshy Free-plan CC BY 4.0 with attribution in `ASSET_LICENSES.md`.

