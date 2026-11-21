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

Private playtest builds can enter through the ordinary campaign menus. The separate Android Bellwether app uses its own files directory. Windows Debug accepts `--bellwether-campaign-dir <local two-resident fixture>`, which checks the pinned models and texture arrays and uses `HordeLanternRT/bellwether-debug/campaign-v1` under Local AppData. New Game and Load prepare both residents with the authored scene before play begins. The lookout and tavern use the existing checkpoint format and camera/presentation barrier.

The hidden Windows test picked New Game and a seeded 1.7-format lookout save through the current native slot menus on the RTX 5050 Laptop GPU. Both reached live control after hardware RT presentation with the two resident sources mounted. Returning from the lookout to the title retired those sources and preserved the test save bytes. RayTracingPipeline and RayQueryCompute each passed two New Game and four lookout assertions on the same Debug executable (SHA-256 `132814ac72207d17f45dd89c4d5a63c7f3090352824ce741818dcbdabddfa043`). The Android package and host menu checks passed. Device validation of the ordinary Android campaign route is still open.

The earned-tavern Windows check extends the same hidden native menu flow. Export a fresh lookout fixture with `horde_rt_campaign_play_session_tests.exe --export-lookout-fixture <fixture-directory>` in an empty `reports/horde-campaign-fixtures-*` directory. Launch Windows Debug with `--lookout-validate-hidden --bellwether-earn-tavern --campaign-test-root <fixture-directory> --bellwether-campaign-dir <resident-fixture>`. Add `--require-rayquery-compute` for the query backend. This check walks the authored route with fixed-step movement input, starts Talk, writes the earned tavern autosave and reloads it through Pause → Load. It replaces the exported seed inside that fixture, so make a fresh export for each run. The assertions are in the fixture's `windows-lookout-validation.json`.

Both Windows hardware RT backends passed all seven assertions on the same Debug executable (SHA-256 `4f352112e829e96b67a08b4a8581bad1629596bd112a652edbe43c56212e9606`). The load presented a fresh world generation and resumed automatically with Kit seated, the keeper at the hearth, Talk complete and Directions available. The resumed autosave matched the bytes written by that run. This starts from a seeded lookout in a hidden window using the keeper stand-in and muted audio. Android device play and sustained performance remain separate open checks.

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

The shared interaction and save path supports the three-line exchange between seated Kit and the tavern keeper. Talk appears only after the hearth lines resolve and both speakers reach their marks. Leaving the hearth or reloading stops the exchange. Choosing Talk again resumes at the first unresolved line. The [editable tavern](../../../assets/models/world/source/bellwether-tavern-v1/bellwether-tavern-v1.blend) marks the keeper beside the south doorway and across from Kit at the hearth, with an aisle route between them. The shared motor walks that path after Kit sits and restores the keeper at the hearth from an earned tavern save. The private campaign enables Talk with the stand-in. The final keeper model and the keeper's voiced lines remain open.

The November character intake records four previously retained town NPCs and an October archive transfer problem. The existing record keeper study is available and supplies the textured runtime fixture. The original tavern keeper package and its rig and motion remain unverified. The signed-in Meshy model search returned no named result under tavern, keeper, caravan or Bellwether. Blender inspection identified the two recent unnamed API results as the base mesh and rig of the same armoured male character. This lookup used zero credits.

In the staffed fixture, the keeper's chair greeting plays as a caption when Kit approaches the south door and the player is within earshot. Solid walls block the cue, and moving on ends it without a repeat on backtracking. Kit keeps walking through it. The cue uses the same admitted-keeper gate as Talk and stays out of saved playback state.

The keeper's resident render pose now comes from the shared simulation. Each admitted walking step advances the clip clock by its travelled distance at the staged walking speed. The door and hearth use the resident's idle pose. The Windows hidden views `bellwether-keeper-reply` and `bellwether-staffed-room` use `--chapter-resident-fixture <directory>` with a local textureless Kit stand-in. Both restore an already-earned tavern record through the normal save transaction. They take the Talk and Skip input edges and verify that the reply originates at the keeper's authored hearth mark. The reply view freezes there. The room view walks back through the real interior and verifies that the caption stops outside Talk range.

