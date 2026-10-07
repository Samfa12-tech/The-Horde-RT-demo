# The Horde — Whole-Campaign Dialogue Bank

> **English bank update, 7 October 2026:** [Candidate v0.2](dialogue/en/README.md) contains the expanded 357-line English source, readable copies, exact TTS text and authoring contracts. The approved Kit arc is injury on the approach to Bellwether, recovery based in town and continued partnership through major returns; exact injury details and dialogue wording remain for review. This supersedes incompatible older Kit-location proposals, including a baseline dungeon/treasury escort. All recording approvals remain false; milestone and runtime scope are unchanged.

**DRAFT v0.1 · 3 October 2026 · authoring proposal, not recording copy**

A playable story spine with representative dialogue from the tomb to the return home. The opening is written more tightly for 1.7; later chapters are broad-strokes scene proposals to test before expanding. This is a selective bank, not every conversation, a locked line count, or approval to produce voices.

**Creative authority:** [Campaign design](CAMPAIGN_DESIGN.md). **Implementation gates:** [Roadmap](ROADMAP.md), [1.7 plan](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md), [provisional hub plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md). Inspected planning baseline: `65b3454922626bdb9bac1360eb6980cb376ebc18`. Later approved directions take precedence.

## 1. What is fixed, and what this draft proposes

**Established canon:** silent player; practical, loyal, opinionated Kit; cursed fourth Keeper and two sworn skeleton guards protect the lantern before the player acquires it; rope rescue and moonlit forest; a useful village; Abbey and Foundry in either order, then Glass Court; three seals; a deceptive entity claiming to be the dead king; real plundered treasure; a release ritual that truly frees the bound dead while also releasing the entity; defeat and one hopeful, playable return.

**Proposed connective tissue throughout this bank:** all new wording, individual clue objects, exact service exchanges, first lantern-speech timing, named-item claimant, side-expedition details, Glass Court confrontation staging, ritual explanations and epilogue scenes. These illustrate the canon; they do not amend it. Bellwether's church-bell name origin and unpulled-bell folklore are approved setting lore in WORLD_LAYOUT.md; specific spoken lines, supernatural explanations and runtime bell events are not thereby approved. The Veyrlands, Bellwether and the real historical King Veyr are approved names (3 October 2026); the lantern's claim to be that king remains deceptive. Brother Ansel, Hester, Bellkeeper and Master of Coin remain working names. Use role-based IDs until names are approved. The entity's true name/form and the royal keeper's fate are still open.

The fourth Keeper's spoken lines below are **future campaign dialogue candidates**, not additions to the active 1.6.2 reveal/engine-polish goal. That milestone's approved sound/reveal brief remains its authority. The masked Briarhold Warden remains a Kit asset candidate pending rights/import/rig/performance validation, not final casting or appearance.

**Milestone boundary:** 1.7 uses its reviewed Kit rescue/forest subset only. First lantern speech is proposed for the later hub chapter, avoiding an unscoped second speaking character in 1.7. Later dungeons/finale are not 1.7 or 1.8 deliverables. No runtime edit, asset transfer, audio generation, paid work, merge or release is requested by this bank.

**Geography approved separately, 3 October 2026:** Follow [WORLD_LAYOUT.md](WORLD_LAYOUT.md). The tomb is in the northwest abandoned burial ground, separate from the village churchyard. The 1.7 forest ends at the lookout overlooking the low-detail, fixed-position future hub; 1.8 extends the same approach road. Shrine, downstream Abbey, eastern Foundry and northeastern high-ridge Court/treasury retain the approved route relationships and gates. This locks location/route continuity, not this bank's proposed spoken wording or new scene details.

## 2. Voice, staging and subtitle rules

| Speaker ID | Voice guide | Avoid |
|---|---|---|
| `kit` | Warm, capable, a little breathless after effort. Practical observations; occasional dry joke; concern comes before profit. Owner's English fantasy direction; light northern English/Yorkshire colouring is an audition suggestion, not selected casting. | Constant quips, faux-medieval grammar, explaining every puzzle, inventing what the player thinks. |
| `lantern` | As “king”: measured, tired, attentive, helpful. Earn trust through accurate practical advice and one costly admission. After release: the same voice, less restraint; possession becomes explicit. | Early villain snarl, compulsive obvious lies, real-person imitation, a separate randomly evil personality. |
| `tavern_keeper` | Former caravan leader. Clear decisions, remembers people and roads. Welcomes useful help without worshipping treasure hunters. | An exposition bartender with no stake in events. |
| `record_keeper` | Careful, curious; distinguishes evidence from guesses. Can correct an earlier reading. Brother Ansel is provisional. | Omniscience, lectures, knowing the twist on arrival. |
| `artificer` | Specific materials, mechanisms and repair limits. Hester is provisional. | Modern corporate jargon or miraculous gadgets. |
| `salvager` | Patient physical instruction, respect for water and exhaustion. | Treating Constitution as invulnerability. |
| `relic_dealer` | Knows prices and provenance; ordinary trade can be fair without settling ownership of every named relic. | Automatically villainous greed or a compulsory moral lecture. |
| `fourth_keeper` | Exhausted duty, fraying control. Sparse intelligible words among separately authored creature sound. | Naming the entity or prison twist before the player has a reason to suspect it. |
| `bellkeeper` | Ritual duty worn into habit; few words. | Random aggression without a place or responsibility. |
| `master_of_coin` | Counts obligations as if people were entries. Precise, brittle authority. | Comedy bookkeeping that destroys the threat. |
| `court_keeper` | Lucid, proud, exhausted by custody. Evidence before accusation. | A perfectly innocent lecturer who could solve everything by speaking sooner. |

