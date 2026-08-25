#pragma once

#include <filesystem>
#include <string>

struct cgltf_data;

namespace horde::scene::assets
{

class GltfDocument
{
public:
    GltfDocument() = default;
    ~GltfDocument();
    GltfDocument(const GltfDocument&) = delete;
    GltfDocument& operator=(const GltfDocument&) = delete;
    GltfDocument(GltfDocument&& other) noexcept;
    GltfDocument& operator=(GltfDocument&& other) noexcept;

    static bool Load(const std::filesystem::path& path,
                     GltfDocument& document,
                     std::string& diagnostic);

    const cgltf_data* Data() const { return data_; }

private:
    cgltf_data* data_ = nullptr;
};

} // namespace horde::scene::assets
