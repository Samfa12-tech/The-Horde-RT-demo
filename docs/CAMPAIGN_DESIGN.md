# The Horde — Campaign, Characters and Progression
**Owner-approved creative direction: 30 September 2026.** Planning, not implemented content or release authority. Read with [ROADMAP.md](ROADMAP.md). This document supersedes older statements that the Horde's nature, dungeon order and final mystery are entirely undecided. Concrete encounters, scripts, balance and technical solutions still require scoped design and testing.

## Identity and pillars
A first-person historical-gothic dark-fantasy treasure-hunting adventure. Primary structural inspiration: Ocarina of Time. Distinctive places, learnable tools, environmental puzzles, secrets, memorable bosses and welcome returns to a useful village. Original characters, art and dialogue; no copied Zelda or Jak and Daxter assets, voices or scripts.
The Horde began as a vast army. Passed-down accounts confuse that lost army with the treasure hoard it carried home. Both meanings can be true. Retain **The Horde** as the working title; no automatic rename.
The treasure is physically real and desirable, not a bait-and-switch metaphor. Much was plundered. “Treasure hunter” and “grave robber” are competing perspectives within the world.
Themes: greed versus enough; protection becoming control; the living inheriting the debts of the dead; belonging while seeking fortune. Darkness and danger need warmth, humour and human stakes in contrast.
Light is causal gameplay through the real RT engine. Enemy placement needs a reason: duty, territory, captivity, hunger or another readable cause, rather than arbitrary combat filler. Mindless monsters still belong in an ecology.

## Campaign structure
Tomb prologue → rope rescue and moonlit forest → village arrival and first small expedition → Abbey and Foundry in either order → Glass Court after both → treasury and authored final confrontation → playable return/epilogue.
The three themed dungeons are additional to the existing tomb. The finale is a compact culmination, not a commitment to a fourth full dungeon. One satisfying ending is the agreed target; do not implement an alternate keeper ending.
Each middle dungeon is solvable with starting equipment, its access solution and its own local tool. The other dungeon's tool can unlock optional secrets/shortcuts, never a hidden mandatory dependency. Essential revelations must work in either order.
Every major area has an access mini-quest with alternative Magic, Tech or Constitution solutions. Mandatory progression must remain reachable for mixed builds; prevent irreversible spending or build choices from trapping the campaign.

## Prologue, rescue and forest
The existing lich is the cursed fourth keeper; the two skeletons are its remaining sworn guards. They protect the lantern's resting place. The player defeats the lich before receiving the lantern: no pre-reward scene may assume lantern ownership.
Give the lich a grander, staged reveal, readable presence and improved animation and sound. Its defensive behaviour should later make sense as that of a desperate jailer.
Before the rescue, Kit's first in-dungeon introduction is a concerned call from above the existing skylight/grate as the player walks past after the skeleton encounter (owner addition, 3 October 2026). Tie it to report **3696c1a2-5fb3-4476-aaeb-456a130837d8**, whose deeper shaft and hanging growth remain 1.6.2 environment polish. The one-shot voiced beat belongs to 1.7: no forced camera, player reply, combat lock or early rescue/lantern reveal. See [the draft line and trigger contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#73-initial-script-and-triggers); exact wording remains provisional.
Kit rescues the player via the planned rope. A familiar, practical fellow treasure hunter, Kit first cares whether the player survived, then whether the expedition was worthwhile. Preserve quiet relief, moonlit wonder and a brief uncanny clue rather than immediately staging another boss.
A waystone/inscription seen differently by lantern light can hint at the lost treasury. Exact clue and spoken wording remain authoring work. Do not reveal the prison twist here.

