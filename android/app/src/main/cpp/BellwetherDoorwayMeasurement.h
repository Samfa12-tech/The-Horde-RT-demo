#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <locale>
#include <optional>
#include <string>
#include <string_view>
#if HORDE_RT_DISPATCH_CLOCK_PROBE
#include <vector>
#endif

#include "gameplay/dialogue/RecordKeeperPlacement.h"
#include "gameplay/dialogue/TavernKeeperPlacement.h"
#include "gameplay/campaign/CampaignCheckpoint.h"
#include "gameplay/validation/BellwetherDoorwayProbe.h"
#include "gameplay/validation/BellwetherRoomFireContract.h"
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
    using Route = gameplay::validation::BellwetherMeasurementRoute;

    bool Start(const gameplay::simulation::SimulationSnapshot& state,
               const std::filesystem::path& report, TimePoint now,
               Route route = Route::Doorway) {
        route_ = route;
        clock_ = gameplay::validation::ChapterRouteMeasurementClock(
            ResidentRoundTrip() ? gameplay::validation::BellwetherDoorwayProbe::kResidentRoundTripDeadlineSeconds
                          : gameplay::validation::ChapterRouteMeasurementClock::kDefaultDeadlineSeconds);
        frames_.open(report, std::ios::trunc);
        frames_.imbue(std::locale::classic());
        if (!frames_ || !probe.Start(state, route_)) return false;
        start_ = now;
#if HORDE_RT_DISPATCH_CLOCK_PROBE
        report_ = report;
        runStartSteadyNs_ = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
        initialRoomTick_ = state.tickIndex;
#endif
        frames_ << "phase,station,returns,elapsed_ns,present_return_ns,scene_epoch,generation,record_serial,"
                   "submission_serial,tick,completion_serial,present_serial,gpu_serial,gpu_ns,cpu_cycle_ns,"
                   "backend,shader_sha256,internal_width,internal_height,output_width,output_height,glass_enabled,"
                   "fire_count,fire_ids,tlas_instances,blas_owners,device_local_bytes,host_visible_bytes,"
                   "kit_mask,window_mask,resident_mask,record_keeper_mask,tavern_door_mask";
        if (ResidentRoundTrip()) frames_ << ",resident_slot0_id,resident_slot1_id";
        if (probe.West()) frames_ << ",stock_head_instances,stock_pose_mask";
        for (const auto& column : kCpuStages) frames_ << ',' << column.name;
#if HORDE_RT_FRAME_STAGE_PROBE
        if (ResidentRoundTrip()) frames_ << ",gpu_stage_serial,gpu_as_ns,gpu_dispatch_ns,gpu_transfer_ns";
#endif
        frames_ << "\n";
        return static_cast<bool>(frames_);
    }

    std::optional<unsigned> StepsDue(TimePoint now) {
        return clock_.Consume(std::chrono::duration<double>(now - start_).count());
    }

    bool Bind(const telemetry::RtSubmittedFrameIdentity& identity, TimePoint presentReturned,
              const vulkan::raytracing::PresentableTinyRtScene& scene,
              const gameplay::simulation::SimulationSnapshot& state
#if HORDE_RT_DISPATCH_CLOCK_PROBE
              , std::uint32_t clockTicket
#endif
              ) {
        if (pending_ || !identity.submissionSerial || !identity.frame.recordSerial ||
            (lastSerial_ && (identity.submissionSerial != lastSerial_ + 1 ||
                             identity.frame.sceneEpoch != epoch_ ||
                             identity.frame.measurementGeneration != generation_))) return false;
        epoch_ = identity.frame.sceneEpoch;
        generation_ = identity.frame.measurementGeneration;
        using Scene = vulkan::raytracing::PresentableTinyRtScene;
        const auto phase = probe.PhaseName();
#if HORDE_RT_DISPATCH_CLOCK_PROBE
        bool captureHearthClock = false;
        std::uint8_t clockSampleIndex = 0;
        std::uint64_t roomDwellTicks = 0;
        if (probe.West() && phase == "room" && probe.Returns() == 0 &&
            state.tickIndex >= initialRoomTick_) {
            roomDwellTicks = state.tickIndex - initialRoomTick_;
            for (std::size_t i = 0; i < clockCaptureWindows_.size(); ++i) {
                const auto [begin, end] = clockCaptureWindows_[i];
                if (!clockCaptureSaved_[i] && roomDwellTicks >= begin && roomDwellTicks <= end) {
                    clockCaptureSaved_[i] = true;
                    captureHearthClock = true;
                    clockSampleIndex = static_cast<std::uint8_t>(i + 1);
                    break;
                }
            }
        }
#endif
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
            ResidentRoundTrip() ? scene.ChapterResidentSourceIds() : std::array<std::uint32_t, 2>{},
            phase, probe.Station(), probe.Returns(),
            probe.West() ? scene.SubmittedStockHeadCount() : 0u,
            probe.West() ? gameplay::validation::BellwetherStockPoseMask(state) : 0u
#if HORDE_RT_DISPATCH_CLOCK_PROBE
            , captureHearthClock ? clockTicket : 0u, roomDwellTicks, clockSampleIndex
#endif
            };
        return true;
    }

