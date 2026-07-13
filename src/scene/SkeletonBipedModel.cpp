#include "scene/SkeletonBipedModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string_view>
#include <utility>

namespace horde::scene
{
namespace
{

enum class JsonType { Null, Boolean, Number, String, Array, Object };

struct JsonValue
{
    JsonType type = JsonType::Null;
    double number = 0.0;
    std::string string;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue, std::less<>> object;

    const JsonValue* Find(std::string_view key) const
    {
        const auto it = object.find(key);
        return it == object.end() ? nullptr : &it->second;
    }
    std::uint32_t Uint() const { return static_cast<std::uint32_t>(number); }
    float Float() const { return static_cast<float>(number); }
};

class JsonParser
{
public:
    explicit JsonParser(std::string_view source) : cursor_(source.data()), end_(source.data() + source.size()) {}

    bool Parse(JsonValue& result, std::string& diagnostic)
    {
        SkipWhitespace();
        if (!ParseValue(result, diagnostic)) return false;
        SkipWhitespace();
        if (cursor_ != end_)
        {
            diagnostic = "Unexpected trailing data in skeleton GLB JSON.";
            return false;
        }
        return true;
    }

private:
    void SkipWhitespace()
    {
        while (cursor_ != end_ && (*cursor_ == ' ' || *cursor_ == '\n' || *cursor_ == '\r' || *cursor_ == '\t')) ++cursor_;
    }
    bool Consume(char expected)
    {
        SkipWhitespace();
        if (cursor_ == end_ || *cursor_ != expected) return false;
        ++cursor_;
        return true;
    }
    bool ParseString(std::string& out, std::string& diagnostic)
    {
        if (cursor_ == end_ || *cursor_++ != '"')
        {
            diagnostic = "Expected a JSON string in skeleton GLB.";
            return false;
        }
        while (cursor_ != end_ && *cursor_ != '"')
        {
            char character = *cursor_++;
            if (character != '\\')
            {
                out.push_back(character);
                continue;
            }
            if (cursor_ == end_)
            {
                diagnostic = "Unterminated escape in skeleton GLB JSON.";
                return false;
            }
            const char escaped = *cursor_++;
            switch (escaped)
            {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default:
                diagnostic = "Unsupported escape in skeleton GLB JSON.";
                return false;
            }
        }
        if (cursor_ == end_)
        {
            diagnostic = "Unterminated string in skeleton GLB JSON.";
            return false;
        }
        ++cursor_;
        return true;
    }
    bool ParseValue(JsonValue& out, std::string& diagnostic)
    {
        SkipWhitespace();
        if (cursor_ == end_)
        {
            diagnostic = "Unexpected end of skeleton GLB JSON.";
            return false;
        }
        if (*cursor_ == '{')
        {
            out.type = JsonType::Object;
            ++cursor_;
            SkipWhitespace();
            if (cursor_ != end_ && *cursor_ == '}') { ++cursor_; return true; }
            while (true)
            {
                std::string key;
                if (!ParseString(key, diagnostic) || !Consume(':')) return false;
                JsonValue value;
                if (!ParseValue(value, diagnostic)) return false;
                out.object.emplace(std::move(key), std::move(value));
                SkipWhitespace();
                if (cursor_ != end_ && *cursor_ == '}') { ++cursor_; return true; }
                if (!Consume(',')) { diagnostic = "Expected comma in skeleton GLB JSON object."; return false; }
            }
        }
        if (*cursor_ == '[')
        {
            out.type = JsonType::Array;
            ++cursor_;
            SkipWhitespace();
            if (cursor_ != end_ && *cursor_ == ']') { ++cursor_; return true; }
            while (true)
            {
                JsonValue value;
                if (!ParseValue(value, diagnostic)) return false;
                out.array.push_back(std::move(value));
                SkipWhitespace();
                if (cursor_ != end_ && *cursor_ == ']') { ++cursor_; return true; }
                if (!Consume(',')) { diagnostic = "Expected comma in skeleton GLB JSON array."; return false; }
            }
        }
        if (*cursor_ == '"')
        {
            out.type = JsonType::String;
            return ParseString(out.string, diagnostic);
        }
        if (std::strncmp(cursor_, "true", 4u) == 0) { cursor_ += 4; out.type = JsonType::Boolean; out.number = 1.0; return true; }
        if (std::strncmp(cursor_, "false", 5u) == 0) { cursor_ += 5; out.type = JsonType::Boolean; return true; }
        if (std::strncmp(cursor_, "null", 4u) == 0) { cursor_ += 4; return true; }
        char* numberEnd = nullptr;
        out.number = std::strtod(cursor_, &numberEnd);
        if (numberEnd == cursor_)
        {
            diagnostic = "Invalid value in skeleton GLB JSON.";
            return false;
        }
        cursor_ = numberEnd;
        out.type = JsonType::Number;
        return true;
    }