## The village and recurring cast
**Bellwether** remains a working village name. A compact settlement among the old kingdom's trade-route ruins, worth returning to and eventually thinking of as home.
The hub must benefit story AND play: information, access quests, gear, upgrades/training, spending/recovering treasure and visible consequences. It cannot be only a level-selection room.
- Kit: loyal companion with humour, competence, personal wants and opinions; sometimes selfish, not an automatic traitor. Not a compulsory combat companion or escort-failure system.
- Tavern keeper/former caravan leader: cares about reopening the road; name TBD. **Do not use Mara**, already reused across the owner's stories.
- Record/shrine keeper: interprets history, inscriptions and magic; Brother Ansel is provisional.
- Artificer/smith: practical gear, recovered mechanisms and repairs; Hester is provisional.
- Retired river salvager: teaches breath control, climbing and endurance.
- Relic dealer: ordinary valuables can be sold; named objects can present sell/return decisions.
Roles may be combined into a small cast. Total campaign cast is not simultaneous actor count.
A repaired mill, reopened trade, a family staying or changed reception should acknowledge meaningful choices. Avoid merely cosmetic quest labels.
Start economy design with ordinary treasure currency, scarce named components and quest-earned training. XP, skill-point costs, respec rules, crafting and final prices remain undecided; do not layer several currencies on by default.

## Three complementary improvement areas
Small mixable progression, not exclusive classes:
- **Magic:** light manipulation, bindings/inscriptions, temporary breathing wards and short magical crossings.
- **Tech:** breathing equipment, filters, climbing gear, mechanisms, lantern improvements and bridge repair.
- **Constitution:** breath control, climbing/endurance, recovery and limited heat/smoke tolerance. “Endurance” is a possible UI label, not a locked rename.
Each route needs useful benefits and alternative access, without requiring all upgrades. Constitution delays exposure consequences; it does not imply infinite breath or immunity to choking.
Access quests should teach a skill and reveal a person/place: a mill component for an air device, a restored wayside shrine for an air ward, or a salvager-led practical training route. These are approved examples to refine, not three mandatory fetch quests.

## The three dungeons
### Drowned Abbey
Flooded religious house, bells beneath water, dead attendants continuing their duties. Access requires a breathing/breath-control solution through one progression area.
Initial design direction: short submerged approaches and air pockets leading to partly dry rooms so technology, magic and trained breath control are credible alternatives. Exact underwater extent and movement model require design.
Water/light interaction is a major artistic and gameplay requirement. Design clear, bounded optical puzzles and test on the actual phone; this is not permission for unbounded fluid/caustic simulation.
Local tool candidate: placeable reflector. Teach safe redirection, combine with water/occlusion and encounters, then test the skill against the **Bellkeeper** (working boss name), a drowned armoured guardian exposed through shutters/reflected light.
Reward: first of the two order-independent treasury seals plus evidence that royal “gifts” were plunder, including an object with a living claimant. The voice admits wartime wrongdoing to gain credibility.

### Ashen Foundry
Royal mint/forge, cursed workers and machinery. Fire and smoke are defining effects and potential hazards. Constitution can help tolerate exposure; Tech filters and Magic protection offer complementary approaches. Hazard logic and visual density must agree.
Access example: repair the broken bridge, climb an old maintenance route, or restore magical stepping stones. Routes converge on the same dungeon.
Local tool candidate: shuttered lantern stand, leaving light in place and controlling exposure; shadows can conceal the player from watchers.
Working boss: **Master of Coin**, skeletal mintmaster in an articulated furnace shell. Combine cover, positioning and learned light mechanics.
Reward: the other order-independent seal and evidence of a royal bargain binding the soldiers beyond death. The voice blames the keepers for prolonging the suffering. Return to meaningful hub changes.

### Glass Court
Ruined mirrored palace, roofless winter garden and surviving lucid lich keeper(s). Beauty should explain the desire to preserve the place. Available after Abbey AND Foundry.
Local capability candidate: focused lantern aperture, combined with earlier tools. No infinite mirror/reflection requirement.
A surviving royal keeper recognises the prison being carried and supplies evidence that the real king helped imprison the entity. Boss staging, dialogue and keeper survival require detailed authoring.
Reward: final seal and treasury location. What seemed like protection is increasingly revealed as possession and control.

