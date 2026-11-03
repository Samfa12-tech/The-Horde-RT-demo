# Superseded candidate checkpoint

The later [private chapter integration](../2026-11-01-forest-integration/README.md)
preserves this owner-approved tree direction, adds selective fences/ground cover
and actual tree contact, and enables the admitted normal chapter roster. The
disabled/default-off statements below describe this earlier immutable checkpoint.

# Forest composition revision — 1 November 2026

**Improved native Debug candidate; not admitted for normal 1.7 play.** F01 now has
woodland crowns across the former empty horizon. F04's fence across the onward
trail is removed through an explicit portal following the approved map's arrival
route. The F01 rail enclosure, repeated trunks and thin lower vegetation still
fall short of convincing whole-route woodland. The production default remains
original64, and overlapping trial rosters remain exclusive.

![Final F01 north, actual hardware RT](193-chapter-landing-north.png)
![Final F04, actual hardware RT](203-chapter-forest-lookout.png)
![Final reverse F01, actual hardware RT](195-chapter-landing-south.png)

## Bounded implementation

The same 480-tree roster size retains all 64 original placements/models. The 96
middle trees and 320 far trees are redistributed into wraparound stands; owned
crowns are broadened/tallened and living branches lowered. The first revision
produced conspicuous large angular leaves. The retained refinement instead spreads
spray centres while preserving small leaves. Same four texture/material families,
no new provider, rendering architecture, shader, quality reduction or capacity
change. Exact source tree cost is 514,048 triangles (+190,784 against original64),
10,897,408 added raw native tree bytes. This is geometry accounting, not GPU cost.

Every boundary span now checks existing imported terrain and attached solid
geometry, rather than only checking spans preflagged by an authoring height rise.
The retained native scene uses 109 fence spans, 81 genuinely blocked natural
sections and six explicit mapped-arrival portal spans. Eleven terminal posts close
visible short fence ends; adjoining fence modules still share a post. Fences have
5,364 rendered triangles and 2,268 exact rendered collision triangles; total added
native vertex/index payload is 11,190,640 bytes. Existing ground, original colliders
and original roots are preserved. No invisible sheet or unproved bank substitutes
for a fence.

`boundarySectionsContained=true` describes only the closed sections.
`boundaryContained=false`, `defaultEnabled=false` and
`forestOrPerformanceAdmitted=false` are explicitly recorded. The open arrival
could permit access around the forest boundary; it does **not** certify that every
uncollided extra tree is unreachable. Normal gameplay activation stays off.

## Checks and actual evidence

- Release application and Debug application builds passed.
- Eight selected Release CTests passed: original64/native forest, exclusive north68,
  full480 native, source authoring, runtime roster, texture identity, placement
  identity and root fitting. Source authoring has eleven tests, including exact
  regeneration, map portal identity and rejection of arbitrary extra gaps.
- Rebuilt Debug graphics-preview parser and character/render-slot ownership checks: 2/2 passed.
- Full480 native evidence includes 2,861 crossing samples; 3,768 standing/crouched
  clearance samples cover the forest and its first mapped arrival segment at
  lateral offsets -1.2/0/+1.2 metres. These finite comparisons preserve clearance
  wherever the original scene was clear; they are not a complete walk/run/dodge,
  Kit/rope/return or reachable-world navigation proof.
- Native CPU packing/removal/restoration uses 17,389 freshly enumerated triangles,
  rejects reuse on original64 and reconstructs the identical full geometry witness.
  This is ownership/recreation evidence, not ray-contributor irrelevance or streaming.
- An earlier concurrent Debug selection passed original64/full480/source but
  north68 exceeded its unchanged 30-second deadline. That failed log is retained;
  the current Release north68 passes. No timeout was increased or test removed.
- Final hidden Windows RTX F01 north, F04 and reverse F01 captures use the same
  executable and final native sources. Per-view exact hashes, resource recreation,
  RT identity and process custody are in [receipt.json](receipt.json).

The capture runs use the established hidden Debug path with no input or activation.
All 151 parked owner-file hashes remain unchanged, as do settings/campaign custody.
Mobile contact-spray omission/High retention is untouched. No phone or iPad session,
foreground takeover, security change, LFS download, paid asset, merge or release.
CI is deliberately skipped on the development backup while the existing no-LFS
restriction persists; local checks are not relabelled as hosted CI.

## Remaining work for normal 1.7 integration

Continue ordinary art/layout work: give the graveyard and route selective,
purposeful bank/rock/short-fence boundaries, and fill the bare lower woodland with
varied owned vegetation/ground mass. Then inspect bends/F03/side/up/reverse and
moving player-height views, preserve Kit/rope/combat/landmark space, and prove the
open arrival cannot bypass the bounds into uncollided scenery (or add collision
to newly reachable trunks). This is unfinished authoring/validation, not a new
permission request or a blocked access claim.

After that acceptance, package the admitted immutable assets and deliberately
connect the existing native preparation to the shared chapter's normal content
selection; remove the Debug-only restriction only for the accepted content path,
retaining the original64 control. Check both platform packaging/intake and actual
Android correctness with an allocated device. The existing RT backend suffices;
no new rendering architecture is needed. Android and matched GPU/resource-peak
cost are unmeasured here; this checkpoint makes no phone performance claim.

Audio/haptic manual revalidation required: **NO** — no event/audio/haptic owner,
assets or timing changed; this candidate remains outside ordinary gameplay.

Raw logs, OBJ exports and interim images remain ignored under
`reports/forest-surroundings-revision2-20261101/`. The previous failed native
checkpoint remains in [its original record](../2026-10-31-forest-surroundings-native/README.md).
