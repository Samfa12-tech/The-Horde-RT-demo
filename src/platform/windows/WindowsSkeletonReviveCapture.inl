// Explicit Debug-only immutable pose preview. No gameplay clock, health,
// encounter, targetability, semantic event or save is changed by these samples.
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
if (!gShowcaseCaptureHidden || IsWindowVisible(context.windowHandle) ||
    GetForegroundWindow() == context.windowHandle || GetFocus() == context.windowHandle ||
    checkpoints.size() != 1u || context.developmentCheckpoint.empty() || forestProof ||
    context.playerDodgeInspectionSnapshot || context.inspectionSkeletonBlends || context.simulation.Events().Size() != 0u ||
    context.simulation.Snapshot().playerMountProfile != items::PlayerMountProfile::AnatomicalBody)
    return fail("Skeleton revival preview needs one unshown production-body pose owner and an empty event queue.");
const char* blendOption = std::getenv("HORDE_SKELETON_REVIVE_BLEND_CAPTURE");
if (blendOption && std::string_view(blendOption) != "1")
    return fail("Skeleton revival blend preview accepts only explicit value 1.");
const bool blendCandidate = blendOption != nullptr;
const char* standPath = std::getenv("HORDE_SKELETON_STAND_CAPTURE_ASSET");
const bool standCandidate = standPath != nullptr;
if (standCandidate && blendCandidate) return fail("Authored stand and rejected blend are separate candidates.");
constexpr auto standClip = static_cast<horde::scene::SkinnedClip>(4u); // explicit caller-selected diagnostic slot

SynchronizeSimulationViewportAspect(context);
context.simulationPaused = true;
context.simulationInput.paused = true;
context.frameDeltaSeconds = 0;
std::array<unsigned char, sizeof(SimulationSnapshot)> applicationBytes{};
std::memcpy(applicationBytes.data(), &context.simulation.Snapshot(), applicationBytes.size());
const auto eventSequence = context.simulation.Events().NextSequence();
struct ResetReviveInspection {
    VulkanSurfaceContext& context;
    ~ResetReviveInspection() {
        context.inspectionSkeletonBlends.reset();
        context.playerDodgeInspectionSnapshot.reset();
    }
} resetReviveInspection{context};

// A separate ordinary fixture constructs coherent camera/arm/torch transforms.
// Only its immutable render copy receives the seeded corpse samples below.
auto config = ProductionGameSimulationConfig();
config.playerStartX = -3.0f; config.playerStartZ = -12.8f;
config.playerStartYawRadians = -1.0f; config.playerStartPitchRadians = -.22f;
GameSimulation poseOwner(config);
poseOwner.SetPresentationAspect(horde::graphics::RtViewAspectFromImageExtent(
    context.swapchainExtent.width, context.swapchainExtent.height,
    horde::graphics::RtPresentationTransform::Identity));
const auto base = poseOwner.Snapshot();
if (base.skeletonEnemyCount != 2u || !base.torchFailure.heldByPlayer || base.torchFailure.triggered ||
    std::abs(base.playerX - config.playerStartX) > .001f || std::abs(base.playerZ - config.playerStartZ) > .001f)
    return fail("Skeleton preview requires the existing two waterfall actors and original held torch.");

const std::string assetPath = standCandidate ? standPath :
    "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
horde::scene::SkinnedMeshAsset source;
std::string diagnostic, assetHash;
std::vector<horde::scene::SkinnedClipBinding> bindings;
if (standCandidate) {
    const auto& ordinary = horde::scene::SkeletonCombatClipSet().clips;
    bindings.assign(ordinary.begin(), ordinary.end());
    bindings.push_back({"SkeletonProof|StandDead0ToIdle", false, true});
}
const bool loaded = standCandidate ? source.LoadClips(assetPath, bindings, diagnostic)
                                   : source.LoadCombatClips(assetPath, diagnostic);
if (!loaded || !Sha256File(assetPath, assetHash, diagnostic))
    return fail("Skeleton preview source identity/loader failed: " + diagnostic);
