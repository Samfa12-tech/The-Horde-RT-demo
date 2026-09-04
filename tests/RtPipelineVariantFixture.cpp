#include "vulkan/raytracing/RtPipelineVariantProvider.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    using namespace horde::vulkan::raytracing;
    const auto& provider = RtPipelineVariantProvider::Compiled();
    std::uint64_t checksum = 0;
    std::size_t artifactIndex = 0;
    for (const auto material : {RtMaterialStrategy::OpaqueFast, RtMaterialStrategy::GenericDielectric}) {
        const auto artifact = provider.ResolveExact({provider.request().instrumentation, provider.request().quality, material});
        if (!artifact || artifact->words.empty() || artifact->words.front() != 0x07230203u) {
            std::cerr << "selected provider fixture did not ODR-use both frozen streams\n";
            return 1;
        }
        for (const std::uint32_t word : artifact->words) { checksum += word; }
        if (argc == 2) {
            const auto path = std::filesystem::path(argv[1]) /
                (artifactIndex == 0 ? "opaque.spv" : "generic.spv");
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output) { return 1; }
            output.write(reinterpret_cast<const char*>(artifact->words.data()),
                         static_cast<std::streamsize>(artifact->words.size_bytes()));
            if (!output) { return 1; }
        }
        ++artifactIndex;
    }
    return checksum == 0 ? 1 : 0;
}
