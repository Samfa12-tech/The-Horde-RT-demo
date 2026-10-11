#pragma once
#include "scene/RescueBlockoutGeometry.h"

namespace horde::scene {
// Original, static relief within the accepted moving lid's collision envelope.
inline constexpr auto RescueLidFinishBoxes() {
    return std::array<RescueBlockoutBox,6>{{
        {{{-34.94f,2.05f,-16.64f}},{{-32.46f,2.16f,-13.76f}},0u},
        {{{-34.94f,2.16f,-16.64f}},{{-32.46f,2.19f,-16.52f}},0u},
        {{{-34.94f,2.16f,-13.88f}},{{-32.46f,2.19f,-13.76f}},0u},
        {{{-34.94f,2.16f,-16.52f}},{{-34.82f,2.19f,-13.88f}},0u},
        {{{-32.58f,2.16f,-16.52f}},{{-32.46f,2.19f,-13.88f}},0u},
        {{{-34.18f,2.16f,-15.88f}},{{-33.22f,2.177f,-14.52f}},0u},
    }};
}
}