const float duration = source.ClipDuration(horde::scene::SkeletonClip::Dead);
if (!(duration > 0 && std::isfinite(duration))) return fail("Skeleton Dead duration is unavailable.");
if (standCandidate && (assetHash != "800632b9b1dbe53bd4f6508a6d167e52e940c6bdb4e902af752429e96d7fe259" ||
                      source.ClipDuration(standClip) != 1.5f))
    return fail("Authored stand capture does not own the verified private export identity/duration.");
const unsigned frameCount = standCandidate ? 251u : blendCandidate ? 165u : 150u;
const unsigned idleStart = standCandidate ? 165u : blendCandidate ? 135u : 120u;
unsigned completedFrames = 0;
std::uint64_t previousSerial = 0;
std::string previewError;
std::ofstream rows(outputDirectory / "skeleton-revive.jsonstream", std::ios::binary);
if (!rows) return fail("Cannot open skeleton revival preview receipt.");
rows.precision(10);
const auto began = std::chrono::steady_clock::now();
const auto writeSummary = [&]() {
    std::ostringstream summary;
    summary << "{\"complete\":" << (previewError.empty() && completedFrames == frameCount ? "true" : "false")
        << ",\"evidenceClass\":\"Debug immutable health-zero render-pose preview; not encounter revival, targetability, events, saves, natural play or performance acceptance\""
        << ",\"sourceSha256\":\"" << assetHash << "\",\"deadDurationSeconds\":" << duration
        << ",\"frames\":" << completedFrames << ",\"sampleHz\":30,\"unpaced\":true,\"directIdleJoinFrame\":"
        << (blendCandidate || standCandidate ? "null" : "120") << ",\"blendCandidate\":" << (blendCandidate ? "true" : "false")
        << ",\"blendDurationSeconds\":" << (blendCandidate ? .5f : 0.f)
        << ",\"standCandidate\":" << (standCandidate ? "true" : "false")
        << ",\"standDurationSeconds\":" << (standCandidate ? 1.5f : 0.f)
        << ",\"resetRepeatStartFrame\":" << (standCandidate ? "195" : "null")
        << ",\"heldRequestFrames\":" << (standCandidate ? "[242,243,244,245,246]" : "[]")
        << ",\"interruptionScope\":\"immutable request hold, clear, repeat and reentry only; not live gameplay pause/retry\""
        << ",\"ordinaryIdleStartFrame\":" << idleStart
        << ",\"applicationSnapshotUnchanged\":"
        << (std::memcmp(applicationBytes.data(), &context.simulation.Snapshot(), applicationBytes.size()) == 0 ? "true" : "false")
        << ",\"error\":\"" << JsonEscape(previewError) << "\"}\n";
    return WriteReportFile(outputDirectory / "skeleton-revive.json", summary.str());
};
const auto previewFail = [&](const std::string& why) {
    previewError = why; (void)writeSummary(); return fail(why);
};
for (unsigned frame = 0; frame < frameCount; ++frame)
{
    if (std::chrono::steady_clock::now() - began > std::chrono::seconds(90) ||
        IsWindowVisible(context.windowHandle) || GetForegroundWindow() == context.windowHandle ||
        GetFocus() == context.windowHandle)
        return previewFail("Skeleton preview lost its finite hidden owner.");
    // 0.5s corpse hold, all 90 authored samples backwards, 0.5s upright
    // Dead0 hold, then either the retained direct join or one explicit 0.5s
    // local-joint blend to Idle0, followed by the same 1s ordinary Idle.
    bool idle = frame >= idleStart;
    const bool blending = blendCandidate && frame >= 120u && frame < idleStart;
    bool standing = standCandidate && frame >= 120u && frame < idleStart;
    float standTime = standing ? float(frame - 119u) / 30.f : 0.f;
    const char* phase = idle ? "ordinary-idle" : frame < 15u ? "corpse-hold" :
        frame <= 104u ? "reverse-dead" : "dead0-hold";
    if (standing) phase = "authored-stand";
    if (blending) phase = "local-joint-blend";
    float sample = idle ? float(frame - idleStart) / 30.f :
        frame < 15u ? duration : frame <= 104u ? std::max(0.f, duration - float(frame - 15u) / 30.f) : 0.f;
    if (standCandidate && frame >= 195u) {
        // A marked request reset is not a smooth natural-join claim. Repeat and
        // clear/reentry witnesses exercise the existing pose cache ownership.
        idle = false; sample = 0.f; phase = "request-reset-dead0";
        standing = frame >= 196u && frame <= 240u;
        if (standing) { standTime = float(frame - 195u) / 30.f; phase = "repeated-stand"; }
        if ((frame >= 242u && frame <= 246u) || frame == 248u || frame == 249u) {
            standing = true; standTime = .75f;
            phase = frame == 248u ? "stand-reentry" : frame == 249u ? "split-actor-requests" : "held-stand-request";
        }
        if (frame == 241u || frame == 247u || frame == 250u) {
            idle = true; standing = false; phase = "request-clear-idle0";
        }
    }
    std::optional<horde::scene::SkinnedClipBlend> blend;
    if (blending) blend = horde::scene::SkinnedClipBlend{
        horde::scene::SkeletonClip::Idle, 0.f, float(frame - 119u) / 15.f};
    if (standing) blend = horde::scene::SkinnedClipBlend{standClip, standTime, 1.f};
    auto pose = base;
    pose.tickIndex = context.simulation.Snapshot().tickIndex + frame + 1u;
    pose.activeSkeletonCount = 0u;
    for (auto& actor : pose.skeletonEnemies)
    {
        actor.health = 0; actor.dead = true; actor.playerHitPulse = actor.parrySuccessPulse = false;
        actor.reaction = CombatReaction::None; actor.reactionTime = actor.damageFlash = actor.actionTime = 0;
        actor.animation = idle ? EnemyAnimation::Idle : EnemyAnimation::Dead;
        actor.action = idle ? EnemyCombatAction::Locomotion : EnemyCombatAction::Dead;
        actor.animationTime = idle ? sample / .90f : sample;
        actor.idlePhaseSeconds = 0;
    }
    context.playerDodgeInspectionSnapshot = pose;
    context.inspectionSkeletonBlends.reset();
    if (blend) {
        horde::vulkan::raytracing::SkeletonPoseBlendRequests requests{};
        for (auto& request : requests) request = blend;
        if (standCandidate && frame == 249u && requests.size() > 1u) requests[1]->timeSeconds = .5f;
        context.inspectionSkeletonBlends = requests;
    }
    bool presented = false;
    if (!RenderFrame(context, clearColor, presented) || !presented ||
        !CompleteRtEvidenceAfterDeviceIdle(context, vkDeviceWaitIdle(context.device)))
        return previewFail("Skeleton preview RT present/completion failed: " + context.lastRtFrameError);
    const auto publication = context.rtFrameEvidence.PublishedStateByValue();
    const auto& complete = publication.completedEvidence;
    const auto& identity = complete.identity.submitted;
    if (!publication.presented || !publication.hasCompletedEvidence ||
        identity.submissionSerial <= previousSerial || identity.frame.simulationTick != pose.tickIndex ||
        identity.frame.sceneEpoch != publication.sceneEpoch ||
        identity.frame.measurementGeneration != publication.measurementGeneration ||
        !complete.scene.dispatch.sceneReady || !complete.scene.dispatch.rtDispatchRecorded ||
        !complete.scene.dispatch.swapchainCopyRecorded ||
        complete.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
        identity.submissionSerial != complete.presentation.lastSuccessfulPresentSubmissionSerial)
        return previewFail("Skeleton preview lacks matching actual presented RT identity.");
    const auto masks = context.rtScene.LastInstanceMasks();
    const bool dedicated = horde::vulkan::raytracing::HasDedicatedPlayerPrimaryOwnership(
        masks, context.rtScene.LastPlayerWorldBodyInstanceFlags());
    const auto arms = context.rtScene.PrimaryPlayerPixelCount();
    const bool counters = context.rtScene.DiagnosticsAvailability() ==
        horde::vulkan::raytracing::RtDiagnosticAvailability::Available;
    if (!dedicated || !counters || !arms ||
        !horde::platform::windows::HasExpectedPlayerCaptureVisibility(dedicated,
            context.rtScene.LastPlayerPrimaryVisible(), counters, arms, false, nullptr) ||
        std::memcmp(applicationBytes.data(), &context.simulation.Snapshot(), applicationBytes.size()) != 0 ||
        context.simulation.Events().Size() != 0u || context.simulation.Events().NextSequence() != eventSequence)
        return previewFail("Skeleton preview lost arm ownership or changed application snapshot/events.");
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;
    const std::string file = "revive-" + std::to_string(frame) + ".png";
    std::string hash, evidence;
    horde::telemetry::RtEvidenceValidationError validation{};
    if (!context.rtScene.CaptureStorageImage(pixels, diagnostic) ||
        !WriteRgbaPng(outputDirectory / file, pixels, diagnostic) ||
        !Sha256File(outputDirectory / file, hash, diagnostic) ||
        !horde::telemetry::SerializeRtPerformanceEvidenceJson(complete, evidence, validation))
        return previewFail("Skeleton preview readback/evidence failed: " + diagnostic);
    rows << "{\"frame\":" << frame << ",\"seconds\":" << float(frame) / 30.f
        << ",\"phase\":\"" << phase << "\",\"clip\":\""
        << (standing ? "SkeletonProof|StandDead0ToIdle" : idle ? "Idle_5" : "Dead")
        << "\",\"sampleSeconds\":" << (standing ? standTime : sample)
        << ",\"blendWeight\":" << (blend ? blend->weight : 0.f)
        << ",\"file\":\"" << file << "\",\"sha256\":\"" << hash << "\",\"armPixels\":" << arms
        << ",\"actors\":[";
    for (std::size_t i = 0; i < pose.skeletonEnemyCount; ++i) {
        const auto& a = pose.skeletonEnemies[i]; if (i) rows << ',';
        rows << "{\"id\":" << unsigned(a.id) << ",\"health\":0,\"dead\":true,\"root\":["
            << a.x << ',' << a.worldY << ',' << a.z << "],\"yaw\":" << a.facingRadians
            << ",\"standTime\":" << (standCandidate && frame == 249u && i == 1u ? .5f : standTime) << '}';
    }
    rows << ']';
    if (frame == 0 || frame == 45 || frame == 75 || frame == 104 || frame == 119 || frame == 120 ||
        (blending && (frame == 124 || frame == 127 || frame == 130 || frame == 134)) ||
        (standCandidate && (frame == 130u || frame == 142u || frame == 154u || frame == 164u ||
            frame == 194u || frame == 195u || frame == 210u || frame >= 240u)) ||
        frame == idleStart || frame == frameCount - 1u) {
        // Native loader witnesses, not contact thresholds or a second gameplay
        // pose. World Y is this local skin value plus each retained root Y.
        const auto clip = standing ? standClip : idle ? horde::scene::SkeletonClip::Idle : horde::scene::SkeletonClip::Dead;
        const float witnessTime = standing ? standTime : sample;
        std::vector<horde::scene::SkinnedRtVertex> vertices;
        constexpr std::array<std::string_view, 5> names{{"Hips", "LeftFoot", "RightFoot", "LeftToeBase", "RightToeBase"}};
        std::array<horde::scene::SkinnedNodeTransform, names.size()> nodes;
        if (!source.Skin(clip, witnessTime, vertices, diagnostic, {}, nullptr,
                         blending && blend ? &*blend : nullptr) || vertices.empty() ||
            (!blending && !source.NodeTransforms(clip, witnessTime, names, nodes, diagnostic)))
            return previewFail("Native skeleton endpoint witness failed: " + diagnostic);
        float minimumY = vertices.front().position[1];
        for (const auto& vertex : vertices) {
            for (unsigned axis = 0; axis < 3; ++axis)
                if (!std::isfinite(vertex.position[axis])) return previewFail("Skeleton native skin is nonfinite.");
            minimumY = std::min(minimumY, vertex.position[1]);
        }
        rows << ",\"nativeSkinMinimumLocalY\":" << minimumY;
        if (!blending) {
            rows << ",\"nativeNodes\":{";
            for (unsigned i = 0; i < names.size(); ++i) {
                if (i) rows << ',';
                rows << '\"' << names[i] << "\":[" << nodes[i][12] << ',' << nodes[i][13] << ',' << nodes[i][14] << ']';
            }
            rows << '}';
        }
    }
    rows << ",\"completedFrame\":" << evidence << "}\n";
    rows.flush(); if (!rows) return previewFail("Skeleton preview receipt write failed.");
    ++completedFrames; previousSerial = identity.submissionSerial;
}
return writeSummary() ? 0 : 1;
