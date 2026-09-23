#include "scene/assets/SkinnedMeshAsset.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace
{

using Vec3 = std::array<float, 3u>;
using Mat4 = std::array<float, 16u>;

constexpr float kUpperLength = 0.30f;
constexpr float kLowerLength = 0.40f;
constexpr float kFoldDistance = 0.20f;
constexpr float kRingRadius = 0.025f;
constexpr float kGripOffsetX = 0.02f;
constexpr float kTolerance = 0.00002f;

bool Require(const bool condition, const std::string& message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

float Distance(const Vec3& left, const Vec3& right)
{
    return std::sqrt((left[0] - right[0]) * (left[0] - right[0]) +
                     (left[1] - right[1]) * (left[1] - right[1]) +
                     (left[2] - right[2]) * (left[2] - right[2]));
}

Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

Vec3 Subtract(const Vec3& left, const Vec3& right)
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

float Dot(const Vec3& left, const Vec3& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

float Length(const Vec3& value)
{
    return std::sqrt(Dot(value, value));
}

Vec3 Normalize(const Vec3& value)
{
    return Scale(value, 1.0f / std::max(Length(value), 0.000001f));
}

Vec3 RotateXY(const Vec3& point, const Vec3& pivot,
              const Vec3& sourceDirection, const Vec3& targetDirection)
{
    // Closed-form planar rotation from the authored bone axis to the expected
    // endpoint vector. This is deliberately independent of the production IK.
    const float cosine = Dot(sourceDirection, targetDirection) /
        (Length(sourceDirection) * Length(targetDirection));
    const float sine = (sourceDirection[0] * targetDirection[1] -
                        sourceDirection[1] * targetDirection[0]) /
        (Length(sourceDirection) * Length(targetDirection));
    const Vec3 relative = Subtract(point, pivot);
    return {{pivot[0] + cosine * relative[0] - sine * relative[1],
             pivot[1] + sine * relative[0] + cosine * relative[1],
             pivot[2] + relative[2]}};
}

struct FixtureVertex
{
    Vec3 position{};
    Vec3 normal{};
    std::array<float, 4u> tangent{};
    std::array<float, 2u> uv{};
    std::array<std::uint8_t, 4u> joints{};
    std::array<float, 4u> weights{};
};

struct BufferView
{
    std::size_t offset = 0u;
    std::size_t length = 0u;
};

class GlbBinary
{
public:
    template <typename T>
    std::size_t AddView(const std::vector<T>& values)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        while ((bytes_.size() & 3u) != 0u) bytes_.push_back(0u);
        BufferView view{bytes_.size(), values.size() * sizeof(T)};
        const auto* begin = reinterpret_cast<const std::uint8_t*>(values.data());
        bytes_.insert(bytes_.end(), begin, begin + view.length);
        views_.push_back(view);
        return views_.size() - 1u;
    }

    const std::vector<std::uint8_t>& Bytes() const { return bytes_; }
    const std::vector<BufferView>& Views() const { return views_; }

private:
    std::vector<std::uint8_t> bytes_;
    std::vector<BufferView> views_;
};

struct TestArm
{
    float side = 1.0f;
    std::uint8_t upperJoint = 0u;
    std::uint8_t forearmJoint = 0u;
    std::uint8_t handJoint = 0u;
    std::uint8_t gripJoint = 0u;
    std::array<std::array<std::uint32_t, 4u>, 5u> rings{};
    Vec3 shoulder{};
    Vec3 elbow{};
    Vec3 hand{};
};

void AddRing(std::vector<FixtureVertex>& vertices,
             std::array<std::uint32_t, 4u>& ring,
             const Vec3& center,
             const std::uint8_t firstJoint,
             const std::uint8_t secondJoint,
             const float firstWeight)
{
    const std::array<Vec3, 4u> offsets{{
        {{kRingRadius, 0.0f, 0.0f}}, {{0.0f, 0.0f, kRingRadius}},
        {{-kRingRadius, 0.0f, 0.0f}}, {{0.0f, 0.0f, -kRingRadius}}}};
    for (std::size_t corner = 0u; corner < offsets.size(); ++corner)
    {
        FixtureVertex vertex;
        vertex.position = Add(center, offsets[corner]);
        vertex.normal = Normalize(offsets[corner]);
        vertex.tangent = {{0.0f, 1.0f, 0.0f, 1.0f}};
        vertex.uv = {{static_cast<float>(corner) / 3.0f,
                      static_cast<float>(vertices.size() % 5u) / 4.0f}};
        vertex.joints = {{firstJoint, secondJoint, 0u, 0u}};
        vertex.weights = {{firstWeight, 1.0f - firstWeight, 0.0f, 0.0f}};
        if (firstJoint == secondJoint)
            vertex.weights = {{1.0f, 0.0f, 0.0f, 0.0f}};
        ring[corner] = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(vertex);
    }
}

void ConnectRings(const std::array<std::uint32_t, 4u>& first,
                  const std::array<std::uint32_t, 4u>& second,
                  std::vector<std::uint16_t>& indices)
{
    for (std::size_t corner = 0u; corner < 4u; ++corner)
    {
        const std::size_t next = (corner + 1u) % 4u;
        const auto a = static_cast<std::uint16_t>(first[corner]);
        const auto b = static_cast<std::uint16_t>(first[next]);
        const auto c = static_cast<std::uint16_t>(second[corner]);
        const auto d = static_cast<std::uint16_t>(second[next]);
        indices.insert(indices.end(), {a, b, c, b, d, c});
    }
}

Mat4 InverseTranslation(const Vec3& translation)
{
    Mat4 result{{1.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 1.0f, 0.0f,
                 -translation[0], -translation[1], -translation[2], 1.0f}};
    return result;
}

struct FixtureData
{
    std::vector<FixtureVertex> vertices;
    std::vector<std::uint16_t> indices;
    std::array<TestArm, 2u> arms{};
    std::array<Vec3, 8u> jointTranslations{};
};

FixtureData MakeFixtureData()
{
    FixtureData fixture;
    fixture.arms[0] = {1.0f, 0u, 1u, 2u, 3u};
    fixture.arms[1] = {-1.0f, 4u, 5u, 6u, 7u};
    for (auto& arm : fixture.arms)
    {
        arm.shoulder = {{0.40f * arm.side, 1.30f, 0.0f}};
        arm.elbow = {{0.40f * arm.side, 1.00f, 0.0f}};
        arm.hand = {{0.40f * arm.side, 0.60f, 0.0f}};
        const std::array<Vec3, 5u> centers{{
            arm.shoulder,
            {{0.40f * arm.side, 1.15f, 0.0f}},
            arm.elbow,
            {{0.40f * arm.side, 0.80f, 0.0f}},
            arm.hand}};
        for (std::size_t ring = 0u; ring < centers.size(); ++ring)
        {
            const std::uint8_t first = ring <= 2u ? arm.upperJoint
                : ring == 4u ? arm.handJoint : arm.forearmJoint;
            const std::uint8_t second = ring == 2u ? arm.forearmJoint : first;
            const float weight = ring == 2u ? 0.5f : 1.0f;
            AddRing(fixture.vertices, arm.rings[ring], centers[ring],
                    first, second, weight);
            if (ring > 0u)
                ConnectRings(arm.rings[ring - 1u], arm.rings[ring],
                             fixture.indices);
        }
    }
    fixture.jointTranslations = {{
        fixture.arms[0].shoulder,
        fixture.arms[0].elbow,
        fixture.arms[0].hand,
        {{0.40f * fixture.arms[0].side + kGripOffsetX, 0.60f, 0.0f}},
        fixture.arms[1].shoulder,
        fixture.arms[1].elbow,
        fixture.arms[1].hand,
        {{0.40f * fixture.arms[1].side + kGripOffsetX, 0.60f, 0.0f}}}};
    return fixture;
}

void Flatten(const std::vector<FixtureVertex>& vertices,
             std::vector<float>& positions,
             std::vector<float>& normals,
             std::vector<float>& tangents,
             std::vector<float>& uvs,
             std::vector<std::uint8_t>& joints,
             std::vector<float>& weights)
{
    for (const auto& vertex : vertices)
    {
        positions.insert(positions.end(), vertex.position.begin(), vertex.position.end());
        normals.insert(normals.end(), vertex.normal.begin(), vertex.normal.end());
        tangents.insert(tangents.end(), vertex.tangent.begin(), vertex.tangent.end());
        uvs.insert(uvs.end(), vertex.uv.begin(), vertex.uv.end());
        joints.insert(joints.end(), vertex.joints.begin(), vertex.joints.end());
        weights.insert(weights.end(), vertex.weights.begin(), vertex.weights.end());
    }
}

struct AccessorSet
{
    std::size_t positions = 0u;
    std::size_t normals = 0u;
    std::size_t tangents = 0u;
    std::size_t uvs = 0u;
    std::size_t joints = 0u;
    std::size_t weights = 0u;
    std::size_t indices = 0u;
    std::size_t inverseBind = 0u;
    std::size_t times = 0u;
    std::size_t translations = 0u;
};

AccessorSet AddFixtureViews(const FixtureData& fixture, GlbBinary& binary)
{
    std::vector<float> positions, normals, tangents, uvs, weights;
    std::vector<std::uint8_t> joints;
    Flatten(fixture.vertices, positions, normals, tangents, uvs, joints, weights);

    std::vector<Mat4> inverseBind;
    inverseBind.reserve(fixture.jointTranslations.size());
    for (const auto& translation : fixture.jointTranslations)
        inverseBind.push_back(InverseTranslation(translation));

    const std::vector<float> times{0.0f, 1.0f};
    const std::vector<float> noOpTranslations{0.0f, 0.0f, 0.0f,
                                               0.0f, 0.0f, 0.0f};
    return {binary.AddView(positions), binary.AddView(normals),
            binary.AddView(tangents), binary.AddView(uvs), binary.AddView(joints),
            binary.AddView(weights), binary.AddView(fixture.indices),
            binary.AddView(inverseBind), binary.AddView(times),
            binary.AddView(noOpTranslations)};
}

void WriteJsonAccessors(std::ostringstream& json, const AccessorSet& accessors,
                        const std::size_t vertexCount,
                        const std::size_t indexCount)
{
    const auto accessor = [&json](const std::size_t view,
                                          const std::uint32_t component,
                                          const std::size_t count,
                                          const char* type,
                                          const bool normalized = false) {
        json << "{\"bufferView\":" << view
             << ",\"componentType\":" << component
             << ",\"count\":" << count
             << ",\"type\":\"" << type << "\"";
        if (normalized) json << ",\"normalized\":true";
        if (view == 0u) json << ",\"min\":[-0.425,0.6,-0.025],\"max\":[0.425,1.3,0.025]";
        if (view == 8u) json << ",\"min\":[0],\"max\":[1]";
        json << '}';
    };
    json << "[";
    accessor(accessors.positions, 5126u, vertexCount, "VEC3"); json << ',';
    accessor(accessors.normals, 5126u, vertexCount, "VEC3"); json << ',';
    accessor(accessors.tangents, 5126u, vertexCount, "VEC4"); json << ',';
    accessor(accessors.uvs, 5126u, vertexCount, "VEC2"); json << ',';
    accessor(accessors.joints, 5121u, vertexCount, "VEC4"); json << ',';
    accessor(accessors.weights, 5126u, vertexCount, "VEC4"); json << ',';
    accessor(accessors.indices, 5123u, indexCount, "SCALAR"); json << ',';
    accessor(accessors.inverseBind, 5126u, 8u, "MAT4"); json << ',';
    accessor(accessors.times, 5126u, 2u, "SCALAR"); json << ',';
    accessor(accessors.translations, 5126u, 2u, "VEC3");
    json << ']';
}

std::string MakeFixtureJson(const FixtureData& fixture,
                            const GlbBinary& binary,
                            const AccessorSet& accessors)
{
    std::ostringstream json;
    json << "{\"asset\":{\"version\":\"2.0\"},"
         << "\"scene\":0,\"scenes\":[{\"nodes\":[0]}],"
         << "\"nodes\":["
         << "{\"name\":\"ArmFixtureRoot\",\"children\":[1,5,9]},"
         << "{\"name\":\"LeftArm\",\"translation\":[0.4,1.3,0],\"children\":[2]},"
         << "{\"name\":\"LeftForeArm\",\"translation\":[0,-0.3,0],\"children\":[3]},"
         << "{\"name\":\"LeftHand\",\"translation\":[0,-0.4,0],\"children\":[4]},"
         << "{\"name\":\"LeftGrip\",\"translation\":[0.02,0,0]},"
         << "{\"name\":\"RightArm\",\"translation\":[-0.4,1.3,0],\"children\":[6]},"
         << "{\"name\":\"RightForeArm\",\"translation\":[0,-0.3,0],\"children\":[7]},"
         << "{\"name\":\"RightHand\",\"translation\":[0,-0.4,0],\"children\":[8]},"
         << "{\"name\":\"RightGrip\",\"translation\":[0.02,0,0]},"
         << "{\"name\":\"ArmFixtureMesh\",\"mesh\":0,\"skin\":0}],"
         << "\"meshes\":[{\"primitives\":[{\"attributes\":{"
         << "\"POSITION\":0,\"NORMAL\":1,\"TANGENT\":2,\"TEXCOORD_0\":3,"
         << "\"JOINTS_0\":4,\"WEIGHTS_0\":5},\"indices\":6,\"material\":0}]}],"
         << "\"materials\":[{\"name\":\"FixtureLeather\"}],"
         << "\"skins\":[{\"joints\":[1,2,3,4,5,6,7,8],\"inverseBindMatrices\":7}],"
         << "\"animations\":["
         << "{\"name\":\"Idle\",\"samplers\":[{\"input\":8,\"output\":9}],"
         << "\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]},"
         << "{\"name\":\"Walking\",\"samplers\":[{\"input\":8,\"output\":9}],"
         << "\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]}],"
         << "\"buffers\":[{\"byteLength\":" << binary.Bytes().size() << "}],"
         << "\"bufferViews\":[";
    for (std::size_t index = 0u; index < binary.Views().size(); ++index)
    {
        if (index != 0u) json << ',';
        const auto& view = binary.Views()[index];
        json << "{\"buffer\":0,\"byteOffset\":" << view.offset
             << ",\"byteLength\":" << view.length << '}';
    }
    json << "],\"accessors\":";
    WriteJsonAccessors(json, accessors, fixture.vertices.size(),
                       fixture.indices.size());
    json << '}';
    return json.str();
}

bool WriteGlb(const std::filesystem::path& path,
              const std::string& sourceJson,
              const std::vector<std::uint8_t>& binary)
{
    std::string json = sourceJson;
    while ((json.size() & 3u) != 0u) json.push_back(' ');
    const std::uint32_t jsonLength = static_cast<std::uint32_t>(json.size());
    const std::uint32_t binaryLength = static_cast<std::uint32_t>(binary.size());
    const std::uint32_t totalLength = 12u + 8u + jsonLength + 8u + binaryLength;
    std::ofstream output(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!output) return false;
    const auto writeU32 = [&output](const std::uint32_t value) {
        output.write(reinterpret_cast<const char*>(&value), sizeof(value));
    };
    writeU32(0x46546c67u);
    writeU32(2u);
    writeU32(totalLength);
    writeU32(jsonLength);
    writeU32(0x4e4f534au);
    output.write(json.data(), static_cast<std::streamsize>(json.size()));
    writeU32(binaryLength);
    writeU32(0x004e4942u);
    output.write(reinterpret_cast<const char*>(binary.data()),
                 static_cast<std::streamsize>(binary.size()));
    return output.good();
}

class TemporaryGlb
{
public:
    TemporaryGlb()
    {
        static std::atomic<std::uint64_t> sequence{0u};
        std::random_device random;
        const auto stamp = static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        for (int attempt = 0; attempt < 8; ++attempt)
        {
            const auto nonce = (static_cast<std::uint64_t>(random()) << 32u) ^ random();
            path_ = std::filesystem::temp_directory_path() /
                ("horde-skinned-arm-reference-" + std::to_string(stamp) + "-" +
                 std::to_string(sequence.fetch_add(1u)) + "-" +
                 std::to_string(nonce) + ".glb");
            if (!std::filesystem::exists(path_)) return;
        }
        path_.clear();
    }

    ~TemporaryGlb()
    {
        if (!path_.empty())
        {
            std::error_code ignored;
            std::filesystem::remove(path_, ignored);
        }
    }

    const std::filesystem::path& Path() const { return path_; }

private:
    std::filesystem::path path_;
};

bool NearVec(const Vec3& actual, const Vec3& expected,
             const float tolerance = kTolerance)
{
    return Distance(actual, expected) <= tolerance;
}

Vec3 OutputPosition(const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices,
                    const std::uint32_t index)
{
    const auto& v = vertices.at(index).position;
    return {{v[0], v[1], v[2]}};
}

Vec3 OutputNormal(const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices,
                  const std::uint32_t index)
{
    const auto& v = vertices.at(index).normal;
    return {{v[0], v[1], v[2]}};
}

Vec3 OutputTangent(const std::vector<horde::scene::SkinnedPbrTangent>& tangents,
                   const std::uint32_t index)
{
    const auto& v = tangents.at(index).tangent;
    return {{v[0], v[1], v[2]}};
}

Vec3 BlendLbs(const Vec3& position, const Vec3& pivotUpper,
              const Vec3& sourceUpper, const Vec3& targetUpper,
              const Vec3& pivotLower, const Vec3& sourceLower,
              const Vec3& targetLower, const Vec3& expectedLowerPivot)
{
    const Vec3 lowerPoint = Add(
        RotateXY(position, pivotLower, sourceLower, targetLower),
        Subtract(expectedLowerPivot, pivotLower));
    return Scale(Add(RotateXY(position, pivotUpper, sourceUpper, targetUpper),
                     lowerPoint), 0.5f);
}

bool CheckPose(horde::scene::SkinnedMeshAsset& asset,
               const FixtureData& fixture,
               const float targetDistance,
               const bool expectFolded)
{
    using namespace horde::scene;
    const Vec3 sourceUpper{{0.0f, -kUpperLength, 0.0f}};
    const Vec3 sourceLower{{0.0f, -kLowerLength, 0.0f}};
    const float adjacent = (kUpperLength * kUpperLength + targetDistance * targetDistance -
                           kLowerLength * kLowerLength) / (2.0f * targetDistance);
    if (!Require(!expectFolded || std::abs(adjacent + 0.075f) < 0.000001f,
                 "reference fold must have the analytically expected negative elbow projection"))
        return false;
    const float bendHeight = std::sqrt(kUpperLength * kUpperLength - adjacent * adjacent);
    std::vector<TexturedSkinnedRtVertex> posed;
    std::vector<SkinnedPbrTangent> tangents;
    for (std::size_t sideIndex = 0u; sideIndex < fixture.arms.size(); ++sideIndex)
    {
        const TestArm& arm = fixture.arms[sideIndex];
        const Vec3 target{{arm.shoulder[0] - arm.side * targetDistance,
                           arm.shoulder[1], arm.shoulder[2]}};
        SkinnedArmIkTarget ik;
        ik.target = target;
        ik.pole = {{0.0f, -1.0f, 0.0f}};
        ik.handOrientation = {{1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f}};
        ik.handOrientationTargetEnabled = true;
        SkinnedArmIkTarget left = sideIndex == 0u ? ik : SkinnedArmIkTarget{};
        SkinnedArmIkTarget right = sideIndex == 1u ? ik : SkinnedArmIkTarget{};
        if (sideIndex == 0u)
        {
            right.target = fixture.arms[1].hand;
            right.pole = {{0.0f, -1.0f, 0.0f}};
            right.handOrientation = ik.handOrientation;
            right.handOrientationTargetEnabled = true;
        }
        else
        {
            left.target = fixture.arms[0].hand;
            left.pole = {{0.0f, -1.0f, 0.0f}};
            left.handOrientation = ik.handOrientation;
            left.handOrientationTargetEnabled = true;
        }

        SkinnedPlayerPose pose;
        std::string diagnostic;
        if (!Require(asset.EvaluatePlayerPose(SkinnedClip::Idle, 0.0f,
                                              left, right, pose, diagnostic),
                     diagnostic)) return false;
        if (!Require(asset.SkinPlayerPoseUniqueTextured(pose, posed, tangents,
                                                        diagnostic),
                     diagnostic)) return false;

        const Vec3 expectedElbow{{arm.shoulder[0] - arm.side * adjacent,
                                  arm.shoulder[1] - bendHeight,
                                  arm.shoulder[2]}};
        const Vec3 solvedUpper = Subtract(expectedElbow, arm.shoulder);
        const Vec3 solvedLower = Subtract(target, expectedElbow);
        const Vec3 actualElbow = Scale(Add(OutputPosition(posed, arm.rings[2][0]),
                                           OutputPosition(posed, arm.rings[2][2])), 0.5f);
        std::cout << (sideIndex == 0u ? "Left" : "Right") << " target=" << targetDistance
                  << " elbow=(" << actualElbow[0] << ',' << actualElbow[1] << ',' << actualElbow[2]
                  << ") upper=" << Distance(arm.shoulder, actualElbow)
                  << " lower=" << Distance(actualElbow, target) << '\n';
        if (!Require(NearVec(actualElbow, expectedElbow),
                     "actual skinned elbow must preserve signed analytic projection")) return false;
        const auto& sockets = pose.Sockets();
        const auto& handSocket = sideIndex == 0u ? sockets.leftHand : sockets.rightHand;
        const auto& gripSocket = sideIndex == 0u ? sockets.leftGrip : sockets.rightGrip;
        const Vec3 expectedGrip{{target[0] + kGripOffsetX, target[1], target[2]}};
        if (!Require(NearVec(Vec3{{handSocket[12], handSocket[13], handSocket[14]}}, target),
                     "actual player solver hand socket must land on the requested target"))
            return false;
        if (!Require(NearVec(Vec3{{gripSocket[12], gripSocket[13], gripSocket[14]}}, expectedGrip),
                     "hand orientation must carry the authored nonzero Grip offset"))
            return false;

        const auto& upperMid = arm.rings[1][0];
        const auto& lowerMid = arm.rings[3][0];
        const Vec3 expectedUpperMid = RotateXY(
            fixture.vertices[upperMid].position, arm.shoulder,
            sourceUpper, solvedUpper);
        const Vec3 expectedLowerMid = RotateXY(
            fixture.vertices[lowerMid].position, arm.elbow,
            sourceLower, solvedLower);
        const Vec3 expectedLowerMidWithElbowTranslation = Add(
            expectedLowerMid, Subtract(expectedElbow, arm.elbow));
        if (!Require(NearVec(OutputPosition(posed, upperMid), expectedUpperMid),
                     "upper-arm LBS sentinel must preserve its analytic bone transform"))
            return false;
        if (!Require(NearVec(OutputPosition(posed, lowerMid),
                             expectedLowerMidWithElbowTranslation),
                     "forearm LBS sentinel must preserve its analytic bone transform"))
            return false;

        const std::uint32_t seamIndex = arm.rings[2][0];
        const Vec3 bindSeam = fixture.vertices[seamIndex].position;
        const Vec3 expectedSeam = BlendLbs(
            bindSeam, arm.shoulder, sourceUpper, solvedUpper,
            arm.elbow, sourceLower, solvedLower, expectedElbow);
        if (!Require(NearVec(OutputPosition(posed, seamIndex), expectedSeam),
                     "50/50 elbow seam must equal the independent linear-blend result"))
            return false;

        const Vec3 sourceNormal{{1.0f, 0.0f, 0.0f}};
        const Vec3 sourceTangent{{0.0f, 1.0f, 0.0f}};
        const Vec3 upperNormal = RotateXY(sourceNormal, {{0.0f, 0.0f, 0.0f}},
                                          sourceUpper, solvedUpper);
        const Vec3 lowerNormal = RotateXY(sourceNormal, {{0.0f, 0.0f, 0.0f}},
                                          sourceLower, solvedLower);
        const Vec3 expectedNormal = Normalize(Scale(Add(upperNormal, lowerNormal), 0.5f));
        const Vec3 upperTangent = RotateXY(sourceTangent, {{0.0f, 0.0f, 0.0f}},
                                           sourceUpper, solvedUpper);
        const Vec3 lowerTangent = RotateXY(sourceTangent, {{0.0f, 0.0f, 0.0f}},
                                           sourceLower, solvedLower);
        Vec3 tangentBlend = Scale(Add(upperTangent, lowerTangent), 0.5f);
        tangentBlend = Subtract(tangentBlend,
                                Scale(expectedNormal, Dot(expectedNormal, tangentBlend)));
        const Vec3 expectedTangent = Normalize(tangentBlend);
        if (!Require(NearVec(OutputNormal(posed, seamIndex), expectedNormal),
                     "seam normal must match inverse-transpose LBS blend"))
            return false;
        if (!Require(NearVec(OutputTangent(tangents, seamIndex), expectedTangent),
                     "seam tangent must be re-orthogonalized after LBS"))
            return false;

    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    using namespace horde::scene;
    // Read-only applicability evidence for an explicitly supplied rig; this
    // does not impose these segment ratios on future admitted assets.
    if (argc == 3 && std::string(argv[1]) == "--inspect-chain-lengths")
    {
        SkinnedMeshAsset asset;
        std::string diagnostic;
        if (!Require(asset.LoadClips(argv[2], PlayerLocomotionClipSet(), diagnostic), diagnostic)) return 1;
        for (const auto& names : {std::array<const char*, 3>{"LeftArm", "LeftForeArm", "LeftHand"},
                                  std::array<const char*, 3>{"RightArm", "RightForeArm", "RightHand"}})
        {
            float minUpper = std::numeric_limits<float>::max(), maxUpper = 0.0f;
            float minLower = minUpper, maxLower = 0.0f, minSquaredDifference = minUpper;
            for (const auto clip : {SkinnedClip::Idle, SkinnedClip::Walking})
                for (int sample = 0; sample <= 120; ++sample)
                {
                    std::array<Vec3, 3> points{};
                    for (std::size_t node = 0; node < names.size(); ++node)
                    {
                        SkinnedNodeTransform transform{};
                        if (!Require(asset.NodeTransform(clip, asset.ClipDuration(clip) * sample / 120.0f,
                                                         names[node], transform, diagnostic), diagnostic)) return 1;
                        points[node] = {{transform[12], transform[13], transform[14]}};
                    }
                    const float upper = Distance(points[0], points[1]);
                    const float lower = Distance(points[1], points[2]);
                    minUpper = std::min(minUpper, upper); maxUpper = std::max(maxUpper, upper);
                    minLower = std::min(minLower, lower); maxLower = std::max(maxLower, lower);
                    minSquaredDifference = std::min(minSquaredDifference, upper * upper - lower * lower);
                }
            std::cout << std::setprecision(9) << names[0] << " 242 idle/walk samples upper=["
                      << minUpper << ',' << maxUpper << "] lower=[" << minLower << ',' << maxLower
                      << "] min(u^2-l^2)=" << minSquaredDifference << '\n';
        }
        return 0;
    }
    if (argc != 1) return 2;
    FixtureData fixture = MakeFixtureData();
    GlbBinary binary;
    const AccessorSet accessors = AddFixtureViews(fixture, binary);
    const std::string json = MakeFixtureJson(fixture, binary, accessors);
    TemporaryGlb temporary;
    if (!Require(!temporary.Path().empty(), "could not allocate a unique fixture path") ||
        !Require(WriteGlb(temporary.Path(), json, binary.Bytes()),
                 "could not write the temporary reference GLB")) return 1;

    SkinnedMeshAsset asset;
    std::string diagnostic;
    if (!Require(asset.LoadClips(temporary.Path().string(), PlayerLocomotionClipSet(),
                                 diagnostic), diagnostic)) return 1;
    if (!Require(asset.HasNode("LeftArm") && asset.HasNode("LeftForeArm") &&
                 asset.HasNode("LeftHand") && asset.HasNode("LeftGrip") &&
                 asset.HasNode("RightArm") && asset.HasNode("RightForeArm") &&
                 asset.HasNode("RightHand") && asset.HasNode("RightGrip"),
                 "fixture must contain the named bilateral player chains")) return 1;

    std::array<SkinnedNodeTransform, 2u> armRoots{};
    std::array<SkinnedNodeTransform, 2u> elbows{};
    std::array<SkinnedNodeTransform, 2u> hands{};
    for (std::size_t side = 0u; side < fixture.arms.size(); ++side)
    {
        const char* armName = side == 0u ? "LeftArm" : "RightArm";
        const char* elbowName = side == 0u ? "LeftForeArm" : "RightForeArm";
        const char* handName = side == 0u ? "LeftHand" : "RightHand";
        if (!Require(asset.NodeTransform(SkinnedClip::Idle, 0.0f, armName,
                                         armRoots[side], diagnostic), diagnostic) ||
            !Require(asset.NodeTransform(SkinnedClip::Idle, 0.0f, elbowName,
                                         elbows[side], diagnostic), diagnostic) ||
            !Require(asset.NodeTransform(SkinnedClip::Idle, 0.0f, handName,
                                         hands[side], diagnostic), diagnostic)) return 1;
        const float expectedSide = side == 0u ? 1.0f : -1.0f;
        if (!Require(armRoots[side][12] * expectedSide > 0.39f,
                     "anatomical Left mount must be +X and Right mount -X")) return 1;
        if (!Require(std::abs(Distance({{armRoots[side][12], armRoots[side][13], armRoots[side][14]}},
                                       {{elbows[side][12], elbows[side][13], elbows[side][14]}}) -
                             kUpperLength) < 0.00001f &&
                     std::abs(Distance({{elbows[side][12], elbows[side][13], elbows[side][14]}},
                                       {{hands[side][12], hands[side][13], hands[side][14]}}) -
                             kLowerLength) < 0.00001f,
                     "fixture bind chain must have exact 0.30/0.40 m lengths")) return 1;
    }

    std::vector<TexturedSkinnedRtVertex> bindVertices;
    if (!Require(asset.SkinUniqueTextured(SkinnedClip::Idle, 0.0f,
                                          bindVertices, diagnostic), diagnostic)) return 1;
    if (!Require(bindVertices.size() == fixture.vertices.size() &&
                 asset.ExpandedVertexCount() == fixture.indices.size(),
                 "fixture topology and unique skinned vertex streams must import intact")) return 1;
    for (std::size_t vertex = 0u; vertex < fixture.vertices.size(); ++vertex)
    {
        if (!Require(NearVec(OutputPosition(bindVertices,
                                           static_cast<std::uint32_t>(vertex)),
                             fixture.vertices[vertex].position, 0.00001f),
                     "inverse-bind palette must reproduce every authored bind vertex")) return 1;
    }
    for (std::size_t index = 0u; index + 2u < fixture.indices.size(); index += 3u)
    {
        const auto p0 = fixture.vertices[fixture.indices[index]].position;
        const auto p1 = fixture.vertices[fixture.indices[index + 1u]].position;
        const auto p2 = fixture.vertices[fixture.indices[index + 2u]].position;
        const Vec3 a = Subtract(p1, p0), b = Subtract(p2, p0);
        const Vec3 cross{{a[1] * b[2] - a[2] * b[1],
                          a[2] * b[0] - a[0] * b[2],
                          a[0] * b[1] - a[1] * b[0]}};
        if (!Require(Length(cross) > 0.000001f,
                     "arm strip fixture must contain no degenerate topology triangles")) return 1;
        const Vec3 outward = Add(Add(fixture.vertices[fixture.indices[index]].normal,
                                      fixture.vertices[fixture.indices[index + 1u]].normal),
                                  fixture.vertices[fixture.indices[index + 2u]].normal);
        if (!Require(Dot(cross, outward) > 0.0f,
                     "fixture triangle winding must agree with its outward normals")) return 1;
    }

    // Fold each shoulder inward by 0.20 m. The unequal-chain cosine law gives
    // axial elbow projection -0.075 m and perpendicular height sqrt(0.3^2-.075^2).
    if (!CheckPose(asset, fixture, kFoldDistance, true)) return 1;
    // Keep an ordinary reachable pose as a control for the same importer/LBS path.
    if (!CheckPose(asset, fixture, 0.50f, false)) return 1;

    std::cout << "Skinned arm analytic reference passed: bind 0.30/0.40 m, "
                 "folded 0.20 m, mirrored mounts, grip sockets, LBS and topology\n";
    return 0;
}
