#pragma once
#include "graphics/RtPresentationTransform.h"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>

namespace horde::telemetry
{
// Command-record facts, retained with the submission and admitted only on its
// completion. Camera aspect is derived exactly as rt_frame.glsl from the actual
// storage/dispatch image and packed presentation transform, not a UI request.
struct RtFrameDimensionsEvidence
{
    std::uint32_t internalWidth = 0, internalHeight = 0;
    std::uint32_t outputWidth = 0, outputHeight = 0;
    std::uint32_t presentationTransform = 0;
    float cameraAspect = 0;
    bool dynamicResolutionEnabled = false;
    bool linearBlit = false;
    bool operator==(const RtFrameDimensionsEvidence&) const = default;
};
inline float RtRecordedCameraAspect(const std::uint32_t width, const std::uint32_t height,
                                     const std::uint32_t transform) noexcept
{
    if (!width || !height || transform > 7) return 0;
    return graphics::RtPresentationCameraAspect(width, height,
        graphics::RtPresentationTransformFromIndex(transform));
}
inline bool ValidRtFrameDimensionsEvidence(const RtFrameDimensionsEvidence& d) noexcept
{
    return d.internalWidth && d.internalHeight && d.outputWidth && d.outputHeight &&
        d.internalWidth <= d.outputWidth && d.internalHeight <= d.outputHeight &&
        d.outputWidth <= 16384 && d.outputHeight <= 16384 && d.presentationTransform <= 7 &&
        std::isfinite(d.cameraAspect) && d.cameraAspect > 0 &&
        std::abs(d.cameraAspect - RtRecordedCameraAspect(d.internalWidth, d.internalHeight,
            d.presentationTransform)) <= 1e-5f &&
        d.linearBlit == (d.internalWidth != d.outputWidth || d.internalHeight != d.outputHeight);
}
inline bool RtRecordedAspectPreserved(const RtFrameDimensionsEvidence& d) noexcept
{
    return std::uint64_t(d.internalWidth) * d.outputHeight ==
        std::uint64_t(d.internalHeight) * d.outputWidth;
}
inline void WriteRtFrameDimensionsEvidenceJson(std::ostream& out, const RtFrameDimensionsEvidence& d)
{
    const auto flags = out.flags(); const auto precision = out.precision();
    out << std::defaultfloat << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "{\"internalExtent\":[" << d.internalWidth << ',' << d.internalHeight <<
        "],\"outputExtent\":[" << d.outputWidth << ',' << d.outputHeight <<
        "],\"cameraAspect\":" << d.cameraAspect << ",\"presentationTransform\":" << d.presentationTransform <<
        ",\"dynamicResolutionEnabled\":" << (d.dynamicResolutionEnabled ? "true" : "false") <<
        ",\"linearBlit\":" << (d.linearBlit ? "true" : "false") <<
        ",\"aspectPreserved\":" << (RtRecordedAspectPreserved(d) ? "true" : "false") << '}';
    out.flags(flags); out.precision(precision);
}
}
