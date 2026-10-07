THE HORDE CAMPAIGN DIALOGUE BANK
English | language tag en | candidate v0.2 | 7 October 2026

WHAT IS LOCKED
Kit is injured during the approach to Bellwether, recovers there, and remains the player's partner through important returns. He is not a dungeon escort. His reason to stay grows from recovery into useful work and attachment to the town. The protagonist has no voiced dialogue. This package is tagged English (en); line IDs remain language independent.

WHAT THIS PACKAGE IS
An expanded campaign authoring and recording-candidate bank, with exact English subtitle and spoken-text parity. It covers the story, town conversations, practical access quests, departures, incomplete returns, side activities and restrained ambient speech. Read the coverage audit for the explicit denominator. A candidate being written is not the same as its scene being approved, implemented or ready for final recording.

The 13 already-delivered opening lines retain their IDs and exact text. 'lockedexisting' means those delivered bytes are preserved; they were previously labelled proposed copy and are not thereby approved performances. 'draft' means new or retained candidate copy. 'newdecisionblocked' means a specific unsettled scene/character/mechanical decision must be settled before recording. Every recording_approved value is false. No audio has been generated or bought. Final-boss speech is optional, separated from the core candidate pack because the owner said 'maybe'.

START HERE
- Horde-Dialogue-Bank-English-en.json is the authoritative authoring source.
- Horde-Dialogue-Bank-English-en.csv is a flat review/export view, generated from JSON.
- Horde-Dialogue-Reading-Copy-English-en.docx is the scene-readable review copy.
- TTS-English-en/core-candidates/<speaker>/<line_id>.txt contains only exact spoken text. It is suitable for pasting into a chosen voice tool, subject to text/casting review.
- TTS-English-en/decision-blocked and optional-final-boss contain excluded candidates. Never batch these into a recording lock automatically.
- Each speaker's combined text contains only spoken text in manifest order. Use the individual TXT files for reliably named per-line generation; generating a combined file will not produce aligned line assets automatically.
- tts-manifest-en.csv maps input file to language, speaker, stable line ID, source revision/hash, and intended output audio path.
- localization-manifest.json is the future-translation scaffold. No translations or timed subtitles have been invented.
- VALIDATION-English-en.json and COVERAGE-English-en.csv separate static authoring checks from outstanding playtests.

STABLE IDENTIFIERS AND ENGLISH
language and source_language are en. An English language tag does not choose an accent or actor. The same line_id is reused in each translation. A change to English source text increments source_text_revision and changes source_text_sha256. Mark dependent translations stale whenever either changes. Delivery notes are separately localizable production notes and never TTS input. Runtime state keys are language-neutral identifiers, never matching English dialogue strings.

Localized speech lives at audio/voice/<language>/<line_id>.wav. Subtitle records key by (language,line_id,source revision). Current audio, measured_duration_ms, start_ms and end_ms are null. Align subtitle timings to each actual recorded take, not to English word counts or an invented SRT. Translations may need different card breaks and durations. Language fallback is requested language -> en; expose an explicit missing-translation indicator during development and preserve the actual spoken/subtitle language relationship in the product policy. Do not silently show an unrelated translated revision over an outdated take.

AUTHORING STATE CONTRACT
Every trigger/flag/location/range in this package is proposed integration, not a claim that a shipped engine API exists. Required flags are AND, forbidden flags are all absent; any_of groups are OR inside each group and AND across groups. These combine with speaker/listener presence, audible range, no combat/hazard warning, current chapter, and stale-event checks at both enqueue and playback. Use explicit world events to set story, inventory and evidence flags. Never set them because a sound finished.

A conversation is an ordered sequence. Reserve all required speakers before starting a multi-speaker exchange. Do not play its answers independently. Allow breathing space. An alternatives scene picks one eligible line, not every line. Mark individual audio filenames and subtitles by their immutable line IDs; speaker filenames must not contain actor names.

PRIORITY AND QUIET
Hazard/attack cues have priority over dialogue. Essential story/tutorial conversation outranks optional conversation, which outranks ambient. Suppress and duck ambient when an important line starts; never cover a critical telegraph with a boss joke. Suppress dialogue during urgent drowning/smoke cues. No forced camera, movement lock, automatic answer or speech-only quest gate.