#if HORDE_RT_DISPATCH_CLOCK_PROBE
    // The clock observer is a shared buffer. Copy only selected settled-view
    // records after their owning fence and before a later dispatch can replace
    // the ticketed samples.
    bool CapturePendingDispatchClock(VkDevice device, VkFence fence,
                                     const vulkan::raytracing::PresentableTinyRtScene& scene,
                                     std::string& diagnostic) {
        if (!pending_) {
            diagnostic = "Selected dispatch clock has no pending submitted frame.";
            return false;
        }
        if (!pending_->clockSampleIndex) return true;
        if (!pending_->clockTicket) {
            diagnostic = "Selected dispatch clock has no nonzero shader ticket.";
            return false;
        }
        const VkResult waitResult = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        if (waitResult != VK_SUCCESS) {
            diagnostic = "Selected dispatch clock frame fence failed with VkResult " + std::to_string(waitResult) + ".";
            return false;
        }
        std::string error;
        if (!scene.CollectDispatchClocks(pending_->clockTicket, pending_->clockSamples,
                pending_->clockTotals, error)) {
            diagnostic = "Selected dispatch clock ticket/grid validation failed: " + error;
            return false;
        }
        pending_->clockCaptured = true;
        return true;
    }
#endif

    bool AwaitingFrame() const { return pending_.has_value(); }
    Route MeasurementRoute() const { return route_; }
    bool ResidentRoundTrip() const { return gameplay::validation::IsBellwetherResidentRoundTrip(route_); }

    bool CompleteFence(const telemetry::RtPerformanceEvidenceSnapshot& sample
#if HORDE_RT_FRAME_STAGE_PROBE
                       , const vulkan::raytracing::RtSceneGpuStageCompletedObservation& gpuStages
#endif
                       , std::string& diagnostic
                       ) {
        diagnostic.clear();
        using namespace telemetry;
        const auto& tag = *pending_;
        const auto& frame = sample.identity.submitted;
        const auto serial = frame.submissionSerial;
        const auto phase = tag.phase;
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
            (phase == "room" && (!gameplay::validation::HasBellwetherRoomFireAdmission(*sample.scene.fireLighting) ||
                                 tag.masks != std::array<unsigned, 4>{1, tag.glassEnabled ? 1u : 0u, 1, 1})) ||
            (ResidentRoundTrip() && (tag.masks[2] != 1 ||
                ((phase == "room" || phase == "complete" || phase == "east-fork" || phase == "stock-yard") &&
                 tag.residentSourceIds != ExpectedResidentSourceIds(phase)))) ||
            (phase == "stock-yard" && tag.stockHeadInstances != 3u)) {
            diagnostic = "Completed Bellwether evidence did not match its submitted, presented frame or workload.";
            return false;
        }

        const auto& resources = sample.scene.resources;
        const auto& dimensions = *sample.scene.dimensions;
        const auto& stages = sample.scene.stages.values;
        const auto& cycle = stages[RtStageIndex(RtStage::WholeFrameCycle)];
        frames_ << phase << ',' << tag.station << ',' << tag.returns << ','
                << tag.elapsedNs << ',' << tag.presentReturnNs << ',' << epoch_ << ',' << generation_ << ','
                << frame.frame.recordSerial << ',' << serial << ',' << frame.frame.simulationTick << ','
                << sample.identity.completionSerial << ',' << sample.presentation.lastSuccessfulPresentSubmissionSerial << ','
                << sample.gpu.completedSubmissionSerial << ',' << sample.gpu.durationNanoseconds << ',' << cycle.durationNanoseconds << ','
                << RtExecutionModeName(sample.scene.pipeline.executionMode) << ',' << RtFixedTextView(sample.scene.pipeline.active.sha256) << ','
                << dimensions.internalWidth << ',' << dimensions.internalHeight << ',' << dimensions.outputWidth << ','
                << dimensions.outputHeight << ',' << (tag.glassEnabled ? 1 : 0) << ','
                << sample.scene.fireLighting->count << ',';
        for (std::uint32_t i = 0; i < sample.scene.fireLighting->count; ++i) {
            if (i) frames_ << ';';
            frames_ << sample.scene.fireLighting->emitters[i].stableId;
        }
        frames_ << ',' << resources.tlasInstanceCount << ','
                << resources.bottomLevelAccelerationStructureCount << ',' << resources.deviceLocalBytes << ',' << resources.hostVisibleBytes;
        for (const unsigned mask : tag.masks) frames_ << ',' << mask;
        frames_ << ',' << tag.tavernDoorMask;
        if (ResidentRoundTrip()) frames_ << ',' << tag.residentSourceIds[0] << ',' << tag.residentSourceIds[1];
        if (probe.West()) frames_ << ',' << tag.stockHeadInstances << ',' << tag.stockPoseMask;
        for (const auto& column : kCpuStages)
            frames_ << ',' << stages[RtStageIndex(column.stage)].durationNanoseconds;
