# Licensing Matrix

This repository is the public source home for the Pocket Audio family, but not
every component is currently offered under the same license. Check this matrix
before reusing, redistributing, or packaging any part of the tree.

| Component | Path | Status | License | Notes |
| --- | --- | --- | --- | --- |
| Repository docs and public project metadata | `README.md`, `docs/`, `CONTRIBUTING.md`, `SECURITY.md`, `PROJECT_MEMORY.md` | Open source | MIT | Covered by the root `LICENSE` unless a document states otherwise. |
| Pocket Chordsmith web app | `apps/chordsmith-web/` | Source-available app | UNLICENSED | Public source for transparency and collaboration. Not yet licensed for broad reuse as an MIT/open-source app. Public app link already used in repo: `https://samfa12.itch.io/pocket-chordsmith`. |
| Pocket DJ web app | `apps/pocket-dj/` | Source-available app | UNLICENSED | Public source for transparency and compatibility work. Not yet licensed for broad reuse as an MIT/open-source app. Public app link already used in repo: `https://samfa12.itch.io/pocket-dj`. |
| Pocket Audio Handoff | `apps/pocket-audio-handoff/` | Source-available hosted utility | UNLICENSED | Public source for the Samfa12 handoff route. Not licensed for broad reuse. |
| Pocket DAW | `apps/pocket-daw/` | Public alpha/private-boundary source | UNLICENSED | Kept `private: true` and native-Windows only. Public alpha releases exist, but the source is not open-source licensed. |
| Pocket Audio Core | `packages/pocket-audio-core/` | WIP/private package source | UNLICENSED | Shared runtime scaffold with tests and examples. Package metadata intentionally remains `private: true` and `UNLICENSED` until an explicit package license decision is made. |
| PCS Format | `packages/pcs-format/` | WIP/private package scaffold | UNLICENSED | Future format helpers and fixtures. Package metadata intentionally remains `private: true` and `UNLICENSED`. |
| Pocket Chordsmith Godot addon | `addons/pocket_chordsmith/` | Open source addon | MIT | Component license lives at `addons/pocket_chordsmith/LICENSE`. Preserve that file with addon releases. |
| Pocket Chordsmith composer Codex skill | `apps/chordsmith-web/skills/pocket-chordsmith-composer/` | Source-available project tool | UNLICENSED | Treat as part of the Chordsmith app boundary unless a separate license is added. |
| Test fixtures and examples | `packages/pocket-audio-core/tests/fixtures/`, `packages/pocket-audio-core/examples/`, `docs/examples/` | Mixed support material | Same as parent component unless stated | Core fixtures/examples follow the Pocket Audio Core license status; docs examples are MIT as documentation. |

This table is the human-facing policy description. The generated component
table in `docs/generated/LICENSING_TABLE.md` is derived from
`FAMILY_MANIFEST.json` and is the drift-checked inventory of current component
paths, licences and distribution states.

## Third-Party Notices

Do not remove third-party notices, generated dependency lockfiles, package
license metadata, or upstream license text. Dependency licenses are governed by
their own package metadata and should be checked before redistribution.

## Website Wording Guidance

Until the app/package rows above are intentionally relicensed, describe this
repository as a public source home with mixed licensing, not as a fully MIT
open-source monorepo.