At least 90 seconds between optional barks globally, at most two optional barks in any five-minute exploration window, no immediate repeat. Ambient conversations use at least 180 seconds between exchanges and no scene repeat in the same hub visit; default once per save for this candidate set. Exact distances (conversation 4 m, close ambient 6 m, grate special anchor) are tuning candidates, not measured acoustic guarantees. Check occlusion/intelligibility in actual scenes. Hints require a relevant stuck state for at least 25 seconds and reset on meaningful progress.

PERSISTENCE AND INTERRUPTIONS
Default once per save/campaign, not once per load. Persist an event ledger independently of scene residency. A deliberate new campaign resets it. Save started/consumed state atomically to stop reload replay farming. Optional scenes consume on start/skip/interruption; cancel the rest if participants leave. Critical scenes consume their spontaneous invitation on start but keep source-appropriate evidence, objective and a player-requested recap available after interruption. A later requested recap starts at an intelligible sentence, never halfway through an audio file. Do not indefinitely auto-retry dialogue.

On death/reload, revalidate all queued conditions and location; discard stale queued lines. Restore current world state first. Tactical boss barks may be explicitly once per attempt, capped at three short optional lines total and never replaying the story reveal. Repeated boss attempts cannot duplicate seals or relic rewards. A story scene skipped before playback does not grant facts through audio; the same world interaction must supply the nonvoiced information.

KNOWLEDGE AND SPOILERS
Kit only reacts to what he witnessed, heard directly, or was shown in Bellwether. A silent player cannot deliver an invisible spoken report. Showing an object/record or replaying a direct lantern exchange is an explicit player-controlled action. He does not know the shrine voice remotely and never speaks in a dungeon in the baseline.

Before the Court truth, the player-facing voice label is Lantern voice. It is never labelled Entity or confirmed King Veyr merely because it claims that identity. Author files contain spoilers; do not expose filenames, raw state names, cast lists, translation notes or future subtitles in early UI. The Court truth and ritual risk must be recoverable from physical evidence even with every optional clue missed. Garden-date evidence and the king's exact order are scene proposals, not newly approved historical minutiae.

GAMEPLAY BOUNDARIES
Abbey and Foundry work in either order. An Abbey line cannot require the Foundry's local tool, or vice versa. Court requires both seals plus the authored access interaction. The reflector, shuttered stand and focusing aperture are distinct approved local tools; each must be taught safely and actually used to defeat its own boss. Spoken copy does not prove the puzzles work. Recovery, recall, air pockets, finite wards/equipment/breath and retreat require playable prototypes. Side quests and relic restitution never replace a seal or trap progress through irreversible spending.

FINAL RECORDING CHECKLIST
Review candidate wording and blocked decisions; select voices and pronunciation; verify source/performance rights; audition clean dry takes without baked-in room reverb or heavy mask filtering; record per-line WAVs; listen for exact text and intelligibility; record sample rate/channels/duration/rights and take version; align each language's subtitle cards; implement and playtest triggers in both quest orders, retries, skips, reloads, stale queues, and no-optional-help ending. No runtime, device, performance or gameplay acceptance is claimed by this authoring package.

LINKED EXCHANGES AND SILENT INTERACTIONS
The army/hoard setup and response share arrival.army_and_hoard. The in-town lantern introduction and Kit reaction share hub.lantern_introduction_to_kit. Use conversation_line_order across their component scenes; reserve Kit as a listener in range, and exempt linked continuations from unrelated-scene cooldowns. Do not let a new knowledge flag set by the first line invalidate the reserved reply. Still cancel if the player leaves, an actor disappears or a hazard starts. A canceled optional answer need not be forced later.

Showing a route, placing a record on the table, presenting a seal, demonstrating a tool, asking to review a record, previewing a sale, accepting a quest, and closing a menu are silent player actions. Their UI labels and resulting state changes are not player voice lines. A missing-item report means the player elects to review the displayed expedition state; Kit must not infer an unseen search outcome merely from proximity.

REGENERATING DERIVED FILES
Use tools/export_bank.py with the authoritative JSON and a fresh output directory. It uses Python 3's standard library, validates IDs/text hashes/parity, and recreates the CSV and exact TTS inputs without calling any service. It never generates audio or edits runtime code. The JSON is the single editing source; regenerate derived files rather than maintaining competing text copies. After text edits, increment the relevant source revision/hash and re-review the reading copy, cast notes, eligibility and translations.