At 960 × 540 on the RTX 5050 Laptop GPU, both hardware RT backends presented the room view with seated Kit, the stand-in, active glazing, and the hearth and reward lantern in the uploaded light roster. The scene used 34 TLAS definitions and 25 BLAS owners. The RayTracingPipeline reply view also preserved the `hub.horde_army` state in its capture manifest. Kit reads mostly as a silhouette from the doorway. The original keeper asset, voiced playback, final scene lighting and phone performance need their own checks. The live approach, welcome and movement remain covered by the separate authored route test.

The table now has an authored candle, holder and wick in the editable tavern source. The recipe exports the wick and light anchors with the furniture, and the shared simulation uses the existing fire path to light the room. The two town fires share the fixed upload budget with the player's light. The lookout is outside their activation distance, and the tomb torches keep their existing triggers. The town export gained 100 solid triangles within the existing four material rows. The window export is byte-identical.

In a matched doorway view, doubling the hearth output mainly brightened its masonry and Kit's cast shadow. The retained candle improved the seat and table outline with the original hearth output. Both Windows RT backends presented the candle, hearth and player lantern at the same camera and 960 × 540 settings. The room still used 34 TLAS definitions and 25 BLAS owners. The native fire, town geometry, authored route and package-policy checks passed. The Android debug APK contains the exact tavern export. Phone rendering and sustained performance remain open.

The closer Windows RayTracingPipeline view replayed the 1.7 lookout continuation and restored the earned tavern seat. At the conversation mark, the candle brings out Kit's hood and armour while keeping the room dim. The earlier view is within 6 mm of the new camera position and has the same angles and seated pose. The staffed reply view also presented with all three lights and retained the `hub.horde_army` caption.

During `hub.kit_introduce`, Kit now glances toward the light inside the player's lantern cage. The shared simulation follows the held pendulum and the authored Light socket offset. The seated clip still owns his hands, feet and body contact. The cue eases in and out during the caption, pauses with the scene and settles when the line ends or is skipped. The Windows `bellwether-kit-lantern-glance` checkpoint restores the authored tavern seat and presses Talk with the development keeper stand-in. Both hardware RT backends presented the same seated pose and roughly 15° of additive head yaw at 960 × 540. The native rig test verifies the seated Head position and checks that adding the glance preserves all vertices below one metre. The Android debug build passed. Voiced timing, continuous motion review and physical phone performance remain open.

The `bellwether-repeat-visit` replay continues from a 1.7 lookout record through the actual road and south door. It observes all three roadside captions and the doorway welcome, waits for Kit to sit and the keeper to reach the hearth, then plays the full exchange after Talk. It exits to the square and returns twice on foot in the same game session. Both return attempts keep Kit seated, the keeper at the hearth and the completed exchange quiet. The host check runs the same replay on authored terrain. Both Windows hardware RT backends presented the final room at 960 × 540 with the development keeper stand-in, glazing and all three lights. Each capture records two quiet returns and the same final simulation tick. The Windows frames cover the final room, with continuous rendered motion and sustained phone performance still open.

The campaign save owner now queues an autosave when more of the tavern exchange resolves at the same boundary. The disk regression opens a slot with the introduction complete, finishes the last two lines, confirms that quitting waits for the queued write, and reopens the same slot with all three lines complete. This uses the existing save schema. The Android debug build passed with the save fix.

The Windows **doorway measurement** starts from the completed repeat-visit checkpoint, advances the ordinary motor at 60 Hz and presents frames while walking out to the square and back twice. The hidden Debug run uses the keeper stand-in and the current High diagnostic shaders at 960 × 540. The journal joins GPU completion to each successful presentation submission. It records the room fire count, active Kit/window/keeper instance masks, and allocated RT resources. Its hidden window leaves display pacing unavailable, and the Debug CPU can fall behind the requested tick clock.

Use [`tools/measure-chapter-shipping-route.ps1`](../../../tools/measure-chapter-shipping-route.ps1) with `-Configuration Debug`, `-BellwetherResidentFixture`, an exact `-Executable`, and a fresh `-Output`. `-Query` selects RayQueryCompute. Run [`tools/analyze-chapter-route-measurement.py`](../../../tools/analyze-chapter-route-measurement.py) with `--bellwether-doorway` and the output directory. The launcher hashes the binary and fixture, and the verifier checks the complete sequence of returned frames before reporting phase GPU statistics.