No player spoken lines, voiced choice paraphrases or subtitles pretending to be the player's thoughts. Selecting a service, placing an object, raising the lantern or entering a route supplies the response. Kit may disagree with an observed action without declaring the player's motives.

For a masked Kit, use a held breath, tilt of the head, weight shift and hand gesture. No requirement for exposed-mouth lip-sync. Do not bake heavy mask filtering or tomb reverb into the source take; intelligibility comes first.

## 3. How to read and implement the bank later

Every table row is one proposed spoken line unless marked **EVENT**. IDs are stable; change text/revision rather than renaming an ID. Scene IDs are authoring labels, not shipped flags. The trigger column describes intended authoritative state, not an existing engine API.

- **C:** critical-path information; ensure the objective, observed event or a short accessible recap preserves it if audio is skipped or fails. C does not force the player to listen.
- **O:** optional character/lore/reaction; never gates progress.
- **H:** delayed hint; one per puzzle stage, at least 25 seconds without progress and only when safe/in range. Reset delay on meaningful progress; no unsolicited full solution.
- **E:** silent event; no voice file or player speech.
- Default **once per campaign checkpoint history**; explicit skip/interruption consumes started optional speech. Fresh run resets it. Important unreceived information remains available via an interaction/recap, not repeated unsolicited playback.
- No speech-only locks, forced cameras or look-at requirements. World interactions complete through simulation state, never audio callbacks. Keep controls and enemy cues available.
- Critical conversation > optional banter; combat/hazard cues interrupt both when needed. Cancel stale queued speech at zone exit. Do not replay a dungeon-intro line halfway through its boss.
- Default non-combat exchanges have breathing room between thoughts and long silent walking stretches. Only Kit speaks when physically present/in audible range; no telepathic companion. Most dungeon lines therefore belong to the lantern, local inhabitants or later hub conversations.
- Subtitles carry speaker identity, scalable text and top-safe-area placement on mobile. Independent Dialogue/Music/SFX controls and offline playback follow the 1.7 plan. Before the reveal, subtitle the entity **Lantern voice** or **Voice**, never “Entity”; a claimed royal identity must not be presented as verified fact.
- A production manifest will add: `line_id, text_revision, speaker_id, scene_id, subtitle, spoiler_stage, start_condition, cancel_condition, priority, consumed_flag, repeat_policy, gesture, audio_asset=null, source_rights=null, measured_duration=null, locale=en-GB, status=DRAFT`. Null audio/rights fields mean **not recorded or cleared**. Split long subtitle cards on natural clauses while preserving the utterance ID.
- No prices, upgrade durations, breath capacity, NPC population or voice-production costs are fixed here.

## 4. Opening: tomb, rescue and forest (1.7 focus)

**Beat:** The guards are defending a charge. Kit's unseen concern makes the later rescue personal. The player earns the lantern, climbs under their own control and shows it without speaking. Let relief turn to wonder before the first clue.

**Location and sequence clarification, 3 October 2026:** Kit's early unseen call comes through the small grated **wall access panel just to the right outside the opening room**, where the intended overgrowth belongs. It does not come from either skylight or depend on defeating the two guards, whose relocation to the waterfall room is now queued for post-reset 1.6.2 under the 5 October owner direction. Kit's voice/trigger itself remains 1.7. The queued post-run 1.6.2 visual change closes the entry-room skylight and leaves the waterfall's own hole/vines untouched, and adds an impassable iron grid to the separate large skylight in that room. Preserve the distinct later-created finale opening for rope rescue. Kit does not know about the lich and is not knowingly waiting for its defeat.