#if HORDE_RT_FRAME_STAGE_PROBE
        if (ResidentRoundTrip()) {
            if (vulkan::raytracing::RtSceneGpuStagesMatchCompletedFrame(gpuStages, frame)) {
                frames_ << ',' << gpuStages.submitted.submissionSerial;
                for (const auto& collection : gpuStages.collections)
                    frames_ << ',' << std::llround(collection.sample.milliseconds * 1e6);
            } else {
                frames_ << ",0,,,";
            }
        }
#endif
        frames_ << '\n';
#if HORDE_RT_DISPATCH_CLOCK_PROBE
        if (tag.clockSampleIndex) {
            frames_.flush();
            if (!frames_) {
                diagnostic = "Bellwether CSV row flush failed before clock witness publication.";
                return false;
            }
            if (!WriteDispatchClockWitness(tag, sample, diagnostic)) return false;
        }
#endif
        lastSerial_ = serial;
        pending_.reset();
        if (probe.Complete()) frames_.close();
        return static_cast<bool>(frames_);
    }

    gameplay::validation::BellwetherDoorwayProbe probe;

private:
    using Stage = telemetry::RtStage;
    struct CpuStageColumn {
        const char* name;
        Stage stage;
    };
    static constexpr CpuStageColumn kCpuStages[] = {
        {"simulation_step_ns", Stage::SimulationStep},
        {"player_skin_ns", Stage::PlayerSkin},
        {"character_skin_ns", Stage::CharacterSkin},
        {"dynamic_upload_ns", Stage::DynamicUpload},
        {"blas_refit_record_ns", Stage::BlasRefitRecord},
        {"tlas_update_record_ns", Stage::TlasUpdateRecord},
        {"trace_copy_record_ns", Stage::TraceCopyRecord},
        {"frame_fence_wait_ns", Stage::FrameFenceWait},
        {"image_acquire_ns", Stage::ImageAcquire},
        {"queue_submit_ns", Stage::QueueSubmit},
        {"present_call_ns", Stage::PresentCall},
    };

    static std::array<std::uint32_t, 2> ExpectedResidentSourceIds(const std::string_view phase) {
        if (phase == "east-fork")
            return {gameplay::bellwether::kArtificerActorId, gameplay::bellwether::kSalvagerActorId};
        if (phase == "stock-yard")
            return {gameplay::dialogue::kTavernKeeperActorId, gameplay::bellwether::kRelicDealerActorId};
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
        std::string_view phase;
        std::size_t station;
        unsigned returns, stockHeadInstances, stockPoseMask;
#if HORDE_RT_DISPATCH_CLOCK_PROBE
        std::uint32_t clockTicket;
        std::uint64_t roomDwellTicks;
        std::uint8_t clockSampleIndex = 0;
        bool clockCaptured = false;
        std::vector<vulkan::raytracing::RtDispatchClockSample> clockSamples;
        std::array<std::uint64_t,8> clockTotals{};
#endif
    };

