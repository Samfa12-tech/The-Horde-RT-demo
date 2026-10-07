#include "telemetry/MotionEvidenceLedger.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <locale>
#include <ostream>

namespace horde::telemetry
{
namespace
{
bool SameIdentity(const RtSubmittedFrameIdentity& left, const RtSubmittedFrameIdentity& right)
{
    return left.submissionSerial == right.submissionSerial &&
        left.frame.sceneEpoch == right.frame.sceneEpoch &&
        left.frame.measurementGeneration == right.frame.measurementGeneration &&
        left.frame.recordAttemptSerial == right.frame.recordAttemptSerial &&
        left.frame.recordSerial == right.frame.recordSerial &&
        left.frame.simulationTick == right.frame.simulationTick && left.frame.frameSlot == right.frame.frameSlot;
}
void Text(std::ostream& output, std::string_view text)
{
    output << '"';
    for (unsigned char c : text)
    {
        switch (c)
        {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (c < 0x20u) output << "\\u00" << "0123456789abcdef"[c >> 4u] << "0123456789abcdef"[c & 15u];
            else output << static_cast<char>(c);
        }
    }
    output << '"';
}
void Commands(std::ostream& output, const horde::gameplay::simulation::SimulationCommandSequences& commands)
{
    output << '[' << commands.attack << ',' << commands.parry << ',' << commands.dodge << ','
           << commands.routeReset << ',' << commands.retry << ',' << commands.interact << ','
           << commands.toggleHeldLightPose << ']';
}
}
bool MotionEvidenceLedger::Reject(std::string_view reason)
{
    if (failure_.empty()) failure_ = reason;
    return false;
}
bool MotionEvidenceLedger::Begin(std::string_view id, horde::gameplay::validation::MotionScenario scenario)
{
    *this = {};
    if (id.empty() || id.size() > 64u || std::any_of(id.begin(), id.end(), [](unsigned char c)
        { return !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-'); }) ||
        std::string_view(horde::gameplay::validation::MotionScenarioName(scenario)) == "invalid")
        return Reject("Unsafe motion run ID or unknown scenario.");
    runId_ = id;
    scenario_ = scenario;
    states_.reserve(kMaximumStateRows); events_.reserve(kMaximumEventRows);
    frames_.reserve(kMaximumRtRows); pipelines_.reserve(kMaximumPipelines);
    scopes_.reserve(kMaximumScopes); retirements_.reserve(kMaximumRetirements);
    return true;
}
bool MotionEvidenceLedger::AppendState(std::uint64_t now,
    const horde::gameplay::simulation::SimulationSnapshot& state,
    const horde::gameplay::simulation::InputSnapshot& input,
    const horde::gameplay::validation::MotionEvidenceScenario& scenario,
    std::span<const horde::gameplay::simulation::GameplayEvent> events)
{
    if (Failed()) return false;
    if (runId_.empty() || scenario.Scenario() != scenario_ || states_.size() >= kMaximumStateRows ||
        events.size() > kMaximumEventRows - events_.size() || input.hasAuthoritativePlayerPose ||
        state.playerMountProfile != horde::gameplay::items::PlayerMountProfile::AnatomicalBody ||
        state.eventQueueOverflowCount != 0u || !std::isfinite(state.playerX) || !std::isfinite(state.playerZ) ||
        !std::isfinite(state.playerYawRadians) || !std::isfinite(state.playerPitchRadians) ||
        !std::isfinite(input.moveForward) || !std::isfinite(input.moveStrafe) ||
        !std::isfinite(input.yawRadians) || !std::isfinite(input.pitchRadians) ||
        std::abs(input.moveForward) > 1.00001f || std::abs(input.moveStrafe) > 1.00001f ||
        (!states_.empty() && (now < states_.back().wallNanoseconds || state.tickIndex < states_.back().tick)))
        return Reject("Motion state input/profile/time/capacity admission failed.");
    if (state.fireEmitterCount > 4u || state.fireEmitterCount > state.fireEmitters.size())
        return Reject("Motion configured fire-source count exceeds its admitted capacity.");
    for (std::size_t index = 0u; index < state.fireEmitterCount; ++index)
    {
        const auto& source = state.fireEmitters[index];
        if (source.stableId == 0u || !std::isfinite(source.strength) || source.strength < 0.0f ||
            !std::isfinite(source.fuel) || source.fuel < 0.0f ||
            !std::isfinite(source.phase) || !std::isfinite(source.leanX) ||
            !std::isfinite(source.leanZ) || !std::isfinite(source.motionTurbulence))
            return Reject("Motion configured fire-source identity/numeric admission failed.");
        for (std::size_t earlier = 0u; earlier < index; ++earlier)
            if (state.fireEmitters[earlier].stableId == source.stableId)
                return Reject("Motion configured fire-source identity is duplicated.");
    }
    for (const auto& event : events)
    {
        if (event.sequence <= latestEventSequence_ ||
            static_cast<std::size_t>(event.type) >= horde::gameplay::simulation::kGameplayEventTypeCount)
            return Reject("Motion semantic event order/type admission failed.");
        latestEventSequence_ = event.sequence;
    }
    const auto& sword = state.heldItems[1];
    const auto& torch = state.torchFailure;
    for (const float value : {torch.flameStrength, torch.leftArmLowerBlend, torch.fallProgress})
        if (!std::isfinite(value) || value < 0.0f || value > 1.0f)
            return Reject("Motion torch progress/strength is invalid.");
    for (const float value : {torch.sequenceTime, torch.phaseTime, torch.droppedX,
             torch.droppedY, torch.droppedZ, torch.droppedYawRadians,
             torch.droppedViewPitchRadians, torch.droppedPitchRadians,
             state.heldItems[0].worldFromItem[12], state.heldItems[0].worldFromItem[13],
             state.heldItems[0].worldFromItem[14]})
        if (!std::isfinite(value)) return Reject("Motion torch time/pose is invalid.");
    if (!std::isfinite(sword.transition.progress) || sword.transition.progress < 0.0f ||
        sword.transition.progress > 1.0f || !std::isfinite(sword.visualGripBlend) ||
        sword.visualGripBlend < 0.0f || sword.visualGripBlend > 1.0f ||
        !std::isfinite(sword.visualStowBlend) || sword.visualStowBlend < 0.0f ||
        sword.visualStowBlend > 1.0f)
        return Reject("Motion sword transition state is invalid.");
    MotionStateRow row;
    row.wallNanoseconds = now; row.tick = state.tickIndex; row.publicationSequence = state.inputPublicationSequence;
    row.simulationSeconds = scenario.SimulationSeconds(); row.stage = scenario.Stage(); row.input = input;
    row.player = {{state.playerX, state.playerZ, state.playerYawRadians, state.playerPitchRadians, state.walkTime, state.walkAmount}};
    row.keeper = state.lich; row.torch = state.torchFailure; row.combat = state.playerCombat;
    row.torchParent = state.heldItems[0].parentMode;
    row.torchItemPosition = {{state.heldItems[0].worldFromItem[12],
                             state.heldItems[0].worldFromItem[13],
                             state.heldItems[0].worldFromItem[14]}};
    row.sword = {sword.parentMode, sword.transition.kind, sword.transition.progress,
                 sword.visualGripBlend, sword.visualStowBlend, sword.transition.active};
    row.chest = state.chestReward; row.heldLight = state.interaction; row.finale = state.finale;
    row.consumed = {state.lastConsumedAttackSequence, state.lastConsumedParrySequence, state.lastConsumedDodgeSequence,
        state.lastConsumedRouteResetSequence, state.lastConsumedRetrySequence, state.lastConsumedInteractSequence,
        state.lastConsumedToggleHeldLightPoseSequence};
    row.overrunCount = state.catchUpOverrunCount; row.ticksThisFrame = state.simulationTicksThisFrame;
    row.retryGeneration = state.retryGeneration; row.vitality = state.playerVitals.vitality;
    row.paused = state.paused; row.playerAlive = state.playerAlive;
    row.simulationFireEmitterCount = static_cast<std::uint32_t>(state.fireEmitterCount);
    for (std::size_t i = 0u; i < state.fireEmitterCount; ++i)
    {
        row.fire[i] = {{state.fireEmitters[i].phase, state.fireEmitters[i].leanX,
                       state.fireEmitters[i].leanZ, state.fireEmitters[i].motionTurbulence}};
        row.simulationFireSources[i] = {state.fireEmitters[i].stableId,
                                       state.fireEmitters[i].strength, state.fireEmitters[i].fuel};
    }
    const auto index = states_.size(); states_.push_back(row);
    for (const auto& event : events) events_.push_back({index, event});
    return true;
}
bool MotionEvidenceLedger::ObserveScope(std::uint64_t generation, std::uint64_t epoch, std::uint64_t measurement)
{
    if (Failed()) return false;
    if (generation == 0u || epoch == 0u || measurement == 0u || runId_.empty())
        return Reject("Motion scope requires the actual nonzero owning resource identity.");
    if (!scopes_.empty())
    {
        const auto& old = scopes_.back();
        if (generation == old.surfaceGeneration && epoch == old.sceneEpoch && measurement == old.measurementGeneration) return true;
        if (generation < old.surfaceGeneration || (generation == old.surfaceGeneration &&
            (epoch < old.sceneEpoch || measurement < old.measurementGeneration)))
            return Reject("Motion resource scope regressed.");
    }
    if (scopes_.size() >= kMaximumScopes) return Reject("Motion scope discontinuity capacity exceeded.");
    scopes_.push_back({generation, epoch, measurement, states_.size()});
    currentScopePresented_ = false;
    return true;
}
bool MotionEvidenceLedger::BindSubmittedFrame(std::uint64_t generation, const RtSubmittedFrameIdentity& identity)
{
    if (Failed()) return false;
    if (scopes_.empty() || generation != scopes_.back().surfaceGeneration ||
        identity.frame.sceneEpoch != scopes_.back().sceneEpoch ||
        identity.frame.measurementGeneration != scopes_.back().measurementGeneration ||
        states_.empty() || identity.submissionSerial == 0u ||
        identity.frame.sceneEpoch == 0u || identity.frame.measurementGeneration == 0u ||
        identity.frame.recordSerial == 0u || identity.frame.recordAttemptSerial == 0u ||
        identity.frame.simulationTick != states_.back().tick || identity.frame.frameSlot >= pending_.size())
        return Reject("Motion frame must bind the current snapshot to its committed graphics identity.");
    auto& pending = pending_[identity.frame.frameSlot];
    if (pending.valid) return Reject("Motion frame slot was reused before its owning completion was observed.");
    pending = {identity, generation, states_.size() - 1u, true};
    return true;
}
bool MotionEvidenceLedger::RetireSubmittedFrame(std::uint64_t generation, const RtSubmittedFrameIdentity& identity)
{
    if (Failed()) return false;
    if (scopes_.empty() || identity.frame.frameSlot >= pending_.size() ||
        retirements_.size() >= kMaximumRetirements)
        return Reject("Motion retirement scope/slot/capacity admission failed.");
    const auto& scope = scopes_.back();
    auto& pending = pending_[identity.frame.frameSlot];
    if (!pending.valid || pending.surfaceGeneration != generation || !SameIdentity(pending.identity, identity) ||
        (generation == scope.surfaceGeneration && identity.frame.sceneEpoch == scope.sceneEpoch &&
         identity.frame.measurementGeneration == scope.measurementGeneration))
        return Reject("Only an exactly owned obsolete pending submission can be retired without a presentation claim.");
    retirements_.push_back({identity, generation, pending.stateRow});
    pending.valid = false;
    return true;
}
bool MotionEvidenceLedger::AppendCompletedFrame(std::uint64_t generation, const RtLifecyclePublishedState& publication)
{
    if (Failed()) return false;
    if (!publication.hasCompletedEvidence) return true; // explicit pending, never a presentation pass
    const auto& completed = publication.completedEvidence;
    const auto& identity = completed.identity;
    RtEvidenceValidationError validationError{};
    if (!ValidateRtPerformanceEvidence(completed, validationError))
        return Reject("Motion completion failed the canonical RT evidence validation contract.");
    if (scopes_.empty() || !publication.running || publication.sceneEpoch != scopes_.back().sceneEpoch ||
        publication.measurementGeneration != scopes_.back().measurementGeneration ||
        !publication.presented || completed.presentation.outcome != RtPresentationOutcome::Presented ||
        completed.presentation.lastSuccessfulPresentSubmissionSerial < identity.submitted.submissionSerial ||
        identity.completionSerial == 0u || !completed.scene.dispatch.sceneReady ||
        !completed.scene.dispatch.rtDispatchRecorded || !completed.scene.dispatch.swapchainCopyRecorded ||
        identity.submitted.frame.frameSlot >= pending_.size() || frames_.size() >= kMaximumRtRows)
        return Reject("Motion frame is unavailable, stale, unpresented or not a completed current RT output.");
    if (!frames_.empty() && generation == frames_.back().surfaceGeneration &&
        SameIdentity(identity.submitted, frames_.back().identity.submitted) &&
        identity.completionSerial == frames_.back().identity.completionSerial) return true;
    auto& pending = pending_[identity.submitted.frame.frameSlot];
    if (!pending.valid || pending.surfaceGeneration != generation || !SameIdentity(pending.identity, identity.submitted))
        return Reject("Completed motion frame has no exact committed input/simulation binding.");
    const auto& pipeline = completed.scene.pipeline;
    auto found = std::find_if(pipelines_.begin(), pipelines_.end(), [&](const auto& candidate)
    {
        return candidate.executionMode == pipeline.executionMode &&
            RtFixedTextView(candidate.active.key) == RtFixedTextView(pipeline.active.key) &&
            RtFixedTextView(candidate.active.sha256) == RtFixedTextView(pipeline.active.sha256) &&
            RtFixedTextView(candidate.opaqueFast.sha256) == RtFixedTextView(pipeline.opaqueFast.sha256) &&
            RtFixedTextView(candidate.genericDielectric.sha256) == RtFixedTextView(pipeline.genericDielectric.sha256);
    });
    if (found == pipelines_.end())
    {
        if (pipelines_.size() >= kMaximumPipelines) return Reject("Motion pipeline identity capacity exceeded.");
        pipelines_.push_back(pipeline); found = std::prev(pipelines_.end());
    }
    MotionRtRow row;
    row.stateRow = pending.stateRow; row.pipelineIndex = static_cast<std::size_t>(found - pipelines_.begin());
    row.surfaceGeneration = generation; row.identity = identity;
    row.resources = completed.scene.resources; row.player = completed.scene.player;
    row.fireLighting = completed.scene.fireLighting;
    row.actualUploadedMistEnabled = completed.scene.actualUploadedMistEnabled;
    row.gpuStatus = completed.gpu.status; row.cpuStatus = completed.scene.stages.status;
    row.gpuDurationAvailable = completed.gpu.hasDuration && completed.gpu.status == RtSampleStatus::Valid &&
        completed.gpu.completedSubmissionSerial == identity.submitted.submissionSerial;
    row.gpuNanoseconds = row.gpuDurationAvailable ? completed.gpu.durationNanoseconds : 0u;
    row.wholeFrameCpuNanoseconds = completed.scene.stages.values[RtStageIndex(RtStage::WholeFrameCycle)].durationNanoseconds;
    row.simulationCpuNanoseconds = completed.scene.stages.values[RtStageIndex(RtStage::SimulationStep)].durationNanoseconds;
    frames_.push_back(row); pending.valid = false;
    const auto& scope = scopes_.back();
    if (generation == scope.surfaceGeneration && identity.submitted.frame.sceneEpoch == scope.sceneEpoch &&
        identity.submitted.frame.measurementGeneration == scope.measurementGeneration) currentScopePresented_ = true;
    return true;
}
bool MotionEvidenceLedger::HasCurrentPresentedFrame(std::uint64_t generation, std::uint64_t epoch,
                                                   std::uint64_t measurement) const noexcept
{
    if (Failed() || scopes_.empty() || !currentScopePresented_) return false;
    const auto& scope = scopes_.back();
    return scope.surfaceGeneration == generation && scope.sceneEpoch == epoch && scope.measurementGeneration == measurement;
}
std::size_t MotionEvidenceLedger::ReservedBytes() const noexcept
{
    return states_.capacity() * sizeof(MotionStateRow) + events_.capacity() * sizeof(MotionEventRow) +
        frames_.capacity() * sizeof(MotionRtRow) + pipelines_.capacity() * sizeof(RtPipelineEvidenceIdentity) +
        scopes_.capacity() * sizeof(MotionScopeRow) + retirements_.capacity() * sizeof(MotionRetirementRow) + sizeof(pending_);
}
bool MotionEvidenceLedger::HasPendingSubmissions() const noexcept
{
    return std::any_of(pending_.begin(), pending_.end(), [](const auto& pending) { return pending.valid; });
}
void MotionEvidenceLedger::WriteJson(std::ostream& output,
    const horde::gameplay::validation::MotionEvidenceScenario& scenario) const
{
    const auto previousLocale = output.getloc(); const auto previousFlags = output.flags(); const auto previousPrecision = output.precision();
    output.imbue(std::locale::classic()); output << std::setprecision(9);
    output << "{\"schema\":1,\"runId\":"; Text(output, runId_);
    output << ",\"scenario\":"; Text(output, horde::gameplay::validation::MotionScenarioName(scenario_));
    output << ",\"stage\":"; Text(output, horde::gameplay::validation::MotionStageName(scenario.Stage()));
    output << ",\"scenarioComplete\":" << (scenario.Complete() ? "true" : "false")
           << ",\"rtAcceptance\":\"requires-completed-frames-and-owner-motion-review\",\"failure\":";
    Text(output, Failed() ? Failure() : scenario.Failure());
    output << ",\"reservedLedgerBytes\":" << ReservedBytes()
           << ",\"inputPolicy\":\"ordinary-axes-and-monotonic-commands-after-one-checkpoint-seed\""
           << ",\"audioAcceptance\":\"not-established-by-this-ledger\",\"states\":[";
    for (std::size_t i = 0u; i < states_.size(); ++i)
    {
        const auto& r = states_[i]; if (i) output << ',';
        output << "{\"row\":" << i << ",\"wallNs\":" << r.wallNanoseconds << ",\"tick\":" << r.tick
               << ",\"simulationSeconds\":" << r.simulationSeconds << ",\"publication\":" << r.publicationSequence << ",\"stage\":";
        Text(output, horde::gameplay::validation::MotionStageName(r.stage));
        output << ",\"axes\":[" << r.input.moveForward << ',' << r.input.moveStrafe << "],\"commands\":"; Commands(output, r.input.commands);
        output << ",\"consumed\":"; Commands(output, r.consumed);
        output << ",\"player\":["; for (std::size_t j = 0u; j < r.player.size(); ++j) { if (j) output << ','; output << r.player[j]; }
        output << "],\"paused\":" << (r.paused ? "true" : "false") << ",\"damageEnabled\":" << (r.input.damageEnabled ? "true" : "false")
               << ",\"playerAlive\":" << (r.playerAlive ? "true" : "false") << ",\"vitality\":" << r.vitality
               << ",\"retry\":" << r.retryGeneration << ",\"ticksThisFrame\":" << r.ticksThisFrame << ",\"overruns\":" << r.overrunCount
               << ",\"keeper\":{\"phase\":" << static_cast<int>(r.keeper.phase) << ",\"revealPhase\":" << static_cast<int>(r.keeper.revealPhase)
               << ",\"revealSeconds\":" << r.keeper.revealElapsedSeconds << ",\"position\":[" << r.keeper.x << ',' << r.keeper.y << ',' << r.keeper.z
               << "],\"tilt\":" << r.keeper.presentationTiltRadians << ",\"facing\":" << r.keeper.facingRadians << ",\"title\":" << r.keeper.titleOpacity
               << ",\"hp\":" << r.keeper.health << ",\"phaseTime\":" << r.keeper.phaseTime << ",\"staff\":" << r.keeper.staffLightStrength
               << ",\"damagePulse\":" << (r.keeper.damagePulse ? "true" : "false") << "},\"torchPhase\":" << static_cast<int>(r.torch.phase)
               << ",\"torchHeld\":" << (r.torch.heldByPlayer ? "true" : "false")
               << ",\"torch\":{\"triggered\":" << (r.torch.triggered ? "true" : "false")
               << ",\"sequenceTime\":" << r.torch.sequenceTime << ",\"phaseTime\":" << r.torch.phaseTime
               << ",\"flameStrength\":" << r.torch.flameStrength << ",\"armLowerBlend\":" << r.torch.leftArmLowerBlend
               << ",\"fallProgress\":" << r.torch.fallProgress << ",\"droppedPosition\":["
               << r.torch.droppedX << ',' << r.torch.droppedY << ',' << r.torch.droppedZ
               << "],\"droppedYaw\":" << r.torch.droppedYawRadians << ",\"droppedViewPitch\":" << r.torch.droppedViewPitchRadians
               << ",\"droppedPitch\":" << r.torch.droppedPitchRadians
               << ",\"itemParent\":" << static_cast<int>(r.torchParent) << ",\"itemPosition\":["
               << r.torchItemPosition[0] << ',' << r.torchItemPosition[1] << ',' << r.torchItemPosition[2] << "]}"
               << ",\"playerAction\":" << static_cast<int>(r.combat.action)
               << ",\"sword\":{\"parent\":" << static_cast<int>(r.sword.parent)
               << ",\"transition\":" << static_cast<int>(r.sword.transition) << ",\"progress\":" << r.sword.progress
               << ",\"gripBlend\":" << r.sword.gripBlend << ",\"stowBlend\":" << r.sword.stowBlend
               << ",\"active\":" << (r.sword.active ? "true" : "false") << '}'
               << ",\"chestPhase\":" << static_cast<int>(r.chest.phase) << ",\"lid\":" << r.chest.lidOpenProgress
               << ",\"heldKind\":" << static_cast<int>(r.heldLight.heldLightKind) << ",\"heldPose\":" << static_cast<int>(r.heldLight.heldLightPose)
               << ",\"finalePhase\":" << static_cast<int>(r.finale.phase) << ",\"roof\":" << r.finale.skylightOpenProgress
               << ",\"dawn\":" << r.finale.dawnRevealProgress << ",\"fire\":[";
        for (std::size_t j = 0u; j < r.fire.size(); ++j) { if (j) output << ','; output << '['; for (std::size_t k = 0u; k < 4u; ++k) { if (k) output << ','; output << r.fire[j][k]; } output << ']'; }
        output << ']';
        if (r.simulationFireEmitterCount)
        {
            output << ",\"simulationFireEmitterCount\":" << *r.simulationFireEmitterCount
                   << ",\"simulationFireSources\":[";
            for (std::size_t j = 0u; j < r.simulationFireSources.size(); ++j)
            {
                if (j != 0u) output << ',';
                const auto& source = r.simulationFireSources[j];
                output << "{\"stableId\":" << source.stableId << ",\"strength\":" << source.strength
                       << ",\"fuel\":" << source.fuel << '}';
            }
            output << ']';
        }
        output << '}';
    }
    output << "],\"events\":[";
    for (std::size_t i = 0u; i < events_.size(); ++i)
    {
        const auto& r = events_[i]; const auto& e = r.event; if (i) output << ',';
        output << "{\"observedStateRow\":" << r.stateRow << ",\"sequence\":" << e.sequence << ",\"type\":" << static_cast<int>(e.type)
               << ",\"source\":" << static_cast<unsigned>(e.source) << ",\"target\":" << static_cast<unsigned>(e.target)
               << ",\"payload\":" << e.payload << ",\"world\":[" << e.worldX << ',' << e.worldY << ',' << e.worldZ
               << "],\"listener\":[" << e.listenerX << ',' << e.listenerZ << ',' << e.listenerYawRadians << "],\"intensity\":" << e.intensity << '}';
    }
    output << "],\"pipelines\":[";
    for (std::size_t i = 0u; i < pipelines_.size(); ++i)
    {
        const auto& p = pipelines_[i]; if (i) output << ',';
        output << "{\"executionMode\":" << static_cast<int>(p.executionMode) << ",\"activeKey\":"; Text(output, RtFixedTextView(p.active.key));
        output << ",\"activeSha256\":"; Text(output, RtFixedTextView(p.active.sha256));
        output << ",\"opaqueSha256\":"; Text(output, RtFixedTextView(p.opaqueFast.sha256));
        output << ",\"genericSha256\":"; Text(output, RtFixedTextView(p.genericDielectric.sha256)); output << '}';
    }
    output << "],\"completedRtFrames\":[";
    for (std::size_t i = 0u; i < frames_.size(); ++i)
    {
        const auto& r = frames_[i]; const auto& f = r.identity.submitted.frame; if (i) output << ',';
        output << "{\"stateRow\":" << r.stateRow << ",\"surfaceGeneration\":" << r.surfaceGeneration << ",\"sceneEpoch\":" << f.sceneEpoch
               << ",\"measurementGeneration\":" << f.measurementGeneration << ",\"tick\":" << f.simulationTick
               << ",\"recordAttempt\":" << f.recordAttemptSerial << ",\"record\":" << f.recordSerial
               << ",\"submission\":" << r.identity.submitted.submissionSerial << ",\"completion\":" << r.identity.completionSerial
               << ",\"frameSlot\":" << f.frameSlot << ",\"pipelineIndex\":" << r.pipelineIndex
               << ",\"gpuStatus\":" << static_cast<int>(r.gpuStatus) << ",\"gpuAvailable\":" << (r.gpuDurationAvailable ? "true" : "false")
               << ",\"gpuNs\":" << r.gpuNanoseconds << ",\"cpuStatus\":" << static_cast<int>(r.cpuStatus)
               << ",\"wholeFrameCpuNs\":" << r.wholeFrameCpuNanoseconds << ",\"simulationCpuNs\":" << r.simulationCpuNanoseconds
               << ",\"skinUpdates\":" << r.player.skinUpdateCount << ",\"socketErrorMicrometres\":" << r.player.maximumSocketErrorMicrometres
               << ",\"hostVisibleBytes\":" << r.resources.hostVisibleBytes << ",\"deviceLocalBytes\":" << r.resources.deviceLocalBytes;
        if (r.actualUploadedMistEnabled.has_value())
            output << ",\"actualUploadedMistEnabled\":" << (*r.actualUploadedMistEnabled ? "true" : "false");
        if (r.fireLighting)
        {
            output << ",\"fireLighting\":";
            WriteRtFireLightingEvidenceJson(output, *r.fireLighting);
        }
        output << '}';
    }
    output << "],\"resourceScopes\":[";
    for (std::size_t i = 0u; i < scopes_.size(); ++i)
    {
        const auto& scope = scopes_[i]; if (i) output << ',';
        output << "{\"surfaceGeneration\":" << scope.surfaceGeneration << ",\"sceneEpoch\":" << scope.sceneEpoch
               << ",\"measurementGeneration\":" << scope.measurementGeneration << ",\"nextStateRow\":" << scope.nextStateRow << '}';
    }
    output << "],\"retiredSubmissions\":[";
    for (std::size_t i = 0u; i < retirements_.size(); ++i)
    {
        const auto& r = retirements_[i]; const auto& f = r.identity.frame; if (i) output << ',';
        output << "{\"surfaceGeneration\":" << r.surfaceGeneration << ",\"sceneEpoch\":" << f.sceneEpoch
               << ",\"measurementGeneration\":" << f.measurementGeneration << ",\"recordAttempt\":" << f.recordAttemptSerial
               << ",\"record\":" << f.recordSerial << ",\"submission\":" << r.identity.submissionSerial
               << ",\"tick\":" << f.simulationTick << ",\"frameSlot\":" << f.frameSlot
               << ",\"stateRow\":" << r.stateRow << ",\"rtPresented\":false}";
    }
    output << "],\"pendingSubmissionCount\":"
           << std::count_if(pending_.begin(), pending_.end(), [](const auto& pending) { return pending.valid; })
           << ",\"currentScopePresented\":" << (currentScopePresented_ ? "true" : "false") << "}\n";
    output.imbue(previousLocale); output.flags(previousFlags); output.precision(previousPrecision);
}

} // namespace horde::telemetry