The grate wording preserves the owner's suggested draft. Other retained IDs below refine the earlier provisional copy: notably renewed contact at the rescue, and a practical destination after reunion. These are proposals for review, not approved performance text.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `prologue.kit_grate` | Kit; unseen beyond the small wall access panel, genuine concern | Mate, are you ok? I heard the collapse! The treasure should be just ahead. Be careful! | O; first safe eligible approach to the small wall panel, independent of the later waterfall skeleton encounter, before reward/rescue; exact one-shot contract in 1.7 §7.3. |
| `prologue.keeper_warning` | Fourth Keeper; spent, controlled | Leave this chamber. My watch is not ended. | O; future campaign candidate at awakening; not part of 1.7 Kit delivery or active 1.6.2 scope. |
| `prologue.keeper_last` | Fourth Keeper; fading effort | I cannot... leave my post. | O; future campaign candidate on defeat, before reward; do not delay existing reward logic. |
| `rescue.found` | Kit; relieved renewed contact | There you are. Still in one piece? | C; roof sufficiently open and Kit positioned above. |
| `rescue.rope` | Kit; practical, no joke | Stay clear. Rope coming down. | C; fixed-anchor deployment ready during lantern pickup; one-time runtime event places rope independently of Kit's idle, not a hand-release marker or audio completion. See [animation plan](KIT_ANIMATION_PLAN_1_7.md). |
| `rescue.climb` | EVENT; player | Player takes the rope and climbs. | E; valid traversal input; no automatic ascent or voiced answer. |
| `reunion.question` | Kit; first checking player, then curious | Steady. Catch your breath. Did you find it? | C; safe summit/reunion mark; settle before line. |
| `reunion.hint` | Kit; gentle, not impatient | Let me see. Raise it. | H; generous idle delay during raise lesson; once. |
| `reunion.answer` | EVENT; player | Fresh manual raise presents the lantern. | E; reject held-over input; reaction waits for visible presented pose. |
| `reunion.proof` | Kit; quiet wonder | Then we're not chasing a story anymore. | C; lantern visibly presented, once. |
| `reunion.first_piece` | Kit; thoughtful | A start, then. Let's see where it leads. | C; after reaction; no invented knowledge of seals. |
| `reunion.depart` | Kit; warmth returning | Come on. There's a fire and a dry seat waiting in the village. | C; exchange complete; route-leading can begin independently of voice end. |
| `forest.night` | Kit; low, taking in the woods | I'd forgotten how big the sky was. | O; first open moonlit view, after quiet walking; no later lore overlap. |
| `forest.waystone` | Kit; notices a real change | Hold it there. There are marks under the moss. | C; authored inscription genuinely revealed by lantern-light interaction. |
| `forest.clue` | Kit; curious, not certain | A road to the treasury, perhaps. Someone in the village might read it. | C; inscription observed; carry objective even if line skipped. |
| `forest.wait` | Kit; nearby, no pressure | I'll wait here. | O; first authored wait point when player explores; once in this chapter. |
| `forest.village` | Kit; relieved | There. Chimney smoke. | C; 1.7 lookout endpoint overlooking the distant village shell; no promise of entering unbuilt content. |

**Clue proposal F1:** the lantern reveals a treasury-route mark beneath weathering. It establishes useful light, not the entity's identity. Exact symbol/text remains art/puzzle authoring. Leave forest sound and moonlight room to work; do not turn every path marker into dialogue.

**Opening safeguards:** the grate beat is optional and missable; rescue lines must work with it heard or unheard. No lantern voice, prison speech or lantern ownership before lich defeat. 1.7 ends at the lookout under WORLD_LAYOUT.md; use that scoped chapter endpoint; the invitation to the village does not itself enable 1.8.

## 5. Village arrival, first lead and first small expedition

**Beat:** The road mattered before the player arrived. Treasure can improve ordinary lives. The lantern offers enough practical truth to deserve attention. Proposed first voice occurs at a quiet inspection after the tavern introduction, not a surprise extra 1.7 scene.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `hub.arrival` | Tavern keeper; brisk welcome | Shut the door behind you. Both of you look frozen. | O; first tavern entry; door action not required to hear next objective. |
| `hub.kit_introduce` | Kit; modest pride | The tomb was there. So was something worth the climb. | C; first introduction, lantern owned. |
| `hub.horde_army` | Tavern keeper; matter-of-fact | My grandfather called the Horde an army. You treasure hunters always hear the other word. | C; first expedition story. |
| `hub.hoard_answer` | Kit; lightly defensive | Armies need paying. I'm hoping we're both right. | O; after army line. |
| `hub.road_stake` | Tavern keeper; personal, plain | I used to bring six wagons through here. Last winter we had one. | O; proposed local stakes; numbers are story copy, not economy simulation. |
| `hub.record_inspect` | Record keeper; focused | Bring the light closer. This mark belongs to the old treasury road. | C; voluntary inscription/lantern inspection. |
| `hub.first_errand` | Record keeper; useful lead | The wayside shrine kept a road register. If it survived, it may name the next stops. | C; propose small expedition, not a full dungeon. |
| `lantern.first_help` | Lantern voice; quiet urgency | The lower stone. Its hinge is hidden under the roots. | C; shrine mechanism inspected; advice must be physically true. |
| `lantern.first_identity` | Lantern voice; restrained vulnerability | I was king here. My keepers sealed me away. | C; after helpful action; a claim, not authorial confirmation. |
| `lantern.first_request` | Lantern voice; controlled | Three seals guard the treasury. Find them, and I can put an end to this. | C; first lead; “this” later clarified as bound service. |
| `kit.voice_reaction` | Kit; unsettled, practical | Next time, a little warning before the lamp starts talking. | O; Kit present at first voice; never replay remotely. |
| `shrine.register` | Record keeper; reading returned evidence | The Abbey. The royal Foundry. Then the Glass Court. | C; register recovered and inspected in hub. |
| `hub.two_routes` | Record keeper; clarifies choice | We can trace the first two roads. The Court's route is still sealed. | C; show both leads; no prescribed Abbey/Foundry order. |
| `hub.kit_stake` | Kit; candid | I'd like a roof that doesn't fall in. Treasure still seems a fair way to get one. | O; quiet first-return talk; personal want without speaking for player. |
| `hub.ordinary_sale` | Relic dealer; fair business | Common coin, ordinary silver. I can give you a price for those. | O; ordinary sale preview, never auto-sell. |
| `hub.named_relic` | Relic dealer; observant | That one's marked. Find whose mark it is before you decide. | O; named item inspected; no compulsory sale/return choice. |

