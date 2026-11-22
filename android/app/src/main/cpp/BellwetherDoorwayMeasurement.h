#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <locale>
#include <optional>

#include "gameplay/dialogue/RecordKeeperPlacement.h"
#include "gameplay/dialogue/TavernKeeperPlacement.h"
#include "gameplay/validation/BellwetherDoorwayProbe.h"
#include "gameplay/validation/ChapterRouteMeasurementClock.h"
#include "gameplay/traversal/BellwetherMill.h"
#include "gameplay/traversal/BellwetherSalvager.h"
#include "telemetry/RtPerformanceEvidence.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::platform::android {

// The private Debug route owns one submitted frame at a time. Write a row only
// after the owning fence proves its RT timestamp and accepted present.
class BellwetherDoorwayMeasurement {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    bool Start(const gameplay::simulation::SimulationSnapshot& state,
               const std::filesystem::path& report, TimePoint now,
               bool combinedEast = false) {
        combinedEast_ = combinedEast;
        clock_ = gameplay::validation::ChapterRouteMeasurementClock(
            combinedEast_ ? gameplay::validation::BellwetherDoorwayProbe::kResidentRoundTripDeadlineSeconds
                          : gameplay::validation::ChapterRouteMeasurementClock::kDefaultDeadlineSeconds);
        frames_.open(report, std::ios::trunc);
        frames_.imbue(std::locale::classic());
        if (!frames_ || !probe.Start(state, combinedEast_)) return false;
        start_ = now;
        frames_ << "phase,station,returns,elapsed_ns,present_return_ns,scene_epoch,generation,record_serial,"
                   "submission_serial,tick,completion_serial,present_serial,gpu_serial,gpu_ns,cpu_cycle_ns,"
                   "backend,shader_sha256,internal_width,internal_height,output_width,output_height,glass_enabled,"
                   "fire_count,tlas_instances,blas_owners,device_local_bytes,host_visible_bytes,"
                   "kit_mask,window_mask,resident_mask,record_keeper_mask,tavern_door_mask";
        if (combinedEast_) frames_ << ",resident_slot0_id,resident_slot1_id";
        frames_ << "\n";
        return static_cast<bool>(frames_);
    }

    std::optional<unsigned> StepsDue(TimePoint now) {
        return clock_.Consume(std::chrono::duration<double>(now - start_).count());
    }

    bool Bind(const telemetry::RtSubmittedFrameIdentity& identity, TimePoint presentReturned,
              const vulkan::raytracing::PresentableTinyRtScene& scene) {
        if (pending_ || !identity.submissionSerial || !identity.frame.recordSerial ||
            (lastSerial_ && (identity.submissionSerial != lastSerial_ + 1 ||
                             identity.frame.sceneEpoch != epoch_ ||
                             identity.frame.measurementGeneration != generation_))) return false;
        epoch_ = identity.frame.sceneEpoch;
        generation_ = identity.frame.measurementGeneration;
        using Scene = vulkan::raytracing::PresentableTinyRtScene;
        pending_ = Frame{
            identity,
            std::chrono::duration_cast<std::chrono::nanoseconds>(presentReturned - start_).count(),
            std::chrono::duration_cast<std::chrono::nanoseconds>(presentReturned.time_since_epoch()).count(),
            scene.GlassEnabled(),
            {scene.SubmittedInstanceMask(Scene::kKeeperMetadataIndex),
             scene.SubmittedInstanceMask(Scene::kChapterWindowMetadataIndex),
             scene.SubmittedInstanceMask(Scene::kFirstResidentMetadataIndex),
             scene.SubmittedInstanceMask(Scene::kFirstResidentMetadataIndex + 1)},
            scene.SubmittedInstanceMask(Scene::kTavernDoorMetadataIndex),
            combinedEast_ ? scene.ChapterResidentSourceIds() : std::array<std::uint32_t, 2>{}};
        return true;
    }

    bool AwaitingFrame() const { return pending_.has_value(); }
    bool CombinedEast() const { return combinedEast_; }

