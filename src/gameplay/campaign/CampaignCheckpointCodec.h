#pragma once

#include "gameplay/campaign/CampaignCheckpoint.h"
#include <cstddef>
#include <span>
#include <vector>

namespace horde::gameplay::campaign::codec {
inline constexpr std::size_t kMaxFileBytes = 4096;
enum class Error {
    None, UnsupportedFormat, UnsupportedSchema, UnsupportedContent,
    Corrupt, Malformed, TooLarge, InvalidCheckpoint
};
// Format 1: 8-byte magic "HRDCPKT\0", then LE u32 format version,
// payload byte count and IEEE CRC32 of the payload (20-byte envelope).
// The 142-byte payload explicitly encodes every Checkpoint logical value;
// integers are LE32, floats IEEE754 binary32, bools a single 0/1 byte.
// No padding, runtime identities, filesystem access or automatic migration.
// Failure leaves the caller's output unchanged; error is always assigned.
bool Encode(const Checkpoint& checkpoint, std::vector<std::byte>& out, Error& error);
bool Decode(std::span<const std::byte> bytes, Checkpoint& out, Error& error);
} // namespace horde::gameplay::campaign::codec
