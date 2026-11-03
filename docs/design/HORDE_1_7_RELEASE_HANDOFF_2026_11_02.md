# Horde 1.7 release handoff — 2 November 2026

This records a development checkpoint, not release approval. At `7292802b`,
the two owner-supplied, hash-pinned Kit basin cuts are packaged and Kit has a
bounded route to the upper-rope basin. Exact-head CI run `41439913167` passed
all six jobs. Focused Windows tests covered the guide on actual basin collision
geometry, ordered conditional dialogue, interrupted guidance, save restoration
and resuming the original forest route. The Windows candidate was built at this
head. The owner subsequently played the isolated Windows package built from
`7292802b` (documentation head `bf4307df`) and confirmed Kit walks to the
basin and explains it correctly for the flask state. That is owner-observed
Windows guide/conditional-speech acceptance, not final ordinary-package or
Android acceptance. The earlier `6054581e` owner test also passed two consecutive
death/Retry cycles.

## Remaining ordinary-build admission

1. The normal application profile still leaves `skeletonRevival1_7`,
   `naturalPracticeRevival1_7`, `healingFlask1_7` and
   `flaskUtilityCandidate` off. The Windows `HORDE_RT_PRIVATE_REVIVAL` and
   `HORDE_RT_FLASK_CANDIDATE` options and Android release equivalents are not
   shipping admission. Admit the already approved behaviour deliberately,
   check assets/licences, then build and test ordinary Windows and Android
   packages, rather than treating a private Debug candidate as the release.
2. `VERSION` remains `1.6.2` and `version-code-map.json` ends at Android code
   10. `1.7.0` with a code greater than 10 is a proposed next identity, not an
   assigned or externally verified Play identity. Use the release-version
   policy and verify external availability before committing a new mapping.
   Do not reuse the immutable `1.6.2` source or code 10 for publication.
3. The owner-only signing backup and recovery checklist is unchecked here.
   Only the owner can confirm those safeguards; the existing local signing
   handoff is not evidence of an independent backup. Do not read or disclose
   signing secrets during this handoff.
4. Check an update from the published `1.6.2` packages to the exact 1.7
   candidate with existing three-slot saves, including normal load, autosave,
   invalid-slot recovery, settings retention and uninstall-free Android
   identity. No safe-upgrade result is claimed from current host tests.
5. Package and inspect both final binaries and their exact source, shader,
   asset and licence receipts. Test cold start, normal chapter journey,
   save/load and Retry, touch/controller/keyboard, lifecycle and the scoped
   forest/combat/flask visuals on actual Windows and Android hardware. Green
   CI proves its selected jobs, not physical presentation or voice balance.
6. The consented private report and public benchmark submission must use the
   final admitted artifact identities, strict receipts and the active service
   catalog. Verify one authorized real end-to-end case for each intended
   platform and the live public page before release. Earlier mocked, local
   export and pre-final catalog evidence do not close this gate. Coordinate
   deployment and public data separately; do not send synthetic results.

## Short owner Kit check when a final ordinary candidate is ready

1. From an isolated normal game, reach Kit after the rope and raise the
   lantern. Follow him back toward the visible upper-rope basin. Check that
   he stops beside it, the player can reach and use Rest without clipping,
   and his single top-up cue is audible when charges remain.
2. With a separately earned empty flask, check the empty cue at that basin,
   then backtrack and continue into the forest. Confirm no repeat cue or
   duplicate reunion speech, Kit resumes the intended route, and save/reload
   or an interruption cannot leave him waiting forever. Do not use a staged
   fixture result as proof of the natural chapter.

The existing Debug-only hidden route validator is not a visual witness for
this change: its driver follows the former direct forest path and the current
Debug profile does not enable the flask guide. It must be adapted and run with
an isolated profile before its RT captures can support a staging claim. The
owner's screen and saves were not touched overnight.

