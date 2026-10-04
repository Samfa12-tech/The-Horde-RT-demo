# Licence scope

The root [LICENSE](LICENSE) grants the MIT licence for **Sam Small's original
Horde Lantern RT engine and game code**, and associated technical documentation,
within the scope below. It is not a blanket licence for everything in this
repository.

## Covered original software

Subject to the exclusions below, the grant covers original Horde software owned
by Sam Small:

- Shared engine, renderer, platform, gameplay, scene-loading, reporting, UI and
  update implementation in `src/`.
- Shader source and generated shader-code representations in `shaders/` and
  `src/`.
- Horde Android application/JNI implementation in `android/app/src/main/java/`
  and `android/app/src/main/cpp/`.
- Original supporting build configuration, automation, tools and test code,
  including `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `tools/`,
  `tests/`, `.github/workflows/`, and Android build/test configuration and code.
- Original technical documentation explaining how to build, use, test or modify
  that software, including the technical portions of `README.md` and the
  source-directory READMEs.

These are software categories, not a licence grant over every file in the listed
directories. Assets, campaign content, third-party material and separately
licensed components remain excluded wherever they occur. Generated code is
covered only to the extent it represents the covered original software; generating
or embedding an asset does not change that asset's licence.

The MIT terms are reproduced unmodified in `LICENSE`, with Sam Small's copyright
notice. They permit commercial and closed-source reuse of covered software,
subject to retaining the copyright and permission notice. This scope document
identifies the material being licensed; it adds no noncommercial, contribution
or other restriction to those MIT terms.

## Excluded material and separate terms

The root MIT grant does **not** apply to:

- Models, meshes, textures, materials, animations, audio, music, fonts, icons,
  logos, branding, screenshots, video, generated artwork or other media, including
  source files, derivatives and test/evidence copies. This includes `assets/`,
  Android graphical/audio resources, and media stored under `tests/` or `docs/`.
- Campaign/story content, lore, dialogue, narrative or artistic design material,
  whether in documents, data or embedded in code. Gameplay/engine implementation
  remains in scope; this grant does not license the separate creative content.
- Third-party code, SDKs, libraries, dependencies or copied material, wherever
  located. Their existing copyright notices, licence texts and applicable
  upstream terms remain in force.
- **Pocket Audio Core**, including all of `third_party/pocket-audio-core/`.
  Its existing component-specific status remains **UNLICENSED**. Neither Sam's
  ownership of Core nor this Horde licence changes that status. The vendored
  upstream MIT text applies only to components designated MIT by its own scope
  notice and licensing matrix; it does not grant MIT rights to Core.
- Published release binaries, archives and other mixed-content packages as a
  whole. Covered original code within them retains its applicable licence, but
  bundled assets and dependencies require their own permissions and notices.

For asset terms and provenance, consult [ASSET_LICENSES.md](ASSET_LICENSES.md) and
each asset's metadata. For software components, retain the licence/NOTICE files
and dependency notices supplied with each component. In particular, see:

- [cgltf's existing licence](third_party/cgltf/LICENSE).
- [Pocket Audio Core's Horde integration notice](third_party/pocket-audio-core/README.horde.md),
  [upstream scope notice](third_party/pocket-audio-core/upstream/LICENSE), and
  [licensing matrix](third_party/pocket-audio-core/upstream/LICENSES.md).
- The [WebView2 SDK notice in ASSET_LICENSES.md](ASSET_LICENSES.md#windows-report-verification-sdk-161-development).

Existing notices and attribution must be preserved. Material without a recorded
licence does not become MIT-licensed merely because it is public or stored here.
No rights held by another contributor are granted by Sam's copyright notice.

## Redistribution limits remain separate

This code-licensing change does not resolve the pending permission to retain the
Hotstrike skeleton derivative in public source/Git LFS, or grant standalone
redistribution rights for Pixabay content. The owner-supplied score remains
authorised for Horde use only. Existing asset restrictions and unresolved
permission/provenance gates remain exactly as recorded in `ASSET_LICENSES.md`;
do not treat the root MIT licence as asset clearance.

The repository is therefore **mixed-licence**, with an MIT-licensed original
Horde code layer. Reusing or distributing a complete working game still requires
checking the separate dependencies and assets, especially Pocket Audio Core.
