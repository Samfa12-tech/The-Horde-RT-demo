#pragma once
#include "scene/assets/SkinnedPbrActor.h"
#include <array>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

namespace horde::scene::assets
{
// One exclusive CPU actor lane. Never submit the same actor to another worker
// or read/move/reload it until the returned Job joins. No Vulkan or telemetry
// accumulator is reachable here. The owner publishes the joined result.
class SkinnedPbrActorWorker
{
public:
    struct Request
    {
        std::uint64_t identity = 0, generation = 0;
        SkinnedClip clip = SkinnedClip::Idle;
        float time = 0;
        SkinnedClipBlend blend{};
        bool hasBlend = false, measure = false;
        struct Rotation { std::array<char, 64> name{}; std::array<float, 4> value{}; };
        std::array<Rotation, 8> rotations{};
        std::size_t rotationCount = 0;

        bool SetRotations(std::span<const SkinnedNodeRotation> source)
        {
            rotationCount = 0;
            if (source.size() > rotations.size()) return false;
            for (std::size_t i = 0; i < source.size(); ++i)
            {
                if (source[i].nodeName.empty() || source[i].nodeName.size() >= rotations[i].name.size())
                    return false;
                rotations[i].name.fill(0);
                std::copy(source[i].nodeName.begin(), source[i].nodeName.end(), rotations[i].name.begin());
                rotations[i].value = source[i].rotation;
            }
            rotationCount = source.size();
            return true;
        }
    };
    struct Result
    {
        std::uint64_t identity = 0, generation = 0, poseNanoseconds = 0;
        bool success = false;
        SkinWorkMetrics work{};
        std::string diagnostic;
    };
private:
    struct State
    {
        explicit State(SkinnedPbrActor& value) : actor(value) {}
        SkinnedPbrActor& actor;
        std::mutex mutex;
        std::condition_variable changed;
        Request request;
        Result result;
        bool active = false, queued = false, ready = false, stopping = false;
    };
    std::shared_ptr<State> state_;
    std::thread thread_;

    static void Run(const std::shared_ptr<State>& state)
    {
        for (;;)
        {
            Request request;
            {
                std::unique_lock lock(state->mutex);
                state->changed.wait(lock, [&] { return state->stopping || state->queued; });
                if (!state->queued) return;
                request = state->request;
                state->queued = false;
            }
            Result result;
            result.identity = request.identity;
            result.generation = request.generation;
            const auto start = std::chrono::steady_clock::now();
            try
            {
                std::array<SkinnedNodeRotation, 8> rotations;
                for (std::size_t i = 0; i < request.rotationCount; ++i)
                    rotations[i] = {request.rotations[i].name.data(), request.rotations[i].value};
                result.success = state->actor.Pose(request.clip, request.time, result.diagnostic,
                    std::span(rotations).first(request.rotationCount),
                    request.hasBlend ? &request.blend : nullptr, request.measure ? &result.work : nullptr);
            }
            catch (const std::exception& error) { result.diagnostic = error.what(); }
            catch (...) { result.diagnostic = "Actor CPU preparation failed with an unknown exception."; }
            result.poseNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - start).count();
            {
                std::lock_guard lock(state->mutex);
                state->result = std::move(result);
                state->ready = true;
            }
            state->changed.notify_all();
        }
    }
public:
    explicit SkinnedPbrActorWorker(SkinnedPbrActor& actor)
        : state_(std::make_shared<State>(actor)), thread_([state = state_] { Run(state); }) {}
    ~SkinnedPbrActorWorker() { Stop(); }
    SkinnedPbrActorWorker(const SkinnedPbrActorWorker&) = delete;
    SkinnedPbrActorWorker& operator=(const SkinnedPbrActorWorker&) = delete;
    // Finishes an admitted job before retiring the thread. A still-live Job can
    // consume its result after Stop; no subsequent request is accepted.
    void Stop()
    {
        { std::lock_guard lock(state_->mutex); state_->stopping = true; }
        state_->changed.notify_all();
        if (thread_.joinable()) thread_.join();
    }
    class Job
    {
        std::shared_ptr<State> state_;
    public:
        Job() = default;
        ~Job() { if (state_) (void)Wait(); }
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;
        bool Start(SkinnedPbrActorWorker& worker, const Request& request)
        {
            if (state_ || request.rotationCount > request.rotations.size()) return false;
            for (std::size_t i = 0; i < request.rotationCount; ++i)
                if (request.rotations[i].name.front() == 0 || request.rotations[i].name.back() != 0)
                    return false;
            {
                std::lock_guard lock(worker.state_->mutex);
                if (worker.state_->active || worker.state_->stopping) return false;
                worker.state_->request = request;
                worker.state_->active = worker.state_->queued = true;
                worker.state_->ready = false;
                state_ = worker.state_;
            }
            state_->changed.notify_one();
            return true;
        }
        bool Active() const { return bool(state_); }
        Result Wait()
        {
            if (!state_) return {};
            auto state = std::move(state_);
            std::unique_lock lock(state->mutex);
            state->changed.wait(lock, [&] { return state->ready; });
            Result result = std::move(state->result);
            state->active = state->ready = false;
            return result;
        }
    };
};
}
