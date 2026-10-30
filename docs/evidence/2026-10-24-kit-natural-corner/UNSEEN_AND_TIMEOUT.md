# Existing evidence limits and Debug test scope

## Off-screen orientation snap

Owner permits orientation snapping only when Kit is genuinely not in view, preserving observable casual/urgent turns. This does not permit teleporting, faster translation, a larger tether, checkpoint grants or arbitrary pose/yaw mismatch.

Current source provides actor activation/resource ownership, not conservative unseen evidence. PresentableTinyRtScene::WantsChapterCompanion (around line 8125) selects Kit from ready assets, rescue state, dead Keeper and grounded support. It is not a visibility result. UpdateDynamicInstances around lines 7168–7174 assigns Kit's TLAS instance mask `0x01` regardless of camera frustum. Hardware ray queries/trace paths can still encounter him through indirect views and shadows (rt_lighting.glsl / rt_hit_decode.glsl). The shared snapshot has no completed-frame, generation-matched all-ray unseen fact. Production pose cache/publication proves which pose was submitted, not that it contributed no pixels.

A main-camera AABB/frustum result would therefore be insufficient. Diagnostic camera or aggregate counters cannot establish a Shipping guarantee. Retiring the actor entirely could establish absence for that resource generation, but that is not the current active outdoor catch-up case. Reliable unseen determination is unavailable at this seam; no broad visibility system or unsafe snap is implemented. A future stable rule would need renderer-owned relevant-ray evidence, camera/scene-generation invalidation, a supported exchange boundary and coherent joint/world-yaw publication.

The natural walking pair intentionally keeps looking at Kit. Even a safe unseen snap would not by itself fix this observable corner-delay case.

## Why the 30-second Debug gate grew beyond its budget

- 7d89445030727a9099524c3728c190dfd636e51b introduced CampaignPlaySessionTests WITH natural combat/claim/ascent and `TIMEOUT 30`. This file did not begin as a storage-only test.
- 15ce496017499e8ac9752e660579167c83d5a86e added earned disk records, fresh lower reload, interrupted summit descent and another completed-descent recovery.
- 32fdcab7864827c9689925d053322713ef1801a1 added full forest progression, endpoint persistence, endpoint reload and the whole loaded reverse trail/descent. The deadline remained 30.
- The separate earlier storage-oriented CampaignSessionTests came from b7f8e169fa57486f9ac4d4341590454695362944.

Saved phase evidence completes 145 checks at 37.067 s with normal turn/start OFF. Terrain is prepared once (1.342 s) and shared across restores; the storage prelude takes 0.039 s and final fault block 0.032 s. Loaded return consumes 15.832 s/3,082 actual fixed ticks after 21.193 s. Five-second Until values are early-exit deadlines, not fixed sleeps. The neutral intervals independently test proof presentation, no repeated autosaves and settled recovery. Fresh sessions test different durable records/story states. No evidence-backed seven-second redundant setup/wait deletion was found.

The issue is expanded aggregate Debug integration work against the original total budget, not a demonstrated nonterminating route. Hot-function CPU attribution is still unmeasured. The30-second CTest deadline remains unchanged and failing; completion outside CTest is not a gate pass. No test partition, wait deletion or deadline increase was made here.