    const char* cursor_;
    const char* end_;
};

struct Accessor
{
    std::size_t offset = 0u;
    std::size_t stride = 0u;
    std::size_t count = 0u;
    std::uint32_t componentType = 0u;
    std::uint32_t components = 0u;
};

std::uint32_t ComponentCount(const std::string& type)
{
    if (type == "SCALAR") return 1u;
    if (type == "VEC2") return 2u;
    if (type == "VEC3") return 3u;
    if (type == "VEC4") return 4u;
    if (type == "MAT4") return 16u;
    return 0u;
}

std::uint32_t ComponentSize(std::uint32_t type)
{
    switch (type)
    {
    case 5120u: case 5121u: return 1u;
    case 5122u: case 5123u: return 2u;
    case 5125u: case 5126u: return 4u;
    default: return 0u;
    }
}

bool GetArrayMember(const JsonValue& object, std::string_view key, const std::vector<JsonValue>*& result, std::string& diagnostic)
{
    const JsonValue* value = object.Find(key);
    if (value == nullptr || value->type != JsonType::Array)
    {
        diagnostic = "Skeleton GLB is missing array '" + std::string(key) + "'.";
        return false;
    }
    result = &value->array;
    return true;
}

bool ReadAccessor(const std::vector<JsonValue>& accessors,
                  const std::vector<JsonValue>& views,
                  std::uint32_t accessorIndex,
                  Accessor& result,
                  std::string& diagnostic)
{
    if (accessorIndex >= accessors.size())
    {
        diagnostic = "Skeleton GLB accessor index is invalid.";
        return false;
    }
    const JsonValue& accessor = accessors[accessorIndex];
    const JsonValue* viewIndex = accessor.Find("bufferView");
    const JsonValue* componentType = accessor.Find("componentType");
    const JsonValue* count = accessor.Find("count");
    const JsonValue* type = accessor.Find("type");
    if (viewIndex == nullptr || componentType == nullptr || count == nullptr || type == nullptr || type->type != JsonType::String || viewIndex->Uint() >= views.size())
    {
        diagnostic = "Skeleton GLB has an unsupported accessor.";
        return false;
    }
    const JsonValue& view = views[viewIndex->Uint()];
    const JsonValue* viewOffset = view.Find("byteOffset");
    const JsonValue* viewStride = view.Find("byteStride");
    const std::uint32_t components = ComponentCount(type->string);
    const std::uint32_t componentBytes = ComponentSize(componentType->Uint());
    if (components == 0u || componentBytes == 0u)
    {
        diagnostic = "Skeleton GLB uses an unsupported accessor type.";
        return false;
    }
    const JsonValue* accessorOffset = accessor.Find("byteOffset");
    result.offset = (viewOffset == nullptr ? 0u : viewOffset->Uint()) + (accessorOffset == nullptr ? 0u : accessorOffset->Uint());
    result.stride = viewStride == nullptr ? components * componentBytes : viewStride->Uint();
    result.count = count->Uint();
    result.componentType = componentType->Uint();
    result.components = components;
    return true;
}

template <typename T>
T ReadScalar(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    T result{};
    std::memcpy(&result, bytes.data() + offset, sizeof(T));
    return result;
}

float ReadFloat(const std::vector<std::uint8_t>& bytes, const Accessor& accessor, std::size_t index, std::size_t component)
{
    return ReadScalar<float>(bytes, accessor.offset + index * accessor.stride + component * sizeof(float));
}

std::uint32_t ReadUnsigned(const std::vector<std::uint8_t>& bytes, const Accessor& accessor, std::size_t index, std::size_t component)
{
    const std::size_t offset = accessor.offset + index * accessor.stride + component * ComponentSize(accessor.componentType);
    switch (accessor.componentType)
    {
    case 5121u: return ReadScalar<std::uint8_t>(bytes, offset);
    case 5123u: return ReadScalar<std::uint16_t>(bytes, offset);
    case 5125u: return ReadScalar<std::uint32_t>(bytes, offset);
    default: return 0u;
    }
}

struct Vec3 { float x = 0.0f; float y = 0.0f; float z = 0.0f; };
struct Quat { float x = 0.0f; float y = 0.0f; float z = 0.0f; float w = 1.0f; };
struct Matrix { std::array<float, 16u> m{}; };

Matrix Multiply(const Matrix& a, const Matrix& b)
{
    Matrix result{};
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                result.m[column * 4 + row] += a.m[k * 4 + row] * b.m[column * 4 + k];
    return result;
}

Vec3 TransformPoint(const Matrix& matrix, Vec3 point)
{
    return {matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8] * point.z + matrix.m[12],
            matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9] * point.z + matrix.m[13],
            matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z + matrix.m[14]};
}