    bool CompleteFence(const telemetry::RtPerformanceEvidenceSnapshot& sample) {
        using namespace telemetry;
        const auto& tag = *pending_;
        const auto& frame = sample.identity.submitted;
        const auto serial = frame.submissionSerial;
        const auto phase = probe.PhaseName();
        if (serial != tag.identity.submissionSerial ||
            frame.frame.recordSerial != tag.identity.frame.recordSerial ||
            frame.frame.simulationTick != tag.identity.frame.simulationTick ||
            frame.frame.sceneEpoch != epoch_ || frame.frame.measurementGeneration != generation_ ||
            sample.identity.completionSerial != serial || !sample.benchmarkEligible ||
            sample.presentation.outcome != RtPresentationOutcome::Presented ||
            sample.presentation.lastSuccessfulPresentSubmissionSerial != serial ||
            sample.gpu.status != RtSampleStatus::Valid || !sample.gpu.hasDuration ||
            sample.gpu.completedSubmissionSerial != serial || sample.gpu.durationNanoseconds == 0 ||
            !sample.scene.fireLighting || !sample.scene.dimensions || tag.masks[3] != 1 || tag.tavernDoorMask != 1 ||
            (phase == "room" && (sample.scene.fireLighting->count != 3 ||
                                 tag.masks != std::array<unsigned, 4>{1, tag.glassEnabled ? 1u : 0u, 1, 1})) ||
            (combinedEast_ && (tag.masks[2] != 1 ||
                               ((phase == "room" || phase == "complete" || phase == "east-fork") &&
                                tag.residentSourceIds != ExpectedResidentSourceIds(phase))))) return false;

        const auto& resources = sample.scene.resources;
        const auto& dimensions = *sample.scene.dimensions;
        const auto& cycle = sample.scene.stages.values[static_cast<std::size_t>(RtStage::WholeFrameCycle)];
        frames_ << phase << ',' << probe.Station() << ',' << probe.Returns() << ','
                << tag.elapsedNs << ',' << tag.presentReturnNs << ',' << epoch_ << ',' << generation_ << ','
                << frame.frame.recordSerial << ',' << serial << ',' << frame.frame.simulationTick << ','
                << sample.identity.completionSerial << ',' << sample.presentation.lastSuccessfulPresentSubmissionSerial << ','
                << sample.gpu.completedSubmissionSerial << ',' << sample.gpu.durationNanoseconds << ',' << cycle.durationNanoseconds << ','
                << RtExecutionModeName(sample.scene.pipeline.executionMode) << ',' << RtFixedTextView(sample.scene.pipeline.active.sha256) << ','
                << dimensions.internalWidth << ',' << dimensions.internalHeight << ',' << dimensions.outputWidth << ','
                << dimensions.outputHeight << ',' << (tag.glassEnabled ? 1 : 0) << ','
                << sample.scene.fireLighting->count << ',' << resources.tlasInstanceCount << ','
                << resources.bottomLevelAccelerationStructureCount << ',' << resources.deviceLocalBytes << ',' << resources.hostVisibleBytes;
        for (const unsigned mask : tag.masks) frames_ << ',' << mask;
        frames_ << ',' << tag.tavernDoorMask;
        if (combinedEast_) frames_ << ',' << tag.residentSourceIds[0] << ',' << tag.residentSourceIds[1];
        frames_ << '\n';
        lastSerial_ = serial;
        pending_.reset();
        if (probe.Complete()) frames_.close();
        return static_cast<bool>(frames_);
    }

    gameplay::validation::BellwetherDoorwayProbe probe;

private:
    static std::array<std::uint32_t, 2> ExpectedResidentSourceIds(const std::string_view phase) {
        if (phase == "east-fork")
            return {gameplay::bellwether::kArtificerActorId, gameplay::bellwether::kSalvagerActorId};
        return {gameplay::dialogue::kTavernKeeperActorId, gameplay::dialogue::kRecordKeeperActorId};
    }

    struct Frame {
        telemetry::RtSubmittedFrameIdentity identity;
        std::int64_t elapsedNs;
        std::int64_t presentReturnNs;
        bool glassEnabled;
        std::array<unsigned, 4> masks;
        unsigned tavernDoorMask;
        std::array<std::uint32_t, 2> residentSourceIds;
    };

    std::ofstream frames_;
    gameplay::validation::ChapterRouteMeasurementClock clock_;
    TimePoint start_{};
    std::optional<Frame> pending_;
    std::uint64_t epoch_ = 0;
    std::uint64_t generation_ = 0;
    std::uint64_t lastSerial_ = 0;
    bool combinedEast_ = false;
};

} // namespace horde::platform::android