The doorway replay also accepts the two-role directory from `bellwether-return-to-kit`. The launcher hashes each role's GLB, manifest, and available Windows texture arrays. For this configuration the verifier requires the record keeper's submitted instance in every frame. The native journal looks up submitted metadata IDs for Kit, the window and both residents so that inserting unrelated props into the TLAS cannot change which instances it measures.

On the RTX 5050 Laptop GPU, RayTracingPipeline and RayQueryCompute each completed both live trips in 5,232 simulation ticks. The runs covered 1,580 and 1,586 successfully presented frames respectively, all with valid GPU timing. After the first five seconds, room GPU p95 was 5.04 ms for Pipeline and 4.75 ms for Query. At the square it was 11.49 ms and 10.91 ms. Both used 34 TLAS definitions, 25 BLAS owners and 422,934,704 bytes of device-local RT allocations across the route. Kit, the window and the stand-in stayed live throughout. Three lights were uploaded in the room and two at the square. These are single hidden Debug runs. Production pacing, the final keeper and physical phone budgets still need their own evidence.

The northwest approach now has a west-facing **CHURCH** arrow at the actual fork. The original post and chisel-stroke lettering are part of the editable Bellwether tavern source. The closed meshes add 364 triangles to the shared render and contact geometry and use the existing timber and char finishes. The wider timber inlay is clearer at the fork than the trial stone lettering in a matched Windows RT view. Both hardware RT backends presented it at 960 × 540 using the normal night and carried lantern.

The [north route](../../../src/gameplay/traversal/BellwetherNorthRoute.h) uses the existing map transform and terrain support. The Windows shrine checkpoint walks from a completed tavern save to the wayside forecourt with Kit still seated. The broader native test stopped before the church-side service mark after earlier tavern exchange failures. Both failures also reproduced at the pre-register revision `04c012fe`. The route test now waits for Talk after the keeper completes her walk to the hearth. At the southwest corner of the church, a 2.2 m stone approach follows the mapped fork and turn to the existing record keeper mark. Its deck and retaining sides share render and contact geometry from the editable town master. It adds 58 triangles using the existing stone material. A ground sweep accepts the deck over a steep bank only where continuous higher walkable triangles cover the bank. The `bellwether-church-approach` debug view walks from the earned tavern save to that service mark. The complete host route now walks from the lookout through the staffed tavern, reaches the church service mark, recovers the register at the shrine, and retraces the route with Kit still seated. Both Windows hardware RT backends presented the stone approach from the church service mark at 960 × 540. The Android ARM64 debug package contains the matching export. Physical phone performance and the record keeper encounter remain open.

After the three-line exchange, **Directions** at Kit’s chair points to the record keeper by the northern church and names the wayside shrine. The player can read the note again on a quiet return or after reopening a completed tavern autosave. The note is text-only and stays readable with speech subtitles off. It introduces no new spoken line or lantern reveal.

The **wayside archive prototype** sits outside B09 on its south-facing spur. The editable town master retains the mapped shrine shell and adds a low, hollow masonry compartment, a separate lower stone and a left hinge partly covered by roots. The authoring receipt records the face, hinge and inspection marks. The player can reach the inspection mark from the completed tavern visit by normal movement. The lower stone is a separate opaque mesh exported around its real hinge. The shared simulation uses the same local triangles and pose for player contact and hardware RT. With Kit settled and the hearth exchange resolved, facing the stone at the authored mark offers **Inspect archive**, then **Open lower stone**. The one-second outward swing pauses with the game and waits if the player enters its path. The normal tavern autosave retains the inspected or open state. A reload during the swing restores the inspected state and allows the action again. Windows Pipeline and RayQuery captures at 960 × 540 both walk from the completed tavern checkpoint and open the stone through the shared interaction handler. The record keeper encounter remains open. The [English bank](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/dialogue/en/Horde-Dialogue-Bank-English-en.json) provides the draft scene order and the later register handoff.

The first lantern scene uses the four English bank lines as draft captions. **Inspect archive** delivers the practical hinge hint while the cover is closed. The identity claim, name and request follow once the cover opens with the player still in reach. The speaker is labelled **Lantern voice** and Kit remains seated in town. The save records the start of the claim so leaving or reloading discards the pending lines. Older saves with an open cover offer **Listen to the lantern** at the shrine. An explicit recap recalls the request without replaying the name. The draft captions stay readable with speech subtitles off because this build has no recorded takes for the scene.

