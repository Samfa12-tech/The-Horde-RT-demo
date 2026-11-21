# Bellwether arrival: locked directing direction

**Owner approval: 7 November 2026. Planning and voice production, not implementation or animation acceptance.** The [shared roadmap](../../ROADMAP.md#18---bellwether-npcs-and-light-rpg-systems) remains the scope authority. Sam approved the 7 November lookout-to-town screenplay with: “ok YES lock this in. we likely need some more lines produces via Mureak - can you organise that now?”

The approved direction is a continuous, largely player-controlled sequence: existing lookout, loose-ground ankle injury, short independent protective walk, square, south tavern door, chair by the hearth, then a player-initiated conversation. This record supersedes the older undecided injury-cause and offered-arm draft, without treating a plan or recording as a finished scene.

**1.8 kickoff, 21 November 2026:** all four pickups below are now present in the private production archive. The receipt, originals, review WAVs, script and listening reels match the archived SHA-256 inventory. The new takes still need listening approval and runtime admission. The five previously approved Kit lines remain the reuse source for the hearth and first conversation.

## Locked direction and boundaries

- Keep the same night, road, skyline and unwalled Bellwether visible from 1.7. Arrive southeast from the lookout without crossing the river. The silent player retains the camera; no forced cinematic shots.
- Kit turns his ankle on a small loose patch of road, catches his balance and tests the foot. No attack, full-body tumble, ragdoll, loud bone crack, quick-time event or punishment for not catching him.
- Kit walks independently over a short route, protecting the ankle. Do not promise player-arm support or paired locomotion. Looking away must not cause the stumble to replay; overtaking, stopping and backtracking must not become an escort-failure system.
- Reach the well/square and approach the tavern through its mapped south-facing doorway. Do not invent a north door to shorten the sequence.
- First arrival features Kit and the capable, practical tavern keeper. Other villagers stay at their own service areas rather than crowding the doorway. The approved keeper voice is the capable young-adult woman, **Ironclad Strategist**, from audition02. Do not silently recast her to match an inconsistent model.
- Kit settles at a sturdy chair/table in a quiet alcove near the hearth, clear of the door swing and player route. His emotional progression is care, frustration, then competence. He remains the player's useful partner, not an abandoned quest marker.
- The longer conversation starts through player interaction after Kit sits. Kit indicates the lantern without taking it. The first lantern speech remains the later shrine scene, with Kit absent. No new lantern voice on arrival, automatic dawn or unexplained recovery.

## Screenplay and exact dialogue

### 1. Lookout, night

Kit and the player see the same town they will reach. Let cold moonlight, warm windows and the existing line breathe. Preserve the view and player control.

**KIT:** “There. Chimney smoke.” (`forest.village`, existing runtime take)

Kit takes the road when the player advances. Completed 1.7 saves do not replay the reveal or demand another lantern presentation. Replace the old demo turnaround/end presentation deliberately when extending the route.

### 2. Lower road

A stone shifts under Kit's foot. His ankle rolls slightly, knee yields and arm/torso counterbalance catches him. He stops with weight on the other leg. A boot scrape, small gravel movement and controlled breath support the action.

**KIT:** “Ah. Wait a moment.” (`kit.approach_injury.stop`)

He tests the foot and removes the weight.

**KIT:** “Loose ground. Turned my ankle. I can stand, but give me a moment.” (`kit.approach_injury.ankle`)

**KIT:** “Stay close a moment. I can manage if we go slowly.” (`kit.approach_injury.slow`, new ID)

The new `slow` line replaces the old unrecorded `kit.approach_injury.arm` (“Lend me your arm for this bit. We'll take it slowly.”). Preserve the old historical bank but do not schedule its arm line or imply that physical support exists.

### 3. Down to the square

Kit uses a restrained protective walk. Injury persists across starts, stops and turns. The player can look around or move ahead. Keep the journey short and readable without constant pain barks, route exposition or an escort timer. The church path remains an actual branch, not blocked scenery.

### 4. Tavern doorway

Warm light comes through the real south doorway. The keeper notices Kit and stays clear of his route and the door swing.

**TAVERN KEEPER:** “Easy. There's a chair just inside.” (`hub.arrival_chair`, new ID)

The existing **“Shut the door behind you. Both of you look frozen.”** (`hub.arrival`) is **conditional**, not compulsory: reuse only if it fits the final door interaction and performance. Closing the door must not gate progression.

### 5. By the fire

**KIT:** “Right. I'm sitting down before I make this worse.” (`kit.accept_base.sit`)

Kit turns, finds the seat and lowers himself with the injured leg protected. Feet, pelvis, chair, clothing and hands must make convincing contact.

**KIT:** “I won't get far along those roads like this. You'll have to go without me.” (`kit.accept_base.limit`)

A pause. His attention moves to the table and what he can still do.

**KIT:** “Bring what you find back here. I can still make myself useful.” (`kit.accept_base.work`)

### 6. First conversation

After settling, the player chooses to talk rather than triggering a lecture on entering.

**KIT:** “The tomb was there. So was something worth the climb.” (`hub.kit_introduce`)

**TAVERN KEEPER:** “My grandfather called the Horde an army. You treasure hunters always hear the other word.” (`hub.horde_army`)

**KIT:** “Armies need paying. I'm hoping we're both right.” (`hub.hoard_answer`)

A useful lead points toward the record keeper and wayside shrine. The record keeper stays by the northern church rather than appearing in the tavern. The player can leave through the same door; Kit remains settled and subsequent visits recognise the completed arrival.

## Geography and staging authority

Use the [town service drawing](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/02b-bellwether-use-1-8.webp), [terrain drawing](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/02a-bellwether-terrain-1-8.webp), [shared coordinate layout](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/shared-coordinate-layout.json) and [world atlas](../../design/world/atlas/README.md). Their metric coordinates are proposed blockout, not certified engine transforms.

The keeper belongs at the tavern; record keeper north by the church; artificer at the southeast smith; salvager east toward river/mill; relic dealer west of the square. These service anchors do not finalize actor heights, facing, pathing or activity. The old square-side Kit marker predates his recovery arc and is not a final seat.

Protect the northwest return to the lookout, northern church/shrine routes and southeast regional road. Future branches remain downstream Abbey, eastern gorge/Foundry and northeast ridge/Glass Court. Exact junctions, entries and terrain remain proposals. Their mouths should be legible without falsely advertising unfinished areas as playable. The regional Foundry is distinct from the village smith; the mill bridge is not the Foundry crossing.

### 1.8 route and tavern checkpoint

The Windows host fixture drives the shared player motor from a completed 1.7 lookout checkpoint, through the fully populated forest, down the mapped road, past the square and into the south tavern entrance. It then returns to the lookout without replaying Kit’s reveal. The normal scene opens the two rail spans on the Bellwether road. The rest of the forest boundary retains its rendered collision, and the explicit historical forest proof still uses the closed turnaround.

The editable tavern blockout is derived from the v4 master. It retains the floor height and roofline and fits an 11.5 m stone ramp to the current terrain. The ramp rises roughly 3.26 m from its buried toe to the 2.2 m wide south doorway. The ramp, floor, building solids and west-wall hearth share the native render geometry with the movement system. See the [Blender recipe](../../../tools/author-bellwether-tavern.py) and [asset receipt](../../../assets/models/world/source/bellwether-tavern-v1/authoring-receipt.json).

The west-wall hearth uses the shared fire system above three charred billets. Original timber furniture stages Kit north of the hearth, clear of the doorway. The 49 cm chair and 75 cm table are solid parts of the rendered room. The authored turn, approach and conversation marks were reached by the normal player motor and the route back through the south door passed. The Windows Diagnostic capture shows the furniture and both fire sources in a 960 × 540 hardware RayTracingPipeline frame on an RTX 5050 Laptop GPU. Final doorway light, smoke and audio still need scene review with the actors present. Android performance and owner playtesting remain open.

The south-wall window uses an original timber frame and one textureless, closed 8 mm glass volume from the same Blender source as the room. The glass adds one material, one 12-triangle BLAS and one live TLAS definition. The existing Glass setting controls its ray visibility, with solid contact in both modes. The exterior route retains it for secondary rays through the approach and return. `bellwether-tavern-window` walks the authored route inside and looks back at the pane and the open south door. Use Glass On explicitly for the combined glass-and-fire phone checkpoint.

Windows Diagnostic presented this view on the RTX 5050 Laptop GPU at 960 × 540 using both RayTracingPipeline and RayQueryCompute. The matched RayTracingPipeline views changed the glass instance mask from 1 to 0 with Glass Off while preserving the road-stone instance and the view through the open doorway. The full chapter scene used 33 live TLAS definitions and 24 BLAS owners with the pane resident. The frame still reads very dark inside. It leaves combined lighting with the keeper, sustained performance and mobile presentation open.

On the south ramp, the flat carry sweep read the rising floor as a wall after 0.64 m. The shared sweep now follows the standing slope for upward walkable faces. The host fixture recovered the full 2.7 m carry lookahead, and the matched Windows RT view recovered 22,119 primary arm pixels from zero. Separate host collision tests still reject raised ground and low ceilings.

The editable [Kit hearth sit](../../../assets/models/characters/source/kit-hearth-sit-study-v1/README.md) establishes a planted stance on the actual seat. The native clip appends to the existing Kit payload, preserving the earlier mesh, skin and animation bytes. Eight native samples agreed with Blender to within 0.026 mm. The current Windows RTX seated view restores a completed 1.7 lookout record through the save transaction, then runs both actors through the actual road and south door. Kit completed the 16-station route, turned into the measured seat and held the final frame at 1.367 s. Kit reads mostly in silhouette under the current hearth and player lantern, so the final conversation needs a dedicated lighting review.

The earned lookout stays quiet until the player takes the road. Kit uses the same terrain and obstacle checks, continues if the player moves ahead, and stops once at the lower-road assessment mark. The [editable road actions](../../../assets/models/characters/source/kit-ankle-approach-v1/README.md) are connected to that stop and the remaining walk. The assessment lasts 1.6 seconds and the protective gait advances with 0.76 m of admitted travel per cycle. The Windows RTX `bellwether-kit-ankle-catch` view shows the catch at 0.33 s on the real road. The silhouette reads more clearly than the foot under the current night lighting. The supported chair turn and sit are connected in campaign play.

The fixed-step catch emits a [boot-and-gravel scrape and an unvoiced recovery breath](../../../assets/audio/kit-ankle/asset.manifest.json) from Kit's position. The native route uses the [loose-stone and ankle-fit study](../../../assets/models/world/source/bellwether-loose-road-stone-v1/README.md). The stone moves 284 mm on the existing assessment clock, settles by the breath and stays settled through the route and tavern reloads. It adds one 42-triangle BLAS and one live TLAS instance, using the shared static material path without a texture layer. The Windows RTX catch presented with 32 live instances at 960 × 540. The dark road still makes the contact subtle.

The three roadside lines now play as named captions for a player within five metres of Kit and on nearby ground. The first follows the breath cue, Kit holds his supported stance through the assessment, and the last line accompanies the start of his protective walk. The player keeps control and can walk on. Leaving the area ends the exchange and Kit continues without repeating it. Caption timings are provisional. The private voice pickups still need listening approval and runtime admission, and the foley mix and final pacing need playtesting.

Once Kit is seated and the player reaches the safe area by the hearth, the ordinary campaign save system earns a **Bellwether tavern** checkpoint. The player restores at the fixed standing mark and Kit restores in his settled pose against the current authored chair. Later safe-side saves retain the seated milestone. Older lookout saves retain their original layout and still begin before the journey. The Windows RTX route diagnostic earned this checkpoint on foot and presented the restored seated scene. A separate disk round-trip checked a fresh session, including rejection when the chair was missing.

Kit's three hearth lines now run as named captions. The first accompanies the lowering, then Kit explains his limits and pauses before offering help. A player who arrives after the sit sees the two seated lines. The tavern autosave tracks resolved lines, so a reload resumes at the first unresolved beat. Audio/haptic manual revalidation required: YES, for the hearth timing, captions and in-room mix. The reviewed recordings remain in private staging.

The shared interaction and save path now supports the three-line exchange between seated Kit and the tavern keeper. Talk appears only after the hearth lines resolve and both speakers reach their marks. Leaving the hearth or reloading stops the exchange. Choosing Talk again resumes at the first unresolved line. The [editable tavern](../../../assets/models/world/source/bellwether-tavern-v1/bellwether-tavern-v1.blend) now marks the keeper beside the south doorway and across from Kit at the hearth, with an aisle route between them. The shared motor walks that path after Kit sits and restores the keeper at the hearth from an earned tavern save. The ordinary scene still needs the keeper model before it can display Talk. Voiced takes, the shrine lead and the combined scene playtest remain open.

In the staffed fixture, the keeper's chair greeting plays as a caption when Kit approaches the south door and the player is within earshot. Solid walls block the cue, and moving on ends it without a repeat on backtracking. Kit keeps walking through it. The cue uses the same admitted-keeper gate as Talk and stays out of saved playback state.

The keeper's resident render pose now comes from the shared simulation. Each admitted walking step advances the clip clock by its travelled distance at the staged walking speed. The door and hearth use the resident's idle pose. The Windows hidden views `bellwether-keeper-reply` and `bellwether-staffed-room` use `--chapter-resident-fixture <directory>` with a local textureless Kit stand-in. Both restore an already-earned tavern record through the normal save transaction. They take the Talk and Skip input edges and verify that the reply originates at the keeper's authored hearth mark. The reply view freezes there. The room view walks back through the real interior and verifies that the caption stops outside Talk range.

At 960 × 540 on the RTX 5050 Laptop GPU, both hardware RT backends presented the room view with seated Kit, the stand-in, active glazing, and the hearth and reward lantern in the uploaded light roster. The scene used 34 TLAS definitions and 25 BLAS owners. The RayTracingPipeline reply view also preserved the `hub.horde_army` state in its capture manifest. Kit reads mostly as a silhouette from the doorway. The original keeper asset, voiced playback, final scene lighting and phone performance need their own checks. The live approach, welcome and movement remain covered by the separate authored route test.

The table now has an authored candle, holder and wick in the editable tavern source. The recipe exports the wick and light anchors with the furniture, and the shared simulation uses the existing fire path to light the room. The two town fires share the fixed upload budget with the player's light. The lookout is outside their activation distance, and the tomb torches keep their existing triggers. The town export gained 100 solid triangles within the existing four material rows. The window export is byte-identical.

In a matched doorway view, doubling the hearth output mainly brightened its masonry and Kit's cast shadow. The retained candle improved the seat and table outline with the original hearth output. Both Windows RT backends presented the candle, hearth and player lantern at the same camera and 960 × 540 settings. The room still used 34 TLAS definitions and 25 BLAS owners. The native fire, town geometry, authored route and package-policy checks passed. The Android debug APK contains the exact tavern export. Phone rendering and sustained performance remain open.

The closer Windows RayTracingPipeline view replayed the 1.7 lookout continuation and restored the earned tavern seat. At the conversation mark, the candle brings out Kit's hood and armour while keeping the room dim. The earlier view is within 6 mm of the new camera position and has the same angles and seated pose. The staffed reply view also presented with all three lights and retained the `hub.horde_army` caption.

During `hub.kit_introduce`, Kit now glances toward the light inside the player's lantern cage. The shared simulation follows the held pendulum and the authored Light socket offset. The seated clip still owns his hands, feet and body contact. The cue eases in and out during the caption, pauses with the scene and settles when the line ends or is skipped. The Windows `bellwether-kit-lantern-glance` checkpoint restores the authored tavern seat and presses Talk with the development keeper stand-in. Both hardware RT backends presented the same seated pose and roughly 15° of additive head yaw at 960 × 540. The native rig test verifies the seated Head position and checks that adding the glance preserves all vertices below one metre. The Android debug build passed. Voiced timing, continuous motion review and physical phone performance remain open.

## Open decisions deliberately preserved

1. **Healing flask:** why it cannot simply heal Kit remains unresolved. Approval of this direction did not supply a new healing rule. Do not invent a diagnosis, flask restriction or miracle-cure workaround. Resolve fiction and interaction consistently before scene acceptance or recording any explanation.
2. **Final blockout:** injury mark, tavern chair/table/hearth/keeper marks, door clearance, actor facing and exact routes still need joint spatial validation. The mapped south door is the continuity constraint, not proof that its model works.
3. **Performance acceptance:** approved dialogue text is not approval of new takes, injury/walk/sit clips or target-device results. Existing Nov 4 Kit-reel listening approval remains valid and should not be reopened unnecessarily.
4. **Optional door line:** decide against the actual door behaviour and avoid redundant greetings.

## Voice audit and production handoff

Four missing takes were identified for this exact arrival, using established voices and plain spoken text without direction tags:

| ID | Voice | Delivery intent |
|---|---|---|
| `kit.approach_injury.stop` | Saved Kit | Brief involuntary catch, then controlled pause. |
| `kit.approach_injury.ankle` | Saved Kit | Calm self-assessment, discomfort without melodrama. |
| `kit.approach_injury.slow` | Saved Kit | Reassuring and practical; independent walk. |
| `hub.arrival_chair` | Ironclad Strategist | Capable keeper notices the difficulty and directs him inside. |

The five existing Kit tavern/conversation WAVs were verified present and hash-matched against the 4 November private intake receipt, with owner listening approval. `forest.village.wav` is present in the runtime folder. Tavern `hub.horde_army.wav` and optional `hub.arrival.wav` are present in the October production archive; casting is approved, but do not treat the older archive's technical checks as blanket approval of every production take.

Keep original MP3s, per-line review WAVs, exact text/voice/cost manifests and a short pickup listening reel in the existing private Horde audio archive. Do not overwrite accepted runtime audio or insert new clips into the game as part of this planning task. Use the pickup receipt for exact production/QC status; this plan does not claim runtime admission, licence clearance for wider distribution, or performance approval.

## Animation and gameplay acceptance gate

First author a short test on the real Kit rig: loose-ground step/catch, protective standing assessment, several injured steps, turn and sit. Keep editable sources. Review normal first-person, front and side views for weight, planted feet, contact and coherent transitions before expanding into the complete scene. A flattering render is insufficient.

Then validate the whole slice in-engine with representative fire/lantern/glass/actors and exterior overlap on Android and Windows. Check looking away, overtaking, stopping/backtracking, repeated visits, subtitle-only/missing audio, skip/pause/quit/reload and door/collision recovery. Persist injury/arrival/settled state without duplicating the incident or trapping the player. Preserve real RT, shared simulation and existing scene/streaming contracts. No build, animation implementation, merge or release is authorized by this document's completion.