**First expedition proposal:** recover the register from a short wayside-shrine route; optional restoration there may contribute to the Magic access quest. It must be playable with starting equipment. A missing optional component or prior treasure sale cannot strand the player. Do not require every service quest before entering either middle dungeon.

## 6. Useful hub: Magic, Tech and Constitution access

**Beat:** Each route comes from a person's knowledge and a small playable lesson. Upgrade labels describe distinct benefits, not exclusive classes. Service copy must reflect the actual implemented limits. A mixed build remains viable.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `access.abbey_problem` | Salvager; sober | The Abbey door is underwater. There are air pockets beyond, if you can reach them. | C; Abbey lead selected. |
| `access.magic_offer` | Record keeper; measured confidence | Restore the shrine's binding, and I can teach you a ward for the crossing. | C; Magic access option inspected. |
| `access.magic_limit` | Record keeper; explicit | It will buy you breath. It will not last forever. | C; before ward lesson/use. |
| `access.tech_offer` | Artificer; practical | The mill has a sound air chamber. Help repair its gearing; I'll fit you a breathing rig. | C; proposed Tech mini-quest, not final recipe. |
| `access.tech_test` | Artificer; insists on proof | Shallow water first. We find the leaks here. | C; before safe rig trial. |
| `access.body_offer` | Salvager; calm | I can teach you the crossing. Slow your breathing. Learn where you can surface. | C; Constitution option inspected. |
| `access.body_limit` | Salvager; firm | Being strong won't make water into air. Turn back while you still can. | C; safe breath-control lesson. |
| `access.abbey_ready` | Salvager; satisfied | You've made the practice crossing. Use the same care at the Abbey. | C; whichever valid route passed; route-neutral recap. |
| `access.foundry_problem` | Tavern keeper; route knowledge | The Foundry bridge has gone. The old maintenance path may still reach the far side. | C; Foundry lead selected. |
| `access.bridge_tech` | Artificer; assesses remains | The anchors held. With the right fittings, that span can carry you again. | C; Tech bridge-repair option. |
| `access.crossing_magic` | Record keeper; inspects stones | Those stones carried a crossing charm. We can restore it. | C; Magic stepping-stone option. |
| `access.climb_body` | Salvager; points out holds | Use the old maintenance climb. Test each hold before you put your weight on it. | C; Constitution route, after appropriate lesson. |
| `access.smoke_tech` | Artificer; honest limits | This filter will help with the smoke. Get clear before it clogs. | C; filter acquired, if selected. |
| `access.smoke_magic` | Record keeper; clear instruction | Keep the ward steady through the smoke. Find clean air before it fades. | C; protection acquired, if selected. |
| `access.smoke_body` | Salvager; serious | Take the short gaps between clear air. If your chest tightens, go back. | C; endurance preparation selected; no immunity promise. |
| `hub.mill_repaired` | Tavern keeper; pleased | The mill's turning again. We can grind here instead of sending flour over the ridge. | O; only if mill repair actually complete and visible. |
| `hub.service_return` | Artificer; familiar welcome | Back already? Put it on the bench. Let's see what survived. | O; first later equipment visit, once per completed expedition. |

**Design gate:** Abbey air-pocket spacing and hazard feedback must make each approved access route credible. Foundry access and smoke preparation are related but not identical; provide a viable starter-safe hazard path or a recoverable preparation opportunity. Dialogue cannot imply a sold item, exclusive class or expensive upgrade is the only solution.

## 7. Drowned Abbey (either first or second)

