#include "scene/assets/SkinnedMeshAsset.h"
#include "scene/assets/SkinNormalTransport.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

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

bool CheckSkinNormalTransport()
{
    using horde::scene::detail::MakeSkinNormalTransport;
    using horde::scene::detail::ApplySkinNormalTransport;
    using Matrix = std::array<float, 16u>;
    using Vector = std::array<float, 3u>;
    // Independent copy of the pre-cache arithmetic. Byte comparisons prevent
    // pre-dividing cofactors or changing operation order from drifting shading.
    const auto legacy = [](const Matrix& m, const Vector& n, Vector& out) {
        const Vector c0{{m[0], m[1], m[2]}}, c1{{m[4], m[5], m[6]}}, c2{{m[8], m[9], m[10]}};
        const Vector f0{{c1[1]*c2[2]-c1[2]*c2[1], c1[2]*c2[0]-c1[0]*c2[2], c1[0]*c2[1]-c1[1]*c2[0]}};
        const Vector f1{{c2[1]*c0[2]-c2[2]*c0[1], c2[2]*c0[0]-c2[0]*c0[2], c2[0]*c0[1]-c2[1]*c0[0]}};
        const Vector f2{{c0[1]*c1[2]-c0[2]*c1[1], c0[2]*c1[0]-c0[0]*c1[2], c0[0]*c1[1]-c0[1]*c1[0]}};
        const float det = c0[0]*f0[0] + c0[1]*f0[1] + c0[2]*f0[2];
        if (!std::isfinite(det) || std::abs(det) <= 1.0e-20f) return false;
        const float inv = 1.0f / det;
        out = {{(f0[0]*n[0]+f1[0]*n[1]+f2[0]*n[2])*inv,
                (f0[1]*n[0]+f1[1]*n[1]+f2[1]*n[2])*inv,
                (f0[2]*n[0]+f1[2]*n[1]+f2[2]*n[2])*inv}};
        return std::isfinite(out[0]) && std::isfinite(out[1]) && std::isfinite(out[2]);
    };
    const Matrix identity{{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
    std::vector<Matrix> matrices{identity,
        {{2,0,0,0, 0,.5f,0,0, 0,0,4,0, 12,-8,5,1}},
        {{1,.3f,.1f,0, .2f,2,-.2f,0, -.1f,.5f,.7f,0, 0,0,0,1}},
        {{-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}}};
    for (const float scale : {0.0f, 1.0e-20f, -1.0e-20f,
         std::nextafter(1.0e-20f, 1.0f), std::nextafter(-1.0e-20f, -1.0f),
         1.0e-19f, std::numeric_limits<float>::infinity(),
         std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()})
    {
        auto m = identity;
        m[0] = scale;
        if (scale == std::numeric_limits<float>::max()) m[5] = scale;
        matrices.push_back(m);
    }
    // Nonuniform rotations/shears, including reflected transforms.
    for (int i = 1; i <= 96; ++i)
    {
        auto m = identity;
        m[0] = std::sin(i*.37f)*2.0f;
        m[1] = std::cos(i*.21f)*.7f;
        m[4] = std::sin(i*.17f)*.3f;
        m[5] = .5f + i*.013f;
        m[6] = std::cos(i*.11f)*.9f;
        m[8] = std::sin(i*.29f)*.4f;
        m[10] = (i%2 ? -1.0f : 1.0f) * (.8f+i*.01f);
        matrices.push_back(m);
    }
    const std::array<Vector, 7u> normals{{{{1,0,0}}, {{0,1,0}}, {{0,0,1}},
        {{-.21f,.7f,.63f}}, {{0,0,0}}, {{std::numeric_limits<float>::max(),1,0}},
        {{std::numeric_limits<float>::infinity(),0,0}}}};
    for (const auto& m : matrices)
    {
        const auto cache = MakeSkinNormalTransport(m);
        for (const auto& n : normals)
        {
            Vector expected{{42,43,44}}, actual = expected;
            const bool ok = legacy(m,n,expected);
            const bool cachedOk = ApplySkinNormalTransport(cache,n[0],n[1],n[2],actual[0],actual[1],actual[2]);
            if (!Check(ok == cachedOk, "cached normal failure boundary differs from legacy")) return false;
            // NaN payloads may differ between compilers; successful and early
            // rejected results must be bit exact, including untouched sentinels.
            if ((ok || !cache.invertible) &&
                !Check(std::memcmp(expected.data(),actual.data(),sizeof(Vector)) == 0,
                       "cached normal arithmetic changed exact float output")) return false;
        }
    }
    const auto singular = MakeSkinNormalTransport(Matrix{});
    auto reused = MakeSkinNormalTransport(identity);
    reused = singular;
    Vector output{{42,43,44}};
    if (!Check(!ApplySkinNormalTransport(reused,1,0,0,output[0],output[1],output[2]) &&
                   output == Vector{{42,43,44}},
               "invalid cache must not reuse a previous valid normal transform")) return false;
    std::cout << "PASS: " << matrices.size()*normals.size()
              << " cached/legacy normal cases including determinant boundaries\n";
    return true;
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

// A finite, CPU-only before/after receipt for the actual shared world/viewmodel
// pose. This does not replace native image inspection or sustained GPU timing.
int WritePlayerSkinningReference(const std::filesystem::path& destination)
{
    using namespace horde::scene;
    if (std::filesystem::exists(destination) ||
        !std::filesystem::create_directories(destination))
        return Check(false, "reference output must be a fresh directory") ? 0 : 1;
    const auto root = std::filesystem::path(HORDE_RT_SOURCE_DIR);
    SkinnedMeshAsset world, viewmodel;
    std::string diagnostic;
    if (!world.LoadClips((root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),
                         PlayerLocomotionClipSet(), diagnostic) ||
        !viewmodel.LoadClips((root / "assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),
                            PlayerLocomotionClipSet(), diagnostic))
    {
        std::cerr << diagnostic << '\n';
        return 1;
    }
    std::ofstream timings(destination / "timings.csv");
    timings << "sample,evaluateUs,worldSkinUs,viewmodelSkinUs,worldVertices,viewmodelVertices\n";
    std::size_t sample = 0u;
    SkinnedPlayerPose pose;
    std::vector<TexturedSkinnedRtVertex> vertices[2];
    std::vector<SkinnedPbrTangent> tangents[2];
    // Reuse one immutable pose for both consumers. Include pelvis and IK changes
    // and loop boundaries; every written byte comes from the production skinning.
    for (int repeat = 0; repeat < 12; ++repeat)
    for (const auto clip : {SkinnedClip::Idle, SkinnedClip::Walking})
    for (const float phase : {0.0f, 0.25f, 0.75f, 1.0f})
    {
        const float time = phase * world.ClipDuration(clip);
        SkinnedNodeTransform leftHand{}, rightHand{};
        if (!world.NodeTransform(clip, time, "LeftHand", leftHand, diagnostic) ||
            !world.NodeTransform(clip, time, "RightHand", rightHand, diagnostic)) return 1;
        const SkinnedArmIkTarget left{{{leftHand[12], leftHand[13] - 0.05f,
                                        leftHand[14] + phase * 0.1f}}, {{1.0f, -1.0f, 0.0f}}};
        const SkinnedArmIkTarget right{{{rightHand[12], rightHand[13] - 0.05f,
                                         rightHand[14] + phase * 0.1f}}, {{-1.0f, -1.0f, 0.0f}}};
        const auto begin = std::chrono::steady_clock::now();
        if (!world.EvaluatePlayerPose(clip, time, left, right, pose, diagnostic,
                                     clip == SkinnedClip::Idle ? phase * 0.12f : 0.0f))
        {
            std::cerr << diagnostic << '\n';
            return 1;
        }
        const auto evaluated = std::chrono::steady_clock::now();
        if (!world.SkinPlayerPoseUniqueTextured(pose, vertices[0], tangents[0], diagnostic))
        {
            std::cerr << diagnostic << '\n';
            return 1;
        }
        const auto worldDone = std::chrono::steady_clock::now();
        if (!viewmodel.SkinPlayerPoseUniqueTextured(pose, vertices[1], tangents[1], diagnostic))
        {
            std::cerr << diagnostic << '\n';
            return 1;
        }
        const auto viewmodelDone = std::chrono::steady_clock::now();
        const auto micros = [](const auto a, const auto b) {
            return std::chrono::duration<double, std::micro>(b - a).count();
        };
        timings << sample << ',' << micros(begin, evaluated) << ',' << micros(evaluated, worldDone)
                << ',' << micros(worldDone, viewmodelDone) << ',' << vertices[0].size()
                << ',' << vertices[1].size() << '\n';
        if (repeat == 0)
        for (int mesh = 0; mesh < 2; ++mesh)
        {
            std::ofstream output(destination / (std::to_string(sample) + (mesh ? "-viewmodel.bin" : "-world.bin")),
                                 std::ios::binary);
            output.write(reinterpret_cast<const char*>(vertices[mesh].data()),
                         static_cast<std::streamsize>(vertices[mesh].size() * sizeof(TexturedSkinnedRtVertex)));
            output.write(reinterpret_cast<const char*>(tangents[mesh].data()),
                         static_cast<std::streamsize>(tangents[mesh].size() * sizeof(SkinnedPbrTangent)));
            if (!output) return 1;
        }
        ++sample;
    }
    if (!timings) return 1;
    std::cout << "PASS: " << sample << " evaluated shared poses and 16 exact vertex/tangent streams\n";
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 3 && std::string_view(argv[1]) == "--write-player-skinning-reference")
        return WritePlayerSkinningReference(argv[2]);
    if (argc != 1) return 2;
    if (!CheckSkinNormalTransport()) return 1;
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
