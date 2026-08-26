#include "scene/assets/SkinnedMeshAsset.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

namespace
{

std::filesystem::path FindRepoRoot()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 7; ++depth)
    {
        if (std::filesystem::exists(candidate /
                "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb"))
            return candidate;
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

double VertexDistance(const horde::scene::TexturedSkinnedRtVertex& left,
                      const horde::scene::TexturedSkinnedRtVertex& right)
{
    return std::hypot(std::hypot(left.position[0] - right.position[0],
                                left.position[1] - right.position[1]),
                      left.position[2] - right.position[2]);
}

} // namespace

int main()
{
    using namespace horde::gameplay::animation;
    using namespace horde::scene;
    using namespace horde::vulkan::raytracing;
    const auto root = FindRepoRoot();
    if (root.empty()) return 2;
    const std::string path =
        (root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string();
    std::string diagnostic;
    SkinnedMeshAsset reference;
    PlayerRenderSlot at30;
    PlayerRenderSlot at60;
    if (!reference.LoadClips(path, PlayerLocomotionClipSet(), diagnostic) ||
        !at30.LoadAsset(path, diagnostic) || !at60.LoadAsset(path, diagnostic))
    {
        std::cerr << diagnostic << '\n';
        return 2;
    }

    constexpr std::uint64_t tickCount = 300u;
    double total30Milliseconds = 0.0;
    double total60Milliseconds = 0.0;
    std::uint64_t updates30 = 0u;
    std::uint64_t updates60 = 0u;
    double maximumMotionError = 0.0;
    float maximumSocketError30 = 0.0f;
    float maximumSocketError60 = 0.0f;
    for (std::uint64_t tick = 1u; tick <= tickCount; ++tick)
    {
        PlayerAnimationSnapshot animation;
        animation.locomotionClip = PlayerLocomotionClip::Walk;
        animation.locomotionBlend = 1.0f;
        animation.locomotionTime = static_cast<float>(tick) / 60.0f;
        SkinnedNodeTransform leftShoulder{};
        SkinnedNodeTransform rightShoulder{};
        if (!reference.NodeTransform(SkinnedClip::Walking, animation.locomotionTime,
                                     "LeftArm", leftShoulder, diagnostic) ||
            !reference.NodeTransform(SkinnedClip::Walking, animation.locomotionTime,
                                     "RightArm", rightShoulder, diagnostic))
        {
            std::cerr << diagnostic << '\n';
            return 2;
        }
        animation.leftIk.target = {{leftShoulder[12] - 0.09f,
                                    leftShoulder[13] + 0.04f,
                                    leftShoulder[14] + 0.66f}};
        animation.leftIk.pole = {{-0.8f, -0.1f, 0.45f}};
        animation.rightIk.target = {{rightShoulder[12] + 0.09f,
                                     rightShoulder[13] + 0.03f,
                                     rightShoulder[14] + 0.66f}};
        animation.rightIk.pole = {{0.8f, -0.1f, 0.45f}};

        bool updated30 = false;
        auto start = std::chrono::steady_clock::now();
        if (!at30.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz30,
                              updated30, diagnostic))
        {
            std::cerr << diagnostic << '\n';
            return 2;
        }
        total30Milliseconds += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        updates30 += updated30 ? 1u : 0u;

        bool updated60 = false;
        start = std::chrono::steady_clock::now();
        if (!at60.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz60,
                              updated60, diagnostic))
        {
            std::cerr << diagnostic << '\n';
            return 2;
        }
        total60Milliseconds += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        updates60 += updated60 ? 1u : 0u;
        maximumSocketError30 = std::max(
            maximumSocketError30,
            std::max(at30.LeftSocketErrorMetres(), at30.RightSocketErrorMetres()));
        maximumSocketError60 = std::max(
            maximumSocketError60,
            std::max(at60.LeftSocketErrorMetres(), at60.RightSocketErrorMetres()));
        const auto& vertices30 = at30.UniqueVertices();
        const auto& vertices60 = at60.UniqueVertices();
        if (vertices30.size() != vertices60.size()) return 2;
        for (std::size_t vertex = 0u; vertex < vertices30.size(); ++vertex)
            maximumMotionError = std::max(
                maximumMotionError, VertexDistance(vertices30[vertex], vertices60[vertex]));
    }

    const double perUpdate30 = total30Milliseconds / std::max<std::uint64_t>(updates30, 1u);
    const double perUpdate60 = total60Milliseconds / std::max<std::uint64_t>(updates60, 1u);
    const PlayerCpuCadenceMeasurements measurements{
        perUpdate30, perUpdate60, static_cast<float>(maximumMotionError), 0.0f};
    const PlayerCpuSkinCadence selected = ChoosePlayerCpuCadence(measurements);
    std::cout << std::fixed << std::setprecision(6)
              << "{\"ticks\":" << tickCount
              << ",\"uniqueVertices\":" << at60.UniqueVertices().size()
              << ",\"hz30\":{\"updates\":" << updates30
              << ",\"cpuMsPerUpdate\":" << perUpdate30
              << ",\"cpuMsPerTick\":" << total30Milliseconds / tickCount
              << ",\"maxSocketErrorM\":" << maximumSocketError30 << "}"
              << ",\"hz60\":{\"updates\":" << updates60
              << ",\"cpuMsPerUpdate\":" << perUpdate60
              << ",\"cpuMsPerTick\":" << total60Milliseconds / tickCount
              << ",\"maxSocketErrorM\":" << maximumSocketError60 << "}"
              << ",\"hz30MaxMotionErrorM\":" << maximumMotionError
              << ",\"selected\":\""
              << (selected == PlayerCpuSkinCadence::Hz30 ? "30" :
                  selected == PlayerCpuSkinCadence::Hz60 ? "60" : "review")
              << "\"}\n";
    return maximumSocketError30 <= kPlayerGripSocketToleranceMetres &&
           maximumSocketError60 <= kPlayerGripSocketToleranceMetres ? 0 : 1;
}