In-app Windows and Android credits now include the tawny owl and cricket
attributions and link to the packaged notice. `release/SAMFA12_SITE_COPY.md`
is explicitly labelled historical 1.6.0 copy, not current release text.


## 2 November evening, sharing and footstep checkpoint

Development runtime43d5886d passed all six hosted CI jobs (run41513565737).
The fixed Windows Standard benchmark completed a real2-lap/26-waypoint run with
1838valid CPU/GPU rows, an acknowledged1920x1080 apply and restored1920x966
window/settings. The owner then recalls a publication tick, but live public GET
showed zero rows and the closed client retained no matching receipt. Public
publication is unconfirmed. Do not resend this report or close the reporting
gate from the local benchmark alone. The private compatibility email for the
earlier missing-assets startup was independently delivered; it has a different
purpose and payload. Issue38 links that manually mis-staged package failure.

The revised Windows results UI gives direct Publish to website and Send to
developer entries, a single local Review step, explicit separate remote
consent and a Verify/Send action. Technical JSON is collapsed by default.
Published, awaiting review and failure statuses are distinct. The exact
payload builders, verification service and immutable retry owner remain in
place. Source/fixture validation is recorded separately; final ordinary
package and owner clarity check remain ahead. Android result UI is a separate
surface and its compact disclosure had earlier private device coverage, not
this Windows flow.

Grounded dry/wet player contacts now use the authored Walking clip's half-cycle
spacing at the production1.9m/s movement reference,0.989583m after the existing
initial0.24m weight shift. This addresses the owner-observed overly frequent
sound cadence while retaining actual-travel and support gating, surface family
routing and wet-step priority. Host checks cover those contracts. Physical
foot-to-sound perception across walk, run, reverse and terrain still needs owner
acceptance on a package built from this commit.

The phone was restored before owner departure: original debug APK SHA and all
593 original files matched backup, rotation and Home restored, production
untouched. No phone evidence from the later Windows sharing/cadence edits.
The owner supplied a local signing-key folder; key material was not read and
independent backup/recovery remains owner-only confirmation.

Release communication scope: itch devlog, itch main-page update and the
samfa12.com Horde page should accompany the final verified1.7 publication.
Post-release owner playthrough video and multi-device benchmarks are future
follow-up, not gates for this source checkpoint.

## Issue 38 private Windows staging guard

The failed `66b707b8` isolated Windows package lacked three acceptance-only
files beside the executable: `private_flask/player.glb`,
`private_revival/skeleton.glb`, and `private_revival/lich_revival.wav`. The
runtime rejected scene creation as designed. Restaging those three pinned
files without changing the executable produced a rendered RT game/menu on the
same laptop. This explains that manually staged package failure; it does not
certify an ordinary shipping package or unrelated field startup reports.

Before launching a manually staged **private revival/flask** Windows package,
run `tools/test-horde-1.7-private-windows-stage.ps1 -StageRoot <stage>`.
It requires the executable and compares all three files against the source
hash pins. The `e969f72e` isolated package passed; an unstaged build output
failed with the expected missing-flask diagnostic. This guard is for the
explicit private acceptance profile. The ordinary `package-alpha.ps1` still
builds with both private feature flags OFF; its eventual 1.7 shipping admission
and asset/licence checks are a separate release gate.

## 2 November ordinary-release preflight at 8a9dbc61

- PR37 is a clean, mergeable **draft**, 61 commits ahead of current main
  `1b119d52`. All six exact-head jobs passed in run `41521394499`. This is
  source readiness, not ordinary-package acceptance; no merge was performed.
- `ProductionGameSimulationConfig()` admits combat foundation, special warnings,
  sequential practice and the route, but leaves skeleton revival, natural
  practice revival, healing flask and its native utility binding OFF. Windows
  CMake gates both trials OFF by default. `tools/package-alpha.ps1` forces them
  OFF even if a cached build selected them. Android Release forces both OFF and
  includes the pinned derivatives only in the Debug asset source set. A normal
  1.7 package therefore cannot be described as shipping the private flask or
  revival owner-accepted in development builds. Admission requires explicit
  release asset inputs, verified package presence and updated contracts on
  both platforms; do not merely remove the OFF guards.
