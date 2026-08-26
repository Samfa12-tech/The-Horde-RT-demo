#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <regex>
#include <string_view>

namespace horde::scene::assets
{

namespace
{

bool ReadFile(const std::filesystem::path& path, std::string& text)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    text.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

bool FindDelimited(std::string_view text,
                   std::string_view key,
                   char open,
                   char close,
                   std::string_view& section)
{
    const std::string quotedKey = "\"" + std::string(key) + "\"";
    const auto keyOffset = text.find(quotedKey);
    if (keyOffset == std::string_view::npos) return false;
    const auto openOffset = text.find(open, keyOffset + quotedKey.size());
    if (openOffset == std::string_view::npos) return false;
    std::size_t depth = 0u;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = openOffset; i < text.size(); ++i)
    {
        const char character = text[i];
        if (inString)
        {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') inString = false;
            continue;
        }
        if (character == '"')
        {
            inString = true;
            continue;
        }
        if (character == open) ++depth;
        else if (character == close)
        {
            if (--depth == 0u)
            {
                section = text.substr(openOffset + 1u, i - openOffset - 1u);
                return true;
            }
        }
    }
    return false;
}

bool ExtractString(std::string_view text, std::string_view key, std::string& value)
{
    const std::regex expression("\\\"" + std::string(key) +
                                "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::cmatch match;
    if (!std::regex_search(text.data(), text.data() + text.size(), match, expression)) return false;
    value = match[1].str();
    return true;
}

bool ExtractNumber(std::string_view text, std::string_view key, double& value)
{
    const std::regex expression("\\\"" + std::string(key) +
                                "\\\"\\s*:\\s*([-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)");
    std::cmatch match;
    if (!std::regex_search(text.data(), text.data() + text.size(), match, expression)) return false;
    try
    {
        value = std::stod(match[1].str());
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ExtractUnsigned(std::string_view text, std::string_view key, std::uint32_t& value)
{
    double number = 0.0;
    if (!ExtractNumber(text, key, number) || !std::isfinite(number) || number < 0.0 ||
        number > static_cast<double>(std::numeric_limits<std::uint32_t>::max()) ||
        std::floor(number) != number)
    {
        return false;
    }
    value = static_cast<std::uint32_t>(number);
    return true;
}

bool ExtractBool(std::string_view text, std::string_view key, bool& value)
{
    const std::regex expression("\\\"" + std::string(key) + "\\\"\\s*:\\s*(true|false)");
    std::cmatch match;
    if (!std::regex_search(text.data(), text.data() + text.size(), match, expression)) return false;
    value = match[1].str() == "true";
    return true;
}

bool HasKey(std::string_view text, std::string_view key)
{
    return text.find("\"" + std::string(key) + "\"") != std::string_view::npos;
}

bool ExtractFloat3(std::string_view text, std::string_view key,
                   std::array<float, 3u>& values)
{
    std::string_view section;
    if (!FindDelimited(text, key, '[', ']', section)) return false;
    const std::regex number(
        "[-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?");
    std::size_t index = 0u;
    for (std::cregex_iterator it(section.data(), section.data() + section.size(), number), end;
         it != end; ++it)
    {
        if (index >= values.size()) return false;
        try
        {
            const double value = std::stod((*it)[0].str());
            if (!std::isfinite(value)) return false;
            values[index++] = static_cast<float>(value);
        }
        catch (...)
        {
            return false;
        }
    }
    return index == values.size();
}

std::vector<std::string_view> ObjectElements(std::string_view array)
{
    std::vector<std::string_view> result;
    std::size_t objectStart = std::string_view::npos;
    std::size_t depth = 0u;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = 0u; i < array.size(); ++i)
    {
        const char character = array[i];
        if (inString)
        {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') inString = false;
            continue;
        }
        if (character == '"') inString = true;
        else if (character == '{')
        {
            if (depth++ == 0u) objectStart = i;
        }
        else if (character == '}' && depth != 0u && --depth == 0u)
        {
            result.push_back(array.substr(objectStart, i - objectStart + 1u));
        }
    }
    return result;
}

std::vector<std::string> StringElements(std::string_view array)
{
    std::vector<std::string> result;
    const std::regex expression("\\\"([^\\\"]*)\\\"");
    for (std::cregex_iterator it(array.data(), array.data() + array.size(), expression), end;
         it != end; ++it)
    {
        result.push_back((*it)[1].str());
    }
    return result;
}

} // namespace

bool AssetManifest::Load(const std::filesystem::path& path,
                         AssetManifest& manifest,
                         std::string& diagnostic)
{
    manifest = {};
    std::string text;
    if (!ReadFile(path, text))
    {
        diagnostic = "Could not read asset manifest: " + path.string();
        return false;
    }

    double metresPerUnit = 0.0;
    std::string_view coordinateSystem;
    std::string_view budgets;
    std::string_view lods;
    std::string_view requiredSockets;
    std::string_view textureProfile;
    std::string_view materialOverrides;
    if (!ExtractUnsigned(text, "schema", manifest.schema) ||
        !ExtractString(text, "asset", manifest.assetName) ||
        !ExtractNumber(text, "metresPerUnit", metresPerUnit) ||
        !FindDelimited(text, "coordinateSystem", '{', '}', coordinateSystem) ||
        !FindDelimited(text, "budgets", '{', '}', budgets) ||
        !FindDelimited(text, "lods", '[', ']', lods) ||
        !FindDelimited(text, "requiredSockets", '[', ']', requiredSockets) ||
        !FindDelimited(text, "runtimeTextureProfile", '{', '}', textureProfile) ||
        !FindDelimited(text, "materialOverrides", '[', ']', materialOverrides) ||
        !ExtractString(coordinateSystem, "up", manifest.upAxis) ||
        !ExtractString(coordinateSystem, "forward", manifest.forwardAxis) ||
        !ExtractUnsigned(budgets, "maxVertices", manifest.budgets.maxVertices) ||
        !ExtractUnsigned(budgets, "maxIndices", manifest.budgets.maxIndices) ||
        !ExtractUnsigned(budgets, "maxPrimitives", manifest.budgets.maxPrimitives) ||
        !ExtractUnsigned(budgets, "maxMaterials", manifest.budgets.maxMaterials) ||
        !ExtractUnsigned(budgets, "maxTextureLayersPerKind", manifest.budgets.maxTextureLayersPerKind) ||
        !ExtractString(textureProfile, "android", manifest.textureProfile.androidEncoding) ||
        !ExtractString(textureProfile, "windows", manifest.textureProfile.windowsEncoding) ||
        !ExtractBool(textureProfile, "mipmapped", manifest.textureProfile.mipmapped))
    {
        diagnostic = "Asset manifest is missing required schema-1 fields.";
        return false;
    }
    manifest.metresPerUnit = static_cast<float>(metresPerUnit);
    manifest.requiredSockets = StringElements(requiredSockets);

    for (const std::string_view lod : ObjectElements(lods))
    {
        AssetLodBudget budget;
        if (!ExtractString(lod, "name", budget.name) ||
            !ExtractUnsigned(lod, "maxTriangles", budget.maxTriangles))
        {
            diagnostic = "Asset manifest contains an invalid LOD budget.";
            return false;
        }
        manifest.lods.push_back(std::move(budget));
    }
    for (const std::string_view sourceOverride : ObjectElements(materialOverrides))
    {
        MaterialOverride materialOverride;
        if (!ExtractString(sourceOverride, "material", materialOverride.material))
        {
            diagnostic = "Asset manifest contains an invalid material override.";
            return false;
        }
        const auto optionalNumber = [&](std::string_view key, float& destination,
                                        bool& present, double minimum,
                                        double maximum) {
            present = HasKey(sourceOverride, key);
            if (!present) return true;
            double value = 0.0;
            if (!ExtractNumber(sourceOverride, key, value) || !std::isfinite(value) ||
                value < minimum || value > maximum)
                return false;
            destination = static_cast<float>(value);
            return true;
        };
        if (!optionalNumber("emissiveStrength", materialOverride.emissiveStrength,
                            materialOverride.hasEmissiveStrength, 0.0,
                            std::numeric_limits<double>::max()) ||
            !optionalNumber("transmissionFactor", materialOverride.transmissionFactor,
                            materialOverride.hasTransmissionFactor, 0.0, 1.0) ||
            !optionalNumber("ior", materialOverride.ior,
                            materialOverride.hasIor, 1.0, 4.0) ||
            !optionalNumber("thicknessFactor", materialOverride.thicknessFactor,
                            materialOverride.hasThicknessFactor, 0.0,
                            std::numeric_limits<double>::max()) ||
            !optionalNumber("attenuationDistance", materialOverride.attenuationDistance,
                            materialOverride.hasAttenuationDistance,
                            std::numeric_limits<double>::min(),
                            std::numeric_limits<double>::max()) ||
            !optionalNumber("roughnessFactor", materialOverride.roughnessFactor,
                            materialOverride.hasRoughnessFactor, 0.0, 1.0))
        {
            diagnostic = "Asset manifest contains an invalid material override.";
            return false;
        }
        materialOverride.hasAttenuationColor = HasKey(sourceOverride, "attenuationColor");
        if (materialOverride.hasAttenuationColor &&
            (!ExtractFloat3(sourceOverride, "attenuationColor",
                            materialOverride.attenuationColor) ||
             std::any_of(materialOverride.attenuationColor.begin(),
                         materialOverride.attenuationColor.end(),
                         [](float channel) { return channel < 0.0f || channel > 1.0f; })))
        {
            diagnostic = "Asset manifest contains an invalid material override.";
            return false;
        }
        materialOverride.hasThinWall = HasKey(sourceOverride, "thinWall");
        if (materialOverride.hasThinWall &&
            !ExtractBool(sourceOverride, "thinWall", materialOverride.thinWall))
        {
            diagnostic = "Asset manifest contains an invalid material override.";
            return false;
        }
        if (!materialOverride.hasEmissiveStrength &&
            !materialOverride.hasTransmissionFactor && !materialOverride.hasIor &&
            !materialOverride.hasThicknessFactor &&
            !materialOverride.hasAttenuationDistance &&
            !materialOverride.hasAttenuationColor &&
            !materialOverride.hasRoughnessFactor && !materialOverride.hasThinWall)
        {
            diagnostic = "Asset manifest contains an invalid material override.";
            return false;
        }
        manifest.materialOverrides.push_back(std::move(materialOverride));
    }

    if (manifest.schema != 1u)
    {
        diagnostic = "Asset manifest schema must be 1.";
        return false;
    }
    if (!std::isfinite(manifest.metresPerUnit) || manifest.metresPerUnit <= 0.0f)
    {
        diagnostic = "Asset manifest metresPerUnit must be finite and greater than zero.";
        return false;
    }
    if (manifest.upAxis != "+Y" || manifest.forwardAxis != "+Z")
    {
        diagnostic = "Asset manifest coordinateSystem must be +Y up and +Z forward.";
        return false;
    }
    if (manifest.budgets.maxVertices == 0u || manifest.budgets.maxIndices == 0u ||
        manifest.budgets.maxPrimitives == 0u || manifest.budgets.maxMaterials == 0u ||
        manifest.budgets.maxTextureLayersPerKind == 0u)
    {
        diagnostic = "Asset manifest budgets must all be greater than zero.";
        return false;
    }
    if (manifest.lods.empty())
    {
        diagnostic = "Asset manifest must define at least one LOD budget.";
        return false;
    }
    for (const AssetLodBudget& lod : manifest.lods)
    {
        if (lod.maxTriangles == 0u)
        {
            diagnostic = "Asset manifest LOD '" + lod.name +
                         "' maxTriangles must be greater than zero.";
            return false;
        }
    }
    if (manifest.textureProfile.androidEncoding != "astc" ||
        manifest.textureProfile.windowsEncoding != "rgba8" ||
        !manifest.textureProfile.mipmapped)
    {
        diagnostic = "Asset manifest runtimeTextureProfile must be Android ASTC, Windows RGBA8, and mipmapped.";
        return false;
    }
    diagnostic.clear();
    return true;
}

} // namespace horde::scene::assets
