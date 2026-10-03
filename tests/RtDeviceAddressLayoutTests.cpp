#include "vulkan/raytracing/RtDeviceAddressLayout.h"

#include <cstdint>
#include <iostream>
#include <limits>

using namespace horde::vulkan::raytracing;

int main()
{
    bool ok = true;
    const auto require = [&](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ok = false; }
    };
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t padded = 123u;
    std::uint64_t offset = 123u;
    require(!TryPaddedDeviceAddressSize(0u, 256u, padded) && padded == 0u,
            "empty scratch size must fail closed");
    require(!TryPaddedDeviceAddressSize(1u, 0u, padded) &&
            !TryPaddedDeviceAddressSize(1u, 3u, padded), "invalid alignments rejected");
    require(!TryPaddedDeviceAddressSize(max, 2u, padded) && padded == 0u,
            "padding overflow rejected");
    require(TryPaddedDeviceAddressSize(max - 255u, 256u, padded) && padded == max,
            "maximum representable padded size accepted");
    require(!TryAlignDeviceAddressRange(0u, 512u, 256u, 256u, offset) && offset == 0u,
            "null device address rejected");
    require(!TryAlignDeviceAddressRange(max - 15u, 17u, 1u, 1u, offset),
            "address range wrapping past uint64 rejected");
    require(!TryAlignDeviceAddressRange(max, 1u, 1u, 256u, offset),
            "alignment wrap rejected");
    require(TryAlignDeviceAddressRange(max, 1u, 1u, 1u, offset) && offset == 0u,
            "last representable byte accepted without exclusive-end overflow");
    require(!TryAlignDeviceAddressRange(4097u, 255u, 1u, 256u, offset),
            "padding cannot leave requested range outside the buffer");
    require(!TryAlignDeviceAddressRange(4097u, 511u, 257u, 256u, offset),
            "one-byte scratch overrun rejected");
    require(TryAlignDeviceAddressRange(4097u, 511u, 256u, 256u, offset) && offset == 255u,
            "worst-case alignment padding preserves exact requested tail");
    for (std::uint64_t alignment = 1u; alignment <= 4096u; alignment *= 2u)
    {
        require(TryPaddedDeviceAddressSize(733u, alignment, padded), "valid padded size");
        for (std::uint64_t residue = 0u; residue < alignment; ++residue)
        {
            const auto base = 8192u + residue;
            require(TryAlignDeviceAddressRange(base, padded, 733u, alignment, offset) &&
                    (base + offset) % alignment == 0u && offset < alignment &&
                    offset + 733u <= padded,
                    "every raw base residue yields aligned scratch wholly inside its buffer");
        }
    }

    RtShaderBindingTableLayout layout{};
    require(TryShaderBindingTableLayout(32u, 32u, 64u, 4096u, layout) &&
            layout.stride == 32u && layout.regionSpacing == 64u && layout.size == 192u,
            "existing three-region SBT layout remains unchanged");
    require(TryShaderBindingTableLayout(33u, 32u, 128u, 64u, layout) &&
            layout.stride == 64u && layout.regionSpacing == 128u && layout.size == 384u,
            "odd handle size rounds stride and region spacing independently");
    require(!TryShaderBindingTableLayout(65u, 32u, 128u, 64u, layout) && layout.size == 0u,
            "SBT maximum stride enforced");
    require(!TryShaderBindingTableLayout(0u, 32u, 64u, 4096u, layout) &&
            !TryShaderBindingTableLayout(32u, 0u, 64u, 4096u, layout) &&
            !TryShaderBindingTableLayout(32u, 32u, 0u, 4096u, layout) &&
            !TryShaderBindingTableLayout(32u, 3u, 64u, 4096u, layout),
            "malformed SBT properties rejected");
    require(!TryShaderBindingTableLayout(UINT32_MAX, 256u, 64u, UINT32_MAX, layout),
            "32-bit aligned stride overflow cannot wrap into a supported value");
    require(TryShaderBindingTableLayout(32u, 32u, 256u, 4096u, layout) &&
            TryPaddedDeviceAddressSize(layout.size, 256u, padded) &&
            TryAlignDeviceAddressRange(4097u, padded, layout.size, 256u, offset),
            "SBT base address can be padded independently of memory alignment");
    for (std::uint64_t group = 0u; group < 3u; ++group)
    {
        require((4097u + offset + group * layout.regionSpacing) % 256u == 0u &&
                offset + group * layout.regionSpacing + layout.stride <= padded,
                "each SBT region base aligns and each single record fits");
    }
    return ok ? 0 : 1;
}