#if HORDE_RT_DISPATCH_CLOCK_PROBE
    bool WriteDispatchClockWitness(const Frame& tag,
            const telemetry::RtPerformanceEvidenceSnapshot& sample, std::string& diagnostic) const {
        using namespace telemetry;
        if (!tag.clockCaptured || tag.clockSamples.empty() || !sample.scene.dimensions) {
            diagnostic = "Completed hearth row has no retained clock payload or dimensions.";
            return false;
        }
        const auto& frame = sample.identity.submitted;
        const auto& dimensions = *sample.scene.dimensions;
        const auto stem = std::string("bellwether-westroundtrip-hearth-clock-") +
            std::to_string(runStartSteadyNs_) + "-return-1-sample-" +
            std::to_string(tag.clockSampleIndex) + "-" + std::to_string(frame.submissionSerial);
        const auto rawPath = report_.parent_path() / (stem + ".bin");
        const auto metadataPath = report_.parent_path() / (stem + ".json");
        auto rawTempPath = rawPath;
        rawTempPath += ".tmp";
        auto metadataTempPath = metadataPath;
        metadataTempPath += ".tmp";
        std::error_code error;
        if (std::filesystem::exists(rawPath, error) || error || std::filesystem::exists(metadataPath, error) || error ||
            std::filesystem::exists(rawTempPath, error) || error || std::filesystem::exists(metadataTempPath, error) || error) {
            diagnostic = "Dispatch clock witness path already exists or cannot be checked; refusing overwrite.";
            return false;
        }
        std::ofstream raw(rawTempPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!raw) {
            diagnostic = "Dispatch clock raw witness could not be opened.";
            std::filesystem::remove(rawTempPath, error);
            return false;
        }
        raw.write(reinterpret_cast<const char*>(tag.clockSamples.data()),
            static_cast<std::streamsize>(tag.clockSamples.size() * sizeof(tag.clockSamples.front())));
        raw.flush();
        bool saved = static_cast<bool>(raw);
        raw.close();
        saved = saved && !raw.fail();
        if (!saved) {
            diagnostic = "Dispatch clock raw witness write or close failed.";
            std::filesystem::remove(rawTempPath, error);
            return false;
        }
        std::ofstream metadata(metadataTempPath, std::ios::out | std::ios::trunc);
        if (!metadata) {
            diagnostic = "Dispatch clock metadata temporary file could not be opened.";
            std::filesystem::remove(rawTempPath, error);
            std::filesystem::remove(metadataTempPath, error);
            return false;
        }
        metadata.imbue(std::locale::classic());
        metadata << "{\"format\":\"horde-rt-dispatch-clock-v1\","
            << "\"raw_file\":\"" << rawPath.filename().string() << "\","
            << "\"record_size_bytes\":" << sizeof(vulkan::raytracing::RtDispatchClockSample) << ','
            << "\"record_count\":" << tag.clockSamples.size() << ','
            << "\"room_id\":\"bellwether_tavern_staffed_hearth\","
            << "\"checkpoint_id\":" << static_cast<std::uint32_t>(gameplay::campaign::CheckpointId::BellwetherTavern) << ','
            << "\"measurement_checkpoint_id\":349,\"route\":\"west-roundtrip\",\"phase\":\"room\","
            << "\"return_index\":1,\"sample_index\":" << static_cast<unsigned>(tag.clockSampleIndex) << ','
            << "\"room_dwell_ticks\":" << tag.roomDwellTicks << ','
            << "\"scene_epoch\":" << frame.frame.sceneEpoch << ','
            << "\"measurement_generation\":" << frame.frame.measurementGeneration << ','
            << "\"record_serial\":" << frame.frame.recordSerial << ','
            << "\"submission_serial\":" << frame.submissionSerial << ','
            << "\"completion_serial\":" << sample.identity.completionSerial << ','
            << "\"presented_serial\":" << sample.presentation.lastSuccessfulPresentSubmissionSerial << ','
            << "\"simulation_tick\":" << frame.frame.simulationTick << ','
            << "\"clock_ticket\":" << tag.clockTicket << ','
            << "\"internal_width\":" << dimensions.internalWidth << ','
            << "\"internal_height\":" << dimensions.internalHeight << ','
            << "\"output_width\":" << dimensions.outputWidth << ','
            << "\"output_height\":" << dimensions.outputHeight << ','
            << "\"pipeline_sha256\":\"" << RtFixedTextView(sample.scene.pipeline.active.sha256) << "\","
            << "\"execution_mode\":\"" << RtExecutionModeName(sample.scene.pipeline.executionMode) << "\","
            << "\"category_order\":[\"other\",\"trace\",\"material\",\"shadow\",\"surface\",\"fire\",\"mist\",\"dust\"],"
            << "\"category_ticks\":[";
        for (std::size_t i = 0; i < tag.clockTotals.size(); ++i) {
            if (i) metadata << ',';
            metadata << tag.clockTotals[i];
        }
        metadata << "]}\n";
        metadata.flush();
        saved = static_cast<bool>(metadata);
        metadata.close();
        saved = saved && !metadata.fail();
        if (!saved) {
            diagnostic = "Dispatch clock metadata write or close failed.";
            std::filesystem::remove(rawTempPath, error);
            std::filesystem::remove(metadataTempPath, error);
            return false;
        }
        std::filesystem::rename(rawTempPath, rawPath, error);
        if (error) {
            diagnostic = "Dispatch clock raw witness publish failed: " + error.message();
            std::filesystem::remove(rawTempPath, error);
            std::filesystem::remove(metadataTempPath, error);
            return false;
        }
        std::filesystem::rename(metadataTempPath, metadataPath, error);
        if (error) {
            diagnostic = "Dispatch clock metadata publish failed: " + error.message();
            std::filesystem::remove(rawPath, error);
            std::filesystem::remove(metadataTempPath, error);
            return false;
        }
        return true;
    }
#endif

    std::ofstream frames_;
#if HORDE_RT_DISPATCH_CLOCK_PROBE
    std::filesystem::path report_;
    std::uint64_t runStartSteadyNs_ = 0;
    std::uint64_t initialRoomTick_ = 0;
#endif
    gameplay::validation::ChapterRouteMeasurementClock clock_;
    TimePoint start_{};
#if HORDE_RT_DISPATCH_CLOCK_PROBE
    static constexpr std::array<std::array<std::uint64_t,2>,3> clockCaptureWindows_{{
        {{300u,330u}}, {{360u,390u}}, {{420u,450u}}}};
    std::array<bool,3> clockCaptureSaved_{};
#endif
    std::optional<Frame> pending_;
    std::uint64_t epoch_ = 0;
    std::uint64_t generation_ = 0;
    std::uint64_t lastSerial_ = 0;
    Route route_ = Route::Doorway;
};

} // namespace horde::platform::android