Vec3 TransformVector(const Matrix& matrix, Vec3 vector)
{
    return {matrix.m[0] * vector.x + matrix.m[4] * vector.y + matrix.m[8] * vector.z,
            matrix.m[1] * vector.x + matrix.m[5] * vector.y + matrix.m[9] * vector.z,
            matrix.m[2] * vector.x + matrix.m[6] * vector.y + matrix.m[10] * vector.z};
}

Matrix LocalMatrix(Vec3 translation, Quat rotation, Vec3 scale)
{
    const float x2 = rotation.x + rotation.x;
    const float y2 = rotation.y + rotation.y;
    const float z2 = rotation.z + rotation.z;
    const float xx = rotation.x * x2, xy = rotation.x * y2, xz = rotation.x * z2;
    const float yy = rotation.y * y2, yz = rotation.y * z2, zz = rotation.z * z2;
    const float wx = rotation.w * x2, wy = rotation.w * y2, wz = rotation.w * z2;
    Matrix result{};
    result.m = {{(1.0f - (yy + zz)) * scale.x, (xy + wz) * scale.x, (xz - wy) * scale.x, 0.0f,
                 (xy - wz) * scale.y, (1.0f - (xx + zz)) * scale.y, (yz + wx) * scale.y, 0.0f,
                 (xz + wy) * scale.z, (yz - wx) * scale.z, (1.0f - (xx + yy)) * scale.z, 0.0f,
                 translation.x, translation.y, translation.z, 1.0f}};
    return result;
}

Quat Normalise(Quat value)
{
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w);
    if (length > 0.00001f) { value.x /= length; value.y /= length; value.z /= length; value.w /= length; }
    return value;
}

Quat Nlerp(Quat a, Quat b, float t)
{
    if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0.0f) { b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w; }
    return Normalise({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t});
}

Vec3 Normalise(Vec3 value)
{
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    return length > 0.00001f ? Vec3{value.x / length, value.y / length, value.z / length} : Vec3{0.0f, 1.0f, 0.0f};
}

} // namespace

struct SkeletonBipedModel::SourceVertex
{
    Vec3 position;
    Vec3 normal;
    std::array<std::uint8_t, 4u> joints{};
    std::array<float, 4u> weights{};
};