The **road register** rests on the sill behind the stone as a separate opaque leather-and-parchment mesh from the same editable town master. Once the reveal has started and the player is free to interact, **Take the road register** requires a clear reach through the opening. It removes the register from the RT scene and delivers the bank's `shrine.recap_register` caption directing the player back to Bellwether. Recovery updates the earned tavern autosave independently of the caption, preserving the stored position and resources. The pickup alone grants no route knowledge to Kit or the record keeper. An older open-archive save still contains the book. After pickup, the normal lantern recap remains available.

Matched 960 × 540 Windows captures on the RTX 5050 Laptop GPU show the register on the sill and the empty niche after interaction on both RayTracingPipeline and RayQueryCompute. The second capture requires the simulation to record recovery in the earned tavern checkpoint. The four captures use the same camera and exercise the staged walk from that checkpoint. Android device performance still needs validation.

The local record keeper review package includes a rigged Meshy model, PBR maps, an editable Blender assembly and voice takes. Its original zero-duration pose and stock walk/run are source material.

### Church register handoff

The record keeper's shared simulation pose uses the existing church service mark. A facing and clear-reach check offers **Show the road mark** before the shrine, based on the forest clue already earned on the lookout route. The three English bank lines direct the player to the shrine. Walking away or losing earshot cancels the captions.

The collected register offers **Show the road register**. That deliberate handoff records the interpretation at the earned tavern save and starts the bank's three reading lines. The Abbey downstream and the Foundry across the eastern gorge can be pursued in either order. The Court's route remains sealed. **Recall the expedition routes** repeats the reading, and the tavern Directions note also reflects the player's progress. These captions use the written English bank.

The `bellwether-record-keeper-handoff` debug view loads a local resident fixture. It walks from the tavern to the shrine, recovers the register, returns to the church and verifies the handoff in simulation and in the save latch. The full native route test also verifies the road-mark lead before pickup and the return reading after pickup.

The local fixture now uses the prepared 15,322-triangle record keeper. Its four-second idle preserves the standing stance, settles the arms using the supplied rig, and adds a small breathing motion and head glance. The shared simulation advances the idle in earshot and holds it while paused or outside earshot. [The source recipe](../../../tools/author-record-keeper-idle.py) preserves the rigged and walking GLBs and writes an editable Blend and a separate local export.

Both Windows hardware RT backends presented the textured actor at 960 × 540 and recorded the same idle time and completed interpretation. The exact-byte texture check verified all 11 mips and the unchanged preceding layers in both Windows RGBA8 and Android ASTC. One resident adds 16,777,212 bytes of Windows texture payload and 2,650,704 bytes of Android texture payload. The Android ARM64 debug package built without the local fixture. Phone performance, model and voice distribution clearance, and normal package integration remain open.

### The lantern speaks to Kit

After hearing the shrine claim, return to Kit's table and raise the lantern on the local prompt. If the lantern is already up, the prompt asks the player to lower it first. Both platforms use the existing raise control and require a newly presented high pose. Each of the two lantern lines then comes from the held lantern, followed by Kit's four replies from his seat. Kit looks toward the lamp from his seated pose. The six captions follow the English bank's `hub.lantern_introduction_to_kit` order. The spoken identity remains the lantern's claim.

The register reading and Kit's firsthand conversation open independently. Each line that starts in earshot updates the existing tavern save, with the saved pose and resources preserved. Leaving or lowering the lantern cancels pending lines, and a fresh raise lets the player continue from the next line. Loading an older save leaves Kit awaiting the introduction. A partial or complete exchange loads quietly. The local audio inventory has a provisional `lantern.town_seals` clip and three Kit review lines. The exact `lantern.town_identity` and `kit.royal_claim.king` takes were absent from the reviewed intake. Voiced playback and phone playtesting remain open.

The hidden Windows `bellwether-return-to-kit` checkpoint starts from the earned tavern save, walks to the shrine for the register, visits the record keeper and walks back to Kit's chair. Pass `--chapter-resident-fixture` a local directory with `tavern-keeper` and `record-keeper` subdirectories. Each holds a `resident.glb` and its manifest. The prepared record keeper also uses his Windows texture arrays. The fixture currently reuses the older tavern keeper stand-in.

