#include "vulkan/PipelineCacheSeed.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

using horde::vulkan::PipelineCacheSeed;

namespace
{
using Device = PipelineCacheSeed::DeviceIdentity;

void WriteLittleEndianU32(std::vector<std::byte>& bytes, const std::size_t offset,
                          const std::uint32_t value)
{
    for (std::size_t index = 0u; index < 4u; ++index)
        bytes[offset + index] = static_cast<std::byte>((value >> (index * 8u)) & 0xffu);
}

Device MakeDevice()
{
    Device device{};
    device.vendorId = 0x10deu;
    device.deviceId = 0x2684u;
    for (std::size_t index = 0u; index < device.pipelineCacheUuid.size(); ++index)
        device.pipelineCacheUuid[index] = static_cast<std::uint8_t>(index * 7u + 3u);
    return device;
}

std::vector<std::byte> MakeBlob(const Device& device, const std::size_t size = 47u)
{
    std::vector<std::byte> bytes(size);
    WriteLittleEndianU32(bytes, 0u, static_cast<std::uint32_t>(PipelineCacheSeed::kHeaderSize));
    WriteLittleEndianU32(bytes, 4u, PipelineCacheSeed::kHeaderVersionOne);
    WriteLittleEndianU32(bytes, 8u, device.vendorId);
    WriteLittleEndianU32(bytes, 12u, device.deviceId);
    for (std::size_t index = 0u; index < device.pipelineCacheUuid.size(); ++index)
        bytes[16u + index] = static_cast<std::byte>(device.pipelineCacheUuid[index]);
    for (std::size_t index = PipelineCacheSeed::kHeaderSize; index < bytes.size(); ++index)
        bytes[index] = static_cast<std::byte>((index * 13u) & 0xffu);
    return bytes;
}

bool SameBytes(const std::span<const std::byte> actual,
               const std::vector<std::byte>& expected)
{
    return actual.size() == expected.size() &&
           std::equal(actual.begin(), actual.end(), expected.begin());
}
} // namespace

int main()
{
    bool ok = true;
    const auto require = [&](const bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ok = false; }
    };

    const Device device = MakeDevice();
    const auto valid = MakeBlob(device);
    PipelineCacheSeed seed;
    require(seed.Empty() && seed.Size() == 0u && seed.ForDevice(device).empty(),
            "cold seed starts empty and returns no bytes");
    require(seed.ReplaceFromDriverData(device, valid, true, false),
            "complete successful driver data is admitted");
    require(!seed.Empty() && seed.Size() == valid.size() && SameBytes(seed.ForDevice(device), valid),
            "round-trip retains the exact opaque bytes");

    auto other = device;
    ++other.vendorId;
    require(seed.ForDevice(other).empty(), "a different device identity receives an empty view");
    other = device;
    ++other.deviceId;
    require(seed.ForDevice(other).empty(), "device ID mismatch receives an empty view");
    other = device;
    other.pipelineCacheUuid[9] ^= 0x80u;
    require(seed.ForDevice(other).empty(), "pipeline cache UUID mismatch receives an empty view");

    auto headerOnly = MakeBlob(device, PipelineCacheSeed::kHeaderSize);
    require(seed.ReplaceFromDriverData(device, headerOnly, true, false) &&
            SameBytes(seed.ForDevice(device), headerOnly),
            "the exact 32-byte minimum header is admitted");
    auto maximum = MakeBlob(device, PipelineCacheSeed::kMaximumSize);
    require(seed.ReplaceFromDriverData(device, maximum, true, false) &&
            seed.Size() == PipelineCacheSeed::kMaximumSize && SameBytes(seed.ForDevice(device), maximum),
            "the exact 16 MiB upper bound is admitted");

    const auto retainsMaximum = [&] {
        return seed.Size() == maximum.size() && SameBytes(seed.ForDevice(device), maximum);
    };
    auto tooShort = MakeBlob(device);
    tooShort.resize(PipelineCacheSeed::kHeaderSize - 1u);
    require(!seed.ReplaceFromDriverData(device, tooShort, true, false) && retainsMaximum(),
            "truncated header is rejected without replacing the current seed");
    auto tooLarge = MakeBlob(device, PipelineCacheSeed::kMaximumSize + 1u);
    require(!seed.ReplaceFromDriverData(device, tooLarge, true, false) && retainsMaximum(),
            "oversized data is rejected without replacing the current seed");

    auto malformed = MakeBlob(device);
    WriteLittleEndianU32(malformed, 0u, 0x20000000u);
    require(!seed.ReplaceFromDriverData(device, malformed, true, false) && retainsMaximum(),
            "non-little-endian header size is rejected");
    malformed = MakeBlob(device);
    WriteLittleEndianU32(malformed, 0u, 31u);
    require(!seed.ReplaceFromDriverData(device, malformed, true, false) && retainsMaximum(),
            "header size other than 32 is rejected");
    malformed = MakeBlob(device);
    WriteLittleEndianU32(malformed, 4u, 2u);
    require(!seed.ReplaceFromDriverData(device, malformed, true, false) && retainsMaximum(),
            "unsupported cache header version is rejected");
    malformed = MakeBlob(device);
    WriteLittleEndianU32(malformed, 8u, device.vendorId + 1u);
    require(!seed.ReplaceFromDriverData(device, malformed, true, false) && retainsMaximum(),
            "header vendor mismatch is rejected");
    malformed = MakeBlob(device);
    WriteLittleEndianU32(malformed, 12u, device.deviceId + 1u);
    require(!seed.ReplaceFromDriverData(device, malformed, true, false) && retainsMaximum(),
            "header device mismatch is rejected");
    malformed = MakeBlob(device);
    malformed[16u + 4u] ^= std::byte{0x01};
    require(!seed.ReplaceFromDriverData(device, malformed, true, false) && retainsMaximum(),
            "header UUID mismatch is rejected");

    const auto smallReplacement = MakeBlob(device, 39u);
    require(!seed.ReplaceFromDriverData(device, smallReplacement, false, false) && retainsMaximum(),
            "failed retrieval cannot replace the previous seed");
    require(!seed.ReplaceFromDriverData(device, smallReplacement, true, true) && retainsMaximum(),
            "incomplete retrieval cannot replace the previous seed");
    require(seed.ReplaceFromDriverData(device, smallReplacement, true, false) &&
            SameBytes(seed.ForDevice(device), smallReplacement),
            "a valid complete candidate atomically replaces the old seed");

    seed.Clear();
    require(seed.Empty() && seed.Size() == 0u && seed.ForDevice(device).empty(),
            "explicit clear removes the seed");

    return ok ? 0 : 1;
}
