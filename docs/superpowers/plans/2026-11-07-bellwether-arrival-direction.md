# Bellwether arrival: locked directing direction

**Owner approval: 7 November 2026. Planning and voice production, not implementation or animation acceptance.** The [shared roadmap](../../ROADMAP.md#18---bellwether-npcs-and-light-rpg-systems) remains the scope authority. Sam approved the 7 November lookout-to-town screenplay with: “ok YES lock this in. we likely need some more lines produces via Mureak - can you organise that now?”

The approved direction is a continuous, largely player-controlled sequence: existing lookout, loose-ground ankle injury, short independent protective walk, square, south tavern door, chair by the hearth, then a player-initiated conversation. This record supersedes the older undecided injury-cause and offered-arm draft, without treating a plan or recording as a finished scene.

**1.8 kickoff, 21 November 2026:** all four pickups below are now present in the private production archive. The receipt, originals, review WAVs, script and listening reels match the archived SHA-256 inventory. The new takes still need listening approval and release admission. The five previously approved Kit lines remain the reuse source for the hearth and first conversation.

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

Use `--bellwether-verify-expedition-leads` in place of `--bellwether-earn-tavern` to continue from the earned tavern through the northern church, wayside archive, register handoff and return to Kit. It uses the ordinary movement, interact and dialogue-skip input sequences. The raised lantern must earn a hardware RT present before Kit's introduction can begin. The final Pause → Load verifies the two discoveries together and checks that the tavern anchor, vitality and flask survive the outing. Each run uses its own freshly exported fixture.

Both standard Windows hardware RT backends passed all 16 checks from the lookout menu load through the final tavern reload. The final save kept the interpreted register and all six lantern exchange lines without replaying speech on load or changing the saved bytes. This is hidden Windows campaign and save evidence. Phone controls, listening and sustained performance still need their own checks.

### Bellwether directions in the pause menu

**Bellwether directions** is available in the pause menu once Kit takes the road.
It points to the south tavern door, then to the hearth and **Talk** when Kit is
seated and the keeper is available. Completing Talk opens the church and the
preparation leads. At the shrine the page follows the lower stone, lantern and
road register. An already-open shrine whose claim is still unheard points to
**Listen to the lantern**. After the reading the page shows the downstream
Abbey and eastern Foundry as leads for future expeditions. Reloading uses the
current campaign progress without an extra save field.

Directions lists the mill workbench and shore practice when the artificer and salvager are part of the playable roster. Either visit can start before the church route. The directions keep the west stall sale location visible whenever the player is carrying ordinary silver, including during the shrine and register steps.

Windows and Android use the same text. **Back to pause menu** keeps the game paused, and **Continue exploring** resumes through the existing pause controls. An action on an old directions page is revoked when the surface, world or objective changes.

The expanded Windows campaign check passed 20 assertions on each hardware RT backend. It opened the directions at the tavern, shrine and church, then reloaded the earned tavern save and checked the same expedition leads. Ten Android menu tests covered the pause actions and stale UI, and the ARM64 Debug package contains the matching native text endpoint. Physical Android remains pending.

### Rest by the hearth

After the first tavern Talk, the authored standing mark by the hearth offers deliberate Rest. Stand close and still with guard released, then use **Rest** on touch, **J** on a keyboard or **D-pad Down** on a controller. The existing two-second recovery restores the player's vitality and fills the flask to its current capacity. Pausing or leaving the action early preserves the old resources. When Rest changes those resources, the game autosaves at the same earned tavern checkpoint with the resolved conversation and existing progress.

This is the tavern's first practical service and uses the [approved flask policy](../../design/HORDE_1_7_FLASK_POLICY_PROPOSAL_2026_10_24.md). Rest is free. Kit's ankle recovery and the wider service economy still have open design decisions. The private Android proof can combine the pinned resident, voice and flask inputs under a separate app ID with its own saves.

The Windows hidden campaign route passed 13 checks on each hardware RT backend with the pinned flask and Bellwether residents. It earned Talk by walking from the lookout, then rested, saved and reloaded. A separate test on authored tavern geometry covered the injured player, interruption and the two-second duration. Four Android control-state tests passed. The local ARM64 Debug APK contains the exact flask, resident, voice and furniture assets. Physical phone performance is pending.

For the combined campaign check, pass `--bellwether-verify-hearth-rest` together with `--bellwether-verify-expedition-leads` in a flask-enabled Windows Debug build. The player earns the tavern, rests, reloads, then walks the church and shrine route and returns to Kit. The final reload verifies the refilled flask alongside both discoveries and the directions page. The route advances its own 60 Hz inputs and verifies that rendering leaves the scripted simulation tick unchanged.

Both Windows hardware RT backends passed all seven assertions on the same Debug executable (SHA-256 `4f352112e829e96b67a08b4a8581bad1629596bd112a652edbe43c56212e9606`). The load presented a fresh world generation and resumed automatically with Kit seated, the keeper at the hearth, Talk complete and Directions available. The resumed autosave matched the bytes written by that run. This starts from a seeded lookout in a hidden window using the keeper stand-in and muted audio. Android device play and sustained performance remain separate open checks.

The editable tavern blockout is derived from the v4 master. It retains the floor height and roofline and fits an 11.5 m stone ramp to the current terrain. The ramp rises roughly 3.26 m from its buried toe to the 2.2 m wide south masonry opening. The ramp, floor, building solids and west-wall hearth share the native render geometry with the movement system. See the [Blender recipe](../../../tools/author-bellwether-tavern.py) and [asset receipt](../../../assets/models/world/source/bellwether-tavern-v1/authoring-receipt.json).

The south entrance has an eight-plank timber leaf, jambs, hinge straps and a pull on each face. The jambs leave a 2.064 m wide and 2.582 m high throat. The leaf starts open at 94° inward against the west jamb. Once Kit sits and the keeper clears the doorway, the shared Interact action opens or closes the door from either side. The native route verifies that the door pauses without pushing the player, finishes closing after the player sidesteps past the leaf, and reopens from the room. Checkpoint loads start with the door open. The welcome and seated conversation work without operating it.

The 264-triangle moving leaf uses one immutable BLAS, one TLAS transform and two untextured material rows. The same pose drives movement and carried-item contact. The 60 frame and pintle triangles stay with the static town. The window and wayside exports retain their previous bytes. The Windows `bellwether-south-door` checkpoint looks back through the open entrance. The staffed `bellwether-south-door-closed` checkpoint walks the arrival, completes Talk, then closes the door through the shared input path.

The west-wall hearth uses the shared fire system above three charred billets. Original timber furniture stages Kit north of the hearth, clear of the doorway. The 49 cm chair and 75 cm table are solid parts of the rendered room. The authored turn, approach and conversation marks were reached by the normal player motor and the route back through the south door passed. The Windows Diagnostic capture shows the furniture and both fire sources in a 960 × 540 hardware RayTracingPipeline frame on an RTX 5050 Laptop GPU. Final doorway light, smoke and audio still need scene review with the actors present. Android performance and owner playtesting remain open.

The shared incidental audio voice now plays a quiet six-second crackle from the lit hearth. It starts after 1.75 seconds on the actual tavern floor within 9 m of the fire and leaves at least three seconds between clips. Crossing the inner face of the south doorway separates the room from outdoor nature audio. Talk, pause, load and leaving the room interrupt the cue. Returning or finishing a line starts a fresh quiet interval. The [source and PCM receipt](../../../assets/audio/bellwether-hearth/asset.manifest.json) pin the CC0 recording and its bounded WAV. Listening and mix acceptance on the Windows and phone builds remain open.

The south-wall window uses an original timber frame and one textureless, closed 8 mm glass volume from the same Blender source as the room. The glass adds one material, one 12-triangle BLAS and one live TLAS definition. The existing Glass setting controls its ray visibility, with solid contact in both modes. The exterior route retains it for secondary rays through the approach and return. `bellwether-tavern-window` walks the authored route inside and looks back at the pane and the open south door. Use Glass On explicitly for the combined glass-and-fire phone checkpoint.

Windows Diagnostic presented this view on the RTX 5050 Laptop GPU at 960 × 540 using both RayTracingPipeline and RayQueryCompute. The matched RayTracingPipeline views changed the glass instance mask from 1 to 0 with Glass Off while preserving the road-stone instance and the view through the open doorway. The full chapter scene used 33 live TLAS definitions and 24 BLAS owners with the pane resident. The frame still reads very dark inside. It leaves combined lighting with the keeper, sustained performance and mobile presentation open.

On the south ramp, the flat carry sweep read the rising floor as a wall after 0.64 m. The shared sweep now follows the standing slope for upward walkable faces. The host fixture recovered the full 2.7 m carry lookahead, and the matched Windows RT view recovered 22,119 primary arm pixels from zero. Separate host collision tests still reject raised ground and low ceilings.

The editable [Kit hearth sit](../../../assets/models/characters/source/kit-hearth-sit-study-v1/README.md) establishes a planted stance on the actual seat. The native clip appends to the existing Kit payload, preserving the earlier mesh, skin and animation bytes. Eight native samples agreed with Blender to within 0.026 mm. The current Windows RTX seated view restores a completed 1.7 lookout record through the save transaction, then runs both actors through the actual road and south door. Kit completed the 16-station route, turned into the measured seat and held the final frame at 1.367 s. Kit reads mostly in silhouette under the current hearth and player lantern, so the final conversation needs a dedicated lighting review.

The earned lookout stays quiet until the player takes the road. Kit uses the same terrain and obstacle checks, continues if the player moves ahead, and stops once at the lower-road assessment mark. The [editable road actions](../../../assets/models/characters/source/kit-ankle-approach-v1/README.md) are connected to that stop and the remaining walk. The assessment lasts 1.6 seconds and the protective gait advances with 0.76 m of admitted travel per cycle. The Windows RTX `bellwether-kit-ankle-catch` view shows the catch at 0.33 s on the real road. The silhouette reads more clearly than the foot under the current night lighting. The supported chair turn and sit are connected in campaign play.

The fixed-step catch emits a [boot-and-gravel scrape and an unvoiced recovery breath](../../../assets/audio/kit-ankle/asset.manifest.json) from Kit's position. The native route uses the [loose-stone and ankle-fit study](../../../assets/models/world/source/bellwether-loose-road-stone-v1/README.md). The stone moves 284 mm on the existing assessment clock, settles by the breath and stays settled through the route and tavern reloads. It adds one 42-triangle BLAS and one live TLAS instance, using the shared static material path without a texture layer. The Windows RTX catch presented with 32 live instances at 960 × 540. The dark road still makes the contact subtle.

The three roadside lines use named captions for a player within five metres of Kit and on nearby ground. The first follows the breath cue, Kit holds his supported stance through the assessment, and the last line accompanies the start of his protective walk. The player keeps control and can walk on. Leaving the area ends the exchange and Kit continues without repeating it. The private voice audition uses the same triggers and the clock uses the measured take lengths. The foley mix and final pacing need playtesting.

After the assessment and any active roadside line end, the existing lookout autosave records the completed beat. Loading this save puts the player back at the earned lookout with the saved resources. Kit resumes his protective walk from the authored roadside mark. The stone stays settled and the injury, roadside captions and foley stay complete. Schema 20 appends the assessment flag. Earlier lookout records still begin before the incident, and the earned tavern save still resumes with Kit seated.

The hidden Windows case `--bellwether-verify-ankle-restore` uses the same fresh lookout fixture and campaign launch as `--bellwether-earn-tavern`. Both hardware RT backends passed all 12 checks. The case observed the injury on foot, checked the lookout autosave bytes, reloaded through the native pause menu, then earned and reloaded the completed tavern exchange. Phone play and listening remain open.

Once Kit is seated and the player reaches the safe area by the hearth, the ordinary campaign save system earns a **Bellwether tavern** checkpoint. The player restores at the fixed standing mark and Kit restores in his settled pose against the current authored chair. Later safe-side saves retain the seated milestone. Older lookout saves retain their original layout and still begin before the journey. The Windows RTX route diagnostic earned this checkpoint on foot and presented the restored seated scene. A separate disk round-trip checked a fresh session, including rejection when the chair was missing.

The native forest replay walks back from an earned Bellwether visit to
the original rope. It descends, encodes and decodes the lower checkpoint,
reloads it and climbs back outside. The check requires Kit to remain seated
in Bellwether and the register reading, air ward, fitted hinge, purse, mill
bearing and lantern exchange to survive both safe checkpoints.

Kit's three hearth lines now run as named captions. The first accompanies the lowering, then Kit explains his limits and pauses before offering help. A player who arrives after the sit sees the two seated lines. The tavern autosave tracks resolved lines, so a reload resumes at the first unresolved beat. Audio/haptic manual revalidation required: YES, for the hearth timing, captions and in-room mix. The reviewed recordings remain in private staging.

The shared interaction and save path supports the three-line exchange between seated Kit and the tavern keeper. Talk appears only after the hearth lines resolve and both speakers reach their marks. Leaving the hearth or reloading stops the exchange. Choosing Talk again resumes at the first unresolved line. The [editable tavern](../../../assets/models/world/source/bellwether-tavern-v1/bellwether-tavern-v1.blend) marks the keeper beside the south doorway and across from Kit at the hearth, with an aisle route between them. The shared motor walks that path after Kit sits and restores the keeper at the hearth from an earned tavern save. The private campaign enables Talk with the stand-in. The final keeper model and the keeper's voiced lines remain open.

The November character intake records four previously retained town NPCs and an October archive transfer problem. The existing record keeper study is available and supplies the textured runtime fixture. The original tavern keeper package and its rig and motion remain unverified. The signed-in Meshy model search returned no named result under tavern, keeper, caravan or Bellwether. Blender inspection identified the two recent unnamed API results as the base mesh and rig of the same armoured male character. This lookup used zero credits.

The private tavern keeper concept study preserves a front T-pose, the source prompt and an exact image receipt (SHA-256 `1c6fdce9377a6160d8d8e0e4f7c7bdf82bb468a2c7ed60457bdb930cfeb50922`). The clothing is fitted olive wool, ivory linen, a flat leather pouch and separate trousers and boots. This keeps the former caravan leader's outline clear at the doorway, with separate limbs for rigging.

The private textured keeper candidate uses that image. Meshy T2.5 generated a 15,421-triangle humanoid with Meshy 7 PBR maps for 15 credits. Rigging at 1.70 m added 24 joints and supplied a walking cycle for 5 more credits. [The Blender authoring tool](../../../tools/author-bellwether-keeper.py) builds a grounded standing pose and retimes the stock walk from measured mid-stance motion to the existing 1.08 m/s route. The candidate uses one 1K mipmapped PBR layer per map alongside the record keeper's layers. The original and prepared files, editable Blender assembly, pose and provider receipts are retained in the private keeper source archive. The Debug campaign accepts the exact textured keeper or the earlier stand-in. Both Windows hardware RT backends presented the candidate's existing hearth exchange. Phone playback, doorstep motion and lighting review, the new voice takes and distribution clearance remain open.

In the staffed fixture, the keeper's chair greeting plays as a caption when Kit approaches the south door and the player is within earshot. The current keeper mark is audible from the sill and upper ramp. Its line to a listener farther down the ramp crosses the east jamb and is blocked, as is the path through the adjacent wall. The geometry check uses the gameplay clearance rule and grounded listener height. Moving on ends the cue without a repeat on backtracking. Kit keeps walking through it. The cue uses the same admitted-keeper gate as Talk and stays out of saved playback state.

The keeper's resident render pose now comes from the shared simulation. Each admitted walking step advances the clip clock by its travelled distance at the staged walking speed. The door and hearth use the resident's idle pose. The Windows hidden views `bellwether-keeper-reply` and `bellwether-staffed-room` use `--chapter-resident-fixture <directory>` with a local textureless Kit stand-in. Both restore an already-earned tavern record through the normal save transaction. They take the Talk and Skip input edges and verify that the reply originates at the keeper's authored hearth mark. The reply view freezes there. The room view walks back through the real interior and verifies that the caption stops outside Talk range.

At 960 × 540 on the RTX 5050 Laptop GPU, both hardware RT backends presented the room view with seated Kit, the stand-in, active glazing, and the hearth and reward lantern in the uploaded light roster. The scene used 34 TLAS definitions and 25 BLAS owners. The RayTracingPipeline reply view also preserved the `hub.horde_army` state in its capture manifest. Kit reads mostly as a silhouette from the doorway. The original keeper asset, voiced playback, final scene lighting and phone performance need their own checks. The live approach, welcome and movement remain covered by the separate authored route test.

The table now has an authored candle, holder and wick in the editable tavern source. The recipe exports the wick and light anchors with the furniture, and the shared simulation uses the existing fire path to light the room. The two town fires share the fixed upload budget with the player's light. The lookout is outside their activation distance, and the tomb torches keep their existing triggers. The town export gained 100 solid triangles within the existing four material rows. The window export is byte-identical.

In a matched doorway view, doubling the hearth output mainly brightened its masonry and Kit's cast shadow. The retained candle improved the seat and table outline with the original hearth output. Both Windows RT backends presented the candle, hearth and player lantern at the same camera and 960 × 540 settings. The room still used 34 TLAS definitions and 25 BLAS owners. The native fire, town geometry, authored route and package-policy checks passed. The Android debug APK contains the exact tavern export. Phone rendering and sustained performance remain open.

The closer Windows RayTracingPipeline view replayed the 1.7 lookout continuation and restored the earned tavern seat. At the conversation mark, the candle brings out Kit's hood and armour while keeping the room dim. The earlier view is within 6 mm of the new camera position and has the same angles and seated pose. The staffed reply view also presented with all three lights and retained the `hub.horde_army` caption.

The candle now stands on the floor beside the table, clear of Kit's seating
path. Raising it on the tabletop brightened the keeper's face and darkened
Kit's hood slightly. The floor position brings the flame closer to Kit while
keeping it a quarter metre above the original wick. Matched 960 × 540 native
captures on both Windows RT backends retained the actor poses, camera,
three-light roster and light strengths. In the same 8-bit luminance crops,
Kit's hood and face rose from 7.12 to 8.45, his upper body from 12.77 to
14.81, and the keeper's head and neck from 27.61 to 35.80. The keeper's
tunic stayed dark. The final candlestick adds 48 solid triangles and
10,496 bytes of device-local allocations in these captures. The door,
glass, register, shrine cover and all three furniture tiers are
byte-identical. The Android Debug package contains the matching room and
manifest. Phone rendering and performance remain open.

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

The record keeper turns in place toward a player who faces him within reach, then keeps addressing that position in earshot during the exchange. Pausing holds the facing and idle time. Leaving returns him to the church-path stance at the same grounded mark. The `bellwether-record-keeper-listener` capture reaches the handoff from 0.66 m off the original approach and waits for the keeper to face the listener. Both Windows hardware RT backends presented the same 21.1° turn and retained the saved register interpretation. The native route and dialogue suites passed. The private ARM64 build includes shared facing, with the physical phone check pending.

A private 23.025-second review reel collects the four available cuts in church scene order. The October `hub.first_errand` and `hub.shrine_direction` cuts still need a listening check at their split. The November `hub.two_routes` and `hub.routes_independent` takes still need listening review. `hub.record_inspect` is logged in a combined audition without a verified individual cut, and the November audit holds the exact `shrine.register` reading. The reel retains each cut's samples with one second between clips. A cue sheet and source hashes accompany it outside the repo. The church encounter continues to use captions.

The `bellwether-record-keeper-handoff` debug view loads a local resident fixture. It walks from the tavern to the shrine, recovers the register, returns to the church and verifies the handoff in simulation and in the save latch. The full native route test also verifies the road-mark lead before pickup and the return reading after pickup.

#### Wayside air ward preparation

The first register reading offers a Magic preparation for the Abbey. Return to the same point in front of the wayside shrine and choose **Restore shrine binding**. The player re-seats the existing lower stone. The reverse sweep uses the original hinge and collision. Closing stops if the player blocks it, and a save during the swing retains the last completed open pose.

When the stone finishes closing, **Bellwether directions** sends the player back to the record keeper. **Learn the air ward** records the lesson for the future Abbey crossing. Both steps save at the existing checkpoint. The player can finish them with an empty purse and still collect the register reward later. The collected register and the two expedition leads remain available, and the shrine still offers the lantern recap.

#### Register reward and coin purse

After the record keeper reads the recovered register, **Collect register reward** offers a one-time payment. The three-coin amount is provisional for the private playtest. The player collects it through the same interaction and sees the balance in **Bellwether directions**. After payment the keeper offers **Recall the expedition routes** again.

The payment and its collection record are saved together at the existing earned checkpoint. A schema 11 or 12 save with an interpreted register still offers the reward on the next visit. Repeated interaction or loading a paid checkpoint preserves the balance and the completed routes. The interpretation, the lantern conversation with Kit and both future leads stay independent of collecting the reward.

#### Lantern hinge fitting

With enough coins from the register reward or ordinary silver sale, approach the tavern keeper from the aisle side and face her from close range. **Fit lantern hinge (2 coins)** shows the provisional price before Interact. Kit's chair retains its own conversation and directions. **Bellwether directions** lists the fitting price and shows **Lantern hinge fitted** after purchase, alongside the remaining purse.

The Tech fitting increases damping in the shared 60 Hz hinge simulation. The carried body and its light follow the same solved hinge pose. The fitting and debit share one checkpoint transaction. Legacy purses keep their saved balance and can purchase on a later visit. Pricing and handling feel await playtesting, and the wider access training remains open.

The native host check loaded the earned tavern save before and after purchase. Each run walked 1.58 m and then stopped on the same input schedule. Summed swing over that sequence fell by 48% with the fitted hinge. The Windows campaign check also reads the quoted price from the native prompt, purchases once, and verifies the remaining coin and fitting after reload.

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

The open hinged-door baseline uses source `82ce5828` and Windows Debug executable SHA-256 `cd7cd0317bd8763da032b2a70ef2d048e2e8621d7ad1cea7932b7e0ce5ae7229`. At 960 × 540 on the same RTX 5050 Laptop GPU, Pipeline presented 1,578 measured frames in 97.82 seconds and Query presented 1,583 in 96.35 seconds. Each run advanced 5,232 simulation ticks and completed two returns. The open door, window, Kit and both residents had active masks throughout. The two runs each held 38 TLAS definitions, 29 BLAS owners and 441,767,696 device-local RT allocation bytes.

| GPU p95 after the five-second warmup | RayTracingPipeline | RayQueryCompute |
| --- | ---: | ---: |
| Tavern | 5.05 ms | 4.72 ms |
| Outbound doorway | 5.32 ms | 4.80 ms |
| Square | 11.50 ms | 10.80 ms |
| Inbound doorway | 10.98 ms | 10.10 ms |

The measurement rejects a closed or closing door in the input replay and requires the submitted door instance on both platforms. The Windows receipt pins the tavern and separate leaf exports. The verifier rejected a missing door during the outbound leg and a mismatched journal mask. The private ARM64 APK from the same source has SHA-256 `86aa207d8302cf678e0001644b8b4052993fdcc072d5b53fcf76ec7e409f9a6e`. The new baseline consists of hidden Windows GPU timings. Phone pacing and sustained performance await an on-device run.

The original [ARM64 Bellwether Debug build](../../../android/README.md#bellwether-three-actor-playtest) used the stand-in and the textured record keeper. Both staged checkpoints entered live controls on SM-S948B, with Kit visible at the tavern and the textured record keeper visible outside the church. Short native timings identify a performance gap at the tavern threshold. The textured tavern keeper's moving route, displayed pacing and sustained performance remain open. See the [exact Android device record](../../ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#2026-11-21---bellwether-three-actor-debug-checkpoints).

The [doorway verifier](../../../tools/analyze-chapter-route-measurement.py)
accepts `--compare-bellwether-keeper <stand-in-run> <textured-keeper-run>`
for a same-backend Windows pair. It verifies both complete routes and
checks the executable, settings, open door, window, roster, and fixed-step
range before comparing phase GPU p95 and allocated resources. The keeper is
standing at the hearth during this measurement. Use the separate native
arrival route to check her walk from the door.

The textured keeper comparison used the same `4af3dad3` Debug executable
(SHA-256 `6b36c335946a8f02d18a54c4fa6062b7750218856022435aa39e19cec6d33803`)
and Diagnostic High at 960 × 540 on the RTX 5050 Laptop GPU. Each of the four
runs completed two returns and 5,232 simulation ticks. All 5,758 presented
frames had valid GPU timings. Each pair held the same room, settings, Kit,
record keeper, window and open door.

| GPU p95 after the five-second warmup | Pipeline stand-in → textured | Query stand-in → textured |
| --- | ---: | ---: |
| Tavern | 8.06 → 8.11 ms | 7.55 → 7.56 ms |
| Outbound doorway | 9.24 → 9.19 ms | 8.29 → 8.61 ms |
| Square | 11.51 → 11.50 ms | 10.82 → 10.84 ms |
| Inbound doorway | 11.44 → 11.19 ms | 10.62 → 10.68 ms |

Both backends peaked at 38 TLAS definitions and 29 BLAS owners with either
keeper. Peak device-local allocations rose from 441,811,472 to
456,752,912 bytes with the textured keeper. Each cell compares one run per
variant. Displayed pacing and sustained phone performance remain open. The
raw runs and both comparison receipts are retained locally under
`bellwether-keeper-matched-20261122-032422`.

### East route to the mill yard

The [east route](../../../src/gameplay/traversal/BellwetherEastRoute.h) starts below the tavern's south ramp and passes the fork to the mapped salvager's spot. It follows the tavern's east flank north before descending into the yard west of the closed B05 mill. The original diagonal crosses a steep break in the imported ground, so this route follows a walkable contour on the existing terrain. The shared 60 Hz player motor completes the trip to the salvager's spot and the mill yard and walks back with Kit seated and the earned tavern checkpoint intact.

The square and the eastern fork now have timber **MILL** arrows. The working edge has the mapped west entrance, a stone landing with a buried approach, and a timber bench with an empty bearing cradle. The player can walk to the bench and back to the landing. The original mill footprint stays closed. The landing, door and bench share render and contact geometry and reuse the existing material rows. The editable town master and its authoring receipt record the work mark and surveyed heights.

The `bellwether-mill-yard` development checkpoint restores a completed tavern visit, walks to the landing and workbench, then steps back to frame the exterior. Both Windows hardware RT backends presented the yard at 960 × 540.

#### Mill bearing preparation

After the hearth conversation, the player can **Inspect mill gearing** at the workbench. Bellwether Directions points to an open spare-parts bin south of the mill path. **Take a spare bearing** records the selected part, and **Check bearing fit** compares it with the retained reference at the bench. Each step updates the earned return record while preserving its position and resources. Earlier saves still start with the gearing unexamined. The fit check is optional and independent of the shrine ward and lantern fitting. It leaves the bearing ready for the [artificer's workbench repair](#workbench-repair-and-rig-hand-off).

The two fixtures add 180 triangles to the shared town mesh and reuse its existing materials. Both Windows hardware RT backends presented the bench reference, and the ARM64 debug package contains the same export. Save tests cover earlier records and all three updates to the existing autosave slot.

#### River practice landing

A 1.9 m stone causeway starts just south of the salvager's existing mark, follows the mill's south edge and reaches the west bank of the main river. A timber rail marks the end of the terrace. The river covers the lower stone platform by about 20 cm. The shared collision and render mesh gives the player a continuous walk to and from dry ground.

Shallow contact queries the existing, finite Vulkan water faces and compares their height with the grounded player's feet. Wet steps use the surface height for the sound source and splash droplets. The breathing rig trial and the salvager's shore practice share the landing. The separate millrace sits upstream.

#### Salvager shore practice

After the opening tavern conversation, the salvager stands by the river fork on the east route. Talk starts a walk to the last dry stone. The first grounded contact with the marked shallows starts an eight-second practice breath. Reach the rail and return to the dry stone while the cue shows time remaining. A player who runs out of time can recover on the stone and try again with their health and gear intact. Turning back early also resets the attempt. The shared 60 Hz simulation pauses the breath timer with the game.

Reporting back records breath control in the existing safe tavern autosave. Earlier earned lessons remain complete. A load returns to that safe anchor, where unfinished drills can be restarted. At the same rail, a player with the rig can choose the seal test after a breath attempt expires. The three authored lines retain their named captions and optional private audition with positional audio.

The standalone shore Debug roster pairs the salvager with the tavern keeper and Kit. The combined campaign uses the same salvager and opens Bellwether Directions after the tavern conversation, with a shore entry before the church handoff. The entry follows the current stage on the causeway and shows completion after the lesson is saved. The eight-second window is provisional for the shallow Bellwether rehearsal. The submerged route and its exposure limits belong to the Abbey chapter.

Bellwether interactions choose a target using the camera input from the same
tick as Interact. The salvager and dealer checks cover turning away and
interacting together. Shore practice starts its breath clock at eligible wet
contact before it can advance to the retreat, including on a retry.

The host exercised expiration, dry recovery, retry, and switching to the rig at the same rail. The full Windows campaign passed 36 checks on each hardware RT backend, with the native countdown at the wet rail, a paused timer and the three preparations retained after the final reload. The private ARM64 build passed 14 Android checks, including the compact countdown and recovery cues at maximum font scale. Phone playtesting remains open.

Both Windows hardware RT backends presented the fork at 960 × 540 with all three resident instance masks active. The ARM64 Debug APK packages the same salvager mesh and the three private WAVs, with single-layer mipmapped ASTC textures. The initial night capture left the salvager's face dark at rest. Voice balance and sustained phone performance still need checking.

A hooded oil lamp sits on the dry side of the fork and faces the salvager. The Blender master adds the post, rain shield, wick and anchors to the same town geometry. The runtime adds 232 triangles to the existing hearth material and selects the lamp through the shared four-emitter hardware RT fire budget. The authoring survey found 0.19 m of clearance beyond the player capsule on the approach to the salvager and 0.43 m on the shore route.

Matched 960 x 540 Windows captures on RayTracingPipeline and RayQueryCompute show the salvager's face in warm light. Median luma in the same face crop rose from 4 to 34 on the 8-bit scale on each backend. The corresponding staffed tavern captures are pixel-identical. The full Windows RayTracingPipeline route passed all 36 checks, including the presented lamp from the normal approach and the final reload with all three preparations intact. The combined ARM64 Debug package contains the exact town mesh and manifest and passed 12 Android checks. Phone readability and sustained performance remain open.

The native walkthrough enters and leaves the shallows, verifies wet-footstep positions against the river height, and continues through the existing mill and shrine interactions. The `bellwether-river-landing` Debug checkpoint walks through the same geometry and then returns to its dry camera position. Both Windows hardware RT backends presented the view at 960 × 540. The Android ARM64 Debug package contains the same town export, with phone performance and listening checks pending.

#### Breathing rig seal trial

The private Debug checkpoint bellwether-rig-leak-trial starts on the dry causeway with the rig ready to test. Enter the shallows at the rail and choose **Start leak test**. Stay on the wet platform for two active seconds. Leaving early resets the test and the player can try again. On a successful hold, the prompt directs the player back to dry stone. **Confirm seal** updates the existing tavern return record. Pausing freezes the test and reloading an unconfirmed result requires a fresh attempt.

This isolated river checkpoint supplies the rig for testing. The separate private mill campaign earns the rig from the artificer after the workbench test. The seal trial and the independently playable salvager shore practice use the existing shallow contact at the river rail. Underwater traversal and the Abbey crossing remain open.

The native walkthrough checks early retreat, pause, retry, the full wet hold and the dry confirmation. It checks that an unfinished trial restarts after loading and the confirmed seal survives loading. Both Windows hardware RT backends presented the Debug starting position at 960 x 540. The ARM64 Debug package passed the Android checkpoint and prompt tests. Phone performance remains open.

The combined Windows Debug route uses `--lookout-validate-hidden --bellwether-verify-preparations` with an exported lookout fixture and the four pinned resident directories. It earns shore practice before any ward or rig, repairs the mill and confirms the shallow seal before learning the air ward. It then follows the church and shrine route, buys the lantern fitting and reloads the same tavern save with the three preparations intact. The Directions check reads and closes the actual pause panel along the way. The Windows panel measures wrapped text with the active font so the shore, rig and ward entries fit at the supported narrow size.

The route passed all 35 checks on RayTracingPipeline and RayQueryCompute. The final text-sizing build repeated the full pipeline check and passed the native narrow-window panel test. The combined ARM64 Debug package contains the new Directions copy and passed 12 Android checkpoint, asset-staging and control-layout checks. Device playtesting remains open.

With the fifth resident admitted, the same preparation check also walks the graveyard, buys the lantern fitting with the silver proceeds, and delivers the rag. After the eastern preparations it reloads before visiting the church and earning the air ward. The register pays three coins separately, and the already fitted hinge cannot be bought again. Both Windows hardware RT backends passed all 43 checks and retained the same final save payload once the save-request time was removed. The private ARM64 Debug package contains the shared Directions update and passed 596 host-side tests, with six skipped. The Galaxy S26 Ultra passed the staged opening Directions panel and both moving tavern-doorway returns on the exact combined Debug APK. The sampled display interval median was 58.33 ms. The full earned preparation route and sustained performance still need phone validation. See the [device record](../../ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md#23-november-2026--bellwether-directions-and-moving-tavern-doorway).

#### Artificer source candidate

The private artificer candidate wears a fitted iron-blue doublet and short leather apron. The original concept, Meshy source and 24-joint rig, supplied walk and run, editable Blender assembly and runtime files are archived with verified hashes. The prepared actor is 1.68 m tall with 15,360 triangles. Blender grounds the idle and retimes the 1.3-second walk to 1.08 m/s. The native character loader and CPU skinning checked the idle and 16 samples of that walk. Khronos validation reported no errors and one warning for the skinned mesh's parent node. Both platform sets of three one-layer 1K KTX2 textures passed validation.

#### Artificer at the mill workbench

The Windows Debug checkpoint `bellwether-mill-artificer` restores the completed tavern return point and follows the authored route to the working side of the mill bench. The shared simulation grounds the artificer at the south end of the bench and checks her clearance against the same terrain used for player movement. The checkpoint keeps the existing limit of three actors with Kit, the tavern keeper and the artificer. The ordinary church route retains the record keeper.

For a local capture, use the checkpoint with `--capture-hidden` and `--chapter-resident-fixture` pointing to a directory containing `tavern-keeper` and `artificer` runtime packages. The capture records the artificer's foot position and requires all three submitted instance masks. Both Windows hardware RT backends presented the scene at 960 × 540. The native walkthrough verified grounded feet and a clear approach and rejected the checkpoint with the church roster. The separate return-to-Kit replay now uses the full first register reading, including the air ward offer, and presented the return with the original roster. The mill still uses the existing dim night lighting. The ARM64 Debug package and Android tests cover the existing church roster.

#### Workbench repair and rig hand-off

With the artificer present, **Seat the bearing** installs the replacement drive in the existing cradle. **Turn the drive** runs the bronze gear for one active second. Looking away or leaving reach resets that attempt, and pausing freezes it. The part remains seated after interruption or reload. Once the drive passes, face the artificer and choose **Take breathing rig** to unlock the river seal trial. The fitted, tested, handed-off and sealed states update the same tavern return record. The [working wheel and millrace hardware](../../../assets/models/world/source/bellwether-millworks-v1/README.md) use the cut on the east side of the mill. Passing the existing drive test starts the wheel. The raised sluice and bearing piers have solid contact.

#### Footway to the wheel and dry crossing

Follow the lower river causeway and turn north before the seal-trial platform. The stone footway reaches a standing view beside the wheel and continues to a 7 m crossing upstream. The span slopes down to a fenced service turnout on the river side of the headrace. The crossing is available before the repair and requires no equipment or progression. The terrain and the finite water course retain their original shapes. [Mill crossing source and fit](../../../assets/models/world/source/bellwether-mill-crossing-v1/README.md)

The original Blender assembly uses one untextured bronze material and 384 triangles. The native renderer reuses the rigid-prop transform path. The drive adds one BLAS and one TLAS instance to the chapter and uses no extra texture layers. The `bellwether-mill-drive-tested` checkpoint walks to the bin and the workbench and reaches the tested stage through the normal action handlers.

On Windows Debug, pass the two-role mill folder to the existing `--bellwether-campaign-dir` option. On Android, build Debug with the pinned `artificer` and `tavern-keeper` directories under the existing resident overlay. The mill profiles use separate save storage, and the existing church profiles continue to use the record keeper.

The native walkthrough installs and tests the drive, takes the rig through the ordinary interaction, walks to the river and confirms the seal after checking pause, retreat, retry and reload. Matched empty and tested Windows captures passed at 960 × 540 on both hardware RT backends. The regular Windows Debug mill profile reached the tavern through the native campaign menus and restored its earned save. The separate ARM64 Debug package passed the checkpoint and interaction tests, and the APK inspection verified the exact gear, resident and voice entries. Moving between those rosters during play, phone performance and exact distribution clearance remain open.

### Relic dealer west of the square

The [west route](../../../src/gameplay/traversal/BellwetherWestRoute.h) branches from the square to the original S6 dealer mark. It uses the existing terrain and village collision. After the tavern conversation, face the dealer within reach and choose **Ask about trade**. The authored `hub.ordinary_sale` caption introduces ordinary silver. The conversation leaves the purse and checkpoint record intact.

The private profile uses the existing two resident slots with the tavern keeper and dealer, alongside Kit. The dealer uses the November 7 rig and its standing pose with one layer of 1K material maps on each platform. The native character loader passed the idle and walk. The existing dealer voice bank is available for audition. The later expedition relic and its living claimant still need design approval. The private mill tally trial below checks recoverable ownership in the current village.

The Windows Debug `bellwether-relic-dealer` capture uses `--chapter-resident-fixture` with the pinned `tavern-keeper` and `relic-dealer` folders. Use `--bellwether-campaign-dir` with the same folders to start the private campaign and its separate save slot. Both hardware RT backends presented the 960 × 540 west approach with all three actor instance masks active and the trade caption selected. The player lantern lights the dealer at the approach. The ordinary Windows menu validation also reached the staffed hearth from the lookout, earned the tavern save and loaded it again.

Android Debug uses the corresponding `hordeBellwetherResidentDir` overlay and the isolated `.debug.bellwether.dealer` package. The `horde.debug.checkpoint` and `horde.debug.autostart` intent extras reach the standalone live player view. The ARM64 package contains the two pinned residents, and the Java tests exercised the checkpoint binding and trade prompt.

The combined campaign can admit the dealer as a fifth source. It prefetches him along the tavern's south ramp and publishes him before the west turn. He shares a resident slot with the record keeper and salvager. The tavern keeper remains active on the south and west approaches, and the eastern approach continues to load the artificer and salvager. Readiness is published after the renderer completes the swap.

Both Windows hardware RT backends passed the ordinary campaign replay from the lookout save through the staffed tavern, a reversal on the west approach, the trade caption, both eastern stations and the final return. Each run recorded ten prefetched swaps and presented the active pair in the same RT scene. The combined Android ARM64 Debug package contained all 25 pinned resident files and passed 15 focused host-side Android tests.

The [S6 stall](../../../assets/models/world/source/bellwether-relic-stall-v1/README.md) adds a roof, counter, stock and a weighing sign. Follow the southern edge of the display and turn in to reach the same talk mark. Its stock screen closes the direct views from the sampled square and east road points while retaining the clear conversation aisle. The counter wick uses the existing ray-traced fire system and shares the exterior slot with the distant river lamp. The maximum scene upload remains four emitters. Physical phone validation is pending.

The updated combined route passed on both Windows hardware RT backends. The replay captured the approach and trade view, verified the wick in the active RT upload, and retained the east activities on return. The private Android ARM64 Debug build contains the exact new town GLB and manifest.

### Ordinary silver sale

In the dealer-enabled private campaign, finish the tavern conversation, then follow the CHURCH arrow at the north-west fork. The [split-lid cache](../../../assets/models/world/source/bellwether-road-cache-v1/README.md) beside the post offers **Take ordinary silver**. At the dealer's west stall, **Sell ordinary silver (+2 coins)** shows the provisional price before Interact. **Ordinary silver sold** confirms the sale. The proceeds go into the existing purse and can pay for the lantern hinge before the register reward. The named relic remains a separate decision.

Collection and sale update the earned tavern checkpoint. The sale writes the sold state and its proceeds together. Save schema 18 adds one byte for the silver state. Previous saves retain their balances and the cache starts uncollected.

Both Windows hardware RT backends passed the replay from the lookout save through collection, the quoted sale, repeat Interact, the hinge purchase, and the subsequent eastern visits. The replay reloaded the saved purse after each change. The combined Android ARM64 Debug package includes the pinned cache geometry and passed 16 focused host-side Android tests. Physical phone performance remains pending.

### Mill tally stamp

A banded case at the dry millrace turnout holds the mill's tally stamp.
The west-door board records the familiar mill mark. After the first tavern
conversation, a player can collect the stamp from the case and return it
directly to the board, or sell it at the separate tray on the dealer's counter.
Facing the dealer still offers ordinary trade. The provisional named sale
pays two coins once, enough for the existing hinge fitting.

The dealer retains the marked piece. If it was sold, use the board to record
the mill's ownership and then recover the stamp at the same tray. Recovery
does not require money, including when its proceeds have been spent. Once
recovered, it can only be returned. The case, board and tray have no new
inventory, patrol, economy or loading system. The purse and ownership save
together at the earned boundary. Earlier saves begin with the stamp unfound.
Bellwether Directions shows the next steps of the chosen route. No Abbey or
Foundry access gate depends on it.

The Windows replay at `e1bd58a8` passed all 30 checks on each hardware RT
backend. It walked the dry crossing, presented the separate ordinary and
named trade marks, and reloaded the earned tavern checkpoint after each
change of ownership. After recording the mill claim, the same replay
recovered the stamp for free, returned it, and verified that existing
silver proceeds and the fitted hinge retained their saved state. The
accelerated replay waits up to 750 ms for the existing resident publication
at the dealer witness. The stand, offering angle and caption are checked
after the roster becomes ready.

The codec test round-trips both return routes, verifies older saves byte for
byte and spends all sale proceeds before testing free recovery. All six host
suites passed. The private combined ARM64 Debug package at `ee1aafa6`
contains the pinned town mesh, resident and voice inputs. Android unit
tests passed 601 with six skipped. Device presentation and timing remain
open. The case, tray and board add 320 solid triangles to the existing town
primitives. Blender re-export reproduced the GLB and generated layout
exactly. Audio/haptic manual revalidation required: NO. The interactions
use text and create no audio or haptic events.

### Covered well in the square

The [T01 well](../../../assets/models/world/source/bellwether-well-v1/README.md) occupies the mapped centre of the square. The stone base is solid in the shared collision mesh. Posts support a clay roof over the planked opening and hanging bucket. The tested route approaches from the south and rejoins the square. Drawing water remains outside this checkpoint.

The combined Windows Debug campaign captures the well from the walked route with both hardware RT backends. The private Android ARM64 Debug package contains the same pinned town mesh. Physical phone performance remains open.

### Western kitchen plots

The [T02 planting](../../../assets/models/world/source/bellwether-garden-v1/README.md) keeps two small beds centred on the mapped kitchen plots. The dry 1.32-metre aisle is accessible from the west stall approach. Raised timber edges use town collision. The foliage shares the existing alder material and is rendered as opaque leaves.

The combined Windows Debug campaign walks through the aisle, raises the lantern for a view of both beds and returns past the stall. Both hardware RT backends presented the garden. The private Android ARM64 Debug build contains the exact town mesh and crop asset. Physical phone performance remains open.

### Settled livestock south of the kitchen plots

A dry side path from the gardens reaches the fenced stock yard. An adult
pig rests on the far side of the closed gate. One hen settles on a
sheltered straw shelf and one rests on the dry ground nearby. Posts, trough,
covered water vessel and the raised roost establish how the animals are
tended. The separate grate and privy retain their own places and purposes.

The fence and bodies stay in the town's existing hardware RT geometry.
The pig occasionally sniffs, the hen on the shelf turns to look, and the
ground hen briefly dips its head. The shared 60 Hz simulation drives the
gestures when the player is nearby and eases the heads into settled poses
on departure. Three visible RT instances share two immutable BLAS geometries
with no resident skinning or new textures. The pig's grunt and the hens'
clucks use the existing incidental ambience voice and begin on the animals'
fixed-step gestures. Starts use each animal's surveyed mount, with a nine-metre
range and a minimum three-second gap across the yard. Dialogue and departure
cancel the active sound. Combined handset cost remains open for the stock pocket.

The [editable source and receipt](../../../assets/models/world/source/bellwether-stock-v1/README.md)
add 4,492 solid triangles to the existing town materials and primitives.
The two head masters contribute 1,044 triangles, with the 340-triangle hen
head instanced twice, for the original 5,876 rendered triangles total.
The settled checkpoint's Windows replay at `43283f44` passed all 31 checks on both
hardware RT backends. It walked from the garden to the closed gate and
roost, presented the pig and hens under the carried lantern, and rejoined
the existing town route without changing the save. All three host suites
passed, including the gate's standing-body collision check and the scene
resource inventory. The combined private ARM64 Debug build at `4ac33718`
contains the exact town mesh and preserves the resident and voice inputs.
Android unit tests passed 601 with six skipped. Phone presentation and
timing remain open. Audio/haptic manual revalidation required: NO for the
settled geometry and viewing route.

The motion walkthrough at `22631e3a` passed all 31 checks on each Windows
hardware RT backend. The dry route presents a resting and a moving pose for
each animal, with the image tied to a successful RT presentation at that
simulation tick. The host asset policy, terrain, scene resource inventory,
shader ABI and render slot checks passed. A second Blender export reproduced
both runtime GLBs, manifests and the mount header byte for byte.

The signed combined ARM64 Debug APK at `27603992` has SHA-256
`7ad33ef39c783e025a3c9783dd492258d0a7631d5433037fe6b9c9f54e9161df`.
It contains the exact town and head assets and the same 25 resident files and
13 voice files as the settled checkpoint. Android unit tests passed 601 with
six skipped. The phone was disconnected during that validation. Handset
presentation, sustained timing and listening acceptance remain open.

### West service yard

The [closed privy](../../../assets/models/world/source/bellwether-privy-v1/README.md) sits west of the tavern with a latched door, washstand, covered jug and sheltered shelf. Its dry approach leaves the well, ramp and main routes clear. Drainage and waste handling still need review.

After the first tavern conversation, the occupant's approved request appears once per loaded session as a caption on a quiet approach. The player can take a [clean rag](../../../assets/models/world/source/bellwether-privy-rag-v1/README.md) from the basket and pass it by facing the shelf. Delivery updates the earned tavern autosave and the cloth stays on the shelf on return. Pickup is temporary, so reloading before delivery restores the rag to the basket. The favour costs nothing and carries no progression reward. The occupant's provisional thanks is a caption. Voice recording remains open.

The private Windows campaign passed all 19 checks on each hardware RT backend. The replay showed the local request, took the rag, reloaded before delivery, then delivered it once and reloaded from the updated autosave. Captures show the cloth in the basket and on the shelf. The private ARM64 Debug package contains the same pinned town and cloth assets. Physical phone play and performance remain open.

### Active graveyard east of the church

The [T03 churchyard](../../../assets/models/world/source/bellwether-graveyard-v1/README.md) connects to the existing church approach. Its path follows the terrain past the south frontage and through an opening in the dry-stone boundary. Nine plain stone markers establish the grave rows. The existing record keeper's service mark and the north wayside route stay available.

The combined native forest and town admit 152,228 collision triangles and retain 10,510,616 bytes of CPU support data. The intake ceiling is 163,840 triangles. This keeps the authored ground and collision in the same owner and permits the full woodland to load with the town.

Each Windows hardware RT backend passed all 20 checks in the private campaign replay. The player walked from the church to the grave rows and back to the fork without creating a save, then continued through the silver, rag and eastern routes. The private ARM64 Debug package contains the same pinned town GLB. Physical phone play and performance remain open.

### Southeast smith yard

The [B06 exterior work yard](../../../assets/models/world/source/bellwether-smith-v1/README.md) sits against the existing southeast smith wall. The open canopy covers a raised stone forge, anvil, heavy workbench and quench tub. The hardstand follows the surveyed hillside, with its edge seated in the existing ground. The building footprint, original west door and route past the yard are retained. A post-mounted oil wick lights the work area through the shared ray-traced fire system. The smith, west stall and river fork share the same exterior light slot. The artificer's breathing-rig work remains at the mill.

The town uses the same opaque material and primitive counts. The yard shares the world mesh and physical support. The Blender recipe reproduces the runtime GLB and layout exactly. The shell checks cover the grounded walk from the square, the mapped road past the yard in both directions and the blocked sweep through the anvil.

The Windows five-resident campaign replay walked the route and presented the wick, forge and workbench from two player views on both hardware RT backends. The private combined ARM64 Debug package pins the new town mesh. Phone play, sustained performance and listening to the stone footstep transition remain open.

### Garden-edge grate

The dry recess is north of the western kitchen plots, on a short branch
from the existing garden aisle. The stone lip and locked grate use the
same geometry for visibility and collision. A small roof shelters a dry
bed board, a covered water vessel and a shelf within reach of the man below. A wall-set oil wick uses the existing exterior light slot to illuminate the recess.

After the first tavern conversation, face the grate from the dry path and
choose **Talk through the grate**. The man recounts the two conflicting
stories and gives a practical reply from his own position. Neither account
is settled. Walking away ends the caption. The conversation confers no
reward, consumes no item and writes no checkpoint. This private version
uses the original caption playback path and an unseen speaker. The separate
rumour performances, recording and any later resolution remain open.

The private Windows replay at `e91fad00` walked from the earned tavern save
to the garden, checked the facing gate, used Interact and captured the
lit grate on an RT frame with the authored wick in the existing four-light
upload. Both hardware RT backends passed all 26 checks, including leaving
the grate without a new save and returning from the eastern route to the
preserved tavern visit. The accelerated walkthrough gives the asynchronous
resident prefetch up to 750 ms at the return marks before verifying the
published identities. This measures the presented route and its state,
without measuring ordinary movement pacing.

The solid grate and wick support add 720 triangles to the existing town
primitives. Blender re-export reproduced the GLB and layout exactly.
All four targeted host suites passed. The private combined ARM64 Debug
APK at `e91fad00` contains the pinned town mesh and unchanged private
resident and voice entries. Android unit tests passed 600 with six skipped.
Hardware phone presentation and timing remain open. Audio/haptic manual
revalidation required: NO for this caption-only staging. There is no new
audio cue or haptic event.

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

The Windows Debug option `--bellwether-voice-dir <local WAV directory>` and the [private Android voice overlay](../../../android/README.md#bellwether-three-actor-playtest) accept the five approved Kit WAVs. With the complete ten-file overlay, both can also audition the three roadside lines, the doorway welcome and the keeper's hearth reply. The spatial voice path uses each line's existing actor source and the Voice preference. Bellwether keeps the required caption whenever playback is unavailable or muted. The shared clock uses the measured lengths in both voice and caption-only play, including the roadside assessment.

The five audition WAV hashes match their production manifests. Each uses 24 kHz mono PCM16, and its manifest text matches the runtime caption. The doorway take lasts 3.699333 s and the reply lasts 5.579333 s. The local review reel includes the three roadside lines, the welcome and the reply in scene order. Exact-build listening, the foley and voice balance, and public distribution clearance remain open.

The combined private audition can include all thirteen pinned WAVs alongside the five-resident town. The two optional groups keep their original captions, sample lengths and listening status. Android opens stored WAVs directly from the APK and keeps cache staging for assets that need it.

The private Horde audio archive retains the original MP3s, per-line review WAVs and exact text, voice and cost manifests. The Debug audition uses byte-for-byte copies, and its review receipt pins the local reel and the five source WAVs.

## Animation and gameplay acceptance gate

First author a short test on the real Kit rig: loose-ground step/catch, protective standing assessment, several injured steps, turn and sit. Keep editable sources. Review normal first-person, front and side views for weight, planted feet, contact and coherent transitions before expanding into the complete scene. A flattering render is insufficient.

Then validate the whole slice in-engine with representative fire/lantern/glass/actors and exterior overlap on Android and Windows. Check looking away, overtaking, stopping/backtracking, repeated visits, subtitle-only/missing audio, skip/pause/quit/reload and door/collision recovery. Persist injury/arrival/settled state without duplicating the incident or trapping the player. Preserve real RT, shared simulation and existing scene/streaming contracts. No build, animation implementation, merge or release is authorized by this document's completion.
