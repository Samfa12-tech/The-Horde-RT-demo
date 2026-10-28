# Android local public benchmark preview

This advances the approved public-reporting scope after the repeated chapter
start correction fffd5ee6. It implements local preparation/review/export, not
submission, a leaderboard, website access, delivery, or publication.

A completed typed benchmark summary now offers a separate **Preview public
statistics** route. It starts with fresh unchecked local-preview consent and
optional hardware choices. The public projection uses a fresh random result ID,
coarsened date and aggregate measurements, omitting private report/run IDs and
raw frame rows. Its existing typed builder still marks unverified, unranked,
unknown comparison revisions/artifact identity and absent measured thermals and
scanout FPS honestly. These gaps are not promoted by the UI.

The existing form/export owner retains an explicit private/public audience.
Editing keeps that audience but discards approved bytes, requires new consent
and a fresh ID. Switching audience closes the previous owner. Stale detached
controls/picker tokens cannot authorize a replacement form. Native code copies
only the expected ready frozen summary and checks its revision again before
returning the exact prepared UTF-8 bytes. Java does not reserialize them. Both
private submission preparation paths explicitly reject a public-preview owner.
No public transport, send/publish consent, background upload, email or backend
mutation was added. Copy/Save acts only on the reviewed bytes; Save uses the
existing user-selected document destination and cancellation/retry ownership.
The public filename is distinct from the private statistics export.

## Validation

Focused Robolectric review/UI/private-report tests cover independent consent,
wrong destination/type/identity, hardware opt-out, rejection of private source ID
reuse, literal bytes, copy/export retry, close/edit, stale picker/control and
replacement-run rejection. The actual Activity form is measured in portrait and
landscape at2x font size, retaining48dp minimum consent targets and scrollable
content; an attempted optional Canvas capture produced blank pixels and is NOT visual
acceptance. Its unused capture-only hook was removed; the failed images remain
in ignored reports. No phone or RT/gameplay capture is claimed. Native four-ABI Debug
Shipping/Mobile compilation and Android lint are recorded in receipt.json.
Existing native projection privacy/population tests remain unchanged. The private
JSON builder and private projection wire format are unchanged by this bridge
factor. Current-source hosted CI is tracked separately from these local checks.

## Remaining scope and held boundaries

Physical picker/contrast/rotation/large-text comfort and actual JNI-backed
completed-benchmark preview remain pending. Windows public-preview UI is still
unfinished. Live private service compatibility/receipt/deduplication and explicit
public submission/abuse controls plus the public stats API/page/filter/sort and
product link remain unfinished. Website access is still denied at
`C:\Users\sam_s\Documents\Samfa12 website`; no alternate-path retry was made.
No synthetic result was sent or published. No phone/ADB, game/CUA launch,
foreground/controller operation, security or deployment action was used. Normal
flask/rings/yellow/shared64 and actor-worker gates remain unchanged.
