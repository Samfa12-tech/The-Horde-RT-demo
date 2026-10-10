#pragma once
#include <algorithm>

namespace horde::platform::windows {
struct VitalityHudLayout {
    int heartWidth, heartHeight, gap, insetX, insetY;
    int columns, rows, width, height;
};
inline int VitalityHudColumns(int width, int heartWidth, int gap, int insetX) {
    return std::max(1, (width - 2 * insetX + gap) / (heartWidth + gap));
}
inline VitalityHudLayout LayoutWindowsVitalityHud(int clientWidth, int maximum, int dpi) {
    const auto scale = [dpi](int value) { return (value * std::max(1, dpi) + 48) / 96; };
    VitalityHudLayout out{scale(18), scale(22), scale(4), scale(6), scale(4)};
    const int count = std::max(0, maximum);
    const int available = std::max(out.heartWidth, clientWidth - scale(40));
    out.columns = std::max(1, std::min(count, (available + out.gap) / (out.heartWidth + out.gap)));
    out.rows = count == 0 ? 1 : (count - 1) / out.columns + 1;
    // Scale each inset once, exactly as painting does. At 125% DPI,
    // Scale(12) is 15 but 2*Scale(6) is 16: the former wrapped the third heart
    // into an unallocated second row despite authoritative vitality being 3/3.
    out.width = out.columns * (out.heartWidth + out.gap) - out.gap + 2 * out.insetX;
    out.height = out.rows * (out.heartHeight + out.gap) - out.gap + 2 * out.insetY;
    return out;
}
} // namespace horde::platform::windows
