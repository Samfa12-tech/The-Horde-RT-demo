#include "scene/assets/GltfDocument.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

namespace horde::scene::assets
{

namespace
{

std::uint32_t ReadU32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

bool ValidateGlbContainer(const std::filesystem::path& path, std::string& diagnostic)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        diagnostic = "Could not read static GLB: " + path.string();
        return false;
    }
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (bytes.size() < 12u)
    {
        diagnostic = "Static GLB chunks are malformed or truncated.";
        return false;
    }
    if (ReadU32(bytes, 0u) != 0x46546c67u)
    {
        diagnostic = "Static GLB header magic is not 'glTF'.";
        return false;
    }
    if (ReadU32(bytes, 4u) != 2u)
    {
        diagnostic = "Static GLB version must be 2.";
        return false;
    }
    if (ReadU32(bytes, 8u) != bytes.size())
    {
        diagnostic = "Static GLB chunks are malformed or truncated.";
        return false;
    }

    std::size_t cursor = 12u;
    std::size_t chunkIndex = 0u;
    while (cursor < bytes.size())
    {
        if (bytes.size() - cursor < 8u)
        {
            diagnostic = "Static GLB chunks are malformed or truncated.";
            return false;
        }
        const std::uint32_t chunkLength = ReadU32(bytes, cursor);
        const std::uint32_t chunkType = ReadU32(bytes, cursor + 4u);
        cursor += 8u;
        if ((chunkLength & 3u) != 0u || chunkLength > bytes.size() - cursor ||
            (chunkIndex == 0u && chunkType != 0x4e4f534au) ||
            (chunkIndex > 1u) || (chunkIndex == 1u && chunkType != 0x004e4942u))
        {
            diagnostic = "Static GLB chunks are malformed or truncated.";
            return false;
        }
        cursor += chunkLength;
        ++chunkIndex;
    }
    if (cursor != bytes.size() || chunkIndex == 0u)
    {
        diagnostic = "Static GLB chunks are malformed or truncated.";
        return false;
    }
    return true;
}

} // namespace

GltfDocument::~GltfDocument()
{
    if (data_ != nullptr) cgltf_free(data_);
}

GltfDocument::GltfDocument(GltfDocument&& other) noexcept : data_(std::exchange(other.data_, nullptr)) {}

GltfDocument& GltfDocument::operator=(GltfDocument&& other) noexcept
{
    if (this == &other) return *this;
    if (data_ != nullptr) cgltf_free(data_);
    data_ = std::exchange(other.data_, nullptr);
    return *this;
}

bool GltfDocument::Load(const std::filesystem::path& path,
                        GltfDocument& document,
                        std::string& diagnostic)
{
    if (!ValidateGlbContainer(path, diagnostic)) return false;

    cgltf_options options{};
    cgltf_data* data = nullptr;
    const std::string pathString = path.string();
    const cgltf_result parseResult = cgltf_parse_file(&options, pathString.c_str(), &data);
    if (parseResult != cgltf_result_success)
    {
        if (data != nullptr) cgltf_free(data);
        diagnostic = parseResult == cgltf_result_invalid_gltf
            ? "Static GLB contains an out-of-range accessor or index reference."
            : "Static GLB could not be parsed by the pinned cgltf reader.";
        return false;
    }
    const cgltf_result bufferResult = cgltf_load_buffers(&options, data, pathString.c_str());
    if (bufferResult != cgltf_result_success)
    {
        cgltf_free(data);
        diagnostic = "Static GLB buffer payload could not be loaded.";
        return false;
    }
    if (cgltf_validate(data) != cgltf_result_success)
    {
        cgltf_free(data);
        diagnostic = "Static GLB failed cgltf structural validation.";
        return false;
    }

    document = GltfDocument{};
    document.data_ = data;
    diagnostic.clear();
    return true;
}

} // namespace horde::scene::assets
