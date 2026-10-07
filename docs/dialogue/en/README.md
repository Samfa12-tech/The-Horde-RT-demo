# The Horde campaign dialogue: English

Candidate v0.2.1, 7 October 2026. **357 lines across 209 scenes**, including 125 Kit lines and 164 ordinary-NPC lines. **All recording approvals are false.**

## Read and review

- [Scene-by-scene reading copy](READING-COPY-English-en.md)
- [DOCX reading copy](Horde-Dialogue-Reading-Copy-English-en.docx)
- [Authoritative English JSON](Horde-Dialogue-Bank-English-en.json) and [CSV review/export view](Horde-Dialogue-Bank-English-en.csv)
- [Full authoring and playback contracts](README-English-en.txt)
- [Coverage and open decisions](COVERAGE-AND-OPEN-DECISIONS-English-en.txt), [coverage inventory](COVERAGE-English-en.csv) and [candidate review](INDEPENDENT-REVIEW-English-en.txt)
- [Cast/pronunciation](CAST-AND-PRONUNCIATION-English-en.txt), [state contract](state-contract.json), [schema](dialogue-bank.schema.json), [source provenance](source-manifest.json) and [historical ID reservations](reserved-historical-ids.json)
- [Localization scaffold](localization-manifest.json): English `en`; stable language-independent IDs; no completed translations or invented audio timings
- [Performance direction and optional tagged audition guidance](PERFORMANCE-DIRECTION-English-en.md)
- [Exact per-line TTS inputs](TTS-English-en) and [input/output manifest](tts-manifest-en.csv), including per-speaker combined review files

339 core review candidates, 11 decision-blocked lines and 7 optional final-boss lines are separate. These are text inputs, not generated voice assets. The 13 original opening lines are preserved exactly.

## Kit continuity

The approved general arc is injury during the approach to Bellwether, recovery based in town, and useful partnership through major returns. He gains attachment to the place and remains present through witnessed or evidence-shown conversations. Exact cause/severity and proposed injury wording remain blocked for decision. Baseline finale reunion is in Bellwether, without an unexplained healing or dungeon escort.

This later direction takes precedence over incompatible Kit-location proposals in the [historical v0.1 bank](../../CAMPAIGN_DIALOGUE_BANK.md). [Campaign design](../../CAMPAIGN_DESIGN.md) remains the creative authority. 1.7 remains the reviewed opening/forest/lookout scope; 1.8 and later chapters retain their implementation gates.

## Validation and regeneration

Run from the repository root:

```sh
python3 docs/dialogue/en/tools/validate_bank.py
python3 docs/dialogue/en/tools/export_bank.py docs/dialogue/en/Horde-Dialogue-Bank-English-en.json --output /tmp/horde-dialogue-export
```

The read-only validator checks English tags, IDs, hashes, exact subtitle/spoken/TXT parity, original opening text, recording flags, participant/knowledge metadata and abstract conditions. [Recorded authoring QA](VALIDATION-English-en.json) also distinguishes the original reading-copy review from unrun runtime, listening and device tests. The exporter recreates CSV and TTS inputs without any service. Edit JSON first and regenerate derived views; re-review the DOCX after wording or direction changes. Schema 1.1 adds direction revisions/hashes and nullable model-specific generation input. Clean text revisions are unchanged in v0.2.1.

[Package SHA-256 inventory](PACKAGE-SHA256.json) covers archived files except itself. The reviewed DOCX is stored in Git LFS. Archive-only metadata edits remove private conversation identifiers and update the handoff note. No runtime code, generated audio, paid generation, merge or release is included.

Performance revision 7 October 2026: see PERFORMANCE-DIRECTION-English-en.md for Kit’s urgent opening, optional cue-dependent acknowledgment, per-line human direction and the separate unrecorded ElevenLabs audition example. Clean spoken/subtitle text is unchanged.
