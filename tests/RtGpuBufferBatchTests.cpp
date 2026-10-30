#include "vulkan/raytracing/RtGpuResources.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
using namespace horde::vulkan::raytracing;
template <class T> T Handle(std::uintptr_t value)
{
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
template <class T> std::size_t Id(T handle)
{
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<std::uintptr_t>(handle);
    else return static_cast<std::size_t>(handle);
}
enum class Failure { None, Create, Allocate, Bind, Map, NullMap, Address };
struct Allocation
{
    std::array<std::byte, 512u> bytes{};
    bool alive = false;
    bool mapped = false;
};
struct State
{
    std::array<Allocation, 32u> memory{};
    std::array<bool, 32u> buffers{};
    std::array<VkDeviceSize, 32u> sizes{};
    std::array<VkBufferUsageFlags, 32u> usages{};
    Failure failure = Failure::None;
    std::size_t failureRow = 0u;
    std::size_t row = 0u;
    std::size_t calls = 0u;
    std::size_t allocations = 0u;
    std::size_t frees = 0u;
    std::size_t destroys = 0u;
    std::size_t maps = 0u;
    std::size_t unmaps = 0u;
    bool lifecycleError = false;
} state;
bool Fail(Failure failure) { return state.failure == failure && state.row == state.failureRow; }
bool Require(bool ok, const std::string& message)
{
    if (!ok) std::cerr << "FAIL: " << message << '\n';
    return ok;
}
bool Empty(const RtGpuBuffer& b)
{
    return b.Empty();
}
bool Same(std::span<const RtGpuBuffer> a, std::span<const RtGpuBuffer> b)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0u; i < a.size(); ++i)
        if (a[i].buffer != b[i].buffer || a[i].memory != b[i].memory ||
            a[i].address != b[i].address || a[i].size != b[i].size ||
            a[i].allocationSize != b[i].allocationSize ||
            a[i].memoryPropertyFlags != b[i].memoryPropertyFlags ||
            a[i].mappedWriteData != b[i].mappedWriteData ||
            a[i].deviceAddressOffset != b[i].deviceAddressOffset) return false;
    return true;
}
bool NoLiveResources()
{
    for (const auto alive : state.buffers) if (alive) return false;
    for (const auto& memory : state.memory) if (memory.alive || memory.mapped) return false;
    return !state.lifecycleError;
}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL Address(VkDevice, const VkBufferDeviceAddressInfo* info)
{
    ++state.calls;
    return Fail(Failure::Address) ? 0u : 4096u * Id(info->buffer);
}
} // namespace

extern "C"
{
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice, VkPhysicalDeviceMemoryProperties* out)
{
    ++state.calls;
    *out = {};
    out->memoryTypeCount = 1u;
    out->memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice, const VkBufferCreateInfo* info, const VkAllocationCallbacks*, VkBuffer* out)
{
    ++state.calls;
    ++state.row;
    if (Fail(Failure::Create)) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    state.buffers[state.row] = true;
    state.sizes[state.row] = info->size;
    state.usages[state.row] = info->usage;
    *out = Handle<VkBuffer>(state.row);
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice, VkBuffer buffer, VkMemoryRequirements* out)
{
    ++state.calls;
    *out = {state.sizes[Id(buffer)] + 64u, 64u, 1u};
}
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory* out)
{
    ++state.calls;
    if (Fail(Failure::Allocate)) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    ++state.allocations;
    state.memory[state.row].alive = true;
    *out = Handle<VkDeviceMemory>(state.row);
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)
{
    ++state.calls;
    return Fail(Failure::Bind) ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags, void** out)
{
    ++state.calls;
    if (Fail(Failure::Map)) return VK_ERROR_MEMORY_MAP_FAILED;
    auto& allocation = state.memory[Id(memory)];
    state.lifecycleError |= !allocation.alive || allocation.mapped || offset + size > allocation.bytes.size();
    allocation.mapped = true;
    ++state.maps;
    *out = Fail(Failure::NullMap) ? nullptr : allocation.bytes.data() + offset;
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice, VkDeviceMemory memory)
{
    ++state.calls;
    auto& allocation = state.memory[Id(memory)];
    state.lifecycleError |= !allocation.alive || !allocation.mapped;
    allocation.mapped = false;
    ++state.unmaps;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice, VkBuffer buffer, const VkAllocationCallbacks*)
{
    ++state.calls;
    state.lifecycleError |= !state.buffers[Id(buffer)];
    state.buffers[Id(buffer)] = false;
    ++state.destroys;
}
VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice, VkDeviceMemory memory, const VkAllocationCallbacks*)
{
    ++state.calls;
    auto& allocation = state.memory[Id(memory)];
    state.lifecycleError |= !allocation.alive || allocation.mapped;
    allocation.alive = false;
    ++state.frees;
}
} // extern C

