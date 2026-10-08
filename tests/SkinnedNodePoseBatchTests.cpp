#include "scene/assets/SkinnedMeshAsset.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

#ifndef HORDE_RT_SOURCE_DIR
#error HORDE_RT_SOURCE_DIR must point to the source checkout.
#endif

namespace
{

using horde::scene::SkinnedClip;
using horde::scene::SkinnedMeshAsset;
using horde::scene::SkinnedNodeTransform;

constexpr std::array<std::string_view, 8u> kNodeNames{{
    "Hips", "Spine", "Spine01", "Spine02",
    "neck", "Head", "LeftArm", "RightArm"}};

struct FrozenSample
{
    SkinnedClip clip;
    float time;
    std::string_view node;
    float m00;
    std::array<float, 3u> translation;
};

// Captured from the original single-node NodeTransform at 45363ac3 using the
// imported skeleton GLB. Tolerance allows harmless cross-compiler float drift.
constexpr std::array<FrozenSample, 6u> kOriginalSamples{{
    {SkinnedClip::Idle, 0.0f, "Hips", 0.00516935019f,
     {{0.0309830271f, 0.768873632f, -0.0202468671f}}},
    {SkinnedClip::Idle, 0.37f, "Spine02", 0.00638648588f,
     {{0.0186883807f, 0.927078128f, -0.0103421686f}}},
    {SkinnedClip::Walking, 0.37f, "LeftFoot", 0.00931437872f,
     {{0.185916394f, 0.110842854f, 0.195849374f}}},
    {SkinnedClip::Attack, 0.37f, "RightHand", 0.00766634569f,
     {{-0.0261676311f, 1.38543987f, 0.226495177f}}},
    {SkinnedClip::Attack, 1.25f, "Head", 0.00936054718f,
     {{-0.0831602216f, 1.00222945f, 0.671505392f}}},
    {SkinnedClip::Dead, 3.5f, "Hips", 0.0095722992f,
     {{0.132439241f, 0.173247814f, 0.651546061f}}},
}};

bool Check(const bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool Near(const float a, const float b, const float tolerance = 0.00001f)
{
    return std::isfinite(a) && std::abs(a - b) <= tolerance;
}

bool SameTransform(const SkinnedNodeTransform& left,
                   const SkinnedNodeTransform& right,
                   const float tolerance = 0.000001f)
{
    for (std::size_t i = 0u; i < left.size(); ++i)
        if (!Near(left[i], right[i], tolerance)) return false;
    return true;
}

SkinnedNodeTransform SentinelTransform()
{
    SkinnedNodeTransform value{};
    value.fill(123.25f);
    return value;
}

bool IsSentinel(const SkinnedNodeTransform& value)
{
    for (const float component : value)
        if (component != 123.25f) return false;
    return true;
}

bool CheckFrozenSamples(const SkinnedMeshAsset& asset, std::string& diagnostic)
{
    for (const FrozenSample& sample : kOriginalSamples)
    {
        SkinnedNodeTransform value{};
        if (!asset.NodeTransform(sample.clip, sample.time, sample.node, value, diagnostic))
        {
            std::cerr << "FAIL: frozen original sample query: " << diagnostic << '\n';
            return false;
        }
        if (!Near(value[0], sample.m00) ||
            !Near(value[12], sample.translation[0]) ||
            !Near(value[13], sample.translation[1]) ||
            !Near(value[14], sample.translation[2]))
        {
            std::cerr << "FAIL: original transform sample changed for "
                      << sample.node << " at " << sample.time << '\n';
            return false;
        }
    }
    return true;
}

bool CheckBatchAgainstSingleNode(const SkinnedMeshAsset& asset,
                                 std::string& diagnostic)
{
    constexpr std::array<float, 5u> kTimeOffsets{{-0.25f, 0.0f, 0.37f, 0.0f, 0.15f}};
    for (std::size_t clipIndex = 0u; clipIndex < 4u; ++clipIndex)
    {
        const auto clip = static_cast<SkinnedClip>(clipIndex);
        const float duration = asset.ClipDuration(clip);
        const std::array<float, 5u> times{{
            kTimeOffsets[0], kTimeOffsets[1], kTimeOffsets[2], duration,
            duration + kTimeOffsets[4]}};
        for (const float time : times)
        {
            std::array<SkinnedNodeTransform, kNodeNames.size()> batch{};
            if (!asset.NodeTransforms(clip, time, kNodeNames, batch, diagnostic))
            {
                std::cerr << "FAIL: batch query: " << diagnostic << '\n';
                return false;
            }
            for (std::size_t nodeIndex = 0u; nodeIndex < kNodeNames.size(); ++nodeIndex)
            {
                SkinnedNodeTransform single{};
                if (!asset.NodeTransform(clip, time, kNodeNames[nodeIndex], single, diagnostic))
                {
                    std::cerr << "FAIL: single query: " << diagnostic << '\n';
                    return false;
                }
                if (!SameTransform(batch[nodeIndex], single))
                {
                    std::cerr << "FAIL: batch differs from single-node result.\n";
                    return false;
                }
            }
        }
    }
    return true;
}

bool CheckLoopAndClampBoundaries(const SkinnedMeshAsset& asset,
                                std::string& diagnostic)
{
    const std::array<std::string_view, 1u> names{{"Hips"}};
    for (const SkinnedClip clip : {SkinnedClip::Idle, SkinnedClip::Walking})
    {
        const float duration = asset.ClipDuration(clip);
        std::array<SkinnedNodeTransform, 1u> zero{};
        std::array<SkinnedNodeTransform, 1u> negative{};
        std::array<SkinnedNodeTransform, 1u> end{};
        std::array<SkinnedNodeTransform, 1u> wrapped{};
        std::array<SkinnedNodeTransform, 1u> offset{};
        if (!asset.NodeTransforms(clip, 0.0f, names, zero, diagnostic) ||
            !asset.NodeTransforms(clip, -0.25f, names, negative, diagnostic) ||
            !asset.NodeTransforms(clip, duration, names, end, diagnostic) ||
            !asset.NodeTransforms(clip, duration + 0.15f, names, wrapped, diagnostic) ||
            !asset.NodeTransforms(clip, 0.15f, names, offset, diagnostic))
            return false;
        if (!SameTransform(zero[0], negative[0]) ||
            !SameTransform(zero[0], end[0]) ||
            !SameTransform(offset[0], wrapped[0]))
            return Check(false, "looping clip boundary/clamp behavior changed");
    }

    for (const SkinnedClip clip : {SkinnedClip::Attack, SkinnedClip::Dead})
    {
        const float duration = asset.ClipDuration(clip);
        std::array<SkinnedNodeTransform, 1u> zero{};
        std::array<SkinnedNodeTransform, 1u> negative{};
        std::array<SkinnedNodeTransform, 1u> end{};
        std::array<SkinnedNodeTransform, 1u> beyond{};
        if (!asset.NodeTransforms(clip, 0.0f, names, zero, diagnostic) ||
            !asset.NodeTransforms(clip, -0.25f, names, negative, diagnostic) ||
            !asset.NodeTransforms(clip, duration, names, end, diagnostic) ||
            !asset.NodeTransforms(clip, duration + 0.15f, names, beyond, diagnostic))
            return false;
        if (!SameTransform(zero[0], negative[0]) ||
            !SameTransform(end[0], beyond[0]))
            return Check(false, "non-looping clip clamp behavior changed");
    }
    return true;
}

bool CheckRejectionsPreserveOutputs(const SkinnedMeshAsset& asset)
{
    std::string diagnostic;
    std::array<SkinnedNodeTransform, 2u> outputs{{SentinelTransform(), SentinelTransform()}};
    const auto unchanged = [&outputs]() {
        return IsSentinel(outputs[0]) && IsSentinel(outputs[1]);
    };
    const std::array<std::string_view, 1u> oneName{{"Hips"}};
    const std::array<std::string_view, 2u> twoNames{{"Hips", "Head"}};
    if (!Check(!asset.NodeTransforms(SkinnedClip::Idle, 0.0f, twoNames,
                                    std::span<SkinnedNodeTransform>(outputs).first(1u), diagnostic) &&
                   unchanged(),
               "size mismatch must reject without changing outputs")) return false;

    std::array<std::string_view, 33u> tooManyNames{};
    tooManyNames.fill("Hips");
    std::array<SkinnedNodeTransform, 33u> tooManyOutputs{};
    tooManyOutputs.fill(SentinelTransform());
    if (!Check(!asset.NodeTransforms(SkinnedClip::Idle, 0.0f, tooManyNames,
                                    tooManyOutputs, diagnostic) &&
                   IsSentinel(tooManyOutputs.front()) && IsSentinel(tooManyOutputs.back()),
               "capacity overflow must reject without changing outputs")) return false;

    const std::array<std::string_view, 2u> missingName{{"Hips", "NoSuchNode"}};
    if (!Check(!asset.NodeTransforms(SkinnedClip::Idle, 0.0f, missingName,
                                    outputs, diagnostic) &&
                   diagnostic == "Skinned asset is missing node: NoSuchNode" && unchanged(),
               "missing node must preserve the original diagnostic and outputs")) return false;

    if (!Check(!asset.NodeTransforms(SkinnedClip::Idle,
                                    std::numeric_limits<float>::quiet_NaN(),
                                    twoNames, outputs, diagnostic) && unchanged(),
               "NaN time must reject without changing outputs")) return false;
    if (!Check(!asset.NodeTransforms(SkinnedClip::Idle,
                                    std::numeric_limits<float>::infinity(),
                                    twoNames, outputs, diagnostic) && unchanged(),
               "infinite time must reject without changing outputs")) return false;

    if (!Check(!asset.NodeTransforms(static_cast<SkinnedClip>(99), 0.0f,
                                    oneName, std::span<SkinnedNodeTransform>(outputs).first(1u),
                                    diagnostic) &&
                   diagnostic == "Requested skinned clip is not mapped." && unchanged(),
               "invalid clip must preserve the original diagnostic and outputs")) return false;

    std::array<SkinnedNodeTransform, 1u> emptyAssetOutput{{SentinelTransform()}};
    SkinnedMeshAsset unloaded;
    if (!Check(!unloaded.NodeTransforms(SkinnedClip::Idle, 0.0f, oneName,
                                       emptyAssetOutput, diagnostic) &&
                   diagnostic == "Skinned model was not loaded." &&
                   IsSentinel(emptyAssetOutput[0]),
               "unloaded asset must preserve the original diagnostic and outputs")) return false;
    return true;
}

bool CheckDuplicatesAndLimit(const SkinnedMeshAsset& asset)
{
    std::string diagnostic;
    const std::array<std::string_view, 4u> duplicateNames{{
        "Head", "Hips", "Head", "LeftArm"}};
    std::array<SkinnedNodeTransform, 4u> duplicateOutputs{};
    if (!asset.NodeTransforms(SkinnedClip::Attack, 0.37f, duplicateNames,
                              duplicateOutputs, diagnostic))
        return Check(false, "duplicate-name batch should succeed");
    if (!SameTransform(duplicateOutputs[0], duplicateOutputs[2]))
        return Check(false, "duplicate names must preserve matching ordered results");

    std::array<std::string_view, SkinnedMeshAsset::kMaximumNodeTransformBatchSize> names{};
    names.fill("Hips");
    std::array<SkinnedNodeTransform, SkinnedMeshAsset::kMaximumNodeTransformBatchSize> outputs{};
    if (!asset.NodeTransforms(SkinnedClip::Idle, 0.0f, names, outputs, diagnostic))
        return Check(false, "maximum-size batch should succeed");
    for (const auto& output : outputs)
        if (!SameTransform(outputs.front(), output))
            return Check(false, "maximum-size duplicate batch ordering changed");
    return true;
}

} // namespace

int main()
{
    const std::filesystem::path assetPath =
        std::filesystem::path(HORDE_RT_SOURCE_DIR) /
        "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    SkinnedMeshAsset asset;
    std::string diagnostic;
    if (!asset.LoadCombatClips(assetPath.string(), diagnostic))
    {
        std::cerr << "FAIL: could not load imported skeleton: " << diagnostic << '\n';
        return 1;
    }

    if (!CheckFrozenSamples(asset, diagnostic) ||
        !CheckBatchAgainstSingleNode(asset, diagnostic) ||
        !CheckLoopAndClampBoundaries(asset, diagnostic) ||
        !CheckRejectionsPreserveOutputs(asset) ||
        !CheckDuplicatesAndLimit(asset))
        return 1;

    std::cout << "PASS: batched skeleton node-pose sampling and rejection contracts\n";
    return 0;
}