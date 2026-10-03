# Fourth bounded Windows picker observation

Human-readable observation, not a verbatim machine log. No production source
change or rebuild. Helper: `windows-real-save-visible-settled.ps1`; executable
SHA-256 C9FE9DF0D1B41E150C91430725AE5C563A092E41A8FE7F4A20F4435A0D9B4D7D.

The three earlier incomplete attempts are preserved. This continuation addresses
their specific validity gap: an initial HWND or accessibility snapshot does not
establish settled, visible picker contents. The helper selects only the process
it creates, requires exactly one **visible** `#32770`, and polls for at most20s.
It examines Edit/ComboBox metadata and exact File name/Filename labels, including
labelled-by and parent-combo associations. Two consecutive identical target
identities would be required before one approved fresh-path write. Directory
item names, values, other applications and clipboard are not logged or modified.

Observed zero picker HWNDs initially, then one visible picker. The settled tree
returned32 `UIProperty` Edit records and one `SearchEditBox`; no candidate matched
the filename-label predicate. No destination value or Save command was sent.
The helper exits1 with the explicit missing-filename-field exception. No JSON
exists at its exact target. Picker cancellation and application-close messages
complete without forced process termination; the process wrapper prints an empty
exit-code field, so **no numeric clean-exit result is asserted**. Follow-up process
inventory found no remaining `HordeLanternRT` process.

Result: actual Windows file-save remains **INCONCLUSIVE**, not a product save
failure or a proof that the filename field is absent for a person. The native
form/injected-destination fixtures and Android actual exports remain accepted.

Next smallest step is one manual Windows report/export to a fresh local file,
then inspect that exact UTF-8 JSON and saved-state UI. Do not rerun these four
helpers unchanged, rebuild unchanged artifacts or replace this gate with mocks.
Continue independent programme work until that desktop interaction is available.
Remote destination/authority and delivery stay separate and open. No phone action,
renderer/music modification, merge, release or publication.