int main()
{
    RtGpuResources resources;
    resources.Bind(Handle<VkPhysicalDevice>(1u), Handle<VkDevice>(2u), nullptr, Address);
    const std::array<std::uint8_t, 11u> payload{{3, 8, 13, 21, 34, 55, 89, 144, 233, 1, 2}};
    std::array<RtGpuBufferUpload, RtGpuResources::kBufferUploadBatchCapacity> uploads{};
    for (std::size_t i = 0u; i < uploads.size(); ++i)
        uploads[i] = {payload.data(), payload.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      true, (i % 2u) == 0u, "candidate row", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    std::array<RtGpuBuffer, uploads.size()> output{};
    std::string diagnostic;
    bool ok = true;

    // The chapter can upload across render turns without publishing partial
    // buffers. A byte budget bounds copies; one turn never allocates two rows.
    state = {};
    {
        RtGpuBufferPreparation preparation(resources);
        ok &= Require(preparation.Begin(uploads, diagnostic) && state.calls == 0u,
            "begin borrows a validated batch without Vulkan allocation");
        if (preparation.Status() == RtGpuBufferPreparation::State::Preparing)
        {
            const auto calls = state.calls;
            ok &= Require(!preparation.Begin(uploads, diagnostic) && state.calls == calls &&
                preparation.Status() == RtGpuBufferPreparation::State::Preparing,
                "second begin cannot replace an owned preparation");
            ok &= Require(!preparation.Advance(0u, diagnostic) && state.calls == calls,
                "zero copy budget preserves preparation without Vulkan calls");
            unsigned turns = 0u;
            while (preparation.Status() == RtGpuBufferPreparation::State::Preparing && turns++ < 100u)
            {
                const auto rows = state.row;
                const auto bytes = preparation.UploadedBytes();
                ok &= Require(preparation.Advance(3u, diagnostic) && state.row <= rows + 1u &&
                    preparation.LastAdvanceCopiedBytes() <= 3u &&
                    preparation.UploadedBytes() == bytes + preparation.LastAdvanceCopiedBytes(),
                    "one allocation and at most three exact copied bytes per turn");
                horde::telemetry::RtResourceInventory inventory;
                preparation.AccumulateResourceInventory(inventory);
                ok &= Require(inventory.bufferCount == state.row &&
                    inventory.memoryAllocationCount == state.row &&
                    inventory.deviceLocalBytes == state.row * (payload.size() + 64u) &&
                    inventory.hostVisibleBytes == inventory.deviceLocalBytes,
                    "partial upload allocations remain visible in actual resource inventory");
                for (const auto& buffer : output) ok &= Require(Empty(buffer), "no partial publication");
            }
            ok &= Require(turns == 32u && preparation.CompletedRows() == uploads.size() &&
                preparation.Status() == RtGpuBufferPreparation::State::Ready &&
                preparation.UploadedBytes() == uploads.size() * payload.size(),
                "all eight rows complete over exactly thirty-two turns");
            ok &= Require(!preparation.Take(std::span(output).first(7u), diagnostic),
                "wrong destination length retains the complete candidate");
            output[7].size = 1u;
            ok &= Require(!preparation.Take(output, diagnostic) && output[7].size == 1u,
                "admitted destination rejects atomic take");
            output = {};
            RtGpuBufferPreparation moved(std::move(preparation));
            preparation.Cancel();
            ok &= Require(moved.Take(output, diagnostic) &&
                moved.Status() == RtGpuBufferPreparation::State::Idle,
                "moved preparation transfers completed ownership exactly once");
            for (std::size_t i = 0u; i < output.size(); ++i)
            {
                ok &= Require(std::memcmp(state.memory[Id(output[i].memory)].bytes.data(),
                    payload.data(), payload.size()) == 0 &&
                    (output[i].mappedWriteData != nullptr) == uploads[i].retainMapping,
                    "incremental payload and final mapping exactly match synchronous route");
                resources.DestroyBuffer(output[i]);
            }
        }
    }
    ok &= Require(NoLiveResources(), "incremental success has no lost allocations");

    state = {};
    {
        RtGpuBufferPreparation first(resources), second(resources);
        ok &= Require(first.Begin(uploads, diagnostic) && first.Advance(3u, diagnostic) &&
            second.Begin(uploads, diagnostic) && second.Advance(3u, diagnostic), "two isolated partial owners");
        second = std::move(first);
        second = std::move(second);
        first.Cancel();
        ok &= Require(state.frees == 1u && second.UploadedBytes() == 3u &&
            second.Advance(3u, diagnostic) && second.UploadedBytes() == 6u,
            "move assignment releases destination, preserves source cursor and tolerates self move");
    }
    ok &= Require(NoLiveResources() && state.maps == state.unmaps && state.allocations == state.frees,
        "partial move and destruction release each allocation once");

    // Cancellation at every copy boundary and every Vulkan failure retains the
    // separately admitted sentinel, including success-null-map cleanup.
    for (unsigned cancelTurn = 0u; cancelTurn <= 32u; ++cancelTurn)
    {
        state = {};
        {
            RtGpuBufferPreparation preparation(resources);
            ok &= Require(preparation.Begin(uploads, diagnostic), "cancellable begin");
            for (unsigned turn = 0u; turn < cancelTurn; ++turn)
                ok &= Require(preparation.Advance(3u, diagnostic), "cancellable bounded copy");
            preparation.Cancel(); preparation.Cancel();
        }
        ok &= Require(NoLiveResources() && state.maps == state.unmaps && state.allocations == state.frees,
            "cancel/destruction at every boundary releases exactly its candidate");
    }
    for (auto failure : {Failure::Create, Failure::Allocate, Failure::Bind,
                         Failure::Map, Failure::NullMap, Failure::Address})
        for (std::size_t row = 1u; row <= uploads.size(); ++row)
        {
            state = {}; state.failure = failure; state.failureRow = row;
            state.memory[31].alive = true; state.buffers[31] = true;
            state.memory[31].bytes.fill(std::byte{0x5a});
            const auto sentinel = state.memory[31].bytes;
            {
                RtGpuBufferPreparation preparation(resources);
                ok &= Require(preparation.Begin(uploads, diagnostic), "failure injection begins");
                while (preparation.Status() == RtGpuBufferPreparation::State::Preparing)
                    if (!preparation.Advance(3u, diagnostic)) break;
                ok &= Require(preparation.Status() == RtGpuBufferPreparation::State::Failed &&
                    !preparation.Take(output, diagnostic) && !diagnostic.empty() &&
                    state.row == row && state.memory[31].alive && state.buffers[31] &&
                    state.memory[31].bytes == sentinel,
                    "incremental driver failure cleans only candidate and preserves admitted owner");
            }
            state.memory[31].alive = false; state.buffers[31] = false;
            ok &= Require(NoLiveResources() && state.maps == state.unmaps && state.allocations == state.frees,
                "incremental Vulkan failure has no mapped or leaked resources");
        }

    // The entire candidate is isolated until every row has allocated/uploaded.
    state = {};
    ok &= Require(resources.PrepareBufferUploads(uploads, output, diagnostic), "complete batch admission");
    if (!Empty(output.front()))
    {
        for (std::size_t i = 0u; i < output.size(); ++i)
        {
            const auto& buffer = output[i];
            const auto& memory = state.memory[Id(buffer.memory)];
            ok &= Require(buffer.size == payload.size() && buffer.allocationSize == payload.size() + 64u &&
                buffer.address == 4096u * Id(buffer.buffer) &&
                buffer.memoryPropertyFlags == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
                (buffer.mappedWriteData != nullptr) == uploads[i].retainMapping &&
                memory.mapped == uploads[i].retainMapping &&
                std::memcmp(memory.bytes.data(), payload.data(), payload.size()) == 0,
                "exact row payload, address, allocation flags and retained mapping");
        }
        const auto saved = output;
        const auto calls = state.calls;
        ok &= Require(!resources.PrepareBufferUploads(uploads, output, diagnostic) &&
            Same(saved, output) && state.calls == calls,
            "live destination rejects replacement without mutation or Vulkan work");
        auto moved = std::exchange(output, {});
        for (auto& buffer : output) resources.DestroyBuffer(buffer);
        ok &= Require(state.frees == 0u, "moved-from destination owns nothing");
        for (auto& buffer : moved) resources.DestroyBuffer(buffer);
        for (auto& buffer : moved) resources.DestroyBuffer(buffer);
        ok &= Require(NoLiveResources() && state.allocations == state.frees &&
            state.destroys == uploads.size() && state.maps == state.unmaps,
            "moved candidate unmaps/destroys/frees each allocation exactly once");
    }

    // Non-addressed descriptor rows and a shorter content profile share the
    // same preparation route; no dummy rows or mappings are retained.
    state = {};
    output = {};
    auto mixed = uploads;
    for (std::size_t i = 0u; i < 6u; ++i) mixed[i].deviceAddress = (i % 2u) != 0u;
    ok &= Require(resources.PrepareBufferUploads(std::span(mixed).first(6u),
        std::span(output).first(6u), diagnostic), "six-row mixed metadata/geometry batch");
    for (std::size_t i = 0u; i < 6u; ++i)
    {
        const auto& buffer = output[i];
        ok &= Require((buffer.address != 0u) == mixed[i].deviceAddress &&
            ((state.usages[Id(buffer.buffer)] & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0u) ==
                mixed[i].deviceAddress,
            "device address use must match each metadata or geometry row");
        resources.DestroyBuffer(output[i]);
    }
    ok &= Require(Empty(output[6]) && Empty(output[7]) && state.allocations == 6u && NoLiveResources(),
        "short batch leaves unused destinations empty and owns exactly six allocations");

    // Every row and each distinct Vulkan failure boundary, with a separately
    // admitted sentinel allocation that must remain byte-identical and live.
    for (auto failure : {Failure::Create, Failure::Allocate, Failure::Bind,
                         Failure::Map, Failure::NullMap, Failure::Address})
        for (std::size_t row = 1u; row <= uploads.size(); ++row)
        {
            state = {};
            state.failure = failure;
            state.failureRow = row;
            state.memory[31].alive = true;
            state.memory[31].bytes.fill(std::byte{0x5a});
            state.buffers[31] = true;
            output = {};
            const auto sentinel = state.memory[31].bytes;
            ok &= Require(!resources.PrepareBufferUploads(uploads, output, diagnostic) && !diagnostic.empty(),
                "injected candidate failure must reject at row " + std::to_string(row));
            for (const auto& buffer : output) ok &= Require(Empty(buffer), "no partially published buffer");
            ok &= Require(state.row == row && state.memory[31].alive && state.buffers[31] &&
                state.memory[31].bytes == sentinel && !state.memory[31].mapped,
                "failure stops immediately without touching admitted sentinel");
            state.memory[31].alive = false;
            state.buffers[31] = false;
            ok &= Require(NoLiveResources() && state.allocations == state.frees && state.maps == state.unmaps,
                "candidate failure cleanup has no live, mapped, duplicate or lost allocation");
        }

    const auto Reject = [&](std::span<const RtGpuBufferUpload> input, std::span<RtGpuBuffer> destination) {
        state = {};
        const auto before = output;
        const bool rejected = !resources.PrepareBufferUploads(input, destination, diagnostic);
        return Require(rejected && !diagnostic.empty() && state.calls == 0u &&
            Same(before, output),
            "invalid batch preflight must preserve output without Vulkan calls");
    };
    output = {};
    ok &= Reject({}, output);
    ok &= Reject(uploads, std::span(output).first(output.size() - 1u));
    std::array<RtGpuBufferUpload, 9u> oversized{};
    ok &= Reject(oversized, output);
    for (std::size_t row = 0u; row < uploads.size(); ++row)
        for (int defect = 0; defect < 5; ++defect)
        {
            auto bad = uploads;
            if (defect == 0) bad[row].data = nullptr;
            if (defect == 1) bad[row].size = 0u;
            if (defect == 2) bad[row].label = nullptr;
            if (defect == 3) bad[row].usage = 0u;
            if (defect == 4) bad[row].label = "";
            ok &= Reject(bad, output);
            RtGpuBufferPreparation preparation(resources);
            state = {};
            ok &= Require(!preparation.Begin(bad, diagnostic) && !diagnostic.empty() && state.calls == 0u &&
                preparation.Status() == RtGpuBufferPreparation::State::Idle,
                "invalid incremental payload rejects before any allocation or cursor mutation");
        }
    for (int field = 0; field < 8; ++field)
    {
        output = {};
        auto& dirty = output[7];
        if (field == 0) dirty.buffer = Handle<VkBuffer>(31u);
        if (field == 1) dirty.memory = Handle<VkDeviceMemory>(31u);
        if (field == 2) dirty.address = 1u;
        if (field == 3) dirty.size = 1u;
        if (field == 4) dirty.allocationSize = 1u;
        if (field == 5) dirty.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (field == 6) dirty.mappedWriteData = state.memory[31].bytes.data();
        if (field == 7) dirty.deviceAddressOffset = 1u;
        ok &= Reject(uploads, output);
    }
    output = {};
    resources.Reset();
    ok &= Reject(uploads, output);
    RtGpuBufferPreparation unbound(resources);
    ok &= Require(!unbound.Begin(uploads, diagnostic) && state.calls == 0u, "unbound incremental context rejects");
    if (ok) std::cout << "GPU buffer batch: exact synchronous/incremental uploads, 96 Vulkan failures, bounded copies, preflight, cancellation, move and cleanup passed.\n";
    return ok ? 0 : 1;
}