**Beat:** Dead attendants still carry out duties in flooded rooms. A local reflector teaches deliberate light placement. The reward includes a seal and evidence of plunder. The lantern admits real wrongdoing to strengthen a false identity.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `abbey.threshold` | Lantern voice; remembers function | The bells called the road caravans to shelter. | O; first safe interior air pocket; no speech while breath is urgent. |
| `abbey.attendants` | Lantern voice; low | They still tend the altar. Give them room. | O; player observes duty loop; behaviour must support statement. |
| `abbey.reflector` | Lantern voice; useful | That plate was made to turn light into the side chapel. | C; reflector acquired/inspected; local tutorial remains visual/playable. |
| `abbey.reflector_hint` | Lantern voice; restrained | The shutter blocks it. Try the light from the other side. | H; relevant stuck puzzle stage only. |
| `abbey.claim_mark` | Lantern voice; a pause before admission | That silver came from the river settlements. We called it tribute. | C; proposed marked votive vessel inspected, critical plunder evidence. |
| `abbey.admission` | Lantern voice; without self-pity | They did not give it willingly. | C; same scene; admission must land without immediate joke. |
| `abbey.bellkeeper_warning` | Bellkeeper; ritual authority | The sanctuary is closed. | C; boss threshold; reflects protective duty. |
| `abbey.bellkeeper_fight` | Bellkeeper; strained command | Keep the light from the altar. | O; first relevant boss state, once; do not obscure attack cue. |
| `abbey.boss_hint` | Lantern voice; concise | Open the shutter. Let the reflected light reach him. | H; only after mechanic taught and repeated failure; boss form/pronoun provisional. |
| `abbey.seal` | Lantern voice; almost relieved | The Abbey seal. Take it carefully. | C; seal available after encounter, independent of dungeon order. |
| `abbey.release_question` | Lantern voice; persuasive truth | Their service should have ended with their lives. The treasury holds the bond. | C; after seal, in safety; does not yet explain prison. |
| `abbey.kit_return` | Kit; sees wet gear | You brought half the river home. Anything worth drying? | O; hub return with Kit present. |
| `abbey.claimant` | Tavern keeper; recognises family mark | My mother's people made that mark. They were told the river took their silver. | C; proposed living claimant for named vessel; inspect without forcing surrender. |
| `abbey.return_relic` | Tavern keeper; quietly affected | Thank you. I'll put it where people can see it. | O; only after voluntary return; named object visibly retained in hub. |
| `abbey.keep_relic` | Tavern keeper; level, no accusation | You know where it came from now. | O; player closes inspection without returning; no inferred motive or automatic penalty. |

If the vessel was sold before its history was known, preserve access to its evidence and an attainable recovery/return route. Do not make a restitution choice a seal gate. The exact vessel, family connection and recovery rules remain proposals.

## 8. Ashen Foundry (either first or second)

**Beat:** A royal mint treats service as debt. Workers keep machines running under compulsion. The local shuttered stand teaches leaving light in place and controlling exposure. The seal and binding record stand on their own regardless of Abbey progress.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `foundry.threshold` | Lantern voice; precise familiarity | This floor cast the army's pay. The furnaces below made its weapons. | O; first safe overlook. |
| `foundry.workers` | Lantern voice; understated | The shift never ended. | C; observe bound workers maintaining machinery. |
| `foundry.stand` | Lantern voice; practical | Set the lantern in the stand. The shutter will hold back its light. | C; local tool lesson, before dangerous application. |
| `foundry.shadow_hint` | Lantern voice; quiet | That watcher follows the light. Close the shutter before you cross. | H; relevant learned exposure/cover puzzle only. |
| `foundry.bond_record` | Lantern voice; reads too readily | Service until the last campaign is ended. That was the oath. | C; binding record inspected; military precision is a fair clue. |
| `foundry.blame` | Lantern voice; contained resentment | My keepers kept the bond alive. Every soldier is still paying for it. | C; manipulative account, not confirmed narration. |
| `foundry.mintmaster_warning` | Master of Coin; formal | No one leaves with the Crown's property. | C; boss threshold; motivates custody. |
| `foundry.mintmaster_command` | Master of Coin; brittle anger | Back to your station! | O; first boss phase change, once. |
| `foundry.boss_hint` | Lantern voice; clipped | Close the shutter. Move while its search passes. | H; taught light/cover state, exact mechanic pending prototype. |
| `foundry.seal` | Lantern voice; urgent beneath restraint | Take the seal. We have kept them waiting long enough. | C; safe reward scene; no order-specific count. |
| `foundry.kit_return` | Kit; dry humour, concern underneath | You smell like the forge. Sit down before you fall down. | O; hub return, Kit present. |
| `foundry.record_review` | Record keeper; disturbed | The names continue after the dates of death. These were people, not an endless supply. | C; examine recovered record. |
| `foundry.component` | Artificer; sees practical value | These fittings could put a wagon back on the road. If you can spare them. | O; optional recovered component; never consumes seal or progression tool. |
| `foundry.trade_returns` | Tavern keeper; modest hope | A wagon came through this morning. We unloaded it together. | O; only after actual road/trade restoration, not dungeon completion alone. |

## 9. Returns and the two-seal convergence

**Beat:** Both middle dungeons work independently. The hub joins evidence without withholding essentials from either order. Do not key story correctness to a fragile “first dungeon” string.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `return.one_seal` | Record keeper; assesses progress | One seal. The other road may tell us what this opens. | C; first seal acquired, exactly one owned; once. |
| `return.abbey_remaining` | Record keeper; practical | The Abbey still holds its seal. Prepare for the submerged approach. | C; Foundry done, Abbey not done; only relevant lead. |
| `return.foundry_remaining` | Record keeper; practical | The Foundry still holds its seal. You'll need a way across the broken bridge. | C; Abbey done, Foundry not done; only relevant lead. |
| `return.two_seals` | Record keeper; putting evidence together | Both seals fit the route mark. We can find the Glass Court now. | C; both acquired and route interaction completed. |
| `return.keeper_word` | Record keeper; careful correction | I read this as keeper of the treasury. Here, the same word means jailer. | C; proposed paired inscription evidence, before Glass Court. |
| `return.kit_doubt` | Kit; private unease | It knows every lock. Has it told us much about the people who lived behind them? | O; both dungeons complete; lantern not answering over the line. |
| `return.voice_reply` | Lantern voice; wounded dignity | I remember what was done here. I would rather you saw it than took my word. | O; deliberate deflection after evidence scene; no certainty of innocence. |
| `return.kit_enough` | Kit; sincere, a little reluctant | We've found more than I ever expected. I'd still like us both here to spend it. | O; before Court departure; player retains intent. |

