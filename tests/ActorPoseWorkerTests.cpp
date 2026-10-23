#include "scene/assets/SkinnedPbrActorWorker.h"
#include "scene/CompanionPose.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace
{
using namespace horde::scene;
using namespace horde::scene::assets;
using Worker = SkinnedPbrActorWorker;
using Clock = std::chrono::steady_clock;
int checks = 0;
void Check(bool pass, const char* description)
{
    ++checks;
    if (!pass) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
SkinnedClipSet Clips()
{
    SkinnedClipSet clips;
    clips.clips[0] = {"Warden|idle", true, true};
    clips.clips[1] = {"Warden|walk", true, true};
    clips.clips[2] = {"", false, false}; clips.clips[3] = {"", false, false};
    return clips;
}
void Load(SkinnedPbrActor& actor)
{
    std::string error;
    Check(actor.Load(std::filesystem::path(HORDE_RT_SOURCE_DIR) /
        "assets/models/characters/runtime/kit-warden-v1", "kit-warden-lod0.runtime.glb", Clips(), error),
        error.c_str());
}
Worker::Request Request(unsigned tick)
{
    Worker::Request request;
    request.identity = tick + 1; request.generation = tick / 27;
    request.time = float(tick) / 60;
    request.hasBlend = true;
    request.blend = {SkinnedClip::Walking, request.time, float(tick % 3) * .5f};
    horde::gameplay::dialogue::CompanionActingPose acting;
    acting.headPitch = .03f * std::sin(request.time * 5);
    acting.headYaw = .025f * std::sin(request.time * 2.3f);
    acting.chestPitch = .012f * std::sin(request.time * 3.1f);
    const auto rotations = CompanionUpperBodyRotations(acting);
    Check(request.SetRotations(rotations), "bounded owned rotation request");
    return request;
}
bool Serial(SkinnedPbrActor& actor, const Worker::Request& request, std::string& error,
            SkinWorkMetrics* work = nullptr)
{
    std::array<SkinnedNodeRotation, 8> rotations;
    for (std::size_t i = 0; i < request.rotationCount; ++i)
        rotations[i] = {request.rotations[i].name.data(), request.rotations[i].value};
    return actor.Pose(request.clip, request.time, error, std::span(rotations).first(request.rotationCount),
        request.hasBlend ? &request.blend : nullptr, work);
}
bool Same(const SkinnedPbrActor& a, const SkinnedPbrActor& b)
{
    return a.Asset().vertices.size() == b.Asset().vertices.size() &&
        std::memcmp(a.Asset().vertices.data(), b.Asset().vertices.data(),
            a.Asset().vertices.size() * sizeof(StaticRtVertex)) == 0 &&
        a.Asset().bounds.minimum == b.Asset().bounds.minimum &&
        a.Asset().bounds.maximum == b.Asset().bounds.maximum;
}
std::uint64_t Hash(std::uint64_t value, const void* data, std::size_t bytes)
{
    const auto* source = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < bytes; ++i) { value ^= source[i]; value *= 1099511628211ull; }
    return value;
}
std::uint64_t Ns(Clock::time_point start)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}
int Contracts()
{
    SkinnedPbrActor serial, threaded;
    Load(serial); Load(threaded);
    Worker worker(threaded);
    for (unsigned tick = 0; tick < 81; ++tick)
    {
        auto request = Request(tick);
        const auto expected = request;
        request.measure = true;
        Worker::Job job, duplicate;
        Check(job.Start(worker, request), "start exclusive job");
        Check(!duplicate.Start(worker, request), "reject second job while outstanding, even if completed");
        request.time += 99; request.rotations[0].name.fill('x'); request.blend.weight = 0;
        std::string error;
        SkinWorkMetrics serialWork;
        Check(Serial(serial, expected, error, &serialWork), error.c_str());
        const auto result = job.Wait();
        Check(result.success && result.identity == expected.identity && result.generation == expected.generation,
            "result retains owned input identity and generation");
        Check(Same(serial, threaded), "exact actual-rig vertex and grounded bounds parity");
        Check(result.work.vertices > 0 && result.work.vertices == serialWork.vertices &&
            result.work.joints == serialWork.joints && result.work.copies == serialWork.copies &&
            result.work.copyBytes == serialWork.copyBytes && result.poseNanoseconds > 0,
            "worker-local metrics are returned only after joining");
        Check(!job.Active(), "joined ticket releases actor ownership");
    }
    const auto request = Request(99);
    {
        Worker::Job earlyReturn;
        Check(earlyReturn.Start(worker, request), "early-return job started");
        // Destructor is the same guard used by owner paths that return before upload.
    }
    std::string error;
    Check(Serial(serial, request, error) && Same(serial, threaded), "early-return guard joins before actor access");
    Worker::Job retirement;
    Check(retirement.Start(worker, Request(100)), "retirement with admitted job");
    worker.Stop(); worker.Stop();
    Check(retirement.Wait().success, "stop drains job and preserves joined result");
    Worker::Job stopped;
    Check(!stopped.Start(worker, request), "retired worker rejects new work");
    Load(threaded); // No thread can retain this actor's old scratch.
    {
        Worker replacement(threaded);
        Worker::Job job;
        Check(job.Start(replacement, request), "reload gets a new exclusive worker");
        Check(job.Wait().success && Same(serial, threaded), "reload preserves output");
        auto invalid = request;
        invalid.rotationCount = invalid.rotations.size() + 1;
        Check(!job.Start(replacement, invalid), "invalid bounded request rejected before access");
    }
    SkinnedPbrActor empty;
    Worker failureWorker(empty);
    Worker::Job failure;
    Check(failure.Start(failureWorker, request), "failure job admitted");
    const auto failed = failure.Wait();
    Check(!failed.success && !failed.diagnostic.empty(), "pose failure returned without owner diagnostic mutation");
    std::cout << checks << " actor worker contracts passed\n";
    return 0;
}
int Benchmark(bool threaded, unsigned samples)
{
    using namespace horde::vulkan::raytracing;
    using namespace horde::gameplay::animation;
    SkinnedPbrActor actor; Load(actor);
    const auto root = std::filesystem::path(HORDE_RT_SOURCE_DIR);
    PlayerRenderSlot player;
    SkinnedMeshAsset viewmodel;
    std::string error;
    Check(player.LoadAsset((root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(), error), error.c_str());
    Check(viewmodel.LoadClips((root / "assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),
        PlayerLocomotionClipSet(), error), error.c_str());
    std::unique_ptr<Worker> worker;
    if (threaded) worker = std::make_unique<Worker>(actor);
    std::vector<TexturedSkinnedRtVertex> viewVertices;
    std::vector<SkinnedPbrTangent> viewTangents;
    std::vector<std::uint64_t> critical, dispatch, join, actorWork, playerWork;
    std::uint64_t digest = 14695981039346656037ull;
    constexpr unsigned warmup = 30;
    for (unsigned tick = 0; tick < warmup + samples; ++tick)
    {
        auto request = Request(tick);
        PlayerAnimationSnapshot animation;
        animation.locomotionClip = PlayerLocomotionClip::Walk;
        animation.locomotionBlend = 1; animation.locomotionTime = float(tick) / 60;
        // Exact anatomical fixture targets used by the existing skin benchmark.
        for (auto* arm : {&animation.leftIk, &animation.rightIk})
        { arm->gripX = {{1,0,0}}; arm->gripY = {{0,1,0}}; arm->gripZ = {{0,0,1}}; }
        std::array<float, 3> center{};
        Check(player.ShoulderCenter(animation, center, error), error.c_str());
        for (unsigned axis = 0; axis < 3; ++axis)
            animation.leftIk.shoulder[axis] = animation.rightIk.shoulder[axis] = center[axis];
        animation.leftIk.shoulder[0] -= .36f; animation.rightIk.shoulder[0] += .36f;
        animation.leftIk.target = {{.16f,1.10f,.35f}}; animation.leftIk.pole = {{1,0,1}};
        animation.rightIk.target = {{-.16f,1.10f,.35f}}; animation.rightIk.pole = {{-1,0,1}};
        Worker::Job job;
        const auto begin = Clock::now();
        auto stage = begin;
        if (worker) Check(job.Start(*worker, request), "benchmark dispatch");
        else Check(Serial(actor, request, error), error.c_str());
        const auto dispatchNs = Ns(stage);
        stage = Clock::now();
        bool updated = false;
        Check(player.PreparePose(animation, tick + 1, PlayerCpuSkinCadence::Hz60, updated, error) && updated, error.c_str());
        Check(viewmodel.SkinPlayerPoseUniqueTextured(player.SolvedPose(), viewVertices, viewTangents, error), error.c_str());
        const auto playerNs = Ns(stage);
        stage = Clock::now();
        Worker::Result result;
        if (worker)
        {
            result = job.Wait();
            Check(result.success && result.identity == request.identity && result.generation == request.generation,
                "benchmark joined current identity");
        }
        const auto joinNs = Ns(stage), totalNs = Ns(begin);
        if (tick < warmup) continue;
        critical.push_back(totalNs); dispatch.push_back(dispatchNs); join.push_back(joinNs);
        actorWork.push_back(worker ? result.poseNanoseconds : dispatchNs); playerWork.push_back(playerNs);
        // Outside critical timer, identical verification cost in both modes.
        digest = Hash(digest, actor.Asset().vertices.data(), actor.Asset().vertices.size() * sizeof(StaticRtVertex));
        digest = Hash(digest, &actor.Asset().bounds, sizeof(actor.Asset().bounds));
        digest = Hash(digest, player.UniqueVertices().data(), player.UniqueVertices().size() * sizeof(TexturedSkinnedRtVertex));
        digest = Hash(digest, viewVertices.data(), viewVertices.size() * sizeof(TexturedSkinnedRtVertex));
    }
    const auto array = [](const char* name, const auto& values)
    { std::cout << ",\"" << name << "\":["; for (std::size_t i=0;i<values.size();++i) { if(i)std::cout<<',';std::cout<<values[i]; } std::cout<<']'; };
    std::cout << "{\"mode\":\"" << (threaded ? "worker" : "serial") << "\",\"scope\":\"actual Kit pose plus player world/viewmodel CPU preparation; dispatch/join included; no upload/AS/GPU/frame\",\"samples\":"
        << samples << ",\"warmup\":" << warmup << ",\"outputFnv1a64\":\"" << std::hex << digest << std::dec << '"';
    array("criticalNs", critical); array("dispatchNs", dispatch); array("joinNs", join);
    array("actorPoseNs", actorWork); array("playerNs", playerWork); std::cout << "}\n";
    return 0;
}
}
int main(int argc, char** argv)
{
    if (argc == 3 && (std::string_view(argv[1]) == "serial" || std::string_view(argv[1]) == "worker"))
    {
        const auto samples = std::stoul(argv[2]);
        if (samples < 30 || samples > 300) return 2;
        return Benchmark(std::string_view(argv[1]) == "worker", unsigned(samples));
    }
    return Contracts();
}