- `ASSET_LICENSES.md` records the flask derivative as an owned source study and
  the revival WAV as a Pixabay-derived private game candidate. It explicitly
  withholds public repository redistribution and a final game-distribution
  decision for the revival clip/stand derivative. Confirm the exact integrated
  public-game permission and distribution method before vending these bytes;
  source MP3/Blend/previews should stay out of runtime packages. Existing Kit
  cuts have separate owner-approved public distribution permission.
- `VERSION` and the Android code map still identify 1.6.2/code10. Local policy
  requires a mutable version and code greater than10; 1.7.0/code11 is a
  proposal only. No external Play-console availability or owner signing-backup
  result was established. The external signing folder exists, but no key or
  password was read. The owner-only safety checklist remains unchecked.
- Eight focused Windows Release host tests passed for save menu, play session,
  save worker/coordinator, codec, storage, checkpoint and version policy.
  Literal legacy schema1/2 codec fixtures and normal dated restore are covered;
  an actual published1.6.2-to-final1.7 package update and owner career
  preservation are **not** covered by those fixtures.
- Current Windows/Android client sources route public benchmark and private
  report requests to distinct HTTPS endpoints and verification purposes.
  Windows classifies a public 202 exact `accepted` receipt as queued and a
  public 200 exact `published` receipt as published; wrong IDs/kinds or malformed
  responses remain uncertain. Android has separate live pending/published UI
  states and tests for separate consent and immutable retries. These are source
  and mock/fixture contracts, not proof of the live service's retention or
  delivery. The
  live public stats read returned zero rows. The private backend handoff's
  prepared catalog v3 documents earlier `7234bdad` Send-capable artifacts;
  it is not a catalog receipt for the newer `e969f72e` Windows candidate or an
  eventual ordinary release. Final exact binary tuples, backend activation,
  strict client receipts and one owner-authorized end-to-end send per platform
  remain coordinated release work. No report was sent in this audit.
- Publication copy is staged for review in
  `release/HORDE_1_7_DRAFT_COPY.md`, with gated features clearly held. Itch and
  website content remains unchanged.

## Ordinary 1.7 admission candidate after owner asset permission

The owner approved including the exact pinned skeleton stand derivative and
revival WAV **inside public game packages**, without standalone/source asset
publication. The owned flask native derivative is likewise included as a
pinned runtime input. The previous private-only distribution gate above is
superseded for these three integrated runtime bytes, but final signed-package
and physical acceptance remain open. `ASSET_LICENSES.md` records the scope.

Source now stages candidate identity `1.7.0`/Android code11, which passed the
local version contract. GitHub releases show no 1.7 tag; external Play-code
availability remains unverified. Normal Windows CMake and Android Release have
an explicit shipping17 admission separate from the private Debug/acceptance
flags. The Gradle Release task fails without an exact three-file local input;
the pinned input policy rejects extra files and wrong hashes. The ordinary
Windows Release executable and application-config test built, and the latter
passed. An unsigned Android Release APK built all four ABIs from a short
temporary drive path; its package name is the production identity,
versionName1.7.0/code11, and the exact three pinned assets passed archive hash
verification. The normal debug, benchmark and actor-worker variants are not
admitted by this shipping property.

The first Android attempt from the long nested worktree failed in Ninja's
object-directory creation before source compilation; the short drive mapping
resolved that host path-length limit. The compiled APK was **unsigned** and
was not installed. Neither this source/build receipt nor the local pinned
files establishes a signed ordinary package, owner-save update, actual RT
scene presentation, sound/animation balance or reporting catalog admission.