**State matrix:** Abbey→Foundry uses Abbey evidence/return, one-seal recap, Foundry lead, Foundry evidence/return, then two-seal convergence. Foundry→Abbey uses the reciprocal order. Never play both remaining-road lines. Suppress one-seal recap after both seals even if its queued audio never played. Acquire/inspect evidence flags independently from speech completion; short hub recaps remain available. Optional tool shortcuts do not become prerequisites.

## 10. Glass Court — warning with evidence

**FULL STORY SPOILERS FROM HERE.** Keep author-facing sections and internal true identities out of early subtitles, quest names, voice filenames exposed to players and public chapter previews.

**Beat:** A beautiful ruined palace makes preservation understandable. A lucid keeper recognises the prison. This draft proposes a confrontation that can include combat, then a surviving interval to present evidence; the final staging and keeper fate require approval. The player must learn the warning before choosing to approach the treasury.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `court.garden` | Lantern voice; a rehearsed softness | I used to walk here after the snow. | O; winter garden; proposed false personal memory. |
| `court.memory_evidence` | Court keeper; specific correction | That garden was planted after the king died. | C; encounter evidence, pays off garden line; also readable in dated record if line missed. |
| `court.recognition` | Court keeper; alarm held in check | Put that lantern down. Keep its shutter closed. | C; keeper recognises carried prison; do not auto-drop inventory. |
| `court.voice_accuses` | Lantern voice; familiar appeal | Another keeper. You know what their mercy looks like. | C; manipulation draws on actual suffering. |
| `court.king_proof` | Court keeper; directs attention | Read the king's order. His seal is beside ours. He ordered the prison made. | C; physical evidence accessible before proceeding. |
| `court.true_bargain` | Court keeper; plain account | He bargained for victory. It gave him an army that could not stop serving. | C; reveal supported by earlier binding record. |
| `court.fourth_keeper` | Court keeper; grief beneath anger | There were four of us. The one beneath the tomb kept watch over what you carry. | C; recontextualises lich and guards without undoing combat. |
| `court.custody` | Court keeper; admits cost | We held it. We could not release the soldiers without opening the bond. | C; proposed precise dilemma; no invented separate curse cure. |
| `court.focus_tool` | Court keeper; practical instruction | Narrow the light. Follow the line through the glass. | C; focused-aperture candidate lesson, before finale. |
| `court.final_seal` | Court keeper; grave | The last seal leads to the treasury. You have seen what it keeps. | C; final seal/location earned; exact fight/transfer staging open. |
| `court.ritual_truth` | Lantern voice; abandons one evasion | Open the bond and the soldiers go free. So do I. | C; explicit risk before treasury ritual; no obscure forced mistake. |
| `court.kit_response` | Kit; steady, worried | Then we find a way to face it before we open anything. | C; later debrief with Kit physically present, not distant dungeon speech. |
| `court.preparation` | Record keeper; practical | Bring what you've learned. The wards and shutters were built to hold its light. | C; hub preparation; any actual containment/counterplay must be taught and tested. |

The garden detail and signed order are **proposed evidence**, not new settled history. If garden line is missed, the king's order and binding record independently establish the contradiction. The keeper may be compromised and defensive, but must not be made inexplicably mute merely to prolong the twist.

## 11. Treasury and final confrontation

**Beat:** Deliver the promised riches. The deception was royal identity, ownership and a claim of harmless restoration. After the Court, the good objective is still real: freeing the soldiers. The player prepares for a known dangerous release rather than ignoring an explicit warning. One ending; preparation can vary, outcome structure does not.