struct SkeletonBipedModel::Node
{
    int parent = -1;
    Vec3 translation{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct SkeletonBipedModel::Channel
{
    enum class Path { Translation, Rotation, Scale } path = Path::Translation;
    std::uint32_t node = 0u;
    std::vector<float> times;
    std::vector<std::array<float, 4u>> values;
};

struct SkeletonBipedModel::Clip
{
    std::vector<Channel> channels;
    float duration = 0.0f;
    bool loops = false;
};

SkeletonBipedModel::SkeletonBipedModel() = default;
SkeletonBipedModel::~SkeletonBipedModel() = default;
SkeletonBipedModel::SkeletonBipedModel(SkeletonBipedModel&&) noexcept = default;
SkeletonBipedModel& SkeletonBipedModel::operator=(SkeletonBipedModel&&) noexcept = default;

bool SkeletonBipedModel::LoadCombatClips(const std::string& glbPath, std::string& diagnostic)
{
    loaded_ = false;
    vertices_.clear(); expandedIndices_.clear(); nodes_.clear(); joints_.clear(); inverseBindMatrices_.clear(); clips_.clear(); skinnedUniqueVertices_.clear();
    std::ifstream stream(glbPath, std::ios::binary);
    if (!stream)
    {
        diagnostic = "Could not open skeleton GLB: " + glbPath;
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::size_t fileSize = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> file(fileSize);
    if (fileSize < 20u || !stream.read(reinterpret_cast<char*>(file.data()), static_cast<std::streamsize>(file.size())))
    {
        diagnostic = "Could not read skeleton GLB.";
        return false;
    }
    if (ReadScalar<std::uint32_t>(file, 0u) != 0x46546c67u || ReadScalar<std::uint32_t>(file, 4u) != 2u)
    {
        diagnostic = "Skeleton asset is not a glTF 2.0 GLB.";
        return false;
    }
    std::size_t cursor = 12u;
    std::string json;
    std::vector<std::uint8_t> binary;
    while (cursor + 8u <= file.size())
    {
        const std::uint32_t length = ReadScalar<std::uint32_t>(file, cursor);
        const std::uint32_t type = ReadScalar<std::uint32_t>(file, cursor + 4u);
        cursor += 8u;
        if (cursor + length > file.size()) { diagnostic = "Skeleton GLB chunk is truncated."; return false; }
        if (type == 0x4e4f534au) json.assign(reinterpret_cast<const char*>(file.data() + cursor), length);
        if (type == 0x004e4942u) binary.assign(file.begin() + static_cast<std::ptrdiff_t>(cursor), file.begin() + static_cast<std::ptrdiff_t>(cursor + length));
        cursor += length;
    }
    JsonValue root;
    JsonParser parser(json);
    if (json.empty() || binary.empty() || !parser.Parse(root, diagnostic)) return false;
    const std::vector<JsonValue>* accessors = nullptr;
    const std::vector<JsonValue>* views = nullptr;
    const std::vector<JsonValue>* meshes = nullptr;
    const std::vector<JsonValue>* skins = nullptr;
    const std::vector<JsonValue>* nodes = nullptr;
    const std::vector<JsonValue>* animations = nullptr;
    if (!GetArrayMember(root, "accessors", accessors, diagnostic) || !GetArrayMember(root, "bufferViews", views, diagnostic) ||
        !GetArrayMember(root, "meshes", meshes, diagnostic) || !GetArrayMember(root, "skins", skins, diagnostic) ||
        !GetArrayMember(root, "nodes", nodes, diagnostic) || !GetArrayMember(root, "animations", animations, diagnostic) ||
        meshes->empty() || skins->empty()) return false;

    const JsonValue* primitives = (*meshes)[0].Find("primitives");
    const JsonValue* attributes = primitives != nullptr && !primitives->array.empty() ? primitives->array[0].Find("attributes") : nullptr;
    const JsonValue* indexAccessor = primitives != nullptr && !primitives->array.empty() ? primitives->array[0].Find("indices") : nullptr;
    if (attributes == nullptr || indexAccessor == nullptr)
    {
        diagnostic = "Skeleton GLB does not contain its expected mesh primitive.";
        return false;
    }
    const JsonValue* position = attributes->Find("POSITION");
    const JsonValue* normal = attributes->Find("NORMAL");
    const JsonValue* joint = attributes->Find("JOINTS_0");
    const JsonValue* weight = attributes->Find("WEIGHTS_0");
    Accessor positions, normals, joints, weights, indices;
    if (position == nullptr || normal == nullptr || joint == nullptr || weight == nullptr ||
        !ReadAccessor(*accessors, *views, position->Uint(), positions, diagnostic) || !ReadAccessor(*accessors, *views, normal->Uint(), normals, diagnostic) ||
        !ReadAccessor(*accessors, *views, joint->Uint(), joints, diagnostic) || !ReadAccessor(*accessors, *views, weight->Uint(), weights, diagnostic) ||
        !ReadAccessor(*accessors, *views, indexAccessor->Uint(), indices, diagnostic) || positions.count != normals.count || positions.count != joints.count || positions.count != weights.count ||
        positions.componentType != 5126u || normals.componentType != 5126u || weights.componentType != 5126u || joints.componentType != 5121u)
    {
        diagnostic = "Skeleton GLB mesh layout is not the audited Meshy layout.";
        return false;
    }
    vertices_.resize(positions.count);
    for (std::size_t i = 0u; i < vertices_.size(); ++i)
    {
        vertices_[i].position = {ReadFloat(binary, positions, i, 0u), ReadFloat(binary, positions, i, 1u), ReadFloat(binary, positions, i, 2u)};
        vertices_[i].normal = {ReadFloat(binary, normals, i, 0u), ReadFloat(binary, normals, i, 1u), ReadFloat(binary, normals, i, 2u)};
        for (std::size_t component = 0u; component < 4u; ++component)
        {
            vertices_[i].joints[component] = static_cast<std::uint8_t>(ReadUnsigned(binary, joints, i, component));
            vertices_[i].weights[component] = ReadFloat(binary, weights, i, component);
        }
    }
    if (indices.componentType != 5123u || indices.count % 3u != 0u)
    {
        diagnostic = "Skeleton GLB index layout is unsupported.";
        return false;
    }
    expandedIndices_.resize(indices.count);
    for (std::size_t i = 0u; i < indices.count; ++i)
    {
        expandedIndices_[i] = ReadUnsigned(binary, indices, i, 0u);
        if (expandedIndices_[i] >= vertices_.size()) { diagnostic = "Skeleton GLB index is out of range."; return false; }
    }

    nodes_.resize(nodes->size());
    for (std::size_t nodeIndex = 0u; nodeIndex < nodes_.size(); ++nodeIndex)
    {
        const JsonValue& node = (*nodes)[nodeIndex];
        const auto readVec = [&node](std::string_view name, float* destination, std::size_t count) {
            const JsonValue* value = node.Find(name);
            if (value != nullptr && value->type == JsonType::Array && value->array.size() == count)
                for (std::size_t i = 0u; i < count; ++i) destination[i] = value->array[i].Float();
        };
        readVec("translation", &nodes_[nodeIndex].translation.x, 3u);
        readVec("rotation", &nodes_[nodeIndex].rotation.x, 4u);
        readVec("scale", &nodes_[nodeIndex].scale.x, 3u);
        if (const JsonValue* children = node.Find("children"); children != nullptr)
            for (const JsonValue& child : children->array)
                if (child.Uint() < nodes_.size()) nodes_[child.Uint()].parent = static_cast<int>(nodeIndex);
    }
    const JsonValue& skin = (*skins)[0];
    const JsonValue* skinJoints = skin.Find("joints");
    const JsonValue* inverseBind = skin.Find("inverseBindMatrices");
    if (skinJoints == nullptr || inverseBind == nullptr || skinJoints->array.empty()) { diagnostic = "Skeleton GLB skin is incomplete."; return false; }
    for (const JsonValue& value : skinJoints->array) joints_.push_back(value.Uint());
    Accessor inverseBinds;
    if (!ReadAccessor(*accessors, *views, inverseBind->Uint(), inverseBinds, diagnostic) || inverseBinds.componentType != 5126u || inverseBinds.components != 16u || inverseBinds.count != joints_.size())
    {
        diagnostic = "Skeleton GLB inverse bind matrices are unsupported.";
        return false;
    }
    inverseBindMatrices_.resize(inverseBinds.count * 16u);
    for (std::size_t i = 0u; i < inverseBindMatrices_.size(); ++i) inverseBindMatrices_[i] = ReadFloat(binary, inverseBinds, i / 16u, i % 16u);

    constexpr std::array<std::string_view, 4u> expectedNames{{"Idle_5", "Walking", "Attack", "Dead"}};
    for (std::size_t clipIndex = 0u; clipIndex < expectedNames.size(); ++clipIndex)
    {
        const JsonValue* sourceAnimation = nullptr;
        for (const JsonValue& animation : *animations)
        {
            const JsonValue* name = animation.Find("name");
            if (name != nullptr && name->string == expectedNames[clipIndex])
            {
                sourceAnimation = &animation;
                break;
            }
        }
        if (sourceAnimation == nullptr)
        {
            diagnostic = "Skeleton GLB is missing combat clip: " + std::string(expectedNames[clipIndex]);
            return false;
        }
        const JsonValue* samplers = sourceAnimation->Find("samplers");
        const JsonValue* animationChannels = sourceAnimation->Find("channels");
        if (samplers == nullptr || animationChannels == nullptr)
        {
            diagnostic = "Skeleton combat clip is incomplete: " + std::string(expectedNames[clipIndex]);
            return false;
        }

        Clip clip;
        clip.loops = clipIndex <= static_cast<std::size_t>(SkeletonClip::Walking);
        for (const JsonValue& sourceChannel : animationChannels->array)
        {
            const JsonValue* samplerIndex = sourceChannel.Find("sampler");
            const JsonValue* target = sourceChannel.Find("target");
            const JsonValue* targetNode = target == nullptr ? nullptr : target->Find("node");
            const JsonValue* path = target == nullptr ? nullptr : target->Find("path");
            if (samplerIndex == nullptr || targetNode == nullptr || path == nullptr || samplerIndex->Uint() >= samplers->array.size()) continue;
            const JsonValue& sampler = samplers->array[samplerIndex->Uint()];
            const JsonValue* input = sampler.Find("input");
            const JsonValue* output = sampler.Find("output");
            if (input == nullptr || output == nullptr) continue;
            Accessor inputAccessor, outputAccessor;
            if (!ReadAccessor(*accessors, *views, input->Uint(), inputAccessor, diagnostic) || !ReadAccessor(*accessors, *views, output->Uint(), outputAccessor, diagnostic) ||
                inputAccessor.componentType != 5126u || outputAccessor.componentType != 5126u || inputAccessor.count != outputAccessor.count ||
                (path->string != "translation" && path->string != "rotation" && path->string != "scale")) return false;
            Channel channel;
            channel.node = targetNode->Uint();
            channel.path = path->string == "rotation" ? Channel::Path::Rotation : (path->string == "scale" ? Channel::Path::Scale : Channel::Path::Translation);
            channel.times.resize(inputAccessor.count);
            channel.values.resize(outputAccessor.count);
            for (std::size_t key = 0u; key < channel.times.size(); ++key)
            {
                channel.times[key] = ReadFloat(binary, inputAccessor, key, 0u);
                clip.duration = std::max(clip.duration, channel.times[key]);
                for (std::size_t component = 0u; component < outputAccessor.components; ++component) channel.values[key][component] = ReadFloat(binary, outputAccessor, key, component);
                if (outputAccessor.components == 3u) channel.values[key][3u] = 0.0f;
            }
            clip.channels.push_back(std::move(channel));
        }
        if (clip.duration <= 0.0f || clip.channels.empty())
        {
            diagnostic = "Skeleton combat clip contains no usable animation channels: " + std::string(expectedNames[clipIndex]);
            return false;
        }
        clips_.push_back(std::move(clip));
    }
    loaded_ = true;
    diagnostic.clear();
    return true;
}

float SkeletonBipedModel::ClipDuration(SkeletonClip clip) const
{
    const std::size_t index = static_cast<std::size_t>(clip);
    return index < clips_.size() ? clips_[index].duration : 0.0f;
}

bool SkeletonBipedModel::Skin(SkeletonClip clipId, float timeSeconds, std::vector<SkinnedRtVertex>& output, std::string& diagnostic) const
{
    if (!loaded_) { diagnostic = "Skeleton model was not loaded."; return false; }
    const std::size_t clipIndex = static_cast<std::size_t>(clipId);
    if (clipIndex >= clips_.size()) { diagnostic = "Skeleton combat clip index is invalid."; return false; }
    const Clip& clip = clips_[clipIndex];
    std::vector<Node> pose = nodes_;
    const float time = clip.loops
        ? std::fmod(std::max(timeSeconds, 0.0f), clip.duration)
        : std::clamp(timeSeconds, 0.0f, clip.duration);
    for (const Channel& channel : clip.channels)
    {
        if (channel.node >= pose.size() || channel.times.empty()) continue;
        const auto upper = std::upper_bound(channel.times.begin(), channel.times.end(), time);
        const std::size_t next = upper == channel.times.end()
            ? (clip.loops ? 0u : channel.times.size() - 1u)
            : static_cast<std::size_t>(upper - channel.times.begin());
        const std::size_t previous = next == 0u ? channel.times.size() - 1u : next - 1u;
        const float previousTime = channel.times[previous];
        const float nextTime = next == 0u ? clip.duration : channel.times[next];
        const float sampledTime = next == 0u && time < previousTime ? time + clip.duration : time;
        const float fraction = nextTime > previousTime ? std::clamp((sampledTime - previousTime) / (nextTime - previousTime), 0.0f, 1.0f) : 0.0f;
        const auto& a = channel.values[previous];
        const auto& b = channel.values[next];
        if (channel.path == Channel::Path::Rotation) pose[channel.node].rotation = Nlerp({a[0], a[1], a[2], a[3]}, {b[0], b[1], b[2], b[3]}, fraction);
        else
        {
            const Vec3 value{a[0] + (b[0] - a[0]) * fraction, a[1] + (b[1] - a[1]) * fraction, a[2] + (b[2] - a[2]) * fraction};
            if (channel.path == Channel::Path::Translation) pose[channel.node].translation = value;
            else pose[channel.node].scale = value;
        }
    }
    std::vector<Matrix> globals(pose.size());
    std::vector<bool> computed(pose.size(), false);
    const auto resolveGlobal = [&pose, &globals, &computed](auto&& self, std::size_t node) -> Matrix {
        if (computed[node]) return globals[node];
        Matrix local = LocalMatrix(pose[node].translation, pose[node].rotation, pose[node].scale);
        globals[node] = pose[node].parent < 0 ? local : Multiply(self(self, static_cast<std::size_t>(pose[node].parent)), local);
        computed[node] = true;
        return globals[node];
    };
    std::vector<Matrix> skinMatrices(joints_.size());
    for (std::size_t i = 0u; i < joints_.size(); ++i)
    {
        if (joints_[i] >= pose.size()) { diagnostic = "Skeleton joint node is invalid."; return false; }
        Matrix inverse{};
        std::memcpy(inverse.m.data(), inverseBindMatrices_.data() + i * 16u, sizeof(float) * 16u);
        skinMatrices[i] = Multiply(resolveGlobal(resolveGlobal, joints_[i]), inverse);
    }
    // Skin each source vertex once. The RT BLAS remains deliberately non-indexed,
    // but transforming the 9,854 unique vertices is much cheaper than repeating
    // the same four-weight work for all 28,206 expanded triangle vertices.
    skinnedUniqueVertices_.resize(vertices_.size());
    for (std::size_t sourceIndex = 0u; sourceIndex < vertices_.size(); ++sourceIndex)
    {
        const SourceVertex& source = vertices_[sourceIndex];
        Vec3 position{};
        Vec3 normal{};
        for (std::size_t influence = 0u; influence < 4u; ++influence)
        {
            const float weight = source.weights[influence];
            const std::size_t joint = source.joints[influence];
            if (weight <= 0.00001f || joint >= skinMatrices.size()) continue;
            const Vec3 transformedPosition = TransformPoint(skinMatrices[joint], source.position);
            const Vec3 transformedNormal = TransformVector(skinMatrices[joint], source.normal);
            position.x += transformedPosition.x * weight; position.y += transformedPosition.y * weight; position.z += transformedPosition.z * weight;
            normal.x += transformedNormal.x * weight; normal.y += transformedNormal.y * weight; normal.z += transformedNormal.z * weight;
        }
        normal = Normalise(normal);
        skinnedUniqueVertices_[sourceIndex] = {{position.x, position.y, position.z, 1.0f}, {normal.x, normal.y, normal.z, 0.0f}};
    }
    output.resize(expandedIndices_.size());
    for (std::size_t outputIndex = 0u; outputIndex < expandedIndices_.size(); ++outputIndex)
        output[outputIndex] = skinnedUniqueVertices_[expandedIndices_[outputIndex]];

    diagnostic.clear();
    return true;
}

} // namespace horde::scene
