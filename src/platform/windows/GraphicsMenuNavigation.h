#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace horde::platform::windows
{

struct GraphicsMenuRect
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

enum class GraphicsMenuDirection
{
    Up,
    Down,
    Left,
    Right,
};

// Selects the nearest enabled/visible control in the requested spatial
// direction. Controls sharing the current row or column take priority. If
// none overlap on the cross axis, only nearby diagonal controls in the
// direction cone are considered.
inline std::optional<std::size_t> FindGraphicsMenuNeighbor(
    const std::vector<GraphicsMenuRect>& rectangles,
    const std::size_t currentIndex,
    const GraphicsMenuDirection direction)
{
    if (currentIndex >= rectangles.size() || rectangles.size() < 2u)
        return std::nullopt;

    const GraphicsMenuRect& current = rectangles[currentIndex];
    const long long currentX = static_cast<long long>(current.left) + current.right;
    const long long currentY = static_cast<long long>(current.top) + current.bottom;
    const bool vertical = direction == GraphicsMenuDirection::Up ||
                          direction == GraphicsMenuDirection::Down;
    const bool positive = direction == GraphicsMenuDirection::Down ||
                          direction == GraphicsMenuDirection::Right;

    std::optional<std::size_t> overlappingBest;
    long long overlappingDistanceSquared = 0;
    long long overlappingCross = 0;
    std::optional<std::size_t> diagonalBest;
    long long diagonalDistanceSquared = 0;
    long long diagonalCross = 0;
    for (std::size_t index = 0; index < rectangles.size(); ++index)
    {
        if (index == currentIndex) continue;
        const auto& candidate = rectangles[index];
        const long long candidateX = static_cast<long long>(candidate.left) + candidate.right;
        const long long candidateY = static_cast<long long>(candidate.top) + candidate.bottom;
        const long long signedPrimary = vertical ? candidateY - currentY : candidateX - currentX;
        if ((positive && signedPrimary <= 0) || (!positive && signedPrimary >= 0)) continue;

        const long long crossDelta = vertical ? candidateX - currentX : candidateY - currentY;
        const long long cross = crossDelta > 0 ? crossDelta : -crossDelta;
        const long long primary = signedPrimary > 0 ? signedPrimary : -signedPrimary;
        const long long distanceSquared = signedPrimary * signedPrimary + crossDelta * crossDelta;
        const bool overlapsCrossAxis = vertical
            ? current.left < candidate.right && candidate.left < current.right
            : current.top < candidate.bottom && candidate.top < current.bottom;

        if (overlapsCrossAxis)
        {
            if (!overlappingBest || distanceSquared < overlappingDistanceSquared ||
                (distanceSquared == overlappingDistanceSquared && cross < overlappingCross))
            {
                overlappingBest = index;
                overlappingDistanceSquared = distanceSquared;
                overlappingCross = cross;
            }
        }
        else if (cross <= primary &&
                 (!diagonalBest || distanceSquared < diagonalDistanceSquared ||
                  (distanceSquared == diagonalDistanceSquared && cross < diagonalCross)))
        {
            diagonalBest = index;
            diagonalDistanceSquared = distanceSquared;
            diagonalCross = cross;
        }
    }
    return overlappingBest ? overlappingBest : diagonalBest;
}

} // namespace horde::platform::windows
