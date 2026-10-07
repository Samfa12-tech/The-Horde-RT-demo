# Performance direction and generation inputs

Candidate v0.2.1, 7 October 2026. Spoken words, subtitles, line IDs and all 13 opening lines are unchanged. Kit's opening call now has urgent, worried, edge-of-panic acting direction. No audio has been generated or auditioned; recording approval remains false.

## Review and recording workflow

The JSON is authoritative. Every line has human-readable delivery_notes, a delivery_notes_revision and a delivery_notes_sha256. The review DOCX and Markdown show direction separately from dialogue; the TTS manifest and per-speaker CSV carry it alongside the clean text. Direction is a brief for a performer/operator, not text to speak or paste indiscriminately into a model.

TTS-English-en remains exact, tag-free spoken input, matching subtitle_text. The separate TTS-Optional-ElevenLabs-en folder contains only the Kit opening audition example. generation_input is null on all other lines; no automatic tag pass has been applied to the bank. The manifest identifies the optional path and its unauditioned status. Never load the tagged text as subtitles, canonical dialogue or a runtime trigger.

ElevenLabs documents square-bracket audio direction for Eleven v3 and v4. Specific tags and voices need listening tests; these descriptive tags are audition hints, not guaranteed controls. Check the selected model before use, and use clean text plus the separate human brief if it does not support the tags. Do not infer that another model accepts them. Do not add environmental sound effects or reverb to this reusable dry voice input. No generation or spending is authorized by this package.

## Kit opening example

Clean dialogue:

Mate, are you ok? I heard the collapse! The treasure should be just ahead. Be careful!

Human performance direction:

Kit is unseen beyond the small grated WALL access panel. Start urgently, worried and on the edge of panic: 'Mate, are you ok?' is a genuine check for life, with a quick breath and strain in the voice rather than a cheerful greeting or a full scream. 'I heard the collapse!' carries the fear that prompted the call. Ease slightly into practical reassurance on 'The treasure should be just ahead', while still shaken; finish 'Be careful!' with protective urgency. Keep the words intelligible through the grate. Any stronger relief must follow a genuinely audible nearby player movement/footstep cue, never an assumed reply or unseen visual confirmation.

Optional ElevenLabs v3/v4 audition input (unrecorded, not guaranteed):

[urgent, worried, on the edge of panic] Mate, are you ok? I heard the collapse! [trying to reassure, still shaken] The treasure should be just ahead. [protective urgency] Be careful!

The emotional easing here is an attempt to reassure, not proof of a reply. A stronger cue-dependent relief beat must be staged after a genuinely perceivable player sound and tested separately; this single input file does not implement branching or timing.

## Opening staging contract

First safe eligible approach/pass through the small WALL-panel zone, independent of the later waterfall fight and before reward/rescue. Kit remains unseen. An optional nonverbal acknowledgment can be the player's actual nearby movement/footsteps audible to Kit; no new mandatory animation, look-at, stop, spoken reply or acknowledgment gate. Do not react to arrival before it occurs. Concern may soften into reassurance after a perceivable cue; do not invent a sightline or visible Kit. Any visual-confirmation variant requires sightline proof and remains optional. Preserve missed-call/pass-by/backtrack behavior and the exact 1.7 section 7.3 one-shot contract. No extra early 'There you are' line: rescue.found remains the later renewed contact. The later fresh player-controlled lantern raise remains the canonical silent reply. This is conditional authoring direction, not implemented runtime or approved recording.

## Revisions and acceptance

A direction edit increments delivery_notes_revision and changes its hash; re-review any existing take against the new brief even when spoken-text hashes stay unchanged. A spoken-word edit still increments source_text_revision and invalidates translations/takes as before. Optional generation text is model-specific metadata, independently reviewable. Listen for exact words, intelligibility, restrained panic, natural reassurance, tag leakage or added noises before approving any future take. Then measure duration/subtitle timings from the accepted audio; leave timing fields null until then.

Sources checked 7 October 2026:
- https://elevenlabs.io/docs/help-center/product/core-capabilities/text-to-speech/how-do-audio-tags-work-with-eleven-v3-and-v4
- https://elevenlabs.io/docs/overview/capabilities/text-to-speech/best-practices
