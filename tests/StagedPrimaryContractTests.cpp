#include "vulkan/raytracing/experimental/StagedPrimaryContract.h"

#include <bit>
#include <iostream>

using namespace horde::vulkan::raytracing::experimental;
int main()
{
    const auto fail = [](const char* label) { std::cerr << label << '\n'; return 1; };
    const auto phone = TryMakeStagedPrimaryExtent(1080u, 2235u, 128u * 1024u * 1024u);
    if (!phone || phone->pixelCount != 2413800u || phone->pageCapacity != 804600u ||
        phone->pageBytes != 102988800u || phone->logicalBytes != 308966400u ||
        phone->logicalReadWriteBytes != 617932800u)
        return fail("75% phone allocation arithmetic");
    if (TryMakeStagedPrimaryExtent(0u, 1u, ~0ull) ||
        TryMakeStagedPrimaryExtent(65536u, 65536u, ~0ull) ||
        TryMakeStagedPrimaryExtent(1080u, 2235u, phone->pageBytes - 1u))
        return fail("zero/uint overflow/device-range admission");
    for (const auto pixels : {1u, 2u, 3u, 4u, 10u, 2413800u}) {
        const auto e = TryMakeStagedPrimaryExtent(pixels, 1u, ~0ull);
        if (!e || e->paddedBufferBytes < e->logicalBytes ||
            e->paddedBufferBytes - e->logicalBytes >= 3u * kStagedRecordBytes)
            return fail("page padding");
        for (const auto index : {0u, pixels - 1u})
            if (index / e->pageCapacity >= kStagedPageCount ||
                (index % e->pageCapacity) * kStagedRecordBytes >= e->pageBytes)
                return fail("page address bounds");
    }
    for (const auto instance : {0u, 19u, 20u, kStagedInstanceMask}) {
        for (const bool guarded : {false, true}) {
            const auto packed = PackStagedInstance(instance, guarded);
            if (!packed || (*packed & kStagedInstanceMask) != instance ||
                ((*packed & kStagedGuardBit) != 0u) != guarded)
                return fail("instance/guard packing");
        }
    }
    if (PackStagedInstance(0x01000000u, false) || PackStagedInstance(kStagedGuardBit, true))
        return fail("non-custom-index rejection");
    // No arithmetic or float cast is permitted for FP32 fields in the codec.
    for (const auto bits : {0u, 0x80000000u, 0x3f800000u, 0x00000001u, 0x7f800000u, 0x7fc01234u})
        if (std::bit_cast<std::uint32_t>(std::bit_cast<float>(bits)) != bits)
            return fail("bit-preserving float representation");
    for (const auto primitive : {-1, 0, 1, 123456}) {
        const auto bits = std::bit_cast<std::uint32_t>(primitive);
        if (std::bit_cast<std::int32_t>(bits) != primitive ||
            (std::bit_cast<std::int32_t>(bits) >= 0) != (primitive != -1))
            return fail("miss sentinel");
    }
    std::cout << "PASS staged primary extent/packing contracts (not GPU/image acceptance)\n";
    return 0;
}