**Proposed ritual staging:** treasury interaction demonstrates the dead are bound to the same ward system holding the entity. The player inspects the bond, readies established light tools and explicitly starts the ritual through an authored world action. This is a design proposal to prototype; no new mandatory last-minute spell, secret counter-seal or unannounced ability.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `treasury.first_sight` | Kit; awed, subdued | It really is here. All this time. | O; proposed Kit presence at safe vault threshold only; no escort requirement. |
| `treasury.recognition` | Kit; recognises earlier craft | That's the same mark as the Abbey silver. | O; only if marked vessel evidence known; independent fallback via visual objects. |
| `treasury.voice_promise` | Lantern voice; possessive warmth | Every banner brought something home. I kept it all. | C; first full treasury view; royal disguise visibly frays. |
| `treasury.bound_dead` | Lantern voice; unadorned fact | Break the bond. Their watch ends here. | C; inspect ritual locus; actual objective corroborated by evidence. |
| `treasury.kit_prepare` | Kit; practical courage | Check your light. Check the way back. I'll keep this passage clear. | C; proposed staging places Kit safely outside boss space. |
| `treasury.ritual_ready` | EVENT; player | Inspect the bond and ready learned tools; deliberately activate the release. | E; player-controlled action, not automatic on proximity or line completion. |
| `treasury.release` | Lantern voice; first unguarded pleasure | At last. | C; release event; show soldiers' bonds genuinely ending. |
| `treasury.entity_claim` | Lantern voice; control exposed | I gave them victory. Their king promised me everything that followed. | C; freed entity, final identity confirmed by action/evidence. |
| `treasury.entity_demand` | Lantern voice; cold entitlement | Put down the light. You have carried it far enough. | C; no enforced player compliance. |
| `finale.entity_pressure` | Lantern voice; anger, not a joke | You would leave this here? | O; once in first appropriate combat lull. |
| `finale.entity_light` | Lantern voice; genuine alarm | Close that shutter! | O; once when player successfully applies learned counterplay; never substitutes for visual feedback. |
| `finale.entity_last` | Lantern voice; control failing | It was promised to me. | O; defeat, before silence; form/staging TBD. |
| `finale.bond_ended` | EVENT; world | Entity defeated; bound soldiers released; vault treasure remains reachable. | E; single canonical outcome, save atomically with rewards/progression. |
| `finale.kit_alive` | Kit; relief before riches | There you are. Come on. One step at a time. | C; safe reunion after combat; no automatic player injury asserted. |
| `finale.kit_treasure` | Kit; exhausted dry humour | We'll need a bigger bag. Later. | O; after a quiet moment; treasure is genuinely available to leave. |

No fight bark prescribes an unapproved boss solution. The precise interactions must emerge from the accepted reflector, shuttered stand and focused aperture prototypes. The bank describes narrative function; it is not evidence that all three mechanics are implemented.

## 12. Playable return and one ending

**Beat:** The village welcomes living people, not merely a loot total. The world remembers tangible help. No compulsory surrender of all treasure and no alternate keeper ending. Distribution and losses remain open.

| ID | Speaker / direction | Proposed text | Type / trigger |
|---|---|---|---|
| `epilogue.welcome` | Tavern keeper; relieved | You're back. Sit down. I'll find something hot. | C; return after entity defeat. |
| `epilogue.record` | Record keeper; gently decisive | We'll write their names as people. Their service is over. | O; recovered records available. |
| `epilogue.mill` | Artificer; ordinary satisfaction | The mill's still turning. I thought you'd like to hear that. | O; repair flag and visible working mill only. |
| `epilogue.relic` | Tavern keeper; quietly proud | People keep stopping to look at it. Some remember the mark. | O; named vessel actually returned and displayed. |
| `epilogue.trade` | Tavern keeper; future in small terms | Another wagon is due tomorrow. We might need to clear the spare room. | O; restored-trade state only; no simulation promise. |
| `epilogue.salvager` | Salvager; affectionate understatement | Good. You remembered to come back up. | O; breath-control training taken. |
| `epilogue.kit_roof` | Kit; pleased, tentative | I asked about the room upstairs. Roof looks sound. | O; callback to personal want; proposed lodging, no automatic spending. |
| `epilogue.kit_close` | Kit; companionable, let it rest | The rest can wait till morning. | C; quiet optional-to-hear closing beat; playable village remains. |

Provide neutral welcome/closure even if no optional repair or restitution was done. Do not announce a repaired mill, saved family or reopened route without the actual corresponding world change. Cosmetic dialogue alone does not meet the useful-hub requirement.

## 13. Exploration, reactions and combat barks

These supplement authored scenes rather than filling every silence. Suggested bounds are tuning starting points: maximum one ambient bark per 90 seconds, no immediate same-line repetition, at most two optional barks in a five-minute exploration stretch. Location/stage checks always outrank random selection. Health/air warnings need reliable UI/SFX and must never depend on dialogue.

| ID | Speaker / text | Trigger / repeat limit |
|---|---|---|
| `bark.kit.loose_stone` | Kit: “Mind that loose edge.” | Only a visibly relevant traversal hazard and Kit present; once per authored hazard, max twice per chapter. |
| `bark.kit.find` | Kit: “Worth a look.” | Optional discovery actually visible; once per chapter; not a compulsory puzzle hint. |
| `bark.kit.detour` | Kit: “I'll keep an eye on the path.” | First deliberate detour near a wait mark; once per chapter. |
| `bark.kit.dark_humour` | Kit: “Lovely place. Shame about the dead.” | Safe early exploration, before tragic revelations; once per campaign. |
| `bark.kit.safe` | Kit: “Take a moment. We're clear.” | After danger has genuinely ended with Kit nearby; once per expedition. |
| `bark.lantern.air` | Lantern voice: “Air above you.” | Only a reachable known air pocket; once per crossing, 90-second cooldown; suppress during critical breath cues. |
| `bark.lantern.smoke` | Lantern voice: “Clear air through that arch.” | Only verified safe direction; once per hazard section; no blind geometric claim. |
| `bark.lantern.light` | Lantern voice: “The light reaches it now.” | First successful optical lesson per tool; no repeated praise for routine use. |
| `bark.guard.hold` | Guard candidate: “Hold the threshold.” | Optional future intelligible guard treatment; once per encounter, not 1.6.2 scope expansion. |
| `bark.bellkeeper.duty` | Bellkeeper: “The bell must sound.” | First authored duty phase; once per attempt, no earlier than 30 seconds after previous speech. |
| `bark.mintmaster.debt` | Master of Coin: “The debt remains.” | First appropriate combat lull; once per attempt. |
| `bark.court.stay_back` | Court keeper: “Keep it away from the glass.” | Only if supported by encounter mechanics; once per attempt. |
| `bark.entity.command` | Lantern voice: “Kneel.” | Freed entity phase only; once per attempt; do not remove player control. |

