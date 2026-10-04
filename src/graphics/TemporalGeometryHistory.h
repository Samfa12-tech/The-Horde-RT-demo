#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace horde::graphics::temporal
{
// CPU vertical slice only. No production accumulation, GPU history images,
// upscaler, or camera-only motion claim is implied by this geometry producer.
struct Vec3 { float x = 0, y = 0, z = 0; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3 operator*(Vec3 a, float b) { return {a.x*b, a.y*b, a.z*b}; }
inline float Dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b)
{ return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline bool Finite(Vec3 v)
{ return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
inline Vec3 Unit(Vec3 v)
{ const float length = std::sqrt(Dot(v,v)); return length > 1e-12f ? v*(1/length) : Vec3{}; }

struct Camera
{
    Vec3 origin, forward, right, up;
    std::uint32_t width = 0, height = 0;
};

// Mirrors rt_frame.glsl's unjittered ray basis, including walk bob. Future
// jitter must be carried separately; motion remains in unjittered UV units.
inline Camera MakeHordeCamera(float x, float z, float yaw, float pitch,
                              float time, float walk, std::uint32_t width,
                              std::uint32_t height)
{
    const float step = time * 6.2f;
    Camera result;
    result.origin = {x + std::sin(step * 0.5f) * 0.035f * walk,
                     0.70f + std::abs(std::sin(step)) * 0.035f * walk, z};
    pitch = std::clamp(pitch + std::sin(step) * 0.012f * walk, -0.32f, 0.28f);
    result.forward = Unit({std::sin(yaw), -0.05f + pitch, -std::cos(yaw)});
    result.right = Unit(Cross(result.forward, {0,1,0}));
    result.up = Unit(Cross(result.right, result.forward));
    result.width = width;
    result.height = height;
    return result;
}

struct Projection { float u, v, depth; };
inline std::optional<Projection> Project(const Camera& camera, Vec3 point)
{
    if (!camera.width || !camera.height || !Finite(point) || !Finite(camera.origin) ||
        !Finite(camera.forward) || !Finite(camera.right) || !Finite(camera.up)) return {};
    const Vec3 delta = point - camera.origin;
    const float depth = Dot(delta, camera.forward);
    if (!std::isfinite(depth) || depth <= 1e-5f) return {};
    const float aspect = static_cast<float>(camera.width) / camera.height;
    Projection result{0.5f + 1.22f * Dot(delta,camera.right)/(2*aspect*depth),
                      0.5f - 1.22f * Dot(delta,camera.up)/(1.48f*depth), depth};
    if (!std::isfinite(result.u) || !std::isfinite(result.v)) return {};
    return result;
}

using Transform = std::array<float,12>;
inline constexpr Transform kIdentity{{1,0,0,0, 0,1,0,0, 0,0,1,0}};
inline Vec3 TransformPoint(const Transform& m, Vec3 p)
{
    return {m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],
            m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],
            m[8]*p.x+m[9]*p.y+m[10]*p.z+m[11]};
}

struct Object
{
    std::uint64_t identity = 0;
    std::uint64_t topologyGeneration = 0;
    Transform transform = kIdentity;
    // Actual post-deformation local vertices, shared by world/viewmodel/static
    // producers. Never reconstruct a previous pose from current animation time.
    std::vector<Vec3> vertices;
};
struct Frame { Camera camera; std::vector<Object> objects; };
inline constexpr std::size_t kMaximumObjects = 32;
inline constexpr std::size_t kMaximumVertices = 4096;

inline bool Valid(const Frame& frame)
{
    if (!frame.camera.width || !frame.camera.height || frame.objects.size() > kMaximumObjects ||
        !Finite(frame.camera.origin) || !Finite(frame.camera.forward) ||
        !Finite(frame.camera.right) || !Finite(frame.camera.up) ||
        std::abs(Dot(frame.camera.forward,frame.camera.forward)-1) > 0.001f ||
        std::abs(Dot(frame.camera.right,frame.camera.right)-1) > 0.001f ||
        std::abs(Dot(frame.camera.up,frame.camera.up)-1) > 0.001f ||
        std::abs(Dot(frame.camera.forward,frame.camera.right)) > 0.001f ||
        std::abs(Dot(frame.camera.forward,frame.camera.up)) > 0.001f ||
        std::abs(Dot(frame.camera.right,frame.camera.up)) > 0.001f) return false;
    std::size_t vertices = 0;
    for (std::size_t i = 0; i < frame.objects.size(); ++i)
    {
        const auto& object = frame.objects[i];
        if (!object.identity || object.vertices.size() > kMaximumVertices-vertices) return false;
        vertices += object.vertices.size();
        for (std::size_t j = 0; j < i; ++j)
            if (frame.objects[j].identity == object.identity) return false;
        for (float value : object.transform) if (!std::isfinite(value)) return false;
        for (Vec3 vertex : object.vertices) if (!Finite(vertex)) return false;
    }
    return true;
}

enum class ResetReason : std::uint8_t
{
    Initial, Retry, CameraCut, Checkpoint, PreviewTransition, Lifecycle,
    BackendChange, SettingsChange, Resize, FailedFrame, InvalidGeometry
};
struct Hit
{
    std::uint64_t identity = 0;
    std::array<std::uint32_t,3> triangle{};
    // Same barycentric correspondence in both actual deformed poses.
    std::array<float,3> barycentric{{1,0,0}};
    bool reactive = false; // misses, glass/water, flame/volume composites
};
struct Motion
{
    float toPreviousU, toPreviousV;
    Projection current, previous;
    Vec3 previousNormal;
    std::uint64_t identity;
};
inline bool Inside(const Projection& p)
{ return p.u >= 0 && p.u <= 1 && p.v >= 0 && p.v <= 1; }

class GeometryHistory
{
public:
    void Invalidate(ResetReason reason)
    { previous_.reset(); lastReason_ = reason; ++generation_; }

    bool Commit(const Frame& frame, std::uint64_t sequence, bool submitted, bool presented)
    {
        if (!submitted || !presented) { Invalidate(ResetReason::FailedFrame); return false; }
        if (!sequence || sequence <= lastSequence_) return false;
        if (!Valid(frame)) { Invalidate(ResetReason::InvalidGeometry); return false; }
        // Repack only bounded live data: caller vector capacity is not trusted.
        Frame bounded;
        bounded.camera = frame.camera;
        bounded.objects.reserve(frame.objects.size());
        for (const Object& object : frame.objects)
            bounded.objects.push_back({object.identity, object.topologyGeneration, object.transform,
                                       std::vector<Vec3>(object.vertices.begin(),object.vertices.end())});
        previous_ = std::move(bounded);
        lastSequence_ = sequence;
        return true;
    }

    std::optional<Motion> Reproject(const Frame& current, const Hit& hit) const
    {
        if (!previous_ || hit.reactive || !Valid(current) ||
            current.camera.width != previous_->camera.width ||
            current.camera.height != previous_->camera.height) return {};
        const Object* now = Find(current,hit.identity);
        const Object* before = Find(*previous_,hit.identity);
        if (!now || !before || now->topologyGeneration != before->topologyGeneration ||
            now->vertices.size() != before->vertices.size()) return {};
        float sum = 0;
        Vec3 currentPoint{}, previousPoint{};
        std::array<Vec3,3> currentTriangle{};
        std::array<Vec3,3> previousTriangle{};
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (!std::isfinite(hit.barycentric[i]) || hit.barycentric[i] < 0 ||
                hit.barycentric[i] > 1 || hit.triangle[i] >= now->vertices.size()) return {};
            sum += hit.barycentric[i];
            previousTriangle[i] = TransformPoint(before->transform,before->vertices[hit.triangle[i]]);
            currentTriangle[i] = TransformPoint(now->transform,now->vertices[hit.triangle[i]]);
            currentPoint = currentPoint + currentTriangle[i] * hit.barycentric[i];
            previousPoint = previousPoint + previousTriangle[i] * hit.barycentric[i];
        }
        if (std::abs(sum-1) > 1e-5f) return {};
        const auto nowProjection = Project(current.camera,currentPoint);
        const auto oldProjection = Project(previous_->camera,previousPoint);
        const Vec3 normal = Unit(Cross(previousTriangle[1]-previousTriangle[0],
                                      previousTriangle[2]-previousTriangle[0]));
        const Vec3 currentNormal = Unit(Cross(currentTriangle[1]-currentTriangle[0],
                                             currentTriangle[2]-currentTriangle[0]));
        if (!nowProjection || !oldProjection || !Inside(*nowProjection) ||
            !Inside(*oldProjection) || Dot(normal,normal) < 0.5f ||
            Dot(currentNormal,currentNormal) < 0.5f) return {};
        return Motion{oldProjection->u-nowProjection->u, oldProjection->v-nowProjection->v,
                      *nowProjection,*oldProjection,normal,hit.identity};
    }

    // This consumes a sampled previous primary depth/normal/identity. The GPU
    // attachments and sampler are absent today, so admission remains blocked.
    static bool ValidatePreviousSurface(const Motion& motion, float sampledDepth,
                                        Vec3 sampledNormal, std::uint64_t sampledIdentity)
    {
        return sampledIdentity == motion.identity && std::isfinite(sampledDepth) &&
               sampledDepth > 0 && Finite(sampledNormal) &&
               std::abs(sampledDepth-motion.previous.depth) <=
                   std::max(0.01f,motion.previous.depth*0.01f) &&
               Dot(Unit(sampledNormal),motion.previousNormal) >= 0.85f;
    }

    std::size_t RetainedVertexBytes() const
    {
        std::size_t result = 0;
        if (previous_) for (const auto& object : previous_->objects)
            result += object.vertices.capacity()*sizeof(Vec3);
        return result;
    }
    std::uint64_t Generation() const { return generation_; }
    ResetReason LastReason() const { return lastReason_; }
private:
    static const Object* Find(const Frame& frame, std::uint64_t identity)
    {
        for (const auto& object : frame.objects) if (object.identity == identity) return &object;
        return nullptr;
    }
    std::optional<Frame> previous_;
    std::uint64_t lastSequence_ = 0, generation_ = 0;
    ResetReason lastReason_ = ResetReason::Initial;
};

// Proposed GPU attachment lower bound: depth R32F(4), motion RG32F(8),
// current/history RGBA16F(8+8), validation identity/confidence(4). This excludes
// output, descriptors, alignment, previous geometry and transient scratch.
inline std::optional<std::uint64_t> ProposedAttachmentBytes(std::uint64_t width,
                                                          std::uint64_t height)
{
    constexpr std::uint64_t bytesPerPixel = 32;
    const auto limit = std::numeric_limits<std::uint64_t>::max();
    if (!width || !height || height > limit/width) return {};
    const std::uint64_t pixels = width*height;
    if (pixels > limit/bytesPerPixel) return {};
    return pixels*bytesPerPixel;
}
} // namespace horde::graphics::temporal
