#pragma once
#include "scene/atmosphere/IndoorDust.h"
namespace horde::scene {
// Level data only. Two small dry subvolumes, inset from masonry/wetline.
// Primary-only prototype: do not expand into mirror/pane/water sightlines.
inline constexpr std::array<atmosphere::IndoorDustZone,2> kShowcaseIndoorDust{{
    {101,0x48a123u,{-0.62f,0.1f,-2.9f},{0.65f,1.6f,-1.3f},
        {0.005f,0.008f,-0.004f},0.12f,0.008f,7.0f,atmosphere::DustZoneShape::Box},
    {102,0x174bbdu,{-6.6f,0.15f,-16.4f},{-4.4f,1.7f,-14.1f},
        {-0.006f,0.010f,0.003f},0.105f,0.0075f,7.0f,atmosphere::DustZoneShape::Ellipsoid}
}};
inline constexpr std::array<atmosphere::IndoorDustZone,1> kPreviewIndoorDust{{
    {103,0x33cd1u,{-4.95f,0.15f,-14.6f},{-4.15f,1.3f,-13.6f},
        {0.005f,0.009f,0.003f},0.105f,0.008f,6.0f,atmosphere::DustZoneShape::Box}
}};
}
