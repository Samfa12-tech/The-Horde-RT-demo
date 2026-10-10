# The Horde 1.7 • funerary utility source kit

Seven original Blender-authored A17/T03 assets. No paid generation, stock model, texture download, flame, light, inscription, loot mechanic or runtime integration.

## Included

- One displaced stone sarcophagus lid, with rounded edges and raised panel. This is ordinary supporting funerary dressing, not a replacement for the Keeper gravestone.
- One footed offering bowl with real interior depth.
- A broken urn base and a curved rim shard, both solid shells with exposed fracture edges.
- Three extinguished candle-stub variations, each with an uneven melted top and a charred wick.

`models/` contains individual GLB 2.0 exports. `source/` contains the matching editable Blender 4.3.2 files and deterministic construction script. `previews/` shows the actual exported/reimported models. `validation/` records independent buffer inspection and Blender reimport/topology checks. The asset manifest records roles, material bindings and collision guidance; SHA256SUMS binds the supplied payload.

## Scope and provenance

Source of requirements: https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/docs/horde-1.6.2-roadmap/docs/ASSET_PLAN_1_7.md, A17/T03. Read from that branch on 6 October 2026. Counts are a small starting family, not a promise to fill every niche.

Original geometry and simple PBR parameters were authored directly in Blender. No third-party models, images, fonts or texture maps are included in the GLBs. The preview layout uses a system font only in rendered evidence.

The original project output is supplied for the user's game development and modification. It introduces no third-party model/texture license obligations. This provenance statement is not an assertion of exclusive copyright or a warranty about legal registrability; no external stock-asset license is being claimed.

## Scale, origin and orientation

Source units are meters, Blender Z-up; portable GLBs use standard glTF Y-up. Each prop has a floor/base-centered origin suitable for static placement. The urn shard is already resting on its convex side. Candle wicks are separate material-bearing geometry within their own export, not loose shared-world objects. Do not rescale to arbitrary gameplay dimensions without checking clearance and intended placement.

## Materials

All materials are OPAQUE, single-sided, dielectric, nonemissive PBR. There are no textures, texture memory allocations, alpha planes, painted world shadows or baked lighting. UVs, normals and tangent bases are exported. Studio lights and ground in preview renders are not present in the GLBs.

- Stone uses `Horde_existing_masonry_binding_required`, a neutral gray-green rough material. Bind the accepted Horde masonry surface at admission. This pack does not duplicate or pretend to include Horde's licensed PBR textures.
- Earthenware uses original warm unglazed clay parameters, with a lighter exposed fracture material where needed.
- Candle geometry uses original beeswax and charred-wick parameters. Fine wax/clay surface maps can be shared later only if they earn their mobile cost.

This is a geometry/source candidate pack, not final in-engine material polish. None of these preview materials implies a texture-array/KTX2 admission or a new material pipeline.

## Mobile and collision guidance

The bounded authoring envelope is at most 1,500 triangles per asset, not a device-performance claim. Each GLB is under 100 KB. These already-small sources have one geometry level; no LOD files are mislabeled or inferred. Cull the small candles/shard at appropriate distances based on measured pixel coverage; introduce 3D LODs only where required by actual placement and native RT profiling.

Use one oriented box or convex hull for a placed lid if it needs blocking. Urn fragments, bowl and candle details should normally be nonblocking. Do not use their visual meshes as automatic triangle-soup collision, generate individual wick colliders, or add rigid-body breakage. Place props on credible shelves/floors and preserve walking/combat lanes and deliberate empty space.

## Verification and open gates

Verified for this export: GLB headers/buffers, finite attributes, indices, normalized orthogonal tangent bases, UV presence, no missing/external resources, intended opaque/nonemissive materials, per-mesh welded manifold topology, degenerate faces, true dimensions, Blender reimport and rendered visual review. See numeric reports rather than relying on this prose alone.

Still required: owner in-game art acceptance, final existing-stone material binding, exact scene placements and collision, Horde native RT import, direct/shadow/reflection/transmission review, Android ASTC/Windows material route if maps are bound, draw/BLAS/TLAS/placement-memory costs and actual phone/Windows performance/clearance testing. This source archive does not constitute runtime admission or a release. No complete T01 niche family, T02 bones or A02 rescue fitting is claimed.

## Rebuild

From the unpacked folder:

    blender --background --factory-startup --python source/build_props.py
    python source/validate_glb.py
    blender --background --factory-startup --threads 4 --python source/render_validate.py

Construction overwrites this pack's named output files in its own directory. Preserve any manual changes in a new source version first. Blender files keep the bevel/triangulate modifiers editable; GLB exports evaluate them. Rendering uses CPU Cycles without denoising because this host's Blender does not include OpenImageDenoise.