The replay raises the lantern through the normal input command. The exchange waits for an acknowledged hardware RT presentation of that pose. The capture then advances to Kit's first reply and verifies that the save retains both the register interpretation and the start of Kit's response. The manifest records the raised-lantern presentation tick and the later reply frame.

At 960 × 540 on the RTX 5050 Laptop GPU, both RayTracingPipeline and RayQueryCompute acknowledged the raise at simulation tick 8,822 and presented the reply at tick 8,985. All three actor instances were active, with 37 TLAS definitions, 28 BLAS owners, three uploaded lights and 441,697,264 bytes in device-local RT allocations on each backend. The fixed-step route stages the reply view before capture. Continuous route performance, streaming retirement, the final tavern keeper and phone validation remain open.

### Measured doorway route with three actors

With the textured record keeper and tavern keeper stand-in loaded, both Windows hardware RT backends completed two live doorway round trips on the RTX 5050 Laptop GPU. The 960 × 540 runs used the same Debug executable and Diagnostic High profile. RayTracingPipeline presented 1,575 frames over 97.33 seconds and RayQueryCompute presented 1,565 over 98.58 seconds. All frames had valid GPU timings, joined to their owning presentations. The submitted masks for Kit, both residents and the window stayed active in every measured frame. Each run advanced 5,232 simulation ticks.

| GPU p95 after the five-second warmup | RayTracingPipeline | RayQueryCompute |
| --- | ---: | ---: |
| Tavern | 5.06 ms | 4.72 ms |
| Outbound doorway | 5.33 ms | 4.81 ms |
| Square | 11.47 ms | 10.74 ms |
| Inbound doorway | 10.74 ms | 10.52 ms |

Both runs held 37 TLAS definitions, 28 BLAS owners and 441,697,264 bytes of device-local RT allocations throughout. A negative replay of the journal rejected a missing record keeper mask at the square. Display pacing, streaming retirement, the final tavern keeper and physical phone performance remain open.

The [separate ARM64 Bellwether Debug build](../../../android/README.md#bellwether-three-actor-playtest) now uses the same two prepared residents on the phone. Both staged checkpoints entered live controls on SM-S948B, with Kit visible at the tavern and the textured record keeper visible outside the church. Short native timings on the phone identify a performance gap at the tavern threshold. The full moving route, displayed pacing and sustained performance remain open. See the [exact Android device record](../../ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#2026-11-21---bellwether-three-actor-debug-checkpoints).

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

The Windows Debug option `--bellwether-voice-dir <local WAV directory>` and the [private Android voice overlay](../../../android/README.md#bellwether-three-actor-playtest) admit the same five exact WAVs. The three hearth lines and both of Kit's first-conversation lines use durations measured from the approved mono PCM at 24 kHz. The spatial voice path uses each line's existing actor source and the Voice preference. Bellwether captions remain available whenever the voice is missing, muted or fails to start. The keeper and the three approach lines still use captions. Exact-build listening and public distribution clearance remain open.

The private audio masters already contain the four approach and doorway pickups and the keeper's `hub.horde_army` reply. The five WAV hashes match their production manifests. Each uses 24 kHz mono PCM16 and its manifest text matches the runtime caption. All five await listening approval. The doorway take lasts 3.699333 s and the reply lasts 5.579333 s. When admitted, use the measured durations and verify the welcome and conversation timing with the existing actor marks and caption fallback.

Keep original MP3s, per-line review WAVs, exact text/voice/cost manifests and a short pickup listening reel in the existing private Horde audio archive. Do not overwrite accepted runtime audio or insert new clips into the game as part of this planning task. Use the pickup receipt for exact production/QC status; this plan does not claim runtime admission, licence clearance for wider distribution, or performance approval.

## Animation and gameplay acceptance gate

First author a short test on the real Kit rig: loose-ground step/catch, protective standing assessment, several injured steps, turn and sit. Keep editable sources. Review normal first-person, front and side views for weight, planted feet, contact and coherent transitions before expanding into the complete scene. A flattering render is insufficient.

Then validate the whole slice in-engine with representative fire/lantern/glass/actors and exterior overlap on Android and Windows. Check looking away, overtaking, stopping/backtracking, repeated visits, subtitle-only/missing audio, skip/pause/quit/reload and door/collision recovery. Persist injury/arrival/settled state without duplicating the incident or trapping the player. Preserve real RT, shared simulation and existing scene/streaming contracts. No build, animation implementation, merge or release is authorized by this document's completion.
