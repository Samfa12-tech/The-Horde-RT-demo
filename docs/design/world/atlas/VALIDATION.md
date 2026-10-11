# Atlas validation / 11 October 2026

This validates a documentation navigation derivative. It does not certify game geography, playable routes, sightlines, collision, water engineering, loading or device performance.

## Source and coordinate checks

- Built in an isolated repository copy from main `8e1a41ee4d04b3c45e63c872e3ef216b0d5e2b0d` on `docs/linked-world-atlas-2026-10-11`. No other checkout, branch, game asset or runtime code was edited.
- Read main's AGENTS.md and map source indexes. No tracked `.agents/skills` or nested AGENTS.md exists on this base; the source inventory also records their absence in the preserved trees.
- Visually inspected all eleven original WebP sheets: regional overview, forest, three Bellwether sheets, approaches, three dungeon floorplans, transitions and progression. Original SVGs, source generators and coordinate JSON were inspected before placement. [Coordinate evidence](coordinate-evidence.json) records 32 pinned source hashes.
- Forest/town JSON equals the dungeon JSON's retained baseline exactly. All 31 rooms satisfy their declared local-to-world translation, with zero rotation. Computed room-plate extents match the three boxes in the original regional source: Abbey 105 × 107 m, Foundry 85 × 124 m, Court 109 × 117 m.
- Every surface view, interior projection and approach inset fits within regional X −150…410, Y −510…440 m. The original equal-axis SVG transform is recovered from its plot rectangle. Original map XML subtree hash is unchanged, including terrain, water, contours, routes and source labels; added navigation is outside that subtree.
- Checked north, metres, lake Z0 datum, 5 m regional / 2 m connected contours and source scale bars: forest 20 m; town plans 40 m; Abbey approach 40 m; gorge and ridge approaches 50 m; each dungeon floorplan 20 m. Sections/profiles are identified separately from plans.
- Tomb/rescue alignment and treasury/finale extents remain unplaced. No runtime screenshot or oblique concept was converted into a measured plan.

## Links and browser review

`python docs/design/world/atlas/verify_atlas.py --remote` checks local files/anchors, source tree paths and hashes, preserved map XML and all bounds. Every one of the **43 unique remote URLs** returned HTTP 200; [individual results](remote-link-check.json) are retained. The frozen commit/path check accompanies HTTP results, so a successful HTML response alone is not treated as proof of a valid source file.

An isolated Chrome browser rendered the SVG and HTML. All ten embedded local images loaded. All eight navigation links, all ten overview links (including the three approach leaders), and every corresponding return link were clicked successfully while the browser context was offline. [Browser measurements](browser-check.json) record the results. No page-script errors occurred.

Desktop viewport: 1180 × 1450. Phone viewport: 390 × 844. Phone overview zoom produced a 1120 px map inside a 364 px scroll window; local-sheet zoom produced a 1600 px image inside the same 364 px window. Document width stayed 390 px. Locators are 120 px on desktop and 85 px on phone. Numbered links, prose and return buttons remain readable without enlarging the page.

Visual review corrected an overflowing forest title, leaders crossing original labels, a locator flex-width conflict and spacing beside axis labels/footer. The final overview and representative forest, Abbey and Bellwether navigation were inspected. New cards do not cover the principal walking routes, river, crossings, scale bar or north arrow. The source's projected interior boxes remain identified as interiors.

- [Final overview](overview.webp) / [vector source](overview.svg)
- [Desktop forest sheet and return link](review/forest-desktop.webp)
- [Desktop Abbey floorplan, locator and return link](review/abbey-desktop.webp)
- [Phone navigation index](review/phone-index.webp)
- [Phone Bellwether sheet header and controls](review/bellwether-phone.webp)
- [Phone overview at readable zoom](review/phone-map-zoom.webp)

## Delivery limits

GitHub Markdown has adjacent sheet links because SVG click-through may be stripped in previews. The [self-contained HTML](atlas.html) provides actual click-through and offline images after downloading/opening it in a browser. GitHub's HTML source page is not a running preview. Browser checks used localhost; no website was deployed and no actual Android device was tested.

The documentation branch is local and unpushed. The parent can publish it through normal review to provide a GitHub link usable on Android. Existing main, PR27 development, planning branches and game assets remain untouched.
