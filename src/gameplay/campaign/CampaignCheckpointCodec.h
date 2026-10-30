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
// Schema1 keeps its exact 142-byte payload. Schema2 appends one canonical bool
// halfHeart (143 bytes). Schema3 retains that half-heart byte and appends one
// canonical upgraded bool and LE32 charge count (148 bytes). Schemas1/2 decode
// an empty base-capacity flask; no health/progress mutation or write-on-read.
// Schema4 appends LE64 UTC save-request seconds and a canonical resource-presence
// bool (157 bytes); zero time is unavailable. Dates cannot enable flask ownership.
// PlaySession dates a write once at request admission; reads never invent dates.
// Schema and payload length must agree. The payload encodes logical values;
// integers are LE32, floats IEEE754 binary32, bools a single 0/1 byte.
// No padding, runtime identities, filesystem access or migration-on-read.
// Failure leaves the caller's output unchanged; error is always assigned.
bool Encode(const Checkpoint& checkpoint, std::vector<std::byte>& out, Error& error);
bool Decode(std::span<const std::byte> bytes, Checkpoint& out, Error& error);
} // namespace horde::gameplay::campaign::codec