## Spoilers: the lantern, king and ending
The lantern voice claims to be the dead king, betrayed and imprisoned by his keepers. It guides the player toward the three seals, promising release and restoration.
In truth it is the entity that offered the king victory. Its bargain created the conquering Horde, its plunder and the soldiers' service beyond death. The king eventually resisted; four keepers divided the prison's seals and became cursed custodians.
The player took part of that prison from the fourth keeper in the prologue. The entity's guidance can be practically helpful while its account of history is false.
Seed fair clues: mistaken personal memories, precise military knowledge, inscriptions where “keeper” means jailer, and guardians defending custody. These are clue candidates; maintain a reveal ledger so evidence is available before its payoff. Do not stage prologue guards attacking a lantern the player does not yet own.
The final deception must remain believable after Glass Court warnings. The voice teaches a ritual that genuinely releases the bound dead but also frees itself. The player achieves a good act with a dangerous consequence, rather than being forced to ignore an obvious warning.
Treasury reveal fulfils the promised spectacle: real riches and recognisable objects from earlier regions. Final encounter uses established tools, light, movement and combat to defeat the freed entity; no unexplained last-minute mandatory ability.
One hopeful, earned ending: soldiers released, entity defeated, treasure able to leave the vault, return to the village. A short playable epilogue reflects what the player helped preserve. Exact distribution, losses, final confrontation, entity form/name and closing scene remain to be written.

## Voice and player character
**Silent player protagonist.** No spoken player dialogue, voiced answers or explanatory player monologues. Convey intent through player-controlled actions, observation and staging. Nonverbal exertion sounds, if desired, need a separate creative decision.
**Eventually fully voiced narrative:** Kit, NPCs, the lantern entity and speaking antagonists, with subtitles and offline playback. This is a campaign production goal, not permission to buy voices or a claim recordings exist.
Dialogue uses its own volume slider, independent of music and SFX, plus an explicit subtitle option. Prefer top-safe-area subtitles on mobile, with scalable readable text clear of essential HUD; preserve comprehension without stereo audio. Kit's grate call is spatially anchored above the opening, with bounded distance/occlusion treatment that stays intelligible. Reuse shared audio infrastructure; see [1.7 dialogue infrastructure](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#74-dialogue-infrastructure).
Kit carries conversational momentum when something needs saying, in the companion-function spirit of Daxter in the first Jak and Daxter. Kit has their own perspective: does not literally quote unspoken player thoughts, decide the player's moral position or incessantly explain puzzles. Quiet exploration remains valuable.
Update the 1.7 script to remove old player lines. A fresh manual lantern raise is the player's answer; Kit reacts once the lantern is visibly presented. Optional hints remain delayed and limited.
Give skeletons appropriate movement/attack/impact/death sounds and the lich its own presence, combat and defeat audio. Avoid repeated synchronised sound/animation loops.

## Existing-demo polish and milestone ownership
Finish, accept, merge and release 1.6.1 first. Existing-demo enemy improvements belong in the planned 1.6.2 polish scope, re-audited against that completed baseline:
- Investigate the owner's observation that both skeletons look identical and start animation together. Provide believable visual and idle/animation variation without desynchronising authoritative attacks, hit windows or feedback. Measure additional independent pose/skinning/RT costs.
- Add/complete skeleton sound coverage; audit existing hit/death cues rather than duplicating them.
- Full lich presentation pass: grander reveal, animation readability and transitions, presence/combat/defeat sounds, and keeper-consistent staging. Preserve lantern reward order and test progression.
1.7 owns voiced Kit, rescue and forest; 1.8 must be re-scoped for a useful playable hub with bounded progression/services and access preparation. Three dungeons and finale remain later gated work.
No runtime implementation, asset/voice generation, paid work, merging, release or device installation is authorised by this documentation update. Audio/haptic manual revalidation: NO for docs; YES for subsequent affected sound/voice implementations.
