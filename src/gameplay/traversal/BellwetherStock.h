#pragma once

#include "../../../assets/models/world/source/bellwether-stock-v1/geometry.generated.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace horde::gameplay::bellwether {

struct StockHeadPose {
    float yawRadians{};
    float pitchRadians{};
    bool operator==(const StockHeadPose&) const = default;
};

inline constexpr std::array<std::size_t, 3> kStockHeadKinds{0u, 1u, 1u};
using StockHeadPoses = std::array<StockHeadPose, kStockHeadKinds.size()>;

class BellwetherStockIdle {
public:
    void Step(float playerX, float playerZ, float seconds) noexcept {
        const float x = playerX - kBellwetherStockPigXZ[0];
        const float z = playerZ - kBellwetherStockPigXZ[1];
        const bool nearby = x*x + z*z < 12.f*12.f;
        strength_ = std::clamp(strength_ + (nearby ? seconds : -seconds), 0.f, 1.f);
        if (nearby || strength_ > 0.f) {
            elapsed_ += seconds;
            if (elapsed_ >= 24.f) elapsed_ -= 24.f;
        }
    }

    StockHeadPoses Poses() const noexcept {
        if (strength_ == 0.f) return {};
        const auto gesture = [this](float start, float duration) {
            const float position = (elapsed_ - start) / duration;
            if (position <= 0.f || position >= 1.f) return 0.f;
            const float wave = std::sin(3.14159265359f * position);
            return strength_ * wave * wave;
        };
        const float pigSniff = gesture(1.f, 2.8f) + .7f * gesture(13.7f, 2.4f);
        const float roostLook = gesture(4.8f, 1.1f) + gesture(17.4f, 1.3f);
        const float groundLook = gesture(7.4f, .8f) + .7f * gesture(8.4f, .62f) + gesture(21.f, 1.1f);
        return {{{.14f * pigSniff, .094f * pigSniff},
                 {-.34f * roostLook, .12f * roostLook},
                 {.27f * groundLook, -.23f * groundLook}}};
    }

private:
    float elapsed_{};
    float strength_{};
};

inline std::array<float, 12> StockHeadWorldFromModel(
    std::size_t head, const StockHeadPose& pose) noexcept {
    const auto& [x, y, z, restYaw, scale] = kBellwetherStockHeadMounts[head];
    const float yaw = restYaw + pose.yawRadians;
    const float c = scale * std::cos(yaw);
    const float s = scale * std::sin(yaw);
    const float cp = std::cos(pose.pitchRadians);
    const float sp = std::sin(pose.pitchRadians);
    return {c*cp, -c*sp, -s, x, scale*sp, scale*cp, 0.f, y, s*cp, -s*sp, c, z};
}

} // namespace horde::gameplay::bellwether