Combat: no overlapping speech with critical attack telegraphs; cap boss speech at three short optional lines per attempt including scene-table combat barks. Repeat attempts can reset tactical barks, not replay the whole reveal. Ordinary skeletons may remain nonverbal; movement/attack/impact/death sounds belong to the separate creature-audio specification. Kit is not assumed to join dungeon combat or be an escort objective.

## 14. Fair-clue and spoiler ledger

| Clue | Earliest availability | Innocent/initial reading | Payoff and fail-safe |
|---|---|---|---|
| F1: hidden treasury-road mark | Forest, light inspection | Lantern is a useful treasure-hunting tool. | Hub register gives a route; no prison reveal. Objective persists if Kit line missed. |
| F2: guards hold threshold; lich refuses to leave post | Tomb observation; optional later dialogue | Cursed defenders protect valuable treasure. | Court identifies fourth jailer; critical Court evidence does not require hearing lich. |
| F3: marked plunder and coerced “tribute” | Abbey critical evidence | Claimed king admits a real wrong. | Treasury repeats recognisable craftsmanship; physical record survives sale/skip. |
| F4: precise oath wording; service after death | Foundry critical record | Claimed king remembers his army. | Court proves entity authored the bargain; either dungeon order works. |
| F5: keeper/jailer reading | Two-seal hub convergence | Translation introduces reasonable doubt. | Court prison recognition, with inspectable inscription and repeatable short recap. |
| F6: garden memory conflicts with dated evidence | Glass Court | Claimed private royal memory briefly humanises voice. | Keeper/record contradicts it; king's signed order independently proves deception. |
| F7: king's order bears his seal alongside keepers | Glass Court, critical | No longer merely competing accusations. | Entity's royal claim breaks; player knows release risk before ritual. |
| F8: shared bond frees both soldiers and entity | Court explanation, treasury inspection | Good act has a dangerous consequence. | Soldiers visibly released; entity must be defeated with learned tools. |

**Spoiler stages:** S0 tomb/forest (no speaking entity required); S1 first hub claim; S2 middle-dungeon wrongdoing and binding; S3 Court identity/prison reveal; S4 ritual/release/finale; S5 return. Author-facing true labels can exist in development data, but player-facing UI, previews and subtitles reveal only current knowledge. Do not use “defeat the fake king” as an early quest title.

## 15. Review and production gates

1. Review the opening wording, including whether to keep the owner's full grate line and the proposed rescue/departure refinements. IDs and silent raise event remain stable.
2. Decide first lantern-speech staging, supporting names/combined roles, and whether the proposed shrine register, marked vessel and dated garden evidence fit the world.
3. Prototype each access option and optical lesson before approving lines that describe exact hazard/puzzle behaviour. No branch may require both middle-dungeon tools.
4. Resolve the Court keeper confrontation/fate, precise shared-bond explanation, final boss form and Kit's safe finale presence. Preserve the single ending and truthful release of the dead.
5. Play the story both Abbey-first and Foundry-first; also test missed grate call, skipped conversations, sold named relic, no optional hub repairs, missing optional clue, death/retry and checkpoint reload.
6. Only then lock text revisions, cast/listen/approve voices, verify commercial rights, record duration and create offline assets. This document contains no recordings or production clearance.
7. Editorial checks for this draft: distinct stable IDs, no player spoken dialogue, no pre-reward lantern possession, role/name status explicit, critical reveals recoverable, references resolve, only documentation changes.

**Audio/haptic manual revalidation required: NO — documentation only.** Any later runtime/voice implementation requires its affected manual audio checks. Repository Markdown readback verifies this bank; it does not certify gameplay, performance or voice quality.

## Opening performance revision 7 October 2026

The expanded English bank [performance brief](dialogue/en/PERFORMANCE-DIRECTION-English-en.md) now carries Kit’s urgent, worried, edge-of-panic opening and the optional audible player-movement acknowledgment. Spoken wording and IDs are unchanged; Kit stays unseen beyond the small wall panel. Preserve the 1.7 safe pass-zone and missed-call contract, with no mandatory reply or animation. The later rescue greeting and fresh manual lantern raise remain distinct.
